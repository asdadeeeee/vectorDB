# build
## 三方库的引入
### 1. 纯头文件式三方库
直接下载源码，将头文件引入third/party CMakeList中增加THIRD_PARTY_INCLUDE_DIR，如 rapidjson
### 2. 系统库安装
apt-get等方式装在系统中，需要将三方库链接到目标库 ；如 faiss
如下：
```
set(VECTORDB_THIRDPARTY_LIBS
    faiss 
    blas
    OpenMP::OpenMP_CXX
    )


target_link_libraries(
    vectorDB
    ${VECTORDB_LIBS}
    ${VECTORDB_THIRDPARTY_LIBS})
```
### 3.三方库源码添加 额外编译 （一般源码自带CMakeLists），如googletest
在third_party/CMakeLists.txt中添加如下
```
add_subdirectory(googletest-1.15.2)
```
此外在test/CMakeLists.txt中添加
```
target_link_libraries(${vectordb_test_name} vectorDB gtest gmock_main)
```
这样在test文件中就可以正常#include "gtest/gtest.h"