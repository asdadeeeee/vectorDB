#include "httpserver/proxy_service_impl.h"
#include <cstdint>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "brpc/controller.h"
#include "httpserver/base_service_impl.h"

namespace vectordb {
void ProxyServiceImpl::search(::google::protobuf::RpcController *controller, const ::nvm::HttpRequest * /*request*/,
                              ::nvm::HttpResponse * /*response*/, ::google::protobuf::Closure *done) {
  global_logger->info("Forwarding POST /search");
  brpc::ClosureGuard done_guard(done);
  auto *cntl = static_cast<brpc::Controller *>(controller);
  ForwardRequest(cntl, done, "/UserService/search");
}

void ProxyServiceImpl::upsert(::google::protobuf::RpcController *controller, const ::nvm::HttpRequest * /*request*/,
                              ::nvm::HttpResponse * /*response*/, ::google::protobuf::Closure *done) {
  global_logger->info("Forwarding POST /upsert");
  brpc::ClosureGuard done_guard(done);
  auto *cntl = static_cast<brpc::Controller *>(controller);
  ForwardRequest(cntl, done, "/UserService/upsert");
}

void ProxyServiceImpl::topology(::google::protobuf::RpcController *controller,
                                const ::nvm::HttpRequest *
                                /*request*/,
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
    node_obj.AddMember("nodeId", node.node_id_, allocator);
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
  global_logger->debug("Received {} request", path);
  std::string partition_key_value;
  if (!ExtractPartitionKeyValue(cntl->request_attachment().to_string(), partition_key_value)) {
    global_logger->debug("Partition key value not found, broadcasting request to all partitions");
    BroadcastRequestToAllPartitions(cntl, path);
    return;
  }

  int partition_id = CalculatePartitionId(partition_key_value);

  NodeInfo target_node;
  if (!SelectTargetNode(cntl, partition_id, path, target_node)) {
    SetTextResponse("No suitable node found for forwarding", cntl, 503);
    return;
  }

  ForwardToTargetNode(cntl, path, target_node);
}

// 解析请求中的分区键
auto ProxyServiceImpl::ExtractPartitionKeyValue(const std::string &req, std::string &partitionKeyValue) -> bool {
  global_logger->debug("Extracting partition key value from request");

  rapidjson::Document doc;
  if (doc.Parse(req.c_str()).HasParseError()) {
    global_logger->debug("Failed to parse request body as JSON");
    return false;
  }

  int active_partition_index = active_partition_index_.load();
  const auto &partition_config = node_partitions_[active_partition_index];

  if (!doc.HasMember(partition_config.partition_key_.c_str())) {
    global_logger->debug("Partition key not found in request");
    return false;
  }

  const rapidjson::Value &key_val = doc[partition_config.partition_key_.c_str()];
  if (key_val.IsString()) {
    partitionKeyValue = key_val.GetString();
  } else if (key_val.IsInt()) {
    partitionKeyValue = std::to_string(key_val.GetInt());
  } else {
    global_logger->debug("Unsupported type for partition key");
    return false;
  }

  global_logger->debug("Partition key value extracted: {}", partitionKeyValue);
  return true;
}

// 根据分区键值计算分区 ID
auto ProxyServiceImpl::CalculatePartitionId(const std::string &partitionKeyValue) -> int {
  global_logger->debug("Calculating partition ID for key value: {}", partitionKeyValue);

  int active_partition_index = active_partition_index_.load();
  const auto &partition_config = node_partitions_[active_partition_index];

  // 使用哈希函数处理 partitionKeyValue
  std::hash<std::string> hasher;
  size_t hash_value = hasher(partitionKeyValue);

  // 使用哈希值计算分区 ID
  int partition_id = static_cast<int>(hash_value % partition_config.number_of_partitions_);
  global_logger->debug("Calculated partition ID: {}", partition_id);

  return partition_id;
}
void ProxyServiceImpl::BroadcastRequestToAllPartitions(brpc::Controller *cntl, const std::string &path) {
  // 解析请求以获取 k 的值
  rapidjson::Document doc;
  doc.Parse(cntl->request_attachment().to_string().c_str());
  if (doc.HasParseError() || !doc.HasMember("k") || !doc["k"].IsInt()) {
    SetTextResponse("Invalid request: missing or invalid 'k'", cntl, 400);
    return;
  }

  int k = doc["k"].GetInt();

  int active_partition_index = active_partition_index_.load();
  const auto &partition_config = node_partitions_[active_partition_index];
  std::vector<std::future<std::unique_ptr<brpc::Controller>>> futures;

  std::unordered_set<int> sent_partition_ids;

  for (const auto &partition : partition_config.nodes_info_) {
    int partition_id = partition.first;
    if (sent_partition_ids.find(partition_id) != sent_partition_ids.end()) {
      // 如果已经发送过请求，跳过
      continue;
    }

    futures.push_back(std::async(std::launch::async, &ProxyServiceImpl::SendRequestToPartition, this, cntl, path,
                                 partition.first));  // 发送请求后，将分区ID添加到已发送的集合中
    sent_partition_ids.insert(partition_id);
  }

  // 收集和处理响应
  std::vector<std::unique_ptr<brpc::Controller>> all_responses;
  all_responses.reserve(futures.size());
  for (auto &future : futures) {
    all_responses.push_back(future.get());
  }

  // 处理响应，包括排序和提取最多 K 个结果
  ProcessAndRespondToBroadcast(cntl, all_responses, k);
}

auto ProxyServiceImpl::SendRequestToPartition(brpc::Controller *cntl, const std::string &path, int partitionId)
    -> std::unique_ptr<brpc::Controller> {
  auto sub_cntl = std::make_unique<brpc::Controller>();
  NodeInfo target_node;
  if (!SelectTargetNode(cntl, partitionId, path, target_node)) {
    global_logger->error("Failed to select target node for partition ID: {}", partitionId);
    // 创建一个空的响应对象并返回
    return sub_cntl;
  }

  // 构建目标 URL
  std::string target_url = target_node.url_ + path;
  global_logger->info("Forwarding request to partition node: {}", target_url);

  // 使用 CURL 发送请求
  CURL *curl = curl_easy_init();
  if (curl == nullptr) {
    global_logger->error("CURL initialization failed");
    return sub_cntl;
  }
  // 设置为 POST 请求
  curl_easy_setopt(curl, CURLOPT_POST, 1L);

  // 设置请求头
  struct curl_slist *headers = nullptr;
  headers = curl_slist_append(headers, "Content-Type: application/json");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  std::string request_data = cntl->request_attachment().to_string();
  global_logger->info("request_data: {}", request_data);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_data.c_str());

