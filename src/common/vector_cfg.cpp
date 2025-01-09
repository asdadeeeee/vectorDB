#include "common/vector_cfg.h"
#include <fstream>
#include <iostream>

// example vectordb_config:

// {
//     "CLUSTER_INFO" :[
//         {
//             "RAFT":{
//                 "NODE_ID":1,
//                 "ENDPOINT":"127.0.0.1:8081",
//                 "PORT":8081
//             },
//             "ROCKS_DB_PATH" : "/home/zhouzj/vectordb1/storage",
//             "WAL_PATH" : "/home/zhouzj/vectordb1/wal",
//             "SNAP_PATH" : "/home/zhouzj/vectordb1/snap/",
//             "ADDRESS" : "0.0.0.0",
//             "PORT" : 7781
//         },
//         {
//             "RAFT":{
//                 "NODE_ID":2,
//                 "ENDPOINT":"127.0.0.1:8082",
//                 "PORT":8082
//             },
//             "ROCKS_DB_PATH" : "/home/zhouzj/vectordb2/storage",
//             "WAL_PATH" : "/home/zhouzj/vectordb2/wal",
//             "SNAP_PATH" : "/home/zhouzj/vectordb2/snap/",
//             "ADDRESS" : "0.0.0.0",
//             "PORT" : 7782

//         }
//     ],
//     "LOG":{
//         "LOG_NAME" : "my_log",
//         "LOG_LEVEL" : 1
//     },
//     "TEST_ROCKS_DB_PATH" : "/home/zhouzj/test_vectordb/storage",
//     "TEST_WAL_PATH" : "/home/zhouzj/test_vectordb/wal",
//     "TEST_SNAP_PATH" : "/home/zhouzj/test_vectordb/snap/"

// }

//     vdb_server use these:
//     "CLUSTER_INFO" :[
//         {
//             "RAFT":{
//                 "NODE_ID":1,
//                 "ENDPOINT":"127.0.0.1:8081",
//                 "PORT":8081
//             },
//             "ROCKS_DB_PATH" : "/home/zhouzj/vectordb1/storage",
//             "WAL_PATH" : "/home/zhouzj/vectordb1/wal",
//             "SNAP_PATH" : "/home/zhouzj/vectordb1/snap/",
//             "ADDRESS" : "0.0.0.0",
//             "PORT" : 7781
//         },
//         {
//             "RAFT":{
//                 "NODE_ID":2,
//                 "ENDPOINT":"127.0.0.1:8082",
//                 "PORT":8082
//             },
//             "ROCKS_DB_PATH" : "/home/zhouzj/vectordb2/storage",
//             "WAL_PATH" : "/home/zhouzj/vectordb2/wal",
//             "SNAP_PATH" : "/home/zhouzj/vectordb2/snap/",
//             "ADDRESS" : "0.0.0.0",
//             "PORT" : 7782

//         }
//     ],

//     gtest use these:
//     "TEST_ROCKS_DB_PATH" : "/home/zhouzj/test_vectordb/storage",
//     "TEST_WAL_PATH" : "/home/zhouzj/test_vectordb/wal",
//     "TEST_SNAP_PATH" : "/home/zhouzj/test_vectordb/snap/",

namespace vectordb {

std::string Cfg::cfg_path{};
int Cfg::node_id{0};

void Cfg::ParseCfgFile(const std::string &path,const  int &node_id) {
  std::ifstream config_file(path);

  rapidjson::IStreamWrapper isw(config_file);

  rapidjson::Document data;

  data.ParseStream(isw);

  assert(data.IsObject());

  if (data.HasMember("CLUSTER_INFO") && data["CLUSTER_INFO"].IsArray()) {
    for (auto &node_cfg : data["CLUSTER_INFO"].GetArray()) {
      if (node_cfg.HasMember("RAFT") && node_cfg["RAFT"].IsObject()) {
        if (node_cfg["RAFT"].HasMember("NODE_ID") && node_cfg["RAFT"]["NODE_ID"].IsInt()) {
          int raft_node_id = node_cfg["RAFT"]["NODE_ID"].GetInt();
          if (raft_node_id == node_id) {
            if (node_cfg.HasMember("ROCKS_DB_PATH") && node_cfg["ROCKS_DB_PATH"].IsString()) {
              m_rocks_db_path_ = node_cfg["ROCKS_DB_PATH"].GetString();
            } else {
              std::cout << "ROCKS_DB_PATH fault" << std::endl;
            }

            if (node_cfg.HasMember("WAL_PATH") && node_cfg["WAL_PATH"].IsString()) {
              wal_path_ = node_cfg["WAL_PATH"].GetString();
            } else {
              std::cout << "WAL_PATH fault" << std::endl;
            }
            if (node_cfg.HasMember("SNAP_PATH") && node_cfg["SNAP_PATH"].IsString()) {
              snap_path_ = node_cfg["SNAP_PATH"].GetString();
            } else {
              std::cout << "SNAP_PATH fault" << std::endl;
            }

      
              if (node_cfg["RAFT"].HasMember("NODE_ID") && node_cfg["RAFT"]["NODE_ID"].IsInt()) {
                raft_cfg_.node_id_ = node_cfg["RAFT"]["NODE_ID"].GetInt();
              } else {
                std::cout << "NODE_ID fault" << std::endl;
              }

              if (node_cfg["RAFT"].HasMember("ENDPOINT") && node_cfg["RAFT"]["ENDPOINT"].IsString()) {
                raft_cfg_.endpoint_ = node_cfg["RAFT"]["ENDPOINT"].GetString();
              } else {
                std::cout << "ENDPOINT fault" << std::endl;
              }
              if (node_cfg["RAFT"].HasMember("PORT") && node_cfg["RAFT"]["PORT"].IsInt()) {
                raft_cfg_.port_ = node_cfg["RAFT"]["PORT"].GetInt();
              } else {
                std::cout << "PORT fault" << std::endl;
              }

            

            if (node_cfg.HasMember("PORT") && node_cfg["PORT"].IsInt()) {
              port_ = node_cfg["PORT"].GetInt();
            } else {
              std::cout << "PORT fault" << std::endl;
            }

            if (node_cfg.HasMember("ADDRESS") && node_cfg["ADDRESS"].IsString()) {
              address_ = node_cfg["ADDRESS"].GetString();
            } else {
              std::cout << "ADDRESS fault" << std::endl;
            }
          }
        } else {
          std::cout << "NODE_ID fault" << std::endl;
        }
      } else {
        std::cout << "RAFT fault" << std::endl;
      }
    }
  }

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