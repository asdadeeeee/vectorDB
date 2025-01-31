#include "httpserver/master_service_impl.h"
#include <cctype>
#include <cstdint>
#include <string>
#include "common/constants.h"
namespace vectordb {

auto ServerInfo::FromJson(const rapidjson::Document &value) -> ServerInfo {
  ServerInfo info;
  info.url_ = value["url"].GetString();
  info.role_ = static_cast<ServerRole>(value["role"].GetInt());
  return info;
}

auto ServerInfo::ToJson() const -> rapidjson::Document {
  rapidjson::Document doc;
  doc.SetObject();
  rapidjson::Document::AllocatorType &allocator = doc.GetAllocator();

  // 将 url 和 role 转换为 rapidjson::Value
  rapidjson::Value url_value;
  url_value.SetString(url_.c_str(), allocator);

  rapidjson::Value role_value;
  role_value.SetInt(static_cast<int>(role_));

  // 使用正确的类型添加成员
  doc.AddMember("url", url_value, allocator);
  doc.AddMember("role", role_value, allocator);

  return doc;
}

void MasterServiceImpl::GetNodeInfo(::google::protobuf::RpcController *controller,
                                    const ::nvm::HttpRequest * /*request*/, ::nvm::HttpResponse * /*response*/,
                                    ::google::protobuf::Closure *done) {
  global_logger->info("Received GetNodeInfo request");

  brpc::ClosureGuard done_guard(done);
  auto *cntl = static_cast<brpc::Controller *>(controller);
  // 解析JSON请求
  rapidjson::Document json_request;
  json_request.Parse(cntl->request_attachment().to_string().c_str());

  // 检查JSON文档是否为有效对象
  if (!json_request.IsObject()) {
    global_logger->error("Invalid JSON request");
    cntl->http_response().set_status_code(400);
    SetErrorJsonResponse(cntl, RESPONSE_RETCODE_ERROR, "Invalid JSON request");
    return;
  }

  // 从JSON请求中获取ID
  uint64_t instance_id = json_request[INSTANCE_ID].GetUint64();
  uint64_t node_id = json_request[NODE_ID].GetUint64();

  try {
    std::string etcd_key = "/instances/" + std::to_string(instance_id) + "/nodes/" + std::to_string(node_id);
    etcd::Response etcd_response = etcd_client_.get(etcd_key).get();
    if (!etcd_response.is_ok()) {
      global_logger->error("etcd response fail");
      SetResponse(cntl, 1, "Error accessing etcd: " + etcd_response.error_message());
      return;
    }

    rapidjson::Document doc;
    doc.SetObject();
    rapidjson::Document::AllocatorType &allocator = doc.GetAllocator();

    // 解析节点信息
    rapidjson::Document node_doc;
    node_doc.Parse(etcd_response.value().as_string().c_str());
    if (!node_doc.IsObject()) {
      SetResponse(cntl, 1, "Invalid JSON format");
      return;
    }

    // 构建响应
    doc.AddMember("instanceId", instance_id, allocator);
    doc.AddMember("nodeId", node_id, allocator);
    doc.AddMember("nodeInfo", node_doc, allocator);

    SetResponse(cntl, 0, "Node info retrieved successfully", &doc);
  } catch (const std::exception &e) {
    global_logger->error("GetNodeInfo exception");
    SetResponse(cntl, 1, "Exception accessing etcd: " + std::string(e.what()));
  }
}

void MasterServiceImpl::AddNode(::google::protobuf::RpcController *controller, const ::nvm::HttpRequest * /*request*/,
                                ::nvm::HttpResponse * /*response*/, ::google::protobuf::Closure *done) {
  global_logger->info("Received AddNode request");

  brpc::ClosureGuard done_guard(done);
  auto *cntl = static_cast<brpc::Controller *>(controller);
  // 解析JSON请求
  rapidjson::Document json_request;
  json_request.Parse(cntl->request_attachment().to_string().c_str());

  // 检查JSON文档是否为有效对象
  if (!json_request.IsObject()) {
    global_logger->error("Invalid JSON request");
    cntl->http_response().set_status_code(400);
    SetErrorJsonResponse(cntl, RESPONSE_RETCODE_ERROR, "Invalid JSON request");
    return;
  }

  try {
    uint64_t instance_id = json_request[INSTANCE_ID].GetUint64();
    uint64_t node_id = json_request[NODE_ID].GetUint64();
    std::string etcd_key = "/instances/" + std::to_string(instance_id) + "/nodes/" + std::to_string(node_id);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    json_request.Accept(writer);

    etcd_client_.set(etcd_key, buffer.GetString()).get();
    SetResponse(cntl, 0, "Node added successfully");
  } catch (const std::exception &e) {
    global_logger->error("AddNode exception");
    SetResponse(cntl, 1, std::string("Error accessing etcd: ") + e.what());
  }
}

void MasterServiceImpl::RemoveNode(::google::protobuf::RpcController *controller,
                                   const ::nvm::HttpRequest *
                                   /*request*/,
                                   ::nvm::HttpResponse * /*response*/, ::google::protobuf::Closure *done) {
  global_logger->info("Received RemoveNode request");

  brpc::ClosureGuard done_guard(done);
  auto *cntl = static_cast<brpc::Controller *>(controller);
  // 解析JSON请求
  rapidjson::Document json_request;
  json_request.Parse(cntl->request_attachment().to_string().c_str());

  // 检查JSON文档是否为有效对象
  if (!json_request.IsObject()) {
    global_logger->error("Invalid JSON request");
    cntl->http_response().set_status_code(400);
    SetErrorJsonResponse(cntl, RESPONSE_RETCODE_ERROR, "Invalid JSON request");
    return;
  }

  // 从JSON请求中获取ID
  uint64_t instance_id = json_request[INSTANCE_ID].GetUint64();
  uint64_t node_id = json_request[NODE_ID].GetUint64();

  std::string etcd_key = "/instances/" + std::to_string(instance_id) + "/nodes/" + std::to_string(node_id);

  try {
    etcd::Response etcd_response = etcd_client_.rm(etcd_key).get();
    if (!etcd_response.is_ok()) {
      global_logger->error("RemoveNode etcd error");
      SetResponse(cntl, 1, "Error removing node from etcd: " + etcd_response.error_message());
      return;
    }
    SetResponse(cntl, 0, "Node removed successfully");
  } catch (const std::exception &e) {
    global_logger->error("RemoveNode etcd Exception");
    SetResponse(cntl, 1, "Exception accessing etcd: " + std::string(e.what()));
  }
}

void MasterServiceImpl::GetInstance(::google::protobuf::RpcController *controller,
                                    const ::nvm::HttpRequest *
                                    /*request*/,
                                    ::nvm::HttpResponse * /*response*/, ::google::protobuf::Closure *done) {
  global_logger->info("Received GetInstance request");

  brpc::ClosureGuard done_guard(done);
  auto *cntl = static_cast<brpc::Controller *>(controller);
  // 解析JSON请求
  rapidjson::Document json_request;
  json_request.Parse(cntl->request_attachment().to_string().c_str());

  // 检查JSON文档是否为有效对象
  if (!json_request.IsObject()) {
    global_logger->error("Invalid JSON request");
    cntl->http_response().set_status_code(400);
    SetErrorJsonResponse(cntl, RESPONSE_RETCODE_ERROR, "Invalid JSON request");
    return;
  }

  // 从JSON请求中获取ID
  uint64_t instance_id = json_request[INSTANCE_ID].GetUint64();
  try {
    std::string etcd_key_prefix = "/instances/" + std::to_string(instance_id) + "/nodes/";
    global_logger->debug("etcd key prefix: {}", etcd_key_prefix);

    etcd::Response etcd_response = etcd_client_.ls(etcd_key_prefix).get();
    global_logger->debug("etcd ls response received");

    if (!etcd_response.is_ok()) {
      global_logger->error("Error accessing etcd: {}", etcd_response.error_message());
      SetResponse(cntl, 1, "Error accessing etcd: " + etcd_response.error_message());
      return;
    }

    const auto &keys = etcd_response.keys();
    const auto &values = etcd_response.values();

    rapidjson::Document doc;
    doc.SetObject();
    rapidjson::Document::AllocatorType &allocator = doc.GetAllocator();

    rapidjson::Value nodes_array(rapidjson::kArrayType);
    for (size_t i = 0; i < keys.size(); ++i) {
      global_logger->debug("Processing key: {}", keys[i]);
      rapidjson::Document node_doc;
      node_doc.Parse(values[i].as_string().c_str());
      if (!node_doc.IsObject()) {
        global_logger->warn("Invalid JSON format for key: {}", keys[i]);
        continue;
      }

      // 使用 CopyFrom 方法将节点信息添加到数组中
      rapidjson::Value node_value(node_doc, allocator);
      nodes_array.PushBack(node_value, allocator);
    }

    doc.AddMember("instanceId", instance_id, allocator);
    doc.AddMember("nodes", nodes_array, allocator);

    global_logger->info("Instance info retrieved successfully for instanceId: {}", instance_id);
    SetResponse(cntl, 0, "Instance info retrieved successfully", &doc);
  } catch (const std::exception &e) {
    global_logger->error("Exception accessing etcd: {}", e.what());
    SetResponse(cntl, 1, "Exception accessing etcd: " + std::string(e.what()));
  }
}

void MasterServiceImpl::UpdateNodeStates() {
  CURL *curl = curl_easy_init();

  if (curl == nullptr) {
    global_logger->error("CURL initialization failed");
    return;
  }

  try {
    std::string nodes_key_prefix = "/instances/";
    global_logger->info("Fetching nodes list from etcd");
    etcd::Response etcd_response = etcd_client_.ls(nodes_key_prefix).get();

    for (size_t i = 0; i < etcd_response.keys().size(); ++i) {
      const std::string &node_key = etcd_response.keys()[i];
      const std::string &node_value = etcd_response.values()[i].as_string();

      rapidjson::Document node_doc;
      node_doc.Parse(node_value.c_str());
      if (!node_doc.IsObject()) {
        global_logger->warn("Invalid JSON format for node: {}", node_key);
        continue;
      }

      std::string get_node_url = std::string(node_doc["url"].GetString()) + "/AdminService/GetNode";
      global_logger->debug("Sending request to {}", get_node_url);

      curl_easy_setopt(curl, CURLOPT_URL, get_node_url.c_str());
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, BaseServiceImpl::WriteCallback);

      std::string response_str;
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_str);

      CURLcode res = curl_easy_perform(curl);
      bool needs_update = false;

      if (res != CURLE_OK) {
        global_logger->error("curl_easy_perform() failed: {}", curl_easy_strerror(res));
        node_error_counts_[node_key]++;
        if (node_error_counts_[node_key] >= 5 && node_doc["status"].GetInt() != 0) {
          node_doc["status"].SetInt(0);  // Set status to 0 (abnormal)
          needs_update = true;
        }
      } else {
        node_error_counts_[node_key] = 0;  // Reset error count
        if (node_doc["status"].GetInt() != 1) {
          node_doc["status"].SetInt(1);  // Set status to 1 (normal)
          needs_update = true;
        }

        rapidjson::Document get_node_response;
        get_node_response.Parse(response_str.c_str());
        if (get_node_response.HasMember("node") && get_node_response["node"].IsObject()) {
          std::string state = get_node_response["node"]["state"].GetString();
          int new_role = (state == "leader") ? 0 : 1;

          if (node_doc["role"].GetInt() != new_role) {
            node_doc["role"].SetInt(new_role);  // Update role
            needs_update = true;
          }
        }
      }

      if (needs_update) {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        node_doc.Accept(writer);

        etcd_client_.set(node_key, buffer.GetString()).get();
        global_logger->info("Updated node {} with new status and role", node_key);
      }
    }
  } catch (const std::exception &e) {
    global_logger->error("Exception while updating node states: {}", e.what());
  }

  curl_easy_cleanup(curl);
}