  curl_easy_setopt(curl, CURLOPT_URL, target_url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);

  std::string response_data;
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

  CURLcode res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    global_logger->error("Curl request failed");
    curl_easy_cleanup(curl);
    return sub_cntl;
  }
  SetJsonResponse(response_data, sub_cntl.get(), 200);
  curl_easy_cleanup(curl);
  return sub_cntl;
}

auto ProxyServiceImpl::SelectTargetNode(brpc::Controller *cntl, int partitionId, const std::string &path,
                                        NodeInfo &targetNode) -> bool {
  global_logger->debug("Selecting target node for partition ID: {}", partitionId);

  // 解析JSON请求
  rapidjson::Document json_request;
  json_request.Parse(cntl->request_attachment().to_string().c_str());

  // 打印用户的输入参数
  global_logger->info("path: {} request parameters: {}", path, cntl->request_attachment().to_string());

  // 检查JSON文档是否为有效对象
  if (!json_request.IsObject()) {
    global_logger->error("Invalid JSON request");
    cntl->http_response().set_status_code(400);
    SetErrorJsonResponse(cntl, RESPONSE_RETCODE_ERROR, "Invalid JSON request");
    return false;
  }

  // 检查是否需要强制路由到主节点
  bool force_master = (json_request.HasMember("forceMaster") && json_request["forceMaster"].GetBool());

  int active_node_index = active_nodes_index_.load();
  const auto &partition_config = node_partitions_[active_partition_index_.load()];
  const auto &partition_nodes = partition_config.nodes_info_.find(partitionId);

  if (partition_nodes == partition_config.nodes_info_.end()) {
    global_logger->error("No nodes found for partition ID: {}", partitionId);
    return false;
  }

  // 获取所有可用的节点
  std::vector<NodeInfo> available_nodes;
  for (const auto &partition_node : partition_nodes->second.nodes_) {
    auto it = std::find_if(nodes_[active_node_index].begin(), nodes_[active_node_index].end(),
                           [&partition_node](const NodeInfo &n) { return n.node_id_ == partition_node.node_id_; });
    if (it != nodes_[active_node_index].end()) {
      available_nodes.push_back(*it);
    }
  }

  if (available_nodes.empty()) {
    global_logger->error("No available nodes for partition ID: {}", partitionId);
    return false;
  }

  if (force_master || write_paths_.find(path) != write_paths_.end()) {
    // 寻找主节点
    for (const auto &node : available_nodes) {
      if (node.role_ == 0) {
        targetNode = node;
        return true;
      }
    }
    global_logger->error("No master node available for partition ID: {}", partitionId);
    return false;
  }  // 读请求 - 使用轮询算法选择节点
  size_t node_index = next_node_index_.fetch_add(1) % available_nodes.size();
  targetNode = available_nodes[node_index];
  return true;
}

