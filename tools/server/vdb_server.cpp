#include <string>
#include "common/vector_cfg.h"
#include "httpserver/http_server.h"
#include "index/index_factory.h"
#include "logger/logger.h"
#include "common/vector_init.h"


// NOLINTNEXTLINE
auto main() -> int {
    vectordb::Init();
    vectordb::global_logger->info("Global IndexFactory initialized");

    // 创建并启动HTTP服务器
    std::string host = "localhost";
    vectordb::VectorDatabase vector_database(vectordb::Cfg::Instance().RocksDbPath(),vectordb::Cfg::Instance().WalPath());
    vector_database.ReloadDatabase();
    vectordb::HttpServer server;
    server.Init(&vector_database);
    vectordb::global_logger->info("HttpServer created");
    std::string server_addr = "localhost:"+std::to_string(vectordb::Cfg::Instance().Port());
    LOG(INFO) << "listen at:" << server_addr;
    brpc::ServerOptions options;
    if (server.Start(server_addr.c_str(), &options) != 0) {
        LOG(ERROR) << "Failed to start server";
        return -1;
    }

    server.RunUntilAskedToQuit();

    return 0;
}
