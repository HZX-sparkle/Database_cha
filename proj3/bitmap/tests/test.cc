////
// @file test.cc
// @brief
// 采用catch作为单元测试方案，需要一个main函数，这里定义。
//
// @author niexw
// @email xiaowen.nie.cn@gmail.com
//
#include <gtest/gtest.h>
// #include <benchmark/benchmark.h>

int main(int argc, char **argv)
{
    // 功能测试
    testing::InitGoogleTest(&argc, argv);
    int ret = testing::UnitTest::GetInstance()->Run();
    if (ret) return ret;

    // 性能测试
    // benchmark::Initialize(&argc, argv);
    // ret = benchmark::ReportUnrecognizedArguments(argc, argv);
    // if (ret) return ret;
    // benchmark::RunSpecifiedBenchmarks();

    return ret;
}
