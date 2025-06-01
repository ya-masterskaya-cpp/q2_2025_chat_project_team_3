#pragma once

#include <drogon/drogon.h> //needed only for logging
#include <server/chat/WsData.h>
#include <server/utils/utils.h>    
#include <server/chat/MessageHandlers.h> 

class MessageHandlerService {
public:
    static drogon::Task<Json::Value>
    processMessage(std::shared_ptr<WsData> wsData, Json::Value j_msg, IAuthNotifier& notifier) {
        if(!j_msg.isMember("channel") || !j_msg["channel"].isString() ||
            !j_msg.isMember("type")    || !j_msg["type"].isString()) {
            co_return makeError("Missing or invalid 'channel'/'type' fields");
        }
        auto channel = j_msg["channel"].asString();
        auto typeStr = j_msg["type"].asString();

        if(channel != "client2server") {
             co_return makeError(j_msg, "Invalid message channel: " + channel);
        }

        try {
            if(typeStr == "getUsers") {
                co_return co_await MessageHandlers::handleGetUsers(std::move(wsData), j_msg);
            } else if(typeStr == "register") {
                co_return co_await MessageHandlers::handleRegister(j_msg);
            } else if(typeStr == "createRoom") {
                co_return co_await MessageHandlers::handleCreateRoom(std::move(wsData), j_msg);
            } else if(typeStr == "auth") {
                co_return co_await MessageHandlers::handleAuth(std::move(wsData), j_msg, notifier);
            } else if(typeStr == "getRooms") {
                co_return co_await MessageHandlers::handleGetRooms(std::move(wsData), j_msg);
            } else if(typeStr == "joinRoom") {
                co_return co_await MessageHandlers::handleJoinRoom(std::move(wsData), j_msg);
            } else if(typeStr == "leaveRoom") {
                co_return co_await MessageHandlers::handleLeaveRoom(std::move(wsData), j_msg);
            } else if(typeStr == "sendMessage") {
                co_return co_await MessageHandlers::handleSendMessage(std::move(wsData), j_msg);
            } else {
                co_return makeError(j_msg, "Unknown message type: " + typeStr);
            }
        } catch(const std::exception &e) { 
            std::string type_info = j_msg.isMember("type") ? j_msg["type"].asString() : "unknown_type"; 
            LOG_ERROR << "Unexpected std::exception during message processing (" << type_info << "): " << e.what();
            co_return makeError(std::string("Unexpected server error: ") + e.what());
        }
    }
};
