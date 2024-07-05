////
// @file hello_test.cc
// @brief
// hello单元测试
//
// @author niexw
// @email niexiaowen@uestc.edu.cn
//
#include <gtest/gtest.h>
// #include <test_target.h>

TEST(hello, world)
{
    ASSERT_TRUE(true);
    printf("hello, world\n");
}
