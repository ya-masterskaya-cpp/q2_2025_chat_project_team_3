#pragma once

#include <drogon/WebSocketConnection.h>
#include <server/chat/WsData.h>

namespace server {

class ChatRoomManager {
public:
    static ChatRoomManager& instance();
    
    drogon::Task<void> registerConnection(int32_t user_id, const drogon::WebSocketConnectionPtr& conn);
    drogon::Task<void> unregisterConnection(const drogon::WebSocketConnectionPtr& conn, const WsData& locked_data);
    drogon::Task<void> addConnectionToRoom(int32_t room_id, const drogon::WebSocketConnectionPtr& conn);
    drogon::Task<void> removeConnectionFromRoom(const drogon::WebSocketConnectionPtr& conn, const WsData& locked_data);

    drogon::Task<std::vector<chat::UserInfo>> getUsersInRoomAsync(
        int32_t room_id, const WsData& locked_caller_data) const;
    
    drogon::Task<void> sendToRoom(int32_t room_id, const chat::Envelope& message) const;
    drogon::Task<void> sendToAll(const chat::Envelope& message) const;
    
    drogon::Task<std::vector<drogon::WebSocketConnectionPtr>> onRoomDeleted(int32_t room_id);
    
private:
    ChatRoomManager() = default;
    ChatRoomManager(const ChatRoomManager&) = delete;
    ChatRoomManager& operator=(const ChatRoomManager&) = delete;

    void sendToRoom_unsafe(int32_t room_id, const chat::Envelope& message) const;
    void sendToAll_unsafe(const chat::Envelope& message) const;

    mutable common::Guarded<int> m_manager_mutex{0};

    std::unordered_map<int32_t, std::unordered_set<drogon::WebSocketConnectionPtr>> m_user_id_to_conns;
    std::unordered_map<int32_t, std::unordered_set<drogon::WebSocketConnectionPtr>> m_room_to_conns;
};

} // namespace server
