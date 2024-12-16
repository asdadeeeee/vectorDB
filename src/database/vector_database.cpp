#include "database/vector_database.h"
#include "database/scalar_storage.h"
#include "index/index_factory.h"
#include "index/faiss_index.h"
#include "index/hnswlib_index.h"
#include <cstdint>
#include <vector>
#include <rapidjson/document.h>

namespace vectordb {

VectorDatabase::VectorDatabase(const std::string& db_path) : scalar_storage_(db_path) {}

void VectorDatabase::Upsert(uint64_t id, const rapidjson::Document& data, vectordb::IndexFactory::IndexType index_type) {
    // 检查标量存储中是否存在给定ID的向量
    rapidjson::Document existing_data; // 修改为驼峰命名
    try {
        existing_data = scalar_storage_.GetScalar(id);
    } catch (const std::runtime_error& e) {
        // 向量不存在，继续执行插入操作
    }

    // 如果存在现有向量，则从索引中删除它
    if (existing_data.IsObject()) { // 使用IsObject()检查existingData是否为空
        std::vector<float> existing_vector(existing_data["vectors"].Size()); // 从JSON数据中提取vectors字段
        for (rapidjson::SizeType i = 0; i < existing_data["vectors"].Size(); ++i) {
            existing_vector[i] = existing_data["vectors"][i].GetFloat();
        }

        void* index = IndexFactory::Instance().GetIndex(index_type);
        switch (index_type) {
            case IndexFactory::IndexType::FLAT: {
                auto* faiss_index = static_cast<FaissIndex*>(index);
                faiss_index->RemoveVectors({static_cast<int64_t>(id)}); // 将id转换为long类型
                break;
            }
            case IndexFactory::IndexType::HNSW: {
                auto* hnsw_index = static_cast<HNSWLibIndex*>(index);
                hnsw_index->RemoveVectors({static_cast<int64_t>(id)});
                break;
            }
            default:
                break;
        }
    }

    // 将新向量插入索引
    std::vector<float> new_vector(data["vectors"].Size()); // 从JSON数据中提取vectors字段
    for (rapidjson::SizeType i = 0; i < data["vectors"].Size(); ++i) {
        new_vector[i] = data["vectors"][i].GetFloat();
    }

    void* index = IndexFactory::Instance().GetIndex(index_type);
    switch (index_type) {
        case IndexFactory::IndexType::FLAT: {
            auto* faiss_index = static_cast<FaissIndex*>(index);
            faiss_index->InsertVectors(new_vector, static_cast<int64_t>(id));
            break;
        }
        case IndexFactory::IndexType::HNSW: {
            auto* hnsw_index = static_cast<HNSWLibIndex*>(index);
            hnsw_index->InsertVectors(new_vector, static_cast<int64_t>(id));
            break;
        }
        default:
            break;
    }

    // 更新标量存储中的向量
    scalar_storage_.InsertScalar(id, data);
}

auto VectorDatabase::Query(uint64_t id) -> rapidjson::Document { // 添加query函数实现
    return scalar_storage_.GetScalar(id);
}

}  // namespace vectordb