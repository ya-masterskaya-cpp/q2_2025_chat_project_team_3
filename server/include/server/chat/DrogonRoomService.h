#pragma once

#include <server/chat/IChatRoomService.h>
#include <drogon/WebSocketConnection.h>

class DrogonRoomService : public IChatRoomService {
public:
    DrogonRoomService(const drogon::WebSocketConnectionPtr& conn);

    void joinRoom() override;
    void leaveCurrentRoom() override;
    void logout() override;
    void login() override;

private:
    const drogon::WebSocketConnectionPtr& m_conn;
};
