#pragma once
#include <string>
#include <optional>

struct WsData {
    bool authenticated{false};
    std::string username;
    std::optional<std::string> currentRoom;
};
