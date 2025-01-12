#pragma once

#include <libnuraft/asio_service.hxx>
#include "cluster/in_memory_state_mgr.h"
#include "log_state_machine.h"
#include "logger/logger.h"  // 包含 logger.h 以使用日志记录器
namespace vectordb {
class RaftStuff {
 public:
  RaftStuff(int node_id, std::string &endpoint, int port, VectorDatabase *vector_database);

  void Init();
  auto AddSrv(int srv_id, const std::string &srv_endpoint) -> bool;
  void EnableElectionTimeout(int lower_bound, int upper_bound);  // 定义 enableElectionTimeout 方法
  auto IsLeader() const -> bool;                                 // 添加 isLeader 方法声明
  auto GetAllNodesInfo() const -> std::vector<std::tuple<int, std::string, std::string, nuraft::ulong, nuraft::ulong>>;
  auto GetCurrentNodesInfo() const -> std::tuple<int, std::string, std::string, nuraft::ulong, nuraft::ulong>;
  auto GetNodeStatus(int node_id) const -> std::string;  // 添加 getNodeStatus 方法声明
  auto AppendEntries(const std::string &entry) -> nuraft::ptr<nuraft::cmd_result<nuraft::ptr<nuraft::buffer>>>;
  auto GetSrvConfig(int srv_id) -> nuraft::ptr<nuraft::srv_config>;

 private:
  int node_id_;
  std::string endpoint_;
  nuraft::ptr<nuraft::state_mgr> smgr_;
  nuraft::ptr<nuraft::state_machine> sm_;
  int port_;
  nuraft::raft_launcher launcher_;
  nuraft::ptr<nuraft::raft_server> raft_instance_;
  VectorDatabase *vector_database_;  // 添加一个 VectorDatabase 指针成员变量
};

}  // namespace vectordb