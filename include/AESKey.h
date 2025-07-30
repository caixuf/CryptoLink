#ifndef AES_KEY_H
#define AES_KEY_H

#include "SymmetricalEncryptionInterface.h"
#include <cryptopp/aes.h>
#include <cryptopp/modes.h>
#include <cryptopp/osrng.h>
#include <cryptopp/base64.h>

using namespace CryptoPP;

class AESKey : public SymmetricalEncryptionInterface {
public:
    AESKey();
    ~AESKey();
    
    bool generateRawKey() override;
    std::string encryptWithLocal(const std::string& plaintext) override;
    std::string decryptWithLocal(const std::string& ciphertext) override;
    std::string encryptWithRemote(const std::string& plaintext) override;
    std::string decryptWithRemote(const std::string& ciphertext) override;
    bool setRemotePublicKey(const std::string& keyString, const std::string& iv) override;
    std::string getLocalKey() override;

private:
    std::string localKey;
    std::string localIV;
    std::string remoteKey;
    std::string remoteIV;
    
    AutoSeededRandomPool rng;
    
    // 辅助函数：将密钥转换为字符串格式（Base64）
    std::string keyToString() const;
    
    // 辅助函数：从字符串格式加载密钥（Base64）
    bool stringToKey() const;
    
    // 辅助函数：Base64编码
    std::string base64Encode(const std::string& data) const;
    
    // 辅助函数：Base64解码
    std::string base64Decode(const std::string& data) const;
    
    // 辅助函数：AES加密
    std::string aesEncrypt(const std::string& plaintext, const std::string& key, const std::string& iv) const;
    
    // 辅助函数：AES解密
    std::string aesDecrypt(const std::string& ciphertext, const std::string& key, const std::string& iv) const;
};

#endif // AES_KEY_H
