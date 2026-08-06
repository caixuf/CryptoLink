#include "AESKey.h"
#include "CryptoError.h"
#include <cryptopp/aes.h>
#include <cryptopp/gcm.h>
#include <cryptopp/filters.h>
#include <iostream>

namespace {
// GCM 推荐的 nonce 长度为 12 字节，认证标签为 16 字节。
constexpr size_t kGcmNonceSize = 12;
constexpr size_t kGcmTagSize = 16;
constexpr size_t kAesKeySize = 32; // AES-256
} // namespace

AESKey::AESKey() = default;

AESKey::~AESKey() = default;

bool AESKey::generateRawKey() {
    try {
        // 生成 AES-256 密钥（32 字节）
        byte key[kAesKeySize];
        rng.GenerateBlock(key, sizeof(key));
        localKey = base64Encode(std::string((char*)key, sizeof(key)));

        // 保留 IV 字段用于握手兼容；实际加密时每条消息使用独立随机 nonce
        byte iv[kGcmNonceSize];
        rng.GenerateBlock(iv, sizeof(iv));
        localIV = base64Encode(std::string((char*)iv, sizeof(iv)));

        return true;
    } catch (const Exception& e) {
        std::cerr << "AES密钥生成失败: " << e.what() << std::endl;
        return false;
    }
}

std::string AESKey::encryptWithLocal(const std::string& plaintext) {
    return aesEncrypt(plaintext, localKey);
}

std::string AESKey::decryptWithLocal(const std::string& ciphertext) {
    return aesDecrypt(ciphertext, localKey);
}

std::string AESKey::encryptWithRemote(const std::string& plaintext) {
    return aesEncrypt(plaintext, remoteKey);
}

std::string AESKey::decryptWithRemote(const std::string& ciphertext) {
    return aesDecrypt(ciphertext, remoteKey);
}

bool AESKey::setRemotePublicKey(const std::string& keyString, const std::string& iv) {
    remoteKey = keyString;
    remoteIV = iv;
    return true;
}

std::string AESKey::getLocalKey() {
    return localKey + ":" + localIV; // 握手时传输：key:iv
}

std::string AESKey::base64Encode(const std::string& data) const {
    std::string encoded;
    StringSource ss(data, true,
        new Base64Encoder(
            new StringSink(encoded),
            false // 不换行，便于放入 JSON
        )
    );
    return encoded;
}

std::string AESKey::base64Decode(const std::string& data) const {
    std::string decoded;
    StringSource ss(data, true,
        new Base64Decoder(
            new StringSink(decoded)
        )
    );
    return decoded;
}

std::string AESKey::aesEncrypt(const std::string& plaintext, const std::string& key) const {
    std::string keyDecoded = base64Decode(key);
    if (keyDecoded.size() != kAesKeySize) {
        throw CryptoError("AES加密失败: 密钥长度非法");
    }

    try {
        // 每条消息生成独立的随机 nonce，避免 nonce 复用导致的严重安全问题
        byte nonce[kGcmNonceSize];
        rng.GenerateBlock(nonce, sizeof(nonce));

        std::string cipherAndTag;
        GCM<AES>::Encryption encryption;
        encryption.SetKeyWithIV((const byte*)keyDecoded.data(), keyDecoded.size(),
                                nonce, sizeof(nonce));

        StringSource ss(plaintext, true,
            new AuthenticatedEncryptionFilter(encryption,
                new StringSink(cipherAndTag),
                false,
                kGcmTagSize
            )
        );

        // 组装 nonce || (ciphertext + tag) 后再 Base64
        std::string blob(reinterpret_cast<char*>(nonce), sizeof(nonce));
        blob += cipherAndTag;
        return base64Encode(blob);
    } catch (const Exception& e) {
        throw CryptoError(std::string("AES加密失败: ") + e.what());
    }
}

std::string AESKey::aesDecrypt(const std::string& ciphertext, const std::string& key) const {
    std::string keyDecoded = base64Decode(key);
    if (keyDecoded.size() != kAesKeySize) {
        throw CryptoError("AES解密失败: 密钥长度非法");
    }

    std::string blob = base64Decode(ciphertext);
    if (blob.size() < kGcmNonceSize + kGcmTagSize) {
        throw CryptoError("AES解密失败: 密文长度不足");
    }

    const byte* nonce = reinterpret_cast<const byte*>(blob.data());
    std::string cipherAndTag = blob.substr(kGcmNonceSize);

    try {
        std::string recovered;
        GCM<AES>::Decryption decryption;
        decryption.SetKeyWithIV((const byte*)keyDecoded.data(), keyDecoded.size(),
                                nonce, kGcmNonceSize);

        // AuthenticatedDecryptionFilter 在标签校验失败时抛出 HashVerificationFailed
        StringSource ss(cipherAndTag, true,
            new AuthenticatedDecryptionFilter(decryption,
                new StringSink(recovered),
                AuthenticatedDecryptionFilter::DEFAULT_FLAGS,
                kGcmTagSize
            )
        );

        return recovered;
    } catch (const Exception& e) {
        // 包含数据被篡改（标签不匹配）的情况
        throw CryptoError(std::string("AES解密失败(数据可能被篡改): ") + e.what());
    }
}
