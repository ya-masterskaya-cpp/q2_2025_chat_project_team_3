#pragma once

#include <server/chat/IChatRoomService.h>
#include <drogon/WebSocketConnection.h>

class DrogonRoomService : public IChatRoomService {
public:
    DrogonRoomService(const drogon::WebSocketConnectionPtr& conn);

    drogon::Task<void> login(const WsData& locked_caller_data) override;
    drogon::Task<void> logout(const WsData& locked_caller_data) override;
    drogon::Task<void> joinRoom(const WsData& locked_caller_data) override;
    drogon::Task<void> leaveCurrentRoom(const WsData& locked_caller_data) override;

private:
    const drogon::WebSocketConnectionPtr& m_conn;
};