// 构建 URL 并使用 CURL 转发请求
void ProxyServiceImpl::ForwardToTargetNode(brpc::Controller *cntl, const std::string &path,
                                           const NodeInfo &targetNode) {
  global_logger->debug("Forwarding request to target node: {}", targetNode.node_id_);
  // 构建目标 URL
  std::string target_url = targetNode.url_ + path;
  global_logger->info("Forwarding request to: {}", target_url);

  // 设置 CURL 选项
  curl_easy_setopt(curl_handle_, CURLOPT_URL, target_url.c_str());

  // 设置为 POST 请求
  curl_easy_setopt(curl_handle_, CURLOPT_POST, 1L);

  // 设置请求头
  struct curl_slist *headers = nullptr;
  headers = curl_slist_append(headers, "Content-Type: application/json");
  curl_easy_setopt(curl_handle_, CURLOPT_HTTPHEADER, headers);

  std::string request_data = cntl->request_attachment().to_string();
  global_logger->info("request_data: {}", request_data);
  curl_easy_setopt(curl_handle_, CURLOPT_POSTFIELDS, request_data.c_str());
  curl_easy_setopt(curl_handle_, CURLOPT_WRITEFUNCTION, WriteCallback);

  // 响应数据容器
  std::string response_data;
  curl_easy_setopt(curl_handle_, CURLOPT_WRITEDATA, &response_data);

  // 执行 CURL 请求
  CURLcode curl_res = curl_easy_perform(curl_handle_);
  if (curl_res != CURLE_OK) {
    global_logger->error("curl_easy_perform() failed");
    SetTextResponse("Internal Server Error", cntl, 500);
  } else {
    global_logger->info("Received response from server");
    // 确保响应数据不为空
    if (response_data.empty()) {
      global_logger->error("Received empty response from server");
      SetTextResponse("Internal Server Error", cntl, 500);
    } else {
      SetJsonResponse(response_data, cntl);
    }
  }
}

