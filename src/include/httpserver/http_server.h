#pragma once
#include "index/faiss_index.h"
#include "httplib/httplib.h"
#include "index/index_factory.h"
#include "database/vector_database.h"
#include "httpserver/admin_service_impl.h"
#include "httpserver/user_service_impl.h"
#include <rapidjson/document.h>
#include <string>

namespace vectordb {

class HttpServer: public brpc::Server {
public:
   auto Init(VectorDatabase *vector_database) -> bool;

    // enum class CheckType {
    //     SEARCH,
    //     INSERT,
    //     UPSERT 
    // };

    // HttpServer(std::string& host, int port, VectorDatabase* vector_database);
    // void Start();

private:
    void SearchHandler(const httplib::Request& req, httplib::Response& res);
    void InsertHandler(const httplib::Request& req, httplib::Response& res);

    void UpsertHandler(const httplib::Request& req, httplib::Response& res);
    void QueryHandler(const httplib::Request& req, httplib::Response& res);
    void SnapshotHandler(const httplib::Request& req, httplib::Response& res);
    void SetJsonResponse(const rapidjson::Document& json_response, httplib::Response& res);
    void SetErrorJsonResponse(httplib::Response& res, int error_code, const std::string& errorMsg); 
    auto IsRequestValid(const rapidjson::Document& json_request, CheckType check_type) -> bool;
    auto GetIndexTypeFromRequest(const rapidjson::Document& json_request) -> vectordb::IndexFactory::IndexType; 

    httplib::Server server_;
    std::string host_;
    int port_;
    VectorDatabase* vector_database_;
    std::unique_ptr<UserServiceImpl> user_service_impl_;
    std::unique_ptr<AdminServiceImpl> admin_service_impl_;
};
}  // namespace vectordb
