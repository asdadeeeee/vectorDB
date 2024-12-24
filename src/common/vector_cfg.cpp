#include "common/vector_cfg.h"
#include <fstream>
#include <iostream>


namespace vectordb {

std::string Cfg::cfg_path{};

void Cfg::ParseCfgFile(const std::string &path) {
  std::ifstream config_file(path);

  rapidjson::IStreamWrapper isw(config_file);

  rapidjson::Document data;

  data.ParseStream(isw);

  assert(data.IsObject());


  if (data.HasMember("TEST_ROCKS_DB_PATH") && data["TEST_ROCKS_DB_PATH"].IsString()) {
    test_rocks_db_path_ = data["TEST_ROCKS_DB_PATH"].GetString();
  } else {
    std::cout << "TEST_ROCKS_DB_PATH fault" << std::endl;
  }

  if (data.HasMember("TEST_WAL_PATH") && data["TEST_WAL_PATH"].IsString()) {
    test_wal_path_ = data["TEST_WAL_PATH"].GetString();
  } else {
    std::cout << "TEST_WAL_PATH fault" << std::endl;
  }
  if (data.HasMember("TEST_SNAP_PATH") && data["TEST_SNAP_PATH"].IsString()) {
    test_snap_path_ = data["TEST_SNAP_PATH"].GetString();
  } else {
    std::cout << "TEST_SNAP_PATH fault" << std::endl;
  }


  if (data.HasMember("ROCKS_DB_PATH") && data["ROCKS_DB_PATH"].IsString()) {
    m_rocks_db_path_ = data["ROCKS_DB_PATH"].GetString();
  } else {
    std::cout << "ROCKS_DB_PATH fault" << std::endl;
  }

  if (data.HasMember("WAL_PATH") && data["WAL_PATH"].IsString()) {
    wal_path_ = data["WAL_PATH"].GetString();
  } else {
    std::cout << "WAL_PATH fault" << std::endl;
  }
  if (data.HasMember("SNAP_PATH") && data["SNAP_PATH"].IsString()) {
    snap_path_ = data["SNAP_PATH"].GetString();
  } else {
    std::cout << "SNAP_PATH fault" << std::endl;
  }

  if (data.HasMember("LOG") && data["LOG"].IsObject()) {
    if (data["LOG"].HasMember("LOG_NAME") && data["LOG"]["LOG_NAME"].IsString()) {
      m_log_cfg_.m_glog_name_ = data["LOG"]["LOG_NAME"].GetString();
    } else {
      std::cout << "LOG_NAME fault" << std::endl;
    }

    if (data["LOG"].HasMember("LOG_LEVEL") && data["LOG"]["LOG_LEVEL"].IsInt()) {
      m_log_cfg_.m_level_ = static_cast<spdlog::level::level_enum>(data["LOG"]["LOG_LEVEL"].GetInt());
    } else {
      std::cout << "LOG_LEVEL fault" << std::endl;
    }

  } else {
    std::cout << "LOG fault" << std::endl;
  }
}

}  // namespace vectordb