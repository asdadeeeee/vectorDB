#pragma once

#include "faiss_index.h"
#include "faiss/IndexFlat.h"
#include "faiss/IndexIDMap.h"
#include "common/vector_utils.h"
#include <map>

namespace vectordb {
class IndexFactory: public Singleton<IndexFactory>{
    friend class  Singleton<IndexFactory>;
public:
    enum class IndexType {
        FLAT,
        HNSW,
        FILTER, // 添加 FILTER 枚举值
        UNKNOWN = -1 
    };

    enum class MetricType {
        L2,
        IP
    };

    void Init(IndexType type, int dim,  int num_data, MetricType metric = MetricType::L2);
    auto GetIndex(IndexType type) const -> void*;
     void SaveIndex(const std::string& folder_path); // 添加 ScalarStorage 参数
    void LoadIndex(const std::string& folder_path); // 添加 loadIndex 方法声明



private:

    std::map<IndexType, void*> index_map_; 

};

}  // namespace vectordb