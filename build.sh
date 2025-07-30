#!/bin/bash

# CryptoLink 项目构建脚本

set -e

echo "=== CryptoLink 构建脚本 ==="

# 检查依赖
echo "检查系统依赖..."

if ! command -v cmake &> /dev/null; then
    echo "错误: 未找到 cmake，请先安装 cmake"
    exit 1
fi

if ! command -v g++ &> /dev/null; then
    echo "错误: 未找到 g++，请先安装 build-essential"
    exit 1
fi

# 检查 Crypto++ 库
if ! pkg-config --exists libcrypto++; then
    echo "警告: 未找到 libcrypto++ 开发包"
    echo "请运行: sudo apt-get install libcrypto++-dev"
fi

# 检查 Boost 库
if [ ! -d "/usr/include/boost" ] && [ ! -d "/usr/local/include/boost" ]; then
    echo "警告: 未找到 Boost 开发包"
    echo "请运行: sudo apt-get install libboost-all-dev"
fi

# 检查 JsonCpp 库
if ! pkg-config --exists jsoncpp; then
    echo "警告: 未找到 jsoncpp 开发包"
    echo "请运行: sudo apt-get install libjsoncpp-dev"
fi

# 创建构建目录
echo "创建构建目录..."
if [ -d "build" ]; then
    echo "清理旧的构建目录..."
    rm -rf build
fi

mkdir build
cd build

# 配置项目
echo "配置项目..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# 编译项目
echo "编译项目..."
make -j$(nproc)

echo "构建完成！"
echo ""
echo "可执行文件位置:"
echo "  服务端: ./server"
echo "  客户端: ./client"
echo ""
echo "运行示例:"
echo "  1. 启动服务端: ./server"
echo "  2. 启动客户端: ./client (在新终端中)"
