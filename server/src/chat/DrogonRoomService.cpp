#include <server/chat/DrogonRoomService.h>
#include <server/chat/ChatRoomManager.h>

namespace server {

DrogonRoomService::DrogonRoomService(const drogon::WebSocketConnectionPtr& conn)
    : m_conn(conn) {}

drogon::Task<void> DrogonRoomService::login(const WsData& locked_caller_data) {
    if(locked_caller_data.user) {
        co_await ChatRoomManager::instance().registerConnection(locked_caller_data.user->id, m_conn);
    }
}

drogon::Task<void> DrogonRoomService::logout(const WsData& locked_caller_data) {
    co_await ChatRoomManager::instance().unregisterConnection(m_conn, locked_caller_data);
}

drogon::Task<void> DrogonRoomService::joinRoom(const WsData& locked_caller_data) {
    if(locked_caller_data.room) {
        co_await ChatRoomManager::instance().addConnectionToRoom(locked_caller_data.room->id, m_conn);
    }
}

drogon::Task<void> DrogonRoomService::leaveCurrentRoom(const WsData& locked_caller_data) {
    co_await ChatRoomManager::instance().removeConnectionFromRoom(m_conn, locked_caller_data);
}

} // namespace server
