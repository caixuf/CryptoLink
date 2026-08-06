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
    std::string localKey;  // Base64 编码的本地会话密钥（32 字节）
    std::string localIV;  // 保留字段：握手时随密钥一起传输（GCM 每条消息使用独立随机 nonce）
    std::string remoteKey;  // Base64 编码的远程会话密钥
    std::string remoteIV;   // 保留字段（同上）

    mutable AutoSeededRandomPool rng;

    // 辅助函数：Base64编码
    std::string base64Encode(const std::string& data) const;

    // 辅助函数：Base64解码
    std::string base64Decode(const std::string& data) const;

    // AES-256-GCM 加密。
    // 输出为 Base64( nonce(12) || ciphertext || tag(16) )，
    // 每次调用生成独立的随机 nonce，密文自带完整性校验标签。
    std::string aesEncrypt(const std::string& plaintext, const std::string& key) const;

    // AES-256-GCM 解密并校验完整性。
    // 若数据被篡改或密钥不匹配，会抛出 CryptoError。
    std::string aesDecrypt(const std::string& ciphertext, const std::string& key) const;
};

#endif  // AES_KEY_H
