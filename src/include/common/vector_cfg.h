#pragma once

#include <cassert>
#include <string>
#include "common/vector_utils.h"
#include "spdlog/spdlog.h"
#include <rapidjson/document.h> 
#include <rapidjson/istreamwrapper.h>
namespace vectordb {

struct LogCfg {
    std::string m_glog_name_;
    spdlog::level::level_enum m_level_{spdlog::level::level_enum::debug};
};

class Cfg : public Singleton<Cfg> {
    friend class Singleton<Cfg>;

public:
    // Need call CfgPath first
    static void CfgPath(const std::string &path)
    {
        assert(cfg_path.empty() && !path.empty());
        cfg_path = path;
    }

    auto RocksDbPath() const noexcept -> const std::string &
    {
        return m_rocks_db_path_;
    }
    auto WalPath() const noexcept -> const std::string &
    {
        return wal_path_;
    }
    auto SnapPath() const noexcept -> const std::string &
    {
        return snap_path_;
    }

    auto GlogName() const noexcept -> const std::string &
    {
        return m_log_cfg_.m_glog_name_;
    }

    auto GlogLevel() const noexcept -> spdlog::level::level_enum
    {
        return static_cast<spdlog::level::level_enum>(m_log_cfg_.m_level_);
    }

private:
    Cfg()
    {
        ParseCfgFile(cfg_path);
    }

    void ParseCfgFile(const std::string &path);

    std::string m_rocks_db_path_;
    std::string wal_path_;
    std::string snap_path_;
    LogCfg m_log_cfg_;

    static std::string cfg_path;
};
}  // namespace vectordb
