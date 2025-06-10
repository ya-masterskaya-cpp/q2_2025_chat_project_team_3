#pragma once

#include <drogon/WebSocketConnection.h>

class ChatRoomManager {
public:
    static ChatRoomManager& instance();

    void addConnectionToRoom(const drogon::WebSocketConnectionPtr& conn);
    void unregisterConnection(const drogon::WebSocketConnectionPtr& conn);
    void sendToRoom(int32_t room_id, const chat::Envelope& message) const;
    std::vector<chat::UserInfo> getUsersInRoom(int32_t room_id) const;

private:
    ChatRoomManager() = default;
    ChatRoomManager(const ChatRoomManager&) = delete;
    ChatRoomManager& operator=(const ChatRoomManager&) = delete;

    std::unordered_map<int32_t, std::unordered_set<drogon::WebSocketConnectionPtr>> m_user_id_to_conns;
    std::unordered_map<int32_t, std::unordered_set<drogon::WebSocketConnectionPtr>> m_room_to_conns;
    mutable std::shared_mutex m_mutex;
};
