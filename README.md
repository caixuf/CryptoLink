# CryptoLink - 安全网络加密通信库

## 项目概述

CryptoLink 是一个基于 WebSocket 的安全网络通信库，实现了端到端加密通信。项目使用 RSA 非对称加密进行密钥交换，使用 AES 对称加密进行数据传输，确保通信的安全性和效率。

## 主要特性

- **双重加密保护**: RSA + AES 混合加密方案
- **WebSocket 通信**: 基于 WebSocket 协议的实时通信
- **密钥交换**: 安全的密钥协商和交换机制
- **多客户端支持**: 服务端支持多个客户端同时连接
- **跨平台**: 基于 CMake 构建，支持多平台编译

## 技术架构

### 加密方案
1. **非对称加密 (RSA-2048)**:
   - 用于安全的密钥交换
   - 数字签名验证
   - 公钥长度: 2048位

2. **对称加密 (AES-256-CBC)**:
   - 用于高效的数据传输加密
   - 密钥长度: 256位
   - 使用随机IV确保安全性

### 通信流程
1. 建立 WebSocket 连接
2. 客户端请求服务端公钥
3. 服务端发送公钥并请求客户端公钥
4. 客户端发送公钥
5. 客户端用服务端公钥加密并发送 AES 会话密钥
6. 开始使用 AES 会话密钥进行加密通信
7. 连接结束时销毁所有密钥

## 项目结构

```
CryptoLink/
├── include/                          # 头文件目录
│   ├── AsymmetricalEncryptionInterface.h  # 非对称加密接口
│   ├── SymmetricalEncryptionInterface.h   # 对称加密接口
│   ├── RSAKey.h                      # RSA 实现类
│   ├── AESKey.h                      # AES 实现类
│   ├── CryptoWebSocketClient.h       # WebSocket 客户端
│   └── CryptoWebSocketServer.h       # WebSocket 服务端
├── src/                              # 源文件目录
│   ├── RSAKey.cpp
│   ├── AESKey.cpp
│   ├── CryptoWebSocketClient.cpp
│   └── CryptoWebSocketServer.cpp
├── examples/                         # 示例程序
│   ├── client.cpp                    # 客户端示例
│   └── server.cpp                    # 服务端示例
├── CMakeLists.txt                    # CMake 配置文件
├── README.md                         # 项目说明
└── 技术方案.md                       # 详细技术方案
```

## 依赖库

- **Crypto++**: 密码学库，提供 RSA 和 AES 算法实现
- **WebSocket++**: C++ WebSocket 库
- **Boost**: C++ 库集合，WebSocket++ 的依赖
- **JsonCpp**: JSON 解析库，用于消息序列化

## 编译安装

### 系统依赖

Ubuntu/Debian:
```bash
sudo apt-get update
sudo apt-get install build-essential cmake
sudo apt-get install libcrypto++-dev libboost-all-dev libjsoncpp-dev
```

CentOS/RHEL:
```bash
sudo yum install gcc-c++ cmake
sudo yum install cryptopp-devel boost-devel jsoncpp-devel
```

### 编译步骤

```bash
# 克隆项目
git clone <repository-url>
cd CryptoLink

# 创建构建目录
mkdir build
cd build

# 配置和编译
cmake ..
make -j4

# 运行示例
./server    # 启动服务端
./client    # 启动客户端（新终端）
```

## 使用示例

### 服务端使用

```cpp
#include "CryptoWebSocketServer.h"

CryptoWebSocketServer server;

// 设置消息回调
server.setMessageCallback([&](websocketpp::connection_hdl hdl, const std::string& message) {
    std::cout << "收到消息: " << message << std::endl;
    server.sendEncryptedMessage(hdl, "回复: " + message);
});

// 启动服务器
server.start(9002);
server.run();
```

### 客户端使用

```cpp
#include "CryptoWebSocketClient.h"

CryptoWebSocketClient client;

// 设置消息回调
client.setMessageCallback([](const std::string& message) {
    std::cout << "收到消息: " << message << std::endl;
});

// 连接并发送消息
client.connect("ws://localhost:9002");
client.run();
client.sendEncryptedMessage("Hello, encrypted world!");
```

## API 文档

### AsymmetricalEncryptionInterface (非对称加密接口)

- `generateKeyPair()`: 生成 RSA 密钥对
- `getLocalPublicKey()`: 获取本地公钥 (Base64)
- `setRemotePublicKey()`: 设置远程公钥
- `encryptWithRemotePublic()`: 使用远程公钥加密
- `decryptWithLocalPrivate()`: 使用本地私钥解密
- `signWithLocalPrivate()`: 使用本地私钥签名
- `verifyWithRemotePublic()`: 使用远程公钥验证签名

### SymmetricalEncryptionInterface (对称加密接口)

- `generateRawKey()`: 生成 AES 密钥和 IV
- `encryptWithLocal()`: 使用本地密钥加密
- `decryptWithLocal()`: 使用本地密钥解密
- `encryptWithRemote()`: 使用远程密钥加密
- `decryptWithRemote()`: 使用远程密钥解密
- `setRemotePublicKey()`: 设置远程会话密钥
- `getLocalKey()`: 获取本地密钥

## 安全性说明

1. **密钥长度**: RSA-2048, AES-256 提供足够的安全强度
2. **密钥管理**: 密钥仅在内存中存储，连接结束后自动销毁
3. **随机性**: 使用 Crypto++ 的安全随机数生成器
4. **消息完整性**: 支持数字签名验证消息完整性
5. **Forward Secrecy**: 每次连接使用独立的会话密钥

## 性能特性

- **高效传输**: AES 对称加密确保数据传输效率
- **最小握手**: 优化的密钥交换流程
- **多线程**: 支持并发连接处理
- **内存管理**: 智能指针管理，防止内存泄漏

## 开发计划

- [ ] 添加更多加密算法支持 (ECC, ChaCha20)
- [ ] 实现完整的 TLS 握手
- [ ] 添加连接认证机制
- [ ] 性能基准测试
- [ ] 单元测试覆盖
- [ ] 文档完善

## 许可证

本项目采用 MIT 许可证，详见 LICENSE 文件。

## 贡献

欢迎提交 Issue 和 Pull Request！

## 联系方式

如有问题，请通过 GitHub Issues 联系。
