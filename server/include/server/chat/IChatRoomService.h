#pragma once

#include <server/chat/WsData.h>

class IChatRoomService {
public:
    virtual ~IChatRoomService() = default;
    virtual drogon::Task<void> login(const WsData& locked_caller_data) = 0;
    virtual drogon::Task<void> logout(const WsData& locked_caller_data) = 0;
    virtual drogon::Task<void> joinRoom(const WsData& locked_caller_data) = 0;
    virtual drogon::Task<void> leaveCurrentRoom(const WsData& locked_caller_data) = 0;
};
