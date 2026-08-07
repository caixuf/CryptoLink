#include <gtest/gtest.h>

#include "AESKey.h"
#include "RSAKey.h"
#include "CryptoError.h"

namespace {

// 让 remote 与 local 共享同一会话密钥（模拟握手后的双方）。
// AESKey 含 AutoSeededRandomPool，不可拷贝/移动，故通过引用就地初始化。
void linkAesPair(AESKey& local, AESKey& remote) {
    EXPECT_TRUE(local.generateRawKey());
    std::string kv = local.getLocalKey();
    auto pos = kv.find(':');
    EXPECT_NE(pos, std::string::npos);
    EXPECT_TRUE(remote.setRemotePublicKey(kv.substr(0, pos), kv.substr(pos + 1)));
}

} // namespace

TEST(AESKeyTest, GcmRoundTrip) {
    AESKey local, remote;
    linkAesPair(local, remote);
    const std::string plaintext = "Hello, GCM! 你好，实战级别加密。";

    std::string cipher = local.encryptWithLocal(plaintext);
    ASSERT_FALSE(cipher.empty());
    EXPECT_EQ(remote.decryptWithRemote(cipher), plaintext);
}

TEST(AESKeyTest, EmptyPlaintextRoundTrip) {
    AESKey local, remote;
    linkAesPair(local, remote);
    std::string cipher = local.encryptWithLocal("");
    // 空明文与"解密失败"必须可区分：这里应正常得到空串，而不是抛异常
    EXPECT_EQ(remote.decryptWithRemote(cipher), "");
}

TEST(AESKeyTest, NonceIsRandomizedPerMessage) {
    AESKey local, remote;
    linkAesPair(local, remote);
    const std::string plaintext = "same message";
    // 相同明文两次加密应产生不同密文（每条消息独立随机 nonce）
    EXPECT_NE(local.encryptWithLocal(plaintext), local.encryptWithLocal(plaintext));
}

TEST(AESKeyTest, TamperedCiphertextThrows) {
    AESKey local, remote;
    linkAesPair(local, remote);
    std::string cipher = local.encryptWithLocal("integrity protected");

    // 篡改密文中间某个 Base64 字符，GCM 标签校验应失败并抛出 CryptoError
    ASSERT_GT(cipher.size(), 4u);
    cipher[cipher.size() / 2] = (cipher[cipher.size() / 2] == 'A') ? 'B' : 'A';
    EXPECT_THROW(remote.decryptWithRemote(cipher), CryptoError);
}

TEST(AESKeyTest, WrongKeyThrows) {
    AESKey local, remote;
    linkAesPair(local, remote);
    std::string cipher = local.encryptWithLocal("secret");

    AESKey other;
    ASSERT_TRUE(other.generateRawKey()); // 不同的密钥
    EXPECT_THROW(other.decryptWithLocal(cipher), CryptoError);
}

TEST(RSAKeyTest, PublicKeyEncryptRoundTrip) {
    RSAKey server;
    RSAKey client;
    ASSERT_TRUE(server.generateKeyPair());
    ASSERT_TRUE(client.generateKeyPair());
    ASSERT_TRUE(client.setRemotePublicKey(server.getLocalPublicKey()));

    const std::string secret = "session-key-material";
    std::string cipher = client.encryptWithRemotePublic(secret);
    ASSERT_FALSE(cipher.empty());
    EXPECT_EQ(server.decryptWithLocalPrivate(cipher), secret);
}

TEST(RSAKeyTest, SignAndVerify) {
    RSAKey signer;
    RSAKey verifier;
    ASSERT_TRUE(signer.generateKeyPair());
    ASSERT_TRUE(verifier.generateKeyPair());
    ASSERT_TRUE(verifier.setRemotePublicKey(signer.getLocalPublicKey()));

    const std::string data = "authenticate this session key";
    std::string signature = signer.signWithLocalPrivate(data);
    ASSERT_FALSE(signature.empty());
    EXPECT_TRUE(verifier.verifyWithRemotePublic(data, signature));
}

