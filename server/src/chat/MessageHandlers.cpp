#include <server/chat/MessageHandlers.h>
#include <server/chat/WsData.h>
#include <server/chat/IChatRoomService.h>

#include <server/models/Users.h>
#include <server/models/Rooms.h>
#include <server/models/Messages.h>
#include <server/models/UserRoomRoles.h>

#include <server/utils/switch_to_io_loop.h>
#include <common/utils/utils.h>
#include <common/utils/limits.h>

#include <utf8.h>

using namespace drogon::orm;
namespace models = drogon_model::drogon_test;

namespace server {

MessageHandlers::MessageHandlers(DbClientPtr dbClient)
    : m_dbClient{std::move(dbClient)} {}

drogon::Task<chat::InitialAuthResponse> MessageHandlers::handleAuthInitial(const WsDataPtr& wsDataGuarded, const chat::InitialAuthRequest& req) const {
    chat::InitialAuthResponse resp;

    auto wsData = co_await wsDataGuarded->lock_unique();

    if(req.username().empty()) {
        common::setStatus(resp, chat::STATUS_FAILURE, "Empty username.");
        co_return resp;
    }
    if (auto error = validateUtf8String(req.username(), common::limits::MAX_USERNAME_LENGTH, "username")) {
        common::setStatus(resp, chat::STATUS_FAILURE, *error);
        co_return resp;
    }
    try {
        auto users = co_await switch_to_io_loop(CoroMapper<models::Users>(m_dbClient)
            .findBy(Criteria(models::Users::Cols::_username, CompareOperator::EQ, req.username())));
        if(users.empty()) {
            common::setStatus(resp, chat::STATUS_UNAUTHORIZED, "Invalid credentials.");
            co_return resp;
        }
        wsData->status = USER_STATUS::Authenticating;
        wsData->user = User{.id = 0, .name = req.username()};
        if (!users.front().getValueOfSalt().empty()) resp.set_salt(users.front().getValueOfSalt());

        common::setStatus(resp, chat::STATUS_SUCCESS);
        co_return resp;
    } catch(const std::exception& e) {
        common::setStatus(resp, chat::STATUS_FAILURE, std::string("Auth failed: ") + e.what());
        co_return resp;
    }
}

drogon::Task<chat::InitialRegisterResponse> MessageHandlers::handleRegisterInitial(const WsDataPtr& wsDataGuarded, const chat::InitialRegisterRequest& req) const {
    chat::InitialRegisterResponse resp;

    auto wsData = co_await wsDataGuarded->lock_unique();

    if(req.username().empty()) {
        common::setStatus(resp, chat::STATUS_FAILURE, "Empty username or password.");
        co_return resp;
    }
    if (auto error = validateUtf8String(req.username(), common::limits::MAX_USERNAME_LENGTH, "username")) {
        common::setStatus(resp, chat::STATUS_FAILURE, *error);
        co_return resp;
    }
    try {
        auto users = co_await switch_to_io_loop(CoroMapper<models::Users>(m_dbClient)
            .findBy(Criteria(models::Users::Cols::_username, CompareOperator::EQ, req.username())));
        if(!users.empty()) {
            common::setStatus(resp, chat::STATUS_FAILURE, "Username already exists.");
            co_return resp;
        }
        wsData->status = USER_STATUS::Registering;
        wsData->user = User{.id = 0, .name = req.username() };
        common::setStatus(resp, chat::STATUS_SUCCESS);
        co_return resp;
    } catch(const std::exception& e) {
        common::setStatus(resp, chat::STATUS_FAILURE, std::string("Registration failed: ") + e.what());
        co_return resp;
    }
}

drogon::Task<chat::AuthResponse> MessageHandlers::handleAuth(const WsDataPtr& wsDataGuarded, const chat::AuthRequest& req, IChatRoomService& room_service) const {
    chat::AuthResponse resp;

    auto wsData = co_await wsDataGuarded->lock_unique();

    if (wsData->status != USER_STATUS::Authenticating) {
        common::setStatus(resp, chat::STATUS_FAILURE, "Not in authentication phase.");
        co_return resp;
    }

    if (req.hash().empty()) {
        common::setStatus(resp, chat::STATUS_FAILURE, "Missing hash.");
        co_return resp;
    }

    try {
        auto users = co_await switch_to_io_loop(CoroMapper<models::Users>(m_dbClient)
            .findBy(Criteria(models::Users::Cols::_username, CompareOperator::EQ, wsData->user->name)));

        if (users.empty()) {
            wsData->status = USER_STATUS::Unauthenticated;
            common::setStatus(resp, chat::STATUS_UNAUTHORIZED, "User not found.");
            co_return resp;
        }

        auto user = users.front();
        auto pas = req.password();
        auto salt = req.salt();
        if (req.has_password() && req.has_salt()) {
            if (user.getValueOfSalt().empty()) {
                if (user.getValueOfHashPassword() != req.password()) {
                    common::setStatus(resp, chat::STATUS_UNAUTHORIZED, "Incorrect password.");
                    co_return resp;
                }
                auto err = co_await WithTransaction(
                    [&](auto tx) -> drogon::Task<std::optional<std::string>> {
                        try {
                            user.setHashPassword(req.hash());
                            user.setSalt(req.salt());
                            co_await switch_to_io_loop(CoroMapper<models::Users>(tx).update(user));
                            co_return std::nullopt;
                        } catch (const DrogonDbException& e) {
                            LOG_ERROR << "User update error: " << e.base().what();
                            co_return "Database error during user update.";
                        }
                    });

                if (err) {
                    wsData->status = USER_STATUS::Unauthenticated;
                    common::setStatus(resp, chat::STATUS_FAILURE, *err);
                    co_return resp;
                }
            } else {
                common::setStatus(resp, chat::STATUS_FAILURE, "User already migrated. Non correct Auth");
                co_return resp;
            }
        } else {
            if (user.getValueOfSalt().empty()) {
                common::setStatus(resp, chat::STATUS_FAILURE, "Migration required.");
                co_return resp;
            }

            if (user.getValueOfHashPassword() != req.hash()) {
                common::setStatus(resp, chat::STATUS_UNAUTHORIZED, "Hash mismatch.");
                co_return resp;
            }
        }
        auto rooms = co_await switch_to_io_loop(CoroMapper<models::Rooms>(m_dbClient).findAll());
        for(const auto& room : rooms) {
            chat::RoomInfo* room_info = resp.add_rooms();
            room_info->set_room_id(room.getValueOfRoomId());
            room_info->set_room_name(room.getValueOfRoomName());
        }
        chat::UserInfo* user_info = resp.mutable_authenticated_user();
        user_info->set_user_id(*user.getUserId());
        user_info->set_user_name(*user.getUsername());
        wsData->user->id = *user.getUserId();
        wsData->status = USER_STATUS::Authenticated;
        co_await room_service.login(*wsData);
        common::setStatus(resp, chat::STATUS_SUCCESS);
        co_return resp;
    } catch (const std::exception& e) {
        common::setStatus(resp, chat::STATUS_FAILURE, std::string("Auth failed: ") + e.what());
        co_return resp;
    }
}

drogon::Task<chat::RegisterResponse> MessageHandlers::handleRegister(const WsDataPtr& wsDataGuarded, const chat::RegisterRequest& req) const {
    chat::RegisterResponse resp;

    auto wsData = co_await wsDataGuarded->lock_unique();

    if (wsData->status != USER_STATUS::Registering) {
        common::setStatus(resp, chat::STATUS_FAILURE, "Non-correct registering");
        co_return resp;
    }
    if(req.salt().empty() || req.hash().empty()) {
        common::setStatus(resp, chat::STATUS_FAILURE, "Empty hash or salt.");
        co_return resp;
    }
    try {
        // Use a transaction to guarantee atomicity and handle duplicate usernames gracefully
        auto err = co_await WithTransaction(
            [&](auto tx) -> drogon::Task<ScopedTransactionResult> {
                try {
                    models::Users u;
                    u.setUsername(wsData->user->name);
                    u.setHashPassword(req.hash());
                    u.setSalt(req.salt());
                    co_await switch_to_io_loop(CoroMapper<models::Users>(tx).insert(u));
                    co_return std::nullopt;
                } catch(const DrogonDbException& e) {
                    const std::string w = e.base().what();
                    LOG_ERROR << "User insert error: " << w;
                    if(w.find("duplicate key") != std::string::npos || w.find("UNIQUE constraint failed") != std::string::npos) {
                        co_return "Username already exists.";
                    }
                    co_return "Database error during user insertion.";
                }
            });

        if(err) {
            wsData->status = USER_STATUS::Unauthenticated;
            common::setStatus(resp, chat::STATUS_FAILURE, *err);
            co_return resp;
        }
        wsData->status = USER_STATUS::Unauthenticated;
        common::setStatus(resp, chat::STATUS_SUCCESS);
        co_return resp;
    } catch(const std::exception& e) {
        LOG_ERROR << "Register error: " << e.what();
        wsData->status = USER_STATUS::Unauthenticated;
        common::setStatus(resp, chat::STATUS_FAILURE, std::string("Registration failed: ") + e.what());
        co_return resp;
    }
}

drogon::Task<chat::SendMessageResponse> MessageHandlers::handleSendMessage(const WsDataPtr& wsDataGuarded, const chat::SendMessageRequest& req, IChatRoomService& room_service) const {
    chat::SendMessageResponse resp;

    auto wsData = co_await wsDataGuarded->lock_shared();

    if(wsData->status != USER_STATUS::Authenticated) {
        common::setStatus(resp, chat::STATUS_UNAUTHORIZED, "User not authenticated.");
        co_return resp;
    }
    if(!wsData->room) {
        common::setStatus(resp, chat::STATUS_FAILURE, "User is not in any room.");
        co_return resp;
    }
    if(req.message().empty()) {
        common::setStatus(resp, chat::STATUS_FAILURE, "Empty 'message' field.");
        co_return resp;
    }
    if (auto error = validateUtf8String(req.message(), common::limits::MAX_MESSAGE_LENGTH, "message")) {
        common::setStatus(resp, chat::STATUS_FAILURE, *error);
        co_return resp;
    }

    models::Messages inserted_message;

    try {
        auto err = co_await WithTransaction(
            [&](auto tx) -> drogon::Task<ScopedTransactionResult> {
                try {
                    models::Messages m;
                    m.setMessageText(req.message());
                    m.setRoomId(wsData->room->id);
                    m.setUserId(wsData->user->id);
                    inserted_message = co_await switch_to_io_loop(CoroMapper<models::Messages>(tx).insert(m));
                    co_return std::nullopt;
                } catch(const DrogonDbException& e) {
                    const std::string w = e.base().what();
                    LOG_ERROR << "Message insert error: " << w;
                    co_return "Database error during message insertion.";
                }
            });

        if(err) {
            common::setStatus(resp, chat::STATUS_FAILURE, *err);
            co_return resp;
        }
    } catch(const std::exception& e) {
        LOG_ERROR << "Inser message error: " << e.what();
        common::setStatus(resp, chat::STATUS_FAILURE, std::string("Insert message failed: ") + e.what());
        co_return resp;
    }

    // Build and broadcast RoomMessage
    chat::Envelope msgEnv;
    auto* msg = msgEnv.mutable_room_message();
    auto* message_info = msg->mutable_message();
    auto* user_info = message_info->mutable_from();

    message_info->set_message(req.message());
    message_info->set_timestamp(inserted_message.getValueOfCreatedAt());

    user_info->set_user_id(wsData->user->id);
    user_info->set_user_name(wsData->user->name);

    co_await room_service.sendToRoom(wsData->room->id, msgEnv);

    common::setStatus(resp, chat::STATUS_SUCCESS);
    co_return resp;
}

drogon::Task<chat::JoinRoomResponse> MessageHandlers::handleJoinRoom(const WsDataPtr& wsDataGuarded, const chat::JoinRoomRequest& req, IChatRoomService& room_service) const {
    chat::JoinRoomResponse resp;

    auto wsData = co_await wsDataGuarded->lock_unique();

    if(wsData->status != USER_STATUS::Authenticated) {
        common::setStatus(resp, chat::STATUS_UNAUTHORIZED, "User not authenticated.");
        co_return resp;
    }
    try {
        
        auto rooms = co_await switch_to_io_loop(CoroMapper<models::Rooms>(m_dbClient)
            .findBy(Criteria(models::Rooms::Cols::_room_id, CompareOperator::EQ, req.room_id())));
        if(rooms.empty()) {
            common::setStatus(resp, chat::STATUS_NOT_FOUND, "Room does not exist.");
            co_return resp;
        }

        std::optional<chat::UserRights> role = co_await getUserRights(wsData->user->id, req.room_id(), rooms.front());
        if (!role.has_value()) {
            auto err = co_await WithTransaction(
                [&](auto tx) -> drogon::Task<ScopedTransactionResult> {
                try {
                    models::UserRoomRoles role;
                    role.setUserId(wsData->user->id);
                    role.setRoomId(req.room_id());
                    role.setRoleType(chat::UserRights_Name(chat::UserRights::REGULAR));
                    co_await switch_to_io_loop(CoroMapper<models::UserRoomRoles>(tx).insert(role));
                    co_return std::nullopt;
                }
                catch (const DrogonDbException& e) {
                    const std::string w = e.base().what();
                    LOG_ERROR << "Failed to join room: " << w;
                    co_return "Database error during user_room_rules creation.";
                }
            });
            if (err) {
                common::setStatus(resp, chat::STATUS_FAILURE, *err);
                co_return resp;
            }
        }
        wsData->room = CurrentRoom{req.room_id(), role.value_or(chat::UserRights::REGULAR)};
        
        co_await room_service.joinRoom(*wsData);
        auto users = co_await room_service.getUsersInRoom(req.room_id(), *wsData);
        *resp.mutable_users() = {std::make_move_iterator(users.begin()), 
                                  std::make_move_iterator(users.end())};

        chat::Envelope user_joined_msg;
        user_joined_msg.mutable_user_joined()->mutable_user()->set_user_id(wsData->user->id);
        user_joined_msg.mutable_user_joined()->mutable_user()->set_user_name(wsData->user->name);
        user_joined_msg.mutable_user_joined()->mutable_user()->set_user_room_rights(wsData->room->rights);
        co_await room_service.sendToRoom(wsData->room->id, user_joined_msg);

        common::setStatus(resp, chat::STATUS_SUCCESS);
        co_return resp;
    } catch(const std::exception& e) {
        common::setStatus(resp, chat::STATUS_FAILURE, "Failed to join room: " + std::string(e.what()));
        co_return resp;
    }
}

drogon::Task<chat::LeaveRoomResponse> MessageHandlers::handleLeaveRoom(const WsDataPtr& wsDataGuarded, const chat::LeaveRoomRequest&, IChatRoomService& room_service) const {
    chat::LeaveRoomResponse resp;

    auto wsData = co_await wsDataGuarded->lock_unique();

    if(wsData->status != USER_STATUS::Authenticated) {
        common::setStatus(resp, chat::STATUS_UNAUTHORIZED, "User not authenticated.");
        co_return resp;
    }
    if(!wsData->room) {
        common::setStatus(resp, chat::STATUS_FAILURE, "User is not in any room.");
        co_return resp;
    }

    co_await room_service.leaveCurrentRoom(*wsData);
    wsData->room.reset();

    common::setStatus(resp, chat::STATUS_SUCCESS);
    co_return resp;
}

drogon::Task<chat::CreateRoomResponse> MessageHandlers::handleCreateRoom(const WsDataPtr& wsDataGuarded, const chat::CreateRoomRequest& req, IChatRoomService& room_service) const {
    chat::CreateRoomResponse resp;

    auto wsData = co_await wsDataGuarded->lock_shared();

    if(wsData->status != USER_STATUS::Authenticated) {
        common::setStatus(resp, chat::STATUS_UNAUTHORIZED, "Not authenticated");
        co_return resp;
    }
    if(req.room_name().empty()) {
        common::setStatus(resp, chat::STATUS_FAILURE, "Empty room name.");
        co_return resp;
    }
    if (auto error = validateUtf8String(req.room_name(), common::limits::MAX_ROOMNAME_LENGTH, "room name")) {
        common::setStatus(resp, chat::STATUS_FAILURE, *error);
        co_return resp;
    }
    uint32_t room_id;
    try {
        auto err = co_await WithTransaction(
            [&](auto tx) -> drogon::Task<ScopedTransactionResult> {
                try {
                    models::Rooms r;
                    r.setRoomName(req.room_name());
                    r.setOwnerId(wsData->user->id);
                    r = co_await switch_to_io_loop(CoroMapper<models::Rooms>(tx).insert(r));
                    room_id = *r.getRoomId();
                    co_return std::nullopt;
                } catch(const DrogonDbException& e) {
                    const std::string w = e.base().what();
                    LOG_ERROR << "Room insert error: " << w;
                    co_return "Database error during room creation.";
                }
            });

        if(err) {
            common::setStatus(resp, chat::STATUS_FAILURE, *err);
            co_return resp;
        }

        chat::Envelope new_room_msg;
        auto* new_room_resp = new_room_msg.mutable_new_room_created();
        new_room_resp->mutable_room()->set_room_id(room_id);
        new_room_resp->mutable_room()->set_room_name(req.room_name());

        co_await room_service.sendToAll(new_room_msg);
        common::setStatus(resp, chat::STATUS_SUCCESS);
        co_return resp;
    } catch(const std::exception& e) {
        LOG_ERROR << "Create room error: " << e.what();
        common::setStatus(resp, chat::STATUS_FAILURE, std::string("Create room failed: ") + e.what());
        co_return resp;
    }
}

drogon::Task<chat::GetMessagesResponse> MessageHandlers::handleGetMessages(const WsDataPtr& wsDataGuarded, const chat::GetMessagesRequest& req) const {
    chat::GetMessagesResponse resp;

    auto wsData = co_await wsDataGuarded->lock_shared();

    if(wsData->status != USER_STATUS::Authenticated) {
        common::setStatus(resp, chat::STATUS_UNAUTHORIZED, "User not authenticated.");
        co_return resp;
    }
    if(!wsData->room) {
        common::setStatus(resp, chat::STATUS_FAILURE, "User is not in any room.");
        co_return resp;
    }
    try {
        
        auto limit = req.limit();

        LOG_TRACE << "Limit: " + std::to_string(limit);
        LOG_TRACE << "Ts: " + std::to_string(req.offset_ts());

        auto criteria = Criteria(models::Messages::Cols::_created_at, 
                                limit > 0 ? CompareOperator::LT : CompareOperator::GT, 
                                req.offset_ts());
        auto order = limit > 0 ? SortOrder::DESC : SortOrder::ASC;
        limit = std::abs(limit);

        auto messages = co_await switch_to_io_loop(CoroMapper<models::Messages>(m_dbClient)
            .orderBy(models::Messages::Cols::_created_at, order)
            .limit(limit)
            .findBy(Criteria(models::Messages::Cols::_room_id, CompareOperator::EQ, wsData->room->id) && criteria));

        for(const auto& message : messages) {
            auto* message_info = resp.add_message();
            message_info->set_message(message.getValueOfMessageText());
            LOG_TRACE << std::string("Message text \"") + message.getValueOfMessageText() + "\"";
            message_info->set_timestamp(message.getValueOfCreatedAt());
            LOG_TRACE << std::string("Message timestamp \"") + std::to_string(message.getValueOfCreatedAt()) + "\"";
            auto* user_info = message_info->mutable_from();
            auto user = message.getUser(m_dbClient); //TODO this is blocking

            user_info->set_user_id(user.getValueOfUserId());
            user_info->set_user_name(user.getValueOfUsername());
        }
        common::setStatus(resp, chat::STATUS_SUCCESS);
        co_return resp;
    } catch(const std::exception& e) {
        common::setStatus(resp, chat::STATUS_FAILURE, "Failed to retrieve messages: " + std::string(e.what()));
        co_return resp;
    }
}

drogon::Task<chat::LogoutResponse> MessageHandlers::handleLogoutUser(const WsDataPtr& wsDataGuarded, IChatRoomService& room_service) const {
    chat::LogoutResponse resp;

    auto wsData = co_await wsDataGuarded->lock_unique();

    if(wsData->status != USER_STATUS::Authenticated) {
        common::setStatus(resp, chat::STATUS_UNAUTHORIZED, "User not authenticated.");
        co_return resp;
    }
    co_await room_service.logout(*wsData);
    wsData->room.reset();
    wsData->user.reset();
    wsData->status = USER_STATUS::Unauthenticated;
    common::setStatus(resp, chat::STATUS_SUCCESS);
    co_return resp;
}

drogon::Task<chat::RenameRoomResponse> MessageHandlers::handleRenameRoom(const WsDataPtr& wsDataGuarded, const chat::RenameRoomRequest& req, IChatRoomService& room_service) {
    chat::RenameRoomResponse resp;

    auto wsData = co_await wsDataGuarded->lock_shared();

    if(wsData->status != USER_STATUS::Authenticated) {
        common::setStatus(resp, chat::STATUS_UNAUTHORIZED, "Not authenticated");
        co_return resp;
    }
    if(req.name().empty()) {
        common::setStatus(resp, chat::STATUS_FAILURE, "Empty room name or id.");
        co_return resp;
    }
    if(!wsData->room || wsData->room->id != req.room_id()) {
        common::setStatus(resp, chat::STATUS_FAILURE, "User is not in the specified room.");
        co_return resp;
    }
    if (wsData->room->rights < chat::UserRights::OWNER) {
        common::setStatus(resp, chat::STATUS_FAILURE, "Insufficient rights to rename room.");
        co_return resp;
    }
    try {
        auto err = co_await WithTransaction(
            [&](auto tx) -> drogon::Task<ScopedTransactionResult> {
                try {
                    auto room = co_await switch_to_io_loop(CoroMapper<models::Rooms>(tx)
                        .findOne(Criteria(models::Rooms::Cols::_room_id, CompareOperator::EQ, req.room_id())));
                    if (!room.getRoomId()) {
                        co_return "Room not found.";
                    }
                    room.setRoomName(req.name());
                    co_await switch_to_io_loop(CoroMapper<models::Rooms>(tx).update(room));
                    co_return std::nullopt;
                } catch(const DrogonDbException& e) {
                    const std::string w = e.base().what();
                    LOG_ERROR << "Room rename error: " << w;
                    co_return "Database error during room rename.";
                }
            });

        if(err) {
            common::setStatus(resp, chat::STATUS_FAILURE, *err);
            co_return resp;
        }

        chat::Envelope env;
        auto* new_name = env.mutable_new_room_name();
        new_name->set_room_id(wsData->room->id);
        new_name->set_name(req.name());
        co_await room_service.sendToAll(env);
        common::setStatus(resp, chat::STATUS_SUCCESS);
        co_return resp;
    } catch(const std::exception& e) {
        LOG_ERROR << "Create room error: " << e.what();
        common::setStatus(resp, chat::STATUS_FAILURE, std::string("Create room failed: ") + e.what());
        co_return resp;
    }
}

drogon::Task<chat::DeleteRoomResponse> MessageHandlers::handleDeleteRoom(const WsDataPtr& wsDataGuarded, const chat::DeleteRoomRequest& req, IChatRoomService& room_service) {
    chat::DeleteRoomResponse resp;

    auto wsData = co_await wsDataGuarded->lock_shared();

    if(wsData->status != USER_STATUS::Authenticated) {
        common::setStatus(resp, chat::STATUS_UNAUTHORIZED, "Not authenticated.");
        co_return resp;
    }
    if (!wsData->room || wsData->room->id != req.room_id()) {
        common::setStatus(resp, chat::STATUS_FAILURE, "User must be in the room to delete it.");
        co_return resp;
    }
    if (wsData->room->rights < chat::UserRights::OWNER) {
        common::setStatus(resp, chat::STATUS_FAILURE, "Insufficient rights to delete this room.");
        co_return resp;
    }
    const auto room_id = req.room_id();
    try {
        auto err = co_await WithTransaction(
            [&](auto tx) -> drogon::Task<ScopedTransactionResult> {
            try {
                co_await switch_to_io_loop(CoroMapper<models::Messages>(tx)
                    .deleteBy(Criteria(models::Messages::Cols::_room_id, CompareOperator::EQ, room_id)));
                co_await switch_to_io_loop(CoroMapper<models::UserRoomRoles>(tx)
                    .deleteBy(Criteria(models::UserRoomRoles::Cols::_room_id, CompareOperator::EQ, room_id)));
                size_t deleted_count = co_await switch_to_io_loop(CoroMapper<models::Rooms>(tx)
                    .deleteBy(Criteria(models::Rooms::Cols::_room_id, CompareOperator::EQ, room_id)));
                if (deleted_count == 0) {
                    co_return "Room could not be deleted as it was not found.";
                }
                co_return std::nullopt;
            } catch(const DrogonDbException& e) {
                const std::string w = e.base().what();
                LOG_ERROR << "Room deletion transaction failed: " << w;
                co_return "Database error during room deletion.";
            }
        });

        if(err) {
            common::setStatus(resp, chat::STATUS_FAILURE, *err);
            co_return resp;
        }
    } catch (const std::exception& e) {
        LOG_ERROR << "Delete room error: " << e.what();
        common::setStatus(resp, chat::STATUS_FAILURE, std::string("Delete room failed: ") + e.what());
        co_return resp;
    }
    co_await room_service.onRoomDeleted(room_id);
    common::setStatus(resp, chat::STATUS_SUCCESS);
    co_return resp;
}

drogon::Task<chat::AssignRoleResponse> MessageHandlers::handleAssignRole(const WsDataPtr& wsDataGuarded, const chat::AssignRoleRequest& req, IChatRoomService& room_service) {
    chat::AssignRoleResponse resp;
    auto wsData = co_await wsDataGuarded->lock_shared();

    if (wsData->status != USER_STATUS::Authenticated) {
        common::setStatus(resp, chat::STATUS_UNAUTHORIZED, "Not authenticated.");
        co_return resp;
    }
    if (!wsData->room || wsData->room->id != req.room_id()) {
        common::setStatus(resp, chat::STATUS_FAILURE, "User is not in the request room.");
        co_return resp;
    }

    try {
        auto targetUserRightsOpt = co_await getUserRights(req.user_id(), req.room_id());
        auto targetUserRights = targetUserRightsOpt.value_or(chat::UserRights::REGULAR);

        if (wsData->room->rights <= targetUserRights) {
            common::setStatus(resp, chat::STATUS_FAILURE, "Insufficient rights to change this user's role.");
            co_return resp;
        }
        ScopedTransactionResult err;
        if (req.new_role() == chat::UserRights::OWNER) {
            err = co_await WithTransaction([&](auto tx) {
                return transferRoomOwnershipInDb(tx, req.room_id(), req.user_id(), wsData->user->id);
            });
            if (!err) {
                chat::Envelope newOwnerEnv;
                newOwnerEnv.mutable_user_role_changed()->set_user_id(req.user_id());
                newOwnerEnv.mutable_user_role_changed()->set_new_role(chat::UserRights::OWNER);
                co_await room_service.sendToRoom(req.room_id(), newOwnerEnv);

                chat::Envelope oldOwnerEnv;
                oldOwnerEnv.mutable_user_role_changed()->set_user_id(wsData->user->id);
                oldOwnerEnv.mutable_user_role_changed()->set_new_role(chat::UserRights::MODERATOR);
                co_await room_service.sendToRoom(req.room_id(), oldOwnerEnv);
            }
        } else {
            err = co_await WithTransaction([&](auto tx) {
                return updateUserRoleInDb(tx, req.user_id(), req.room_id(), req.new_role());
            });

            if (!err) {
                chat::Envelope roleChangedEnv;
                roleChangedEnv.mutable_user_role_changed()->set_user_id(req.user_id());
                roleChangedEnv.mutable_user_role_changed()->set_new_role(req.new_role());
                co_await room_service.sendToRoom(req.room_id(), roleChangedEnv);
            }
        }
        if (err) {
            common::setStatus(resp, chat::STATUS_FAILURE, *err);
            co_return resp;
        }
        common::setStatus(resp, chat::STATUS_SUCCESS);
        co_return resp;
    } catch (const std::exception& e) {
        LOG_ERROR << "Assign role error: " << e.what();
        common::setStatus(resp, chat::STATUS_FAILURE, std::string("Assign role failed: ") + e.what());
        co_return resp;
    }
}

drogon::Task<std::optional<chat::UserRights>> MessageHandlers::getUserRights(int32_t user_id, int32_t room_id) const {
    auto user = co_await switch_to_io_loop(CoroMapper<models::Users>(m_dbClient)
        .findBy(
            Criteria(models::Users::Cols::_user_id, CompareOperator::EQ, user_id) &&
            Criteria(models::Users::Cols::_is_admin, CompareOperator::EQ, true)));
    if (!user.empty()){
        co_return chat::UserRights::ADMIN;
    }
    auto room = co_await switch_to_io_loop(CoroMapper<models::Rooms>(m_dbClient)
        .findOne(Criteria(models::Rooms::Cols::_room_id, CompareOperator::EQ, room_id)));
    if (room.getOwnerId() && *room.getOwnerId() == user_id) {
        co_return chat::UserRights::OWNER;
    }
    co_return co_await findStoredUserRole(user_id, room_id);
}

drogon::Task<std::optional<chat::UserRights>> MessageHandlers::getUserRights(int32_t user_id, int32_t room_id, const models::Rooms& room) const {
    auto user = co_await switch_to_io_loop(CoroMapper<models::Users>(m_dbClient)
        .findBy(
            Criteria(models::Users::Cols::_user_id, CompareOperator::EQ, user_id) &&
            Criteria(models::Users::Cols::_is_admin, CompareOperator::EQ, true)));
    if (!user.empty()){
        co_return chat::UserRights::ADMIN;
    }
    if (room.getOwnerId() && *room.getOwnerId() == user_id) {
        co_return chat::UserRights::OWNER;
    }
    co_return co_await findStoredUserRole(user_id, room_id);
}

drogon::Task<std::optional<chat::UserRights>> MessageHandlers::findStoredUserRole(int32_t user_id, int32_t room_id) const {
    auto role = co_await switch_to_io_loop(CoroMapper<models::UserRoomRoles>(m_dbClient)
        .findBy(
            Criteria(models::UserRoomRoles::Cols::_user_id, CompareOperator::EQ, user_id) &&
            Criteria(models::UserRoomRoles::Cols::_room_id, CompareOperator::EQ, room_id)));
    if (role.empty()) {
        co_return std::nullopt;
    }
    chat::UserRights rights;
    if (chat::UserRights_Parse(*role.front().getRoleType(), &rights)) {
        co_return rights;
    }
    co_return chat::UserRights::REGULAR;
}

std::optional<std::string> MessageHandlers::validateUtf8String(
    const std::string_view& textToValidate,
    size_t maxLength,
    const std::string_view& fieldName) const {
    
    try {
        size_t length = utf8::distance(textToValidate.begin(), textToValidate.end());
        if (length > maxLength) {
            return std::string("Field '") + std::string(fieldName) +
                   "' is too long. Max length: " + std::to_string(maxLength) + " chars.";
        }
    }
    catch (const utf8::invalid_utf8&) {
        return std::string("Field '") + std::string(fieldName) + "' contains invalid UTF-8 characters.";
    }
    catch (const std::exception& e) {
        return std::string("An unexpected error occurred during ") +
               std::string(fieldName) + " validation: " + e.what();
    }

    return std::nullopt;
}


drogon::Task<ScopedTransactionResult> MessageHandlers::updateUserRoleInDb(drogon::orm::DbClientPtr tx, int32_t userId, int32_t roomId, chat::UserRights newRole) {
    try {
        auto roles = co_await switch_to_io_loop(CoroMapper<models::UserRoomRoles>(tx)
            .findBy(Criteria(models::UserRoomRoles::Cols::_user_id, CompareOperator::EQ, userId) &&
                    Criteria(models::UserRoomRoles::Cols::_room_id, CompareOperator::EQ, roomId)));

        if (roles.empty()) {
            models::UserRoomRoles roleEntry;
            roleEntry.setUserId(userId);
            roleEntry.setRoomId(roomId);
            roleEntry.setRoleType(chat::UserRights_Name(newRole));
            co_await switch_to_io_loop(CoroMapper<models::UserRoomRoles>(tx).insert(roleEntry));
        } else {
            auto roleToUpdate = roles.front();
            roleToUpdate.setRoleType(chat::UserRights_Name(newRole));
            co_await switch_to_io_loop(CoroMapper<models::UserRoomRoles>(tx).update(roleToUpdate));
        }
        co_return std::nullopt;
    } catch (const DrogonDbException& e) {
        LOG_ERROR << "Role update/insert failed: " << e.base().what();
        co_return "Database error during role update.";
    }
}

drogon::Task<ScopedTransactionResult> MessageHandlers::transferRoomOwnershipInDb(drogon::orm::DbClientPtr tx, int32_t roomId, int32_t newOwnerId, int32_t oldOwnerId) {
    try {
        // 1. Находим комнату
        auto room = co_await switch_to_io_loop(CoroMapper<models::Rooms>(tx)
            .findOne(Criteria(models::Rooms::Cols::_room_id, CompareOperator::EQ, roomId)));
        if (!room.getRoomId()) {
            co_return "Room not found during ownership transfer.";
        }

        // 2. Назначаем нового владельца
        room.setOwnerId(newOwnerId);
        co_await switch_to_io_loop(CoroMapper<models::Rooms>(tx).update(room));

        // 3. Понижаем старого владельца до модератора, используя уже существующий хелпер
        auto demoteResult = co_await updateUserRoleInDb(tx, oldOwnerId, roomId, chat::UserRights::MODERATOR);
        if (demoteResult) {
            co_return "Failed to demote old owner: " + *demoteResult;
        }

        // 4. Удаляем запись о роли для нового владельца из UserRoomRoles (если она была)
        co_await switch_to_io_loop(CoroMapper<models::UserRoomRoles>(tx)
            .deleteBy(Criteria(models::UserRoomRoles::Cols::_user_id, CompareOperator::EQ, newOwnerId) &&
                        Criteria(models::UserRoomRoles::Cols::_room_id, CompareOperator::EQ, roomId)));
        
        co_return std::nullopt;
    } catch (const DrogonDbException& e) {
        LOG_ERROR << "Ownership transfer failed: " << e.base().what();
        co_return "Database error during ownership transfer.";
    }
}


} // namespace server
