message(STATUS "hello src")

include_directories(
    ${CMAKE_SOURCE_DIR}/include 
    ${CMAKE_SOURCE_DIR}/src
    )

set(LIB_INV_INDEX_IMPL BitmapIndex.cc)
add_library(bitmap_impl STATIC ${LIB_INV_INDEX_IMPL})