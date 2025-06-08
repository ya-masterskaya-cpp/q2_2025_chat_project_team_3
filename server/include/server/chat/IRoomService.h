#pragma once

class IRoomService {
public:
    virtual ~IRoomService() = default;
    virtual void joinRoom() = 0;
    virtual void leaveCurrentRoom() = 0;
};
