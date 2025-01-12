# vectorDB

a vector database implementation based on 《从零构建向量数据库》

支持FaissIndex，HnswlibIndex；
支持标量向量混合查询；
支持数据持久化存储；
使用http请求对数据库发起访问，插入或查询vector

## How to build
### 安装通用依赖库

Ubuntu:
``` shell
sudo apt-get install cmake openssl libssl-dev libz-dev gfortran
```

### Prepare environment variables

```shell
echo "export VECTORDB_CODE_BASE=_______" >> ~/.bashrc  #下载后的代码根路径 例如/home/zhouzj/project/vectorDB
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

### Build protobuf
After change the third_part/proto, you should run
``` shell
cd third_party
bash build.sh --package protobuf
```
to rebuild .pb.h and .pb.cc

### Start Server
Switch to the project directory:

```shell
cd build
./bin/vdb_server
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


### Build Cluster

Switch to the project directory:

```shell
cd build
./bin/vdb_server 1
```

在另一终端中
```shell
cd build
./bin/vdb_server 2
```
1，2为nodeID
只要在vectordb_config中配置好node信息,即可在终端中执行```./vdb_server $nodeid```

选取其中一个作为主节点，将其他节点作为从节点加入集群:
```shell
curl -X POST -H "Content-Type: application/json" -d '{"nodeId": 2, "endpoint": "127.0.0.1:8082"}' http://localhost:7781/AdminService/AddFollower
```
想让哪个节点作为主节点，就向该节点port发出请求，-d后为待加入的从节点信息，一次加入一个，加入完毕后通过List来查看是否已加入集群:
```shell
curl -X GET http://localhost:7781/AdminService/ListNode
```
此外可以通过GetNode来查看当前节点状态
```shell
curl -X GET http://localhost:7781/AdminService/GetNode
```

向主节点中发起upsert请求，从节点中进行search，也可查到数据;
```shell
curl -X POST -H "Content-Type: application/json" -d '{"vectors": [0.999], "id":6, "int_field":47,"indexType": "FLAT"}'  http://localhost:7781/UserService/upsert

curl -X POST -H "Content-Type: application/json" -d '{"vectors": [0.999], "k": 5 , "indexType": "FLAT","filter":{"fieldName":"int_field","value":47,"op":"="}}'  http://localhost:7782/UserService/search
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

### Repository
非常感谢 Xiaoccer , third_party下的patches,build.sh,CMakeLists.txt基于 [TineVecDB](https://github.com/Xiaoccer/TinyVecDB.git) 中third_party下的内容进行修改

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

### 压缩WAL日志文件
采用snappy压缩算法，对WAL进行压缩及解压缩，减少WAL文件所占空间




