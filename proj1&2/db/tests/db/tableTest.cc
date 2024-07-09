////
// @file tableTest.cc
// @brief
// 测试存储管理
//
// @author niexw
// @email niexiaowen@uestc.edu.cn
//
#include "../catch.hpp"
#include <db/table.h>
#include <db/block.h>
#include <db/buffer.h>
#include<iostream>
using namespace db;

namespace {
void dump(Table &table)
{
    // 打印所有记录，检查是否正确
    int rcount = 0;
    int bcount = 0;
    for (Table::BlockIterator bi = table.beginblock(); bi != table.endblock();
         ++bi, ++bcount) {
        for (unsigned short i = 0; i < bi->getSlots(); ++i, ++rcount) {
            Slot *slot = bi->getSlotsPointer() + i;
            Record record;
            record.attach(
                bi->buffer_ + be16toh(slot->offset), be16toh(slot->length));

            unsigned char *pkey;
            unsigned int len;
            long long key;
            record.refByIndex(&pkey, &len, 0);
            memcpy(&key, pkey, len);
            key = be64toh(key);
            printf(
                "key=%lld, offset=%d, rcount=%d, blkid=%d\n",
                key,
                be16toh(slot->offset),
                rcount,
                bcount);
        }
    }
    printf("total records=%zd\n", table.recordCount());
}
bool check(Table &table)
{
    int rcount = 0;
    int bcount = 0;
    long long okey = 0;
    for (Table::BlockIterator bi = table.beginblock(); bi != table.endblock();
         ++bi, ++bcount) {
        for (DataBlock::RecordIterator ri = bi->beginrecord();
             ri != bi->endrecord();
             ++ri, ++rcount) {
            unsigned char *pkey;
            unsigned int len;
            long long key;
            ri->refByIndex(&pkey, &len, 0);
            memcpy(&key, pkey, len);
            key = be64toh(key);
            if (okey >= key) {
                dump(table);
                printf("check error %d\n", rcount);
                return true;
            }
            okey = key;
        }
    }
    return false;
}
} // namespace

