#include "database/persistence.h"
#include <rapidjson/document.h>      // 包含 <rapidjson/document.h> 以使用 rapidjson::Document 类型
#include <rapidjson/stringbuffer.h>  // 包含 rapidjson/stringbuffer.h 以使用 StringBuffer 类
#include <rapidjson/writer.h>        // 包含 rapidjson/writer.h 以使用 Writer 类
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include "common/vector_utils.h"
#include "logger/logger.h"
namespace vectordb {

Persistence::Persistence() : increase_id_(10), last_snapshot_id_(0) {}

Persistence::~Persistence() {
  if (wal_log_file_.is_open()) {
    wal_log_file_.close();
  }
}

void Persistence::Init(const std::string &local_path) {
  if (!std::filesystem::exists(local_path)) {
    // 文件不存在，先创建文件
    std::ofstream temp_file(local_path);
    temp_file.close();
  }

  wal_log_file_.open(local_path, std::ios::in | std::ios::out |
                                     std::ios::app);  // 以 std::ios::in | std::ios::out | std::ios::app 模式打开文件
  if (!wal_log_file_.is_open()) {
    global_logger->error("An error occurred while writing the WAL log entry. Reason: {}",
                         std::strerror(errno));  // 使用日志打印错误消息和原因
    throw std::runtime_error("Failed to open WAL log file at path: " + local_path);
  }

  LoadLastSnapshotId(Cfg::Instance().SnapPath());
}

auto Persistence::IncreaseId() -> uint64_t {
  increase_id_++;
  return increase_id_;
}

auto Persistence::GetId() const -> uint64_t { return increase_id_; }

void Persistence::WriteWalLog(const std::string &operation_type, const rapidjson::Document &json_data,
                              const std::string &version) {  // 添加 version 参数
  uint64_t log_id = IncreaseId();

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  json_data.Accept(writer);

  std::string json_data_str = buffer.GetString();
  // 拼接日志条目
  std::ostringstream oss;
  oss << log_id << "|" << version << "|" << operation_type << "|" << json_data_str;

  // 压缩日志条目
  std::string compressed_data;
  snappy::Compress(oss.str().c_str(), oss.str().size(), &compressed_data);

  // 写入压缩后的日志条目到文件
  wal_log_file_ << compressed_data << std::endl;

  if (wal_log_file_.fail()) {  // 检查是否发生错误
    global_logger->error("An error occurred while writing the WAL log entry. Reason: {}",
                         std::strerror(errno));  // 使用日志打印错误消息和原因
  } else {
    global_logger->debug("Wrote WAL log entry: log_id={}, version={}, operation_type={}, json_data_str={}", log_id,
                         version, operation_type, buffer.GetString());  // 打印日志
    wal_log_file_.flush();                                              // 强制持久化
  }
}

void Persistence::WriteWalRawLog(uint64_t log_id, const std::string &operation_type, const std::string &raw_data,
                                 const std::string &version) {
  // 拼接日志条目
  std::ostringstream oss;
  oss << log_id << "|" << version << "|" << operation_type << "|" << raw_data;

  // 压缩日志条目
  std::string compressed_data;
  snappy::Compress(oss.str().c_str(), oss.str().size(), &compressed_data);

  // 写入压缩后的日志条目到文件
  wal_log_file_ << compressed_data << std::endl;

  if (wal_log_file_.fail()) {  // 检查是否发生错误
    global_logger->error("An error occurred while writing the WAL raw log entry. Reason: {}",
                         std::strerror(errno));  // 使用日志打印错误消息和原因
  } else {
    global_logger->debug("Wrote WAL raw log entry: log_id={}, version={}, operation_type={}, raw_data={}", log_id,
                         version, operation_type, raw_data);  // 打印日志
    wal_log_file_.flush();                                    // 强制持久化
  }
}

void Persistence::ReadNextWalLog(std::string *operation_type, rapidjson::Document *json_data) {
  global_logger->debug("Reading next WAL log entry");

  std::string compressed_line;
  while (std::getline(wal_log_file_, compressed_line)) {
    std::string decompressed_data;
    if (!snappy::Uncompress(compressed_line.c_str(), compressed_line.size(), &decompressed_data)) {
      global_logger->error("Failed to decompress WAL log entry");
      return;
    }

    // 解析解压后的日志条目
    std::istringstream iss(decompressed_data);

    std::string log_id_str;
    std::string version;
    std::string json_data_str;

    std::getline(iss, log_id_str, '|');
    std::getline(iss, version, '|');
    std::getline(iss, *operation_type, '|');  // 使用指针参数返回 operation_type
    std::getline(iss, json_data_str, '|');

    uint64_t log_id = std::stoull(log_id_str);  // 将 log_id_str 转换为 uint64_t 类型
    if (log_id > increase_id_) {                // 如果 log_id 大于当前 increase_id_
      increase_id_ = log_id;                    // 更新 increase_id_
    }

    if (log_id > last_snapshot_id_) {
      json_data->Parse(json_data_str.c_str());  // 使用指针参数返回 json_data
      global_logger->debug("Read WAL log entry: log_id={}, operation_type={}, json_data_str={}", log_id_str,
                           *operation_type, json_data_str);
      return;
    }
    // TODO(zhouzj): 增加last_snapshot_id_ 前WAL LOG的清除
    global_logger->debug("Skip Read WAL log entry: log_id={}, operation_type={}, json_data_str={}", log_id_str,
                         *operation_type, json_data_str);
  }
  operation_type->clear();
  wal_log_file_.clear();
  global_logger->debug("No more WAL log entries to read");
}

void Persistence::TakeSnapshot() {          // 移除 takeSnapshot 方法的参数
  global_logger->debug("Taking snapshot");  // 添加调试信息

  last_snapshot_id_ = increase_id_;
  std::string snapshot_folder_path = Cfg::Instance().SnapPath();
  auto &index_factory = IndexFactory::Instance();  // 通过全局指针获取 IndexFactory 实例
  index_factory.SaveIndex(snapshot_folder_path);
  SaveLastSnapshotId(snapshot_folder_path);
}

void Persistence::LoadSnapshot() {           // 添加 loadSnapshot 方法实现
  global_logger->debug("Loading snapshot");  // 添加调试信息
  auto &index_factory = IndexFactory::Instance();
  std::string snapshot_folder_path = Cfg::Instance().SnapPath();
  index_factory.LoadIndex(snapshot_folder_path);  // 将 scalar_storage 传递给 loadIndex 方法
}

void Persistence::SaveLastSnapshotId(const std::string &folder_path) {  // 添加 saveLastSnapshotID 方法实现
  std::string file_path = folder_path + "MaxLogID";
  std::ofstream file("file_path");
  if (file.is_open()) {
    file << last_snapshot_id_;
    file.close();
  } else {
    global_logger->error("Failed to open file snapshots_MaxID for writing");
  }
  global_logger->debug("save snapshot Max log ID {}", last_snapshot_id_);  // 添加调试信息
}

void Persistence::LoadLastSnapshotId(const std::string &folder_path) {  // 添加 loadLastSnapshotID 方法实现
  std::string file_path = folder_path + ".MaxLogID";
  std::ifstream file("file_path");
  if (file.is_open()) {
    file >> last_snapshot_id_;
    file.close();
  } else {
    global_logger->warn("Failed to open file snapshots_MaxID for reading");
  }

  global_logger->debug("Loading snapshot Max log ID {}", last_snapshot_id_);  // 添加调试信息
}

}  // namespace vectordb