#include "CryptoWebSocketServer.h"
#include "CryptoError.h"
#include <iostream>
#include <memory>
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
    wsServer.set_open_handler([this](websocketpp::connection_hdl hdl) { this->onOpen(hdl); });

    wsServer.set_close_handler([this](websocketpp::connection_hdl hdl) { this->onClose(hdl); });

    wsServer.set_message_handler(
        [this](websocketpp::connection_hdl hdl, message_ptr msg) { this->onMessage(hdl, msg); });
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

std::shared_ptr<CryptoWebSocketServer::ClientSession> CryptoWebSocketServer::getSession(
    websocketpp::connection_hdl hdl) {
    std::lock_guard<std::mutex> lock(sessionsMutex);
    auto it = sessions.find(hdl);
    return it == sessions.end() ? nullptr : it->second;
}

void CryptoWebSocketServer::broadcastEncryptedMessage(const std::string& message) {
    // 先在锁内复制当前会话句柄，避免持锁期间发送造成的锁竞争/重入
    std::vector<websocketpp::connection_hdl> targets;
    {
        std::lock_guard<std::mutex> lock(sessionsMutex);
        for (auto& pair : sessions) {
            if (pair.second->handshakeComplete) {
                targets.push_back(pair.first);
            }
        }
    }
    for (auto& hdl : targets) {
        sendEncryptedMessage(hdl, message);
    }
}

bool CryptoWebSocketServer::sendEncryptedMessage(websocketpp::connection_hdl hdl,
                                                 const std::string& message) {
    auto session = getSession(hdl);
    if (!session || !session->handshakeComplete) {
        std::cerr << "客户端未找到或握手未完成" << std::endl;
        return false;
    }

    try {
        // 使用客户端的AES会话密钥加密消息
        std::string encryptedData = session->aesKey->encryptWithRemote(message);

        Message msg = {ENCRYPTED_DATA, encryptedData, ""};
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

void CryptoWebSocketServer::setMessageCallback(
    std::function<void(websocketpp::connection_hdl, const std::string&)> callback) {
    messageCallback = callback;
}

void CryptoWebSocketServer::run() {
    serverThread = std::thread([this]() { wsServer.run(); });
}

void CryptoWebSocketServer::onOpen(websocketpp::connection_hdl hdl) {
    std::cout << "新客户端连接" << std::endl;
    initializeClientCrypto(hdl);
}

void CryptoWebSocketServer::onClose(websocketpp::connection_hdl hdl) {
    std::cout << "客户端断开连接" << std::endl;

    // 清理客户端相关的会话（密钥在此被销毁）
    std::lock_guard<std::mutex> lock(sessionsMutex);
    sessions.erase(hdl);
}

void CryptoWebSocketServer::onMessage(websocketpp::connection_hdl hdl, message_ptr msg) {
    std::string payload = msg->get_payload();

    auto session = getSession(hdl);
    if (!session || !session->handshakeComplete) {
        handleHandshakeMessage(hdl, payload);
        return;
    }

    // 处理加密消息
    Message parsedMsg = parseMessage(payload);
    if (parsedMsg.type == ENCRYPTED_DATA) {
        try {
            std::string decryptedData = session->aesKey->decryptWithRemote(parsedMsg.data);
            if (messageCallback) {
                messageCallback(hdl, decryptedData);
            }
        } catch (const CryptoError& e) {
            // 解密失败（数据可能被篡改）——不再当成空消息静默处理
            std::cerr << "解密来自客户端的消息失败: " << e.what() << std::endl;
        }
    }
}

void CryptoWebSocketServer::handleHandshakeMessage(websocketpp::connection_hdl hdl,
                                                   const std::string& message) {
    Message msg = parseMessage(message);
    auto session = getSession(hdl);
    if (!session) {
        return;
    }

    switch (msg.type) {
        case PUBLIC_KEY_REQUEST: {
            // 响应公钥请求
            Message response = {PUBLIC_KEY_RESPONSE, serverRSAKey->getLocalPublicKey(), ""};
            std::string serialized = serializeMessage(response);

            websocketpp::lib::error_code ec;
            wsServer.send(hdl, serialized, websocketpp::frame::opcode::text, ec);
            break;
        }
        case PUBLIC_KEY_RESPONSE: {
            // 设置客户端公钥
            session->rsaKey->setRemotePublicKey(msg.data);
            break;
        }
        case SESSION_KEY: {
            try {
                // 解密会话密钥（RSA-OAEP）
                std::string decryptedSessionKey = serverRSAKey->decryptWithLocalPrivate(msg.data);

                // 验证会话密钥来源：用客户端公钥校验其对会话密钥的签名，
                // 防止中间人替换会话密钥（要求已收到客户端公钥）。
                if (!session->rsaKey->verifyWithRemotePublic(decryptedSessionKey, msg.signature)) {
                    std::cerr << "会话密钥签名校验失败，拒绝握手" << std::endl;
                    break;
                }

                // 解析会话密钥（格式：key:iv）
                size_t colonPos = decryptedSessionKey.find(':');
                if (colonPos != std::string::npos) {
                    std::string key = decryptedSessionKey.substr(0, colonPos);
                    std::string iv = decryptedSessionKey.substr(colonPos + 1);

                    session->aesKey->setRemotePublicKey(key, iv);
                    session->handshakeComplete = true;
                    std::cout << "客户端握手完成！" << std::endl;
                }
            } catch (const CryptoError& e) {
                std::cerr << "处理会话密钥失败: " << e.what() << std::endl;
            }
            break;
        }
        default:
            break;
    }
}

void CryptoWebSocketServer::initializeClientCrypto(websocketpp::connection_hdl hdl) {
    // 为新客户端创建会话与密钥对象
    auto session = std::make_shared<ClientSession>();
    session->rsaKey = std::make_unique<RSAKey>();
    session->aesKey = std::make_unique<AESKey>();
    session->handshakeComplete = false;

    session->rsaKey->generateKeyPair();
    session->aesKey->generateRawKey();

    std::lock_guard<std::mutex> lock(sessionsMutex);
    sessions[hdl] = std::move(session);
}

std::string CryptoWebSocketServer::serializeMessage(const Message& msg) {
    Json::Value root;
    root["type"] = static_cast<int>(msg.type);
    root["data"] = msg.data;
    if (!msg.signature.empty()) {
        root["sig"] = msg.signature;
    }

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, root);
}

CryptoWebSocketServer::Message CryptoWebSocketServer::parseMessage(const std::string& data) {
    Json::Value root;
    Json::CharReaderBuilder builder;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());

    Message msg{};
    std::string errors;
    bool success = reader->parse(data.c_str(), data.c_str() + data.length(), &root, &errors);
    if (success) {
        msg.type = static_cast<MessageType>(root["type"].asInt());
        msg.data = root["data"].asString();
        msg.signature = root.get("sig", "").asString();
    }

    return msg;
}
