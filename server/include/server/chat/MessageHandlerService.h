#pragma once

#include <server/chat/WsData.h>

namespace server {

class MessageHandlers;
class IChatRoomService;

class MessageHandlerService {
public:
    explicit MessageHandlerService(std::unique_ptr<MessageHandlers> handlers);
    ~MessageHandlerService();

    drogon::Task<chat::Envelope> processMessage(const WsDataPtr& wsData, const chat::Envelope& env, IChatRoomService& room_service) const;
private:
    std::unique_ptr<MessageHandlers> m_handlers;

};

} // namespace server
