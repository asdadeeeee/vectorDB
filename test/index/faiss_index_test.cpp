#include "index/faiss_index.h"
#include <logger/logger.h>
#include <cstdint>
#include "common/vector_init.h"
#include "gtest/gtest.h"
#include "index/index_factory.h"
namespace vectordb {
// NOLINTNEXTLINE
TEST(IndexTest, FaissSampleTest) {
  VdbServerInit(1);
  auto &indexfactory = IndexFactory::Instance();
  auto index_type = IndexFactory::IndexType::FLAT;
  void *index = indexfactory.GetIndex(IndexFactory::IndexType::FLAT);
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

  switch (index_type) {
    case IndexFactory::IndexType::FLAT: {
      auto *faiss_index = static_cast<FaissIndex *>(index);
      faiss_index->RemoveVectors({3});
      break;
    }
    // 在此处添加其他索引类型的处理逻辑
    default:
      break;
  }

  std::pair<std::vector<int64_t>, std::vector<float>> results2;
  switch (index_type) {
    case IndexFactory::IndexType::FLAT: {
      auto *faiss_index = static_cast<FaissIndex *>(index);
      results2 = faiss_index->SearchVectors(query, k);
      break;
    }
    // 在此处添加其他索引类型的处理逻辑
    default:
      break;
  }

  EXPECT_EQ(results2.first.at(0), -1);

  rapidjson::Document data;
  data.SetObject();
  rapidjson::Document::AllocatorType &allocator = data.GetAllocator();

  rapidjson::Value vectors(rapidjson::kArrayType);
  vectors.PushBack(0.7, allocator);

  data.AddMember("vectors", vectors, allocator);

  // 将新向量插入索引
  std::vector<float> new_vector(data["vectors"].Size());  // 从JSON数据中提取vectors字段
  for (rapidjson::SizeType i = 0; i < data["vectors"].Size(); ++i) {
    new_vector[i] = data["vectors"][i].GetFloat();
  }
  uint64_t label = 1;
  switch (index_type) {
    case IndexFactory::IndexType::FLAT: {
      auto *faiss_index = static_cast<FaissIndex *>(index);
      faiss_index->InsertVectors(new_vector, static_cast<int64_t>(label));
      break;
    }
    default:
      break;
  }

  switch (index_type) {
    case IndexFactory::IndexType::FLAT: {
      auto *faiss_index = static_cast<FaissIndex *>(index);
      faiss_index->RemoveVectors({static_cast<int64_t>(label)});  // 将id转换为long类型
      break;
    }
    default:
      break;
  }
}
}  // namespace vectordb