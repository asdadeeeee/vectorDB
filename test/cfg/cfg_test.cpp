#include "index/faiss_index.h"
#include <logger/logger.h>
#include <strings.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include "gtest/gtest.h"
#include "common/vector_cfg.h"
namespace vectordb {    

// NOLINTNEXTLINE
TEST(CfgTest, SampleTest){
    auto path_str = GetCfgPath();
    EXPECT_EQ(path_str, "/home/zhouzj/vectorDB/vectorDB/vectordb_config");
    Cfg::CfgPath(path_str);
    EXPECT_EQ(Cfg::Instance().RocksDbPath(), "/home/zhouzj/vectordb/storage");
    EXPECT_EQ(Cfg::Instance().GlogLevel(), 1);
    EXPECT_EQ(Cfg::Instance().GlogName(), "my_log");
}
}  // namespace vectordb 