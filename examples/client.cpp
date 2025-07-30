#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include "CryptoWebSocketClient.h"

int main() {
    std::cout << "=== CryptoLink 客户端 ===" << std::endl;
    
    CryptoWebSocketClient client;
    
    // 设置消息接收回调
    client.setMessageCallback([](const std::string& message) {
        std::cout << "收到解密消息: " << message << std::endl;
    });
    
    // 连接到服务器
    std::string serverUri = "ws://localhost:9003";  // 更新端口号
    std::cout << "连接到服务器: " << serverUri << std::endl;
    
    if (!client.connect(serverUri)) {
        std::cerr << "连接失败！" << std::endl;
        return -1;
    }
    
    // 运行客户端
    client.run();
    
    // 等待连接建立和握手完成
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // 发送测试消息
    std::cout << "开始发送加密消息..." << std::endl;
    
    for (int i = 1; i <= 5; ++i) {
        std::string message = "客户端消息 #" + std::to_string(i);
        std::cout << "发送: " << message << std::endl;
        
        if (!client.sendEncryptedMessage(message)) {
            std::cerr << "发送消息失败！" << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    // 交互式模式
    std::cout << "\\n进入交互模式，输入消息发送（输入 'quit' 退出）:" << std::endl;
    std::string input;
    while (std::getline(std::cin, input)) {
        if (input == "quit") {
            break;
        }
        
        if (!input.empty()) {
            client.sendEncryptedMessage(input);
        }
    }
    
    std::cout << "断开连接..." << std::endl;
    client.disconnect();
    client.stop();
    
    return 0;
}
