#ifndef CRYPTO_WEBSOCKET_SERVER_H
#define CRYPTO_WEBSOCKET_SERVER_H

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <memory>
#include <functional>
#include <thread>
#include <map>
#include "RSAKey.h"
#include "AESKey.h"

typedef websocketpp::server<websocketpp::config::asio> server;
typedef server::message_ptr message_ptr;

class CryptoWebSocketServer {
public:
    CryptoWebSocketServer();
    ~CryptoWebSocketServer();
    
    // 启动服务器
    bool start(uint16_t port);
    
    // 停止服务器
    void stop();
    
    // 广播加密消息给所有连接的客户端
    void broadcastEncryptedMessage(const std::string& message);
    
    // 发送加密消息给特定客户端
    bool sendEncryptedMessage(websocketpp::connection_hdl hdl, const std::string& message);
    
    // 设置消息接收回调
    void setMessageCallback(std::function<void(websocketpp::connection_hdl, const std::string&)> callback);
    
    // 运行服务器
    void run();

private:
    server wsServer;
    std::map<websocketpp::connection_hdl, std::unique_ptr<RSAKey>, std::owner_less<websocketpp::connection_hdl>> clientRSAKeys;
    std::map<websocketpp::connection_hdl, std::unique_ptr<AESKey>, std::owner_less<websocketpp::connection_hdl>> clientAESKeys;
    std::map<websocketpp::connection_hdl, bool, std::owner_less<websocketpp::connection_hdl>> handshakeStatus;
    
    std::unique_ptr<RSAKey> serverRSAKey;
    std::function<void(websocketpp::connection_hdl, const std::string&)> messageCallback;
    std::thread serverThread;
    bool isRunning;
    
    // WebSocket事件处理
    void onOpen(websocketpp::connection_hdl hdl);
    void onClose(websocketpp::connection_hdl hdl);
    void onMessage(websocketpp::connection_hdl hdl, message_ptr msg);
    
    // 加密握手过程
    void handleHandshakeMessage(websocketpp::connection_hdl hdl, const std::string& message);
    void initializeClientCrypto(websocketpp::connection_hdl hdl);
    
    // 消息类型
    enum MessageType {
        PUBLIC_KEY_REQUEST = 1,
        PUBLIC_KEY_RESPONSE = 2,
        SESSION_KEY = 3,
        ENCRYPTED_DATA = 4
    };
    
    struct Message {
        MessageType type;
        std::string data;
    };
    
    std::string serializeMessage(const Message& msg);
    Message parseMessage(const std::string& data);
};

#endif // CRYPTO_WEBSOCKET_SERVER_H
