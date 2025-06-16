#pragma once

class IChatRoomService {
public:
    virtual ~IChatRoomService() = default;
    virtual void joinRoom() = 0;
    virtual void leaveCurrentRoom() = 0;
    virtual void logout() = 0;
    virtual void login() = 0;
};