void ProxyServiceImpl::FetchAndUpdateNodes() {
  global_logger->info("Fetching nodes from Master Server");

  // 构建请求 URL
  std::string url =
      "http://" + master_server_host_ + ":" + std::to_string(master_server_port_) + "/MasterService/GetInstance";

  // 创建 JSON 数据
  std::string json_data = "{\"instanceId\": " + std::to_string(instance_id_) + "}";

  // 设置 CURL 选项
  curl_easy_setopt(curl_handle_, CURLOPT_URL, url.c_str());

  // 设置为 POST 请求
  curl_easy_setopt(curl_handle_, CURLOPT_POST, 1L);

  // 设置请求头
  struct curl_slist *headers = nullptr;
  headers = curl_slist_append(headers, "Content-Type: application/json");
  curl_easy_setopt(curl_handle_, CURLOPT_HTTPHEADER, headers);

  // 设置 POST 数据
  curl_easy_setopt(curl_handle_, CURLOPT_POSTFIELDS, json_data.c_str());

  std::string response_data;
  curl_easy_setopt(curl_handle_, CURLOPT_WRITEFUNCTION, BaseServiceImpl::WriteCallback);
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
    if (node_val["status"].GetInt() == 1) {  // 只添加状态为 1 的节点
      NodeInfo node;
      node.node_id_ = node_val["nodeId"].GetUint64();
      node.url_ = node_val["url"].GetString();
      node.role_ = node_val["role"].GetInt();
      nodes_[inactive_index].push_back(node);
    } else {
      global_logger->info("Skipping inactive node: {}", node_val["nodeId"].GetUint64());
    }
  }

  // 原子地切换活动数组索引
  active_nodes_index_.store(inactive_index);
  global_logger->info("Nodes updated successfully");
}

void ProxyServiceImpl::FetchAndUpdatePartitionConfig() {
  global_logger->info("Fetching Partition Config from Master Server");
  // 使用 curl 获取分区配置
  // 构建请求 URL
  std::string url =
      "http://" + master_server_host_ + ":" + std::to_string(master_server_port_) + "/MasterService/GetPartitionConfig";

  // 创建 JSON 数据
  std::string json_data = "{\"instanceId\": " + std::to_string(instance_id_) + "}";
  // 设置 CURL 选项
  curl_easy_setopt(curl_handle_, CURLOPT_URL, url.c_str());

  // 设置为 POST 请求
  curl_easy_setopt(curl_handle_, CURLOPT_POST, 1L);

  
  // 设置请求头
  struct curl_slist *headers = nullptr;
  headers = curl_slist_append(headers, "Content-Type: application/json");
  curl_easy_setopt(curl_handle_, CURLOPT_HTTPHEADER, headers);

  // 设置 POST 数据
  curl_easy_setopt(curl_handle_, CURLOPT_POSTFIELDS, json_data.c_str());


  std::string response_data;
  curl_easy_setopt(curl_handle_, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl_handle_, CURLOPT_WRITEDATA, &response_data);

  CURLcode curl_res = curl_easy_perform(curl_handle_);
  if (curl_res != CURLE_OK) {
    global_logger->error("curl_easy_perform() failed");
    return;
  }

  // 解析响应数据并更新 nodePartitions_ 数组
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

  int inactive_index = active_partition_index_.load() ^ 1;  // 获取非活动数组的索引
  node_partitions_[inactive_index].nodes_info_.clear();

  // 获取 partitionKey 和 numberOfPartitions
  node_partitions_[inactive_index].partition_key_ = doc["data"]["partitionKey"].GetString();
  node_partitions_[inactive_index].number_of_partitions_ = doc["data"]["numberOfPartitions"].GetInt();

  // 填充 nodePartitions_[inactiveIndex] 数据
  const auto &partitions_array = doc["data"]["partitions"].GetArray();
  for (const auto &partition_val : partitions_array) {
    uint64_t partition_id = partition_val["partitionId"].GetUint64();
    uint64_t node_id = partition_val["nodeId"].GetUint64();

    // 查找或创建新的 NodePartitionInfo

    auto it = node_partitions_[inactive_index].nodes_info_.find(partition_id);

    if (it == node_partitions_[inactive_index].nodes_info_.end()) {
      // 新建 NodePartitionInfo
      NodePartitionInfo new_partition;
      new_partition.partition_id_ = partition_id;
      node_partitions_[inactive_index].nodes_info_[partition_id] = new_partition;
      it = std::prev(node_partitions_[inactive_index].nodes_info_.end());
    }

    // 添加节点信息
    NodeInfo node_info;
    node_info.node_id_ = node_id;
    // nodeInfo.url 和 nodeInfo.role 需要从某处获取或者设定
    it->second.nodes_.push_back(node_info);
  }

  // 原子地切换活动数组索引
  active_partition_index_.store(inactive_index);

  global_logger->info("Partition Config updated successfully");
}

