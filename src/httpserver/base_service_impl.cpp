#include "httpserver/base_service_impl.h"
#include <string>

namespace vectordb {
void BaseServiceImpl::SetJsonResponse(const rapidjson::Document &json_response, brpc::Controller *cntl) {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  json_response.Accept(writer);
  cntl->response_attachment().append(buffer.GetString());
  cntl->http_response().set_content_type(RESPONSE_CONTENT_TYPE_JSON);
}

void BaseServiceImpl::SetTextResponse(const std::string &response, brpc::Controller *cntl) {
  cntl->response_attachment().append(response);
  cntl->http_response().set_content_type(RESPONSE_CONTENT_TYPE_TEXT);
}

void BaseServiceImpl::SetJsonResponse(const std::string &response, brpc::Controller *cntl) {
  cntl->response_attachment().append(response);
  cntl->http_response().set_content_type(RESPONSE_CONTENT_TYPE_TEXT);
}

void BaseServiceImpl::SetErrorJsonResponse(brpc::Controller *cntl, int error_code, const std::string &errorMsg) {
  rapidjson::Document json_response;
  json_response.SetObject();
  rapidjson::Document::AllocatorType &allocator = json_response.GetAllocator();
  json_response.AddMember(RESPONSE_RETCODE, error_code, allocator);
  json_response.AddMember(RESPONSE_ERROR_MSG, rapidjson::StringRef(errorMsg.c_str()), allocator);  // 使用宏定义
  SetJsonResponse(json_response, cntl);
}

auto BaseServiceImpl::IsRequestValid(const rapidjson::Document &json_request, CheckType check_type) -> bool {
  switch (check_type) {
    case CheckType::SEARCH:
      return json_request.HasMember(REQUEST_VECTORS) && json_request.HasMember(REQUEST_K) &&
             (!json_request.HasMember(REQUEST_INDEX_TYPE) || json_request[REQUEST_INDEX_TYPE].IsString());
    case CheckType::INSERT:
    case CheckType::UPSERT:
      return json_request.HasMember(REQUEST_VECTORS) && json_request.HasMember(REQUEST_ID) &&
             (!json_request.HasMember(REQUEST_INDEX_TYPE) || json_request[REQUEST_INDEX_TYPE].IsString());
    default:
      return false;
  }
}

void BaseServiceImpl::SetResponse(brpc::Controller *cntl, int retCode, const std::string &msg,
                                  const rapidjson::Document *data) {
  rapidjson::Document doc;
  doc.SetObject();
  rapidjson::Document::AllocatorType &allocator = doc.GetAllocator();

  doc.AddMember("retCode", retCode, allocator);
  doc.AddMember("msg", rapidjson::Value(msg.c_str(), allocator), allocator);

  if (data != nullptr && data->IsObject()) {
    rapidjson::Value data_value(rapidjson::kObjectType);
    data_value.CopyFrom(*data, allocator);  // 正确地复制 data
    doc.AddMember("data", data_value, allocator);
  }
  SetJsonResponse(doc, cntl);
}

auto BaseServiceImpl::GetIndexTypeFromRequest(const rapidjson::Document &json_request)
    -> vectordb::IndexFactory::IndexType {
  // 获取请求参数中的索引类型
  if (json_request.HasMember(REQUEST_INDEX_TYPE)) {
    std::string index_type_str = json_request[REQUEST_INDEX_TYPE].GetString();
    if (index_type_str == "FLAT") {
      return vectordb::IndexFactory::IndexType::FLAT;
    }
    if (index_type_str == "HNSW") {
      return vectordb::IndexFactory::IndexType::HNSW;
    }
  }
  return vectordb::IndexFactory::IndexType::UNKNOWN;
}

auto BaseServiceImpl::WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) -> size_t {
  (static_cast<std::string *>(userp))->append(static_cast<char *>(contents), size * nmemb);
  return size * nmemb;
}
}  // namespace vectordb
