#pragma once

#include <brpc/server.h>
#include "httpserver/master_service_impl.h"
namespace vectordb {

class MasterServer : public brpc::Server  {
 public:
  ~MasterServer();
  auto Init(const std::string &etcdEndpoints) -> bool;

 private:
  std::unique_ptr<MasterServiceImpl> master_service_impl_;
};
}  // namespace vectordb