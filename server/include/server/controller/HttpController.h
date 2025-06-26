#pragma once
#include <drogon/HttpController.h>

namespace server {

using namespace drogon;

namespace http {
	
class HttpController : public drogon::HttpController<HttpController> {
public:
    void healthCheck(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const;
    
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(HttpController::healthCheck, "/health", Get);
    METHOD_LIST_END    
};

} // namespace http

} // namespace server
