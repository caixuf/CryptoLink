#ifndef RSA_KEY_H
#define RSA_KEY_H

#include "AsymmetricalEncryptionInterface.h"
#include <cryptopp/rsa.h>
#include <cryptopp/osrng.h>
#include <cryptopp/base64.h>
#include <cryptopp/pssr.h>
#include <memory>

using namespace CryptoPP;

class RSAKey : public AsymmetricalEncryptionInterface {
public:
    RSAKey();
    ~RSAKey();
    
    bool generateKeyPair() override;
    std::string getLocalPublicKey() override;
    bool setRemotePublicKey(const std::string& publicKey) override;
    std::string encryptWithLocalPrivate(const std::string& plaintext) override;
    std::string decryptWithLocalPrivate(const std::string& ciphertext) override;
    std::string encryptWithRemotePublic(const std::string& plaintext) override;
    std::string decryptWithRemotePublic(const std::string& ciphertext) override;
    std::string signWithLocalPrivate(const std::string& data) override;
    bool verifyWithRemotePublic(const std::string& data, const std::string& signature) override;

private:
    AutoSeededRandomPool rng;
    std::unique_ptr<RSA::PrivateKey> localPrivateKey;
    std::unique_ptr<RSA::PublicKey> localPublicKey;
    std::unique_ptr<RSA::PublicKey> remotePublicKey;
    
    // 辅助函数：将密钥转换为Base64字符串
    std::string keyToString(const RSA::PublicKey& key) const;
    
    // 辅助函数：从Base64字符串恢复公钥
    bool stringToPublicKey(const std::string& keyString, RSA::PublicKey& key) const;
    
    // 辅助函数：Base64编码
    std::string base64Encode(const std::string& data) const;
    
    // 辅助函数：Base64解码
    std::string base64Decode(const std::string& data) const;
};

#endif // RSA_KEY_H
