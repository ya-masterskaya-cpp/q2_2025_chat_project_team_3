#pragma once

#include <drogon/WebSocketController.h>

namespace server {

class WsRequestProcessor;

class WsController : public drogon::WebSocketController<WsController> {
public:
    WsController();
    ~WsController();

    void handleNewConnection(const drogon::HttpRequestPtr& req, const drogon::WebSocketConnectionPtr& conn) override;
    void handleNewMessage(const drogon::WebSocketConnectionPtr& conn, std::string&& msg_str, const drogon::WebSocketMessageType& type) override;
    void handleConnectionClosed(const drogon::WebSocketConnectionPtr& conn) override;

    WS_PATH_LIST_BEGIN
        WS_PATH_ADD("/ws", drogon::Get);
    WS_PATH_LIST_END

private:
    std::unique_ptr<WsRequestProcessor> m_requestProcessor;
};

} // namespace server
