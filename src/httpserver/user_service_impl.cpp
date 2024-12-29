#include "httpserver/user_service_impl.h"
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
void UserServiceImpl::search(::google::protobuf::RpcController *controller, const ::nvm::HttpRequest * /*request*/,
                             ::nvm::HttpResponse * /*response*/, ::google::protobuf::Closure *done) {
  global_logger->debug("Received search request");

  brpc::ClosureGuard done_guard(done);
  auto *cntl = static_cast<brpc::Controller *>(controller);

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

  // 检查请求的合法性
  if (!IsRequestValid(json_request, BaseServiceImpl::CheckType::SEARCH)) {
    global_logger->error("Missing vectors or k parameter in the request");
    cntl->http_response().set_status_code(400);
    SetErrorJsonResponse(cntl, RESPONSE_RETCODE_ERROR, "Missing vectors or k parameter in the request");
    done->Run();
    return;
  }

  // 获取查询参数
  std::vector<float> query;
  for (const auto &q : json_request[REQUEST_VECTORS].GetArray()) {
    query.push_back(q.GetFloat());
  }
  int k = json_request[REQUEST_K].GetInt();

  global_logger->debug("Query parameters: k = {}", k);

  // 获取请求参数中的索引类型
  IndexFactory::IndexType index_type = GetIndexTypeFromRequest(json_request);

  // 如果索引类型为UNKNOWN，返回400错误
  if (index_type == IndexFactory::IndexType::UNKNOWN) {
    global_logger->error("Invalid indexType parameter in the request");
    cntl->http_response().set_status_code(400);
    SetErrorJsonResponse(cntl, RESPONSE_RETCODE_ERROR, "Invalid indexType parameter in the request");
    done->Run();
    return;
  }

  // 使用 VectorDatabase 的 search 接口执行查询
  std::pair<std::vector<int64_t>, std::vector<float>> results = vector_database_->Search(json_request);

  // 将结果转换为JSON
  rapidjson::Document json_response;
  json_response.SetObject();
  rapidjson::Document::AllocatorType &allocator = json_response.GetAllocator();

  // 检查是否有有效的搜索结果
  bool valid_results = false;
  rapidjson::Value vectors(rapidjson::kArrayType);
  rapidjson::Value distances(rapidjson::kArrayType);
  for (size_t i = 0; i < results.first.size(); ++i) {
    if (results.first[i] != -1) {
      valid_results = true;
      vectors.PushBack(results.first[i], allocator);
      distances.PushBack(results.second[i], allocator);
    }
  }

  if (valid_results) {
    json_response.AddMember(RESPONSE_VECTORS, vectors, allocator);
    json_response.AddMember(RESPONSE_DISTANCES, distances, allocator);
  }

  // 设置响应
  json_response.AddMember(RESPONSE_RETCODE, RESPONSE_RETCODE_SUCCESS, allocator);
  SetJsonResponse(json_response, cntl);
}

void UserServiceImpl::insert(::google::protobuf::RpcController *controller, const ::nvm::HttpRequest * /*request*/,
                             ::nvm::HttpResponse * /*response*/, ::google::protobuf::Closure *done) {
  global_logger->debug("Received insert request");
  brpc::ClosureGuard done_guard(done);
  auto *cntl = static_cast<brpc::Controller *>(controller);
  // 解析JSON请求
  rapidjson::Document json_request;
  json_request.Parse(cntl->request_attachment().to_string().c_str());

  // 打印用户的输入参数
  global_logger->info("Insert request parameters: {}", cntl->request_attachment().to_string());

  // 检查JSON文档是否为有效对象
  if (!json_request.IsObject()) {
    global_logger->error("Invalid JSON request");
    cntl->http_response().set_status_code(400);
    SetErrorJsonResponse(cntl, RESPONSE_RETCODE_ERROR, "Invalid JSON request");
    return;
  }

  // 检查请求的合法性
  if (!IsRequestValid(json_request, BaseServiceImpl::CheckType::INSERT)) {  // 添加对isRequestValid的调用
    global_logger->error("Missing vectors or id parameter in the request");
    cntl->http_response().set_status_code(400);
    SetErrorJsonResponse(cntl, RESPONSE_RETCODE_ERROR, "Missing vectors or k parameter in the request");
    return;
  }

  // 获取插入参数
  std::vector<float> data;
  for (const auto &d : json_request[REQUEST_VECTORS].GetArray()) {
    data.push_back(d.GetFloat());
  }
  uint64_t label = json_request[REQUEST_ID].GetUint64();  // 使用宏定义

  global_logger->debug("Insert parameters: label = {}", label);

  // 获取请求参数中的索引类型
  IndexFactory::IndexType index_type = GetIndexTypeFromRequest(json_request);

  // 如果索引类型为UNKNOWN，返回400错误
  if (index_type == IndexFactory::IndexType::UNKNOWN) {
    global_logger->error("Invalid indexType parameter in the request");
    cntl->http_response().set_status_code(400);
    SetErrorJsonResponse(cntl, RESPONSE_RETCODE_ERROR, "Invalid indexType parameter in the request");
    return;
  }

  // 使用全局IndexFactory获取索引对象
  void *index = IndexFactory::Instance().GetIndex(index_type);
  assert(index != nullptr);

  // 根据索引类型初始化索引对象并调用insert_vectors函数
  switch (index_type) {
    case IndexFactory::IndexType::FLAT: {
      auto *faiss_index = static_cast<FaissIndex *>(index);
      faiss_index->InsertVectors(data, label);
      break;
    }
    case IndexFactory::IndexType::HNSW: {
      auto *hnsw_index = static_cast<HNSWLibIndex *>(index);
      hnsw_index->InsertVectors(data, label);
      break;
    }
    // 在此处添加其他索引类型的处理逻辑
    default:
      break;
  }

  // 设置响应
  rapidjson::Document json_response;
  json_response.SetObject();
  rapidjson::Document::AllocatorType &allocator = json_response.GetAllocator();

  // 添加retCode到响应
  json_response.AddMember(RESPONSE_RETCODE, RESPONSE_RETCODE_SUCCESS, allocator);

  SetJsonResponse(json_response, cntl);
}