void ProxyServiceImpl::ProcessAndRespondToBroadcast(brpc::Controller *cntl,
                                                    const std::vector<std::unique_ptr<brpc::Controller>> &allResponses,
                                                    uint k) {
  global_logger->debug("Processing broadcast responses. Expected max results: {}", k);

  struct CombinedResult {
    double distance_;
    double vector_;
  };

  std::vector<CombinedResult> all_results;

  // 解析并合并响应
  for (const auto &response : allResponses) {
    global_logger->debug("Processing response from a partition");
    if (response == nullptr) {
      global_logger->error("response empty");
    } else {
      if (response->http_response().status_code() == 200) {
        global_logger->debug("Response parsed successfully");
        rapidjson::Document doc;
        doc.Parse(response->response_attachment().to_string().c_str());

        if (!doc.HasParseError() && doc.IsObject() && doc.HasMember("vectors") && doc.HasMember("distances")) {
          const auto &vectors = doc["vectors"].GetArray();
          const auto &distances = doc["distances"].GetArray();

          for (rapidjson::SizeType i = 0; i < vectors.Size(); ++i) {
            CombinedResult result = {distances[i].GetDouble(), vectors[i].GetDouble()};
            all_results.push_back(result);
          }
        }
      } else {
        global_logger->debug("Response status: {}", response->http_response().status_code());
      }
    }
  }

  // 对 allResults 根据 distances 排序
  global_logger->debug("Sorting combined results");
  std::sort(all_results.begin(), all_results.end(), [](const CombinedResult &a, const CombinedResult &b) {
    return a.distance_ < b.distance_;  // 以 distance 作为排序的关键字
  });

  // 提取最多 K 个结果
  global_logger->debug("Resizing results to max of {} from {}", k, all_results.size());
  if (all_results.size() > k) {
    all_results.resize(k);
  }

  // 构建最终响应
  global_logger->debug("Building final response");
  rapidjson::Document final_doc;
  final_doc.SetObject();
  rapidjson::Document::AllocatorType &allocator = final_doc.GetAllocator();

  rapidjson::Value final_vectors(rapidjson::kArrayType);
  rapidjson::Value final_distances(rapidjson::kArrayType);

  for (const auto &result : all_results) {
    final_vectors.PushBack(result.vector_, allocator);
    final_distances.PushBack(result.distance_, allocator);
  }

  final_doc.AddMember("vectors", final_vectors, allocator);
  final_doc.AddMember("distances", final_distances, allocator);
  final_doc.AddMember("retCode", 0, allocator);

  global_logger->debug("Final response prepared");

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  final_doc.Accept(writer);

  SetJsonResponse(buffer.GetString(), cntl);
}

// void ProxyServiceImpl::ForwardRequest(brpc::Controller *cntl, ::google::protobuf::Closure *done,
//                                       const std::string &path) {
//   int active_index = active_nodes_index_.load();  // 读取当前活动的数组索引
//   if (nodes_[active_index].empty()) {
//     global_logger->error("No available nodes for forwarding");