TEST(RSAKeyTest, VerifyRejectsTamperedData) {
    RSAKey signer;
    RSAKey verifier;
    ASSERT_TRUE(signer.generateKeyPair());
    ASSERT_TRUE(verifier.generateKeyPair());
    ASSERT_TRUE(verifier.setRemotePublicKey(signer.getLocalPublicKey()));

    std::string signature = signer.signWithLocalPrivate("original data");
    // 数据被篡改后验签必须失败（此前的实现存在恒返回 true 的严重 bug）
    EXPECT_FALSE(verifier.verifyWithRemotePublic("tampered data", signature));
}

TEST(RSAKeyTest, DecryptGarbageThrows) {
    RSAKey server;
    ASSERT_TRUE(server.generateKeyPair());
    EXPECT_THROW(server.decryptWithLocalPrivate("bm90LWEtdmFsaWQtY2lwaGVy"), CryptoError);
}

// 模拟完整握手中的密钥交换与认证：
// 客户端用服务端公钥加密会话密钥，并用自己的私钥对会话密钥签名；
// 服务端解密后用客户端公钥验签，验证通过才接受会话密钥。
TEST(HandshakeTest, AuthenticatedSessionKeyExchange) {
    RSAKey serverRsa;      // 服务端密钥
    RSAKey clientRsa;      // 客户端密钥
    ASSERT_TRUE(serverRsa.generateKeyPair());
    ASSERT_TRUE(clientRsa.generateKeyPair());

    // 交换公钥（握手前两步）
    ASSERT_TRUE(clientRsa.setRemotePublicKey(serverRsa.getLocalPublicKey()));

    RSAKey serverViewOfClient;
    ASSERT_TRUE(serverViewOfClient.generateKeyPair());
    ASSERT_TRUE(serverViewOfClient.setRemotePublicKey(clientRsa.getLocalPublicKey()));

    // 客户端生成会话密钥并加密 + 签名
    AESKey clientAes;
    ASSERT_TRUE(clientAes.generateRawKey());
    std::string sessionKey = clientAes.getLocalKey();
    std::string encrypted = clientRsa.encryptWithRemotePublic(sessionKey);
    std::string signature = clientRsa.signWithLocalPrivate(sessionKey);

    // 服务端解密 + 验签
    std::string decrypted = serverRsa.decryptWithLocalPrivate(encrypted);
    ASSERT_EQ(decrypted, sessionKey);
    EXPECT_TRUE(serverViewOfClient.verifyWithRemotePublic(decrypted, signature));

    // 服务端装配 AES 会话密钥后应能与客户端互通
    auto pos = decrypted.find(':');
    ASSERT_NE(pos, std::string::npos);
    AESKey serverAes;
    ASSERT_TRUE(serverAes.setRemotePublicKey(decrypted.substr(0, pos), decrypted.substr(pos + 1)));

    const std::string msg = "post-handshake payload";
    EXPECT_EQ(serverAes.decryptWithRemote(clientAes.encryptWithLocal(msg)), msg);
}

TEST(HandshakeTest, ForgedSignatureIsRejected) {
    RSAKey serverRsa;
    RSAKey clientRsa;
    RSAKey attackerRsa;
    ASSERT_TRUE(serverRsa.generateKeyPair());
    ASSERT_TRUE(clientRsa.generateKeyPair());
    ASSERT_TRUE(attackerRsa.generateKeyPair());
    ASSERT_TRUE(attackerRsa.setRemotePublicKey(serverRsa.getLocalPublicKey()));

    // 服务端认为对端是合法客户端
    RSAKey serverViewOfClient;
    ASSERT_TRUE(serverViewOfClient.generateKeyPair());
    ASSERT_TRUE(serverViewOfClient.setRemotePublicKey(clientRsa.getLocalPublicKey()));

    // 攻击者用自己的私钥签名（冒充客户端）
    AESKey attackerAes;
    ASSERT_TRUE(attackerAes.generateRawKey());
    std::string sessionKey = attackerAes.getLocalKey();
    std::string signature = attackerRsa.signWithLocalPrivate(sessionKey);

    // 服务端用真实客户端公钥验签，必须失败
    EXPECT_FALSE(serverViewOfClient.verifyWithRemotePublic(sessionKey, signature));
}
