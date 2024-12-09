#include "index/hnswlib_index.h"
#include <cstdint>
#include <vector>
namespace vectordb {

HNSWLibIndex::HNSWLibIndex(int dim, int num_data, IndexFactory::MetricType metric, int M, int ef_construction) : dim_(dim) { // 将MetricType参数修改为第三个参数
    bool normalize = false;
    if (metric == IndexFactory::MetricType::L2) {
        space_ = new hnswlib::L2Space(dim);
    } else {
        throw std::runtime_error("Invalid metric type.");
    }
    index_ = new hnswlib::HierarchicalNSW<float>(space_, num_data, M, ef_construction);
}

void HNSWLibIndex::InsertVectors(const std::vector<float>& data, uint64_t label) {
    index_->addPoint(data.data(), label);
}

auto HNSWLibIndex::SearchVectors(const std::vector<float>& query, int k, int ef_search) -> std::pair<std::vector<int64_t>, std::vector<float>> { // 修改返回类型
    index_->setEf(ef_search);
    auto result = index_->searchKnn(query.data(), k);

    std::vector<int64_t> indices(k);
    std::vector<float> distances(k);
    for (int j = 0; j < k; j++) {
        auto item = result.top();
        indices[j] = item.second;
        distances[j] = item.first;
        result.pop();
    }

    return {indices, distances};
}
}  // namespace vectordb