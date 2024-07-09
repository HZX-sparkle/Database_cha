////
// @file tableTest.cc
// @brief
// 测试B+树
//
// @author niexw
// @email niexiaowen@uestc.edu.cn
//
#include "../catch.hpp"
#include <db/table.h>
#include <db/block.h>
#include <db/buffer.h>
#include <db/bplustree.h>
#include<iostream>
using namespace db;

TEST_CASE("db/bplustree.h")
{
	SECTION("init")
	{
		Table table;
        table.open("table");
		BPlusTree bplustree;
		bool ret = bplustree.init(&table);
		REQUIRE(ret);
		REQUIRE(bplustree.root_);
	}
}