void UserServiceImpl::upsert(::google::protobuf::RpcController *controller, const ::nvm::HttpRequest * /*request*/,
                             ::nvm::HttpResponse * /*response*/, ::google::protobuf::Closure *done) {
  global_logger->debug("Received upsert request");
  brpc::ClosureGuard done_guard(done);
  auto *cntl = static_cast<brpc::Controller *>(controller);

  // 解析JSON请求
  rapidjson::Document json_request;
  json_request.Parse(cntl->request_attachment().to_string().c_str());

  global_logger->info("Upsert request parameters: {}", cntl->request_attachment().to_string());

  // 检查JSON文档是否为有效对象
  if (!json_request.IsObject()) {
    global_logger->error("Invalid JSON request");
    cntl->http_response().set_status_code(400);
    SetErrorJsonResponse(cntl, RESPONSE_RETCODE_ERROR, "Invalid JSON request");
    return;
  }

  // 检查请求的合法性
  if (!IsRequestValid(json_request, BaseServiceImpl::CheckType::UPSERT)) {
    global_logger->error("Missing vectors or id parameter in the request");
    cntl->http_response().set_status_code(400);
    SetErrorJsonResponse(cntl, RESPONSE_RETCODE_ERROR, "Missing vectors or id parameter in the request");
    return;
  }

  uint64_t label = json_request[REQUEST_ID].GetUint64();

  // 获取请求参数中的索引类型
  IndexFactory::IndexType index_type = GetIndexTypeFromRequest(json_request);
  vector_database_->Upsert(label, json_request, index_type);
  // 在 upsert 调用之后调用 VectorDatabase::writeWALLog
  vector_database_->WriteWalLog("upsert", json_request);

  rapidjson::Document json_response;
  json_response.SetObject();
  rapidjson::Document::AllocatorType &response_allocator = json_response.GetAllocator();

  // 添加retCode到响应
  json_response.AddMember(RESPONSE_RETCODE, RESPONSE_RETCODE_SUCCESS, response_allocator);

  SetJsonResponse(json_response, cntl);
}

void UserServiceImpl::query(::google::protobuf::RpcController *controller, const ::nvm::HttpRequest * /*request*/,
                            ::nvm::HttpResponse * /*response*/, ::google::protobuf::Closure *done) {
  global_logger->debug("Received query request");

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
  uint64_t id = json_request[REQUEST_ID].GetUint64();  // 使用宏REQUEST_ID

  // 查询JSON数据
  rapidjson::Document json_data = vector_database_->Query(id);

  // 将结果转换为JSON
  rapidjson::Document json_response;
  json_response.SetObject();
  rapidjson::Document::AllocatorType &allocator = json_response.GetAllocator();

  // 如果查询到向量，则将json_data对象的内容合并到json_response对象中
  if (!json_data.IsNull()) {
    for (auto it = json_data.MemberBegin(); it != json_data.MemberEnd(); ++it) {
      json_response.AddMember(it->name, it->value, allocator);
    }
  }

  // 设置响应
  json_response.AddMember(RESPONSE_RETCODE, RESPONSE_RETCODE_SUCCESS, allocator);
  SetJsonResponse(json_response, cntl);
}

}  // namespace vectordb