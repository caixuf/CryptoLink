cmake_minimum_required(VERSION 3.10)
project(CryptoLink)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找crypto++库
find_package(PkgConfig REQUIRED)
pkg_check_modules(CRYPTOPP REQUIRED libcrypto++)

# 查找WebSocket++相关依赖
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(Threads REQUIRED)

# 查找jsoncpp库
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

# 包含头文件目录
include_directories(include)
include_directories(${CRYPTOPP_INCLUDE_DIRS})
include_directories(${JSONCPP_INCLUDE_DIRS})

# 添加源文件
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "include/*.h")

# 创建静态库
add_library(CryptoLinkLib STATIC ${SOURCES})
target_link_libraries(CryptoLinkLib ${CRYPTOPP_LIBRARIES} ${JSONCPP_LIBRARIES} ${Boost_LIBRARIES} Threads::Threads)
target_compile_options(CryptoLinkLib PRIVATE ${CRYPTOPP_CFLAGS_OTHER} ${JSONCPP_CFLAGS_OTHER})

# 创建客户端可执行文件
add_executable(client examples/client.cpp)
target_link_libraries(client CryptoLinkLib)

# 创建服务端可执行文件
add_executable(server examples/server.cpp)
target_link_libraries(server CryptoLinkLib)

# 创建加密测试程序
add_executable(crypto_test examples/crypto_test.cpp)
target_link_libraries(crypto_test CryptoLinkLib)

# 创建简单测试程序
add_executable(simple_test examples/simple_test.cpp)
target_link_libraries(simple_test CryptoLinkLib)

# 创建签名测试程序
add_executable(signature_test examples/signature_test.cpp)
target_link_libraries(signature_test CryptoLinkLib)

# 设置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
