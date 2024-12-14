#include "index/faiss_index.h"
#include <common.h>
#include <logger/logger.h>
#include <cstdint>
#include "gtest/gtest.h"
#include "index/index_factory.h"
#include "database/scalar_storage.h"
namespace vectordb {
// NOLINTNEXTLINE
TEST(ScalartTest, SampleTest){
    auto cfg_path  = GetCfgPath();
    Cfg::CfgPath(cfg_path);
    
    InitGlobalLogger();
    SetLogLevel(Cfg::Instance().GlogLevel());
    ScalarStorage storage(Cfg::Instance().RocksDbPath());

    rapidjson::Document doc;
    doc.SetObject();
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

    // 添加retCode到响应
    doc.AddMember("value", 10, allocator);
    storage.InsertScalar(1,doc);
    auto res1 = storage.GetScalar(1);
    EXPECT_EQ(res1["value"],10);
}
}  // namespace vectordb 