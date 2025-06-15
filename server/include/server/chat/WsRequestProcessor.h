#pragma once

#include <server/chat/DrogonRoomService.h>

class WsRequestProcessor {
public:
    static drogon::Task<> handleIncomingMessage(const drogon::WebSocketConnectionPtr& conn, const std::string& bytes);
};
