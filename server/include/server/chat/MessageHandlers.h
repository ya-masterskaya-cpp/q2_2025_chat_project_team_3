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

namespace models = drogon_model::drogon_test;

class MessageHandlers {
public:

    static drogon::Task<Json::Value>
    handleAuth(std::shared_ptr<WsData> wsData, Json::Value j, IAuthNotifier &notifier) {
        auto db = drogon::app().getDbClient();
        if(!db) {
            co_return makeError(j, "Server configuration error: DB not available.");
        }

        auto &data = j["data"];
        if(!data.isObject() ||
            !data["username"].isString() ||
            !data["password"].isString()) {
            co_return makeError(j, "Bad auth format.");
        }
        auto username = data["username"].asString();
        auto password = data["password"].asString();
        if(username.empty() || password.empty()) {
            co_return makeError(j, "Empty username or password.");
        }

        try {
            using namespace drogon::orm;
            auto users = co_await CoroMapper<models::Users>(db)
                                .findBy(Criteria(models::Users::Cols::_username, CompareOperator::EQ, username));
            if(users.empty()) {
                co_return makeError(j, "Invalid credentials.");
            }
            auto &user = users.front();
            if(user.getValueOfPassword() != password) {
                co_return makeError(j, "Invalid credentials.");
            }

            wsData->authenticated = true;
            wsData->username = username;
            
            notifier.onUserAuthenticated(username);

            co_return makeOK(j);
        }
        catch(const std::exception &e) {
            LOG_ERROR << "Auth error: " << e.what();
            co_return makeError(j, std::string("Auth failed: ") + e.what());
        }
    }

    static drogon::Task<Json::Value>
    handleSendMessage(std::shared_ptr<WsData> wsData, const Json::Value &j) {
        if(!wsData->authenticated) {
            co_return makeError(j, "User not authenticated.");
        }

        if(!wsData->currentRoom.has_value()) {
            co_return makeError(j, "User is not in any room.");
        }

        if(!j.isMember("data") || !j["data"].isMember("message") || !j["data"]["message"].isString()) {
            co_return makeError(j, "Missing or invalid 'message' field.");
        }

        std::string room = *wsData->currentRoom;
        std::string message = j["data"]["message"].asString();
        std::string username = wsData->username;

        Json::Value msg;
        msg["channel"] = "server2client";
        msg["type"] = "roomMessage";
        msg["data"]["username"] = username;
        msg["data"]["message"] = message;

        for (const auto &user : UserRoomRegistry::instance().getUsersInRoom(room)) {
            //TODO, quick hack for client
            //if(username != user) { //dont resend to ourselves
                UserConnectionRegistry::instance().sendToUser(user, msg);
            //}
        }

        co_return makeOK(j);
    }

    static drogon::Task<Json::Value>
    handleGetUsers(std::shared_ptr<WsData> wsData, const Json::Value &j) {
        if(!wsData->authenticated) {
            co_return makeError(j, "Not authenticated");
        }

        auto db = drogon::app().getDbClient();
        if(!db) {
            co_return makeError(j, "Server configuration error: DB not available.");
        }

        try {
            auto users = co_await drogon::orm::CoroMapper<models::Users>(db).findAll();
            Json::Value resp;
            resp["users"] = Json::arrayValue;
            for (auto &u : users) {
                resp["data"]["users"].append(u.getValueOfUsername());
            }
            co_return makeOK(j, resp);
        }
        catch(const std::exception &e) {
            LOG_ERROR << "GetUsers error: " << e.what();
            co_return makeError(j, "Database error while fetching users.");
        }
    }

    static drogon::Task<Json::Value>
    handleRegister(Json::Value j) {
        auto db = drogon::app().getDbClient();
        if(!db) {
            co_return makeError(j, "Server configuration error: DB not available.");
        }

        auto &data = j["data"];
        if(!data.isObject() ||
            !data["username"].isString() ||
            !data["password"].isString()) {
            co_return makeError(j, "Bad register format.");
        }
        auto username = data["username"].asString();
        auto password = data["password"].asString();
        if(username.empty() || password.empty()) {
            co_return makeError(j, "Empty username or password.");
        }

        try {
            auto err = co_await WithTransaction(
                [&](auto tx) -> drogon::Task<ScopedTransactionResult> {
                    try {
                        models::Users u;
                        u.setUsername(username);
                        u.setPassword(password);
                        co_await drogon::orm::CoroMapper<models::Users>(tx).insert(u);
                        co_return std::nullopt;
                    } catch(const drogon::orm::DrogonDbException &e) {
                        const std::string w = e.base().what();
                        LOG_ERROR << "User insert error: " << w;
                        if(w.find("duplicate key") != std::string::npos ||
                            w.find("UNIQUE constraint failed") != std::string::npos) {
                            co_return "Username already exists.";
                        }
                        co_return "Database error during user insertion.";
                    }
                });

            if(err) {
                co_return makeError(j, *err);
            }

            co_return makeOK(j);
        } catch(const std::exception &e) {
            LOG_ERROR << "Register error: " << e.what();
            co_return makeError(std::string("Registration failed: ") + e.what());
        }
    }

