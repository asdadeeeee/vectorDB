#include "index/filter_index.h"
#include <common.h>
#include <logger/logger.h>
#include <cstdint>
#include "common/vector_init.h"
#include "gtest/gtest.h"
#include "index/index_factory.h"

namespace vectordb {
// NOLINTNEXTLINE
TEST(IndexTest, FilterSampleTest) {
  Init();
  auto &indexfactory = IndexFactory::Instance();
  auto index_type = IndexFactory::IndexType::FILTER;
  void *index = indexfactory.GetIndex(index_type);
  EXPECT_NE(index, nullptr);

  auto *filter_index = static_cast<FilterIndex *>(index);
  filter_index->AddIntFieldFilter("index", 10, 1);
  auto filter_bitmap = roaring_bitmap_create();
  filter_index->GetIntFieldFilterBitmap("index", FilterIndex::Operation::EQUAL, 10, filter_bitmap);
  EXPECT_EQ(roaring_bitmap_contains(filter_bitmap, static_cast<uint32_t>(1)), true);

  int64_t *old_field_value_p = nullptr;
  // 如果存在现有向量，则从 FilterIndex 中更新 int 类型字段
  old_field_value_p = static_cast<int64_t *>(malloc(sizeof(int64_t)));
  *old_field_value_p = 10;
  filter_index->UpdateIntFieldFilter("index", old_field_value_p, 20, 1);
//   auto filter_bitmap2 = roaring_bitmap_create();
  filter_index->GetIntFieldFilterBitmap("index", FilterIndex::Operation::EQUAL, 20, filter_bitmap);
  EXPECT_EQ(roaring_bitmap_contains(filter_bitmap,  static_cast<uint32_t>(1)), true);
//   auto filter_bitmap3 = roaring_bitmap_create();
  filter_index->GetIntFieldFilterBitmap("index", FilterIndex::Operation::EQUAL, 10, filter_bitmap);
  EXPECT_EQ(roaring_bitmap_contains(filter_bitmap,  static_cast<uint32_t>(1)), false);
}
}  // namespace vectordb