////
// @file table.h
// @brief
// 存储管理
//
// @author niexw
// @email niexiaowen@uestc.edu.cn
//
#ifndef __DB_TABLE_H__
#define __DB_TABLE_H__

#include <string>
#include <algorithm>
#include <vector>
#include <deque>
#include <queue>
#include <iterator>
#include <utility>
#include <iostream>
#include <iomanip>
#include "./record.h"
#include "./schema.h"
#include "./block.h"
#include "./buffer.h"
//#include "./bplustree.h"
namespace db {

////
// @brief
// 表操作接口
//
class Table
{
  public:
    // 表的迭代器
    struct BlockIterator
    {
        DataBlock block;
        BufDesp *bufdesp;

        BlockIterator();
        ~BlockIterator();
        BlockIterator(const BlockIterator &other);

        // 前置操作
        BlockIterator &operator++();
        // 后置操作
        BlockIterator operator++(int);
        // 数据块指针
        DataBlock *operator->();

        // 释放buffer
        void release();
    };

    //B+树
    class B_plus_tree
    {
      public:
         
        Table *table_;
        unsigned int root_blkid;
        B_plus_tree(Table *table_)
            : table_(table_)
            , root_blkid(NULL)
        {}


        //测试用
        std::vector<unsigned int> get_all_blkid(IndexBlock data)
        { 
            std::vector<unsigned int> ids;
            ids.push_back(data.getNext());
            for (int index = 0; index < data.getSlots(); index++) {
                Record record;
                Slot *slots = data.getSlotsPointer();
                record.attach(
                    data.buffer_ + be16toh(slots[index].offset),
                    be16toh(slots[index].length));
                unsigned int len;
                unsigned int blkid;
                record.getByIndex((char *) &blkid, &len, 1);
                findDataType("INT")->betoh(&blkid);
                ids.push_back(blkid);
            }
            return ids;
        }
        void block_print_all_blk_id(unsigned int index_blkid, unsigned floor)
        {
            IndexBlock data;
            data.setTable(table_);

            // 从buffer中借用
            BufDesp *bd = kBuffer.borrow(table_->name_.c_str(), index_blkid);
            data.attach(bd->buffer);
            std::vector<unsigned int> all_blkid = get_all_blkid(data);

            for (unsigned i : all_blkid) {
                for (unsigned i = 0; i < floor; i++) {
                    std::cout << "    ";
                }
                    std::cout<< i << std::endl;
                if (data.getType() == BLOCK_TYPE_INDEX_INTERNAL) {
                    block_print_all_blk_id(i, floor + 1);
                }

            }
        }
        void print_all_blk_id()
        {
            block_print_all_blk_id(root_blkid, 0);
        }


        unsigned int get_goal(IndexBlock data, void *key_buff, size_t len)
        {
             int index = (int)data.searchRecord(key_buff, len);

            Record record;
            if (index < data.getSlots()) {
                Slot *slots = data.getSlotsPointer();
                record.attach(
                    data.buffer_ + be16toh(slots[index].offset),
                    be16toh(slots[index].length));
                unsigned char *pkey;
                unsigned int len;
                record.refByIndex(&pkey, &len, 0);
                if (memcmp(pkey, key_buff, len) == 0) index++;
            }
            index--;
            unsigned int goal;
            if (index < 0) {
                goal = data.getNext();
            } else {
                Slot *slots = data.getSlotsPointer();
                record.attach(
                    data.buffer_ + be16toh(slots[index].offset),
                    be16toh(slots[index].length));
                unsigned int len;
                record.getByIndex((char*) & goal, &len, 1);
                findDataType("INT")->betoh(&goal);
            }
            return goal;
        }

        unsigned int block_search(unsigned int index_blkid,void *key_buff, unsigned int len)
        { 
            IndexBlock data;
            data.setTable(table_);

            // 从buffer中借用
            BufDesp *bd = kBuffer.borrow(table_->name_.c_str(), index_blkid);
            data.attach(bd->buffer);
            unsigned goal = get_goal(data, key_buff, len);

            if (data.getType() == BLOCK_TYPE_INDEX_POINT_TO_LEAF) {
                return goal;
            }

            //当data.getType() == BLOCK_TYPE_INDEX_INTERNA
            return block_search(goal,key_buff,len);
        }
         
