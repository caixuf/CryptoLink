#include "CryptoWebSocketClient.h"
#include <iostream>
#include <jsoncpp/json/json.h>

CryptoWebSocketClient::CryptoWebSocketClient() 
    : isConnected(false), handshakeComplete(false) {
    
    // 初始化加密对象
    rsaKey = std::make_unique<RSAKey>();
    aesKey = std::make_unique<AESKey>();
    
    // 生成密钥对
    rsaKey->generateKeyPair();
    aesKey->generateRawKey();
    
    // 配置WebSocket客户端
    wsClient.set_access_channels(websocketpp::log::alevel::all);
    wsClient.clear_access_channels(websocketpp::log::alevel::frame_payload);
    wsClient.init_asio();
    
    // 设置回调函数
    wsClient.set_open_handler([this](websocketpp::connection_hdl hdl) {
        this->onOpen(hdl);
    });
    
    wsClient.set_close_handler([this](websocketpp::connection_hdl hdl) {
        this->onClose(hdl);
    });
    
    wsClient.set_message_handler([this](websocketpp::connection_hdl hdl, message_ptr msg) {
        this->onMessage(hdl, msg);
    });
    
    wsClient.set_fail_handler([this](websocketpp::connection_hdl hdl) {
        this->onFail(hdl);
    });
}

CryptoWebSocketClient::~CryptoWebSocketClient() {
    stop();
}

bool CryptoWebSocketClient::connect(const std::string& uri) {
    try {
        websocketpp::lib::error_code ec;
        client::connection_ptr con = wsClient.get_connection(uri, ec);
        
        if (ec) {
            std::cerr << "连接创建失败: " << ec.message() << std::endl;
            return false;
        }
        
        connectionHandle = con->get_handle();
        wsClient.connect(con);
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "连接异常: " << e.what() << std::endl;
        return false;
    }
}

void CryptoWebSocketClient::disconnect() {
    if (isConnected) {
        wsClient.close(connectionHandle, websocketpp::close::status::normal, "Client disconnect");
        isConnected = false;
        handshakeComplete = false;
    }
}

bool CryptoWebSocketClient::sendEncryptedMessage(const std::string& message) {
    if (!isConnected || !handshakeComplete) {
        std::cerr << "客户端未连接或握手未完成" << std::endl;
        return false;
    }
    
    try {
        // 使用AES会话密钥加密消息
        std::string encryptedData = aesKey->encryptWithLocal(message);
        
        Message msg = {ENCRYPTED_DATA, encryptedData};
        std::string serialized = serializeMessage(msg);
        
        websocketpp::lib::error_code ec;
        wsClient.send(connectionHandle, serialized, websocketpp::frame::opcode::text, ec);
        
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

void CryptoWebSocketClient::setMessageCallback(std::function<void(const std::string&)> callback) {
    messageCallback = callback;
}

void CryptoWebSocketClient::run() {
    clientThread = std::thread([this]() {
        wsClient.run();
    });
}

void CryptoWebSocketClient::stop() {
    wsClient.stop();
    if (clientThread.joinable()) {
        clientThread.join();
    }
}

void CryptoWebSocketClient::onOpen(websocketpp::connection_hdl hdl) {
    std::cout << "连接已建立，开始握手..." << std::endl;
    isConnected = true;
    performHandshake();
}

void CryptoWebSocketClient::onClose(websocketpp::connection_hdl hdl) {
    std::cout << "连接已关闭" << std::endl;
    isConnected = false;
    handshakeComplete = false;
}

void CryptoWebSocketClient::onMessage(websocketpp::connection_hdl hdl, message_ptr msg) {
    std::string payload = msg->get_payload();
    
    if (!handshakeComplete) {
        handleHandshakeMessage(payload);
    } else {
        // 处理加密消息
        Message parsedMsg = parseMessage(payload);
        if (parsedMsg.type == ENCRYPTED_DATA) {
            std::string decryptedData = aesKey->decryptWithLocal(parsedMsg.data);
            if (messageCallback) {
                messageCallback(decryptedData);
            }
        }
    }
}

void CryptoWebSocketClient::onFail(websocketpp::connection_hdl hdl) {
    std::cerr << "连接失败" << std::endl;
    isConnected = false;
}

void CryptoWebSocketClient::performHandshake() {
    // 发送公钥请求
    Message msg = {PUBLIC_KEY_REQUEST, ""};
    std::string serialized = serializeMessage(msg);
    
    websocketpp::lib::error_code ec;
    wsClient.send(connectionHandle, serialized, websocketpp::frame::opcode::text, ec);
    
    if (ec) {
        std::cerr << "发送公钥请求失败: " << ec.message() << std::endl;
    }
}

void CryptoWebSocketClient::handleHandshakeMessage(const std::string& message) {
    Message msg = parseMessage(message);
    
    switch (msg.type) {
        case PUBLIC_KEY_RESPONSE: {
            // 设置服务器公钥
            rsaKey->setRemotePublicKey(msg.data);
            
            // 发送客户端公钥
            Message response = {PUBLIC_KEY_RESPONSE, rsaKey->getLocalPublicKey()};
            std::string serialized = serializeMessage(response);
            
            websocketpp::lib::error_code ec;
            wsClient.send(connectionHandle, serialized, websocketpp::frame::opcode::text, ec);
            
            // 发送会话密钥（用服务器公钥加密）
            std::string sessionKey = aesKey->getLocalKey();
            std::string encryptedSessionKey = rsaKey->encryptWithRemotePublic(sessionKey);
            
            Message sessionMsg = {SESSION_KEY, encryptedSessionKey};
            std::string sessionSerialized = serializeMessage(sessionMsg);
            wsClient.send(connectionHandle, sessionSerialized, websocketpp::frame::opcode::text, ec);
            
            handshakeComplete = true;
            std::cout << "握手完成！" << std::endl;
            break;
        }
        default:
            break;
    }
}

std::string CryptoWebSocketClient::serializeMessage(const Message& msg) {
    Json::Value root;
    root["type"] = static_cast<int>(msg.type);
    root["data"] = msg.data;
    
    Json::StreamWriterBuilder builder;
    return Json::writeString(builder, root);
}

CryptoWebSocketClient::Message CryptoWebSocketClient::parseMessage(const std::string& data) {
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
