#include "common/vector_init.h"
#include "index/index_factory.h"
#include "logger/logger.h"
#include "database/persistence.h"
namespace vectordb {
void Init(int node_id) {
  auto cfg_path = GetCfgPath();
  Cfg::SetCfg(cfg_path,node_id);
  InitGlobalLogger();
  SetLogLevel(Cfg::Instance().GlogLevel());
  auto &indexfactory = IndexFactory::Instance();
  int dim = 1;  // 向量维度
  indexfactory.Init(IndexFactory::IndexType::FLAT, dim, 100);
  indexfactory.Init(IndexFactory::IndexType::HNSW, dim, 100);
  indexfactory.Init(IndexFactory::IndexType::FILTER, dim, 100);
}
}  // namespace vectordb