#include "index/index_factory.h"
#include "index/hnswlib_index.h"
#include "index/filter_index.h"
namespace vectordb {

void IndexFactory::Init(IndexType type, int dim,  int num_data,MetricType metric) {
    faiss::MetricType faiss_metric = (metric == MetricType::L2) ? faiss::METRIC_L2 : faiss::METRIC_INNER_PRODUCT;

    switch (type) {
        case IndexType::FLAT:
            index_map_[type] = new vectordb::FaissIndex(new faiss::IndexIDMap(new faiss::IndexFlat(dim, faiss_metric)));
            break;
        case IndexType::HNSW:
            index_map_[type] = new vectordb::HNSWLibIndex(dim, num_data, metric, 16, 200);
            break;
        case IndexType::FILTER: // 初始化 FilterIndex 对象
            index_map_[type] = new FilterIndex();
            break;
        default:
            break;
    }
}

auto IndexFactory::GetIndex(IndexType type) const -> void* { 
    auto it = index_map_.find(type);
    if (it != index_map_.end()) {
        return it->second;
    }
    return nullptr;
}

void IndexFactory::SaveIndex(const std::string& folder_path) { // 添加 ScalarStorage 参数

    for (const auto& index_entry : index_map_) {
        IndexType index_type = index_entry.first;
        void* index = index_entry.second;

        // 为每个索引类型生成一个文件名
        std::string file_path = folder_path + std::to_string(static_cast<int>(index_type)) + ".index";

        // 根据索引类型调用相应的 saveIndex 函数
        if (index_type == IndexType::FLAT) {
            static_cast<FaissIndex*>(index)->SaveIndex(file_path);
        } else if (index_type == IndexType::HNSW) {
            static_cast<HNSWLibIndex*>(index)->SaveIndex(file_path);
        } else if (index_type == IndexType::FILTER) { // 保存 FilterIndex 类型的索引
            static_cast<FilterIndex*>(index)->SaveIndex(file_path);
        }
    }
}

void IndexFactory::LoadIndex(const std::string& folder_path) { // 添加 loadIndex 方法实现
    for (const auto& index_entry : index_map_) {
        IndexType index_type = index_entry.first;
        void* index = index_entry.second;

        // 为每个索引类型生成一个文件名
        std::string file_path = folder_path + std::to_string(static_cast<int>(index_type)) + ".index";

        // 根据索引类型调用相应的 loadIndex 函数
        if (index_type == IndexType::FLAT) {
            static_cast<FaissIndex*>(index)->LoadIndex(file_path);
        } else if (index_type == IndexType::HNSW) {
            static_cast<HNSWLibIndex*>(index)->LoadIndex(file_path);
        } else if (index_type == IndexType::FILTER) { // 加载 FilterIndex 类型的索引
            static_cast<FilterIndex*>(index)->LoadIndex(file_path);
        }
    }
}

}  // namespace vectordb