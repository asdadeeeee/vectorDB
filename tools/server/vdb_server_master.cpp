#include "common/master_cfg.h"
#include "common/vector_init.h"
#include "httpserver/master_server.h"
#include "index/index_factory.h"
#include "logger/logger.h"

auto main() -> int {
  vectordb::MasterServerInit();

  vectordb::MasterServer server;
  server.Init(vectordb::MasterCfg::Instance().EtcdEndpoints());
  vectordb::global_logger->info("MasterServer created");

  std::string server_addr =
      vectordb::MasterCfg::Instance().Address() + ":" + std::to_string(vectordb::MasterCfg::Instance().Port());
  LOG(INFO) << "listen at:" << server_addr;

  brpc::ServerOptions options;
  if (server.Start(server_addr.c_str(), &options) != 0) {
    LOG(ERROR) << "Failed to start server";
    return -1;
  }

  server.RunUntilAskedToQuit();
  return 0;
}
