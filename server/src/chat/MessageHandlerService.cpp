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
        case chat::Envelope::kDeleteMessageRequest: {
            *respEnv.mutable_delete_message_response() = co_await m_handlers->handleDeleteMessage(wsData, env.delete_message_request(), room_service);
            break;
        }
        case chat::Envelope::kUserTypingStartRequest: {
            *respEnv.mutable_user_typing_start_response() = co_await m_handlers->handleUserTypingStart(wsData, room_service);
            break;
        }
        case chat::Envelope::kUserTypingStopRequest: {
            *respEnv.mutable_user_typing_stop_response() = co_await m_handlers->handleUserTypingStop(wsData, room_service);
            break;
		}
        case chat::Envelope::kBecomeMemberRequest: {
            *respEnv.mutable_become_member_response() = co_await m_handlers->handleBecomeMember(wsData, env.become_member_request());
            break;
		}
        case chat::Envelope::kChangeUsernameRequest: {
            *respEnv.mutable_change_username_response() = co_await m_handlers->handleChangeUsername(wsData, env.change_username_request(), room_service);
            break;
        }
        case chat::Envelope::kGetMySaltRequest: {
            *respEnv.mutable_get_my_salt_response() = co_await m_handlers->handleGetSalt(wsData);
            break;
        }
        case chat::Envelope::kChangePasswordRequest: {
            *respEnv.mutable_change_password_response() = co_await m_handlers->handleChangePassword(wsData, env.change_password_request());
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
