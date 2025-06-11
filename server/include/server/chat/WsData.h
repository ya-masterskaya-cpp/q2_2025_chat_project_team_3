#pragma once

struct User {
    int32_t id;
    std::string name;
};

struct CurrentRoom {
    int32_t id;
    chat::UserRights rights;
};

//user's data associated with websocket
struct WsData {
    std::optional<User> user;
    std::optional<CurrentRoom> room;
};
