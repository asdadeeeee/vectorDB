// #include "cluster/raft_stuff.h"

// RaftStuff::RaftStuff(int node_id, const std::string& endpoint, int port)
//     : node_id(node_id), endpoint(endpoint), port_(port) {
//     Init();
// }

// void RaftStuff::Init() {
//     smgr_ = cs_new<inmem_state_mgr>(node_id, endpoint);
//     sm_ = cs_new<log_state_machine>();

//     asio_service::options asio_opt;
//     asio_opt.thread_pool_size_ = 1;

//     raft_params params;
//     params.election_timeout_lower_bound_ = 100000000; // 设置为一个非常大的值
//     params.election_timeout_upper_bound_ = 200000000; // 设置为一个非常大的值

//     raft_instance_ = launcher_.init(sm_, smgr_, nullptr, port_, asio_opt, params);
//     GlobalLogger->debug("RaftStuff initialized with node_id: {}, endpoint: {}, port: {}", node_id, endpoint, port_); // 添加打印日志
// }

// ptr< cmd_result< ptr<buffer> > > RaftStuff::addSrv(int srv_id, const std::string& srv_endpoint) {
//     ptr<srv_config> peer_srv_conf = cs_new<srv_config>(srv_id, srv_endpoint);
//     GlobalLogger->debug("Adding server with srv_id: {}, srv_endpoint: {}", srv_id, srv_endpoint); // 添加打印日志
//     return raft_instance_->add_srv(*peer_srv_conf);
// }

// void RaftStuff::enableElectionTimeout(int lower_bound, int upper_bound) {
//     if (raft_instance_) {
//         raft_params params = raft_instance_->get_current_params();
//         params.election_timeout_lower_bound_ = lower_bound;
//         params.election_timeout_upper_bound_ = upper_bound;
//         raft_instance_->update_params(params);
//     }
// }

// bool RaftStuff::isLeader() const {
//     if (!raft_instance_) {
//         return false;
//     }
//     return raft_instance_->is_leader(); // 调用 raft_instance_ 的 is_leader() 方法
// }

// std::vector<std::tuple<int, std::string, std::string, nuraft::ulong, nuraft::ulong>> RaftStuff::getAllNodesInfo() const {
//     std::vector<std::tuple<int, std::string, std::string, nuraft::ulong, nuraft::ulong>> nodes_info;

//     if (!raft_instance_) {
//         return nodes_info;
//     }

//     // 获取配置信息
//     auto config = raft_instance_->get_config();
//     if (!config) {
//         return nodes_info;
//     }

//     // 获取服务器列表
//     auto servers = config->get_servers();
//     // 遍历所有节点并将其添加到 nodes_info 中
//     for (const auto& srv : servers) {
//         if (srv) {
//             // 获取节点状态
//             std::string node_state;
//             if (srv->get_id() == raft_instance_->get_id()) {
//                 node_state = raft_instance_->is_leader() ? "leader" : "follower";
//             } else {
//                 node_state = "follower";
//             }
            
//             // 使用正确的类型
//             nuraft::raft_server::peer_info node_info = raft_instance_->get_peer_info(srv->get_id());
//             nuraft::ulong last_log_idx = node_info.last_log_idx_;
//             nuraft::ulong last_succ_resp_us = node_info.last_succ_resp_us_;

//             nodes_info.push_back(std::make_tuple(srv->get_id(), srv->get_endpoint(), node_state, last_log_idx, last_succ_resp_us));
//         }
//     }

//     return nodes_info;
// }