#include <string>
#include "common/vector_cfg.h"
#include "common/vector_init.h"
#include "httpserver/http_server.h"
#include "index/index_factory.h"
#include "logger/logger.h"

// NOLINTNEXTLINE
auto main(int argc, char *argv[]) -> int {
  int node_id = 1;
  if (argc == 2) {
    node_id = std::atoi(argv[1]);  // Convert argument to integer if provided
    std::cout << "The node_id you input is " << node_id << std::endl;
  } else {
    std::cout << "No number provided, using default value: " << node_id << std::endl;
  }
  vectordb::Init(node_id);
  vectordb::global_logger->info("Global IndexFactory initialized");

  // 创建并启动HTTP服务器

  vectordb::VectorDatabase vector_database(vectordb::Cfg::Instance().RocksDbPath(),
                                           vectordb::Cfg::Instance().WalPath());
  vector_database.ReloadDatabase();
  vectordb::global_logger->info("VectorDatabase initialized");

  int raft_node_id = vectordb::Cfg::Instance().RaftNodeId();
  std::string endpoint = vectordb::Cfg::Instance().RaftEndpoint();
  int raft_port = vectordb::Cfg::Instance().RaftPort();

  vectordb::RaftStuff raft_stuff(raft_node_id, endpoint, raft_port, &vector_database);
  vectordb::global_logger->info("RaftStuff object created with node_id: {}, endpoint: {}, port: {}", raft_node_id, endpoint,
                                raft_port);  // 添加调试日志

  vectordb::HttpServer server;
  server.Init(&vector_database, &raft_stuff);
  vectordb::global_logger->info("HttpServer created");

  std::string server_addr =
      vectordb::Cfg::Instance().Address() + ":" + std::to_string(vectordb::Cfg::Instance().Port());
  LOG(INFO) << "listen at:" << server_addr;

  brpc::ServerOptions options;
  if (server.Start(server_addr.c_str(), &options) != 0) {
    LOG(ERROR) << "Failed to start server";
    return -1;
  }

  server.RunUntilAskedToQuit();

  return 0;
}
