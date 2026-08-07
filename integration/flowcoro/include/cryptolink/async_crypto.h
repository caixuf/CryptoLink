#ifndef CRYPTOLINK_FLOWCORO_ASYNC_CRYPTO_H
#define CRYPTOLINK_FLOWCORO_ASYNC_CRYPTO_H

// CryptoLink × FlowCoro 集成层
// --------------------------------------------------------------------------
// 目的：把 RSA/AES 这类"阻塞 CPU"的加解密计算从事件循环/主线程卸载到
// FlowCoro 的线程池执行，用 C++20 协程（flowcoro::Task<T>）以顺序、可读的
// 方式编排异步加密流程。
//
// 注意：本集成只依赖 CryptoLink 的加密核心类（RSAKey/AESKey），不涉及
// websocketpp（其 0.8.x 版本无法在 C++20 下编译）。传输层仍保持 C++17。

#include <coroutine>
#include <exception>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

#include <flowcoro.hpp>

#include "AESKey.h"
#include "RSAKey.h"

namespace cryptolink::flowcoro_integration {

// 将任意可调用对象放到 FlowCoro 全局线程池执行，并把它做成可 co_await 的
// awaitable：协程在此挂起，工作线程算完后通过 FlowCoro 调度器恢复协程。
template <typename F>
class PoolAwaitable {
public:
    using ResultType = std::invoke_result_t<F>;
    static_assert(!std::is_void_v<ResultType>,
                  "PoolAwaitable 仅支持有返回值的可调用对象");

    explicit PoolAwaitable(F fn) : fn_(std::move(fn)) {}

    bool await_ready() const noexcept { return false; }

    void await_suspend(std::coroutine_handle<> handle) {
        ::flowcoro::GlobalThreadPool::enqueue_void([this, handle]() {
            try {
                result_.emplace(fn_());
            } catch (...) {
                error_ = std::current_exception();
            }
            // 回到 FlowCoro 调度器上安全恢复协程
            ::flowcoro::schedule_coroutine_enhanced(handle);
        });
    }

    ResultType await_resume() {
        if (error_) {
            std::rethrow_exception(error_);
        }
        return std::move(*result_);
    }

private:
    F fn_;
    std::optional<ResultType> result_;
    std::exception_ptr error_;
};

// 便捷工厂：off-load 一个计算到线程池并返回 awaitable
template <typename F>
PoolAwaitable<std::decay_t<F>> run_on_pool(F&& fn) {
    return PoolAwaitable<std::decay_t<F>>(std::forward<F>(fn));
}

// --------------------------------------------------------------------------
// 常用加密操作的协程封装
// 说明：这些封装通过引用捕获密钥对象，调用方需保证密钥在协程完成前存活。
// --------------------------------------------------------------------------

// 异步生成 RSA 密钥对（2048 位生成较慢，非常适合卸载到线程池）
inline ::flowcoro::Task<bool> generate_rsa_keypair_async(RSAKey& key) {
    bool ok = co_await run_on_pool([&key]() { return key.generateKeyPair(); });
    co_return ok;
}

// 异步 AES-GCM 加密（使用本地会话密钥）
inline ::flowcoro::Task<std::string> aes_encrypt_async(AESKey& key, std::string plaintext) {
    std::string out = co_await run_on_pool(
        [&key, pt = std::move(plaintext)]() { return key.encryptWithLocal(pt); });
    co_return out;
}

// 异步 AES-GCM 解密（使用远端会话密钥）；解密失败时会抛出 CryptoError
inline ::flowcoro::Task<std::string> aes_decrypt_async(AESKey& key, std::string ciphertext) {
    std::string out = co_await run_on_pool(
        [&key, ct = std::move(ciphertext)]() { return key.decryptWithRemote(ct); });
    co_return out;
}

// 异步用远端公钥做 RSA 加密（握手中加密会话密钥）
inline ::flowcoro::Task<std::string> rsa_encrypt_async(RSAKey& key, std::string plaintext) {
    std::string out = co_await run_on_pool(
        [&key, pt = std::move(plaintext)]() { return key.encryptWithRemotePublic(pt); });
    co_return out;
}

} // namespace cryptolink::flowcoro_integration

#endif // CRYPTOLINK_FLOWCORO_ASYNC_CRYPTO_H
