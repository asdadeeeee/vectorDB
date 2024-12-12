#include "index/index_factory.h"
#include "index/hnswlib_index.h"
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
}  // namespace vectordb