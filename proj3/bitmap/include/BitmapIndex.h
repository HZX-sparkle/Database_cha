#ifndef BITMAP_INDEX_H
#define BITMAP_INDEX_H

#include <unordered_map>
#include <vector>
#include <string>
#include "roaring.hh"  // 引入CRoaring库的头文件

using namespace roaring;

class BitmapIndex {
public:
    bool readCSV(const std::string &filename);
    void buildInvertedIndex();
    Roaring getBitmap(const std::string &tag) const;
    std::vector<std::string> getTags(const std::string &id) const;
    static Roaring andOperation(const std::vector<Roaring> &bitmaps);
    static Roaring orOperation(const std::vector<Roaring> &bitmaps);
    static Roaring xorOperation(const std::vector<Roaring> &bitmaps);

private:
    std::unordered_map<std::string, std::vector<std::string>> idToTags;
    std::unordered_map<std::string, Roaring> tagToBitmap;
};

#endif // BITMAP_INDEX_H
