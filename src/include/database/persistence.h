#pragma once

#include <string>
#include <fstream>
#include <cstdint> // 包含 <cstdint> 以使用 uint64_t 类型
#include <rapidjson/document.h> // 包含 rapidjson/document.h 以使用 JSON 对象
#include <snappy/snappy.h>
namespace vectordb {

class Persistence {
public:
    Persistence();
    ~Persistence();

    void Init(const std::string& local_path); // 添加 init 方法声明
    auto IncreaseId() -> uint64_t;
    auto GetId() const -> uint64_t;
    void WriteWalLog(const std::string& operation_type, const rapidjson::Document& json_data, const std::string& version); // 添加 version 参数
    void ReadNextWalLog(std::string* operation_type, rapidjson::Document* json_data); // 更改返回类型为 void 并添加指针参数

private:
    uint64_t increase_id_;
    std::fstream wal_log_file_; // 将 wal_log_file_ 类型更改为 std::fstream
};

}  // namespace vectordb