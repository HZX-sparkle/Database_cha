# 编译google-test
if (WIN32)
set(BUILD_GMOCK off)
set(INSTALL_GTEST off)
set(gtest_force_shared_crt on)
add_subdirectory(googletest-1.14.0)
endif()

# 关闭CRoaring的单元测试编译
set(ROARING_USE_CPM off)
set(ENABLE_ROARING_TESTS off)
add_subdirectory(CRoaring-4.0.0)