//     cntl->http_response().set_status_code(503);
//     SetTextResponse("Service Unavailable", cntl);
//     done->Run();
//     return;
//   }

//   size_t node_index = 0;

//   // 解析JSON请求
//   rapidjson::Document json_request;
//   json_request.Parse(cntl->request_attachment().to_string().c_str());

//   // 打印用户的输入参数
//   global_logger->info("path: {} request parameters: {}", path, cntl->request_attachment().to_string());

//   // 检查JSON文档是否为有效对象
//   if (!json_request.IsObject()) {
//     global_logger->error("Invalid JSON request");
//     cntl->http_response().set_status_code(400);
//     SetErrorJsonResponse(cntl, RESPONSE_RETCODE_ERROR, "Invalid JSON request");
//     done->Run();
//     return;
//   }

//   // 检查是否需要强制路由到主节点
//   bool force_master = (json_request.HasMember("forceMaster") && json_request["forceMaster"].GetBool());
//   if (force_master || write_paths_.find(path) != write_paths_.end()) {
//     // 强制主节点或写请求 - 寻找 role 为 0 的节点
//     for (size_t i = 0; i < nodes_[active_index].size(); ++i) {
//       if (nodes_[active_index][i].role_ == 0) {
//         node_index = i;
//         break;
//       }
//     }
//   } else {
//     // 读请求 - 轮询选择任何角色的节点
//     node_index = next_node_index_.fetch_add(1) % nodes_[active_index].size();
//   }

//   const auto &target_node = nodes_[active_index][node_index];
//   std::string target_url = target_node.url_ + path;
//   global_logger->info("Forwarding request to: {}", target_url);

//   // 设置 CURL 选项
//   curl_easy_setopt(curl_handle_, CURLOPT_URL, target_url.c_str());

//   // 设置为 POST 请求
//   curl_easy_setopt(curl_handle_, CURLOPT_POST, 1L);

//   // 设置请求头
//   struct curl_slist *headers = nullptr;
//   headers = curl_slist_append(headers, "Content-Type: application/json");
//   curl_easy_setopt(curl_handle_, CURLOPT_HTTPHEADER, headers);

//   // 设置 POST 数据

//   // std::string json_data = "{\"instanceId\": " + std::to_string(instance_id_) + "}";

//   std::string request_data = cntl->request_attachment().to_string();
//   global_logger->info("request_data: {}", request_data);
//   curl_easy_setopt(curl_handle_, CURLOPT_POSTFIELDS, request_data.c_str());

//   curl_easy_setopt(curl_handle_, CURLOPT_WRITEFUNCTION, WriteCallback);

//   // 响应数据容器
//   std::string response_data;
//   curl_easy_setopt(curl_handle_, CURLOPT_WRITEDATA, &response_data);

//   // 执行 CURL 请求
//   CURLcode curl_res = curl_easy_perform(curl_handle_);
//   if (curl_res != CURLE_OK) {
//     global_logger->error("curl_easy_perform() failed: {}", curl_easy_strerror(curl_res));
//     cntl->http_response().set_status_code(500);
//     SetTextResponse("Internal Server Error", cntl);
//     done->Run();
//     return;
//   }
//   global_logger->info("Received response from server");
//   // 确保响应数据不为空
//   if (response_data.empty()) {
//     global_logger->error("Received empty response from server");
//     cntl->http_response().set_status_code(500);
//     SetTextResponse("Internal Server Error", cntl);
//     done->Run();
//   } else {
//     SetJsonResponse(response_data, cntl);
//   }
// }

// void ProxyServiceImpl::broad(::google::protobuf::RpcController *controller, const ::nvm::HttpRequest * /*request*/,
//                              ::nvm::HttpResponse * /*response*/, ::google::protobuf::Closure *done) {
//   // 解析请求以获取 k 的值
//   brpc::ClosureGuard done_guard(done);
//   auto *cntl = static_cast<brpc::Controller *>(controller);

