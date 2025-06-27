#include <server/chat/DrogonRoomService.h>
#include <server/chat/ChatRoomManager.h>

namespace server {

DrogonRoomService::DrogonRoomService(const drogon::WebSocketConnectionPtr& conn)
    : m_conn(conn) {}

drogon::Task<void> DrogonRoomService::login(const WsData& locked_data) {
    if(locked_data.user) {
        co_await ChatRoomManager::instance().registerConnection(locked_data.user->id, m_conn);
    }
}

drogon::Task<void> DrogonRoomService::logout(const WsData& locked_data) {
    co_await ChatRoomManager::instance().unregisterConnection(m_conn, locked_data);
}

drogon::Task<void> DrogonRoomService::joinRoom(const WsData& locked_data) {
    if(locked_data.room) {
        co_await ChatRoomManager::instance().addConnectionToRoom(locked_data.room->id, m_conn);
    }
}

drogon::Task<void> DrogonRoomService::leaveCurrentRoom(const WsData& locked_data) {
    co_await ChatRoomManager::instance().removeConnectionFromRoom(m_conn, locked_data);
}

drogon::Task<std::vector<chat::UserInfo>> DrogonRoomService::getUsersInRoom(
        int32_t room_id, const WsData& locked_data) const {
    co_return co_await ChatRoomManager::instance().getUsersInRoom(room_id, locked_data);
}

drogon::Task<void> DrogonRoomService::sendToRoom(int32_t room_id, const chat::Envelope& message) const {
    co_await ChatRoomManager::instance().sendToRoom(room_id, message);
}

drogon::Task<void> DrogonRoomService::sendToAll(const chat::Envelope& message) const {
    co_await ChatRoomManager::instance().sendToAll(message);
}

drogon::Task<void> DrogonRoomService::onRoomDeleted(int32_t room_id) {
    co_await ChatRoomManager::instance().onRoomDeleted(room_id);
}

} // namespace server
