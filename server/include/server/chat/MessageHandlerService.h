#pragma once

#include <server/chat/WsData.h>
#include <server/chat/IRoomService.h>

class MessageHandlerService {
public:
    static drogon::Task<chat::Envelope> processMessage(const std::shared_ptr<WsData>& wsData, const chat::Envelope& env, IRoomService& room_service);
};
