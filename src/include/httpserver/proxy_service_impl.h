#pragma once
#include <curl/curl.h>
#include <rapidjson/document.h>
#include <cstdint>
#include <string>
#include <utility>
#include "brpc/stream.h"
#include "cluster/raft_stuff.h"
#include "database/vector_database.h"
#include "http.pb.h"
#include "httpserver/base_service_impl.h"
#include "index/faiss_index.h"
#include "index/index_factory.h"

namespace vectordb {
// 节点信息结构
struct NodeInfo {
  uint64_t node_id_;
  std::string url_;
  int role_;  // 例如，0 表示主节点，1 表示从节点
};

struct NodePartitionInfo {
  uint64_t partition_id_;
  std::vector<NodeInfo> nodes_;  // 存储具有相同 partitionId 的所有节点
};

struct NodePartitionConfig {
  std::string partition_key_;  // 分区键
  int number_of_partitions_;   // 分区的数量
  std::map<uint64_t, NodePartitionInfo> nodes_info_;
};

class ProxyServiceImpl : public nvm::ProxyService, public BaseServiceImpl {
 public:
  explicit ProxyServiceImpl(std::set<std::string> read_path, std::set<std::string> write_paths,
                            std::string masterServerHost, int masterServerPort, const int &instanceId,
                            CURL *curl_handle)
      : instance_id_(instanceId),
        master_server_host_(std::move(masterServerHost)),
        master_server_port_(masterServerPort),
        read_paths_(std::move(read_path)),
        write_paths_(std::move(write_paths)),
        curl_handle_(curl_handle){};

  ~ProxyServiceImpl() override = default;

  void search(::google::protobuf::RpcController *controller, const ::nvm::HttpRequest * /*request*/,
              ::nvm::HttpResponse * /*response*/, ::google::protobuf::Closure *done) override;

  void upsert(::google::protobuf::RpcController *controller, const ::nvm::HttpRequest * /*request*/,
              ::nvm::HttpResponse * /*response*/, ::google::protobuf::Closure *done) override;

  void topology(::google::protobuf::RpcController *controller, const ::nvm::HttpRequest * /*request*/,
                ::nvm::HttpResponse * /*response*/, ::google::protobuf::Closure *done) override;

  void FetchAndUpdateNodes();
  void FetchAndUpdatePartitionConfig();

 private:
  void ForwardRequest(brpc::Controller *cntl, ::google::protobuf::Closure *done, const std::string &path);
  auto ExtractPartitionKeyValue(const std::string &req, std::string &partitionKeyValue) -> bool;
  auto CalculatePartitionId(const std::string &partitionKeyValue) -> int;
  auto SendRequestToPartition(brpc::Controller *cntl, const std::string& path, int partitionId) -> std::unique_ptr<brpc::Controller>;
  void BroadcastRequestToAllPartitions(brpc::Controller *cntl, const std::string& path);
  auto SelectTargetNode(brpc::Controller *cntl, int partitionId, const std::string& path, NodeInfo& targetNode) -> bool;
  void ForwardToTargetNode(brpc::Controller *cntl, const std::string& path, const NodeInfo& targetNode);
  void ProcessAndRespondToBroadcast(brpc::Controller *cntl, const std::vector< std::unique_ptr<brpc::Controller>>& allResponses, uint k);

 private:
  int instance_id_;                      // 当前 Proxy Server 所属的实例 ID
  std::string master_server_host_;       // Master Server 的主机地址
  int master_server_port_;               // Master Server 的端口
  std::vector<NodeInfo> nodes_[2];       // 使用两个数组
  std::atomic<int> active_nodes_index_;  // 指示当前活动的数组索引
  std::atomic<size_t> next_node_index_;  // 轮询索引
  std::mutex nodes_mutex_;               // 保证节点信息的线程安全访问
  std::set<std::string> read_paths_;     // 读请求的路径集合
  std::set<std::string> write_paths_;    // 写请求的路径集合
  CURL *curl_handle_;

  NodePartitionConfig node_partitions_[2];   // 使用两个数组进行无锁交替更新
  std::atomic<int> active_partition_index_;  // 指示当前活动的分区数组索引
};
}  // namespace vectordb