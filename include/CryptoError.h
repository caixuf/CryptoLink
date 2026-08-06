#ifndef CRYPTO_ERROR_H
#define CRYPTO_ERROR_H

#include <stdexcept>
#include <string>

// 加解密/密钥操作失败时抛出的异常。
// 相比之前"静默返回空字符串"的做法，抛出异常可以让调用方
// 区分"空明文"与"解密失败/数据被篡改"这两种完全不同的情况。
class CryptoError : public std::runtime_error {
public:
    explicit CryptoError(const std::string& message)
        : std::runtime_error(message) {}
};

#endif // CRYPTO_ERROR_H