void MasterServiceImpl::GetPartitionConfig(::google::protobuf::RpcController *controller,
                                           const ::nvm::HttpRequest * /*request*/, ::nvm::HttpResponse * /*response*/,
                                           ::google::protobuf::Closure *done) {
  global_logger->info("Received GetPartitionConfig request");

  brpc::ClosureGuard done_guard(done);
  auto *cntl = static_cast<brpc::Controller *>(controller);
  // 解析JSON请求
  rapidjson::Document json_request;
  json_request.Parse(cntl->request_attachment().to_string().c_str());

  // 检查JSON文档是否为有效对象
  if (!json_request.IsObject()) {
    global_logger->error("Invalid JSON request");
    cntl->http_response().set_status_code(400);
    SetErrorJsonResponse(cntl, RESPONSE_RETCODE_ERROR, "Invalid JSON request");
    return;
  }

  // 从JSON请求中获取ID
  uint64_t instance_id = json_request[INSTANCE_ID].GetUint64();
  try {
    PartitionConfig config = DoGetPartitionConfig(instance_id);

    rapidjson::Document doc;
    doc.SetObject();
    rapidjson::Document::AllocatorType &allocator = doc.GetAllocator();

    // 添加分区配置信息到响应
    doc.AddMember("partitionKey", rapidjson::Value(config.partition_key_.c_str(), allocator), allocator);
    doc.AddMember("numberOfPartitions", config.number_of_partitions_, allocator);

    rapidjson::Value partitions_array(rapidjson::kArrayType);
    for (const auto &partition : config.partitions_) {
      rapidjson::Value partition_obj(rapidjson::kObjectType);
      partition_obj.AddMember("partitionId", partition.partition_id_, allocator);
      partition_obj.AddMember("nodeId", partition.node_id_, allocator);
      partitions_array.PushBack(partition_obj, allocator);
    }
    doc.AddMember("partitions", partitions_array, allocator);

    SetResponse(cntl, 0, "Partition config retrieved successfully", &doc);
  } catch (const std::exception &e) {
    SetResponse(cntl, 1, "Exception occurred: " + std::string(e.what()));
  }
}





