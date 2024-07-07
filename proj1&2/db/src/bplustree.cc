////
// @file bplustree.cc
// @brief
// 实现存储管理
//
// @author niexw
// @email niexiaowen@uestc.edu.cn
//
#include <db/bplustree.h>

namespace db {

// 设定table，填充b+tree的info_(部分)，遍历table的块，插入索引记录
bool BPlusTree::init(Table* table)
{
    if (!table) return false;
    if (root_) return true; // 已经初始化过

    RelationInfo* info = table->info_;
    unsigned int key = info->key;
    table_ = table;
    info_->key = 0;
    info_->type = info->type;
    info_->count = 2;
    info_->fields[0] = info->fields[key];

    // 分配一个块，作为根节点
    IndexBlock root;
    root.setTable(table_);
    root.setBtree(this);
    root_ = allocate();
    BufDesp *desp = kBuffer.borrow(table_->name_.c_str(), root_);
    root.attach(desp->buffer);

    if (table_->dataCount() == 1)
    {
        root.setNext(table_->beginblock()->getSelf());
    }
    else
    {
        for (auto bi = table->beginblock(); bi != table->endblock(); ++bi)
        {
            unsigned int blkid = bi->getSelf();
            auto ri = bi->beginrecord();
            std::vector<struct iovec> iov;
            unsigned char header;
            ri->ref(iov, &header);
            insert(blkid, iov);
        }
    }
    return true;
}

// btree搜索
unsigned int BPlusTree::search(void* keybuf, unsigned int len, unsigned int cur_root)
{
    if (!root_) init(table_);
    IndexBlock current;
    BufDesp *desp = kBuffer.borrow(table_->name_.c_str(), cur_root);
    current.attach(desp->buffer);
    
    // 根节点没有slots，说明只有一个datablock，返回getNext
    if (!current.getSlots()) return current.getNext();
    
    unsigned short index = current.searchRecord(keybuf, len);
    Record record;
    bool ret = current.refslots(index, record);
    if (!ret) // 比所有key值都大的话
    {
        current.refslots(current.getSlots() - 1, record);
        unsigned char* pkey;
        unsigned int plen;
        record.refByIndex(&pkey, &plen, 1);
        unsigned int blkid;
        memcpy(&blkid, pkey, plen);
        blkid = be64toh(blkid);
        IndexBlock next;
        BufDesp *desp = kBuffer.borrow(table_->name_.c_str(), blkid);
        next.attach(desp->buffer);
        if (next.getType() == BLOCK_TYPE_DATA) return blkid;
        else search(keybuf, len, blkid);
    }
    else
    {
        unsigned char* pkey;
        unsigned int plen;
        record.refByIndex(&pkey, &plen, 0);
        if (memcmp(pkey, keybuf, len) == 0)
        {
            unsigned int blkid;
            memcpy(&blkid, pkey, plen);
            blkid = be64toh(blkid);
            IndexBlock next;
            BufDesp* desp = kBuffer.borrow(table_->name_.c_str(), blkid);
            next.attach(desp->buffer);
            if (next.getType() == BLOCK_TYPE_DATA) return blkid;
            else search(keybuf, len, blkid);
        }
    }

}

int BPlusTree::insert(unsigned int blkid, std::vector<struct iovec>& iov)
{
    
}

unsigned int BPlusTree::allocate()
{
    // 空闲链上有block
    IndexBlock data;
    SuperBlock super;
    BufDesp *desp;

    if (table_->idle_) {
        // 读idle块，获得下一个空闲块
        desp = kBuffer.borrow(table_->name_.c_str(), table_->idle_);
        data.attach(desp->buffer);
        unsigned int next = data.getNext();
        data.detach();
        desp->relref();

        // 读超块，设定空闲块
        desp = kBuffer.borrow(table_->name_.c_str(), 0);
        super.attach(desp->buffer);
        super.setIdle(next);
        super.setIdleCounts(super.getIdleCounts() - 1);
        //super.setDataCounts(super.getDataCounts() + 1);
        super.setChecksum();
        super.detach();
        kBuffer.writeBuf(desp);
        desp->relref();

        unsigned int current = table_->idle_;
        table_->idle_ = next;

        desp = kBuffer.borrow(table_->name_.c_str(), current);
        data.attach(desp->buffer);
        data.clear(1, current, BLOCK_TYPE_INDEX);
        desp->relref();

        return current;
    }

    // 没有空闲块
    ++table_->maxid_;
    // 读超块，设定空闲块
    desp = kBuffer.borrow(table_->name_.c_str(), 0);
    super.attach(desp->buffer);
    super.setMaxid(table_->maxid_);
    //super.setDataCounts(super.getDataCounts() + 1);
    super.setChecksum();
    super.detach();
    kBuffer.writeBuf(desp);
    desp->relref();
    // 初始化数据块
    desp = kBuffer.borrow(table_->name_.c_str(), table_->maxid_);
    data.attach(desp->buffer);
    data.clear(1, table_->maxid_, BLOCK_TYPE_INDEX);
    desp->relref();

    return table_->maxid_;
}

void BPlusTree::deallocate(unsigned int blockid)
{
    // 读idle块，获得下一个空闲块
    DataBlock data;
    BufDesp *desp = kBuffer.borrow(table_->name_.c_str(), blockid);
    data.attach(desp->buffer);
    data.setNext(table_->idle_);
    data.setChecksum();
    data.detach();
    kBuffer.writeBuf(desp);
    desp->relref();

    // 读超块，设定空闲块
    SuperBlock super;
    desp = kBuffer.borrow(table_->name_.c_str(), 0);
    super.attach(desp->buffer);
    super.setIdle(blockid);
    super.setIdleCounts(super.getIdleCounts() + 1);
    //super.setDataCounts(super.getDataCounts() - 1);
    super.setChecksum();
    super.detach();
    kBuffer.writeBuf(desp);
    desp->relref();

    // 设定自己
    table_->idle_ = blockid;
}

} // namespace db