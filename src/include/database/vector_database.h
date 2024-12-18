#pragma once

#include "database/scalar_storage.h"
#include "index/index_factory.h"
#include <string>
#include <vector>
#include <rapidjson/document.h>
#include "database/persistence.h"
namespace vectordb {

class VectorDatabase {
public:
    // 构造函数
    explicit VectorDatabase(const std::string& db_path,const std::string& wal_path);

    // 插入或更新向量
    void Upsert(uint64_t id, const rapidjson::Document& data, IndexFactory::IndexType index_type);
    auto Query(uint64_t id) -> rapidjson::Document; // 添加query接口
    auto Search(const rapidjson::Document& json_request) -> std::pair<std::vector<int64_t>, std::vector<float>>;
    void ReloadDatabase(); // 添加 reloadDatabase 方法声明
    void WriteWalLog(const std::string& operation_type, const rapidjson::Document& json_data); // 添加 writeWALLog 方法声明
    auto GetIndexTypeFromRequest(const rapidjson::Document& json_request) -> vectordb::IndexFactory::IndexType; 
private:
    ScalarStorage scalar_storage_;
    Persistence persistence_; // 添加 Persistence 对象
};
}  // namespace vectordb