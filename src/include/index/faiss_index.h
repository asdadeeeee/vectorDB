#pragma once

#include <faiss/impl/IDSelector.h>
#include "faiss/Index.h"
#include <faiss/utils/utils.h>
#include <cstdint>
#include <vector>
#include "roaring/roaring.h"
namespace vectordb {

    // 定义 RoaringBitmapIDSelector 结构体
struct RoaringBitmapIDSelector : faiss::IDSelector {
    explicit RoaringBitmapIDSelector(const roaring_bitmap_t* bitmap) : bitmap_(bitmap) {}

    auto is_member(int64_t id) const -> bool final;

    ~RoaringBitmapIDSelector() override = default;

    const roaring_bitmap_t* bitmap_;
};
class FaissIndex {
public:
    explicit FaissIndex(faiss::Index* index); 
    void InsertVectors(const std::vector<float>& data, int64_t label);
    auto SearchVectors(const std::vector<float>& query, int k, const roaring_bitmap_t* bitmap = nullptr) -> std::pair<std::vector<int64_t>, std::vector<float>>;
    void RemoveVectors(const std::vector<int64_t>& ids);
    void SaveIndex(const std::string& file_path); // 添加 saveIndex 方法声明
    void LoadIndex(const std::string& file_path); // 将返回类型更改为 faiss::Index*
private:
    faiss::Index* index_;
};
}  // namespace vectordb
