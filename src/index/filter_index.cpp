#include "index/filter_index.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <set>
#include <sstream>
#include "logger.h"  // 包含 logger.h 以使用日志记录器
#include "logger/logger.h"
#include "snappy.h"
namespace vectordb {

vectordb::FilterIndex::FilterIndex() = default;

void FilterIndex::AddIntFieldFilter(const std::string &fieldname, int64_t value, uint64_t id) {
  roaring_bitmap_t *bitmap = roaring_bitmap_create();
  roaring_bitmap_add(bitmap, id);
  int_field_filter_[fieldname][value] = bitmap;
  global_logger->debug("Added int field filter: fieldname={}, value={}, id={}", fieldname, value, id);  // 添加打印信息
}

void FilterIndex::UpdateIntFieldFilter(const std::string &fieldname, int64_t *old_value, int64_t new_value,
                                       uint64_t id) {  // 将 old_value 参数更改为指针类型
  if (old_value != nullptr) {
    global_logger->debug("Updated int field filter: fieldname={}, old_value={}, new_value={}, id={}", fieldname,
                         *old_value, new_value, id);
  } else {
    global_logger->debug("Updated int field filter: fieldname={}, old_value=nullptr, new_value={}, id={}", fieldname,
                         new_value, id);
  }

  auto it = int_field_filter_.find(fieldname);
  if (it != int_field_filter_.end()) {
    std::map<int64_t, roaring_bitmap_t *> &value_map = it->second;

    // 查找旧值对应的位图，并从位图中删除 ID
    auto old_bitmap_it =
        (old_value != nullptr) ? value_map.find(*old_value) : value_map.end();  // 使用解引用的 old_value
    if (old_bitmap_it != value_map.end()) {
      roaring_bitmap_t *old_bitmap = old_bitmap_it->second;
      roaring_bitmap_remove(old_bitmap, id);
    }

    // 查找新值对应的位图，如果不存在则创建一个新的位图
    auto new_bitmap_it = value_map.find(new_value);
    if (new_bitmap_it == value_map.end()) {
      roaring_bitmap_t *new_bitmap = roaring_bitmap_create();
      value_map[new_value] = new_bitmap;
      new_bitmap_it = value_map.find(new_value);
    }

    roaring_bitmap_t *new_bitmap = new_bitmap_it->second;
    roaring_bitmap_add(new_bitmap, id);
  } else {
    AddIntFieldFilter(fieldname, new_value, id);
  }
}

void FilterIndex::GetIntFieldFilterBitmap(const std::string &fieldname, Operation op, int64_t value,
                                          roaring_bitmap_t *result_bitmap) {  // 添加 result_bitmap 参数
  auto it = int_field_filter_.find(fieldname);
  if (it != int_field_filter_.end()) {
    auto &value_map = it->second;

    if (op == Operation::EQUAL) {
      auto bitmap_it = value_map.find(value);
      if (bitmap_it != value_map.end()) {
        global_logger->debug("Retrieved EQUAL bitmap for fieldname={}, value={}", fieldname, value);
        roaring_bitmap_overwrite(result_bitmap, bitmap_it->second);  // 更新 result_bitmap
      }
    } else if (op == Operation::NOT_EQUAL) {
      for (const auto &entry : value_map) {
        if (entry.first != value) {
          roaring_bitmap_overwrite(result_bitmap, entry.second);  // 更新 result_bitmap
        }
      }
      global_logger->debug("Retrieved NOT_EQUAL bitmap for fieldname={}, value={}", fieldname, value);
    }
  }
}

auto FilterIndex::SerializeIntFieldFilter() -> std::string {
  std::ostringstream oss;

  for (const auto &field_entry : int_field_filter_) {
    const std::string &field_name = field_entry.first;
    const std::map<int64_t, roaring_bitmap_t *> &value_map = field_entry.second;

    for (const auto &value_entry : value_map) {
      int64_t value = value_entry.first;
      const roaring_bitmap_t *bitmap = value_entry.second;

      // 将位图序列化为字节数组
      uint32_t size = roaring_bitmap_portable_size_in_bytes(bitmap);
      char *serialized_bitmap = new char[size];
      roaring_bitmap_portable_serialize(bitmap, serialized_bitmap);

      // 将字段名、值和序列化的位图写入输出流
      oss << field_name << "|" << value << "|";
      oss.write(serialized_bitmap, size);

      delete[] serialized_bitmap;
    }
  }

  return oss.str();
}

void FilterIndex::DeserializeIntFieldFilter(const std::string &serialized_data) {
  std::istringstream iss(serialized_data);
  
  // 是否需要clear int_field_filter_? 反正调用前先clear了

  std::string line;
  while (std::getline(iss, line)) {
    std::istringstream line_iss(line);

    // 从输入流中读取字段名、值和序列化的位图
    std::string field_name;
    std::getline(line_iss, field_name, '|');

    std::string value_str;
    std::getline(line_iss, value_str, '|');
    int64_t value = std::stol(value_str);

    // 读取序列化的位图
    std::string serialized_bitmap(std::istreambuf_iterator<char>(line_iss), {});

    // 反序列化位图
    roaring_bitmap_t *bitmap = roaring_bitmap_portable_deserialize(serialized_bitmap.data());

    // 将反序列化的位图插入 intFieldFilter
    int_field_filter_[field_name][value] = bitmap;
  }
}

void FilterIndex::SaveIndex(const std::string &path) {  // 添加 key 参数
  std::string serialized_data = SerializeIntFieldFilter();
  std::fstream index_file;  // 将 wal_log_file_ 类型更改为 std::fstream

  // 将序列化的数据存储到文件

  if (!std::filesystem::exists(path)) {
    // 文件不存在，先创建文件
    std::ofstream temp_file(path);
    temp_file.close();
  }

  index_file.open(path,  std::ios::in |std::ios::out |std::ios::trunc);  // 以 std::ios::in | std::ios::out | std::ios::app 模式打开文件
  if (!index_file.is_open()) {
    global_logger->error("An error occurred while writing the filter index entry. Reason: {}",
                         std::strerror(errno));  // 使用日志打印错误消息和原因
    throw std::runtime_error("Failed to open filter index file at path: " + path);
  }

  // 压缩日志条目
  std::string compressed_data;
  snappy::Compress(serialized_data.c_str(), serialized_data.size(), &compressed_data);

  // 写入压缩后的日志条目到文件
  index_file << compressed_data << std::endl;

  if (index_file.fail()) {  // 检查是否发生错误
    global_logger->error("An error occurred while writing the filter index file. Reason: {}",
                         std::strerror(errno));  // 使用日志打印错误消息和原因
  } else {
    global_logger->debug("Wrote filter index file");  // 打印日志
    index_file.flush();                               // 强制持久化
  }
  index_file.close();
}

void FilterIndex::LoadIndex(const std::string &path) {  // 添加 key 参数
  std::string compressed_line;
  std::fstream index_file;
  if (!std::filesystem::exists(path)) {
    // 文件不存在，先创建文件
    std::ofstream temp_file(path);
    temp_file.close();
  }

  index_file.open(path, std::ios::in | std::ios::out |
                            std::ios::app);  // 以 std::ios::in | std::ios::out | std::ios::app 模式打开文件
  if (!index_file.is_open()) {
    global_logger->error("An error occurred while load the filter index entry. Reason: {}",
                         std::strerror(errno));  // 使用日志打印错误消息和原因
    throw std::runtime_error("Failed to load filter index file at path: " + path);
  }
  std::string decompressed_data;
  if (std::getline(index_file, compressed_line)) {
    
    if (!snappy::Uncompress(compressed_line.c_str(), compressed_line.size(), &decompressed_data)) {
      global_logger->error("Failed to decompress WAL log entry");
      return;
    }
  } else {
    index_file.clear();
    global_logger->debug("No more filter index file to read");
  }
  index_file.close();
  // 从序列化的数据中反序列化 intFieldFilter
  int_field_filter_.clear();
  DeserializeIntFieldFilter(decompressed_data);
}

}  // namespace vectordb