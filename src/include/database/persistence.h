#pragma once

#include <string>
#include <fstream>
#include <cstdint> // 包含 <cstdint> 以使用 uint64_t 类型
#include <rapidjson/document.h> // 包含 rapidjson/document.h 以使用 JSON 对象
#include <snappy/snappy.h>
#include "index/index_factory.h"
#include "common/vector_cfg.h"
namespace vectordb {

class Persistence {
public:
    Persistence();
    ~Persistence();

    void Init(const std::string& local_path); // 添加 init 方法声明
    auto IncreaseId() -> uint64_t;
    auto GetId() const -> uint64_t;
    void WriteWalLog(const std::string& operation_type, const rapidjson::Document& json_data, const std::string& version); // 添加 version 参数
    void WriteWalRawLog(uint64_t log_id, const std::string& operation_type, const std::string& raw_data, const std::string& version); // 添加 writeWALRawLog 函数声明
    void ReadNextWalLog(std::string* operation_type, rapidjson::Document* json_data); // 更改返回类型为 void 并添加指针参数
    void TakeSnapshot(); 
    void LoadSnapshot(); // 添加 loadSnapshot 方法声明
    void SaveLastSnapshotId(const std::string& folder_path); // 添加 saveLastSnapshotID 方法声明
    void LoadLastSnapshotId(const std::string& folder_path); // 添加 loadLastSnapshotID 方法声明


private:
    uint64_t increase_id_;
    uint64_t last_snapshot_id_; // 添加 lastSnapshotID_ 成员变量
    std::fstream wal_log_file_; // 将 wal_log_file_ 类型更改为 std::fstream
};

}  // namespace vectordb