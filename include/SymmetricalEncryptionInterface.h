#ifndef SYMMETRICAL_ENCRYPTION_INTERFACE_H
#define SYMMETRICAL_ENCRYPTION_INTERFACE_H

#include <string>

class SymmetricalEncryptionInterface {
public:
    virtual ~SymmetricalEncryptionInterface() = default;
    
    // 生成base64编码的密钥
    virtual bool generateRawKey() = 0;
    
    // 使用base64编码的本地密钥加密数据
    virtual std::string encryptWithLocal(const std::string& plaintext) = 0;
    
    // 使用base64编码的本地密钥解密数据
    virtual std::string decryptWithLocal(const std::string& ciphertext) = 0;
    
    // 使用base64编码的远程密钥加密数据
    virtual std::string encryptWithRemote(const std::string& plaintext) = 0;
    
    // 使用base64编码的远程密钥解密数据
    virtual std::string decryptWithRemote(const std::string& ciphertext) = 0;
    
    // 设置远程发来的会话密钥
    virtual bool setRemotePublicKey(const std::string& keyString, const std::string& iv) = 0;
    
    // 获取本地密钥
    virtual std::string getLocalKey() = 0;
};

#endif // SYMMETRICAL_ENCRYPTION_INTERFACE_H
