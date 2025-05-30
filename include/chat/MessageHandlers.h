#pragma once

#include <utils/utils.h>
#include <utils/scoped_coro_transaction.h>
#include <chat/WsData.h>
#include <chat/UserConnectionRegistry.h>
#include <chat/IAuthNotifier.h>
#include <chat/UserRoomRegistry.h>

#include <drogon/drogon.h> //needed for logging
#include <drogon/orm/CoroMapper.h> //needed for ORM

#include <models/Users.h>
#include <models/Rooms.h>

namespace models = drogon_model::drogon_test;

class MessageHandlers {
public:

    static drogon::Task<Json::Value>
    handleAuth(std::shared_ptr<WsData> wsData, Json::Value j, IAuthNotifier &notifier) {
        auto db = drogon::app().getDbClient();
        if (!db) {
            co_return makeError("Server configuration error: DB not available.");
        }

        auto &data = j["data"];
        if (!data.isObject() ||
            !data["username"].isString() ||
            !data["password"].isString()) {
            co_return makeError("Bad auth format.");
        }
        auto username = data["username"].asString();
        auto password = data["password"].asString();
        if (username.empty() || password.empty()) {
            co_return makeError("Empty username or password.");
        }

        try {
            using namespace drogon::orm;
            auto users = co_await CoroMapper<models::Users>(db)
                                .findBy(Criteria(models::Users::Cols::_username, CompareOperator::EQ, username));
            if (users.empty()) {
                co_return makeError("Invalid credentials.");
            }
            auto &user = users.front();
            if (user.getValueOfPassword() != password) {
                co_return makeError("Invalid credentials.");
            }

            wsData->authenticated = true;
            wsData->username = username;
            
            notifier.onUserAuthenticated(username);

            co_return makeOK();
        }
        catch (const std::exception &e) {
            LOG_ERROR << "Auth error: " << e.what();
            co_return makeError(std::string("Auth failed: ") + e.what());
        }
    }

    static drogon::Task<Json::Value> handleSendMessage(std::shared_ptr<WsData> wsData, const Json::Value &j_msg) {
        if (!wsData->authenticated) {
            co_return makeError("User not authenticated.");
        }

        if (!wsData->currentRoom.has_value()) {
            co_return makeError("User is not in any room.");
        }

        if (!j_msg.isMember("data") || !j_msg["data"].isMember("message") || !j_msg["data"]["message"].isString()) {
            co_return makeError("Missing or invalid 'message' field.");
        }

        std::string room = *wsData->currentRoom;
        std::string message = j_msg["data"]["message"].asString();
        std::string username = wsData->username;

        Json::Value msg;
        msg["channel"] = "server2client";
        msg["type"] = "roomMessage";
        msg["data"]["username"] = username;
        msg["data"]["message"] = message;

        for (const auto &user : UserRoomRegistry::instance().getUsersInRoom(room)) {
            if(username != user) { //dont resend to ourselves
                UserConnectionRegistry::instance().sendToUser(user, msg);
            }
        }

        co_return makeOK();
    }



    static drogon::Task<Json::Value>
    handleGetUsers(std::shared_ptr<WsData> wsData) {
        if (!wsData->authenticated)
            co_return makeError("Not authenticated");

        auto db = drogon::app().getDbClient();
        if (!db)
            co_return makeError("Server configuration error: DB not available.");

        try {
            auto users = co_await drogon::orm::CoroMapper<models::Users>(db).findAll();
            Json::Value resp;
            resp["channel"] = "server2client";
            resp["type"]    = "getUsers";
            resp["data"]["users"] = Json::arrayValue;
            for (auto &u : users) {
                resp["data"]["users"].append(u.getValueOfUsername());
            }
            co_return resp;
        }
        catch (const std::exception &e) {
            LOG_ERROR << "GetUsers error: " << e.what();
            co_return makeError("Database error while fetching users.");
        }
    }

    static drogon::Task<Json::Value>
    handleRegister(Json::Value j) {
        auto db = drogon::app().getDbClient();
        if (!db)
            co_return makeError("Server configuration error: DB not available.");

        auto &data = j["data"];
        if (!data.isObject() ||
            !data["username"].isString() ||
            !data["password"].isString()) {
            co_return makeError("Bad register format.");
        }
        auto username = data["username"].asString();
        auto password = data["password"].asString();
        if (username.empty() || password.empty()) {
            co_return makeError("Empty username or password.");
        }

        try {
            auto err = co_await WithTransaction(
                [username, password](auto tx) -> drogon::Task<std::optional<Json::Value>> {
                    try {
                        models::Users u;
                        u.setUsername(username);
                        u.setPassword(password);
                        co_await drogon::orm::CoroMapper<models::Users>(tx).insert(u);
                        co_return std::nullopt;
                    }
                    catch (const drogon::orm::DrogonDbException &e) {
                        const std::string w = e.base().what();
                        LOG_ERROR << "User insert error: " << w;
                        if (w.find("duplicate key") != std::string::npos ||
                            w.find("UNIQUE constraint failed") != std::string::npos) {
                            co_return makeError("Username already exists.");
                        }
                        co_return makeError("Database error during user insertion.");
                    }
                });

            if (err) {
                co_return *err;
            }

            co_return makeOK();
        }
        catch (const std::exception &e) {
            LOG_ERROR << "Register error: " << e.what();
            co_return makeError(std::string("Registration failed: ") + e.what());
        }
    }

