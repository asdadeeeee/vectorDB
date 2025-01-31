#include "httpserver/proxy_server.h"
#include <iostream>
#include <sstream>
#include "httpserver/proxy_service_impl.h"
#include "logger/logger.h"  // 包含 rapidjson 头文件
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
namespace vectordb {

auto ProxyServer::Init(const std::string &masterServerHost, int masterServerPort, int instanceId,
                       const std::set<std::string> &read_path, const std::set<std::string> &write_paths) -> bool {
  master_server_host_ = masterServerHost;
  master_server_port_ = masterServerPort;
  instance_id_ = instanceId;
  read_paths_ = read_path;
  write_paths_ = write_paths;
  InitCurl();
  proxy_service_impl_ = std::make_unique<ProxyServiceImpl>(read_path, write_paths, masterServerHost, masterServerPort,
                                                           instanceId, curl_handle_);
  if (masterServerHost.empty()) {
    global_logger->info("zhouzj print test ProxyServer::Init() after ProxyServiceImpl, masterServerHost empty");
  } else {
    global_logger->info("zhouzj print test ProxyServer::Init() after ProxyServiceImpl, masterServerHost  not empty");
  }
  if (AddService(proxy_service_impl_.get(), brpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
    global_logger->error("Failed to add http_service_impl");
    return false;
  }
  running_ = true;
  FetchAndUpdateNodes();
  FetchAndUpdatePartitionConfig();

  StartNodeUpdateTimer();  // 启动节点更新定时器
  StartPartitionUpdateTimer(); // 启动分区配置更新定时器
  
  global_logger->info("ProxyServer init success");
  return true;
}

ProxyServer::~ProxyServer() {
  running_ = false;  // 停止定时器循环
  CleanupCurl();
}

void ProxyServer::InitCurl() {
  curl_global_init(CURL_GLOBAL_DEFAULT);
  curl_handle_ = curl_easy_init();
  if (curl_handle_ != nullptr) {
    curl_easy_setopt(curl_handle_, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(curl_handle_, CURLOPT_TCP_KEEPIDLE, 120L);
    curl_easy_setopt(curl_handle_, CURLOPT_TCP_KEEPINTVL, 60L);
  }
}

void ProxyServer::CleanupCurl() {
  if (curl_handle_ != nullptr) {
    curl_easy_cleanup(curl_handle_);
  }
  curl_global_cleanup();
}

void ProxyServer::FetchAndUpdateNodes() {
  if (proxy_service_impl_ == nullptr) {
    global_logger->error("proxy_service_impl_ empty");
  } else {
    proxy_service_impl_->FetchAndUpdateNodes();
  }
}

void ProxyServer::FetchAndUpdatePartitionConfig() {
  if (proxy_service_impl_ == nullptr) {
    global_logger->error("proxy_service_impl_ empty");
  } else {
    proxy_service_impl_->FetchAndUpdatePartitionConfig();
  }
}

void ProxyServer::StartNodeUpdateTimer() {
  std::thread([this]() {
    while (running_) {
      std::this_thread::sleep_for(std::chrono::seconds(30));
      if (proxy_service_impl_ == nullptr) {
        global_logger->error("proxy_service_impl_ empty");
      } else {
        proxy_service_impl_->FetchAndUpdateNodes();
      }
    }
  }).detach();
}

void ProxyServer::StartPartitionUpdateTimer() {
  std::thread([this]() {
    while (running_) {
      std::this_thread::sleep_for(std::chrono::minutes(1));  // 假设每5分钟更新一次，可以根据需要调整
      if (proxy_service_impl_ == nullptr) {
        global_logger->error("proxy_service_impl_ empty");
      } else {
        proxy_service_impl_->FetchAndUpdatePartitionConfig();
      }
    }
  }).detach();
}

}  // namespace vectordb