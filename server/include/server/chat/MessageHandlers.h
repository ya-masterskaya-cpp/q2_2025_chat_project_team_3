#pragma once

#include <server/utils/utils.h>
#include <server/utils/scoped_coro_transaction.h>
#include <server/chat/WsData.h>
#include <server/chat/UserConnectionRegistry.h>
#include <server/chat/IAuthNotifier.h>
#include <server/chat/UserRoomRegistry.h>

#include <drogon/drogon.h> //needed for logging
#include <drogon/orm/CoroMapper.h> //needed for ORM

#include <server/models/Users.h>
#include <server/models/Rooms.h>

#include <common/proto/chat.pb.h>

namespace models = drogon_model::drogon_test;

class MessageHandlers {
public:
    static drogon::Task<chat::AuthResponse>
    handleAuth(std::shared_ptr<WsData> wsData, const chat::AuthRequest& req, IAuthNotifier& notifier) {
        chat::AuthResponse resp;
        auto db = drogon::app().getDbClient();
        if (!db) {
            setStatus(resp, chat::STATUS_FAILURE, "Server configuration error: DB not available.");
            co_return resp;
        }
        if (req.username().empty() || req.password().empty()) {
            setStatus(resp, chat::STATUS_FAILURE, "Empty username or password.");
            co_return resp;
        }
        try {
            using namespace drogon::orm;
            auto users = co_await drogon::orm::CoroMapper<models::Users>(db)
                .findBy(Criteria(models::Users::Cols::_username, CompareOperator::EQ, req.username()));
            if (users.empty() || users.front().getValueOfPassword() != req.password()) {
                setStatus(resp, chat::STATUS_UNAUTHORIZED, "Invalid credentials.");
                co_return resp;
            }
            wsData->authenticated = true;
            wsData->username = req.username();
            notifier.onUserAuthenticated(req.username());
            setStatus(resp, chat::STATUS_SUCCESS, "OK");
            resp.set_token("dummy_token");
            co_return resp;
        } catch (const std::exception &e) {
            setStatus(resp, chat::STATUS_FAILURE, std::string("Auth failed: ") + e.what());
            co_return resp;
        }
    }

    static drogon::Task<chat::RegisterResponse>
    handleRegister(const chat::RegisterRequest& req) {
        chat::RegisterResponse resp;
        auto db = drogon::app().getDbClient();
        if (!db) {
            setStatus(resp, chat::STATUS_FAILURE, "Server configuration error: DB not available.");
            co_return resp;
        }
        if (req.username().empty() || req.password().empty()) {
            setStatus(resp, chat::STATUS_FAILURE, "Empty username or password.");
            co_return resp;
        }
        try {
            models::Users u;
            u.setUsername(req.username());
            u.setPassword(req.password());
            co_await drogon::orm::CoroMapper<models::Users>(db).insert(u);
            setStatus(resp, chat::STATUS_SUCCESS, "Registered!");
            co_return resp;
        } catch (const drogon::orm::DrogonDbException &e) {
            const std::string w = e.base().what();
            if (w.find("duplicate key") != std::string::npos || w.find("UNIQUE constraint failed") != std::string::npos) {
                setStatus(resp, chat::STATUS_FAILURE, "Username already exists.");
            } else {
                setStatus(resp, chat::STATUS_FAILURE, "Database error during user insertion.");
            }
            co_return resp;
        } catch (const std::exception &e) {
            setStatus(resp, chat::STATUS_FAILURE, std::string("Registration failed: ") + e.what());
            co_return resp;
        }
    }

    static drogon::Task<chat::SendMessageResponse>
    handleSendMessage(std::shared_ptr<WsData> wsData, const chat::SendMessageRequest& req) {
        chat::SendMessageResponse resp;
        if (!wsData->authenticated) {
            setStatus(resp, chat::STATUS_UNAUTHORIZED, "User not authenticated.");
            co_return resp;
        }
        if (!wsData->currentRoom.has_value()) {
            setStatus(resp, chat::STATUS_FAILURE, "User is not in any room.");
            co_return resp;
        }
        if (req.message().empty()) {
            setStatus(resp, chat::STATUS_FAILURE, "Missing or empty 'message' field.");
            co_return resp;
        }
        std::string room = *wsData->currentRoom;
        std::string username = wsData->username;

        // Build and broadcast RoomMessage
        chat::Envelope msgEnv;
        auto* msg = msgEnv.mutable_room_message();
        msg->set_username(username);
        msg->set_message(req.message());

        for (const auto& user : UserRoomRegistry::instance().getUsersInRoom(room)) {
            UserConnectionRegistry::instance().sendToUser(user, msgEnv);
        }

        setStatus(resp, chat::STATUS_SUCCESS, "Message sent");
        co_return resp;
    }

    static drogon::Task<chat::GetUsersResponse>
    handleGetUsers(std::shared_ptr<WsData> wsData, const chat::GetUsersRequest&) {
        chat::GetUsersResponse resp;
        if (!wsData->authenticated) {
            setStatus(resp, chat::STATUS_UNAUTHORIZED, "Not authenticated");
            co_return resp;
        }
        auto db = drogon::app().getDbClient();
        if (!db) {
            setStatus(resp, chat::STATUS_FAILURE, "Server configuration error: DB not available.");
            co_return resp;
        }
        try {
            auto users = co_await drogon::orm::CoroMapper<models::Users>(db).findAll();
            for (auto& u : users) {
                resp.add_users(u.getValueOfUsername());
            }
            setStatus(resp, chat::STATUS_SUCCESS, "Fetched users");
            co_return resp;
        } catch (const std::exception &e) {
            setStatus(resp, chat::STATUS_FAILURE, "Database error while fetching users.");
            co_return resp;
        }
    }

