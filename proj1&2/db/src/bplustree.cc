////
// @file bplustree.cc
// @brief
// 实现b+树
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
    RelationInfo btree_info;
    unsigned int key = info->key;
    table_ = table;
    btree_info.key = 0;
    btree_info.type = info->type;
    btree_info.count = 2;
    btree_info.fields.push_back(info->fields[key]);
    FieldInfo field;
    field.name = "blkid";
    field.index = 1;
    field.length = 8;
    field.type = findDataType("INT");
    btree_info.fields.push_back(field);
    info_ = &btree_info;

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
        desp->relref();
    }
    else
    {
        for (auto bi = table->beginblock(); bi != table->endblock(); ++bi)
        {
            std::vector<struct iovec> iov(2);
            unsigned int blkid = bi->getSelf();
            auto ri = bi->beginrecord();
            unsigned char* buf;
            unsigned int len;
            ri->refByIndex(&buf, &len, key);
            iov[0].iov_base = buf;
            iov[0].iov_len = len;
            findDataType("INT")->htobe(&blkid);
            iov[1].iov_base = &blkid;
            iov[1].iov_len = 8;
            std::vector<unsigned int> path;
            path.push_back(root_);
            desp->relref();
            insert(path, iov);
        }
    }
    return true;
}

// btree搜索
std::vector<unsigned int> BPlusTree::search(void* keybuf, unsigned int len, std::vector<unsigned int> path)
{
    if (!root_) init(table_);
    IndexBlock current;
    unsigned int cur_root = path.back();
    current.setBtree(this);
    BufDesp *desp = kBuffer.borrow(table_->name_.c_str(), cur_root);
    current.attach(desp->buffer);
    
    // 根节点没有slots，说明只有一个datablock，path加上getNext()
    if (!current.getSlots())
    {
        path.push_back(current.getNext());
        desp->relref();
        return path;
    }
    
    unsigned int blkid = 0;
    unsigned short index = current.searchRecord(keybuf, len);
    Record record;
    bool ret = current.refslots(index, record);
    // 比所有的key都大的话
    if (!ret) current.refslots(current.getSlots() - 1, record);
    else
    {
        unsigned char* pkey;
        unsigned int plen;
        record.refByIndex(&pkey, &plen, 0);
        // 不相等，说明小于
        if (memcmp(pkey, keybuf, len) != 0)
        {
            ret = current.refslots(index - 1, record);
            // 如果比所有的key都小
            if (!ret) blkid = current.getNext();
        }
    }
    if (blkid == 0)
    {
        unsigned char* pkey;
        unsigned int plen;
        record.refByIndex(&pkey, &plen, 1);
        memcpy(&blkid, pkey, plen);
        findDataType("INT")->betoh(&blkid);
    }
    IndexBlock next;
    next.setBtree(this);
    next.setTable(table_);
    kBuffer.releaseBuf(desp); // 释放buffer
    desp = kBuffer.borrow(table_->name_.c_str(), blkid);
    next.attach(desp->buffer);
    path.push_back(blkid);
    desp->relref();
    if (next.getType() == BLOCK_TYPE_DATA) return path;
    else return search(keybuf, len, path);
}