void MasterServiceImpl::UpdatePartitionConfig(::google::protobuf::RpcController *controller,
                                              const ::nvm::HttpRequest * /*request*/,
                                              ::nvm::HttpResponse * /*response*/, ::google::protobuf::Closure *done) {
 global_logger->info("Received AddNode request");

  brpc::ClosureGuard done_guard(done);
  auto *cntl = static_cast<brpc::Controller *>(controller);
  // 解析JSON请求
  rapidjson::Document json_request;
  json_request.Parse(cntl->request_attachment().to_string().c_str());

  // 检查JSON文档是否为有效对象
  if (!json_request.IsObject()) {
    global_logger->error("Invalid JSON request");
    cntl->http_response().set_status_code(400);
    SetErrorJsonResponse(cntl, RESPONSE_RETCODE_ERROR, "Invalid JSON request");
    return;
  }

  try {
    uint64_t instance_id = json_request["instanceId"].GetUint64();
    std::string partition_key = json_request["partitionKey"].GetString();
    int number_of_partitions = json_request["numberOfPartitions"].GetInt();

    std::list<Partition> partition_list;
    const rapidjson::Value &partitions = json_request["partitions"];
    for (const auto &partition : partitions.GetArray()) {
      uint64_t partition_id = partition["partitionId"].GetUint64();
      uint64_t node_id = partition["nodeId"].GetUint64();
      partition_list.push_back({partition_id, node_id});
    }
    DoUpdatePartitionConfig(instance_id, partition_key, number_of_partitions, partition_list);
    SetResponse(cntl, 0, "Partition configuration updated successfully");
  } catch (const std::exception &e) {
    SetResponse(cntl, 1, std::string("Error updating partition config: ") + e.what());
  }
}

