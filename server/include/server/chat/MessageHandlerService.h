#pragma once

#include <server/chat/WsData.h>
#include <server/utils/utils.h>
#include <server/chat/MessageHandlers.h>
#include <server/chat/IChatRoomService.h>

class MessageHandlerService {
public:
    static drogon::Task<chat::Envelope> processMessage(const std::shared_ptr<WsData>& wsData, const chat::Envelope& env, IChatRoomService& room_service) {
        chat::Envelope respEnv;
        switch(env.payload_case()) {
            case chat::Envelope::kInitialAuthRequest: {
                *respEnv.mutable_initial_auth_response() = co_await MessageHandlers::handleAuthInitial(wsData, env.initial_auth_request());
                break;
            }
            case chat::Envelope::kInitialRegisterRequest: {
                *respEnv.mutable_initial_register_response() = co_await MessageHandlers::handleRegisterInitial(wsData, env.initial_register_request());
                break;
            }
            case chat::Envelope::kAuthRequest: {
                *respEnv.mutable_auth_response() = co_await MessageHandlers::handleAuth(wsData, env.auth_request(), room_service);
                break;
            }
            case chat::Envelope::kRegisterRequest: {
                *respEnv.mutable_register_response() = co_await MessageHandlers::handleRegister(wsData, env.register_request());
                break;
            }
            case chat::Envelope::kSendMessageRequest: {
                *respEnv.mutable_send_message_response() = co_await MessageHandlers::handleSendMessage(wsData, env.send_message_request());
                break;
            }
            case chat::Envelope::kJoinRoomRequest: {
                *respEnv.mutable_join_room_response() = co_await MessageHandlers::handleJoinRoom(wsData, env.join_room_request(), room_service);
                break;
            }
            case chat::Envelope::kLeaveRoomRequest: {
                *respEnv.mutable_leave_room_response() = co_await MessageHandlers::handleLeaveRoom(wsData, env.leave_room_request(), room_service);
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
                break;
            }
            case chat::Envelope::kLogoutRequest: {
                *respEnv.mutable_logout_response() = co_await MessageHandlers::handleLogoutUser(wsData, room_service);
                break;
            }
            default: {
                respEnv = makeGenericErrorEnvelope("Unknown or empty payload");
                break;
            }
        }
        co_return respEnv;
    }
};
