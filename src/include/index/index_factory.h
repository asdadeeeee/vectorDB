#pragma once

#include "faiss_index.h"
#include "faiss/IndexFlat.h"
#include "faiss/IndexIDMap.h"

#include <map>

namespace vectordb {
class IndexFactory {
public:
    enum class IndexType {
        FLAT,
        UNKNOWN = -1 
    };

    enum class MetricType {
        L2,
        IP
    };

    void Init(IndexType type, int dim, MetricType metric = MetricType::L2);
    auto GetIndex(IndexType type) const -> void*;

private:
    std::map<IndexType, void*> index_map_; 
};

auto GetGlobalIndexFactory() -> IndexFactory*;
}  // namespace vectordb
