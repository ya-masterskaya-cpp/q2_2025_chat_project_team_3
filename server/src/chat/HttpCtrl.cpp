#include "server/chat/HttpCtrl.h"

namespace http {

void HttpCtrl::healthCheck(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const {
    auto resp = HttpResponse::newHttpResponse();
    resp->setStatusCode(k200OK);
    resp->setContentTypeCode(CT_TEXT_PLAIN);
    resp->setBody("OK");
    callback(resp);
}

} // namespace http