TEST_CASE("db/table.h")
{
    SECTION("less")
    {
        Buffer::BlockMap::key_compare compare;
        const char* table = "hello";
        std::pair<const char*, unsigned int> key1(table, 1);
        const char* table2 = "hello";
        std::pair<const char*, unsigned int> key2(table2, 1);
        bool ret = !compare(key1, key2);
        REQUIRE(ret);
        ret = !compare(key2, key1);
        REQUIRE(ret);
    }

    SECTION("open")
    {
        // NOTE: schemaTest.cc中创建
        Table table;
        int ret = table.open("table");
        REQUIRE(ret == S_OK);
        REQUIRE(table.name_ == "table");
        REQUIRE(table.maxid_ == 1);
        REQUIRE(table.idle_ == 0);
        REQUIRE(table.first_ == 1);
        REQUIRE(table.info_->key == 0);
        REQUIRE(table.info_->count == 3);
    }

    SECTION("bi")
    {
        Table table;
        int ret = table.open("table");
        REQUIRE(ret == S_OK);

        Table::BlockIterator bi = table.beginblock();
        REQUIRE(bi.block.table_ == &table);
        REQUIRE(bi.block.buffer_);

        unsigned int blockid = bi->getSelf();
        REQUIRE(blockid == 1);
        REQUIRE(blockid == bi.bufdesp->blockid);
        REQUIRE(bi.bufdesp->ref == 1);

        Table::BlockIterator bi1 = bi;
        REQUIRE(bi.bufdesp->ref == 2);
        bi.bufdesp->relref();

        ++bi;
        bool bret = bi == table.endblock();
        REQUIRE(bret);
    }

    SECTION("locate")
    {
        Table table;
        table.open("table");

        long long id = htobe64(5);
        int blkid = table.locate(&id, sizeof(id));
        REQUIRE(blkid == 1);
        id = htobe64(1);
        blkid = table.locate(&id, sizeof(id));
        REQUIRE(blkid == 1);
        id = htobe64(32);
        blkid = table.locate(&id, sizeof(id));
        REQUIRE(blkid == 1);
    }

    // 插满一个block
    SECTION("insert")
    {
        Table table;
        table.open("table");
        DataType* type = table.info_->fields[table.info_->key].type;

        // 检查表记录
        long long records = table.recordCount();
        REQUIRE(records == 0);
        Table::BlockIterator bi = table.beginblock();
        REQUIRE(bi->getSlots() == 4); // 已插入4条记录，但表上没记录
        bi.release();
        // 修正表记录
        BufDesp* bd = kBuffer.borrow("table", 0);
        REQUIRE(bd);
        SuperBlock super;
        super.attach(bd->buffer);
        super.setRecords(4);
        super.setDataCounts(1);
        REQUIRE(!check(table));

        // table = id(BIGINT)+phone(CHAR[20])+name(VARCHAR)
        // 准备添加
        std::vector<struct iovec> iov(3);
        long long nid;
        char phone[20];
        char addr[128];

        // 先填充
        int i, ret;
        for (i = 0; i < 91; ++i) {
            // 构造一个记录
            nid = rand();
            // printf("key=%lld\n", nid);
            type->htobe(&nid);
            iov[0].iov_base = &nid;
            iov[0].iov_len = 8;
            iov[1].iov_base = phone;
            iov[1].iov_len = 20;
            iov[2].iov_base = (void*)addr;
            iov[2].iov_len = 128;

            // locate位置
            unsigned int blkid =
                table.locate(iov[0].iov_base, (unsigned int)iov[0].iov_len);
            // 插入记录
            ret = table.insert(blkid, iov);
            if (ret == EEXIST) { printf("id=%lld exist\n", be64toh(nid)); }
            if (ret == EFAULT) break;
        }
        // 这里测试表明再插入到91条记录后出现分裂
        REQUIRE(i + 4 == table.recordCount());
        REQUIRE(!check(table));
    }

    SECTION("split")
    {
        Table table;
        table.open("table");
        DataType* type = table.info_->fields[table.info_->key].type;

        Table::BlockIterator bi = table.beginblock();

        unsigned short slot_count = bi->getSlots();
        size_t space = 162; // 前面的记录大小

        // 测试split，考虑插入位置在一半之前
        unsigned short index = (slot_count / 2 - 10); // 95/2-10
        std::pair<unsigned short, bool> ret = bi->splitPosition(space, index);
        REQUIRE(ret.first == 47); // 95/2=47，同时将新表项算在内
        REQUIRE(ret.second);

        // 在后半部分
        index = (slot_count / 2 + 10); // 95/2+10
        ret = bi->splitPosition(space, index);
        REQUIRE(ret.first == 48); // 95/2=47，同时将新表项算在内
        REQUIRE(!ret.second);

        // 在中间的位置上
        index = slot_count / 2; // 47
        ret = bi->splitPosition(space, index);
        REQUIRE(ret.first == 47); // 95/2=47，同时将新表项算在内
        REQUIRE(ret.second);

        // 在中间后一个位置上
        index = slot_count / 2 + 1; // 48
        ret = bi->splitPosition(space, index);
        REQUIRE(ret.first == 48); // 95/2=47，同时将新表项算在内
        REQUIRE(!ret.second);

        // 考虑space大小，超过一半
        space = BLOCK_SIZE / 2;
        index = (slot_count / 2 - 10); // 95/2-10
        ret = bi->splitPosition(space, index);
        REQUIRE(ret.first == index); // 未将新插入记录考虑在内
        REQUIRE(!ret.second);

        // space>1/2，位置在后部
        index = (slot_count / 2 + 10); // 95/2+10
        ret = bi->splitPosition(space, index);
        REQUIRE(ret.first == 48); // 95/2=47，同时将新表项算在内
        REQUIRE(!ret.second);

        // 测试说明，这个分裂策略可能使得前一半小于1/2
    }

    SECTION("allocate")
    {
        Table table;
        table.open("table");

        REQUIRE(table.dataCount() == 1);
        REQUIRE(table.idleCount() == 0);

        REQUIRE(table.maxid_ == 1);
        unsigned int blkid = table.allocate();
        REQUIRE(table.maxid_ == 2);
        REQUIRE(table.dataCount() == 2);

        Table::BlockIterator bi = table.beginblock();
        REQUIRE(bi.bufdesp->blockid == 1);
        ++bi;
        REQUIRE(bi == table.endblock()); // 新分配block未插入数据链
        REQUIRE(table.idle_ == 0);       // 也未放在空闲链上
        bi.release();

        // 回收该block
        table.deallocate(blkid);
        REQUIRE(table.idleCount() == 1);
        REQUIRE(table.dataCount() == 1);
        REQUIRE(table.idle_ == blkid);

        // 再从idle上分配
        blkid = table.allocate();
        REQUIRE(table.idleCount() == 0);
        REQUIRE(table.maxid_ == 2);
        REQUIRE(table.idle_ == 0);
        table.deallocate(blkid);
        REQUIRE(table.idleCount() == 1);
        REQUIRE(table.dataCount() == 1);
    }

    SECTION("insert2")
    {
        Table table;
        table.open("table");
        DataType* type = table.info_->fields[table.info_->key].type;

        // 准备添加
        std::vector<struct iovec> iov(3);
        long long nid;
        char phone[20];
        char addr[128];

        // 构造一个记录
        nid = rand();
        type->htobe(&nid);
        iov[0].iov_base = &nid;
        iov[0].iov_len = 8;
        iov[1].iov_base = phone;
        iov[1].iov_len = 20;
        iov[2].iov_base = (void*)addr;
        iov[2].iov_len = 128;

        int ret = table.insert(1, iov);
        REQUIRE(ret == S_OK);

        Table::BlockIterator bi = table.beginblock();
        REQUIRE(bi.bufdesp->blockid == 1);
        REQUIRE(bi->getSelf() == 1);
        REQUIRE(bi->getNext() == 2);
        unsigned short count1 = bi->getSlots();
        ++bi;
        REQUIRE(bi->getSelf() == 2);
        REQUIRE(bi->getNext() == 0);
        unsigned short count2 = bi->getSlots();
        REQUIRE(count1 + count2 == 96);
        REQUIRE(count1 + count2 == table.recordCount());
        REQUIRE(!check(table));

        // dump(table);
        // bi = table.beginblock();
        // bi->shrink();
        // dump(table);
        // bi->reorder(type, 0);
        // dump(table);
        // REQUIRE(!check(table));
    }

    // 再插入10000条记录
    SECTION("insert3")
    {
        Table table;
        table.open("table");
        DataType* type = table.info_->fields[table.info_->key].type;

        // 准备添加
        std::vector<struct iovec> iov(3);
        long long nid;
        char phone[20];
        char addr[128];

        iov[0].iov_base = &nid;
        iov[0].iov_len = 8;
        iov[1].iov_base = phone;
        iov[1].iov_len = 20;
        iov[2].iov_base = (void*)addr;
        iov[2].iov_len = 128;

        int count = 96;
        int count2 = 0;
        for (int i = 0; i < 10000; ++i) {
            nid = rand();
            type->htobe(&nid);
            // locate位置
            unsigned int blkid =
                table.locate(iov[0].iov_base, (unsigned int)iov[0].iov_len);
            // 插入记录
            int ret = table.insert(blkid, iov);
            if (ret == S_OK) ++count;
        }

        for (Table::BlockIterator bi = table.beginblock();
            bi != table.endblock();
            ++bi)
            count2 += bi->getSlots();
        REQUIRE(count == count2);
        REQUIRE(count == table.recordCount());
        REQUIRE(table.idleCount() == 0);

        REQUIRE(!check(table));
    }

    // 每插一条删一条，重复10次
    SECTION("remove")
    {
        Table table;
        table.open("table");
        DataType* type = table.info_->fields[table.info_->key].type;

        // 准备添加
        std::vector<struct iovec> iov(3);
        long long nid;
        char phone[20];
        char addr[128];

        iov[0].iov_base = &nid;
        iov[0].iov_len = 8;
        iov[1].iov_base = phone;
        iov[1].iov_len = 20;
        iov[2].iov_base = (void*)addr;
        iov[2].iov_len = 128;

        int count = 0;
        int count2 = 0;

        for (Table::BlockIterator bi = table.beginblock();
            bi != table.endblock();
            ++bi)
            count += bi->getSlots();

        count2 = 0;

        for (int i = 0; i < 1000; ++i) {
            nid = rand();
            type->htobe(&nid);
            // locate位置
            unsigned int blkid =
                table.locate(iov[0].iov_base, (unsigned int)iov[0].iov_len);
            // 插入记录
            int ret = table.insert(blkid, iov);
            if (ret == S_OK) ++count;
            // 删除记录
            ret = table.remove(blkid, iov[0].iov_base, (unsigned int)iov[0].iov_len);
            if (ret == S_OK) --count;
        }

        for (Table::BlockIterator bi = table.beginblock();
            bi != table.endblock();
            ++bi)
            count2 += bi->getSlots();

        REQUIRE(count == count2);
        REQUIRE(count == table.recordCount());
        REQUIRE(table.idleCount() == 0);

        REQUIRE(!check(table));
    }

    SECTION("update1")
    {
        Table table;
        table.open("table");
        DataType* type = table.info_->fields[table.info_->key].type;

        // 准备添加
        std::vector<struct iovec> iov(3);
        long long nid;
        char phone[20];
        char addr[120];

        strcpy_s(phone, "1234567890");
        strcpy_s(addr, "1234 Test Address");

        iov[0].iov_base = &nid;
        iov[0].iov_len = 8;
        iov[1].iov_base = phone;
        iov[1].iov_len = 20;
        iov[2].iov_base = (void*)addr;
        iov[2].iov_len = 120;


        nid = rand();
        type->htobe(&nid);

        // locate位置
        unsigned int blkid =
            table.locate(iov[0].iov_base, (unsigned int)iov[0].iov_len);

        // 插入记录
        int ret = table.insert(blkid, iov);

        // 修改并更新记录
        strcpy_s(phone, "0987654321");
        strcpy_s(addr, "4321 Test Address");
        ret = table.update(blkid, iov);
        REQUIRE(ret == S_OK);

        // 比较查询记录与修改记录是否一样
        RelationInfo* info = table.info_;
        unsigned int key = info->key;

        Table::BlockIterator data;
        Table::BlockIterator prev = table.beginblock();

        int is_return = false;
        for (Table::BlockIterator bi = table.beginblock(); bi != table.endblock(); ++bi) {
            // 获取第1个记录
            Record record;
            bi->refslots(0, record);

            // 与参数比较
            unsigned char* pkey;
            unsigned int klen;
            record.refByIndex(&pkey, &klen, key);
            bool bret = type->less(pkey, klen, (unsigned char*)iov[0].iov_base, (unsigned int)iov[0].iov_len);
            if (bret) {
                prev = bi;
                continue;
            }
            // 要排除相等的情况
            bret = type->less((unsigned char*)iov[0].iov_base, (unsigned int)iov[0].iov_len, pkey, klen);
            if (bret)
            {
                data = prev; //
                is_return = true;
                break;
            }
            else
            {
                data = bi; // 相等
                is_return = true;
                break;
            }
        }
        if (!is_return)
            data = prev;

        unsigned short index =
            type->search(data->buffer_, key, iov[key].iov_base, iov[key].iov_len);

        DataBlock::RecordIterator ri = data->beginrecord();
        Record record;
        if (index < data->getSlots()) {
            Slot* slots = data->getSlotsPointer();
            record.attach(
                data->buffer_ + be16toh(slots[index].offset),
                be16toh(slots[index].length));
            unsigned char* pkey;
            unsigned int len;
            record.refByIndex(&pkey, &len, 1);
            REQUIRE(strcmp((const char*)pkey, "0987654321") == 0);
            record.refByIndex(&pkey, &len, 2);
            REQUIRE(strcmp((const char*)pkey, "4321 Test Address") == 0);
        }

        REQUIRE(!check(table));
    }

    // 更新不存在的record
    SECTION("update2")
    {
        Table table;
        table.open("table");
        DataType* type = table.info_->fields[table.info_->key].type;

        // 准备删除
        std::vector<struct iovec> iov(3);
        long long nid;
        char phone[20];
        char addr[120];

        iov[0].iov_base = &nid;
        iov[0].iov_len = 8;
        iov[1].iov_base = phone;
        iov[1].iov_len = 20;
        iov[2].iov_base = (void*)addr;
        iov[2].iov_len = 128;

        nid = rand();
        type->htobe(&nid);

        // locate位置
        unsigned int blkid =
            table.locate(iov[0].iov_base, (unsigned int)iov[0].iov_len);
        // 删除记录
        table.remove(blkid, iov[0].iov_base, (unsigned int)iov[0].iov_len);
        // 更新不存在的记录
        int ret = table.update(blkid, iov);
        REQUIRE(ret == ENOENT);
        REQUIRE(!check(table));
    }

    // b+tree 搜索
    SECTION("search")
	{
		Table table;
        table.open("table");
		BPlusTree bplustree;
		bplustree.init(&table);

		long long id = htobe64(5);
        int blkid = table.search(&id, sizeof(id));
        REQUIRE(blkid == 1);
        id = htobe64(1);
        blkid = table.search(&id, sizeof(id));
        REQUIRE(blkid == 1);
        id = htobe64(32);
        blkid = table.search(&id, sizeof(id));
        REQUIRE(blkid == 1);
	}

    // b+tree 插入
    SECTION("btree_insert")
    {

    }
}