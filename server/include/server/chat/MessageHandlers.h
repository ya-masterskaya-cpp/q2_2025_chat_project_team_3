#pragma once

#include <drogon/orm/CoroMapper.h>

#include <server/chat/WsData.h>
#include <server/chat/IChatRoomService.h>
#include <server/chat/ChatRoomManager.h>
#include <server/utils/utils.h>
#include <server/utils/scoped_coro_transaction.h>

#include <server/models/Users.h>
#include <server/models/Rooms.h>
#include <server/models/Messages.h>
#include <utf8.h>

namespace models = drogon_model::drogon_test;

class MessageHandlers {
public:
    static drogon::Task<chat::InitialAuthResponse> handleAuthInitial(const std::shared_ptr<WsData>& wsData, const chat::InitialAuthRequest& req) {
        chat::InitialAuthResponse resp;
        auto db = drogon::app().getDbClient();
        if(!db) {
            setStatus(resp, chat::STATUS_FAILURE, "Server configuration error: DB not available.");
            co_return resp;
        }
        if(req.username().empty()) {
            setStatus(resp, chat::STATUS_FAILURE, "Empty username.");
            co_return resp;
        }
        try {
            using namespace drogon::orm;
            auto users = co_await drogon::orm::CoroMapper<models::Users>(db)
                .findBy(Criteria(models::Users::Cols::_username, CompareOperator::EQ, req.username()));
            if(users.empty()) {
                setStatus(resp, chat::STATUS_UNAUTHORIZED, "Invalid credentials.");
                co_return resp;
            }
            wsData->status = USER_STATUS::Authenticating;
            wsData->user = User{.id = 0, .name = req.username()};
            if (!users.front().getValueOfSalt().empty()) resp.set_salt(users.front().getValueOfSalt());

            setStatus(resp, chat::STATUS_SUCCESS);
            co_return resp;
        } catch(const std::exception& e) {
            setStatus(resp, chat::STATUS_FAILURE, std::string("Auth failed: ") + e.what());
            co_return resp;
        }
    }

    static drogon::Task<chat::InitialRegisterResponse> handleRegisterInitial(const std::shared_ptr<WsData>& wsData, const chat::InitialRegisterRequest& req) {
        chat::InitialRegisterResponse resp;
        auto db = drogon::app().getDbClient();
        if(!db) {
            setStatus(resp, chat::STATUS_FAILURE, "Server configuration error: DB not available.");
            co_return resp;
        }
        if(req.username().empty()) {
            setStatus(resp, chat::STATUS_FAILURE, "Empty username or password.");
            co_return resp;
        }
        try {
            using namespace drogon::orm;
            auto users = co_await drogon::orm::CoroMapper<models::Users>(db)
                .findBy(Criteria(models::Users::Cols::_username, CompareOperator::EQ, req.username()));
            if(!users.empty()) {
                setStatus(resp, chat::STATUS_FAILURE, "Username already exists.");
                co_return resp;
            }
            wsData->status = USER_STATUS::Registering;
            wsData->user = User{.id = 0, .name = req.username() };
            setStatus(resp, chat::STATUS_SUCCESS);
            co_return resp;
        } catch(const std::exception& e) {
            setStatus(resp, chat::STATUS_FAILURE, std::string("Registration failed: ") + e.what());
            co_return resp;
        }
    }