//   std::vector<std::future<std::unique_ptr<brpc::Controller>>> futures;

//   std::unordered_set<uint64_t> sent_node_ids;
//   std::vector<NodeInfo> nodes;
//   nodes.push_back({0, "http://127.0.0.1:7781/UserService/test", 0});
//   nodes.push_back({1, "http://127.0.0.1:7782/UserService/test", 1});
//   for (const auto &node_info : nodes) {
//     int node_id = node_info.node_id_;
//     if (sent_node_ids.find(node_id) != sent_node_ids.end()) {
//       // 如果已经发送过请求，跳过
//       continue;
//     }

//     futures.push_back(
//         std::async(std::launch::async, &ProxyServiceImpl::SendRequestToNode, this, node_id, node_info.url_));
//     // 发送请求后，将分区ID添加到已发送的集合中
//     sent_node_ids.insert(node_id);
//   }

//   // 收集和处理响应
//   std::vector<std::unique_ptr<brpc::Controller>> all_cntls;
//   all_cntls.reserve(futures.size());
//   for (auto &future : futures) {
//     all_cntls.push_back(future.get());
//   }

//   // 处理响应，包括排序和提取最多 K 个结果
//   ProcessAndRespondToBroadcast(cntl, all_cntls);
// }

// auto ProxyServiceImpl::SendRequestToNode(uint64_t node_id, const std::string &url) ->
// std::unique_ptr<brpc::Controller> {
//   auto cntl = std::make_unique<brpc::Controller>();
//   // 构建目标 URL
//   std::string target_url = url;
//   global_logger->info("Forwarding request to node: {}", target_url);

//   // 使用 CURL 发送请求
//   CURL *curl = curl_easy_init();
//   if (curl == nullptr) {
//     global_logger->error("CURL initialization failed");
//     return nullptr;
//   }

//   curl_easy_setopt(curl, CURLOPT_URL, target_url.c_str());
//   curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);

//   std::string response_data;
//   curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

//   CURLcode res = curl_easy_perform(curl);
//   if (res != CURLE_OK) {
//     global_logger->error("Curl request failed: url:{}", target_url);
//     curl_easy_cleanup(curl);
//     return nullptr;
//   }

//   // 创建响应对象并填充数据
//   SetJsonResponse(response_data, cntl.get(), 200);

//   curl_easy_cleanup(curl);
//   return cntl;
// }

// void ProxyServiceImpl::ProcessAndRespondToBroadcast(brpc::Controller *res,
//                                                     const std::vector<std::unique_ptr<brpc::Controller>>
//                                                     &allResponses) {
//   global_logger->debug("Processing broadcast responses. Expected max results: {}");

//   int node_cnt = 0;
//   // 解析并合并响应
//   for (const auto &response : allResponses) {
//     global_logger->debug("Processing response from a partition");
//     if (response == nullptr) {
//       global_logger->error("response empty");
//     } else {
//       if (response->http_response().status_code() == 200) {
//         global_logger->debug("Response parsed successfully");
//         node_cnt++;
//       } else {
//         global_logger->debug("Response status: {}", response->http_response().status_code());
//       }
//     }
//   }

//   // 构建最终响应
//   global_logger->debug("Building final response");
//   rapidjson::Document final_doc;
//   final_doc.SetObject();
//   rapidjson::Document::AllocatorType &allocator = final_doc.GetAllocator();

//   rapidjson::Value final_ids(rapidjson::kArrayType);

//   final_doc.AddMember("node_cnt", node_cnt, allocator);
//   final_doc.AddMember("retCode", 0, allocator);

//   global_logger->debug("Final response prepared");

//   rapidjson::StringBuffer buffer;
//   rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
//   final_doc.Accept(writer);

//   SetJsonResponse(buffer.GetString(), res);
// }

}  // namespace vectordb