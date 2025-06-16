#include <server/chat/DrogonRoomService.h>
#include <server/chat/ChatRoomManager.h>

DrogonRoomService::DrogonRoomService(const drogon::WebSocketConnectionPtr& conn)
    : m_conn(conn) {
}

void DrogonRoomService::joinRoom() {
    ChatRoomManager::instance().addConnectionToRoom(m_conn);
}

void DrogonRoomService::leaveCurrentRoom() {
    ChatRoomManager::instance().removeConnectionFromRoom(m_conn);
}

void DrogonRoomService::logout() {
    ChatRoomManager::instance().unregisterConnection(m_conn);
}

void DrogonRoomService::login() {
    ChatRoomManager::instance().registerConnection(m_conn);
}
