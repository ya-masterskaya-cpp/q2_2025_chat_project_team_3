#pragma once

class MessageHandlers;
class IChatRoomService;
struct WsData;

class MessageHandlerService {
public:
    explicit MessageHandlerService(std::unique_ptr<MessageHandlers> handlers);
    ~MessageHandlerService();

    drogon::Task<chat::Envelope> processMessage(const std::shared_ptr<WsData>& wsData, const chat::Envelope& env, IChatRoomService& room_service) const;
private:
    std::unique_ptr<MessageHandlers> m_handlers;
};
