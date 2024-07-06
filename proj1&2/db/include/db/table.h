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
        
        class Node
        {   
        public:
            // 先使用一个自定义数据结构替代index_block
            // TODO:弄懂info_后,将其替换为index_bloc
            class custom_index
            {
              public:
                unsigned int sub_blkid;
                Node *forward_sub_node;
                void *key;
                unsigned int len;
                custom_index(unsigned int sub_blkid)
                    : sub_blkid(sub_blkid)
                {}
                custom_index(Node *forward_sub_node)
                    : forward_sub_node(forward_sub_node)
                {}
                custom_index(
                    unsigned int sub_blkid,
                    void *key,
                    unsigned int len)
                    : sub_blkid(sub_blkid)
                    , key(key)
                    , len(len)
                {}
                custom_index(
                    Node *forward_sub_node,
                    void *key,
                    unsigned int len)
                    : forward_sub_node(forward_sub_node)
                    , key(key)
                    , len(len)
                {}
                custom_index(void *key, unsigned int len)
                    : key(key)
                    , len(len)
                {}
            };

            unsigned int max_key_size;
            db::Table *table_;
            unsigned short node_type;
            custom_index last_sub_unit;
            std::vector<custom_index> sub_nodes;
            
            Node(
                unsigned int max_key_size,unsigned short type,
                custom_index last_sub_node)
                : max_key_size(max_key_size)
                , node_type(type)
                , last_sub_unit(last_sub_node)
            {}
            auto find_upper_bound(void *keybuf, unsigned int len)
            {
                /* RelationInfo *info = table_->info_;
                unsigned int key = info->key;
                DataType *type = info->fields[key].type;*/
                // 测试用
                DataType *type = findDataType("INT");
                /*return std::upper_bound(
                    sub_nodes.begin(),
                    sub_nodes.end(),
                    custom_index(keybuf, len),
                    [&](const custom_index &a, const custom_index &b) {
                        return type->less((unsigned char *) a.key,
                                          a.len,
                            (unsigned char *) b.key,
                            b.len);
                    });*/
                return std::upper_bound(
                    sub_nodes.begin(),
                    sub_nodes.end(),
                    custom_index(keybuf, len),
                    [&](const custom_index &a, const custom_index &b) {
                        return *(unsigned int *) a.key < *(unsigned int *) b.key;
                    });
            }
            bool is_full() { 
                return sub_nodes.size() == max_key_size;
            }
            unsigned int search(void* keybuf, unsigned int len) {
                auto search_result = find_upper_bound(keybuf, len);
                custom_index goal=(search_result==sub_nodes.end())?last_sub_unit:*search_result;
                
                if (this->node_type == BLOCK_TYPE_INDEX_POINT_TO_LEAF) {
                    return goal.sub_blkid;
                } 
                return goal.forward_sub_node->search(keybuf,  len);

            }
            void insert(unsigned int blkid, void *keybuf, unsigned int len){
                auto search_result = find_upper_bound(keybuf, len);
                custom_index goal = (search_result == sub_nodes.end())
                                        ? last_sub_unit
                                        : *search_result;

                if (node_type == BLOCK_TYPE_INDEX_POINT_TO_LEAF) {
                    custom_index pre_insert(blkid, keybuf, len);
                    if (search_result == sub_nodes.end()) { 
                        std::swap(
                            pre_insert.sub_blkid,
                            last_sub_unit.sub_blkid);
                    } else {
                        std::swap(
                            pre_insert.sub_blkid,
                            search_result->sub_blkid);
                    }
                    sub_nodes.insert(search_result, pre_insert);
                    return;
                }

                if (node_type == BLOCK_TYPE_INDEX_INTERNAL) {
                    Node* sub = goal.forward_sub_node;
                    if (sub->is_full()) { 
                        auto upper_key = sub->spare();
                        if (search_result == sub_nodes.end()) {
                            std::swap(
                                upper_key.forward_sub_node,
                                last_sub_unit.forward_sub_node);
                        } else {
                            std::swap(
                                upper_key.forward_sub_node,
                                search_result->forward_sub_node);
                        }
                        sub_nodes.insert(search_result, upper_key);
                        insert(blkid, keybuf, len);
                    } else {
                        sub->insert(blkid, keybuf, len);
                    }
                    
                }

            }
            custom_index spare() { 
                int mid = max_key_size / 2;
                auto upper_key = *std::next(sub_nodes.begin(), mid);
                Node *next_node =
                    new Node(max_key_size,node_type, last_sub_unit);
                last_sub_unit = upper_key;

                std::vector<custom_index> next_sub_nodes;

                copy(
                    std::next(sub_nodes.begin(), mid+1),
                    sub_nodes.end(),
                    std::back_inserter(next_sub_nodes));
                sub_nodes.erase(
                    std::next(sub_nodes.begin(), mid), sub_nodes.end());
                next_node->sub_nodes = next_sub_nodes;
                upper_key.forward_sub_node = next_node;
                return upper_key;
            }

            //测试用
            void collect_value(
                std::vector<std::pair<int, int> > &value,int floor)
            {
                for (auto& node : sub_nodes) {
                    
                    if (node_type == BLOCK_TYPE_INDEX_INTERNAL) {
                        node.forward_sub_node->collect_value(value, floor + 1);
                        
                    }
                    if (node_type == BLOCK_TYPE_INDEX_POINT_TO_LEAF) {
                        value.emplace_back(node.sub_blkid, floor+1);
                    }
                    value.emplace_back(*(int *) node.key, floor);
                }
                if (node_type == BLOCK_TYPE_INDEX_INTERNAL) {
                    last_sub_unit.forward_sub_node->collect_value(
                        value, floor + 1);
                }
                if (node_type == BLOCK_TYPE_INDEX_POINT_TO_LEAF) {
                   value.emplace_back(last_sub_unit.sub_blkid, floor + 1);
                }
            }
        };

        Node* root;
        bool is_init;
        unsigned int max_key_size;
        const unsigned int default_max_key_size=100;
        //Table *table_; // 指向table

        B_plus_tree() :is_init (false){}
        B_plus_tree(unsigned int max_key_size)
            : is_init(false)
            , max_key_size(max_key_size)
        {}
        
        void print() {
            std::vector<std::pair<int, int> > value;
            root->collect_value(value, 0);
            for (auto v : value) {
                std::cout <<std::setw(v.second*4)<< v.first << std::endl;
            }
        }
        unsigned int search(void *keybuf, unsigned int len){
            return root->search(keybuf, len);
        }
        void insert(unsigned int blkid, void* keybuf, unsigned int len) {
            if (!is_init) {
                root = new Node(
                    max_key_size, BLOCK_TYPE_INDEX_POINT_TO_LEAF, blkid);
                is_init = true;
                return;
            }
            if (root->is_full()) {
                auto seconde_son = root->spare();
                Node *new_root =
                    new Node(root->max_key_size, BLOCK_TYPE_INDEX_INTERNAL, seconde_son);
                seconde_son.forward_sub_node = root;
                new_root->sub_nodes.push_back(seconde_son);
                root = new_root;
            }
            root->insert(blkid, keybuf, len);
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
