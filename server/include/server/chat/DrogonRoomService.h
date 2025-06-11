#pragma once

#include <server/chat/IRoomService.h>
#include <drogon/WebSocketConnection.h>

class DrogonRoomService : public IRoomService {
public:
    DrogonRoomService(const drogon::WebSocketConnectionPtr& conn);

    void joinRoom() override;
    void leaveCurrentRoom() override;

private:
    const drogon::WebSocketConnectionPtr& m_conn;
};
