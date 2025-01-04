#include "database/vector_database.h"
#include <rapidjson/document.h>
#include <cstdint>
#include <vector>
#include "common/constants.h"
#include "database/scalar_storage.h"
#include "index/faiss_index.h"
#include "index/filter_index.h"
#include "index/hnswlib_index.h"
#include "index/index_factory.h"
#include "logger/logger.h"
#include <rapidjson/stringbuffer.h> // 包含 rapidjson/stringbuffer.h 以使用 StringBuffer 类
#include <rapidjson/writer.h> // 包含 rapidjson/writer.h 以使用 Writer 类

namespace vectordb {

VectorDatabase::VectorDatabase(const std::string &db_path, const std::string& wal_path) : scalar_storage_(db_path) {
    persistence_.Init(wal_path); // 初始化 persistence_ 对象
}

void VectorDatabase::ReloadDatabase() {
    global_logger->info("Entering VectorDatabase::reloadDatabase()"); // 在方法开始时打印日志

    persistence_.LoadSnapshot();

    std::string operation_type;
    rapidjson::Document json_data;
    persistence_.ReadNextWalLog(&operation_type, &json_data); // 通过指针的方式调用 readNextWALLog

    while (!operation_type.empty()) {
        global_logger->info("Operation Type: {}", operation_type);

        // 打印读取的一行内容fmt::detail::buffer
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        json_data.Accept(writer);
        global_logger->info("Read Line: {}", buffer.GetString());

       if (operation_type == "upsert") {
            uint64_t id = json_data[REQUEST_ID].GetUint64();
            IndexFactory::IndexType index_type = GetIndexTypeFromRequest(json_data);

            Upsert(id, json_data, index_type); // 调用 VectorDatabase::upsert 接口重建数据
        }

        // 清空 json_data
        rapidjson::Document().Swap(json_data);

        // 读取下一条 WAL 日志
        operation_type.clear();
        persistence_.ReadNextWalLog(&operation_type, &json_data);
    }
}

void VectorDatabase::WriteWalLog(const std::string& operation_type, const rapidjson::Document& json_data) {
    std::string version = "1.0"; // 您可以根据需要设置版本
    persistence_.WriteWalLog(operation_type, json_data, version); // 将 version 传递给 writeWALLog 方法
}
auto VectorDatabase::GetIndexTypeFromRequest(const rapidjson::Document& json_request) -> IndexFactory::IndexType {
    // 获取请求参数中的索引类型
    if (json_request.HasMember(REQUEST_INDEX_TYPE) && json_request[REQUEST_INDEX_TYPE].IsString()) {
        std::string index_type_str = json_request[REQUEST_INDEX_TYPE].GetString();
        if (index_type_str == INDEX_TYPE_FLAT) {
            return IndexFactory::IndexType::FLAT;
        } if (index_type_str == INDEX_TYPE_HNSW) {
            return IndexFactory::IndexType::HNSW;
        }
    }
    return IndexFactory::IndexType::UNKNOWN; // 返回UNKNOWN值
}

void VectorDatabase::Upsert(uint64_t id, const rapidjson::Document &data,
                            vectordb::IndexFactory::IndexType index_type) {
  // 检查标量存储中是否存在给定ID的向量
  rapidjson::Document existing_data;  // 修改为驼峰命名
  try {
    existing_data = scalar_storage_.GetScalar(id);
  } catch (const std::runtime_error &e) {
    // 向量不存在，继续执行插入操作
  }

  // 如果存在现有向量，则从索引中删除它
  if (existing_data.IsObject()) {  // 使用IsObject()检查existingData是否为空
    std::vector<float> existing_vector(existing_data["vectors"].Size());  // 从JSON数据中提取vectors字段
    for (rapidjson::SizeType i = 0; i < existing_data["vectors"].Size(); ++i) {
      existing_vector[i] = existing_data["vectors"][i].GetFloat();
    }

    void *index = IndexFactory::Instance().GetIndex(index_type);
    switch (index_type) {
      case IndexFactory::IndexType::FLAT: {
        auto *faiss_index = static_cast<FaissIndex *>(index);
        faiss_index->RemoveVectors({static_cast<int64_t>(id)});  // 将id转换为long类型
        break;
      }
      case IndexFactory::IndexType::HNSW: {
        auto *hnsw_index = static_cast<HNSWLibIndex *>(index);
        hnsw_index->RemoveVectors({static_cast<int64_t>(id)});
        break;
      }
      default:
        break;
    }
  }

  // 将新向量插入索引
  std::vector<float> new_vector(data["vectors"].Size());  // 从JSON数据中提取vectors字段
  for (rapidjson::SizeType i = 0; i < data["vectors"].Size(); ++i) {
    new_vector[i] = data["vectors"][i].GetFloat();
  }

  void *index = IndexFactory::Instance().GetIndex(index_type);
  switch (index_type) {
    case IndexFactory::IndexType::FLAT: {
      auto *faiss_index = static_cast<FaissIndex *>(index);
      faiss_index->InsertVectors(new_vector, static_cast<int64_t>(id));
      break;
    }
    case IndexFactory::IndexType::HNSW: {
      auto *hnsw_index = static_cast<HNSWLibIndex *>(index);
      hnsw_index->InsertVectors(new_vector, static_cast<int64_t>(id));
      break;
    }
    default:
      break;
  }

  global_logger->debug("try add new filter");  // 添加打印信息
  // 检查客户写入的数据中是否有 int 类型的 JSON 字段
  auto *filter_index = static_cast<FilterIndex *>(IndexFactory::Instance().GetIndex(IndexFactory::IndexType::FILTER));
  for (auto it = data.MemberBegin(); it != data.MemberEnd(); ++it) {
    std::string field_name = it->name.GetString();
    global_logger->debug("try filter member {} {}", it->value.IsInt(), field_name);  // 添加打印信息
    if (it->value.IsInt() && field_name != "id") {                                   // 过滤名称为 "id" 的字段
      int64_t field_value = it->value.GetInt64();
      int64_t *old_field_value_p = nullptr;
      // 如果存在现有向量，则从 FilterIndex 中更新 int 类型字段
      if (existing_data.IsObject()) {
        old_field_value_p = static_cast<int64_t *>(malloc(sizeof(int64_t)));
        *old_field_value_p = existing_data[field_name.c_str()].GetInt64();
      }
      filter_index->UpdateIntFieldFilter(field_name, old_field_value_p, field_value, id);
      delete old_field_value_p;
    }
  }

  // 更新标量存储中的向量
  scalar_storage_.InsertScalar(id, data);
}

auto VectorDatabase::Query(uint64_t id) -> rapidjson::Document {  // 添加query函数实现
  return scalar_storage_.GetScalar(id);
}


auto VectorDatabase::Search(const rapidjson::Document& json_request) -> std::pair<std::vector<int64_t>, std::vector<float>> {
    // 从 JSON 请求中获取查询参数
    std::vector<float> query;
    for (const auto& q : json_request[REQUEST_VECTORS].GetArray()) {
        query.push_back(q.GetFloat());
    }
    int k = json_request[REQUEST_K].GetInt();

    // 获取请求参数中的索引类型
    IndexFactory::IndexType index_type = IndexFactory::IndexType::UNKNOWN;
    if (json_request.HasMember(REQUEST_INDEX_TYPE) && json_request[REQUEST_INDEX_TYPE].IsString()) {
        std::string index_type_str = json_request[REQUEST_INDEX_TYPE].GetString();
        if (index_type_str == INDEX_TYPE_FLAT) {
            index_type = IndexFactory::IndexType::FLAT;
        } else if (index_type_str == INDEX_TYPE_HNSW) {
            index_type = IndexFactory::IndexType::HNSW;
        }
    }

    // 检查请求中是否包含 filter 参数
    roaring_bitmap_t* filter_bitmap = nullptr;
    if (json_request.HasMember("filter") && json_request["filter"].IsObject()) {
        const auto& filter = json_request["filter"];
        std::string field_name = filter["fieldName"].GetString();
        std::string op_str = filter["op"].GetString();
        int64_t value = filter["value"].GetInt64();

        FilterIndex::Operation op = (op_str == "=") ? FilterIndex::Operation::EQUAL : FilterIndex::Operation::NOT_EQUAL;

        // 通过 getGlobalIndexFactory 的 getIndex 方法获取 FilterIndex
        auto* filter_index = static_cast<FilterIndex*>(IndexFactory::Instance().GetIndex(IndexFactory::IndexType::FILTER));

        // 调用 FilterIndex 的 getIntFieldFilterBitmap 方法
        filter_bitmap = roaring_bitmap_create();
        filter_index->GetIntFieldFilterBitmap(field_name, op, value, filter_bitmap);
    }

    // 使用全局 IndexFactory 获取索引对象
    void* index = IndexFactory::Instance().GetIndex(index_type);

    // 根据索引类型初始化索引对象并调用 search_vectors 函数
    std::pair<std::vector<int64_t>, std::vector<float>> results;
    switch (index_type) {
        case IndexFactory::IndexType::FLAT: {
            auto* faiss_index = static_cast<FaissIndex*>(index);
            results = faiss_index->SearchVectors(query, k, filter_bitmap); // 将 filter_bitmap 传递给 search_vectors 方法
            break;
        }
        case IndexFactory::IndexType::HNSW: {
            auto* hnsw_index = static_cast<HNSWLibIndex*>(index);
            results = hnsw_index->SearchVectors(query, k, filter_bitmap); // 将 filter_bitmap 传递给 search_vectors 方法
            break;
        }
        // 在此处添加其他索引类型的处理逻辑
        default:
            break;
    }
    delete filter_bitmap;
    return results;
}
void VectorDatabase::TakeSnapshot() { // 添加 takeSnapshot 方法实现
    persistence_.TakeSnapshot();
}

auto VectorDatabase::GetStartIndexId() const -> int64_t {
    return persistence_.GetId(); // 通过调用 persistence_ 的 GetID 方法获取起始索引 ID
}

}  // namespace vectordb