auto MasterServiceImpl::DoGetPartitionConfig(uint64_t instanceId) -> PartitionConfig {
    PartitionConfig config;
    std::string etcd_key = "/instancesConfig/" + std::to_string(instanceId) + "/partitionConfig";
    etcd::Response etcd_response = etcd_client_.get(etcd_key).get();
    rapidjson::Document doc;
    doc.Parse(etcd_response.value().as_string().c_str());

    if (doc.IsObject()) {
        if (doc.HasMember("partitionKey")) {
            config.partition_key_ = doc["partitionKey"].GetString();
        }
        if (doc.HasMember("numberOfPartitions")) {
            config.number_of_partitions_ = doc["numberOfPartitions"].GetInt();
        }
        if (doc.HasMember("partitions") && doc["partitions"].IsArray()) {
            for (const auto& partition : doc["partitions"].GetArray()) {
                Partition p;
                p.partition_id_ = partition["partitionId"].GetUint64();
                p.node_id_ = partition["nodeId"].GetUint64();
                config.partitions_.push_back(p);
            }
        }
    }

    return config;
}

void MasterServiceImpl::DoUpdatePartitionConfig(uint64_t instanceId, const std::string& partitionKey, int numberOfPartitions, const std::list<Partition>& partitions) {
    rapidjson::Document doc;
    doc.SetObject();
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

    // 设置分区键和分区数目
    doc.AddMember("partitionKey", rapidjson::Value(partitionKey.c_str(), allocator), allocator);
    doc.AddMember("numberOfPartitions", numberOfPartitions, allocator);

    // 设置分区映射
    rapidjson::Value partition_array(rapidjson::kArrayType);
    for (const auto& partition : partitions) {
        rapidjson::Value partition_obj(rapidjson::kObjectType);
        partition_obj.AddMember("partitionId", partition.partition_id_, allocator);
        partition_obj.AddMember("nodeId", partition.node_id_, allocator);
        partition_array.PushBack(partition_obj, allocator);
    }
    doc.AddMember("partitions", partition_array, allocator);

    // 将配置写入 etcd
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    std::string etcd_key = "/instancesConfig/" + std::to_string(instanceId) + "/partitionConfig";
    etcd_client_.set(etcd_key, buffer.GetString()).get();
    global_logger->info("Updated partition config for instance {}", instanceId);
}

}  // namespace vectordb