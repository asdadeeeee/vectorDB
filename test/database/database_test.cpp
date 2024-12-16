#include <common.h>
#include <logger/logger.h>
#include <cstdint>
#include "common/vector_init.h"
#include "database/vector_database.h"
#include "gtest/gtest.h"
#include "index/faiss_index.h"
#include "index/index_factory.h"
namespace vectordb {
// NOLINTNEXTLINE
TEST(DatabaseTest, SampleTest) {
  Init();
  VectorDatabase db(Cfg::Instance().RocksDbPath());
  rapidjson::Document doc;
  doc.SetObject();
  rapidjson::Document::AllocatorType &allocator = doc.GetAllocator();
  rapidjson::Value vectors(rapidjson::kArrayType);
  vectors.PushBack(10, allocator);
  doc.AddMember("vectors", vectors, allocator);

  IndexFactory::IndexType index_type = IndexFactory::IndexType::FLAT;
  db.Upsert(1, doc, index_type);
  auto res = db.Query(1);
  std::vector<float> vec;
  vec.clear();
  for (const auto &d : res["vectors"].GetArray()) {
    vec.push_back(d.GetFloat());
  }
  EXPECT_EQ(int(vec[0]), 10);


  rapidjson::Document doc2;
  doc2.SetObject();
  rapidjson::Document::AllocatorType &allocator2 = doc2.GetAllocator();

  rapidjson::Value vectors2(rapidjson::kArrayType);
  vectors2.PushBack(11, allocator2);

  doc2.AddMember("vectors", vectors2, allocator2);

  db.Upsert(1, doc2, index_type);
  auto res2 = db.Query(1);
  vec.clear();
  for (const auto &d : res2["vectors"].GetArray()) {
    vec.push_back(d.GetFloat());
  }
  EXPECT_EQ(int(vec[0]), 11);
}
}  // namespace vectordb