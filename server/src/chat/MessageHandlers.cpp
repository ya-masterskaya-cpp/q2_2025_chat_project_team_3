#include <server/chat/MessageHandlers.h>
#include <server/chat/WsData.h>
#include <server/chat/IChatRoomService.h>

#include <server/models/Users.h>
#include <server/models/Rooms.h>
#include <server/models/Messages.h>
#include <server/models/RoomMembership.h>
#include <server/models/UserRoomData.h>

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
        if (req.has_password() && req.has_salt()) {
            if (user.getValueOfSalt().empty()) {
                if (user.getValueOfHashPassword() != req.password()) {
                    common::setStatus(resp, chat::STATUS_UNAUTHORIZED, "Incorrect password.");
                    co_return resp;
                }
                auto err = co_await WithTransaction(
                    [&](const auto& tx) -> drogon::Task<ScopedTransactionResult> {
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
            [&](const auto& tx) -> drogon::Task<ScopedTransactionResult> {
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
            [&](const auto& tx) -> drogon::Task<ScopedTransactionResult> {
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
    message_info->set_message_id(inserted_message.getValueOfMessageId());

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
        
        if(wsData->room) {
            co_await room_service.leaveCurrentRoom(*wsData);
            wsData->room.reset();
        }

        auto rooms = co_await switch_to_io_loop(CoroMapper<models::Rooms>(m_dbClient)
            .findBy(Criteria(models::Rooms::Cols::_room_id, CompareOperator::EQ, req.room_id())));
        if(rooms.empty()) {
            common::setStatus(resp, chat::STATUS_NOT_FOUND, "Room does not exist.");
            co_return resp;
        }

        auto room = rooms.front();

        auto membership_status = co_await getUserMembershipStatus(m_dbClient, wsData->user->id, req.room_id());

        if(room.getValueOfIsPrivate() && (!membership_status || *membership_status != chat::MembershipStatus::JOINED)) {
            common::setStatus(resp, chat::STATUS_UNAUTHORIZED, "Cannot join private room.");
            co_return resp;
        }

        if(!room.getValueOfIsPrivate() && (!membership_status || *membership_status != chat::MembershipStatus::JOINED)) {
            co_await setUserMembershipStatus(m_dbClient, wsData->user->id, req.room_id(), chat::MembershipStatus::JOINED);
        }

        std::optional<chat::UserRights> role = co_await getUserRights(m_dbClient, wsData->user->id, req.room_id(), room);

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
    int32_t room_id;
    try {
        auto err = co_await WithTransaction(
            [&](const auto& tx) -> drogon::Task<ScopedTransactionResult> {
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
            message_info->set_message_id(message.getValueOfMessageId());
            LOG_TRACE << std::string("Message id \"") + std::to_string(message.getValueOfMessageId()) + "\"";
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
            [&](const auto& tx) -> drogon::Task<ScopedTransactionResult> {
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
            [&](const auto& tx) -> drogon::Task<ScopedTransactionResult> {
            try {
                auto deleted_count = co_await switch_to_io_loop(CoroMapper<models::Rooms>(tx)
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

    //unfortunately, we need unique lock here
    //user can "demote" themselves from owning a room
    auto wsData = co_await wsDataGuarded->lock_unique(); 

    if(wsData->status != USER_STATUS::Authenticated) {
        common::setStatus(resp, chat::STATUS_UNAUTHORIZED, "Not authenticated.");
        co_return resp;
    }
    if(!wsData->room || wsData->room->id != req.room_id()) {
        common::setStatus(resp, chat::STATUS_FAILURE, "User is not in the request room.");
        co_return resp;
    }

    int32_t oldOwnerId = 0;
    std::optional<chat::UserRights> oldOwnerNewRole_optional;

    try {
        //do ALL db operations first, inside SINGLE transaction
        auto err = co_await WithTransaction([&](const auto& tx) -> drogon::Task<ScopedTransactionResult> {
            auto targetUserRightsOpt = co_await this->getUserRights(tx, req.user_id(), req.room_id());
            auto targetUserRights = targetUserRightsOpt.value_or(chat::UserRights::REGULAR);

            // first, check that target role is below ours. special case: room ownership transfer
            if (req.new_role() >= wsData->room->rights &&
                !(wsData->room->rights >= chat::UserRights::OWNER && req.new_role() == chat::UserRights::OWNER)) {
                co_return "Insufficient rights to change this user's role.";
            }

            if(targetUserRights >= wsData->room->rights) { //then check if target user's role is below ours
                co_return "Insufficient rights to change this user's role.";
            }

            if(req.new_role() == chat::UserRights::OWNER) { //special case, changing owner
                //step 1
                //fetch room
                auto room = co_await switch_to_io_loop(CoroMapper<models::Rooms>(tx)
                .findOne(Criteria(models::Rooms::Cols::_room_id, CompareOperator::EQ, req.room_id())));

                //step 2
                //store old owner id, if there was any
                oldOwnerId = room.getOwnerId() ? *room.getOwnerId() : 0;

                //step 3
                //update room with new owner id
                room.setOwnerId(req.user_id());
                co_await switch_to_io_loop(CoroMapper<models::Rooms>(tx).update(room));

                if(oldOwnerId) {
                    //step 4
                    //if there was an old owner, then fetch their current role as per db
                    oldOwnerNewRole_optional = co_await getUserRights(tx, oldOwnerId, req.room_id(), room);

                    //step 5
                    //if old owner is not a global admin, or had no explicit record for role, or if that role is below `MODERATOR`
                    //then we need to either create new record or update existing one
                    if(!oldOwnerNewRole_optional || *oldOwnerNewRole_optional < chat::UserRights::MODERATOR) {
                        oldOwnerNewRole_optional = chat::UserRights::MODERATOR;
                        auto inner_err = co_await updateUserRoleInDb(tx, oldOwnerId, req.room_id(), *oldOwnerNewRole_optional);
                        if(inner_err) {
                            co_return inner_err;
                        }
                    }

                    // NOTE: we do not have to clean up new owner's explicit role, cuz `OWNER` is stored inside `rooms`
                    // table directly and takes precedence, if user is demoted later then code above will make sure they are at least `MODERATOR`
                }
            } else { //much simpler regular case :з
                auto inner_err = co_await updateUserRoleInDb(tx, req.user_id(), req.room_id(), req.new_role());
                if(inner_err) {
                    co_return inner_err;
                }
            }

            co_return std::nullopt;
        });

        if(err) {
            common::setStatus(resp, chat::STATUS_FAILURE, *err);
            co_return resp;
        }

        //now we need to update our in memory cache
        //and send info to connected clients

        if(oldOwnerId) { //special case, owner change
            co_await room_service.updateUserRoomRights(oldOwnerId, req.room_id(), *oldOwnerNewRole_optional, *wsData);
        }
        co_await room_service.updateUserRoomRights(req.user_id(), req.room_id(), req.new_role(), *wsData);

        common::setStatus(resp, chat::STATUS_SUCCESS);
        co_return resp;
    } catch(const std::exception& e) {
        LOG_ERROR << "Assign role error: " << e.what();
        common::setStatus(resp, chat::STATUS_FAILURE, std::string("Assign role failed: ") + e.what());
        co_return resp;
    }
}

drogon::Task<std::optional<chat::UserRights>> MessageHandlers::getUserRights(const drogon::orm::DbClientPtr& db, int32_t user_id, int32_t room_id) const {
    // 1. Fetch the room object.
    auto room = co_await switch_to_io_loop(CoroMapper<models::Rooms>(db)
        .findOne(Criteria(models::Rooms::Cols::_room_id, CompareOperator::EQ, room_id)));
        
    // 2. Delegate to the second overload, passing the fetched room.
    co_return co_await getUserRights(db, user_id, room_id, room);
}

drogon::Task<std::optional<chat::UserRights>> MessageHandlers::getUserRights(const drogon::orm::DbClientPtr& db, int32_t user_id, int32_t room_id, const models::Rooms& room) const {
    // Check if the user is a global admin first.
    auto user = co_await switch_to_io_loop(CoroMapper<models::Users>(db)
        .findBy(
            Criteria(models::Users::Cols::_user_id, CompareOperator::EQ, user_id) &&
            Criteria(models::Users::Cols::_is_admin, CompareOperator::EQ, true)));
    if (!user.empty()){
        co_return chat::UserRights::ADMIN;
    }

    // Then, check if they are the owner of this specific room.
    if (room.getOwnerId() && *room.getOwnerId() == user_id) {
        co_return chat::UserRights::OWNER;
    }

    // Finally, look for a specific role entry in the user_room_data table.
    co_return co_await findStoredUserRole(db, user_id, room_id);
}

drogon::Task<std::optional<chat::UserRights>> MessageHandlers::findStoredUserRole(const drogon::orm::DbClientPtr& db, int32_t user_id, int32_t room_id) const {
    auto data = co_await switch_to_io_loop(CoroMapper<models::UserRoomData>(db)
        .findBy(
            Criteria(models::UserRoomData::Cols::_user_id, CompareOperator::EQ, user_id) &&
            Criteria(models::UserRoomData::Cols::_room_id, CompareOperator::EQ, room_id)));
    if(data.empty()) {
        co_return std::nullopt;
    }

    if(*data.front().getIsModerator()) {
        co_return chat::UserRights::MODERATOR;
    }

    co_return std::nullopt;
}

drogon::Task<chat::DeleteMessageResponse> MessageHandlers::handleDeleteMessage(const WsDataPtr& wsDataGuarded, const chat::DeleteMessageRequest& req, IChatRoomService& room_service) {
    chat::DeleteMessageResponse resp;

    auto wsData = co_await wsDataGuarded->lock_shared();

    if (wsData->status != USER_STATUS::Authenticated) {
        common::setStatus(resp, chat::STATUS_UNAUTHORIZED, "Not authenticated.");
        co_return resp;
    }
    if (!wsData->room) {
        common::setStatus(resp, chat::STATUS_FAILURE, "User is not in any room.");
        co_return resp;
    }
    if (wsData->room->rights < chat::UserRights::MODERATOR) {
        common::setStatus(resp, chat::STATUS_UNAUTHORIZED, "Insufficient rights to delete messages.");
        co_return resp;
    }

    const int32_t messageId = req.message_id();
    const int32_t roomId = wsData->room->id;
    try {
        auto err = co_await WithTransaction([&](const auto& tx) -> drogon::Task<ScopedTransactionResult> {
            try {
                auto messages = co_await switch_to_io_loop(CoroMapper<models::Messages>(tx)
                    .findBy(Criteria(models::Messages::Cols::_message_id, CompareOperator::EQ, messageId) &&
                            Criteria(models::Messages::Cols::_room_id, CompareOperator::EQ, roomId)));
                if (messages.empty()) {
                    co_return "Message not found or does not belong to this room.";
                }
                size_t deletedCount = co_await switch_to_io_loop(CoroMapper<models::Messages>(tx)
                    .deleteBy(Criteria(models::Messages::Cols::_message_id, CompareOperator::EQ, messageId)));

                if (deletedCount == 0) {
                    co_return "Message could not be deleted.";
                }
                co_return std::nullopt;
            } catch (const DrogonDbException& e) {
                LOG_ERROR << "Message deletion transaction failed: " << e.base().what();
                co_return "Database error during message deletion.";
            }
        });
        if (err) {
            common::setStatus(resp, chat::STATUS_FAILURE, *err);
            co_return resp;
        }
    } catch (const std::exception& e) {
        LOG_ERROR << "Delete message error: " << e.what();
        common::setStatus(resp, chat::STATUS_FAILURE, std::string("Delete message failed: ") + e.what());
        co_return resp;
    }
    
    chat::Envelope deletedNoticeEnv;
    auto* messageDeleted = deletedNoticeEnv.mutable_message_deleted();
    messageDeleted->set_message_id(messageId);
    co_await room_service.sendToRoom(roomId, deletedNoticeEnv);
    
    common::setStatus(resp, chat::STATUS_SUCCESS);
    co_return resp;
}

drogon::Task<chat::UserTypingStartResponse> MessageHandlers::handleUserTypingStart(const WsDataPtr& wsDataGuarded, IChatRoomService& room_service) const {
    chat::UserTypingStartResponse resp;

    auto wsData = co_await wsDataGuarded->lock_shared();

    if (wsData->status != USER_STATUS::Authenticated) {
        common::setStatus(resp, chat::STATUS_UNAUTHORIZED, "Not authenticated.");
        co_return resp;
    }
    if (!wsData->room) {
        common::setStatus(resp, chat::STATUS_FAILURE, "User is not in any room.");
        co_return resp;
    }

    chat::Envelope broadcastEnv;
    auto* startedTypingMsg = broadcastEnv.mutable_user_started_typing();

    auto* userInfo = startedTypingMsg->mutable_user();
    userInfo->set_user_id(wsData->user->id);
    userInfo->set_user_name(wsData->user->name);
    userInfo->set_user_room_rights(wsData->room->rights);
    
	co_await room_service.sendToRoom(wsData->room->id, broadcastEnv);

    common::setStatus(resp, chat::STATUS_SUCCESS);
    co_return resp;
}

drogon::Task<chat::UserTypingStopResponse> MessageHandlers::handleUserTypingStop(const WsDataPtr& wsDataGuarded, IChatRoomService& room_service) const {
    chat::UserTypingStopResponse resp;

    auto wsData = co_await wsDataGuarded->lock_shared();

    if (wsData->status != USER_STATUS::Authenticated) {
        common::setStatus(resp, chat::STATUS_UNAUTHORIZED, "Not authenticated.");
        co_return resp;
    }
    if (!wsData->room) {
        common::setStatus(resp, chat::STATUS_FAILURE, "User is not in any room.");
        co_return resp;
    }

    chat::Envelope broadcastEnv;
    auto* stoppedTypingMsg = broadcastEnv.mutable_user_stopped_typing();

    auto* userInfo = stoppedTypingMsg->mutable_user();
    userInfo->set_user_id(wsData->user->id);
    userInfo->set_user_name(wsData->user->name);
    userInfo->set_user_room_rights(wsData->room->rights);

    co_await room_service.sendToRoom(wsData->room->id, broadcastEnv);

    common::setStatus(resp, chat::STATUS_SUCCESS);
    co_return resp;
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

drogon::Task<ScopedTransactionResult> MessageHandlers::updateUserRoleInDb(const std::shared_ptr<drogon::orm::Transaction>& tx, int32_t userId, int32_t roomId, chat::UserRights newRole) {
    try {

        if(newRole != chat::UserRights::REGULAR && newRole != chat::UserRights::MODERATOR) {
            co_return "For now this function can only toggle moderator status";
        }

        auto room_data = co_await switch_to_io_loop(CoroMapper<models::UserRoomData>(tx)
            .findBy(Criteria(models::UserRoomData::Cols::_user_id, CompareOperator::EQ, userId) &&
                    Criteria(models::UserRoomData::Cols::_room_id, CompareOperator::EQ, roomId)));

        if (room_data.empty()) {
            models::UserRoomData userRoomData;
            userRoomData.setUserId(userId);
            userRoomData.setRoomId(roomId);
            userRoomData.setIsModerator(newRole == chat::UserRights::MODERATOR);
            co_await switch_to_io_loop(CoroMapper<models::UserRoomData>(tx).insert(userRoomData));
        } else {
            auto dataToUpdate = room_data.front();
            dataToUpdate.setIsModerator(newRole == chat::UserRights::MODERATOR);
            co_await switch_to_io_loop(CoroMapper<models::UserRoomData>(tx).update(dataToUpdate));
        }
        co_return std::nullopt;
    } catch (const DrogonDbException& e) {
        LOG_ERROR << "Role update/insert failed: " << e.base().what();
        co_return "Database error during role update.";
    }
}

drogon::Task<std::optional<chat::MembershipStatus>> MessageHandlers::getUserMembershipStatus(const drogon::orm::DbClientPtr& db, int32_t user_id, int32_t room_id) {
    auto room_membership = co_await switch_to_io_loop(CoroMapper<models::RoomMembership>(db)
        .findBy(Criteria(models::RoomMembership::Cols::_user_id, CompareOperator::EQ, user_id) &&
                Criteria(models::RoomMembership::Cols::_room_id, CompareOperator::EQ, room_id)));

    if(room_membership.empty()) {
        co_return std::nullopt;
    }

    chat::MembershipStatus status;

    if(chat::MembershipStatus_Parse(*room_membership.front().getMembershipStatus(), &status)) {
        co_return status;
    }

    co_return std::nullopt;
}

drogon::Task<ScopedTransactionResult> MessageHandlers::setUserMembershipStatus(const drogon::orm::DbClientPtr& db, int32_t user_id, int32_t room_id, chat::MembershipStatus status) {
    try {
        auto room_membership = co_await switch_to_io_loop(CoroMapper<models::RoomMembership>(db)
            .findBy(Criteria(models::RoomMembership::Cols::_user_id, CompareOperator::EQ, user_id) &&
                    Criteria(models::RoomMembership::Cols::_room_id, CompareOperator::EQ, room_id)));
    
        if(room_membership.empty()) {
            models::RoomMembership membership;
            membership.setUserId(user_id);
            membership.setRoomId(room_id);
            membership.setMembershipStatus(chat::MembershipStatus_Name(status));
            co_await switch_to_io_loop(CoroMapper<models::RoomMembership>(db).insert(membership));
            co_return std::nullopt;
        } else {
            auto membership = room_membership.front();
            membership.setMembershipStatus(chat::MembershipStatus_Name(status));
            co_await switch_to_io_loop(CoroMapper<models::RoomMembership>(db).update(membership));
            co_return std::nullopt;
        }
    
    } catch(const DrogonDbException& e) {
        LOG_ERROR << "Membership status update/insert failed: " << e.base().what();
        co_return "Database error during membership update.";
    }
}

} // namespace server
