#include <server/chat/ChatRoomManager.h>
#include <server/utils/utils.h>
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

void ChatRoomManager::addConnectionToRoom(const drogon::WebSocketConnectionPtr& conn) {
    auto& ws_data = conn->getContextRef<WsData>();
    int32_t user_id = ws_data.user->id;
    int32_t room_id = ws_data.room->id;

    std::unique_lock lock(m_mutex);

    m_user_id_to_conns[user_id].insert(conn);
    m_room_to_conns[room_id].insert(conn);
}

void ChatRoomManager::unregisterConnection(const drogon::WebSocketConnectionPtr& conn) {
    auto& ws_data = conn->getContextRef<WsData>();

    std::unique_lock lock(m_mutex);

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
        int32_t room_id = ws_data.room->id;
        // The connection must be in the room's set if ws_data.room is present
        m_room_to_conns[room_id].erase(conn);
        if (m_room_to_conns[room_id].empty()) {
            m_room_to_conns.erase(room_id);
        }
    }
}

void ChatRoomManager::sendToRoom(int32_t room_id, const chat::Envelope& message) const {
    std::shared_lock lock(m_mutex);

    if(auto it = m_room_to_conns.find(room_id); it != m_room_to_conns.end()) {
        const auto& connections_in_room = it->second;
        for (const auto& conn : connections_in_room) {
            sendEnvelope(conn, message);
        }
    }
}
