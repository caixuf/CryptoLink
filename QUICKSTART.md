# CryptoLink 快速启动指南

## 快速开始

### 1. 安装依赖

**自动安装（推荐）:**
```bash
./install_deps.sh
```

**手动安装（Ubuntu/Debian）:**
```bash
sudo apt-get update
sudo apt-get install build-essential cmake libcrypto++-dev libboost-all-dev libjsoncpp-dev
```

### 2. 编译项目

**自动编译:**
```bash
./build.sh
```

**手动编译:**
```bash
mkdir build && cd build
cmake ..
make -j4
```

### 3. 运行测试

```bash
cd build
./crypto_test
```

### 4. 运行示例

**启动服务端（终端1）:**
```bash
cd build
./server
```

**启动客户端（终端2）:**
```bash
cd build  
./client
```

## 使用说明

### 服务端操作
- 启动后会监听 9002 端口
- 按 Enter 键广播测试消息
- 输入文本后按 Enter 广播自定义消息
- 输入 'quit' 退出服务端

### 客户端操作
- 自动连接到 localhost:9002
- 会自动发送几条测试消息
- 进入交互模式后可输入消息发送
- 输入 'quit' 退出客户端

### 加密流程
1. 客户端连接服务端
2. 自动执行 RSA 密钥交换
3. 客户端发送 AES 会话密钥（RSA加密）
4. 后续通信使用 AES 加密
5. 断开连接时自动销毁密钥

## 故障排除

### 编译错误
- **找不到 crypto++**: 安装 `libcrypto++-dev`
- **找不到 boost**: 安装 `libboost-all-dev`  
- **找不到 jsoncpp**: 安装 `libjsoncpp-dev`

### 运行错误
- **连接失败**: 确保服务端已启动并监听正确端口
- **握手失败**: 检查防火墙设置，确保端口 9002 可访问

### 网络测试
```bash
# 测试端口是否监听
netstat -tlnp | grep 9002

# 测试连接
telnet localhost 9002
```

## 开发使用

### 集成到你的项目

1. 包含头文件:
```cpp
#include "CryptoWebSocketClient.h"  // 客户端
#include "CryptoWebSocketServer.h"  // 服务端
```

2. 链接库:
```cmake
target_link_libraries(your_project CryptoLinkLib)
```

### 简单示例

**客户端:**
```cpp
CryptoWebSocketClient client;
client.setMessageCallback([](const std::string& msg) {
    std::cout << "收到: " << msg << std::endl;
});
client.connect("ws://localhost:9002");
client.run();
client.sendEncryptedMessage("Hello!");
```

**服务端:**
```cpp
CryptoWebSocketServer server;
server.setMessageCallback([&](auto hdl, const std::string& msg) {
    server.sendEncryptedMessage(hdl, "回复: " + msg);
});
server.start(9002);
server.run();
```
