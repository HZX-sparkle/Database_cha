////
// @file bplustree.h
// @brief
// 存储管理
//
// @author niexw
// @email niexiaowen@uestc.edu.cn
//
#ifndef __DB_BPLUSTREE_H__
#define __DB_BPLUSTREE_H__

#include <string>
#include <vector>
#include "./record.h"
#include "./schema.h"
#include "./block.h"
#include "./buffer.h"
#include "./table.h"

namespace db {

////
// @brief
// 索引表操作接口
//
class BPlusTree
{

  public:
    Table *table_;       // 连接表
    RelationInfo *info_; // 索引表的元数据
    unsigned int root_;  // 根节点

  public:
    BPlusTree()
        : table_(NULL)
        , info_(NULL)
        , root_(0)
    {}

    // 初始化B+tree
    bool init(Table *table);
    // btree搜索
    std::vector<unsigned int> BPlusTree::search(void* keybuf, unsigned int len, std::vector<unsigned int> path);
    int insert(std::vector<unsigned int> path, std::vector<struct iovec>& iov);

    // 新分配一个indexblock，返回blockid，但并没有将该block插入数据链上
    unsigned int allocate();
    // 回收一个block
    void deallocate(unsigned int blockid);
};


} // namespace db

#endif // __DB_BPlusTree_H__
