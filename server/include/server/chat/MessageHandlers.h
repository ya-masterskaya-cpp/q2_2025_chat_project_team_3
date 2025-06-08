#pragma once

#include <drogon/drogon.h>
#include <drogon/orm/CoroMapper.h>

#include <common/proto/chat.pb.h>
#include <server/chat/WsData.h>
#include <server/chat/IRoomService.h>
#include <server/chat/ChatRoomManager.h>
#include <server/utils/utils.h>
#include <server/utils/scoped_coro_transaction.h>

#include <server/models/Users.h>
#include <server/models/Rooms.h>
#include <server/models/Messages.h>

namespace models = drogon_model::drogon_test;

class MessageHandlers {
public:
    static drogon::Task<chat::AuthResponse> handleAuth(const std::shared_ptr<WsData>& wsData, const chat::AuthRequest& req) {
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
            wsData->user = User{*users.front().getUserId(), req.username()};
            setStatus(resp, chat::STATUS_SUCCESS);
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
        int32_t room_id = wsData->room->id;
        std::string& username = wsData->user->name;
        uint64_t time = trantor::Date::now().microSecondsSinceEpoch();

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
        message_info->set_from(username);
        message_info->set_message(req.message());
        message_info->set_timestamp(time);

        ChatRoomManager::instance().sendToRoom(room_id, msgEnv);

        setStatus(resp, chat::STATUS_SUCCESS);
        co_return resp;
    }

    static drogon::Task<chat::JoinRoomResponse> handleJoinRoom(const std::shared_ptr<WsData>& wsData, const chat::JoinRoomRequest& req, IRoomService& room_service) {
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

            setStatus(resp, chat::STATUS_SUCCESS);
            co_return resp;
        } catch(const std::exception& e) {
            setStatus(resp, chat::STATUS_FAILURE, "Failed to join room: " + std::string(e.what()));
            co_return resp;
        }
    }

    static drogon::Task<chat::LeaveRoomResponse> handleLeaveRoom(const std::shared_ptr<WsData>& wsData, const chat::LeaveRoomRequest&, IRoomService& room_service) {
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

    static drogon::Task<chat::GetRoomsResponse> handleGetRooms(const std::shared_ptr<WsData>& wsData, const chat::GetRoomsRequest&) {
        chat::GetRoomsResponse resp;
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
            resp.mutable_info()->set_room_id(room_id);
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
            auto messages = co_await mapper
            .orderBy(models::Messages::Cols::_created_at, SortOrder::DESC)
            .limit(req.limit())
            .findBy(Criteria(models::Messages::Cols::_room_id, CompareOperator::EQ, wsData->room->id));
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
