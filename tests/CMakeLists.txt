# 单元测试（GoogleTest）

# 优先使用系统安装的 GTest；找不到时回退到 FetchContent 拉取
find_package(GTest QUIET)

if(NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

add_executable(crypto_unit_tests crypto_tests.cpp)

# 只依赖加密核心库（不含 websocketpp），保证测试在 C++17/C++20 下均可编译
target_link_libraries(crypto_unit_tests PRIVATE CryptoLinkCrypto GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(crypto_unit_tests)
