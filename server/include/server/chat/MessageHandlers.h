#pragma once

#include <server/utils/utils.h>
#include <server/utils/scoped_coro_transaction.h>
#include <server/chat/WsData.h>
#include <server/chat/UserConnectionRegistry.h>
#include <server/chat/IAuthNotifier.h>
#include <server/chat/UserRoomRegistry.h>
#include <drogon/drogon.h> // needed for logging
#include <drogon/orm/CoroMapper.h> // needed for ORM
#include <server/models/Users.h>
#include <server/models/Rooms.h>
#include <server/models/Messages.h>
#include <common/proto/chat.pb.h>

namespace models = drogon_model::drogon_test;

class MessageHandlers {
public:
    static drogon::Task<chat::AuthResponse> handleAuth(std::shared_ptr<WsData> wsData, const chat::AuthRequest& req, IAuthNotifier& notifier) {
        chat::AuthResponse resp;
        auto db = drogon::app().getDbClient();
        if(!db) {
            setStatus(resp, chat::STATUS_FAILURE, "Server configuration error: DB not available.");
            co_return resp;
        }
        if(req.username().empty() || req.password().empty()) {
            setStatus(resp, chat::STATUS_FAILURE, "Empty username or password.");
            co_return resp;
        }
        try {
            using namespace drogon::orm;
            auto users = co_await drogon::orm::CoroMapper<models::Users>(db)
                .findBy(Criteria(models::Users::Cols::_username, CompareOperator::EQ, req.username()));
            if(users.empty() || users.front().getValueOfPassword() != req.password()) {
                setStatus(resp, chat::STATUS_UNAUTHORIZED, "Invalid credentials.");
                co_return resp;
            }
            wsData->authenticated = true;
            wsData->username = req.username();
            wsData->user_id = *users.front().getUserId();
            notifier.onUserAuthenticated(req.username());
            setStatus(resp, chat::STATUS_SUCCESS);
            resp.set_token("dummy_token");
            co_return resp;
        } catch(const std::exception& e) {
            setStatus(resp, chat::STATUS_FAILURE, std::string("Auth failed: ") + e.what());
            co_return resp;
        }
    }

