#include "index/hnswlib_index.h"
#include <cstdint>
#include <vector>
#include "logger/logger.h"
namespace vectordb {

HNSWLibIndex::HNSWLibIndex(int dim, int num_data, IndexFactory::MetricType metric, int M, int ef_construction):max_elements_(num_data)
{ // 将MetricType参数修改为第三个参数
    // bool normalize = false;
    if (metric == IndexFactory::MetricType::L2) {
        space_ = new hnswlib::L2Space(dim);
    } else {
        throw std::runtime_error("Invalid metric type.");
    }
    index_ = new hnswlib::HierarchicalNSW<float>(space_, num_data, M, ef_construction);
}

void HNSWLibIndex::InsertVectors(const std::vector<float>& data, int64_t label) {
    assert(index_ != nullptr);
    index_->addPoint(data.data(), label);
}

// 找到最多K个 可能不满K个 不满的都是label distance 为-1
auto HNSWLibIndex::SearchVectors(const std::vector<float>& query, int k,const roaring_bitmap_t* bitmap , int ef_search) -> std::pair<std::vector<int64_t>, std::vector<float>> { // 修改返回类型
    assert(index_ != nullptr);
    index_->setEf(ef_search);

    RoaringBitmapIDFilter* selector = nullptr;
    if (bitmap != nullptr) {
        selector = new RoaringBitmapIDFilter(bitmap);
    } 

    auto result = index_->searchKnn(query.data(), k,selector);

    std::vector<int64_t> indices(k,-1);
    std::vector<float> distances(k,-1);
    int j = 0;
    global_logger->debug("Retrieved values:");
    while(!result.empty()){
        auto item = result.top();
        indices[j] = item.second;
        distances[j] = item.first;
        result.pop();
        global_logger->debug("ID: {}, Distance: {}", indices[j], distances[j]);
        j++;
        if(j == k){
            break;
        }
    }
    global_logger->debug("HNSW index found {} vectors",j);

    if (bitmap != nullptr) {
        delete selector;
    } 
    return {indices, distances};
}

void HNSWLibIndex::RemoveVectors(const std::vector<int64_t>& ids) { // 添加RemoveVectors函数实现
    assert(index_ != nullptr);
    for(const auto &id:ids){
        index_->markDelete(id);
    }
}

void HNSWLibIndex::SaveIndex(const std::string& file_path) { // 添加 saveIndex 方法实现
    index_->saveIndex(file_path);
}

void HNSWLibIndex::LoadIndex(const std::string& file_path) { // 添加 loadIndex 方法实现
    std::ifstream file(file_path); // 尝试打开文件
    if (file.good()) { // 检查文件是否存在
        file.close();
        index_->loadIndex(file_path, space_, max_elements_);
    } else {
        global_logger->warn("File not found: {}. Skipping loading index.", file_path);
    }
}

}  // namespace vectordb