    static drogon::Task<chat::AuthResponse> handleAuth(const std::shared_ptr<WsData>& wsData, const chat::AuthRequest& req, IChatRoomService& room_service) {
    chat::AuthResponse resp;

    if (wsData->status != USER_STATUS::Authenticating) {
        setStatus(resp, chat::STATUS_FAILURE, "Not in authentication phase.");
        co_return resp;
    }

    if (req.hash().empty()) {
        setStatus(resp, chat::STATUS_FAILURE, "Missing hash.");
        co_return resp;
    }

    try {
        using namespace drogon::orm;
        auto db = drogon::app().getDbClient();

        auto users = co_await CoroMapper<models::Users>(db)
            .findBy(Criteria(models::Users::Cols::_username, CompareOperator::EQ, wsData->user->name));

        if (users.empty()) {
            wsData->status = USER_STATUS::Unauthenticated;
            setStatus(resp, chat::STATUS_UNAUTHORIZED, "User not found.");
            co_return resp;
        }

        auto user = users.front();
        auto pas = req.password();
        auto salt = req.salt();
        if (req.has_password() && req.has_salt()) {
            if (user.getValueOfSalt().empty()) {
                if (user.getValueOfHashPassword() != req.password()) {
                    setStatus(resp, chat::STATUS_UNAUTHORIZED, "Incorrect password.");
                    co_return resp;
                }
                auto err = co_await WithTransaction(
                    [&](auto tx) -> drogon::Task<std::optional<std::string>> {
                        try {
                            user.setHashPassword(req.hash());
                            user.setSalt(req.salt());
                            co_await CoroMapper<models::Users>(tx).update(user);
                            co_return std::nullopt;
                        } catch (const drogon::orm::DrogonDbException& e) {
                            LOG_ERROR << "User update error: " << e.base().what();
                            co_return "Database error during user update.";
                        }
                    });

                if (err) {
                    wsData->status = USER_STATUS::Unauthenticated;
                    setStatus(resp, chat::STATUS_FAILURE, *err);
                    co_return resp;
                }
            } else {
                setStatus(resp, chat::STATUS_FAILURE, "User already migrated. Non correct Auth");
                co_return resp;
            }
        } else {
            if (user.getValueOfSalt().empty()) {
                setStatus(resp, chat::STATUS_FAILURE, "Migration required.");
                co_return resp;
            }

            if (user.getValueOfHashPassword() != req.hash()) {
                setStatus(resp, chat::STATUS_UNAUTHORIZED, "Hash mismatch.");
                co_return resp;
            }
        }
        
        auto rooms = co_await drogon::orm::CoroMapper<models::Rooms>(db).findAll();
        for(const auto& room : rooms) {
            chat::RoomInfo* room_info = resp.add_rooms();
            room_info->set_room_id(room.getValueOfRoomId());
            room_info->set_room_name(room.getValueOfRoomName());
        }

        wsData->user->id = *user.getUserId();
        wsData->status = USER_STATUS::Authenticated;
        room_service.login();
        setStatus(resp, chat::STATUS_SUCCESS);
        co_return resp;
    } catch (const std::exception& e) {
        setStatus(resp, chat::STATUS_FAILURE, std::string("Auth failed: ") + e.what());
        co_return resp;
    }
    }

