#include "index/faiss_index.h"
#include <common.h>
#include <logger/logger.h>
#include <cstdint>
#include "gtest/gtest.h"
#include "index/index_factory.h"
namespace vectordb {
// NOLINTNEXTLINE
TEST(IndexTest, SampleTest) {
  InitGlobalLogger();
  SetLogLevel(spdlog::level::debug);
  int dim = 1;  // 向量维度
  IndexFactory *indexfactory = GetGlobalIndexFactory();
  EXPECT_NE(indexfactory, nullptr);
  IndexFactory::IndexType index_type = IndexFactory::IndexType::FLAT;
  indexfactory->Init(index_type, dim);

  void *index = indexfactory->GetIndex(IndexFactory::IndexType::FLAT);
  EXPECT_NE(index, nullptr);

  // 根据索引类型初始化索引对象并调用insert_vectors函数

  std::vector<float> base_data{0.8};
  uint64_t base_label = 3;
  switch (index_type) {
    case IndexFactory::IndexType::FLAT: {
      auto *faiss_index = static_cast<FaissIndex *>(index);
      EXPECT_NE(index, nullptr);
      faiss_index->InsertVectors(base_data, base_label);
      break;
    }
    // 在此处添加其他索引类型的处理逻辑
    default:
      break;
  }

  std::vector<float> query{0.8};
  int k = 1;
  std::pair<std::vector<int64_t>, std::vector<float>> results;
  switch (index_type) {
    case IndexFactory::IndexType::FLAT: {
      auto *faiss_index = static_cast<FaissIndex *>(index);
      results = faiss_index->SearchVectors(query, k);
      break;
    }
    // 在此处添加其他索引类型的处理逻辑
    default:
      break;
  }

  EXPECT_EQ(results.first.size(), 1);
  EXPECT_EQ(results.second.size(), 1);
  EXPECT_EQ(results.first.at(0), 3);
}
}  // namespace vectordb