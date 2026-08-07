// CryptoLink × FlowCoro 集成示例
// --------------------------------------------------------------------------
// 演示如何用 FlowCoro 协程编排加密流程：
//   1. 并发地在线程池上生成服务端/客户端 RSA 密钥对（原本是最耗时的阻塞操作）
//   2. 客户端用服务端公钥加密会话密钥（卸载到线程池）
//   3. 用 AES-GCM 异步加解密消息
// 全程以顺序的 co_await 表达，避免了回调地狱，且计算不阻塞调用线程。

#include <iostream>
#include <string>

#include <flowcoro.hpp>

#include "cryptolink/async_crypto.h"

using namespace cryptolink::flowcoro_integration;

flowcoro::Task<void> demo() {
    RSAKey serverRsa;
    RSAKey clientRsa;

    // 并发生成两对 RSA 密钥：两个 Task 创建后会各自在首个挂起点让出，
    // 由线程池并行完成 2048 位密钥生成。
    auto serverKeygen = generate_rsa_keypair_async(serverRsa);
    auto clientKeygen = generate_rsa_keypair_async(clientRsa);
    bool serverOk = co_await serverKeygen;
    bool clientOk = co_await clientKeygen;
    std::cout << "RSA 密钥生成: server=" << serverOk << ", client=" << clientOk << "\n";

    // 客户端拿到服务端公钥
    clientRsa.setRemotePublicKey(serverRsa.getLocalPublicKey());

    // 客户端生成 AES 会话密钥，并异步用服务端公钥加密
    AESKey clientAes;
    clientAes.generateRawKey();
    std::string sessionKey = clientAes.getLocalKey();
    std::string encryptedSessionKey = co_await rsa_encrypt_async(clientRsa, sessionKey);
    std::cout << "已加密会话密钥，密文长度: " << encryptedSessionKey.size() << "\n";

    // 服务端解密会话密钥，装配 AES
    std::string recovered = serverRsa.decryptWithLocalPrivate(encryptedSessionKey);
    auto pos = recovered.find(':');
    AESKey serverAes;
    serverAes.setRemotePublicKey(recovered.substr(0, pos), recovered.substr(pos + 1));

    // 异步加解密一条消息
    const std::string message = "Hello from FlowCoro-powered CryptoLink!";
    std::string cipher = co_await aes_encrypt_async(clientAes, message);
    std::string plain = co_await aes_decrypt_async(serverAes, cipher);
    std::cout << "往返明文一致: " << (plain == message ? "是" : "否") << "\n";
    std::cout << "解密结果: " << plain << "\n";
    co_return;
}

int main() {
    flowcoro::initialize();
    flowcoro::sync_wait(demo());
    flowcoro::shutdown();
    return 0;
}
