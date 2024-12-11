#include "database/scalar_storage.h"
#include "logger/logger.h"
#include <rocksdb/db.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h> // 包含rapidjson/stringbuffer.h头文件
#include <rapidjson/writer.h>
#include <vector>
namespace vectordb {

ScalarStorage::ScalarStorage(const std::string& db_path) {
    rocksdb::Options options;
    options.create_if_missing = true;
    rocksdb::Status status = rocksdb::DB::Open(options, db_path, &db_);
    if (!status.ok()) {
        throw std::runtime_error("Failed to open RocksDB: " + status.ToString());
    }
}

ScalarStorage::~ScalarStorage() {
    delete db_;
}

void ScalarStorage::InsertScalar(uint64_t id, const rapidjson::Document& data) { // 将参数类型更改为rapidjson::Document
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    data.Accept(writer);
    std::string value = buffer.GetString();

    rocksdb::Status status = db_->Put(rocksdb::WriteOptions(), std::to_string(id), value);
    if (!status.ok()) {
        global_logger->error("Failed to insert scalar: {}", status.ToString()); // 使用GlobalLogger打印错误日志
    }
}

auto ScalarStorage::GetScalar(uint64_t id) -> rapidjson::Document { // 将返回类型更改为rapidjson::Document
    std::string value;
    rocksdb::Status status = db_->Get(rocksdb::ReadOptions(), std::to_string(id), &value);
    if (!status.ok()) {
        return rapidjson::Document(); // 返回一个空的rapidjson::Document对象
    }

    rapidjson::Document data;
    data.Parse(value.c_str());

    // 打印从ScalarStorage获取的数据和rocksdb::Status status
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    data.Accept(writer);
    global_logger->debug("Data retrieved from ScalarStorage: {}, RocksDB status: {}", buffer.GetString(), status.ToString()); // 添加rocksdb::Status status

    return data;
}
}  // namespace vectordb