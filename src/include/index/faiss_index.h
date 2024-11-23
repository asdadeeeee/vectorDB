#pragma once

#include <faiss/Index.h>
#include <cstdint>
#include <vector>

class FaissIndex {
public:
    explicit FaissIndex(faiss::Index* index); 
    void InsertVectors(const std::vector<float>& data, uint64_t label);
    auto SearchVectors(const std::vector<float>& query, int k) -> std::pair<std::vector<int64_t>, std::vector<float>>;

private:
    faiss::Index* index_;
};