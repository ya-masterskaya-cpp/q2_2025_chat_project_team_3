#pragma once

#include <server/chat/WsData.h>

/**
 * @file IChatRoomService.h
 * @brief Defines the abstract interface for all real-time chat state and broadcast operations.
 */

namespace server {

/**
 * @class IChatRoomService
 * @brief An abstract interface for real-time chat state and broadcast operations.
 *
 * @details This class defines the contract for all interactions with the server's
 * real-time, in-memory state. It decouples the core business logic (in
 * `MessageHandlers`) from any specific state management implementation (like an
 * in-memory singleton or a distributed cache like Redis).
 *
 * This abstraction is crucial for:
 * - **Testability**: Allows business logic to be unit-tested with mock implementations.
 * - **Flexibility**: The underlying state manager can be replaced without
 *   changing any business logic code.
 * - **Clarity**: Provides a clear boundary and a single point of interaction for
 *   all real-time state changes and broadcasts.
 */
class IChatRoomService {
public:
    virtual ~IChatRoomService() = default;

    /**
     * @brief Handles the state change when a user logs in.
     * @param locked_data The already-locked WsData of the logging-in user.
     * @return A drogon::Task<void> to be awaited.
     */
    virtual drogon::Task<void> login(const WsData& locked_data) = 0;

    /**
     * @brief Handles the state change when a user logs out or disconnects.
     * @param locked_data The already-locked WsData of the logging-out user.
     * @return A drogon::Task<void> to be awaited.
     */
    virtual drogon::Task<void> logout(const WsData& locked_data) = 0;

    /**
     * @brief Handles the state change when a user joins a room.
     * @param locked_data The user's locked WsData, containing the target room info.
     * @return A drogon::Task<void> to be awaited.
     */
    virtual drogon::Task<void> joinRoom(const WsData& locked_data) = 0;

    /**
     * @brief Handles the state change when a user leaves their current room.
     * @param locked_data The user's locked WsData.
     * @return A drogon::Task<void> to be awaited.
     */
    virtual drogon::Task<void> leaveCurrentRoom(const WsData& locked_data) = 0;

    /**
     * @brief Asynchronously retrieves a list of all users currently in a room.
     * @param room_id The ID of the room to query.
     * @param locked_data A reference to the calling connection's locked
     *        WsData. This is a crucial optimization to prevent deadlocking when
     *        the caller is a member of the room it is querying.
     * @return A drogon::Task resolving to a std::vector of UserInfo objects.
     */
    virtual drogon::Task<std::vector<chat::UserInfo>> getUsersInRoom(
        int32_t room_id, const WsData& locked_data) const = 0;
    
    /*
     * @brief Sends a Protobuf message to all users in a specific room.
     * @param room_id The target room's ID.
     * @param message The Protobuf Envelope to send.
     * @return A drogon::Task<void> to be awaited.
     */
    virtual drogon::Task<void> sendToRoom(int32_t room_id, const chat::Envelope& message) const = 0;

    /**
     * @brief Sends a Protobuf message to all authenticated users on the server.
     * @param message The Protobuf Envelope to send.
     * @return A drogon::Task<void> to be awaited.
     */
    virtual drogon::Task<void> sendToAll(const chat::Envelope& message) const = 0;
    
    /**
     * @brief Handles the server-side state cleanup when a room is deleted.
     * @param room_id The ID of the room that was deleted.
     * @return A drogon::Task<void> to be awaited.
     */
    virtual drogon::Task<void> onRoomDeleted(int32_t room_id) = 0;
};

} // namespace server
