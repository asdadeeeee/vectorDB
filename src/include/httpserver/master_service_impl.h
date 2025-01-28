#pragma once
#include <curl/curl.h>
#include <rapidjson/document.h>
#include <etcd/Client.hpp>
#include <string>
#include <utility>
#include "brpc/stream.h"
#include "cluster/raft_stuff.h"
#include "database/vector_database.h"
#include "gmock/gmock.h"
#include "http.pb.h"
#include "httpserver/base_service_impl.h"
#include "index/faiss_index.h"
#include "index/index_factory.h"
namespace vectordb {

enum class ServerRole { Master, Backup };

struct ServerInfo {
  std::string url_;
  ServerRole role_;
  auto ToJson() const -> rapidjson::Document;
  static auto FromJson(const rapidjson::Document &value) -> ServerInfo;
};
class MasterServiceImpl : public nvm::MasterService, public BaseServiceImpl {
 public:
  explicit MasterServiceImpl(const std::string &etcdEndpoints) : etcd_client_(etcdEndpoints){};

  ~MasterServiceImpl() override = default;

  void GetNodeInfo(::google::protobuf::RpcController *controller, const ::nvm::HttpRequest * /*request*/,
                   ::nvm::HttpResponse * /*response*/, ::google::protobuf::Closure *done) override;

  void AddNode(::google::protobuf::RpcController *controller, const ::nvm::HttpRequest * /*request*/,
               ::nvm::HttpResponse * /*response*/, ::google::protobuf::Closure *done) override;

  void RemoveNode(::google::protobuf::RpcController *controller, const ::nvm::HttpRequest * /*request*/,
                  ::nvm::HttpResponse * /*response*/, ::google::protobuf::Closure *done) override;

  void GetInstance(::google::protobuf::RpcController *controller, const ::nvm::HttpRequest * /*request*/,
                   ::nvm::HttpResponse * /*response*/, ::google::protobuf::Closure *done) override;
                   
  void UpdateNodeStates();

 private:
  

 private:
  etcd::Client etcd_client_;
  std::map<std::string, int> node_error_counts_; // 错误计数器
};
}  // namespace vectordb
