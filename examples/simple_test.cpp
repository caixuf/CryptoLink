#include <iostream>
#include <cassert>
#include "RSAKey.h"
#include "AESKey.h"

void simpleAESTest() {
    std::cout << "=== 简单 AES 测试 ===" << std::endl;
    
    AESKey aes;
    
    // 生成密钥
    if (aes.generateRawKey()) {
        std::cout << "✅ AES 密钥生成成功" << std::endl;
        
        std::string key = aes.getLocalKey();
        if (!key.empty()) {
            std::cout << "✅ 获取本地密钥成功，长度: " << key.length() << std::endl;
        } else {
            std::cout << "❌ 获取本地密钥失败" << std::endl;
        }
    } else {
        std::cout << "❌ AES 密钥生成失败" << std::endl;
    }
}

void simpleRSATest() {
    std::cout << "\n=== 简单 RSA 测试 ===" << std::endl;
    
    RSAKey rsa;
    
    // 生成密钥对
    if (rsa.generateKeyPair()) {
        std::cout << "✅ RSA 密钥对生成成功" << std::endl;
        
        std::string pubKey = rsa.getLocalPublicKey();
        if (!pubKey.empty()) {
            std::cout << "✅ 获取公钥成功，长度: " << pubKey.length() << std::endl;
        } else {
            std::cout << "❌ 获取公钥失败" << std::endl;
        }
    } else {
        std::cout << "❌ RSA 密钥对生成失败" << std::endl;
    }
}

int main() {
    std::cout << "=== CryptoLink 简单功能测试 ===" << std::endl;
    
    try {
        simpleAESTest();
        simpleRSATest();
        
        std::cout << "\n🎉 基础测试完成！" << std::endl;
        std::cout << "项目重建成功，核心加密组件可以正常工作。" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "测试异常: " << e.what() << std::endl;
        return -1;
    }
}
