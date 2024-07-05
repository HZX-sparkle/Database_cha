# tests

单元测试采用gtest框架，需要**自行安装**gtest、gmock、google-benchmark三个库。如果没有性能测试，可以不安装goolge-benchmark。

~~~bash
# 解压
tar -zxf googletest-1.14.0.tar.gz
# 进入解压后的目录
cd googletest-1.14.0
# 为编译创建一个目录
mkdir build && cd build
cmake ..
make
# 安装
sudo make install
~~~