    static drogon::Task<chat::RegisterResponse> handleRegister(const std::shared_ptr<WsData>& wsData, const chat::RegisterRequest& req) {
        chat::RegisterResponse resp;
        if (wsData->status != USER_STATUS::Registering) {
            setStatus(resp, chat::STATUS_FAILURE, "Non-correct registering");
            co_return resp;
        }
        if(req.salt().empty() || req.hash().empty()) {
            setStatus(resp, chat::STATUS_FAILURE, "Empty hash or salt.");
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
                        co_await drogon::orm::CoroMapper<models::Users>(tx).insert(u);
                        co_return std::nullopt;
                    } catch(const drogon::orm::DrogonDbException& e) {
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
                setStatus(resp, chat::STATUS_FAILURE, *err);
                co_return resp;
            }
            wsData->status = USER_STATUS::Unauthenticated;
            setStatus(resp, chat::STATUS_SUCCESS);
            co_return resp;
        } catch(const std::exception& e) {
            LOG_ERROR << "Register error: " << e.what();
            wsData->status = USER_STATUS::Unauthenticated;
            setStatus(resp, chat::STATUS_FAILURE, std::string("Registration failed: ") + e.what());
            co_return resp;
        }
    }

    static drogon::Task<chat::SendMessageResponse> handleSendMessage(const std::shared_ptr<WsData>& wsData, const chat::SendMessageRequest& req) {
        chat::SendMessageResponse resp;
        if(!wsData->user) {
            setStatus(resp, chat::STATUS_UNAUTHORIZED, "User not authenticated.");
            co_return resp;
        }
        if(!wsData->room) {
            setStatus(resp, chat::STATUS_FAILURE, "User is not in any room.");
            co_return resp;
        }
        if(req.message().empty()) {
            setStatus(resp, chat::STATUS_FAILURE, "Empty 'message' field.");
            co_return resp;
        }

        try {
            if (size_t dist = utf8::distance(req.message().begin(), req.message().end()); dist > 512) {
                setStatus(resp, chat::STATUS_FAILURE, "Big message (>512ch).");
                co_return resp;
            }
        }
        catch (const std::exception& e) {
            setStatus(resp, chat::STATUS_FAILURE, "Invalid UTF-8 message.");
            co_return resp;
        }

        auto db = drogon::app().getDbClient();
        if(!db) {
            setStatus(resp, chat::STATUS_FAILURE, "Server configuration error: DB not available.");
            co_return resp;
        }

        try {
            auto err = co_await WithTransaction(
                [&](auto tx) -> drogon::Task<ScopedTransactionResult> {
                    try {
                        models::Messages m;
                        m.setMessageText(req.message());
                        m.setRoomId(wsData->room->id);
                        m.setUserId(wsData->user->id);
                        co_await drogon::orm::CoroMapper<models::Messages>(tx).insert(m);
                        co_return std::nullopt;
                    } catch(const drogon::orm::DrogonDbException& e) {
                        const std::string w = e.base().what();
                        LOG_ERROR << "Message insert error: " << w;
                        co_return "Database error during message insertion.";
                    }
                });

            if(err) {
                setStatus(resp, chat::STATUS_FAILURE, *err);
                co_return resp;
            }
        } catch(const std::exception& e) {
            LOG_ERROR << "Inser message error: " << e.what();
            setStatus(resp, chat::STATUS_FAILURE, std::string("Insert message failed: ") + e.what());
            co_return resp;
        }

        // Build and broadcast RoomMessage
        chat::Envelope msgEnv;
        auto* msg = msgEnv.mutable_room_message();
        auto* message_info = msg->mutable_message();
        auto* user_info = message_info->mutable_from();

        message_info->set_message(req.message());
        message_info->set_timestamp(trantor::Date::now().microSecondsSinceEpoch());

        user_info->set_user_id(wsData->user->id);
        user_info->set_user_name(wsData->user->name);

        ChatRoomManager::instance().sendToRoom(wsData->room->id, msgEnv);

        setStatus(resp, chat::STATUS_SUCCESS);
        co_return resp;
    }

    static drogon::Task<chat::JoinRoomResponse> handleJoinRoom(const std::shared_ptr<WsData>& wsData, const chat::JoinRoomRequest& req, IChatRoomService& room_service) {
        chat::JoinRoomResponse resp;
        if(!wsData->user) {
            setStatus(resp, chat::STATUS_UNAUTHORIZED, "User not authenticated.");
            co_return resp;
        }
        auto db = drogon::app().getDbClient();
        if(!db) {
            setStatus(resp, chat::STATUS_FAILURE, "DB not available.");
            co_return resp;
        }
        try {
            using namespace drogon::orm;
            auto rooms = co_await CoroMapper<models::Rooms>(db)
                .findBy(Criteria(models::Rooms::Cols::_room_id, CompareOperator::EQ, req.room_id()));
            if(rooms.empty()) {
                setStatus(resp, chat::STATUS_NOT_FOUND, "Room does not exist.");
                co_return resp;
            }
            wsData->room = CurrentRoom{req.room_id(), chat::UserRights::REGULAR};

            room_service.joinRoom();

            auto users = ChatRoomManager::instance().getUsersInRoom(req.room_id());
            *resp.mutable_users() = {std::make_move_iterator(users.begin()), 
                                      std::make_move_iterator(users.end())};

            chat::Envelope user_joined_msg;
            user_joined_msg.mutable_user_joined()->mutable_user()->set_user_id(wsData->user->id);
            user_joined_msg.mutable_user_joined()->mutable_user()->set_user_name(wsData->user->name);
            user_joined_msg.mutable_user_joined()->mutable_user()->set_user_room_rights(wsData->room->rights);
            ChatRoomManager::instance().sendToRoom(wsData->room->id, user_joined_msg);

            setStatus(resp, chat::STATUS_SUCCESS);
            co_return resp;
        } catch(const std::exception& e) {
            setStatus(resp, chat::STATUS_FAILURE, "Failed to join room: " + std::string(e.what()));
            co_return resp;
        }
    }

    static drogon::Task<chat::LeaveRoomResponse> handleLeaveRoom(const std::shared_ptr<WsData>& wsData, const chat::LeaveRoomRequest&, IChatRoomService& room_service) {
        chat::LeaveRoomResponse resp;
        if(!wsData->user) {
            setStatus(resp, chat::STATUS_UNAUTHORIZED, "User not authenticated.");
            co_return resp;
        }
        if(!wsData->room) {
            setStatus(resp, chat::STATUS_FAILURE, "User is not in any room.");
            co_return resp;
        }

        room_service.leaveCurrentRoom();
        wsData->room.reset();

        setStatus(resp, chat::STATUS_SUCCESS);
        co_return resp;
    }

    static drogon::Task<chat::CreateRoomResponse> handleCreateRoom(const std::shared_ptr<WsData>& wsData, const chat::CreateRoomRequest& req) {
        chat::CreateRoomResponse resp;
        if(!wsData->user) {
            setStatus(resp, chat::STATUS_UNAUTHORIZED, "Not authenticated");
            co_return resp;
        }
        if(req.room_name().empty()) {
            setStatus(resp, chat::STATUS_FAILURE, "Empty room name.");
            co_return resp;
        }
        uint32_t room_id;
        try {
            auto err = co_await WithTransaction(
                [&](auto tx) -> drogon::Task<ScopedTransactionResult> {
                    try {
                        models::Rooms r;
                        r.setRoomName(req.room_name());
                        r = co_await drogon::orm::CoroMapper<models::Rooms>(tx).insert(r);
                        room_id = *r.getRoomId();
                        co_return std::nullopt;
                    } catch(const drogon::orm::DrogonDbException& e) {
                        const std::string w = e.base().what();
                        LOG_ERROR << "Room insert error: " << w;
                        co_return "Database error during room creation.";
                    }
                });

            if(err) {
                setStatus(resp, chat::STATUS_FAILURE, *err);
                co_return resp;
            }

            chat::Envelope new_room_msg;
            auto* new_room_resp = new_room_msg.mutable_new_room_response();
            new_room_resp->mutable_room()->set_room_id(room_id);
            new_room_resp->mutable_room()->set_room_name(req.room_name());

            ChatRoomManager::instance().sendToAll(new_room_msg);
            setStatus(resp, chat::STATUS_SUCCESS);
            co_return resp;
        } catch(const std::exception& e) {
            LOG_ERROR << "Create room error: " << e.what();
            setStatus(resp, chat::STATUS_FAILURE, std::string("Create room failed: ") + e.what());
            co_return resp;
        }
    }

    static drogon::Task<chat::GetMessagesResponse> handleGetMessages(const std::shared_ptr<WsData>& wsData, const chat::GetMessagesRequest& req) {
        chat::GetMessagesResponse resp;
        if(!wsData->user) {
            setStatus(resp, chat::STATUS_UNAUTHORIZED, "User not authenticated.");
            co_return resp;
        }
        if(!wsData->room) {
            setStatus(resp, chat::STATUS_FAILURE, "User is not in any room.");
            co_return resp;
        }
        auto db = drogon::app().getDbClient();
        if(!db) {
            setStatus(resp, chat::STATUS_FAILURE, "DB not available.");
            co_return resp;
        }
        try {
            using namespace drogon::orm;
            auto mapper = drogon::orm::CoroMapper<models::Messages>(db);

            auto limit = req.limit();

            LOG_DEBUG << "Limit: " + std::to_string(limit);
            LOG_DEBUG << "Ts: " + std::to_string(req.offset_ts());

            auto criteria = Criteria(models::Messages::Cols::_created_at, 
                                    limit > 0 ? CompareOperator::LT : CompareOperator::GT, 
                                    trantor::Date(req.offset_ts()));
            auto order = limit > 0 ? SortOrder::DESC : SortOrder::ASC;
            limit = std::abs(limit);

            auto messages = co_await mapper
                .orderBy(models::Messages::Cols::_created_at, order)
                .limit(limit)
                .findBy(Criteria(models::Messages::Cols::_room_id, CompareOperator::EQ, wsData->room->id) && criteria);

            for(const auto& message : messages) {
                auto* message_info = resp.add_message();
                message_info->set_message(message.getValueOfMessageText());
                LOG_DEBUG << std::string("Message text \"") + message.getValueOfMessageText() + "\"";
                message_info->set_timestamp(message.getValueOfCreatedAt().microSecondsSinceEpoch());
                LOG_DEBUG << std::string("Message timestamp \"") + std::to_string(message.getValueOfCreatedAt().microSecondsSinceEpoch()) + "\"";
                auto* user_info = message_info->mutable_from();
                auto user = message.getUser(db); //TODO this is blocking

                user_info->set_user_id(user.getValueOfUserId());
                user_info->set_user_name(user.getValueOfUsername());
            }
            setStatus(resp, chat::STATUS_SUCCESS);
            co_return resp;
        } catch(const std::exception& e) {
            setStatus(resp, chat::STATUS_FAILURE, "Failed to retrieve messages: " + std::string(e.what()));
            co_return resp;
        }
    }

    static drogon::Task<chat::LogoutResponse> handleLogoutUser(const std::shared_ptr<WsData>& wsData, IChatRoomService& room_service) {
        chat::LogoutResponse resp;
        if(wsData->status != USER_STATUS::Authenticated) {
            setStatus(resp, chat::STATUS_UNAUTHORIZED, "User not authenticated.");
            co_return resp;
        }
        room_service.logout();
        wsData->room.reset();
        wsData->user.reset();
        wsData->status = USER_STATUS::Unauthenticated;
        setStatus(resp, chat::STATUS_SUCCESS);
        co_return resp;
    }

};