    static drogon::Task<chat::JoinRoomResponse>
    handleJoinRoom(std::shared_ptr<WsData> wsData, const chat::JoinRoomRequest& req) {
        chat::JoinRoomResponse resp;
        if (!wsData->authenticated) {
            setStatus(resp, chat::STATUS_UNAUTHORIZED, "User not authenticated.");
            co_return resp;
        }
        if (req.room().empty()) {
            setStatus(resp, chat::STATUS_FAILURE, "Room name cannot be empty.");
            co_return resp;
        }
        auto db = drogon::app().getDbClient();
        if (!db) {
            setStatus(resp, chat::STATUS_FAILURE, "DB not available.");
            co_return resp;
        }
        try {
            using namespace drogon::orm;
            auto rooms = co_await drogon::orm::CoroMapper<models::Rooms>(db)
                .findBy(Criteria(models::Rooms::Cols::_room_name, CompareOperator::EQ, req.room()));
            if (rooms.empty()) {
                setStatus(resp, chat::STATUS_NOT_FOUND, "Room does not exist.");
                co_return resp;
            }
            wsData->currentRoom = req.room();
            UserRoomRegistry::instance().addUserToRoom(wsData->username, req.room());
            setStatus(resp, chat::STATUS_SUCCESS, "Joined room");
            co_return resp;
        } catch (const std::exception &e) {
            setStatus(resp, chat::STATUS_FAILURE, "Failed to join room: " + std::string(e.what()));
            co_return resp;
        }
    }

    static drogon::Task<chat::LeaveRoomResponse>
    handleLeaveRoom(std::shared_ptr<WsData> wsData, const chat::LeaveRoomRequest&) {
        chat::LeaveRoomResponse resp;
        if (!wsData->authenticated) {
            setStatus(resp, chat::STATUS_UNAUTHORIZED, "User not authenticated.");
            co_return resp;
        }
        if (!wsData->currentRoom) {
            setStatus(resp, chat::STATUS_FAILURE, "User is not in any room.");
            co_return resp;
        }
        std::string leftRoom = *wsData->currentRoom;
        wsData->currentRoom.reset();
        UserRoomRegistry::instance().removeUser(wsData->username);
        setStatus(resp, chat::STATUS_SUCCESS, "Left room");
        co_return resp;
    }

    static drogon::Task<chat::GetRoomsResponse>
    handleGetRooms(std::shared_ptr<WsData> wsData, const chat::GetRoomsRequest&) {
        chat::GetRoomsResponse resp;
        if (!wsData->authenticated) {
            setStatus(resp, chat::STATUS_UNAUTHORIZED, "User not authenticated.");
            co_return resp;
        }
        auto db = drogon::app().getDbClient();
        if (!db) {
            setStatus(resp, chat::STATUS_FAILURE, "DB not available.");
            co_return resp;
        }
        try {
            auto rooms = co_await drogon::orm::CoroMapper<models::Rooms>(db).findAll();
            for (const auto& room : rooms) {
                resp.add_rooms(room.getValueOfRoomName());
            }
            setStatus(resp, chat::STATUS_SUCCESS, "Fetched rooms");
            co_return resp;
        } catch (const std::exception &e) {
            setStatus(resp, chat::STATUS_FAILURE, "Failed to retrieve rooms: " + std::string(e.what()));
            co_return resp;
        }
    }

    static drogon::Task<chat::CreateRoomResponse>
    handleCreateRoom(std::shared_ptr<WsData> wsData, const chat::CreateRoomRequest& req) {
        chat::CreateRoomResponse resp;
        if (!wsData->authenticated) {
            setStatus(resp, chat::STATUS_UNAUTHORIZED, "Not authenticated");
            co_return resp;
        }
        if (req.room_name().empty()) {
            setStatus(resp, chat::STATUS_FAILURE, "Empty room name.");
            co_return resp;
        }
        auto db = drogon::app().getDbClient();
        if (!db) {
            setStatus(resp, chat::STATUS_FAILURE, "Server configuration error: DB not available.");
            co_return resp;
        }
        try {
            models::Rooms r;
            r.setRoomName(req.room_name());
            co_await drogon::orm::CoroMapper<models::Rooms>(db).insert(r);
            setStatus(resp, chat::STATUS_SUCCESS, "Created room");
            co_return resp;
        } catch(const drogon::orm::DrogonDbException &e) {
            const std::string w = e.base().what();
            if (w.find("duplicate key") != std::string::npos || w.find("UNIQUE constraint failed") != std::string::npos) {
                setStatus(resp, chat::STATUS_FAILURE, "Room name already exists.");
            } else {
                setStatus(resp, chat::STATUS_FAILURE, "Database error during room creation.");
            }
            co_return resp;
        } catch(const std::exception &e) {
            setStatus(resp, chat::STATUS_FAILURE, std::string("Create room failed: ") + e.what());
            co_return resp;
        }
    }
};
