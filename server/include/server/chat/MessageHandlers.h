#pragma once

#include <drogon/orm/DbClient.h>
#include <server/chat/WsData.h>

class IChatRoomService;

namespace drogon_model {
namespace drogon_test {
    class Rooms;
}
}

class MessageHandlers {
public:
    explicit MessageHandlers(drogon::orm::DbClientPtr dbClient);

    drogon::Task<chat::InitialAuthResponse> handleAuthInitial(const WsDataPtr& wsDataGuarded, const chat::InitialAuthRequest& req) const;
    drogon::Task<chat::InitialRegisterResponse> handleRegisterInitial(const WsDataPtr& wsDataGuarded, const chat::InitialRegisterRequest& req) const;
    drogon::Task<chat::AuthResponse> handleAuth(const WsDataPtr& wsDataGuarded, const chat::AuthRequest& req, IChatRoomService& room_service) const;
    drogon::Task<chat::RegisterResponse> handleRegister(const WsDataPtr& wsDataGuarded, const chat::RegisterRequest& req) const;
    drogon::Task<chat::SendMessageResponse> handleSendMessage(const WsDataPtr& wsDataGuarded, const chat::SendMessageRequest& req) const;
    drogon::Task<chat::JoinRoomResponse> handleJoinRoom(const WsDataPtr& wsDataGuarded, const chat::JoinRoomRequest& req, IChatRoomService& room_service) const;
    drogon::Task<chat::LeaveRoomResponse> handleLeaveRoom(const WsDataPtr& wsDataGuarded, const chat::LeaveRoomRequest&, IChatRoomService& room_service) const;
    drogon::Task<chat::CreateRoomResponse> handleCreateRoom(const WsDataPtr& wsDataGuarded, const chat::CreateRoomRequest& req) const;
    drogon::Task<chat::GetMessagesResponse> handleGetMessages(const WsDataPtr& wsDataGuarded, const chat::GetMessagesRequest& req) const;
    drogon::Task<chat::LogoutResponse> handleLogoutUser(const WsDataPtr& wsDataGuarded, IChatRoomService& room_service) const;
    drogon::Task<chat::RenameRoomResponse> handleRenameRoom(const WsDataPtr& wsDataGuarded, const chat::RenameRoomRequest& req);
    drogon::Task<chat::DeleteRoomResponse> handleDeleteRoom(const WsDataPtr& wsDataGuarded, const chat::DeleteRoomRequest& req);
private:

    std::optional<std::string> validateUtf8String(const std::string_view& textToValidate, size_t maxLength, const std::string_view& fieldName) const;
    drogon::Task<std::optional<chat::UserRights>> getUserRights(uint32_t user_id, uint32_t room_id) const;
    drogon::Task<std::optional<chat::UserRights>> getUserRights(uint32_t user_id, uint32_t room_id, const drogon_model::drogon_test::Rooms& room) const;
    drogon::Task<std::optional<chat::UserRights>> findStoredUserRole(uint32_t user_id, uint32_t room_id) const;

    drogon::orm::DbClientPtr m_dbClient;

};
