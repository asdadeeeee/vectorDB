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
        HNSW,
        UNKNOWN = -1 
    };

    enum class MetricType {
        L2,
        IP
    };

    static auto GetInstance() -> IndexFactory& ;

        // 删除拷贝构造函数和赋值运算符，确保单例不可复制
    IndexFactory(const IndexFactory&) = delete;
    auto operator=(const IndexFactory&) -> IndexFactory& = delete;


    void Init(IndexType type, int dim,  int num_data, MetricType metric = MetricType::L2);
    auto GetIndex(IndexType type) const -> void*;


private:

    IndexFactory()= default; // 禁止外部构造
    ~IndexFactory()= default; // 禁止外部析构
    std::map<IndexType, void*> index_map_; 

};

}  // namespace vectordb
