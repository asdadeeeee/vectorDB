#pragma once
#include "index/faiss_index.h"
#include "httplib.h"
#include "index/index_factory.h"
#include <rapidjson/document.h>
#include <string>

namespace vectordb {
class HttpServer {
public:
    enum class CheckType {
        SEARCH,
        INSERT 
    };

    HttpServer(std::string& host, int port);
    void Start();

private:
    void SearchHandler(const httplib::Request& req, httplib::Response& res);
    void InsertHandler(const httplib::Request& req, httplib::Response& res);
    void SetJsonResponse(const rapidjson::Document& json_response, httplib::Response& res);
    void SetErrorJsonResponse(httplib::Response& res, int error_code, const std::string& errorMsg); 
    auto IsRequestValid(const rapidjson::Document& json_request, CheckType check_type) -> bool;
    auto GetIndexTypeFromRequest(const rapidjson::Document& json_request) -> vectordb::IndexFactory::IndexType; 

    httplib::Server server_;
    std::string host_;
    int port_;
};
}  // namespace vectordb
