#include "httpserver/http_server.h"
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <cassert>
#include <cstdint>
#include <iostream>
#include "common/constants.h"
#include "index/faiss_index.h"
#include "index/hnswlib_index.h"
#include "index/index_factory.h"
#include "logger/logger.h"

namespace vectordb {
  auto HttpServer::Init(VectorDatabase *vector_database,RaftStuff *raft_stuff) -> bool{
    vector_database_ = vector_database;
    raft_stuff_ = raft_stuff;
    user_service_impl_ = std::make_unique<UserServiceImpl>(vector_database_);
    admin_service_impl_ = std::make_unique<AdminServiceImpl>(vector_database_,raft_stuff);
    if (AddService(user_service_impl_.get(), brpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
         global_logger->error("Failed to add http_service_impl");
        return false;
    }
    if (AddService(admin_service_impl_.get(), brpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
        global_logger->error("Failed to add admin_service_impl");
        return false;
    }
    global_logger->info("HttpServer init success");
    return true;
  }
}  // namespace vectordb