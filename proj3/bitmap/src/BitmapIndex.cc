#include "BitmapIndex.h"
#include <fstream>
#include <sstream>
#include <iostream>

void BitmapIndex::readCSV(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    std::string line, id, tag;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::getline(ss, id, '|');
        std::vector<std::string> tags;
        while (std::getline(ss, tag, '|')) {
            tags.push_back(tag);
        }
        idToTags[id] = tags;
    }
}

void BitmapIndex::buildInvertedIndex() {
    for (const auto &entry : idToTags) {
        const std::string &id = entry.first;
        uint32_t id_int = std::stoi(id);
        const std::vector<std::string> &tags = entry.second;

        for (const std::string &tag : tags) {
            tagToBitmap[tag].add(id_int);
        }
    }
}

Roaring BitmapIndex::getBitmap(const std::string &tag) const {
    auto it = tagToBitmap.find(tag);
    if (it != tagToBitmap.end()) {
        return it->second;
    }
    return Roaring();
}

std::vector<std::string> BitmapIndex::getTags(const std::string &id) const {
    auto it = idToTags.find(id);
    if (it != idToTags.end()) {
        return it->second;
    }
    return {};
}

Roaring BitmapIndex::andOperation(const std::vector<Roaring> &bitmaps) {
    if (bitmaps.empty()) return Roaring();
    Roaring result = bitmaps[0];
    for (size_t i = 1; i < bitmaps.size(); ++i) {
        result &= bitmaps[i];
    }
    return result;
}

Roaring BitmapIndex::orOperation(const std::vector<Roaring> &bitmaps) {
    Roaring result;
    for (const auto &bitmap : bitmaps) {
        result |= bitmap;
    }
    return result;
}

Roaring BitmapIndex::xorOperation(const std::vector<Roaring> &bitmaps) {
    Roaring result;
    for (const auto &bitmap : bitmaps) {
        result ^= bitmap;
    }
    return result;
}
