#pragma once

#include <drogon/orm/DbClient.h>
#include <server/chat/WsData.h>
#include <server/utils/scoped_coro_transaction.h>

/**
 * @file MessageHandlers.h
 * @brief Defines the class containing all core business logic for chat operations.
 */

// Forward declaration for Drogon model to avoid including the full model header.
namespace drogon_model {
namespace drogon_test {
    class Rooms;
}
}

namespace server {

class IChatRoomService;

/**
 * @class MessageHandlers
 * @brief Contains the implementations of all core business logic for chat operations.
 *
 * @details This class is the heart of the server's application logic. Each public
 * method corresponds to a specific client request (e.g., `handleAuth`,
 * `handleSendMessage`). These methods are responsible for:
 *
 * - Validating incoming data (e.g., checking for empty fields, valid UTF-8).
 * - Interacting with the database via the Drogon ORM for data persistence.
 * - Calling the `IChatRoomService` to perform real-time actions like
 *   broadcasting messages or updating room membership.
 * - Modifying the connection's state (`WsData`).
 * - Constructing and returning the appropriate response message.
 *
 * All handlers are asynchronous and return a `drogon::Task`, designed to be
 * called from a coroutine context.
 */
class MessageHandlers {
public:
    /**
     * @brief Constructs the handlers with a database client.
     * @param dbClient A shared pointer to the Drogon database client, used for all ORM operations.
     */
    explicit MessageHandlers(drogon::orm::DbClientPtr dbClient);

    /** @brief Handles the first step of user authentication (salt retrieval). */
    drogon::Task<chat::InitialAuthResponse> handleAuthInitial(const WsDataPtr& wsDataGuarded, const chat::InitialAuthRequest& req) const;
    
    /** @brief Handles the first step of user registration (username validation). */
    drogon::Task<chat::InitialRegisterResponse> handleRegisterInitial(const WsDataPtr& wsDataGuarded, const chat::InitialRegisterRequest& req) const;
    
    /** @brief Handles the final step of user authentication (hash verification). */
    drogon::Task<chat::AuthResponse> handleAuth(const WsDataPtr& wsDataGuarded, const chat::AuthRequest& req, IChatRoomService& room_service) const;
    
    /** @brief Handles the final step of user registration (storing user credentials). */
    drogon::Task<chat::RegisterResponse> handleRegister(const WsDataPtr& wsDataGuarded, const chat::RegisterRequest& req) const;
    
    /** @brief Handles a request to send a message to the user's current room. */
    drogon::Task<chat::SendMessageResponse> handleSendMessage(const WsDataPtr& wsDataGuarded, const chat::SendMessageRequest& req, IChatRoomService& room_service) const;
    
    /** @brief Handles a request for a user to join a chat room. */
    drogon::Task<chat::JoinRoomResponse> handleJoinRoom(const WsDataPtr& wsDataGuarded, const chat::JoinRoomRequest& req, IChatRoomService& room_service) const;
    
    /** @brief Handles a request for a user to leave their current chat room. */
    drogon::Task<chat::LeaveRoomResponse> handleLeaveRoom(const WsDataPtr& wsDataGuarded, const chat::LeaveRoomRequest&, IChatRoomService& room_service) const;
    
    /** @brief Handles a request from a user to create a new chat room. */
    drogon::Task<chat::CreateRoomResponse> handleCreateRoom(const WsDataPtr& wsDataGuarded, const chat::CreateRoomRequest& req, IChatRoomService& room_service) const;
    
    /** @brief Handles a request to retrieve a batch of historical messages from the user's current room. */
    drogon::Task<chat::GetMessagesResponse> handleGetMessages(const WsDataPtr& wsDataGuarded, const chat::GetMessagesRequest& req) const;
    
    /** @brief Handles a user's request to log out. */
    drogon::Task<chat::LogoutResponse> handleLogoutUser(const WsDataPtr& wsDataGuarded, IChatRoomService& room_service) const;

    /** @brief Handles a request to rename an existing room. */
    drogon::Task<chat::RenameRoomResponse> handleRenameRoom(const WsDataPtr& wsDataGuarded, const chat::RenameRoomRequest& req, IChatRoomService& room_service);

    /** @brief Handles a request to delete an existing room and all its messages. */
    drogon::Task<chat::DeleteRoomResponse> handleDeleteRoom(const WsDataPtr& wsDataGuarded, const chat::DeleteRoomRequest& req, IChatRoomService& room_service);
    
    /** @brief Handles a request to assign a new role to a user in a specific room. */
    drogon::Task<chat::AssignRoleResponse> handleAssignRole(const WsDataPtr&, const chat::AssignRoleRequest&, IChatRoomService&);

    /** @brief Handles a request to delete message from current room. */
    drogon::Task<chat::DeleteMessageResponse> handleDeleteMessage(const WsDataPtr&, const chat::DeleteMessageRequest&, IChatRoomService&);

	/** @brief Handles a request to start typing in the current room. */
	drogon::Task<chat::UserTypingStartResponse> handleUserTypingStart(const WsDataPtr& wsDataGuarded, IChatRoomService& room_service) const;

	/** @brief Handles a request to stop typing in the current room. */
	drogon::Task<chat::UserTypingStopResponse> handleUserTypingStop(const WsDataPtr& wsDataGuarded, IChatRoomService& room_service) const;
private:
    /**
     * @brief Validates a string for valid UTF-8 encoding and maximum length.
     * @param textToValidate The string to check.
     * @param maxLength The maximum allowed number of UTF-8 characters.
     * @param fieldName The name of the field being validated (for error messages).
     * @return An optional string containing an error message if validation fails, or std::nullopt on success.
     */
    std::optional<std::string> validateUtf8String(const std::string_view& textToValidate, size_t maxLength, const std::string_view& fieldName) const;

    /**
     * @brief Determines a user's rights (e.g., ADMIN, OWNER) for a specific room.
     * @param db DbClientPtr
     * @param user_id The ID of the user.
     * @param room_id The ID of the room.
     * @return A task resolving to an optional UserRights enum. Returns nullopt if the user has no specific role.
     */
    drogon::Task<std::optional<chat::UserRights>> getUserRights(const drogon::orm::DbClientPtr& db, int32_t user_id, int32_t room_id) const;

    /**
     * @brief Overload of getUserRights that accepts a pre-fetched room object to avoid an extra DB query.
     * @param db DbClientPtr
     * @param user_id The ID of the user.
     * @param room_id The ID of the room.
     * @param room A pre-fetched room model object.
     * @return A task resolving to an optional UserRights enum.
     */
    drogon::Task<std::optional<chat::UserRights>> getUserRights(const drogon::orm::DbClientPtr& db, int32_t user_id, int32_t room_id, const drogon_model::drogon_test::Rooms& room) const;

    /**
     * @brief Helper to query the `user_room_roles` table for an explicit role assignment.
     * @param db DbClientPtr
     * @param user_id The ID of the user.
     * @param room_id The ID of the room.
     * @return A task resolving to an optional UserRights enum if a role is found.
     */
    drogon::Task<std::optional<chat::UserRights>> findStoredUserRole(const drogon::orm::DbClientPtr& db, int32_t user_id, int32_t room_id) const;

    /** @brief Updates or creates a user's role entry in the UserRoomRoles table within a transaction. */
    drogon::Task<ScopedTransactionResult> updateUserRoleInDb(const std::shared_ptr<drogon::orm::Transaction>& tx, int32_t userId, int32_t roomId, chat::UserRights newRole);

    /// @brief The shared database client for all ORM operations.
    drogon::orm::DbClientPtr m_dbClient;
};

} // namespace server
