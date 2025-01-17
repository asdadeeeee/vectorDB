#include "common/proxy_cfg.h"
#include <fstream>
#include <iostream>

// example proxy_config:
// {
//     "INSTANCE_ID" : 1,
//     "PROXY_ADDRESS" : "0.0.0.0",
//     "PROXY_PORT" : 80,

//     "MASTER_HOST" : "127.0.0.1",
//     "MASTER_PORT" : 6060,

//     "READ_PATHS" :[ "/search"],
//     "WRITE_PATHS" :[ "/upsert"],
    
//     "LOG":{
//         "LOG_NAME" : "my_log",
//         "LOG_LEVEL" : 1
//     }
// }

namespace vectordb {

std::string ProxyCfg::cfg_path{};

void ProxyCfg::ParseCfgFile(const std::string &path) {
  std::ifstream config_file(path);

  rapidjson::IStreamWrapper isw(config_file);

  rapidjson::Document data;

  data.ParseStream(isw);

  assert(data.IsObject());

  if (data.HasMember("INSTANCE_ID") && data["INSTANCE_ID"].IsInt()) {
    instance_id_ = data["INSTANCE_ID"].GetInt();
  } else {
    std::cout << "INSTANCE_ID fault" << std::endl;
  }

  if (data.HasMember("PROXY_ADDRESS") && data["PROXY_ADDRESS"].IsString()) {
    proxy_host_ = data["PROXY_ADDRESS"].GetString();
  } else {
    std::cout << "PROXY_ADDRESS fault" << std::endl;
  }

  if (data.HasMember("PROXY_PORT") && data["PROXY_PORT"].IsInt()) {
    proxy_port_ = data["PROXY_PORT"].GetInt();
  } else {
    std::cout << "PROXY_PORT fault" << std::endl;
  }

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

  if (data.HasMember("READ_PATHS") && data["READ_PATHS"].IsArray()) {
    for (auto &read_path : data["READ_PATHS"].GetArray()) {
      if (read_path.IsString()) {
        read_paths_.emplace(read_path.GetString());
      } else {
        std::cout << "READ_PATH fault" << std::endl;
      }
    }
  } else {
    std::cout << "READ_PATHS fault" << std::endl;
  }

  if (data.HasMember("WRITE_PATHS") && data["WRITE_PATHS"].IsArray()) {
    for (auto &write_path : data["WRITE_PATHS"].GetArray()) {
      if (write_path.IsString()) {
        write_paths_.emplace(write_path.GetString());
      } else {
        std::cout << "WRITE_PATH fault" << std::endl;
      }
    }
  } else {
    std::cout << "WRITE_PATHS fault" << std::endl;
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