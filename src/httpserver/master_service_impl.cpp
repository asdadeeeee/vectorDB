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
    std::string instance_id = json_request[INSTANCE_ID].GetString();
    std::string node_id = json_request[NODE_ID].GetString();
    std::string etcd_key = "/instances/" + instance_id + "/nodes/" + node_id;

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

void MasterServiceImpl:: GetInstance(::google::protobuf::RpcController *controller, const ::nvm::HttpRequest *
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
        std::string etcd_key_prefix = "/instances/" +  std::to_string(instance_id)+ "/nodes/";
        global_logger->debug("etcd key prefix: {}", etcd_key_prefix);

        etcd::Response etcd_response = etcd_client_.ls(etcd_key_prefix).get();
        global_logger->debug("etcd ls response received");

        if (!etcd_response.is_ok()) {
            global_logger->error("Error accessing etcd: {}", etcd_response.error_message());
            SetResponse(cntl, 1, "Error accessing etcd: " + etcd_response.error_message());
            return;
        }

        const auto& keys = etcd_response.keys();
        const auto& values = etcd_response.values();

        rapidjson::Document doc;
        doc.SetObject();
        rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

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
    } catch (const std::exception& e) {
        global_logger->error("Exception accessing etcd: {}", e.what());
        SetResponse(cntl, 1, "Exception accessing etcd: " + std::string(e.what()));
    }
}

}  // namespace vectordb