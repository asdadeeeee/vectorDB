#include "httpserver/proxy_service_impl.h"
namespace vectordb {
void ProxyServiceImpl::search(::google::protobuf::RpcController *controller, const ::nvm::HttpRequest * /*request*/,
                              ::nvm::HttpResponse * /*response*/, ::google::protobuf::Closure *done) {
  global_logger->info("Forwarding POST /search");
  brpc::ClosureGuard done_guard(done);
  auto *cntl = static_cast<brpc::Controller *>(controller);
  ForwardRequest(cntl, done, "/search");
}

void ProxyServiceImpl::upsert(::google::protobuf::RpcController *controller, const ::nvm::HttpRequest * /*request*/,
                              ::nvm::HttpResponse * /*response*/, ::google::protobuf::Closure *done) {
  global_logger->info("Forwarding POST /upsert");
  brpc::ClosureGuard done_guard(done);
  auto *cntl = static_cast<brpc::Controller *>(controller);
  ForwardRequest(cntl, done, "/upsert");
}

void ProxyServiceImpl::topology(::google::protobuf::RpcController *controller, const ::nvm::HttpRequest * /*request*/,
                                ::nvm::HttpResponse * /*response*/, ::google::protobuf::Closure *done) {
  global_logger->debug("Received topology request");
  brpc::ClosureGuard done_guard(done);
  auto *cntl = static_cast<brpc::Controller *>(controller);

  rapidjson::Document doc;
  doc.SetObject();
  rapidjson::Document::AllocatorType &allocator = doc.GetAllocator();

  // 添加 Master Server 信息
  doc.AddMember("masterServer", rapidjson::Value(master_server_host_.c_str(), allocator), allocator);
  doc.AddMember("masterServerPort", master_server_port_, allocator);

  // 添加 instanceId
  doc.AddMember("instanceId", instance_id_, allocator);

  // 添加节点信息
  rapidjson::Value nodes_array(rapidjson::kArrayType);
  int active_index = active_nodes_index_.load();
  for (const auto &node : nodes_[active_index]) {
    rapidjson::Value node_obj(rapidjson::kObjectType);
    node_obj.AddMember("nodeId", rapidjson::Value(node.node_id_.c_str(), allocator), allocator);
    node_obj.AddMember("url", rapidjson::Value(node.url_.c_str(), allocator), allocator);
    node_obj.AddMember("role", node.role_, allocator);
    nodes_array.PushBack(node_obj, allocator);
  }
  doc.AddMember("nodes", nodes_array, allocator);
  // 设置响应
  SetJsonResponse(doc, cntl);
}

void ProxyServiceImpl::ForwardRequest(brpc::Controller *cntl, ::google::protobuf::Closure *done,
                                      const std::string &path) {
  int active_index = active_nodes_index_.load();  // 读取当前活动的数组索引
  if (nodes_[active_index].empty()) {
    global_logger->error("No available nodes for forwarding");

    cntl->http_response().set_status_code(503);
    SetTextResponse("Service Unavailable", cntl);
    done->Run();
    return;
  }

  size_t node_index = 0;

  // 解析JSON请求
  rapidjson::Document json_request;
  json_request.Parse(cntl->request_attachment().to_string().c_str());

  // 打印用户的输入参数
  global_logger->info("Search request parameters: {}", cntl->request_attachment().to_string());

  // 检查JSON文档是否为有效对象
  if (!json_request.IsObject()) {
    global_logger->error("Invalid JSON request");
    cntl->http_response().set_status_code(400);
    SetErrorJsonResponse(cntl, RESPONSE_RETCODE_ERROR, "Invalid JSON request");
    done->Run();
    return;
  }

  // 检查是否需要强制路由到主节点
  bool force_master = (json_request.HasMember("forceMaster") && json_request["forceMaster"].GetBool());
  if (force_master || write_paths_.find(path) != write_paths_.end()) {
    // 强制主节点或写请求 - 寻找 role 为 0 的节点
    for (size_t i = 0; i < nodes_[active_index].size(); ++i) {
      if (nodes_[active_index][i].role_ == 0) {
        node_index = i;
        break;
      }
    }
  } else {
    // 读请求 - 轮询选择任何角色的节点
    node_index = next_node_index_.fetch_add(1) % nodes_[active_index].size();
  }

  const auto &target_node = nodes_[active_index][node_index];
  std::string target_url = target_node.url_ + path;
  global_logger->info("Forwarding request to: {}", target_url);

  // 设置 CURL 选项
  curl_easy_setopt(curl_handle_, CURLOPT_URL, target_url.c_str());
  if (cntl->http_request().method() == brpc::HTTP_METHOD_POST) {
    curl_easy_setopt(curl_handle_, CURLOPT_POSTFIELDS, cntl->request_attachment().to_string().c_str());
  } else {
    curl_easy_setopt(curl_handle_, CURLOPT_HTTPGET, 1L);
  }
  curl_easy_setopt(curl_handle_, CURLOPT_WRITEFUNCTION, WriteCallback);

  // 响应数据容器
  std::string response_data;
  curl_easy_setopt(curl_handle_, CURLOPT_WRITEDATA, &response_data);

  // 执行 CURL 请求
  CURLcode curl_res = curl_easy_perform(curl_handle_);
  if (curl_res != CURLE_OK) {
    global_logger->error("curl_easy_perform() failed: {}", curl_easy_strerror(curl_res));
    cntl->http_response().set_status_code(500);
    SetTextResponse("Internal Server Error", cntl);
    done->Run();
    return;
  }
  global_logger->info("Received response from server");
  // 确保响应数据不为空
  if (response_data.empty()) {
    global_logger->error("Received empty response from server");
    cntl->http_response().set_status_code(500);
    SetTextResponse("Internal Server Error", cntl);
    done->Run();
  } else {
    SetJsonResponse(response_data, cntl);
  }
}


void ProxyServiceImpl::FetchAndUpdateNodes() {
  global_logger->info("Fetching nodes from Master Server");

  // 构建请求 URL
  std::string url = "http://" + master_server_host_ + ":" + std::to_string(master_server_port_) +
                    "/getInstance?instanceId=" +  std::to_string(instance_id_);
  global_logger->debug("Requesting URL: {}", url);

  // 设置 CURL 选项
  curl_easy_setopt(curl_handle_, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl_handle_, CURLOPT_HTTPGET, 1L);
  std::string response_data;
  curl_easy_setopt(curl_handle_, CURLOPT_WRITEFUNCTION, ProxyServiceImpl::WriteCallback);
  curl_easy_setopt(curl_handle_, CURLOPT_WRITEDATA, &response_data);

  // 执行 CURL 请求
  CURLcode curl_res = curl_easy_perform(curl_handle_);
  if (curl_res != CURLE_OK) {
    global_logger->error("curl_easy_perform() failed: {}", curl_easy_strerror(curl_res));
    return;
  }

  // 解析响应数据
  rapidjson::Document doc;
  if (doc.Parse(response_data.c_str()).HasParseError()) {
    global_logger->error("Failed to parse JSON response");
    return;
  }

  // 检查返回码
  if (doc["retCode"].GetInt() != 0) {
    global_logger->error("Error from Master Server: {}", doc["msg"].GetString());
    return;
  }

  int inactive_index = active_nodes_index_.load() ^ 1;  // 获取非活动数组的索引
  nodes_[inactive_index].clear();
  const auto &nodes_array = doc["data"]["nodes"].GetArray();
  for (const auto &node_val : nodes_array) {
    NodeInfo node;
    node.node_id_ = node_val["nodeId"].GetString();
    node.url_ = node_val["url"].GetString();
    node.role_ = node_val["role"].GetInt();
    nodes_[inactive_index].push_back(node);
  }

  // 原子地切换活动数组索引
  active_nodes_index_.store(inactive_index);
  global_logger->info("Nodes updated successfully");
}


auto ProxyServiceImpl::WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) -> size_t {
  (static_cast<std::string *>(userp))->append(static_cast<char *>(contents), size * nmemb);
  return size * nmemb;
}

}  // namespace vectordb