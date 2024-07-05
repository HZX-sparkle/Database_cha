////
// @file BitmapIndex_test.cc
// @brief
// BitmapIndex 单元测试
//
// @author niexw
// @email niexiaowen@uestc.edu.cn
//
#include <gtest/gtest.h>
#include "BitmapIndex.h"
// #include <test_target.h>

TEST(hello, bitmap)
{
    ASSERT_TRUE(true);
    printf("hello, Bitmap\n");
}

TEST(BitmapIndexTest, readCSV1)
{
    BitmapIndex index;
    bool ret = index.readCSV("../data/tags.csv");
    EXPECT_TRUE(ret);
    auto tags = index.getTags("1");
    EXPECT_EQ(tags.size(), 3);
    EXPECT_EQ(tags[0], "tag1");
    EXPECT_EQ(tags[1], "tag2");
    EXPECT_EQ(tags[2], "tag3");

    tags = index.getTags("2");
    EXPECT_EQ(tags.size(), 3);
    EXPECT_EQ(tags[0], "tag1");
    EXPECT_EQ(tags[1], "tag4");
    EXPECT_EQ(tags[2], "tag5");

    tags = index.getTags("2");
    EXPECT_EQ(tags.size(), 3);
    EXPECT_EQ(tags[0], "tag1");
    EXPECT_EQ(tags[1], "tag4");
    EXPECT_EQ(tags[2], "tag5");

    tags = index.getTags("3");
    EXPECT_EQ(tags.size(), 2);
    EXPECT_EQ(tags[0], "tag2");
    EXPECT_EQ(tags[1], "tag6");

    tags = index.getTags("4");
    EXPECT_EQ(tags.size(), 4);
    EXPECT_EQ(tags[0], "tag3");
    EXPECT_EQ(tags[1], "tag5");
    EXPECT_EQ(tags[2], "tag7");
    EXPECT_EQ(tags[3], "tag6");
}

TEST(BitmapIndexTest, readCSV2)
{
    BitmapIndex index2;
    bool ret = index2.readCSV("unknown file");
    EXPECT_FALSE(ret);
}