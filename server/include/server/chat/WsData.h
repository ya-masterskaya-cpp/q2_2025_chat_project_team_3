#pragma once
#include <string>
#include <optional>

//user's data associated with websocket
struct WsData {
    bool authenticated{false};
    std::string username;
    std::optional<std::string> currentRoom;
};
