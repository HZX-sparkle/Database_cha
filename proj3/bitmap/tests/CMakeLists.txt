
# 指定头文件搜索目录
include_directories(${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/src
	${CMAKE_SOURCE_DIR}/third/google-test-1.14.0/googletest/include)

# 添加可执行程序目标
add_executable(btest test.cc hello_test.cc)
# 链接库
target_link_libraries(btest PRIVATE gtest)
