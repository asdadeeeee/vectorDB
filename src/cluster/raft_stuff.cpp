#include "cluster/raft_stuff.h"
#include "cluster/raft_logger_wrapper.h"
#include "logger/logger.h"

namespace vectordb {
RaftStuff::RaftStuff(int node_id, std::string &endpoint, int port, VectorDatabase *vector_database)
    : node_id_(node_id),
      endpoint_(endpoint),
      port_(port),
      vector_database_(vector_database) {  // 初始化 vector_database_ 指针
  Init();
}

void RaftStuff::Init() {
  smgr_ = nuraft::cs_new<InmemStateMgr>(node_id_, endpoint_, vector_database_);
  sm_ = nuraft::cs_new<LogStateMachine>();

  // 将 state_machine 对象强制转换为 log_state_machine 对象
  nuraft::ptr<LogStateMachine> log_sm = std::dynamic_pointer_cast<LogStateMachine>(sm_);

  log_sm->SetVectorDatabase(
      vector_database_);  // 将 vector_database_ 参数传递给 log_state_machine 的 setVectorDatabase 函数

  nuraft::asio_service::options asio_opt;
  asio_opt.thread_pool_size_ = 1;

  // nuraft::raft_params params;
  // params.election_timeout_lower_bound_ = 100000000;  // 设置为一个非常大的值
  // params.election_timeout_upper_bound_ = 200000000;  // 设置为一个非常大的值

  // Raft parameters.
  nuraft::raft_params params;
#if defined(WIN32) || defined(_WIN32)
  // heartbeat: 1 sec, election timeout: 2 - 4 sec.
  params.heart_beat_interval_ = 1000;
  params.election_timeout_lower_bound_ = 2000;
  params.election_timeout_upper_bound_ = 4000;
#else
  // heartbeat: 100 ms, election timeout: 200 - 400 ms.
  params.heart_beat_interval_ = 100;
  params.election_timeout_lower_bound_ = 200;
  params.election_timeout_upper_bound_ = 400;
#endif
  // Upto 5 logs will be preserved ahead the last snapshot.
  params.reserved_log_items_ = 5;
  // Snapshot will be created for every 5 log appends.
  params.snapshot_distance_ = 5;
  // Client timeout: 3000 ms.
  params.client_req_timeout_ = 3000;
  // According to this method, `append_log` function
  // should be handled differently.
  params.return_method_ = nuraft::raft_params::blocking;

  // Logger.
  std::string log_file_name = "./srv" + std::to_string(node_id_) + ".log";
  nuraft::ptr<LoggerWrapper> log_wrap = nuraft::cs_new<LoggerWrapper>(log_file_name);

  raft_instance_ = launcher_.init(sm_, smgr_, log_wrap, port_, asio_opt, params);

  if (!raft_instance_) {
    global_logger->error("Failed to initialize launcher (see the message in the log file)");
    log_wrap.reset();
    exit(-1);
  }

  // Wait until Raft server is ready (upto 5 seconds).
  const size_t max_try = 100;
  global_logger->info("init Raft instance ");
  for (size_t ii = 0; ii < max_try; ++ii) {
    if (raft_instance_->is_initialized()) {
      global_logger->info("done");
      global_logger->debug("RaftStuff initialized with node_id: {}, endpoint: {}, port: {}", node_id_, endpoint_,
                           port_);  // 添加打印日志
      return;
    }
    global_logger->info(".");
    fflush(stdout);
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
  }
  global_logger->error("RaftStuff Init FAILED");
  log_wrap.reset();
  exit(-1);
}

auto RaftStuff::AddSrv(int srv_id, const std::string &srv_endpoint) -> bool {
  bool success = false;
  nuraft::ptr<nuraft::srv_config> peer_srv_conf = nuraft::cs_new<nuraft::srv_config>(srv_id, srv_endpoint);
  global_logger->debug("Adding server with srv_id: {}, srv_endpoint: {}", srv_id, srv_endpoint);  // 添加打印日志
  auto ret = raft_instance_->add_srv(*peer_srv_conf);

  if (!ret->get_accepted()) {
    global_logger->error("raft_stuff  AddSrv  failed");
    return false;
  }

  // Wait until it appears in server list.
  const size_t max_try = 40;
  for (size_t jj = 0; jj < max_try; ++jj) {
    global_logger->info("Wait for add follower.");
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    auto conf = GetSrvConfig(srv_id);
    if (conf) {
      success = true;
      global_logger->info(" Add follower done.");
      break;
    }
  }
  return success;
}

auto RaftStuff::GetSrvConfig(int srv_id) -> nuraft::ptr<nuraft::srv_config> {
  global_logger->debug("get server config with srv_id: {}", srv_id);  // 添加打印日志
  return raft_instance_->get_srv_config(srv_id);
}

auto RaftStuff::AppendEntries(const std::string &entry)
    -> nuraft::ptr<nuraft::cmd_result<nuraft::ptr<nuraft::buffer>>> {
  if (!raft_instance_ || !raft_instance_->is_leader()) {
    // 添加调试日志
    if (!raft_instance_) {
      global_logger->debug("Cannot append entries: Raft instance is not available");
    } else {
      global_logger->debug("Cannot append entries: Current node is not the leader");
    }
    return nullptr;
  }

  // 计算所需的内存大小
  size_t total_size = sizeof(int) + entry.size();

  // 添加调试日志
  global_logger->debug("Total size of entry: {}", total_size);

  // 创建一个 Raft 日志条目
  nuraft::ptr<nuraft::buffer> log_entry_buffer = nuraft::buffer::alloc(total_size);
  nuraft::buffer_serializer bs_log(log_entry_buffer);

  bs_log.put_str(entry);

  // 添加调试日志
  global_logger->debug("Created log_entry_buffer at address: {}", static_cast<const void *>(log_entry_buffer.get()));

  // 添加调试日志
  global_logger->debug("Appending entry to Raft instance");

  // 将日志条目追加到 Raft 实例中
  return raft_instance_->append_entries({log_entry_buffer});
}

void RaftStuff::EnableElectionTimeout(int lower_bound, int upper_bound) {
  if (raft_instance_) {
    nuraft::raft_params params = raft_instance_->get_current_params();
    params.election_timeout_lower_bound_ = lower_bound;
    params.election_timeout_upper_bound_ = upper_bound;
    raft_instance_->update_params(params);
  }
}

auto RaftStuff::IsLeader() const -> bool {
  if (!raft_instance_) {
    return false;
  }
  return raft_instance_->is_leader();  // 调用 raft_instance_ 的 is_leader() 方法
}

auto RaftStuff::GetAllNodesInfo() const
    -> std::vector<std::tuple<int, std::string, std::string, nuraft::ulong, nuraft::ulong>> {
  std::vector<std::tuple<int, std::string, std::string, nuraft::ulong, nuraft::ulong>> nodes_info;

  if (!raft_instance_) {
    global_logger->warn("raft_instance empty");
    return nodes_info;
  }

  // 获取配置信息

  // get_srv_config_all
  // auto config = raft_instance_->get_config();
  // if (!config) {
  //   return nodes_info;
  // }

  // 获取服务器列表
  // auto servers = config->get_servers();

  std::vector<nuraft::ptr<nuraft::srv_config>> configs;
  raft_instance_->get_srv_config_all(configs);

  int leader_id = raft_instance_->get_leader();
  for (auto &entry : configs) {
    nuraft::ptr<nuraft::srv_config> &srv = entry;
    // 获取节点状态
    std::string node_state;
    if (srv->get_id() == leader_id) {
      node_state = "leader";
    } else {
      node_state = "follower";
    }

    // 使用正确的类型
    nuraft::raft_server::peer_info node_info = raft_instance_->get_peer_info(srv->get_id());
    nuraft::ulong last_log_idx = node_info.last_log_idx_;
    nuraft::ulong last_succ_resp_us = node_info.last_succ_resp_us_;

    nodes_info.emplace_back(
        std::make_tuple(srv->get_id(), srv->get_endpoint(), node_state, last_log_idx, last_succ_resp_us));
  }
  return nodes_info;
}

auto RaftStuff::GetCurrentNodesInfo() const -> std::tuple<int, std::string, std::string, nuraft::ulong, nuraft::ulong> {
  std::tuple<int, std::string, std::string, nuraft::ulong, nuraft::ulong> nodes_info;

  if (!raft_instance_) {
    return nodes_info;
  }

  // 获取配置信息
  auto config = raft_instance_->get_config();
  if (!config) {
    return nodes_info;
  }

  // 获取服务器列表
  auto servers = config->get_servers();

  for (const auto &srv : servers) {
    if (srv && srv->get_id() == node_id_) {
      // 获取节点状态
      std::string node_state;
      if (srv->get_id() == raft_instance_->get_leader()) {
        node_state = "leader";
      } else {
        node_state = "follower";
      }

      // 使用正确的类型
      nuraft::raft_server::peer_info node_info = raft_instance_->get_peer_info(srv->get_id());
      nuraft::ulong last_log_idx = node_info.last_log_idx_;
      nuraft::ulong last_succ_resp_us = node_info.last_succ_resp_us_;
      nodes_info = std::make_tuple(srv->get_id(), srv->get_endpoint(), node_state, last_log_idx, last_succ_resp_us);
      break;
    }
  }

  return nodes_info;
}
}  // namespace vectordb