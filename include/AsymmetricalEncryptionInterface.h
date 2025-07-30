#ifndef ASYMMETRICAL_ENCRYPTION_INTERFACE_H
#define ASYMMETRICAL_ENCRYPTION_INTERFACE_H

#include <string>

class AsymmetricalEncryptionInterface {
public:
    virtual ~AsymmetricalEncryptionInterface() = default;
    
    // 生成密钥对
    virtual bool generateKeyPair() = 0;
    
    // 获取本地公钥（base64编码）
    virtual std::string getLocalPublicKey() = 0;
    
    // 设置远程公钥（base64编码）
    virtual bool setRemotePublicKey(const std::string& publicKey) = 0;
    
    // 使用本地私钥加密数据
    virtual std::string encryptWithLocalPrivate(const std::string& plaintext) = 0;
    
    // 使用本地私钥解密数据
    virtual std::string decryptWithLocalPrivate(const std::string& ciphertext) = 0;
    
    // 使用远程公钥加密数据
    virtual std::string encryptWithRemotePublic(const std::string& plaintext) = 0;
    
    // 使用远程公钥解密数据
    virtual std::string decryptWithRemotePublic(const std::string& ciphertext) = 0;
    
    // 使用本地私钥对数据进行签名
    virtual std::string signWithLocalPrivate(const std::string& data) = 0;
    
    // 使用远程公钥验证签名
    virtual bool verifyWithRemotePublic(const std::string& data, const std::string& signature) = 0;
};

#endif // ASYMMETRICAL_ENCRYPTION_INTERFACE_H
