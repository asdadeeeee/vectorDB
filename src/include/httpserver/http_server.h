#pragma once
#include "index/faiss_index.h"
#include "httplib/httplib.h"
#include "index/index_factory.h"
#include "database/vector_database.h"
#include "httpserver/admin_service_impl.h"
#include "httpserver/user_service_impl.h"
#include <rapidjson/document.h>
#include <string>
#include <brpc/server.h>

namespace vectordb {

class HttpServer: public brpc::Server {
public:
   auto Init(VectorDatabase *vector_database) -> bool;

private:

    VectorDatabase* vector_database_;
    std::unique_ptr<UserServiceImpl> user_service_impl_;
    std::unique_ptr<AdminServiceImpl> admin_service_impl_;
};
}  // namespace vectordb
