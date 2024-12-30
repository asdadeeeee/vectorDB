#pragma once
#include <rapidjson/document.h>
#include <string>
#include "brpc/controller.h"
#include "brpc/stream.h"
#include "common/constants.h"
#include "database/vector_database.h"
#include "http.pb.h"
#include "httplib/httplib.h"
#include "index/faiss_index.h"
#include "index/index_factory.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
namespace vectordb {
class BaseServiceImpl {
 public:
  enum class CheckType { SEARCH, INSERT, UPSERT };
  void SetJsonResponse(const rapidjson::Document &json_response, brpc::Controller *cntl);

  void SetErrorJsonResponse(brpc::Controller *cntl, int error_code, const std::string &errorMsg);

  auto IsRequestValid(const rapidjson::Document &json_request, CheckType check_type) -> bool;

  auto GetIndexTypeFromRequest(const rapidjson::Document &json_request) -> vectordb::IndexFactory::IndexType;
};

}  // namespace vectordb