    static drogon::Task<Json::Value>
    handleJoinRoom(std::shared_ptr<WsData> wsData, Json::Value j) {
        if(!wsData->authenticated) {
            co_return makeError(j, "User not authenticated.");
        }

        auto &data = j["data"];
        if(!data.isObject() || !data["room"].isString()) {
            co_return makeError(j, "Missing or invalid 'room' field.");
        }

        auto roomName = data["room"].asString();
        if(roomName.empty()) {
            co_return makeError(j, "Room name cannot be empty.");
        }

        auto db = drogon::app().getDbClient();
        if(!db) {
            co_return makeError(j, "DB not available.");
        }

        try {
            using namespace drogon::orm;
            auto rooms = co_await CoroMapper<models::Rooms>(db)
                                .findBy(Criteria(models::Rooms::Cols::_room_name, CompareOperator::EQ, roomName));
            if(rooms.empty()) {
                co_return makeError(j, "Room does not exist.");
            }

            wsData->currentRoom = roomName;

            UserRoomRegistry::instance().addUserToRoom(wsData->username, roomName);

            co_return makeOK(j);
        } catch(const std::exception &e) {
            co_return makeError(j, "Failed to join room: " + std::string(e.what()));
        }
    }

    static drogon::Task<Json::Value>
    handleLeaveRoom(std::shared_ptr<WsData> wsData, const Json::Value &j) {
        if(!wsData->authenticated) {
            co_return makeError(j, "User not authenticated.");
        }

        if(!wsData->currentRoom) {
            co_return makeError(j, "User is not in any room.");
        }

        std::string leftRoom = *wsData->currentRoom;
        wsData->currentRoom.reset();

        UserRoomRegistry::instance().removeUser(wsData->username);

        co_return makeOK(j);
    }

    static drogon::Task<Json::Value>
    handleGetRooms(std::shared_ptr<WsData> wsData, const Json::Value &j) {
        if(!wsData->authenticated) {
            co_return makeError(j, "User not authenticated.");
        }

        auto db = drogon::app().getDbClient();
        if(!db)
            co_return makeError(j, "DB not available.");

        try {
            auto rooms = co_await drogon::orm::CoroMapper<models::Rooms>(db).findAll();
            Json::Value resp;
            resp["rooms"] = Json::arrayValue;
            for (const auto &room : rooms) {
                resp["rooms"].append(room.getValueOfRoomName());
            }
            co_return makeOK(j, resp);
        } catch(const std::exception &e) {
            co_return makeError(j, "Failed to retrieve rooms: " + std::string(e.what()));
        }
    }

    static drogon::Task<Json::Value>
    handleCreateRoom(std::shared_ptr<WsData> wsData, Json::Value j) {
        if(!wsData->authenticated) {
            co_return makeError(j, "Not authenticated");
        }

        auto db = drogon::app().getDbClient();
        if(!db) {
            co_return makeError(j, "Server configuration error: DB not available.");
        }

        auto &data = j["data"];
        if(!data.isObject() || !data["roomName"].isString()) {
            co_return makeError(j, "Bad createRoom format.");
        }
        auto roomName = data["roomName"].asString();
        if(roomName.empty()) {
            co_return makeError(j, "Empty room name.");
        }

        try {
            auto err = co_await WithTransaction(
                [&](auto tx) -> drogon::Task<ScopedTransactionResult> {
                    try {
                        models::Rooms r;
                        r.setRoomName(roomName);
                        co_await drogon::orm::CoroMapper<models::Rooms>(tx).insert(r);
                        co_return std::nullopt;
                    }
                    catch(const drogon::orm::DrogonDbException &e) {
                        const std::string w = e.base().what();
                        LOG_ERROR << "Room insert error: " << w;
                        if(w.find("duplicate key") != std::string::npos ||
                            w.find("UNIQUE constraint failed") != std::string::npos) {
                            co_return "Room name already exists.";
                        }
                        co_return "Database error during room creation.";
                    }
                });

            if(err) {
                co_return makeError(j, *err);
            }

            co_return makeOK(j);
        } catch(const std::exception &e) {
            LOG_ERROR << "CreateRoom error: " << e.what();
            co_return makeError(std::string("Create room failed: ") + e.what());
        }
    }
};
