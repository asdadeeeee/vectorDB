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

class MasterCfg : public Singleton<MasterCfg> {
  friend class Singleton<MasterCfg>;

 public:
  // Need call SetCfg first
  static void SetCfg(const std::string &path) {
    assert(cfg_path.empty() && !path.empty());
    cfg_path = path;
  }

  auto Port() const noexcept -> int { return master_port_; }
  auto Address() const noexcept -> const std::string & { return master_host_; }
  auto EtcdEndpoints() const noexcept -> const std::string & { return etcd_endpoints_; }

  auto GlogName() const noexcept -> const std::string & { return m_log_cfg_.m_glog_name_; }
  auto GlogLevel() const noexcept -> spdlog::level::level_enum {
    return static_cast<spdlog::level::level_enum>(m_log_cfg_.m_level_);
  }

 private:
  MasterCfg() { ParseCfgFile(cfg_path); }

  void ParseCfgFile(const std::string &path);

 private:
  int master_port_;
  std::string master_host_;
  std::string etcd_endpoints_;

  LogCfg m_log_cfg_;

  static std::string cfg_path;
};
}  // namespace vectordb
