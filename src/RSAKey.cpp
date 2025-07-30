#include "RSAKey.h"
#include <cryptopp/rsa.h>
#include <cryptopp/pssr.h>
#include <cryptopp/hex.h>
#include <cryptopp/filters.h>
#include <cryptopp/files.h>
#include <cryptopp/queue.h>
#include <iostream>

RSAKey::RSAKey() {
    localPrivateKey = std::make_unique<RSA::PrivateKey>();
    localPublicKey = std::make_unique<RSA::PublicKey>();
    remotePublicKey = std::make_unique<RSA::PublicKey>();
}

RSAKey::~RSAKey() = default;

bool RSAKey::generateKeyPair() {
    try {
        // 直接使用 RSA 密钥生成
        localPrivateKey->GenerateRandomWithKeySize(rng, 2048);
        
        // 从私钥派生公钥
        *localPublicKey = *localPrivateKey;
        
        return true;
    } catch (const Exception& e) {
        std::cerr << "RSA密钥对生成失败: " << e.what() << std::endl;
        return false;
    }
}

std::string RSAKey::getLocalPublicKey() {
    try {
        return keyToString(*localPublicKey);
    } catch (const Exception& e) {
        std::cerr << "获取本地公钥失败: " << e.what() << std::endl;
        return "";
    }
}

bool RSAKey::setRemotePublicKey(const std::string& publicKey) {
    try {
        return stringToPublicKey(publicKey, *remotePublicKey);
    } catch (const Exception& e) {
        std::cerr << "设置远程公钥失败: " << e.what() << std::endl;
        return false;
    }
}

std::string RSAKey::encryptWithLocalPrivate(const std::string& plaintext) {
    try {
        std::string ciphertext;
        RSASS<PSSR, SHA256>::Signer signer(*localPrivateKey);
        
        StringSource ss(plaintext, true,
            new SignerFilter(rng, signer,
                new StringSink(ciphertext)
            )
        );
        
        return base64Encode(ciphertext);
    } catch (const Exception& e) {
        std::cerr << "使用本地私钥加密失败: " << e.what() << std::endl;
        return "";
    }
}

std::string RSAKey::decryptWithLocalPrivate(const std::string& ciphertext) {
    try {
        std::string decoded = base64Decode(ciphertext);
        std::string recovered;
        
        RSAES_OAEP_SHA_Decryptor decryptor(*localPrivateKey);
        
        StringSource ss(decoded, true,
            new PK_DecryptorFilter(rng, decryptor,
                new StringSink(recovered)
            )
        );
        
        return recovered;
    } catch (const Exception& e) {
        std::cerr << "使用本地私钥解密失败: " << e.what() << std::endl;
        return "";
    }
}

std::string RSAKey::encryptWithRemotePublic(const std::string& plaintext) {
    try {
        std::string ciphertext;
        RSAES_OAEP_SHA_Encryptor encryptor(*remotePublicKey);
        
        StringSource ss(plaintext, true,
            new PK_EncryptorFilter(rng, encryptor,
                new StringSink(ciphertext)
            )
        );
        
        return base64Encode(ciphertext);
    } catch (const Exception& e) {
        std::cerr << "使用远程公钥加密失败: " << e.what() << std::endl;
        return "";
    }
}

std::string RSAKey::decryptWithRemotePublic(const std::string& ciphertext) {
    try {
        std::string decoded = base64Decode(ciphertext);
        std::string recovered;
        
        RSASS<PSSR, SHA256>::Verifier verifier(*remotePublicKey);
        
        StringSource ss(decoded, true,
            new SignatureVerificationFilter(verifier,
                new StringSink(recovered),
                SignatureVerificationFilter::THROW_EXCEPTION
            )
        );
        
        return recovered;
    } catch (const Exception& e) {
        std::cerr << "使用远程公钥解密失败: " << e.what() << std::endl;
        return "";
    }
}

std::string RSAKey::signWithLocalPrivate(const std::string& data) {
    try {
        std::string signature;
        RSASS<PSSR, SHA256>::Signer signer(*localPrivateKey);
        
        StringSource ss(data, true,
            new SignerFilter(rng, signer,
                new StringSink(signature)
            )
        );
        
        return base64Encode(signature);
    } catch (const Exception& e) {
        std::cerr << "使用本地私钥签名失败: " << e.what() << std::endl;
        return "";
    }
}

bool RSAKey::verifyWithRemotePublic(const std::string& data, const std::string& signature) {
    try {
        std::string decoded = base64Decode(signature);
        RSASS<PSSR, SHA256>::Verifier verifier(*remotePublicKey);
        
        StringSource ss(data + decoded, true,
            new SignatureVerificationFilter(verifier,
                nullptr,
                SignatureVerificationFilter::PUT_RESULT
            )
        );
        
        return true;
    } catch (const Exception& e) {
        std::cerr << "使用远程公钥验证签名失败: " << e.what() << std::endl;
        return false;
    }
}

std::string RSAKey::keyToString(const RSA::PublicKey& key) const {
    std::string keyString;
    StringSink ss(keyString);
    key.Save(ss);
    return base64Encode(keyString);
}

bool RSAKey::stringToPublicKey(const std::string& keyString, RSA::PublicKey& key) const {
    try {
        std::string decoded = base64Decode(keyString);
        StringSource ss(decoded, true);
        key.Load(ss);
        return true;
    } catch (const Exception& e) {
        std::cerr << "从字符串恢复公钥失败: " << e.what() << std::endl;
        return false;
    }
}

std::string RSAKey::base64Encode(const std::string& data) const {
    std::string encoded;
    StringSource ss(data, true,
        new Base64Encoder(
            new StringSink(encoded)
        )
    );
    return encoded;
}

std::string RSAKey::base64Decode(const std::string& data) const {
    std::string decoded;
    StringSource ss(data, true,
        new Base64Decoder(
            new StringSink(decoded)
        )
    );
    return decoded;
}
