#pragma once

#include "database/scalar_storage.h"
#include "index/index_factory.h"
#include <string>
#include <vector>
#include <rapidjson/document.h>
namespace vectordb {

class VectorDatabase {
public:
    // 构造函数
    explicit VectorDatabase(const std::string& db_path);

    // 插入或更新向量
    void Upsert(uint64_t id, const rapidjson::Document& data, IndexFactory::IndexType index_type);
    auto Query(uint64_t id) -> rapidjson::Document; // 添加query接口

private:
    ScalarStorage scalar_storage_;
};
}  // namespace vectordb