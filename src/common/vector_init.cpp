#include "common/vector_init.h"
#include "common/master_cfg.h"
#include "common/proxy_cfg.h"
#include "common/vector_cfg.h"
#include "index/index_factory.h"
#include "logger/logger.h"
#include "database/persistence.h"
namespace vectordb {
void VdbServerInit(int node_id) {
  auto cfg_path = GetCfgPath("vectordb_config");
  Cfg::SetCfg(cfg_path,node_id);
  InitGlobalLogger(Cfg::Instance().GlogName());
  SetLogLevel(Cfg::Instance().GlogLevel());
  auto &indexfactory = IndexFactory::Instance();
  int dim = 1;  // 向量维度
  indexfactory.Init(IndexFactory::IndexType::FLAT, dim, 100);
  indexfactory.Init(IndexFactory::IndexType::HNSW, dim, 100);
  indexfactory.Init(IndexFactory::IndexType::FILTER, dim, 100);
}


void ProxyServerInit() {
  auto cfg_path = GetCfgPath("proxy_config");
  ProxyCfg::SetCfg(cfg_path);
  InitGlobalLogger(ProxyCfg::Instance().GlogName());
  SetLogLevel(ProxyCfg::Instance().GlogLevel());
}

void MasterServerInit() {
  auto cfg_path = GetCfgPath("master_config");
  MasterCfg::SetCfg(cfg_path);
  InitGlobalLogger(MasterCfg::Instance().GlogName());
  SetLogLevel(MasterCfg::Instance().GlogLevel());
}

}  // namespace vectordb