#include "httpserver/http_server.h"
#include "index/index_factory.h"
#include "logger/logger.h"



// NOLINTNEXTLINE
auto main() -> int {
    // 初始化全局日志记录器
    vectordb::InitGlobalLogger();
    vectordb::SetLogLevel(spdlog::level::debug); // 设置日志级别为debug

    vectordb::global_logger->info("Global logger initialized");

    // 初始化全局IndexFactory实例
    int dim = 1; // 向量维度
    auto &global_index_factory = vectordb::IndexFactory::GetInstance();
    int num_data = 5;
    global_index_factory.Init(vectordb::IndexFactory::IndexType::FLAT, dim,num_data);
    vectordb::global_logger->info("Global IndexFactory initialized");

    // 创建并启动HTTP服务器
    std::string host = "localhost";
    vectordb::HttpServer server(host, 8080);
    vectordb::global_logger->info("HttpServer created");
    server.Start();

    return 0;
}
