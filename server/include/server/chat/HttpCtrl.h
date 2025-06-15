#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

class HttpCtrl : public drogon::HttpController<HttpCtrl> {
public:
    void healthCheck(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const;
    
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(HttpCtrl::healthCheck, "/health", Get);
    METHOD_LIST_END    
};
