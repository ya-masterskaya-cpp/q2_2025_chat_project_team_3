#include <server/chat/MessageHandlerService.h>
#include <server/chat/MessageHandlers.h>
#include <common/utils/utils.h>

namespace server {

MessageHandlerService::MessageHandlerService(std::unique_ptr<MessageHandlers> handlers)
    : m_handlers(std::move(handlers)) {}

MessageHandlerService::~MessageHandlerService() = default;

drogon::Task<chat::Envelope> MessageHandlerService::processMessage(const WsDataPtr& wsData, const chat::Envelope& env, IChatRoomService& room_service) const {
    chat::Envelope respEnv;
    switch(env.payload_case()) {
        case chat::Envelope::kInitialAuthRequest: {
            *respEnv.mutable_initial_auth_response() = co_await m_handlers->handleAuthInitial(wsData, env.initial_auth_request());
            break;
        }
        case chat::Envelope::kInitialRegisterRequest: {
            *respEnv.mutable_initial_register_response() = co_await m_handlers->handleRegisterInitial(wsData, env.initial_register_request());
            break;
        }
        case chat::Envelope::kAuthRequest: {
            *respEnv.mutable_auth_response() = co_await m_handlers->handleAuth(wsData, env.auth_request(), room_service);
            break;
        }
        case chat::Envelope::kRegisterRequest: {
            *respEnv.mutable_register_response() = co_await m_handlers->handleRegister(wsData, env.register_request());
            break;
        }
        case chat::Envelope::kSendMessageRequest: {
            *respEnv.mutable_send_message_response() = co_await m_handlers->handleSendMessage(wsData, env.send_message_request(), room_service);
            break;
        }
        case chat::Envelope::kJoinRoomRequest: {
            *respEnv.mutable_join_room_response() = co_await m_handlers->handleJoinRoom(wsData, env.join_room_request(), room_service);
            break;
        }
        case chat::Envelope::kLeaveRoomRequest: {
            *respEnv.mutable_leave_room_response() = co_await m_handlers->handleLeaveRoom(wsData, env.leave_room_request(), room_service);
            break;
        }
        case chat::Envelope::kCreateRoomRequest: {
            *respEnv.mutable_create_room_response() = co_await m_handlers->handleCreateRoom(wsData, env.create_room_request(), room_service);
            break;
        }
        case chat::Envelope::kGetMessagesRequest: {
            *respEnv.mutable_get_messages_response() = co_await m_handlers->handleGetMessages(wsData, env.get_messages_request());
               break;
        }
        case chat::Envelope::kLogoutRequest: {
            *respEnv.mutable_logout_response() = co_await m_handlers->handleLogoutUser(wsData, room_service);
            break;
        }
        case chat::Envelope::kRenameRoomRequest: {
            *respEnv.mutable_rename_room_response() = co_await m_handlers->MessageHandlers::handleRenameRoom(wsData, env.rename_room_request(), room_service);
            break;
        }
        case chat::Envelope::kDeleteRoomRequest: {
            *respEnv.mutable_delete_room_response() = co_await m_handlers->MessageHandlers::handleDeleteRoom(wsData, env.delete_room_request(), room_service);
            break;
        }
        case chat::Envelope::kAssignRoleRequest: {
            *respEnv.mutable_assign_role_response() = co_await m_handlers->handleAssignRole(wsData, env.assign_role_request(), room_service);
            break;
        }
        default: {
            respEnv = common::makeGenericErrorEnvelope("Unknown or empty payload");
            break;
        }
    }
    co_return respEnv;
}

} // namespace server
