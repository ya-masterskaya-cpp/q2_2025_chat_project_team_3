#include <server/chat/ChatRoomManager.h>
#include <common/utils/utils.h>
#include <server/chat/WsData.h>

#include <server/chat/ChatRoomManager.h>
#include <common/utils/utils.h>

namespace server {

ChatRoomManager& ChatRoomManager::instance() {
    static ChatRoomManager inst;
    return inst;
}

// Helper function to create UserInfo from WsData.
// This is synchronous because it operates on already-locked data.
static chat::UserInfo makeUserInfo(const WsData& data) {
    chat::UserInfo ui;
    if(data.user) {
        ui.set_user_id(data.user->id);
        ui.set_user_name(data.user->name);
        if(data.room) {
            ui.set_user_room_rights(data.room->rights);
        }
    }
    return ui;
}

drogon::Task<std::vector<chat::UserInfo>> ChatRoomManager::getUsersInRoom(
    int32_t room_id, const WsData& locked_data) const {
    auto manager_lock = co_await m_manager_mutex.lock_shared();

    auto it = m_room_to_conns.find(room_id);
    if(it == m_room_to_conns.end()) {
        co_return {};
    }

    std::vector<chat::UserInfo> user_list;
    user_list.reserve(it->second.size());

    for(const auto& conn : it->second) {
        auto peer_guarded = conn->getContext<WsDataGuarded>();

        if(peer_guarded->isHolding(locked_data)) {
            user_list.push_back(makeUserInfo(locked_data));
        } else {
            auto peer_proxy = co_await peer_guarded->lock_shared();
            user_list.push_back(makeUserInfo(*peer_proxy));
        }
    }
    co_return user_list;
}

drogon::Task<void> ChatRoomManager::registerConnection(int32_t user_id, const drogon::WebSocketConnectionPtr& conn) {
    auto lock = co_await m_manager_mutex.lock_unique();
    m_user_id_to_conns[user_id].insert(conn);
}

drogon::Task<void> ChatRoomManager::addConnectionToRoom(int32_t room_id, const drogon::WebSocketConnectionPtr& conn) {
    auto lock = co_await m_manager_mutex.lock_unique();
    m_room_to_conns[room_id].insert(conn);
}

drogon::Task<void> ChatRoomManager::removeConnectionFromRoom(const drogon::WebSocketConnectionPtr& conn, const WsData& locked_data) {
    auto lock = co_await m_manager_mutex.lock_unique();
    
    if (locked_data.user && locked_data.room) {
        int32_t room_id = locked_data.room->id;
        
        chat::Envelope user_left_msg;
        auto* user_info = user_left_msg.mutable_user_left()->mutable_user();
        user_info->set_user_id(locked_data.user->id);
        user_info->set_user_name(locked_data.user->name);
        user_info->set_user_room_rights(locked_data.room->rights);
        sendToRoom_unsafe(room_id, user_left_msg);
        
        if (auto it = m_room_to_conns.find(room_id); it != m_room_to_conns.end()) {
            it->second.erase(conn);
            if (it->second.empty()) {
                m_room_to_conns.erase(it);
            }
        }
    }
}

drogon::Task<void> ChatRoomManager::unregisterConnection(const drogon::WebSocketConnectionPtr& conn, const WsData& locked_data) {
    // First, handle the room departure logic if the user was in a room.
    if (locked_data.room) {
        co_await removeConnectionFromRoom(conn, locked_data);
    }
    
    // Then, perform the final user cleanup.
    auto lock = co_await m_manager_mutex.lock_unique();
    if (locked_data.user) {
        if (auto it = m_user_id_to_conns.find(locked_data.user->id); it != m_user_id_to_conns.end()) {
            it->second.erase(conn);
            if (it->second.empty()) {
                m_user_id_to_conns.erase(it);
            }
        }
    }
}

drogon::Task<void> ChatRoomManager::onRoomDeleted(int32_t room_id) {
    auto lock = co_await m_manager_mutex.lock_unique();
    if (auto it = m_room_to_conns.find(room_id); it != m_room_to_conns.end()) {
        m_room_to_conns.erase(it);
    }
    
    chat::Envelope room_deleted_msg;
    room_deleted_msg.mutable_room_deleted()->set_room_id(room_id);
    sendToAll_unsafe(room_deleted_msg);
}

drogon::Task<void> ChatRoomManager::sendToRoom(int32_t room_id, const chat::Envelope& message) const {
    auto lock = co_await m_manager_mutex.lock_shared();
    sendToRoom_unsafe(room_id, message);
}

void ChatRoomManager::sendToRoom_unsafe(int32_t room_id, const chat::Envelope& message) const {
    if(auto it = m_room_to_conns.find(room_id); it != m_room_to_conns.end()) {
        const auto& connections_in_room = it->second;
        for (const auto& conn : connections_in_room) {
            common::sendEnvelope(conn, message);
        }
    }
}

drogon::Task<void> ChatRoomManager::sendToAll(const chat::Envelope& message) const {
    auto lock = co_await m_manager_mutex.lock_shared();
    sendToAll_unsafe(message);
}

void ChatRoomManager::sendToAll_unsafe(const chat::Envelope& message) const {
    for (const auto& [user_id, conns] : m_user_id_to_conns) {
        for (const auto& conn : conns) {
            common::sendEnvelope(conn, message);
        }
    }
}

} // namespace server
