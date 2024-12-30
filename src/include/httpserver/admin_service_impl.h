#pragma once
#include <rapidjson/document.h>
#include <string>
#include "brpc/stream.h"
#include "database/vector_database.h"
#include "http.pb.h"
#include "httplib/httplib.h"
#include "httpserver/base_service_impl.h"
#include "index/faiss_index.h"
#include "index/index_factory.h"

namespace vectordb {

class AdminServiceImpl : public nvm::AdminService, public BaseServiceImpl {
 public:
  explicit AdminServiceImpl(VectorDatabase *database) : vector_database_(database){};
  ~AdminServiceImpl() override = default;

    void snapshot(::google::protobuf::RpcController *controller, const ::nvm::HttpRequest * /*request*/,
             ::nvm::HttpResponse * /*response*/, ::google::protobuf::Closure *done) override;

 private:
  VectorDatabase *vector_database_ = nullptr;
};
}  // namespace vectordb