message(STATUS "hello src")

# 指定头文件搜索目录
include_directories(${CMAKE_SOURCE_DIR}/include
	${CMAKE_SOURCE_DIR}/third/CRoaring-4.0.0/include)

# 添加可执行程序目标
add_executable(demo demo.cc)
# 链接库
target_link_libraries(demo PRIVATE roaring)