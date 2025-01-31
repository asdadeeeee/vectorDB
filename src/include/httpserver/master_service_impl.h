#pragma once
#include <curl/curl.h>
#include <rapidjson/document.h>
#include <cstdint>
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

struct Partition {
  uint64_t partition_id_;
  uint64_t node_id_;
};

struct PartitionConfig {
  std::string partition_key_;
  int number_of_partitions_;
  std::list<Partition> partitions_;  // 使用 std::list 存储分区信息
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

  void GetPartitionConfig(::google::protobuf::RpcController *controller, const ::nvm::HttpRequest * /*request*/,
                          ::nvm::HttpResponse * /*response*/, ::google::protobuf::Closure *done) override;

  void UpdatePartitionConfig(::google::protobuf::RpcController *controller, const ::nvm::HttpRequest * /*request*/,
                             ::nvm::HttpResponse * /*response*/, ::google::protobuf::Closure *done) override;

  void UpdateNodeStates();

 private:
  auto DoGetPartitionConfig(uint64_t instanceId) -> PartitionConfig;
  void DoUpdatePartitionConfig(uint64_t instanceId, const std::string &partitionKey, int numberOfPartitions,
                               const std::list<Partition> &partitions);

 private:
  etcd::Client etcd_client_;
  std::map<std::string, int> node_error_counts_;  // 错误计数器
};
}  // namespace vectordb
