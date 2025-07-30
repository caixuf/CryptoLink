#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include "CryptoWebSocketServer.h"

int main() {
    std::cout << "=== CryptoLink 服务端 ===" << std::endl;
    
    CryptoWebSocketServer server;
    
    // 设置消息接收回调
    server.setMessageCallback([&server](websocketpp::connection_hdl hdl, const std::string& message) {
        std::cout << "收到客户端解密消息: " << message << std::endl;
        
        // 回显消息
        std::string response = "服务端回复: " + message;
        server.sendEncryptedMessage(hdl, response);
    });
    
    // 启动服务器
    uint16_t port = 9003;  // 更改端口号避免冲突
    if (!server.start(port)) {
        std::cerr << "服务器启动失败！" << std::endl;
        return -1;
    }
    
    // 运行服务器
    server.run();
    
    std::cout << "服务器正在运行，按 Enter 键广播测试消息，输入 'quit' 退出..." << std::endl;
    
    // 交互式模式
    std::string input;
    int messageCount = 1;
    
    while (std::getline(std::cin, input)) {
        if (input == "quit") {
            break;
        }
        
        if (input.empty()) {
            // 按Enter键广播消息
            std::string broadcastMsg = "服务端广播消息 #" + std::to_string(messageCount++);
            std::cout << "广播: " << broadcastMsg << std::endl;
            server.broadcastEncryptedMessage(broadcastMsg);
        } else {
            // 输入消息广播
            std::cout << "广播: " << input << std::endl;
            server.broadcastEncryptedMessage(input);
        }
    }
    
    std::cout << "关闭服务器..." << std::endl;
    server.stop();
    
    return 0;
}
