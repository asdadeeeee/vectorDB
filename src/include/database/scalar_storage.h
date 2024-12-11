#pragma once

#include <rocksdb/db.h>
#include <string>
#include <vector>
#include <rapidjson/document.h> // 包含rapidjson头文件
namespace vectordb {
class ScalarStorage {
public:
    // 构造函数，打开RocksDB
    explicit ScalarStorage(const std::string& db_path);

    // 析构函数，关闭RocksDB
    ~ScalarStorage();

    // 向量插入函数
    void InsertScalar(uint64_t id, const rapidjson::Document& data); // 将参数类型更改为rapidjson::Document

    // 根据ID查询向量函数
    auto GetScalar(uint64_t id) -> rapidjson::Document; // 将返回类型更改为rapidjson::Document

private:
    // RocksDB实例
    rocksdb::DB* db_;
};
}  // namespace vectordb