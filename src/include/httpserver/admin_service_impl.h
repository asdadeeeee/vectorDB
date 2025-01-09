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

class AdminServiceImpl : public nvm::AdminService, public BaseServiceImpl {
 public:
  explicit AdminServiceImpl(VectorDatabase *database, RaftStuff *raft_stuff)
      : vector_database_(database), raft_stuff_(raft_stuff){};
  ~AdminServiceImpl() override = default;

  void snapshot(::google::protobuf::RpcController *controller, const ::nvm::HttpRequest * /*request*/,
                ::nvm::HttpResponse * /*response*/, ::google::protobuf::Closure *done) override;

  void SetLeader(::google::protobuf::RpcController *controller, const ::nvm::HttpRequest * /*request*/,
                 ::nvm::HttpResponse * /*response*/, ::google::protobuf::Closure *done) override;
  void AddFollower(::google::protobuf::RpcController *controller, const ::nvm::HttpRequest * /*request*/,
                   ::nvm::HttpResponse * /*response*/, ::google::protobuf::Closure *done) override;
  void ListNode(::google::protobuf::RpcController *controller, const ::nvm::HttpRequest * /*request*/,
                ::nvm::HttpResponse * /*response*/, ::google::protobuf::Closure *done) override;
  void GetNode(::google::protobuf::RpcController *controller, const ::nvm::HttpRequest * /*request*/,
               ::nvm::HttpResponse * /*response*/, ::google::protobuf::Closure *done) override;

 private:
  VectorDatabase *vector_database_ = nullptr;
  RaftStuff *raft_stuff_ = nullptr;
};
}  // namespace vectordb