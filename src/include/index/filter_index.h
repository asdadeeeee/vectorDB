#pragma once

#include <cstdint>
#include <vector>
#include <map>
#include <string>
#include <set>
#include <memory> // 包含 <memory> 以使用 std::shared_ptr
#include "database/scalar_storage.h"
#include "roaring/roaring.h"

namespace vectordb {

class FilterIndex {
public:
    enum class Operation {
        EQUAL,
        NOT_EQUAL
    };

    FilterIndex();
    void AddIntFieldFilter(const std::string& fieldname, int64_t value, uint64_t id);
    void UpdateIntFieldFilter(const std::string& fieldname, int64_t* old_value, int64_t new_value, uint64_t id); // 将 old_value 参数更改为指针类型
    void GetIntFieldFilterBitmap(const std::string& fieldname, Operation op, int64_t value, roaring_bitmap_t* result_bitmap); // 添加 result_bitmap 参数
    auto SerializeIntFieldFilter() -> std::string; // 添加 serializeIntFieldFilter 方法声明
    void DeserializeIntFieldFilter(const std::string& serialized_data); // 添加 deserializeIntFieldFilter 方法声明
    void SaveIndex(const std::string& path); // 添加 path 参数
    void LoadIndex(const std::string& path); // 添加 path 参数

private:
    std::map<std::string, std::map<int64_t, roaring_bitmap_t*>> int_field_filter_;
};

}  // namespace vectordb