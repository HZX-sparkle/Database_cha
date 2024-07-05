# proj3

## 目录结构

工程建议在Linux下实现，目录结构：

- include   头文件
- src       源文件
- tests     单元测试
- third     第三方库

## Linux编译

```bash
mkdir build
cd build
cmake ..
make
./bin/btest
```

## windows编译

```bash
mkdir build
cd build
cmake ..
devenv proj3.sln /build Release /project all_build
.\bin\btest
```