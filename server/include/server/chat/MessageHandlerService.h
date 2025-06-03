#pragma once

#include <drogon/drogon.h> // needed only for logging
#include <server/chat/WsData.h>
#include <server/utils/utils.h>
#include <server/chat/MessageHandlers.h>
#include <common/proto/chat.pb.h>

class MessageHandlerService {
public:
    static drogon::Task<chat::Envelope> processMessage(std::shared_ptr<WsData> wsData, const chat::Envelope& env, IAuthNotifier& notifier) {
        chat::Envelope respEnv;
        switch(env.payload_case()) {
            case chat::Envelope::kAuthRequest: {
                *respEnv.mutable_auth_response() = co_await MessageHandlers::handleAuth(wsData, env.auth_request(), notifier);
                break;
            }
            case chat::Envelope::kRegisterRequest: {
                *respEnv.mutable_register_response() = co_await MessageHandlers::handleRegister(env.register_request());
                break;
            }
            case chat::Envelope::kSendMessageRequest: {
                *respEnv.mutable_send_message_response() = co_await MessageHandlers::handleSendMessage(wsData, env.send_message_request());
                break;
            }
            case chat::Envelope::kGetUsersRequest: {
                *respEnv.mutable_get_users_response() = co_await MessageHandlers::handleGetUsers(wsData, env.get_users_request());
                break;
            }
            case chat::Envelope::kJoinRoomRequest: {
                *respEnv.mutable_join_room_response() = co_await MessageHandlers::handleJoinRoom(wsData, env.join_room_request());
                break;
            }
            case chat::Envelope::kLeaveRoomRequest: {
                *respEnv.mutable_leave_room_response() = co_await MessageHandlers::handleLeaveRoom(wsData, env.leave_room_request());
                break;
            }
            case chat::Envelope::kGetRoomsRequest: {
                *respEnv.mutable_get_rooms_response() = co_await MessageHandlers::handleGetRooms(wsData, env.get_rooms_request());
                break;
            }
            case chat::Envelope::kCreateRoomRequest: {
                *respEnv.mutable_create_room_response() = co_await MessageHandlers::handleCreateRoom(wsData, env.create_room_request());
                break;
            }
            case chat::Envelope::kGetMessagesRequest: {
                *respEnv.mutable_get_messages_response() = co_await MessageHandlers::handleGetMessages(wsData, env.get_messages_request());
            }
            default: {
                respEnv = makeGenericErrorEnvelope("Unknown or empty payload");
                break;
            }
        }
        co_return respEnv;
    }
};