int BPlusTree::insert(std::vector<unsigned int> path, std::vector<struct iovec>& iov)
{
    std::pair<bool, unsigned short> ret;
    IndexBlock block;
    block.setBtree(this);
    block.setTable(table_);
    unsigned int cur_root = path.back();
    path.pop_back();
    BufDesp *bd = kBuffer.borrow(table_->name_.c_str(), cur_root);
    block.attach(bd->buffer);
    if (block.getType() == BLOCK_TYPE_DATA)
    {
        // 叶子节点，尝试插入
        SuperBlock super;
        ret = block.insertDataRecord(iov);
        if (ret.first) {
            kBuffer.releaseBuf(bd); // 释放buffer
            // 修改表头统计
            bd = kBuffer.borrow(table_->name_.c_str(), 0);
            super.attach(bd->buffer);
            super.setRecords(super.getRecords() + 1);
            bd->relref();
            return S_OK; // 插入成功
        } else if (ret.second == (unsigned short) -1) {
            kBuffer.releaseBuf(bd); // 释放buffer
            return EEXIST;          // key已经存在
        }
        // 需要分裂DataBlock
        unsigned short insert_position = ret.second;
        std::pair<unsigned short, bool> split_position =
            block.splitDataPosition(Record::size(iov), insert_position);

        // 先分配一个block
        DataBlock next;
        next.setTable(table_);
        unsigned int blkid = table_->allocate();
        BufDesp *bd2 = kBuffer.borrow(table_->name_.c_str(), blkid);
        next.attach(bd2->buffer);

        // 移动记录到新的block上
        while (block.getSlots() > split_position.first) {
            Record record;
            block.refslots(split_position.first, record);
            block.copyRecord(record);
            block.deallocate(split_position.first);
        }
        // 插入新记录，不需要再重排顺序
        if (split_position.second)
            block.insertDataRecord(iov);
        else
            next.insertRecord(iov);

        // 往父节点上插入，先构造记录
        Record *record = block.beginrecord().operator->();
        unsigned char* keybuf;
        unsigned int keylen;
        record->refByIndex(&keybuf, &keylen, table_->info_->key);
        std::vector<iovec> pair(2);
        pair[0].iov_base = keybuf;
        pair[0].iov_len = keylen;
        findDataType("INT")->htobe(&blkid);
        pair[1].iov_base = &blkid;
        pair[1].iov_len = 8;

        // 维持数据链
        next.setNext(block.getNext());
        block.setNext(next.getSelf());
        bd2->relref();

        bd = kBuffer.borrow(table_->name_.c_str(), 0);
        super.attach(bd->buffer);
        super.setRecords(super.getRecords() + 1);
        bd->relref();

        // 向父节点插入pair
        return insert(path, pair);
    }
    else
    {
        // 非叶子节点
        ret = block.insertRecord(iov);
        if (ret.first) {
            bd->relref();
            return S_OK; // 插入成功
        } else if (ret.second == (unsigned short) -1) {
            kBuffer.releaseBuf(bd); // 释放buffer
            return EEXIST;          // key已经存在
        }
        // 需要分裂非叶子节点
        unsigned short insert_position = ret.second;
        std::pair<unsigned short, bool> split_position =
            block.splitPosition(Record::size(iov), insert_position);

        // 先分配一个indexblock
        IndexBlock next;
        next.setBtree(this);
        next.setTable(table_);
        unsigned int blkid = allocate();
        BufDesp *bd2 = kBuffer.borrow(table_->name_.c_str(), blkid);
        next.attach(bd2->buffer);

        // 移动记录到新的indexblock上
        while (block.getSlots() > split_position.first) {
            Record record;
            block.refslots(split_position.first, record);
            block.copyRecord(record);
            block.deallocate(split_position.first);
        }

        // 插入新记录，不需要再重排顺序
        if (split_position.second)
            block.insertRecord(iov);
        else
            next.insertRecord(iov);

        // 往父节点上插入key-blkid，先构造记录
        Record *record = block.beginrecord().operator->();
        unsigned char* keybuf;
        unsigned int keylen;
        record->refByIndex(&keybuf, &keylen, 0);
        std::vector<iovec> pair(2);
        pair[0].iov_base = keybuf;
        pair[0].iov_len = keylen;
        findDataType("INT")->htobe(&blkid);
        pair[1].iov_base = &blkid;
        pair[1].iov_len = 8;

        // 设置next的Next，使其指向第一条记录的blkid，然后删除第一条记录
        unsigned int next_id;
        unsigned char* pkey;
        unsigned int plen;
        record->refByIndex(&pkey, &plen, 1);
        memcpy(&next_id, pkey, plen);
        findDataType("INT")->betoh(&next_id);
        next.setNext(next_id);
        next.deallocate(0);
        bd2->relref();

        if (cur_root == root_)
        {
            // 如果当前节点是根节点
            // 再分配一个indexblock作为根节点
            IndexBlock root;
            root.setBtree(this);
            root.setTable(table_);
            root_ = allocate();
            BufDesp *bd3 = kBuffer.borrow(table_->name_.c_str(), root_);
            root.attach(bd3->buffer);
            root.setNext(block.getSelf());

            path.push_back(root_);
            bd3->relref();
        }
        
        // 插入父节点
        return insert(path, pair);
    }
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
        //super.setIndexCounts(super.getIndexCounts() + 1);
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
    //super.setDataCounts(super.getIndexCounts() + 1);
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
    IndexBlock data;
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
    //super.setIndexCounts(super.getIndexCounts() - 1);
    super.setChecksum();
    super.detach();
    kBuffer.writeBuf(desp);
    desp->relref();

    // 设定自己
    table_->idle_ = blockid;
}

} // namespace db