#include "index/faiss_index.h"
#include <experimental/filesystem>
#include <logger/logger.h>
#include <cstdint>
#include "gtest/gtest.h"
#include "index/index_factory.h"
#include "database/scalar_storage.h"
#include "common/vector_init.h"
namespace vectordb {
// NOLINTNEXTLINE
TEST(ScalarTest, SampleTest){
    Init();
    std::experimental::filesystem::remove_all(Cfg::Instance().TestRocksDbPath());
    ScalarStorage storage(Cfg::Instance().TestRocksDbPath());

    rapidjson::Document doc;
    doc.SetObject();
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

    // 添加retCode到响应
    doc.AddMember("value", 10, allocator);
    storage.InsertScalar(1,doc);
    auto res1 = storage.GetScalar(1);
    EXPECT_EQ(res1["value"],10);


    rapidjson::Document doc2;
    doc2.SetObject();
    rapidjson::Document::AllocatorType& allocator2 = doc2.GetAllocator();

    // 添加retCode到响应
    doc2.AddMember("value", 11, allocator2);
    storage.InsertScalar(1,doc2);
    auto res2 = storage.GetScalar(1);
    EXPECT_EQ(res2["value"],11);

}
}  // namespace vectordb 