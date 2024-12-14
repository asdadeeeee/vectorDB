#pragma once

#include "faiss/Index.h"
#include <cstdint>
#include <vector>
namespace vectordb {
class FaissIndex {
public:
    explicit FaissIndex(faiss::Index* index); 
    void InsertVectors(const std::vector<float>& data, int64_t label);
    auto SearchVectors(const std::vector<float>& query, int k) -> std::pair<std::vector<int64_t>, std::vector<float>>;
    void RemoveVectors(const std::vector<int64_t>& ids);
private:
    faiss::Index* index_;
};
}  // namespace vectordb
