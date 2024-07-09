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
		REQUIRE(bplustree.info_->count == 2);
		REQUIRE(bplustree.info_->count == 2);
		REQUIRE(bplustree.info_->key == 0);
		FieldInfo field = bplustree.info_->fields[1];
		REQUIRE(field.name == "blkid");
		REQUIRE(field.index == 1);
		REQUIRE(field.length == 8);
		REQUIRE(field.type == findDataType("INT"));
	}

	
}