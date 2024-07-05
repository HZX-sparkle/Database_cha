#include <iostream>
#include "BitmapIndex.h"

int main() {
    BitmapIndex index;
    
    // 读取CSV文件并构建映射关系
    index.readCSV("../data/tags.csv");

    // 构建倒排索引
    index.buildInvertedIndex();

    // 查询示例
    std::string tag = "tag1";
    Roaring bitmap = index.getBitmap(tag);
    std::cout << "IDs for " << tag << ": ";
    bitmap.printf();

    // 查询id的标签
    std::string id = "1";
    std::vector<std::string> tags = index.getTags(id);
    std::cout << "Tags for id " << id << ": ";
    for (const std::string &t : tags) {
        std::cout << t << " ";
    }
    std::cout << std::endl;

    // 多标签操作示例
    std::vector<Roaring> bitmaps = {index.getBitmap("tag1"), index.getBitmap("tag2")};
    Roaring andResult = BitmapIndex::andOperation(bitmaps);
    std::cout << "AND operation result: ";
    andResult.printf();

    return 0;
}
