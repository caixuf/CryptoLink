#!/bin/bash

# CryptoLink 依赖安装脚本

set -e

echo "=== CryptoLink 依赖安装脚本 ==="

# 检测操作系统
if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS=$NAME
    VER=$VERSION_ID
else
    echo "无法检测操作系统"
    exit 1
fi

echo "检测到操作系统: $OS"

# 根据操作系统安装依赖
case $OS in
    "Ubuntu"|"Debian GNU/Linux")
        echo "使用 apt 包管理器安装依赖..."
        sudo apt-get update
        
        echo "安装构建工具..."
        sudo apt-get install -y build-essential cmake pkg-config
        
        echo "安装 Crypto++ 库..."
        sudo apt-get install -y libcrypto++-dev
        
        echo "安装 Boost 库..."
        sudo apt-get install -y libboost-all-dev
        
        echo "安装 JsonCpp 库..."
        sudo apt-get install -y libjsoncpp-dev
        
        echo "安装 WebSocket++ 库（可选，项目已包含头文件）..."
        sudo apt-get install -y libwebsocketpp-dev || echo "WebSocket++ 安装可选，项目可能已包含"
        ;;
        
    "CentOS Linux"|"Red Hat Enterprise Linux")
        echo "使用 yum/dnf 包管理器安装依赖..."
        
        # 检测包管理器
        if command -v dnf &> /dev/null; then
            PKG_MANAGER="dnf"
        else
            PKG_MANAGER="yum"
        fi
        
        echo "安装构建工具..."
        sudo $PKG_MANAGER install -y gcc-c++ cmake pkgconfig
        
        echo "安装 EPEL 仓库（如果需要）..."
        sudo $PKG_MANAGER install -y epel-release || echo "EPEL 可能已安装"
        
        echo "安装 Crypto++ 库..."
        sudo $PKG_MANAGER install -y cryptopp-devel
        
        echo "安装 Boost 库..."
        sudo $PKG_MANAGER install -y boost-devel
        
        echo "安装 JsonCpp 库..."
        sudo $PKG_MANAGER install -y jsoncpp-devel
        ;;
        
    "Fedora")
        echo "使用 dnf 包管理器安装依赖..."
        
        echo "安装构建工具..."
        sudo dnf install -y gcc-c++ cmake pkgconfig
        
        echo "安装 Crypto++ 库..."
        sudo dnf install -y cryptopp-devel
        
        echo "安装 Boost 库..."
        sudo dnf install -y boost-devel
        
        echo "安装 JsonCpp 库..."
        sudo dnf install -y jsoncpp-devel
        ;;
        
    "Arch Linux")
        echo "使用 pacman 包管理器安装依赖..."
        
        echo "更新包数据库..."
        sudo pacman -Sy
        
        echo "安装构建工具..."
        sudo pacman -S --noconfirm base-devel cmake
        
        echo "安装 Crypto++ 库..."
        sudo pacman -S --noconfirm crypto++
        
        echo "安装 Boost 库..."
        sudo pacman -S --noconfirm boost
        
        echo "安装 JsonCpp 库..."
        sudo pacman -S --noconfirm jsoncpp
        ;;
        
    *)
        echo "不支持的操作系统: $OS"
        echo "请手动安装以下依赖:"
        echo "  - build-essential 或 gcc-c++"
        echo "  - cmake"
        echo "  - libcrypto++-dev 或 cryptopp-devel"
        echo "  - libboost-all-dev 或 boost-devel"
        echo "  - libjsoncpp-dev 或 jsoncpp-devel"
        exit 1
        ;;
esac

echo ""
echo "依赖安装完成！"
echo ""
echo "现在可以运行构建脚本:"
echo "  ./build.sh"
echo ""
echo "或者手动构建:"
echo "  mkdir build && cd build"
echo "  cmake .."
echo "  make -j\$(nproc)"
