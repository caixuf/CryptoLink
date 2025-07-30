#include <iostream>
#include <cassert>
#include "RSAKey.h"
#include "AESKey.h"

void testRSAEncryption() {
    std::cout << "测试 RSA 加密/解密..." << std::endl;
    
    RSAKey rsa1, rsa2;
    
    // 生成密钥对
    assert(rsa1.generateKeyPair());
    assert(rsa2.generateKeyPair());
    
    // 交换公钥
    std::string publicKey1 = rsa1.getLocalPublicKey();
    std::string publicKey2 = rsa2.getLocalPublicKey();
    
    assert(!publicKey1.empty());
    assert(!publicKey2.empty());
    
    assert(rsa1.setRemotePublicKey(publicKey2));
    assert(rsa2.setRemotePublicKey(publicKey1));
    
    // 测试加密/解密
    std::string plaintext = "Hello, RSA World!";
    std::string encrypted = rsa1.encryptWithRemotePublic(plaintext);
    std::string decrypted = rsa2.decryptWithLocalPrivate(encrypted);
    
    assert(!encrypted.empty());
    assert(decrypted == plaintext);
    
    std::cout << "RSA 测试通过！" << std::endl;
}

void testAESEncryption() {
    std::cout << "测试 AES 加密/解密..." << std::endl;
    
    AESKey aes1, aes2;
    
    // 生成密钥
    assert(aes1.generateRawKey());
    assert(aes2.generateRawKey());
    
    // 模拟密钥交换
    std::string key1 = aes1.getLocalKey();
    assert(!key1.empty());
    
    // 解析密钥格式 key:iv
    size_t colonPos = key1.find(':');
    assert(colonPos != std::string::npos);
    
    std::string keyPart = key1.substr(0, colonPos);
    std::string ivPart = key1.substr(colonPos + 1);
    
    assert(aes2.setRemotePublicKey(keyPart, ivPart));
    
    // 测试加密/解密
    std::string plaintext = "Hello, AES World! 这是一个测试消息。";
    std::string encrypted = aes1.encryptWithLocal(plaintext);
    std::string decrypted = aes2.decryptWithRemote(encrypted);
    
    assert(!encrypted.empty());
    assert(decrypted == plaintext);
    
    std::cout << "AES 测试通过！" << std::endl;
}

void testRSASignature() {
    std::cout << "测试 RSA 数字签名..." << std::endl;
    
    RSAKey rsa1, rsa2;
    
    // 生成密钥对并交换公钥
    assert(rsa1.generateKeyPair());
    assert(rsa2.generateKeyPair());
    
    std::string publicKey1 = rsa1.getLocalPublicKey();
    std::string publicKey2 = rsa2.getLocalPublicKey();
    
    assert(rsa1.setRemotePublicKey(publicKey2));
    assert(rsa2.setRemotePublicKey(publicKey1));
    
    // 测试签名和验证
    std::string data = "Important message to sign";
    std::string signature = rsa1.signWithLocalPrivate(data);
    
    assert(!signature.empty());
    assert(rsa2.verifyWithRemotePublic(data, signature));
    
    // 测试篡改检测
    std::string tamperedData = "Tampered message";
    assert(!rsa2.verifyWithRemotePublic(tamperedData, signature));
    
    std::cout << "RSA 签名测试通过！" << std::endl;
}

int main() {
    std::cout << "=== CryptoLink 加密功能测试 ===" << std::endl;
    
    try {
        testAESEncryption();
        testRSAEncryption();
        testRSASignature();
        
        std::cout << "\\n所有测试通过！加密库工作正常。" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "测试失败: " << e.what() << std::endl;
        return -1;
    }
}
