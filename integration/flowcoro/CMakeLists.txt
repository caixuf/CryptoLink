# FlowCoro 集成子项目
# 仅在顶层配置 -DWITH_FLOWCORO=ON 时才会被加入构建。
#
# 该集成需要 C++20，并通过 FetchContent 拉取 flowcoro 源码。
# 为保持轻量，关闭 flowcoro 自带的测试/示例/基准/PGO 等选项。

include(FetchContent)

# 允许通过 -DFLOWCORO_GIT_TAG=... 覆盖版本
set(FLOWCORO_GIT_TAG "main" CACHE STRING "flowcoro git tag/branch to fetch")

# 关闭 flowcoro 的重型子构建，缩短集成构建时间
set(FLOWCORO_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(FLOWCORO_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(FLOWCORO_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(FLOWCORO_ENABLE_PGO OFF CACHE BOOL "" FORCE)
set(FLOWCORO_ENABLE_DATABASE OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    flowcoro
    GIT_REPOSITORY https://github.com/caixuf/flowcoro.git
    GIT_TAG ${FLOWCORO_GIT_TAG}
)
FetchContent_MakeAvailable(flowcoro)

# 集成示例：用 C++20 编译，链接 flowcoro 运行时与 CryptoLink 加密核心库
add_executable(flowcoro_async_crypto examples/async_crypto_demo.cpp)
target_compile_features(flowcoro_async_crypto PRIVATE cxx_std_20)
target_include_directories(flowcoro_async_crypto PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
# flowcoro_net 携带 flowcoro 的运行时实现（调度器/线程池等符号）
target_link_libraries(flowcoro_async_crypto PRIVATE CryptoLinkCrypto flowcoro_net)
