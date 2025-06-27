#pragma once

#include <common/utils/guarded.h>

/**
 * @file WsData.h
 * @brief Defines the stateful data structures associated with a WebSocket connection.
 */

namespace server {

/**
 * @enum USER_STATUS
 * @brief Defines the authentication and connection state of a user's WebSocket session.
 */
enum class USER_STATUS {
    /// The connection has just been established and has not yet attempted to authenticate or register.
    Unauthenticated,
    /// The user has sent an `InitialRegisterRequest` and the server is waiting for the final `RegisterRequest`.
    Registering,
    /// The user has sent an `InitialAuthRequest` and the server is waiting for the final `AuthRequest`.
    Authenticating,
    /// The user has successfully completed the authentication process.
    Authenticated
};

/**
 * @struct User
 * @brief A simple data structure holding the essential information of an authenticated user.
 */
struct User {
    /// The unique identifier for the user, corresponding to the primary key in the `users` database table.
    int32_t id;
    /// The user's chosen display name.
    std::string name;
};

/**
 * @struct CurrentRoom
 * @brief A data structure holding information about the room a user is currently active in.
 */
struct CurrentRoom {
    /// The unique identifier for the room, corresponding to the primary key in the `rooms` database table.
    int32_t id;
    /// The user's permission level (e.g., REGULAR, OWNER) within this specific room.
    chat::UserRights rights;
};

/**
 * @struct WsData
 * @brief A container for all state information associated with a single WebSocket connection.
 *
 * @details This structure is the primary state object for a client session. An
 * instance of this struct (wrapped in a `common::Guarded` object for thread
 * safety) is attached to each `drogon::WebSocketConnection`'s context. It tracks
 * everything from the user's authentication status to their current room membership.
 */
struct WsData {
    /// @brief Holds the authenticated user's information. Contains a value only if `status` is `Authenticated`.
    std::optional<User> user;
    /// @brief Holds details about the room the user has joined. Contains a value only if the user is in a room.
    std::optional<CurrentRoom> room;
    /// @brief The current authentication state of the connection.
    USER_STATUS status = USER_STATUS::Unauthenticated;
};

/// @brief A type alias for `WsData` protected by a `common::Guarded` wrapper for thread-safe access.
using WsDataGuarded = common::Guarded<WsData>;
/// @brief A type alias for a shared pointer to a `WsDataGuarded` object, as stored in the connection context.
using WsDataPtr = std::shared_ptr<WsDataGuarded>;

/// @brief A type alias for the read-only proxy returned by `lock_shared()` on a `WsDataGuarded` object.
using WsDataSharedProxy = WsDataGuarded::SharedProxy;
/// @brief A type alias for the read-write proxy returned by `lock_unique()` on a `WsDataGuarded` object.
using WsDataUniqueProxy = WsDataGuarded::UniqueProxy;

} // namespace server
