#ifndef CRYPTO_WEBSOCKET_CLIENT_H
#define CRYPTO_WEBSOCKET_CLIENT_H

#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
#include <memory>
#include <functional>
#include <thread>
#include "RSAKey.h"
#include "AESKey.h"

typedef websocketpp::client<websocketpp::config::asio_client> client;
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

class CryptoWebSocketClient {
public:
    CryptoWebSocketClient();
    ~CryptoWebSocketClient();
    
    // 连接到服务器
    bool connect(const std::string& uri);
    
    // 断开连接
    void disconnect();
    
    // 发送加密消息
    bool sendEncryptedMessage(const std::string& message);
    
    // 设置消息接收回调
    void setMessageCallback(std::function<void(const std::string&)> callback);
    
    // 运行客户端
    void run();
    
    // 停止客户端
    void stop();

private:
    client wsClient;
    websocketpp::connection_hdl connectionHandle;
    std::unique_ptr<RSAKey> rsaKey;
    std::unique_ptr<AESKey> aesKey;
    std::function<void(const std::string&)> messageCallback;
    std::thread clientThread;
    bool isConnected;
    bool handshakeComplete;
    
    // WebSocket事件处理
    void onOpen(websocketpp::connection_hdl hdl);
    void onClose(websocketpp::connection_hdl hdl);
    void onMessage(websocketpp::connection_hdl hdl, message_ptr msg);
    void onFail(websocketpp::connection_hdl hdl);
    
    // 加密握手过程
    void performHandshake();
    void handleHandshakeMessage(const std::string& message);
    
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

#endif // CRYPTO_WEBSOCKET_CLIENT_H
