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
    auto SearchVectors(const std::vector<float>& query, int k, const roaring_bitmap_t* bitmap = nullptr,int ef_search = 50) -> std::pair<std::vector<int64_t>, std::vector<float>>;

    void RemoveVectors(const std::vector<int64_t>& ids);

    void SaveIndex(const std::string& file_path); // 添加 saveIndex 方法声明
    void LoadIndex(const std::string& file_path); // 添加 loadIndex 方法声明

        // 定义 RoaringBitmapIDFilter 类
    class RoaringBitmapIDFilter : public hnswlib::BaseFilterFunctor {
    public:
        explicit RoaringBitmapIDFilter(const roaring_bitmap_t* bitmap) : bitmap_(bitmap) {}

        auto operator()(hnswlib::labeltype label) -> bool override {
            return roaring_bitmap_contains(bitmap_, static_cast<uint32_t>(label));
        }

    private:
        const roaring_bitmap_t* bitmap_;
    };
    
private:
    // int dim_;
    hnswlib::SpaceInterface<float>* space_;
    hnswlib::HierarchicalNSW<float>* index_;
    size_t max_elements_; // 添加 max_elements 成员变量
};
}  // namespace vectordb

