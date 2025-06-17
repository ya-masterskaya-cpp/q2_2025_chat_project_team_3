#include <server/chat/ChatRoomManager.h>
#include <common/utils/utils.h>
#include <server/chat/WsData.h>

ChatRoomManager& ChatRoomManager::instance() {
    static ChatRoomManager inst;
    return inst;
}

std::vector<chat::UserInfo> ChatRoomManager::getUsersInRoom(int32_t room_id) const {
    std::vector<chat::UserInfo> res;
    {
        std::shared_lock lock(m_mutex);

        auto it = m_room_to_conns.find(room_id);

        if(it != m_room_to_conns.end()) {
            res.reserve(it->second.size());
            for(const auto& conn : it->second) {
                auto& ws_data = conn->getContextRef<WsData>();
                chat::UserInfo ui;
                ui.set_user_id(ws_data.user->id);
                ui.set_user_name(ws_data.user->name);
                ui.set_user_room_rights(ws_data.room->rights);
                res.emplace_back(std::move(ui));
            }
        }
    }
    return res;
}

void ChatRoomManager::registerConnection(const drogon::WebSocketConnectionPtr& conn) {
    std::unique_lock lock(m_mutex);
    auto& ws_data = conn->getContextRef<WsData>();
    int32_t user_id = ws_data.user->id;
    m_user_id_to_conns[user_id].insert(conn);
}

void ChatRoomManager::addConnectionToRoom(const drogon::WebSocketConnectionPtr& conn) {
    std::unique_lock lock(m_mutex);
    auto& ws_data = conn->getContextRef<WsData>();
    int32_t room_id = ws_data.room->id;
    m_room_to_conns[room_id].insert(conn);
}

void ChatRoomManager::removeConnectionFromRoom(const drogon::WebSocketConnectionPtr& conn) {
    std::unique_lock lock(m_mutex);
    removeFromRoom_unsafe(conn);
}

void ChatRoomManager::unregisterConnection(const drogon::WebSocketConnectionPtr& conn) {
    std::unique_lock lock(m_mutex);

    auto& ws_data = conn->getContextRef<WsData>();

    // Clean up user-related mapping if user data is present
    if (ws_data.user) {
        int32_t user_id = ws_data.user->id;
        // The user must be in the map if ws_data.user is present
        m_user_id_to_conns[user_id].erase(conn);
        if (m_user_id_to_conns[user_id].empty()) {
            m_user_id_to_conns.erase(user_id);
        }
    }

    // Clean up room-related mapping if room data is present
    if (ws_data.room) {
        removeFromRoom_unsafe(conn);
    }
}

void ChatRoomManager::sendToRoom(int32_t room_id, const chat::Envelope& message) const {
    std::shared_lock lock(m_mutex);
    sendToRoom_unsafe(room_id, message);
}

void ChatRoomManager::sendToRoom_unsafe(int32_t room_id, const chat::Envelope& message) const {
    if(auto it = m_room_to_conns.find(room_id); it != m_room_to_conns.end()) {
        const auto& connections_in_room = it->second;
        for (const auto& conn : connections_in_room) {
            sendEnvelope(conn, message);
        }
    }
}

void ChatRoomManager::removeFromRoom_unsafe(const drogon::WebSocketConnectionPtr& conn) {
    auto& ws_data = conn->getContextRef<WsData>();
    int32_t room_id = ws_data.room->id;

    chat::Envelope user_left_msg;
    user_left_msg.mutable_user_left()->mutable_user()->set_user_id(ws_data.user->id);
    user_left_msg.mutable_user_left()->mutable_user()->set_user_name(ws_data.user->name);
    user_left_msg.mutable_user_left()->mutable_user()->set_user_room_rights(ws_data.room->rights);
    ChatRoomManager::instance().sendToRoom_unsafe(ws_data.room->id, user_left_msg);

    m_room_to_conns[room_id].erase(conn);
    if (m_room_to_conns[room_id].empty()) {
        m_room_to_conns.erase(room_id);
    }
}

void ChatRoomManager::sendToAll(const chat::Envelope& message) const {
    std::shared_lock lock(m_mutex);
    for (const auto& [user_id, conns] : m_user_id_to_conns) {
        for (const auto& conn : conns) {
            if (conn->getContextRef<WsData>().status == USER_STATUS::Authenticated) {
                sendEnvelope(conn, message);
            }
        }
    }
}
