#pragma once

#include <server/chat/IChatRoomService.h>
#include <drogon/WebSocketConnection.h>

/**
 * @file DrogonRoomService.h
 * @brief Defines the concrete implementation of the IChatRoomService for an in-memory state manager.
 */

namespace server {

/**
 * @class DrogonRoomService
 * @brief A concrete implementation of IChatRoomService.
 *
 * @details This class implements the `IChatRoomService` interface by acting as an
 * adapter or bridge to the `ChatRoomManager` singleton. It is instantiated on a
 * per-request basis and holds a reference to the specific WebSocket connection
 * that the request originated from.
 *
 * This design allows the core business logic to depend on the `IChatRoomService`
 * abstraction, while this class handles the specific task of communicating with
 * the ChatRoomManager singleton, making the system more modular and testable.
 */
class DrogonRoomService : public IChatRoomService {
public:
    /**
     * @brief Constructs a new room service for a specific connection.
     * @param conn The WebSocket connection that this service instance will manage.
     */
    explicit DrogonRoomService(const drogon::WebSocketConnectionPtr& conn);

    /** @see IChatRoomService::login */
    drogon::Task<void> login(const WsData& locked_data) override;

    /** @see IChatRoomService::logout */
    drogon::Task<void> logout(const WsData& locked_data) override;

    /** @see IChatRoomService::joinRoom */
    drogon::Task<void> joinRoom(const WsData& locked_data) override;

    /** @see IChatRoomService::leaveCurrentRoom */
    drogon::Task<void> leaveCurrentRoom(const WsData& locked_data) override;

    /** @see IChatRoomService::getUsersInRoom */
    drogon::Task<std::vector<chat::UserInfo>> getUsersInRoom(
        int32_t room_id, const WsData& locked_data) const override;

    /** @see IChatRoomService::sendToRoom */
    drogon::Task<void> sendToRoom(int32_t room_id, const chat::Envelope& message) const override;

    /** @see IChatRoomService::sendToAll */
    drogon::Task<void> sendToAll(const chat::Envelope& message) const override;

    /** @see IChatRoomService::onRoomDeleted */
    drogon::Task<void> onRoomDeleted(int32_t room_id) override;

    /** @see IChatRoomService::updateUserRoomRights */
    drogon::Task<void> updateUserRoomRights(int32_t userId, int32_t roomId, chat::UserRights newRights) override;

private:
    /// @brief The specific WebSocket connection this service instance operates on.
    const drogon::WebSocketConnectionPtr& m_conn;
};

} // namespace server
