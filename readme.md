# vectorDB

a vector database implementation based on 《从零构建向量数据库》

支持FaissIndex，HnswlibIndex；
支持标量向量混合查询；
支持数据持久化存储；
使用http请求对数据库发起访问，插入或查询vector

## How to build

### Prepare environment variables

```shell
echo "export CODE_BASE=_______" >> ~/.bashrc  # openGauss-server的路径，例如/home/zhouzj/vectorDB/vectorDB
source ~/.bashrc
```

### Build third_party

Switch to the project directory:

```shell
cd third_party
bash build.sh
```
you can use
```shell
bash build.sh --help to see more detail
```

### Build vectorDB

Switch to the project directory:

```shell
$ mkdir build
$ cd build
$ cmake ..
$ make
```

If you want to compile the system in debug mode, pass in the following flag to cmake:
Debug mode:

```shell
$ cmake -DCMAKE_BUILD_TYPE=Debug ..
$ make -j`nproc`
```

## How to use

### Edit vectordb_config
You can find vectordb_config under project directory;
You can modify the content of each item according to your preferences.
See the example and explaination in common/vector_cfg.cpp

### Build directory
According to your vectordb_config,you should make sure these path exist;
like this:
```shell
mkdir ~/vectordb/
cd ~/vectordb
mkdir snap
mkdir storage

mkdir ~/test_vectordb/
cd ~/test_vectordb
mkdir snap
mkdir storage
```
When you use vdb_server you will use ~/vectordb,when vdb_server is restarted, the contents will be retained
If you want to reset, you can remove these and create again like this:
``` shell
cd ~/vectordb
rm -rf wal snap* storage*
mkdir snap
mkdir storage
```

### Start Server
Switch to the project directory:

```shell
cd bin
./vdb_server
```

### Operation
You can open another terminal,and input commands, following the example commands in `test/test.h`.
Remember to modify the port to keep it consistent with the one in vectordb_config

### Use Google Test
You can build and use different google test like this

Switch to the project directory:
```shell
$ mkdir build
$ cd build
$ cmake ..
$ make faiss_index_test
$ cd test
$ .faiss_index_test
```

## Reference

### Book

* 《从零构建向量数据库》

### Third-Party Libraries

* faiss
* hnswlib
* openblas
* brpc
* rapidjson
* httplib
* spdlog
* gflags
* protobuf
* glog
* crypto
* leveldb
* ssl
* z
* rocksdb
* snappy
* lz4
* bz2
* roaring
* gtest
* backward-cpp

## License

vectorDB is licensed under the MIT License. For more details, please refer to the [LICENSE](./LICENSE) file.

## Optimization
目前已完成《从零构建向量数据库》前四章所有内容，并在此之上做如下优化：

### 代码架构
将代码结构化，添加CMakeList一键编译；
添加三方库一键编译；

### 添加vectordb_config文件
将涉及的路径、端口等用config文件进行配置
读取config文件进行配置解析，采用Cfg单例来读取解析后的配置内容

### 单元测试
添加google test并加入cmake，支持编译运行单元测试
目前添加了部分单元测试

### 堆栈跟踪美化打印
添加backward-cpp
只需在CMakeList中添加需要backward的target即可
例如add_backward(vdb_server)

### 添加代码规范检查 
添加 clang-tidy,clang-format

### 添加单例抽象类
添加单例模板类，将IndexFactory改造为真正的单例模式





