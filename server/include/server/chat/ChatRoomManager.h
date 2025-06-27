#pragma once

#include <drogon/WebSocketConnection.h>
#include <server/chat/WsData.h>

/**
 * @file ChatRoomManager.h
 * @brief Defines the singleton for managing real-time chat state.
 */

namespace server {

/**
 * @class ChatRoomManager
 * @brief A thread-safe singleton managing the server's in-memory, real-time state.
 *
 * @details This class is the central authority for tracking active WebSocket
 * connections, user-to-connection mappings, and room memberships. It provides
 * asynchronous, coroutine-based methods to modify this state and to broadcast
 * messages to specific rooms or to all connected users.
 *
 * All public methods are asynchronous and thread-safe, returning a `drogon::Task`
 * that must be `co_await`ed. Access to the internal data structures is
 * protected by an asynchronous shared mutex.
 *
 * @note This class is implemented as a singleton, accessible via the `instance()`
 * static method.
 */
class ChatRoomManager {
public:
    /**
     * @brief Gets the singleton instance of the ChatRoomManager.
     * @return A reference to the single ChatRoomManager instance.
     */
    static ChatRoomManager& instance();
    
    /**
     * @brief Registers a new connection for an authenticated user.
     * @param user_id The ID of the authenticated user.
     * @param conn The user's WebSocket connection pointer.
     * @return A drogon::Task<void> to be awaited.
     */
    drogon::Task<void> registerConnection(int32_t user_id, const drogon::WebSocketConnectionPtr& conn);

    /**
     * @brief Unregisters a connection entirely from the manager.
     *
     * @details This is typically called when a WebSocket connection is closed. It
     * ensures the connection is removed from its current room (if any) and from
     * the global user-to-connection map.
     *
     * @param conn The WebSocket connection that is closing.
     * @param locked_data A reference to the connection's WsData, which is assumed
     *        to be already locked by the caller. This provides access to user
     *        and room details without causing a deadlock.
     * @return A drogon::Task<void> to be awaited.
     */
    drogon::Task<void> unregisterConnection(const drogon::WebSocketConnectionPtr& conn, const WsData& locked_data);

    /**
     * @brief Adds an existing connection to a specific chat room.
     * @param room_id The ID of the room to join.
     * @param conn The user's WebSocket connection pointer.
     * @return A drogon::Task<void> to be awaited.
     */
    drogon::Task<void> addConnectionToRoom(int32_t room_id, const drogon::WebSocketConnectionPtr& conn);

    /**
     * @brief Removes a connection from its current chat room.
     *
     * @details This method broadcasts a "user left" notification to the room
     * before removing the connection from the room's membership list.
     *
     * @param conn The WebSocket connection leaving the room.
     * @param locked_data A reference to the connection's WsData, assumed to be
     *        locked by the caller, to access room and user information.
     * @return A drogon::Task<void> to be awaited.
     */
    drogon::Task<void> removeConnectionFromRoom(const drogon::WebSocketConnectionPtr& conn, const WsData& locked_data);

    /**
     * @brief Asynchronously retrieves a list of all users currently in a room.
     * @param room_id The ID of the room to query.
     * @param locked_data A reference to the calling connection's locked
     *        WsData. This is a crucial optimization to prevent deadlocking when
     *        the caller is a member of the room it is querying.
     * @return A drogon::Task resolving to a std::vector of UserInfo objects.
     */
    drogon::Task<std::vector<chat::UserInfo>> getUsersInRoom(
        int32_t room_id, const WsData& locked_data) const;
    
    /**
     * @brief Sends a Protobuf message to all users in a specific room.
     * @param room_id The target room's ID.
     * @param message The Protobuf Envelope to send.
     * @return A drogon::Task<void> to be awaited.
     */
    drogon::Task<void> sendToRoom(int32_t room_id, const chat::Envelope& message) const;

    /**
     * @brief Sends a Protobuf message to all authenticated users on the server.
     * @param message The Protobuf Envelope to send.
     * @return A drogon::Task<void> to be awaited.
     */
    drogon::Task<void> sendToAll(const chat::Envelope& message) const;
    
    /**
     * @brief Handles the server-side cleanup when a room is deleted.
     * @details This function removes the room from the internal state and broadcasts
     * a `RoomDeleted` notification to all connected clients.
     * @param room_id The ID of the room that was deleted.
     * @return A drogon::Task<void> to be awaited.
     */
    drogon::Task<void> onRoomDeleted(int32_t room_id);
    
    /**
     * @brief Updates the in-memory room rights for a specific user.
     * @param userId The ID of the user whose rights are being updated.
     * @param roomId The ID of the room where the rights changed.
     * @param newRights The new rights for the user in that room.
     * @return A drogon::Task<void> to be awaited.
     */
    drogon::Task<void> updateUserRoomRights(int32_t userId, int32_t roomId, chat::UserRights newRights);
    
private:
    ChatRoomManager() = default;
    ChatRoomManager(const ChatRoomManager&) = delete;
    ChatRoomManager& operator=(const ChatRoomManager&) = delete;

    /**
     * @brief Sends a message to a room without acquiring a lock.
     * @note This is an internal helper and assumes the caller holds a lock on `m_manager_mutex`.
     */
    void sendToRoom_unsafe(int32_t room_id, const chat::Envelope& message) const;
    
    /**
     * @brief Sends a message to all users without acquiring a lock.
     * @note This is an internal helper and assumes the caller holds a lock on `m_manager_mutex`.
     */
    void sendToAll_unsafe(const chat::Envelope& message) const;

    /// @brief An asynchronous mutex protecting all internal data structures.
    mutable common::Guarded<int> m_manager_mutex{0};

    /// @brief Maps a user's ID to the set of their active WebSocket connections.
    std::unordered_map<int32_t, std::unordered_set<drogon::WebSocketConnectionPtr>> m_user_id_to_conns;
    
    /// @brief Maps a room's ID to the set of WebSocket connections currently in that room.
    std::unordered_map<int32_t, std::unordered_set<drogon::WebSocketConnectionPtr>> m_room_to_conns;
};

} // namespace server
