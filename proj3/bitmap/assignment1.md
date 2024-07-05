# Project x. 基于bitmap的标签倒排索引

标签tag是一种分类标识，通常附着在对象上，作为对象的属性。假设一个对象id，该对象对应于一系列标签，或者说该对象被分类到一系列的标签下。将id看成一个web页面，标签看成是这个页面上的关键字，本项目尝试构建标签到id的倒排索引。

## bitmap

在标签倒排索引中，使用位图Bitmap来表示标签的存在或缺失是一种高效的方法。然而，位图可能会占用大量内存，因此压缩位图是提高性能和减少存储空间的关键。以下是一些压缩位图的常见技术：

### Run-Length Encoding (RLE)

RLE 是一种简单且有效的压缩方法。
连续相同的位值被编码为一个计数和一个位值。
例如，000000111111100000 可以被压缩为 6个0 + 6个1 + 5个0。

### Word-Aligned Hybrid (WAH)

WAH 是一种高度压缩的位图表示方法。
将位图分成块，每个块包含一个控制位和一组数据位。
控制位指示数据位的类型（全0、全1或混合）。
WAH 可以有效地压缩位图，但查询速度可能较慢。

### Elias-Fano Encoding

Elias-Fano 是一种基于分割的压缩方法。
将位图分成两部分：高位和低位。
高位存储每个1的位置，低位存储偏移量。
查询速度较快，但需要更多的存储空间。

### Roaring Bitmaps

Roaring Bitmaps 是一种用于压缩位图的数据结构。
它将位图分成多个块，每个块使用不同的压缩方法。
Roaring Bitmaps 在查询速度和存储效率之间取得了良好的平衡。

## 要求

1. 对象定义为某个id；

2. 每个id标记了一系列标签{tag}；

3. 对象的id -> {tag}的映射存储在一个文件中，每一行对应于这样一个映射，每行采用csv格式；

```
id1 | tag1 | tag2 | tag3
id2 | tag1 | tag4 | tag5
```

4. 这个文件以增量形式追加，不考虑对这个文件内容的修改或者删除；

5. 采用bitmap构造一个标签倒排索引，每个tag对应于多个id，每个id在该标签的bitmap上标注。提供tag到bitmap的查询接口，基于bitmap支持多标签的交并差操作。同时提供id -> {tag}的正向接口。

6. 由于无法预计标签数目，bitmap的宽度要足够大，因此需要引入bitmap压缩算法；

## 参考链接

@misc{190810590:online,
author = {},
title = {[1908.10598] Techniques for Inverted Index Compression},
howpublished = {\url{https://arxiv.org/abs/1908.10598}},
month = {},
year = {},
note = {(Accessed on 03/27/2024)}
}

@misc{Optimizi20:online,
author = {},
title = {Optimizing partitioning strategies for faster inverted index compression | Frontiers of Computer Science},
howpublished = {\url{https://link.springer.com/article/10.1007/s11704-016-6252-5}},
month = {},
year = {},
note = {(Accessed on 03/27/2024)}
}

@misc{ChenDCC224:online,
author = {},
title = {ChenDCC2010.pdf},
howpublished = {\url{https://stanford.edu/%7Ebgirod/pdfs/ChenDCC2010.pdf}},
month = {},
year = {},
note = {(Accessed on 03/27/2024)}
}

@misc{lecture150:online,
author = {},
title = {lecture16-2.pdf},
howpublished = {\url{https://people.csail.mit.edu/jshun/6506-s23/lectures/lecture16-2.pdf}},
month = {},
year = {},
note = {(Accessed on 03/27/2024)}
}

@misc{1908105999:online,
author = {},
title = {[1908.10598] Techniques for Inverted Index Compression},
howpublished = {\url{https://arxiv.org/abs/1908.10598}},
month = {},
year = {},
note = {(Accessed on 03/27/2024)}
}

@misc{Techniqu76:online,
author = {},
title = {Techniques for Inverted Index Compression | ACM Computing Surveys},
howpublished = {\url{https://dl.acm.org/doi/10.1145/3415148}},
month = {},
year = {},
note = {(Accessed on 03/27/2024)}
}

https://github.com/RoaringBitmap/CRoaring

https://github.com/chiendo97/inverted_index
