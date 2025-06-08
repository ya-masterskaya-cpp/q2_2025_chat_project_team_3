#include <server/chat/DrogonRoomService.h>
#include <server/chat/ChatRoomManager.h>

DrogonRoomService::DrogonRoomService(const drogon::WebSocketConnectionPtr& conn)
    : m_conn(conn) {
}

void DrogonRoomService::joinRoom() {
    ChatRoomManager::instance().addConnectionToRoom(m_conn);
}

void DrogonRoomService::leaveCurrentRoom() {
    ChatRoomManager::instance().unregisterConnection(m_conn);
}
