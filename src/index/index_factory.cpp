#include "index/index_factory.h"

namespace vectordb {


namespace {
    IndexFactory global_index_factory; 
}

auto GetGlobalIndexFactory() -> IndexFactory* {
    return &global_index_factory; 
}

void IndexFactory::Init(IndexType type, int dim, MetricType metric) {
    faiss::MetricType faiss_metric = (metric == MetricType::L2) ? faiss::METRIC_L2 : faiss::METRIC_INNER_PRODUCT;

    switch (type) {
        case IndexType::FLAT:
            index_map_[type] = new vectordb::FaissIndex(new faiss::IndexIDMap(new faiss::IndexFlat(dim, faiss_metric)));
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