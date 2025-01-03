#include "httpserver/base_service_impl.h"

namespace vectordb {
void BaseServiceImpl::SetJsonResponse(const rapidjson::Document &json_response, brpc::Controller *cntl) {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  json_response.Accept(writer);
  cntl->response_attachment().append(buffer.GetString());
  cntl->http_response().set_content_type(RESPONSE_CONTENT_TYPE_JSON);
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
}  // namespace vectordb
