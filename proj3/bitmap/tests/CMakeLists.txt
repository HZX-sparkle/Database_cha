
# 指定头文件搜索目录
include_directories(${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/src
	${CMAKE_SOURCE_DIR}/third/googletest-1.14.0/googletest/include
	${CMAKE_SOURCE_DIR}/third/CRoaring-4.0.0/include
    ${CMAKE_SOURCE_DIR}/third/CRoaring-4.0.0/cpp)

# 添加可执行程序目标
add_executable(btest test.cc hello_test.cc BitmapIndex_test.cc)
# 链接库
add_dependencies(btest bitmap_impl)
target_link_libraries(btest bitmap_impl gtest roaring)
