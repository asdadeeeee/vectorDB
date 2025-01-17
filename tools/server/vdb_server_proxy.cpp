
#include "common/proxy_cfg.h"
#include "common/vector_init.h"
#include "httpserver/proxy_server.h"
#include "index/index_factory.h"
#include "logger/logger.h"

auto main() -> int {
  vectordb::ProxyServerInit();

  vectordb::ProxyServer server;
  server.Init(vectordb::ProxyCfg::Instance().MasterHost(), vectordb::ProxyCfg::Instance().MasterPort(),
              vectordb::ProxyCfg::Instance().InstanceId(), vectordb::ProxyCfg::Instance().ReadPaths(),
              vectordb::ProxyCfg::Instance().WritePaths());
  vectordb::global_logger->info("HttpServer created");

  std::string server_addr =
      vectordb::ProxyCfg::Instance().Address() + ":" + std::to_string(vectordb::ProxyCfg::Instance().Port());
  LOG(INFO) << "listen at:" << server_addr;

  brpc::ServerOptions options;
  if (server.Start(server_addr.c_str(), &options) != 0) {
    LOG(ERROR) << "Failed to start server";
    return -1;
  }

  server.RunUntilAskedToQuit();
  return 0;
}
