#pragma once

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <cassert>
#include <string>
#include "common/vector_utils.h"
#include "spdlog/spdlog.h"
namespace vectordb {

struct LogCfg {
  std::string m_glog_name_;
  spdlog::level::level_enum m_level_{spdlog::level::level_enum::debug};
};

struct RaftCfg {
  int node_id_;
  std::string endpoint_;
  int port_;
};

class Cfg : public Singleton<Cfg> {
  friend class Singleton<Cfg>;

 public:
  // Need call SetCfg first
  static void SetCfg(const std::string &path,int id) {
    assert(cfg_path.empty() && !path.empty() && node_id == 0 && id != 0);
    cfg_path = path;
    node_id = id;
  }

  auto RocksDbPath() const noexcept -> const std::string & { return m_rocks_db_path_; }
  auto WalPath() const noexcept -> const std::string & { return wal_path_; }
  auto SnapPath() const noexcept -> const std::string & { return snap_path_; }

  auto TestRocksDbPath() const noexcept -> const std::string & { return test_rocks_db_path_; }
  auto TestWalPath() const noexcept -> const std::string & { return test_wal_path_; }
  auto TestSnapPath() const noexcept -> const std::string & { return test_snap_path_; }

  auto GlogName() const noexcept -> const std::string & { return m_log_cfg_.m_glog_name_; }

  auto GlogLevel() const noexcept -> spdlog::level::level_enum {
    return static_cast<spdlog::level::level_enum>(m_log_cfg_.m_level_);
  }

  auto Port() const noexcept -> int { return port_; }
  auto Address() const noexcept -> const std::string & { return address_; }
  auto RaftNodeId() const noexcept -> int { return raft_cfg_.node_id_; }
  auto RaftPort() const noexcept -> int { return raft_cfg_.port_; }
  auto RaftEndpoint() const noexcept -> const std::string & { return raft_cfg_.endpoint_; }

 private:
  Cfg() { ParseCfgFile(cfg_path,node_id); }

  void ParseCfgFile(const std::string &path, const int &node_id);

  
  std::string m_rocks_db_path_;
  std::string wal_path_;
  std::string snap_path_;
  LogCfg m_log_cfg_;
  RaftCfg raft_cfg_;

  std::string test_rocks_db_path_;
  std::string test_wal_path_;
  std::string test_snap_path_;
  std::string address_;
  int port_;

  static std::string cfg_path;
  static int node_id;
};
}  // namespace vectordb
