#include "httpserver/admin_service_impl.h"
#include <brpc/controller.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <cassert>
#include <cstdint>
#include <iostream>
#include "common/constants.h"
#include "index/faiss_index.h"
#include "index/hnswlib_index.h"
#include "index/index_factory.h"
#include "logger/logger.h"

namespace vectordb {
void AdminServiceImpl::snapshot(::google::protobuf::RpcController *controller, const ::nvm::HttpRequest * /*request*/,
                                ::nvm::HttpResponse * /*response*/, ::google::protobuf::Closure *done) {
  global_logger->debug("Received snapshot request");
  brpc::ClosureGuard done_guard(done);
  auto *cntl = static_cast<brpc::Controller *>(controller);

  vector_database_->TakeSnapshot();  // 调用 VectorDatabase::takeSnapshot

  rapidjson::Document json_response;
  json_response.SetObject();
  rapidjson::Document::AllocatorType &allocator = json_response.GetAllocator();

  // 设置响应
  json_response.AddMember(RESPONSE_RETCODE, RESPONSE_RETCODE_SUCCESS, allocator);
  SetJsonResponse(json_response, cntl);
}

void AdminServiceImpl::SetLeader(::google::protobuf::RpcController *controller, const ::nvm::HttpRequest * /*request*/,
                                 ::nvm::HttpResponse * /*response*/, ::google::protobuf::Closure *done) {
  global_logger->debug("Received setLeader request");
  brpc::ClosureGuard done_guard(done);
  auto *cntl = static_cast<brpc::Controller *>(controller);

  // 将当前节点设置为主节点
  raft_stuff_->EnableElectionTimeout(10000, 20000);

  rapidjson::Document json_response;
  json_response.SetObject();
  rapidjson::Document::AllocatorType &allocator = json_response.GetAllocator();

  // 设置响应
  json_response.AddMember(RESPONSE_RETCODE, RESPONSE_RETCODE_SUCCESS, allocator);
  SetJsonResponse(json_response, cntl);
}

void AdminServiceImpl::AddFollower(::google::protobuf::RpcController *controller,
                                   const ::nvm::HttpRequest * /*request*/, ::nvm::HttpResponse * /*response*/,
                                   ::google::protobuf::Closure *done) {
  global_logger->debug("Received addFollower request");

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
    done->Run();
    return;
  }

  // 检查当前节点是否为leader
  if (!raft_stuff_->IsLeader()) {
    global_logger->error("Current node is not the leader");
    cntl->http_response().set_status_code(400);
    SetErrorJsonResponse(cntl, RESPONSE_RETCODE_ERROR, "Invalid JSON request");
    done->Run();
    return;
  }

  // 从JSON请求中获取follower节点信息
  int node_id = json_request["nodeId"].GetInt();
  std::string endpoint = json_request["endpoint"].GetString();

  // 调用 RaftStuff 的 addSrv 方法将新的follower节点添加到集群中
  auto ret = raft_stuff_->AddSrv(node_id, endpoint);
  if (!ret->get_accepted()) {
    global_logger->error("raft_stuff  AddSrv  failed");
    cntl->http_response().set_status_code(400);
    SetErrorJsonResponse(cntl, RESPONSE_RETCODE_ERROR, "raft_stuff  AddSrv  failed");
    done->Run();
    return;
  }

  // Wait until it appears in server list.
  bool success = false;
  const size_t max_try = 40;
  for (size_t jj = 0; jj < max_try; ++jj) {
    global_logger->info("Wait for add follower.");
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    auto conf = raft_stuff_->GetSrvConfig(node_id);
    if (conf) {
      success = true;
      global_logger->info(" Add follower done.");
      break;
    }
  }

  if (!success) {
    global_logger->error("raft_stuff  AddSrv  failed");
    cntl->http_response().set_status_code(400);
    SetErrorJsonResponse(cntl, RESPONSE_RETCODE_ERROR, "raft_stuff  AddSrv  failed");
    done->Run();
    return;
  }

