##
# @file CMakeLists.txt
# @brief
#
# @author
# @email
#
cmake_minimum_required(VERSION 3.12)

# 工程名
project(proj3 LANGUAGES CXX VERSION 0.1.0)
message(STATUS "proj3 -- a bitmap inverted index project")

# c++版本
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设定输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${PROJECT_BINARY_DIR}/bin")

# 源文件
set(SOURCES 
    ${CMAKE_SOURCE_DIR}/src/BitmapIndex.cc
    ${CMAKE_SOURCE_DIR}/src/demo.cc)

# 包含头文件
include_directories(${CMAKE_SOURCE_DIR}/include
${CMAKE_SOURCE_DIR}/third/CRoaring-4.0.0/include)

# 编译目录
add_subdirectory(third)
add_subdirectory(src)
add_subdirectory(tests)

# 添加可执行程序目标
add_executable(BitmapIndexProject ${SOURCES})

# 链接库
target_link_libraries(BitmapIndexProject roaring)

