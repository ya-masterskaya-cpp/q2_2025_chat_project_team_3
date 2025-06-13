#pragma once
#include <optional>
#include <stdint.h>
#include <string>

enum class USER_STATUS {
    Unauthenticated, // any ws conn
    Registering,     // 1'st step of reg
    Authenticating,  // 1'st step of auth
    Authenticated    // authenticated user
};

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
    USER_STATUS status = USER_STATUS::Unauthenticated;
};