        //******** 所需信息 ********
        // unsigned int state (S_OK|SPARED|EEXIT);
        // unsigned int next_blkid;
        // void *key;
        // void *len;
        #define SPARED 2
        std::pair < unsigned int,
            std::vector<struct iovec>>
        block_insert_to_this_node(unsigned int index_blkid, std::vector<struct iovec> &iov)
        {
            IndexBlock data;
            SuperBlock super;
            data.setTable(table_);
            auto name_ = table_->name_;
            BufDesp *bd = kBuffer.borrow(name_.c_str(), index_blkid);
            data.attach(bd->buffer);
            // 尝试插入
            std::pair<bool, unsigned short> ret = data.insertRecord(iov);
            if (ret.first) {
                kBuffer.releaseBuf(bd); // 释放buffer
                // 修改表头统计
                bd = kBuffer.borrow(name_.c_str(), 0);
                super.attach(bd->buffer);
                super.setRecords(super.getRecords() + 1);
                bd->relref();
                return { S_OK, std::vector<struct iovec>()
                }; // 插入成功
            } else if (ret.second == (unsigned short) -1) {
                kBuffer.releaseBuf(bd); // 释放buffer
                return {EEXIST, std::vector<struct iovec>()};
                 // key已经存在
            }

            // 分裂block
            unsigned short insert_position = ret.second;
            std::pair<unsigned short, bool> split_position =
                data.splitPosition(Record::size(iov), insert_position);
            // 先分配一个block
            IndexBlock next;
            next.setTable(table_);
            unsigned int blkid = table_->allocate();
            BufDesp *bd2 = kBuffer.borrow(name_.c_str(), blkid);
            next.attach(bd2->buffer);
            next.setType(data.getType());
            // 移动记录到新的block上
            while (data.getSlots() > split_position.first) {
                Record record;
                data.refslots(split_position.first, record);
                next.copyRecord(record);
                data.deallocate(split_position.first);
            }
            // 插入新记录，不需要再重排顺序
            if (split_position.second)
                data.insertRecord(iov);
            else
                next.insertRecord(iov);

            std::vector<struct iovec> upper_key(2);
            
            unsigned int len;
            next.beginrecord()->refByIndex(
                (unsigned char **) &upper_key[0].iov_base,
                &len,
                0);
            upper_key[0].iov_len = len;
            
            Slot *slots = next.getSlotsPointer();
            Record record;
            record.attach(
                data.buffer_ + be16toh(slots[0].offset),
                be16toh(slots[0].length));
            
          
            upper_key[1].iov_base =
                &(reinterpret_cast<MetaHeader *>(next.buffer_)->self);
            upper_key[1].iov_len = 4;
            bd2->relref();

            bd = kBuffer.borrow(name_.c_str(), 0);
            super.attach(bd->buffer);
            super.setRecords(super.getRecords() + 1);
            bd->relref();
            
            return {SPARED, upper_key};
        }

        std::pair<unsigned int, std::vector<struct iovec>>
        block_insert_blkid(
            unsigned int index_blkid,
            std::vector<struct iovec>& iov)
        {

            IndexBlock data;
            data.setTable(table_);

            // 从buffer中借用
            BufDesp *bd = kBuffer.borrow(table_->name_.c_str(), index_blkid);
            data.attach(bd->buffer);

            if (data.getType() == BLOCK_TYPE_INDEX_POINT_TO_LEAF) {
                return block_insert_to_this_node(index_blkid, iov);
            }
            //当 data.getType() == BLOCK_TYPE_INDEX_INTERNAL
            unsigned int goal = get_goal(data, iov[0].iov_base, iov[0].iov_len);

            auto ret = block_insert_blkid(goal, iov);
            if (ret.first != SPARED) { return ret; }
            return block_insert_to_this_node(index_blkid, ret.second);
        }

        unsigned int search(void *key_buff, unsigned int len) {
            if (root_blkid == NULL) return -1;
            return block_search(root_blkid, key_buff, len);
            
        }

        unsigned int insert(std::vector<struct iovec> &iov)
        {
            if (root_blkid == NULL) { 
                IndexBlock new_root;
                new_root.setTable(table_);
                unsigned int blkid = table_->allocate();
                BufDesp *bd2 = kBuffer.borrow(table_->name_.c_str(), blkid);
                new_root.attach(bd2->buffer);
                new_root.setType(BLOCK_TYPE_INDEX_POINT_TO_LEAF);
                findDataType("INT")->htobe(iov[1].iov_base);
                new_root.setNext(*(unsigned int*)(iov[1].iov_base));
                root_blkid = new_root.getSelf();
                return S_OK;
            }
            auto ret = block_insert_blkid(root_blkid, iov);
            if (ret.first != SPARED) { return ret.first; }

            IndexBlock new_root;
            new_root.setTable(table_);
            unsigned int blkid = table_->allocate();
            BufDesp *bd2 = kBuffer.borrow(table_->name_.c_str(), blkid);
            new_root.attach(bd2->buffer);
            new_root.setType(BLOCK_TYPE_INDEX_INTERNAL);
            new_root.setNext(root_blkid);
            block_insert_to_this_node(new_root.getSelf(), ret.second);
            root_blkid = new_root.getSelf();
            return S_OK;
        }
    };

  public:
    std::string name_;   // 表名
    RelationInfo *info_; // 表的元数据
    unsigned int maxid_; // 最大的blockid
    unsigned int idle_;  // 空闲链
    unsigned int first_; // 数据链

  public:
    Table()
        : info_(NULL)
        , maxid_(0)
        , idle_(0)
        , first_(0)
    {}

    // 打开一张表
    int open(const char *name);

    // 采用枚举的方式定位一个key在哪个block
    unsigned int locate(void *keybuf, unsigned int len);
    // 定位一个block后，插入一条记录
    int insert(unsigned int blkid, std::vector<struct iovec> &iov);
    int remove(unsigned int blkid, void *keybuf, unsigned int len);
    int update(unsigned int blkid, std::vector<struct iovec> &iov);
    // btree搜索
    unsigned int search(void *keybuf, unsigned int len);

    // 返回表上总的记录数目
    size_t recordCount();
    // 返回表上数据块个数
    unsigned int dataCount();
    // 返回表上空闲块个数
    unsigned int idleCount();

    // block迭代器
    BlockIterator beginblock();
    BlockIterator endblock();

    // 新分配一个block，返回blockid，但并没有将该block插入数据链上
    unsigned int allocate();
    // 回收一个block
    void deallocate(unsigned int blockid);
};

inline bool
operator==(const Table::BlockIterator &x, const Table::BlockIterator &y)
{
    if (x.block.table_ != y.block.table_)
        return false;
    else if (x.block.buffer_ == y.block.buffer_)
        return true;
    else
        return false;
}
inline bool
operator!=(const Table::BlockIterator &x, const Table::BlockIterator &y)
{
    if (x.block.table_ != y.block.table_)
        return true;
    else if (x.block.buffer_ != y.block.buffer_)
        return true;
    else
        return false;
}

} // namespace db

#endif // __DB_TABLE_H__
