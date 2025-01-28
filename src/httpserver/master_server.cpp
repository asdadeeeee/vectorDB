

#include "httpserver/master_server.h"
#include <iostream>
#include <sstream>
#include "httpserver/master_service_impl.h"
#include "logger/logger.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

namespace vectordb {
auto MasterServer::Init(const std::string &etcdEndpoints) -> bool {
  master_service_impl_ = std::make_unique<MasterServiceImpl>(etcdEndpoints);

  if (AddService(master_service_impl_.get(), brpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
    global_logger->error("Failed to add master_service_impl_");
    return false;
  }
  running_ = true;
  StartNodeUpdateTimer();
  global_logger->info("MasterServer init success");
  return true;
}

MasterServer::~MasterServer() {
  running_ = false;  // 停止定时器循环
}

void MasterServer::StartNodeUpdateTimer() {
  std::thread([this]() {
    while (running_) {                                            // 这里可能需要一种更优雅的退出机制
      std::this_thread::sleep_for(std::chrono::seconds(10));  // 每10秒更新一次
      if (master_service_impl_ == nullptr) {
        global_logger->error("master_service_impl_ empty");
      } else {
        master_service_impl_->UpdateNodeStates();
      }
    }
  }).detach();
}

}  // namespace vectordb
