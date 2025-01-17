#pragma once

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <cassert>
#include <set>
#include <string>
#include "common/vector_cfg.h"
#include "common/vector_utils.h"
#include "spdlog/spdlog.h"
namespace vectordb {

class ProxyCfg : public Singleton<ProxyCfg> {
  friend class Singleton<ProxyCfg>;

 public:
  // Need call SetCfg first
  static void SetCfg(const std::string &path) {
    assert(cfg_path.empty() && !path.empty());
    cfg_path = path;
  }
  auto InstanceId() const noexcept -> int { return instance_id_; }
  auto Port() const noexcept -> int { return proxy_port_; }
  auto Address() const noexcept -> const std::string & { return proxy_host_; }

  auto MasterPort() const noexcept -> int { return master_port_; }
  auto MasterHost() const noexcept -> const std::string & { return master_host_; }
  auto ReadPaths() const noexcept -> const std::set<std::string> & { return read_paths_; }
  auto WritePaths() const noexcept -> const std::set<std::string> & { return write_paths_; }
  auto GlogName() const noexcept -> const std::string & { return m_log_cfg_.m_glog_name_; }
  auto GlogLevel() const noexcept -> spdlog::level::level_enum {
    return static_cast<spdlog::level::level_enum>(m_log_cfg_.m_level_);
  }

 private:
  ProxyCfg() { ParseCfgFile(cfg_path); }

  void ParseCfgFile(const std::string &path);

 private:
  int instance_id_;
  int proxy_port_;
  std::string proxy_host_;
  int master_port_;
  std::string master_host_;
  std::set<std::string> read_paths_;   // 读请求的路径集合
  std::set<std::string> write_paths_;  // 写请求的路径集合

  LogCfg m_log_cfg_;

  static std::string cfg_path;
};
}  // namespace vectordb
