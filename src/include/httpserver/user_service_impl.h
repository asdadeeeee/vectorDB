#pragma once
#include <rapidjson/document.h>
#include <string>
#include "brpc/stream.h"
#include "cluster/raft_stuff.h"
#include "database/vector_database.h"
#include "http.pb.h"
#include "httplib/httplib.h"
#include "httpserver/base_service_impl.h"
#include "index/faiss_index.h"
#include "index/index_factory.h"

namespace vectordb {

class UserServiceImpl : public nvm::UserService, public BaseServiceImpl {
 public:
  explicit UserServiceImpl(VectorDatabase *database, RaftStuff *raft_stuff)
      : vector_database_(database), raft_stuff_(raft_stuff){};
  ~UserServiceImpl() override = default;

  void search(::google::protobuf::RpcController *controller, const ::nvm::HttpRequest * /*request*/,
              ::nvm::HttpResponse * /*response*/, ::google::protobuf::Closure *done) override;

  void insert(::google::protobuf::RpcController *controller, const ::nvm::HttpRequest * /*request*/,
              ::nvm::HttpResponse * /*response*/, ::google::protobuf::Closure *done) override;

  void upsert(::google::protobuf::RpcController *controller, const ::nvm::HttpRequest * /*request*/,
              ::nvm::HttpResponse * /*response*/, ::google::protobuf::Closure *done) override;

  void query(::google::protobuf::RpcController *controller, const ::nvm::HttpRequest * /*request*/,
             ::nvm::HttpResponse * /*response*/, ::google::protobuf::Closure *done) override;

 private:
  VectorDatabase *vector_database_ = nullptr;
  RaftStuff *raft_stuff_ = nullptr;
};
}  // namespace vectordb