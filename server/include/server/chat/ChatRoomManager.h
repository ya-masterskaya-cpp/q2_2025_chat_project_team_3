#pragma once

#include <drogon/WebSocketConnection.h>

class ChatRoomManager {
public:
    static ChatRoomManager& instance();
    void registerConnection(const drogon::WebSocketConnectionPtr& conn);
    void addConnectionToRoom(const drogon::WebSocketConnectionPtr& conn);
    void removeConnectionFromRoom(const drogon::WebSocketConnectionPtr& conn);
    void unregisterConnection(const drogon::WebSocketConnectionPtr& conn);
    void sendToRoom(int32_t room_id, const chat::Envelope& message) const;
    std::vector<chat::UserInfo> getUsersInRoom(int32_t room_id) const;
    void sendToAll(const chat::Envelope& message) const;
    void onRoomDeleted(int32_t room_id);
    
private:
    ChatRoomManager() = default;
    ChatRoomManager(const ChatRoomManager&) = delete;
    ChatRoomManager& operator=(const ChatRoomManager&) = delete;

    void sendToRoom_unsafe(int32_t room_id, const chat::Envelope& message) const;
    void removeFromRoom_unsafe(const drogon::WebSocketConnectionPtr& conn);
    void sendToAll_unsafe(const chat::Envelope& message) const;

    std::unordered_map<int32_t, std::unordered_set<drogon::WebSocketConnectionPtr>> m_user_id_to_conns;
    std::unordered_map<int32_t, std::unordered_set<drogon::WebSocketConnectionPtr>> m_room_to_conns;
    mutable std::shared_mutex m_mutex;
};
