#include "index/faiss_index.h"
#include <common.h>
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
    const char* code_base = std::getenv("VECTORDB_CODE_BASE");
    EXPECT_NE(code_base,nullptr);
    std::string cfg_name = "vectordb_config";
    char *config_path =static_cast<char*>(malloc(strlen(code_base)+cfg_name.length()+2));
    memset(config_path, 0, strlen(code_base) + cfg_name.length() + 2);
    strcat(config_path, code_base);
    strcat(config_path, "/");
    strcat(config_path,cfg_name.c_str());
    std::string path_str(config_path);
    EXPECT_EQ(path_str, "/home/zhouzj/vectorDB/vectorDB/vectordb_config");
    Cfg::CfgPath(config_path);
    
}
}  // namespace vectordb 