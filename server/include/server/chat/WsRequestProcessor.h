#pragma once

#include <drogon/WebSocketConnection.h>

class MessageHandlerService;

class WsRequestProcessor {
public:
    explicit WsRequestProcessor(std::unique_ptr<MessageHandlerService> dispatcher);
    ~WsRequestProcessor();

    drogon::Task<> handleIncomingMessage(drogon::WebSocketConnectionPtr conn, std::string bytes) const;
private:
    std::unique_ptr<MessageHandlerService> m_dispatcher;
};
