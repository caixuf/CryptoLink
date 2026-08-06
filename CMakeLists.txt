cmake_minimum_required(VERSION 3.16)
project(CryptoLink VERSION 1.0.0 LANGUAGES CXX)

# ---------------------------------------------------------------------------
# 全局设置
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# 构建选项
option(BUILD_EXAMPLES "Build example client/server programs" ON)
option(BUILD_TESTS    "Build unit tests (GoogleTest)"        ON)
# flowcoro 需要 C++20；由于系统 websocketpp 0.8.x 无法在 C++20 下编译，
# flowcoro 集成仅作用于与 websocketpp 无关的 RSA/AES 计算，且默认关闭。
option(WITH_FLOWCORO  "Build optional flowcoro coroutine integration (requires C++20)" OFF)

include(GNUInstallDirs)

# ---------------------------------------------------------------------------
# 依赖查找
# ---------------------------------------------------------------------------
find_package(PkgConfig REQUIRED)
pkg_check_modules(CRYPTOPP REQUIRED libcrypto++)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(Threads REQUIRED)

# ---------------------------------------------------------------------------
# 加密核心库（不依赖 websocketpp，C++17/C++20 均可编译）
#   —— 供单元测试与 flowcoro 集成复用
# ---------------------------------------------------------------------------
add_library(CryptoLinkCrypto STATIC
    src/RSAKey.cpp
    src/AESKey.cpp
)
target_include_directories(CryptoLinkCrypto PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    ${CRYPTOPP_INCLUDE_DIRS}
)
target_link_libraries(CryptoLinkCrypto PUBLIC ${CRYPTOPP_LIBRARIES})
target_compile_options(CryptoLinkCrypto PRIVATE ${CRYPTOPP_CFLAGS_OTHER})

# ---------------------------------------------------------------------------
# 传输库（WebSocket + JSON 协议），依赖加密核心库
# ---------------------------------------------------------------------------
add_library(CryptoLinkLib STATIC
    src/CryptoWebSocketClient.cpp
    src/CryptoWebSocketServer.cpp
)
target_include_directories(CryptoLinkLib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    ${JSONCPP_INCLUDE_DIRS}
)
target_link_libraries(CryptoLinkLib PUBLIC
    CryptoLinkCrypto
    ${JSONCPP_LIBRARIES}
    ${Boost_LIBRARIES}
    Threads::Threads
)
target_compile_options(CryptoLinkLib PRIVATE ${JSONCPP_CFLAGS_OTHER})

# 通用告警选项
foreach(_tgt CryptoLinkCrypto CryptoLinkLib)
    target_compile_options(${_tgt} PRIVATE -Wall -Wextra)
endforeach()

# ---------------------------------------------------------------------------
# 示例程序
# ---------------------------------------------------------------------------
if(BUILD_EXAMPLES)
    add_executable(client examples/client.cpp)
    target_link_libraries(client PRIVATE CryptoLinkLib)

    add_executable(server examples/server.cpp)
    target_link_libraries(server PRIVATE CryptoLinkLib)

    add_executable(crypto_test examples/crypto_test.cpp)
    target_link_libraries(crypto_test PRIVATE CryptoLinkCrypto)

    add_executable(simple_test examples/simple_test.cpp)
    target_link_libraries(simple_test PRIVATE CryptoLinkCrypto)

    add_executable(signature_test examples/signature_test.cpp)
    target_link_libraries(signature_test PRIVATE CryptoLinkCrypto)
endif()

# ---------------------------------------------------------------------------
# 单元测试
# ---------------------------------------------------------------------------
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ---------------------------------------------------------------------------
# 可选：flowcoro 协程集成（把 RSA/AES 计算卸载到 flowcoro 线程池）
# ---------------------------------------------------------------------------
if(WITH_FLOWCORO)
    add_subdirectory(integration/flowcoro)
endif()

# ---------------------------------------------------------------------------
# 安装与导出
# ---------------------------------------------------------------------------
install(TARGETS CryptoLinkCrypto CryptoLinkLib
    EXPORT CryptoLinkTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/CryptoLinkConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(EXPORT CryptoLinkTargets
    FILE CryptoLinkTargets.cmake
    NAMESPACE CryptoLink::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CryptoLink
)
configure_package_config_file(cmake/CryptoLinkConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/CryptoLinkConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CryptoLink
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/CryptoLinkConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/CryptoLinkConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CryptoLink
)
