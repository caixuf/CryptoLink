#include "CryptoWebSocketServer.h"
#include <iostream>
#include <jsoncpp/json/json.h>

CryptoWebSocketServer::CryptoWebSocketServer() : isRunning(false) {
    // 初始化服务器RSA密钥
    serverRSAKey = std::make_unique<RSAKey>();
    serverRSAKey->generateKeyPair();
    
    // 配置WebSocket服务器
    wsServer.set_access_channels(websocketpp::log::alevel::all);
    wsServer.clear_access_channels(websocketpp::log::alevel::frame_payload);
    wsServer.init_asio();
    wsServer.set_reuse_addr(true);
    
    // 设置回调函数
    wsServer.set_open_handler([this](websocketpp::connection_hdl hdl) {
        this->onOpen(hdl);
    });
    
    wsServer.set_close_handler([this](websocketpp::connection_hdl hdl) {
        this->onClose(hdl);
    });
    
    wsServer.set_message_handler([this](websocketpp::connection_hdl hdl, message_ptr msg) {
        this->onMessage(hdl, msg);
    });
}

CryptoWebSocketServer::~CryptoWebSocketServer() {
    stop();
}

bool CryptoWebSocketServer::start(uint16_t port) {
    try {
        wsServer.listen(port);
        wsServer.start_accept();
        
        isRunning = true;
        std::cout << "服务器启动在端口: " << port << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "服务器启动失败: " << e.what() << std::endl;
        return false;
    }
}

void CryptoWebSocketServer::stop() {
    if (isRunning) {
        wsServer.stop();
        isRunning = false;
        
        if (serverThread.joinable()) {
            serverThread.join();
        }
    }
}

void CryptoWebSocketServer::broadcastEncryptedMessage(const std::string& message) {
    for (auto& pair : clientAESKeys) {
        if (handshakeStatus[pair.first]) {
            sendEncryptedMessage(pair.first, message);
        }
    }
}

bool CryptoWebSocketServer::sendEncryptedMessage(websocketpp::connection_hdl hdl, const std::string& message) {
    auto it = clientAESKeys.find(hdl);
    auto statusIt = handshakeStatus.find(hdl);
    
    if (it == clientAESKeys.end() || statusIt == handshakeStatus.end() || !statusIt->second) {
        std::cerr << "客户端未找到或握手未完成" << std::endl;
        return false;
    }
    
    try {
        // 使用客户端的AES会话密钥加密消息
        std::string encryptedData = it->second->encryptWithRemote(message);
        
        Message msg = {ENCRYPTED_DATA, encryptedData};
        std::string serialized = serializeMessage(msg);
        
        websocketpp::lib::error_code ec;
        wsServer.send(hdl, serialized, websocketpp::frame::opcode::text, ec);
        
        if (ec) {
            std::cerr << "发送消息失败: " << ec.message() << std::endl;
            return false;
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "发送加密消息异常: " << e.what() << std::endl;
        return false;
    }
}

void CryptoWebSocketServer::setMessageCallback(std::function<void(websocketpp::connection_hdl, const std::string&)> callback) {
    messageCallback = callback;
}

void CryptoWebSocketServer::run() {
    serverThread = std::thread([this]() {
        wsServer.run();
    });
}

void CryptoWebSocketServer::onOpen(websocketpp::connection_hdl hdl) {
    std::cout << "新客户端连接" << std::endl;
    initializeClientCrypto(hdl);
}

void CryptoWebSocketServer::onClose(websocketpp::connection_hdl hdl) {
    std::cout << "客户端断开连接" << std::endl;
    
    // 清理客户端相关的加密对象
    clientRSAKeys.erase(hdl);
    clientAESKeys.erase(hdl);
    handshakeStatus.erase(hdl);
}

void CryptoWebSocketServer::onMessage(websocketpp::connection_hdl hdl, message_ptr msg) {
    std::string payload = msg->get_payload();
    
    auto statusIt = handshakeStatus.find(hdl);
    if (statusIt == handshakeStatus.end() || !statusIt->second) {
        handleHandshakeMessage(hdl, payload);
    } else {
        // 处理加密消息
        Message parsedMsg = parseMessage(payload);
        if (parsedMsg.type == ENCRYPTED_DATA) {
            auto it = clientAESKeys.find(hdl);
            if (it != clientAESKeys.end()) {
                std::string decryptedData = it->second->decryptWithRemote(parsedMsg.data);
                if (messageCallback) {
                    messageCallback(hdl, decryptedData);
                }
            }
        }
    }
}

void CryptoWebSocketServer::handleHandshakeMessage(websocketpp::connection_hdl hdl, const std::string& message) {
    Message msg = parseMessage(message);
    
    switch (msg.type) {
        case PUBLIC_KEY_REQUEST: {
            // 响应公钥请求
            Message response = {PUBLIC_KEY_RESPONSE, serverRSAKey->getLocalPublicKey()};
            std::string serialized = serializeMessage(response);
            
            websocketpp::lib::error_code ec;
            wsServer.send(hdl, serialized, websocketpp::frame::opcode::text, ec);
            break;
        }
        case PUBLIC_KEY_RESPONSE: {
            // 设置客户端公钥
            auto it = clientRSAKeys.find(hdl);
            if (it != clientRSAKeys.end()) {
                it->second->setRemotePublicKey(msg.data);
            }
            break;
        }
        case SESSION_KEY: {
            // 解密会话密钥
            std::string decryptedSessionKey = serverRSAKey->decryptWithLocalPrivate(msg.data);
            
            // 解析会话密钥（格式：key:iv）
            size_t colonPos = decryptedSessionKey.find(':');
            if (colonPos != std::string::npos) {
                std::string key = decryptedSessionKey.substr(0, colonPos);
                std::string iv = decryptedSessionKey.substr(colonPos + 1);
                
                auto it = clientAESKeys.find(hdl);
                if (it != clientAESKeys.end()) {
                    it->second->setRemotePublicKey(key, iv);
                    handshakeStatus[hdl] = true;
                    std::cout << "客户端握手完成！" << std::endl;
                }
            }
            break;
        }
        default:
            break;
    }
}

void CryptoWebSocketServer::initializeClientCrypto(websocketpp::connection_hdl hdl) {
    // 为新客户端创建RSA和AES对象
    clientRSAKeys[hdl] = std::make_unique<RSAKey>();
    clientAESKeys[hdl] = std::make_unique<AESKey>();
    handshakeStatus[hdl] = false;
    
    // 生成密钥
    clientRSAKeys[hdl]->generateKeyPair();
    clientAESKeys[hdl]->generateRawKey();
}

std::string CryptoWebSocketServer::serializeMessage(const Message& msg) {
    Json::Value root;
    root["type"] = static_cast<int>(msg.type);
    root["data"] = msg.data;
    
    Json::StreamWriterBuilder builder;
    return Json::writeString(builder, root);
}

CryptoWebSocketServer::Message CryptoWebSocketServer::parseMessage(const std::string& data) {
    Json::Value root;
    Json::CharReaderBuilder builder;
    Json::CharReader* reader = builder.newCharReader();
    
    std::string errors;
    bool success = reader->parse(data.c_str(), data.c_str() + data.length(), &root, &errors);
    delete reader;
    
    Message msg;
    if (success) {
        msg.type = static_cast<MessageType>(root["type"].asInt());
        msg.data = root["data"].asString();
    }
    
    return msg;
}
