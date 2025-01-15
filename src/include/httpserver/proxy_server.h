#include <brpc/server.h>
#include <curl/curl.h>
#include <atomic>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include "httpserver/proxy_service_impl.h"

namespace vectordb {

class ProxyServer : public brpc::Server {
 public:
  // ProxyServer(const std::string& masterServerHost, int masterServerPort, const std::string& instanceId);
  ~ProxyServer();
  // void start(int port);

  auto Init(std::string &masterServerHost, int masterServerPort, const int &instanceId,
            std::set<std::string> &read_path, std::set<std::string> &write_paths) -> bool;

 private:
  void InitCurl();
  void CleanupCurl();
  void FetchAndUpdateNodes();   // 获取并更新节点信息
  void StartNodeUpdateTimer();  // 启动节点更新定时器
 private:
  int instance_id_;                    // 当前 Proxy Server 所属的实例 ID
  std::string master_server_host_;     // Master Server 的主机地址
  int master_server_port_;             // Master Server 的端口
  bool running_;                       // 控制定时器线程的运行
  std::set<std::string> read_paths_;   // 读请求的路径集合
  std::set<std::string> write_paths_;  // 写请求的路径集合
  CURL *curl_handle_;

  std::unique_ptr<ProxyServiceImpl> proxy_service_impl_;
};
}  // namespace vectordb