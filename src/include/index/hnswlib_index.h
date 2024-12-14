#pragma once

#include <vector>
#include "hnswlib/hnswlib.h"
#include "index_factory.h"
namespace vectordb {
class HNSWLibIndex {
public:
    // 构造函数
    HNSWLibIndex(int dim, int num_data, IndexFactory::MetricType metric, int M = 16, int ef_construction = 200); // 将MetricType参数修改为第三个参数

    // 插入向量
    void InsertVectors(const std::vector<float>& data, int64_t label);

    // 查询向量
    auto SearchVectors(const std::vector<float>& query, int k, int ef_search = 50) -> std::pair<std::vector<int64_t>, std::vector<float>>;

    void RemoveVectors(const std::vector<int64_t>& ids);
private:
    // int dim_;
    hnswlib::SpaceInterface<float>* space_;
    hnswlib::HierarchicalNSW<float>* index_;
};
}  // namespace vectordb

