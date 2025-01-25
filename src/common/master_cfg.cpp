#include "common/master_cfg.h"
#include <fstream>
#include <iostream>
#include "common/proxy_cfg.h"

// example master_config:
// {
//     "MASTER_HOST" : "0.0.0.0",
//     "MASTER_PORT" : 6060,
//     "ETCD_ENDPOINTS" : "http://127.0.0.1:2379",
    
//     "LOG":{
//         "LOG_NAME" : "my_log",
//         "LOG_LEVEL" : 1
//     }
// }

namespace vectordb {

std::string MasterCfg::cfg_path{};

void MasterCfg::ParseCfgFile(const std::string &path) {
  std::ifstream config_file(path);

  rapidjson::IStreamWrapper isw(config_file);

  rapidjson::Document data;

  data.ParseStream(isw);

  assert(data.IsObject());

  if (data.HasMember("MASTER_HOST") && data["MASTER_HOST"].IsString()) {
    master_host_ = data["MASTER_HOST"].GetString();
  } else {
    std::cout << "MASTER_HOST fault" << std::endl;
  }

  if (data.HasMember("MASTER_PORT") && data["MASTER_PORT"].IsInt()) {
    master_port_ = data["MASTER_PORT"].GetInt();
  } else {
    std::cout << "MASTER_PORT fault" << std::endl;
  }

  if (data.HasMember("ETCD_ENDPOINTS") && data["ETCD_ENDPOINTS"].IsString()) {
    etcd_endpoints_ = data["ETCD_ENDPOINTS"].GetString();
  } else {
    std::cout << "ETCD_ENDPOINTS fault" << std::endl;
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