  rapidjson::Document json_response;
  json_response.SetObject();
  rapidjson::Document::AllocatorType &allocator = json_response.GetAllocator();

  // 设置响应
  json_response.AddMember(RESPONSE_RETCODE, RESPONSE_RETCODE_SUCCESS, allocator);
  SetJsonResponse(json_response, cntl);
}

void AdminServiceImpl::ListNode(::google::protobuf::RpcController *controller, const ::nvm::HttpRequest * /*request*/,
                                ::nvm::HttpResponse * /*response*/, ::google::protobuf::Closure *done) {
  global_logger->debug("Received listNode request");
  brpc::ClosureGuard done_guard(done);
  auto *cntl = static_cast<brpc::Controller *>(controller);

  // 获取所有节点信息
  auto nodes_info = raft_stuff_->GetAllNodesInfo();

  rapidjson::Document json_response;
  json_response.SetObject();
  rapidjson::Document::AllocatorType &allocator = json_response.GetAllocator();

  // 将节点信息添加到JSON响应中
  rapidjson::Value nodes_array(rapidjson::kArrayType);
  for (const auto &node_info : nodes_info) {
    rapidjson::Value node_object(rapidjson::kObjectType);
    node_object.AddMember("nodeId", std::get<0>(node_info), allocator);
    node_object.AddMember("endpoint", rapidjson::Value(std::get<1>(node_info).c_str(), allocator), allocator);
    node_object.AddMember("state", rapidjson::Value(std::get<2>(node_info).c_str(), allocator),
                          allocator);                                               // 添加节点状态
    node_object.AddMember("last_log_idx", std::get<3>(node_info), allocator);       // 添加节点最后日志索引
    node_object.AddMember("last_succ_resp_us", std::get<4>(node_info), allocator);  // 添加节点最后成功响应时间
    nodes_array.PushBack(node_object, allocator);
  }
  json_response.AddMember("nodes", nodes_array, allocator);

  // 设置响应
  json_response.AddMember(RESPONSE_RETCODE, RESPONSE_RETCODE_SUCCESS, allocator);
  SetJsonResponse(json_response, cntl);
}

void AdminServiceImpl::GetNode(::google::protobuf::RpcController *controller, const ::nvm::HttpRequest * /*request*/,
                               ::nvm::HttpResponse * /*response*/, ::google::protobuf::Closure *done) {
  global_logger->debug("Received getNode request");
  brpc::ClosureGuard done_guard(done);
  auto *cntl = static_cast<brpc::Controller *>(controller);

  // 获取所有节点信息
  std::tuple<int, std::string, std::string, nuraft::ulong, nuraft::ulong> node_info =
      raft_stuff_->GetCurrentNodesInfo();

  rapidjson::Document json_response;
  json_response.SetObject();
  rapidjson::Document::AllocatorType &allocator = json_response.GetAllocator();

  // 将节点信息添加到JSON响应中
  rapidjson::Value nodes_array(rapidjson::kArrayType);
  rapidjson::Value node_object(rapidjson::kObjectType);
  node_object.AddMember("nodeId", std::get<0>(node_info), allocator);
  node_object.AddMember("endpoint", rapidjson::Value(std::get<1>(node_info).c_str(), allocator), allocator);
  node_object.AddMember("state", rapidjson::Value(std::get<2>(node_info).c_str(), allocator),
                        allocator);                                               // 添加节点状态
  node_object.AddMember("last_log_idx", std::get<3>(node_info), allocator);       // 添加节点最后日志索引
  node_object.AddMember("last_succ_resp_us", std::get<4>(node_info), allocator);  // 添加节点最后成功响应时间

  json_response.AddMember("node", node_object, allocator);

  // 设置响应
  json_response.AddMember(RESPONSE_RETCODE, RESPONSE_RETCODE_SUCCESS, allocator);
  SetJsonResponse(json_response, cntl);
}

}  // namespace vectordb