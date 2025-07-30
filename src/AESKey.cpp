#include "AESKey.h"
#include <cryptopp/aes.h>
#include <cryptopp/modes.h>
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>
#include <iostream>

AESKey::AESKey() = default;

AESKey::~AESKey() = default;

bool AESKey::generateRawKey() {
    try {
        // 生成AES-256密钥（32字节）
        byte key[32];  // AES-256 需要32字节密钥
        rng.GenerateBlock(key, sizeof(key));
        localKey = base64Encode(std::string((char*)key, sizeof(key)));
        
        // 生成IV（16字节）
        byte iv[AES::BLOCKSIZE];
        rng.GenerateBlock(iv, sizeof(iv));
        localIV = base64Encode(std::string((char*)iv, sizeof(iv)));
        
        return true;
    } catch (const Exception& e) {
        std::cerr << "AES密钥生成失败: " << e.what() << std::endl;
        return false;
    }
}

std::string AESKey::encryptWithLocal(const std::string& plaintext) {
    return aesEncrypt(plaintext, localKey, localIV);
}

std::string AESKey::decryptWithLocal(const std::string& ciphertext) {
    return aesDecrypt(ciphertext, localKey, localIV);
}

std::string AESKey::encryptWithRemote(const std::string& plaintext) {
    return aesEncrypt(plaintext, remoteKey, remoteIV);
}

std::string AESKey::decryptWithRemote(const std::string& ciphertext) {
    return aesDecrypt(ciphertext, remoteKey, remoteIV);
}

bool AESKey::setRemotePublicKey(const std::string& keyString, const std::string& iv) {
    try {
        remoteKey = keyString;
        remoteIV = iv;
        return true;
    } catch (const Exception& e) {
        std::cerr << "设置远程密钥失败: " << e.what() << std::endl;
        return false;
    }
}

std::string AESKey::getLocalKey() {
    return localKey + ":" + localIV; // 简单的格式，实际项目中可能需要更复杂的格式
}

std::string AESKey::keyToString() const {
    return localKey + ":" + localIV;
}

bool AESKey::stringToKey() const {
    // 这个函数在当前实现中不需要，因为我们直接操作Base64字符串
    return true;
}

std::string AESKey::base64Encode(const std::string& data) const {
    std::string encoded;
    StringSource ss(data, true,
        new Base64Encoder(
            new StringSink(encoded)
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

std::string AESKey::aesEncrypt(const std::string& plaintext, const std::string& key, const std::string& iv) const {
    try {
        std::string keyDecoded = base64Decode(key);
        std::string ivDecoded = base64Decode(iv);
        
        std::string ciphertext;
        
        CBC_Mode<AES>::Encryption encryption;
        encryption.SetKeyWithIV((const byte*)keyDecoded.c_str(), keyDecoded.size(),
                               (const byte*)ivDecoded.c_str());
        
        StringSource ss(plaintext, true,
            new StreamTransformationFilter(encryption,
                new StringSink(ciphertext)
            )
        );
        
        return base64Encode(ciphertext);
    } catch (const Exception& e) {
        std::cerr << "AES加密失败: " << e.what() << std::endl;
        return "";
    }
}

std::string AESKey::aesDecrypt(const std::string& ciphertext, const std::string& key, const std::string& iv) const {
    try {
        std::string keyDecoded = base64Decode(key);
        std::string ivDecoded = base64Decode(iv);
        std::string ciphertextDecoded = base64Decode(ciphertext);
        
        std::string recovered;
        
        CBC_Mode<AES>::Decryption decryption;
        decryption.SetKeyWithIV((const byte*)keyDecoded.c_str(), keyDecoded.size(),
                               (const byte*)ivDecoded.c_str());
        
        StringSource ss(ciphertextDecoded, true,
            new StreamTransformationFilter(decryption,
                new StringSink(recovered)
            )
        );
        
        return recovered;
    } catch (const Exception& e) {
        std::cerr << "AES解密失败: " << e.what() << std::endl;
        return "";
    }
}
