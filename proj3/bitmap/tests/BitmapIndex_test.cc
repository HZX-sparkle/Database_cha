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

// 读取文件测试
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

// 读取不存在的文件
TEST(BitmapIndexTest, readCSV2)
{
    BitmapIndex index2;
    bool ret = index2.readCSV("unknown file");
    EXPECT_FALSE(ret);
}

// 倒排索引测试
TEST(BitmapIndexTest, buildInvertedIndex)
{
    BitmapIndex index;
    index.readCSV("../data/tags.csv");
    index.buildInvertedIndex();
    auto bitmap = index.getBitmap("tag1");
    bitmap.printf();
    printf("\n");
    EXPECT_TRUE(bitmap.contains(1));
    EXPECT_TRUE(bitmap.contains(2));

    bitmap = index.getBitmap("tag2");
    bitmap.printf();
    printf("\n");
    EXPECT_TRUE(bitmap.contains(1));
    EXPECT_TRUE(bitmap.contains(3));
}

// 交或差运算
TEST(BitmapIndexTest, Operation)
{
    BitmapIndex index;
    index.readCSV("../data/tags.csv");
    index.buildInvertedIndex();
    std::vector <Roaring> bitmaps = {index.getBitmap("tag3"), index.getBitmap("tag4")};

    // andOperation
    Roaring andResult = BitmapIndex::andOperation(bitmaps);
    std::cout << "AND operation result: ";
    andResult.printf();
    std::cout << "\n";
    EXPECT_TRUE(andResult.isEmpty());

    // orOperation
    Roaring orResult = BitmapIndex::orOperation(bitmaps);
    std::cout << "OR operation result: ";
    orResult.printf();
    std::cout << "\n";
    EXPECT_TRUE(orResult.contains(1));
    EXPECT_TRUE(orResult.contains(2));
    EXPECT_TRUE(orResult.contains(4));

    // xorOperation
    Roaring xorResult = BitmapIndex::xorOperation(bitmaps);
    std::cout << "XOR operation result: ";
    xorResult.printf();
    std::cout << "\n";
    EXPECT_TRUE(xorResult.contains(1));
    EXPECT_TRUE(xorResult.contains(2));
    EXPECT_TRUE(xorResult.contains(4));
}