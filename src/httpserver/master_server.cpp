

#include <iostream>
#include <sstream>
#include "httpserver/master_service_impl.h"
#include "logger/logger.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "httpserver/master_server.h"

namespace vectordb {
auto MasterServer::Init(const std::string &etcdEndpoints) -> bool {
  master_service_impl_ = std::make_unique<MasterServiceImpl>(etcdEndpoints);

  if (AddService(master_service_impl_.get(), brpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
    global_logger->error("Failed to add master_service_impl_");
    return false;
  }
  global_logger->info("MasterServer init success");
  return true;
}
}  // namespace vectordb