    static drogon::Task<Json::Value>
    handleJoinRoom(std::shared_ptr<WsData> wsData, Json::Value j) {
        if (!wsData->authenticated) {
            co_return makeError("User not authenticated.");
        }

        auto &data = j["data"];
        if (!data.isObject() || !data["room"].isString()) {
            co_return makeError("Missing or invalid 'room' field.");
        }

        auto roomName = data["room"].asString();
        if (roomName.empty()) {
            co_return makeError("Room name cannot be empty.");
        }

        auto db = drogon::app().getDbClient();
        if (!db) {
            co_return makeError("DB not available.");
        }

        try {
            using namespace drogon::orm;
            auto rooms = co_await CoroMapper<models::Rooms>(db)
                                .findBy(Criteria(models::Rooms::Cols::_room_name, CompareOperator::EQ, roomName));
            if (rooms.empty()) {
                co_return makeError("Room does not exist.");
            }

            wsData->currentRoom = roomName;

            UserRoomRegistry::instance().addUserToRoom(wsData->username, roomName);

            co_return makeOK();
        } catch (const std::exception &e) {
            co_return makeError("Failed to join room: " + std::string(e.what()));
        }
    }

    static drogon::Task<Json::Value>
    handleLeaveRoom(std::shared_ptr<WsData> wsData) {
        if (!wsData->authenticated) {
            co_return makeError("User not authenticated.");
        }

        if (!wsData->currentRoom) {
            co_return makeError("User is not in any room.");
        }

        std::string leftRoom = *wsData->currentRoom;
        wsData->currentRoom.reset();

        UserRoomRegistry::instance().removeUser(wsData->username);

        co_return makeOK();
    }

    static drogon::Task<Json::Value>
    handleGetRooms(std::shared_ptr<WsData> wsData) {
        if (!wsData->authenticated) {
            co_return makeError("User not authenticated.");
        }

        auto db = drogon::app().getDbClient();
        if (!db)
            co_return makeError("DB not available.");

        try {
            auto rooms = co_await drogon::orm::CoroMapper<models::Rooms>(db).findAll();
            Json::Value resp;
            resp["channel"] = "server2client";
            resp["type"]    = "getRooms";
            resp["data"]["rooms"] = Json::arrayValue;
            for (const auto &room : rooms) {
                resp["data"]["rooms"].append(room.getValueOfRoomName());
            }
            co_return resp;
        } catch (const std::exception &e) {
            co_return makeError("Failed to retrieve rooms: " + std::string(e.what()));
        }
    }

    static drogon::Task<Json::Value>
    handleCreateRoom(std::shared_ptr<WsData> wsData, Json::Value j) {
        if (!wsData->authenticated)
            co_return makeError("Not authenticated");

        auto db = drogon::app().getDbClient();
        if (!db) {
            co_return makeError("Server configuration error: DB not available.");
        }

        auto &data = j["data"];
        if (!data.isObject() || !data["roomName"].isString()) {
            co_return makeError("Bad createRoom format.");
        }
        auto roomName = data["roomName"].asString();
        if (roomName.empty()) {
            co_return makeError("Empty room name.");
        }

        try {
            auto err = co_await WithTransaction(
                [roomName](auto tx) -> drogon::Task<std::optional<Json::Value>> {
                    try {
                        models::Rooms r;
                        r.setRoomName(roomName);
                        co_await drogon::orm::CoroMapper<models::Rooms>(tx).insert(r);
                        co_return std::nullopt;
                    }
                    catch (const drogon::orm::DrogonDbException &e) {
                        const std::string w = e.base().what();
                        LOG_ERROR << "Room insert error: " << w;
                        if (w.find("duplicate key") != std::string::npos ||
                            w.find("UNIQUE constraint failed") != std::string::npos) {
                            co_return makeError("Room name already exists.");
                        }
                        co_return makeError("Database error during room creation.");
                    }
                });

            if (err) {
                co_return *err;
            }

            co_return makeOK();
        }
        catch (const std::exception &e) {
            LOG_ERROR << "CreateRoom error: " << e.what();
            co_return makeError(std::string("Create room failed: ") + e.what());
        }
    }
};
