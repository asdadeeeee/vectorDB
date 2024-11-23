#include "index/faiss_index.h"
#include "logger/logger.h"
#include "common/constants.h" 
#include <cstdint>
#include <iostream>
#include <vector>

FaissIndex::FaissIndex(faiss::Index* index) : index_(index) {}

void FaissIndex::InsertVectors(const std::vector<float>& data, uint64_t label) {
    auto id = static_cast<int64_t>(label);
    index_->add_with_ids(1, data.data(), &id);
}

auto FaissIndex::SearchVectors(const std::vector<float>& query, int k) -> std::pair<std::vector<int64_t>, std::vector<float>> {
    int dim = index_->d;
    int num_queries = query.size() / dim;
    std::vector<int64_t> indices(num_queries * k);
    std::vector<float> distances(num_queries * k);

    index_->search(num_queries, query.data(), k, distances.data(), indices.data());

    global_logger->debug("Retrieved values:");
    for (size_t i = 0; i < indices.size(); ++i) {
        if (indices[i] != -1) {
            global_logger->debug("ID: {}, Distance: {}", indices[i], distances[i]);
        } else {
            global_logger->debug("No specific value found");
        }
    }
    return {indices, distances};
}