    static drogon::Task<chat::RegisterResponse> handleRegister(const chat::RegisterRequest& req) {
        chat::RegisterResponse resp;
        if(req.username().empty() || req.password().empty()) {
            setStatus(resp, chat::STATUS_FAILURE, "Empty username or password.");
            co_return resp;
        }
        try {
            // Use a transaction to guarantee atomicity and handle duplicate usernames gracefully
            auto err = co_await WithTransaction(
                [&](auto tx) -> drogon::Task<ScopedTransactionResult> {
                    try {
                        models::Users u;
                        u.setUsername(req.username());
                        u.setPassword(req.password());
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
                setStatus(resp, chat::STATUS_FAILURE, *err);
                co_return resp;
            }

            setStatus(resp, chat::STATUS_SUCCESS);
            co_return resp;
        } catch(const std::exception& e) {
            LOG_ERROR << "Register error: " << e.what();
            setStatus(resp, chat::STATUS_FAILURE, std::string("Registration failed: ") + e.what());
            co_return resp;
        }
    }

    static drogon::Task<chat::SendMessageResponse> handleSendMessage(std::shared_ptr<WsData> wsData, const chat::SendMessageRequest& req) {
        chat::SendMessageResponse resp;
        if(!wsData->authenticated) {
            setStatus(resp, chat::STATUS_UNAUTHORIZED, "User not authenticated.");
            co_return resp;
        }
        if(!wsData->currentRoomId.has_value()) {
            setStatus(resp, chat::STATUS_FAILURE, "User is not in any room.");
            co_return resp;
        }
        if(req.message().empty()) {
            setStatus(resp, chat::STATUS_FAILURE, "Empty 'message' field.");
            co_return resp;
        }
        uint32_t room_id = *wsData->currentRoomId;
        std::string username = wsData->username;
        uint64_t time = trantor::Date::now().microSecondsSinceEpoch();

        // Build and broadcast RoomMessage
        chat::Envelope msgEnv;
        auto* msg = msgEnv.mutable_room_message();
        auto* message_info = msg->mutable_message();
        message_info->set_from(username);
        message_info->set_message(req.message());
        message_info->set_timestamp(time);

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
                        m.setRoomId(*wsData->currentRoomId);
                        m.setUserId(wsData->user_id);
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
        for(const auto& user : UserRoomRegistry::instance().getUsersInRoom(room_id)) {
            UserConnectionRegistry::instance().sendToUser(user, msgEnv);
        }

        setStatus(resp, chat::STATUS_SUCCESS);
        co_return resp;
    }

    static drogon::Task<chat::GetUsersResponse> handleGetUsers(std::shared_ptr<WsData> wsData, const chat::GetUsersRequest&) {
        chat::GetUsersResponse resp;
        if(!wsData->authenticated) {
            setStatus(resp, chat::STATUS_UNAUTHORIZED, "Not authenticated");
            co_return resp;
        }
        auto db = drogon::app().getDbClient();
        if(!db) {
            setStatus(resp, chat::STATUS_FAILURE, "Server configuration error: DB not available.");
            co_return resp;
        }
        try {
            auto users = co_await drogon::orm::CoroMapper<models::Users>(db).findAll();
            for(auto& u : users) {
                resp.add_users(u.getValueOfUsername());
            }
            setStatus(resp, chat::STATUS_SUCCESS);
            co_return resp;
        } catch(const std::exception& e) {
            setStatus(resp, chat::STATUS_FAILURE, "Database error while fetching users.");
            co_return resp;
        }
    }

    static drogon::Task<chat::JoinRoomResponse> handleJoinRoom(std::shared_ptr<WsData> wsData, const chat::JoinRoomRequest& req) {
        chat::JoinRoomResponse resp;
        if(!wsData->authenticated) {
            setStatus(resp, chat::STATUS_UNAUTHORIZED, "User not authenticated.");
            co_return resp;
        }
        if(!req.has_room_id()) {
            setStatus(resp, chat::STATUS_FAILURE, "Room id cannot be empty.");
            co_return resp;
        }
        auto db = drogon::app().getDbClient();
        if(!db) {
            setStatus(resp, chat::STATUS_FAILURE, "DB not available.");
            co_return resp;
        }
        try {
            using namespace drogon::orm;
            auto rooms = co_await drogon::orm::CoroMapper<models::Rooms>(db)
                .findBy(Criteria(models::Rooms::Cols::_room_id, CompareOperator::EQ, req.room_id()));
            if(rooms.empty()) {
                setStatus(resp, chat::STATUS_NOT_FOUND, "Room does not exist.");
                co_return resp;
            }
            wsData->currentRoomId = req.room_id();
            UserRoomRegistry::instance().addUserToRoom(wsData->username, req.room_id());
            setStatus(resp, chat::STATUS_SUCCESS);
            co_return resp;
        } catch(const std::exception& e) {
            setStatus(resp, chat::STATUS_FAILURE, "Failed to join room: " + std::string(e.what()));
            co_return resp;
        }
    }

    static drogon::Task<chat::LeaveRoomResponse> handleLeaveRoom(std::shared_ptr<WsData> wsData, const chat::LeaveRoomRequest&) {
        chat::LeaveRoomResponse resp;
        if(!wsData->authenticated) {
            setStatus(resp, chat::STATUS_UNAUTHORIZED, "User not authenticated.");
            co_return resp;
        }
        if(!wsData->currentRoomId) {
            setStatus(resp, chat::STATUS_FAILURE, "User is not in any room.");
            co_return resp;
        }
        uint32_t leftRoomId = *wsData->currentRoomId;
        wsData->currentRoomId.reset();
        UserRoomRegistry::instance().removeUser(wsData->username);
        setStatus(resp, chat::STATUS_SUCCESS);
        co_return resp;
    }

    static drogon::Task<chat::GetRoomsResponse> handleGetRooms(std::shared_ptr<WsData> wsData, const chat::GetRoomsRequest&) {
        chat::GetRoomsResponse resp;
        if(!wsData->authenticated) {
            setStatus(resp, chat::STATUS_UNAUTHORIZED, "User not authenticated.");
            co_return resp;
        }
        auto db = drogon::app().getDbClient();
        if(!db) {
            setStatus(resp, chat::STATUS_FAILURE, "DB not available.");
            co_return resp;
        }
        try {
            auto rooms = co_await drogon::orm::CoroMapper<models::Rooms>(db).findAll();
            for(const auto& room : rooms) {
                chat::RoomInfo* room_info = resp.add_rooms();
                room_info->set_room_id(room.getValueOfRoomId());
                room_info->set_room_name(room.getValueOfRoomName());
            }
            setStatus(resp, chat::STATUS_SUCCESS);
            co_return resp;
        } catch(const std::exception& e) {
            setStatus(resp, chat::STATUS_FAILURE, "Failed to retrieve rooms: " + std::string(e.what()));
            co_return resp;
        }
    }

    static drogon::Task<chat::CreateRoomResponse> handleCreateRoom(std::shared_ptr<WsData> wsData, const chat::CreateRoomRequest& req) {
        chat::CreateRoomResponse resp;
        if(!wsData->authenticated) {
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
            resp.set_room_id(room_id);
            setStatus(resp, chat::STATUS_SUCCESS);
            co_return resp;
        } catch(const std::exception& e) {
            LOG_ERROR << "Create room error: " << e.what();
            setStatus(resp, chat::STATUS_FAILURE, std::string("Create room failed: ") + e.what());
            co_return resp;
        }
    }

    static drogon::Task<chat::GetMessagesResponse> handleGetMessages(std::shared_ptr<WsData> wsData, const chat::GetMessagesRequest& req) {
        chat::GetMessagesResponse resp;
        if(!wsData->authenticated) {
            setStatus(resp, chat::STATUS_UNAUTHORIZED, "User not authenticated.");
            co_return resp;
        }
        if(!wsData->currentRoomId) {
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
            auto messages = co_await mapper
            .orderBy(models::Messages::Cols::_created_at, SortOrder::DESC)
            .limit(req.limit())
            .findBy(Criteria(models::Messages::Cols::_room_id, CompareOperator::EQ, *wsData->currentRoomId));
                for(const auto& message : messages) {
                    chat::MessageInfo* message_info = resp.add_message();
                    message_info->set_message(*message.getMessageText());
                    message_info->set_from(*message.getUser(db).getUsername());
                    message_info->set_timestamp(message.getCreatedAt()->microSecondsSinceEpoch());
            }
            setStatus(resp, chat::STATUS_SUCCESS);
            co_return resp;
        } catch(const std::exception& e) {
            setStatus(resp, chat::STATUS_FAILURE, "Failed to retrieve messages: " + std::string(e.what()));
            co_return resp;
        }
    }
};
