#include <iostream>
#include <thread>
#include <chrono>

// 简单的网络测试 - 验证端口连接性
int main() {
    std::cout << "=== 网络连接测试 ===" << std::endl;
    
    // 使用系统命令测试端口连接
    std::cout << "测试端口 9003 连接性..." << std::endl;
    
    int result = system("timeout 1s bash -c 'echo test | nc -w1 localhost 9003' >/dev/null 2>&1");
    
    if (result == 0) {
        std::cout << "✅ 端口 9003 可以连接" << std::endl;
    } else {
        std::cout << "⚠️  端口 9003 暂时不可连接（这是正常的，因为服务端未运行）" << std::endl;
    }
    
    std::cout << "\n🔧 要进行完整测试，请按以下步骤：" << std::endl;
    std::cout << "1. 在一个终端运行: ./server" << std::endl;
    std::cout << "2. 在另一个终端运行: ./client" << std::endl;
    std::cout << "3. 观察加密握手和消息传输过程" << std::endl;
    
    return 0;
}
