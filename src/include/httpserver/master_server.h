#pragma once

#include <brpc/server.h>
#include "httpserver/master_service_impl.h"
namespace vectordb {
class MasterServer : public brpc::Server  {
 public:
  ~MasterServer();
  auto Init(const std::string &etcdEndpoints) -> bool;
  void StartNodeUpdateTimer();

 private:
  std::unique_ptr<MasterServiceImpl> master_service_impl_;
  bool running_;                       // 控制定时器线程的运行
};
}  // namespace vectordb