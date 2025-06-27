#pragma once

#include <drogon/orm/DbClient.h>
#include <server/chat/WsData.h>

namespace drogon_model {
namespace drogon_test {
    class Rooms;
}
}

namespace server {

class IChatRoomService;

class MessageHandlers {
public:
    explicit MessageHandlers(drogon::orm::DbClientPtr dbClient);

    drogon::Task<chat::InitialAuthResponse> handleAuthInitial(const WsDataPtr& wsDataGuarded, const chat::InitialAuthRequest& req) const;
    drogon::Task<chat::InitialRegisterResponse> handleRegisterInitial(const WsDataPtr& wsDataGuarded, const chat::InitialRegisterRequest& req) const;
    drogon::Task<chat::AuthResponse> handleAuth(const WsDataPtr& wsDataGuarded, const chat::AuthRequest& req, IChatRoomService& room_service) const;
    drogon::Task<chat::RegisterResponse> handleRegister(const WsDataPtr& wsDataGuarded, const chat::RegisterRequest& req) const;
    drogon::Task<chat::SendMessageResponse> handleSendMessage(const WsDataPtr& wsDataGuarded, const chat::SendMessageRequest& req, IChatRoomService& room_service) const;
    drogon::Task<chat::JoinRoomResponse> handleJoinRoom(const WsDataPtr& wsDataGuarded, const chat::JoinRoomRequest& req, IChatRoomService& room_service) const;
    drogon::Task<chat::LeaveRoomResponse> handleLeaveRoom(const WsDataPtr& wsDataGuarded, const chat::LeaveRoomRequest&, IChatRoomService& room_service) const;
    drogon::Task<chat::CreateRoomResponse> handleCreateRoom(const WsDataPtr& wsDataGuarded, const chat::CreateRoomRequest& req, IChatRoomService& room_service) const;
    drogon::Task<chat::GetMessagesResponse> handleGetMessages(const WsDataPtr& wsDataGuarded, const chat::GetMessagesRequest& req) const;
    drogon::Task<chat::LogoutResponse> handleLogoutUser(const WsDataPtr& wsDataGuarded, IChatRoomService& room_service) const;
    drogon::Task<chat::RenameRoomResponse> handleRenameRoom(const WsDataPtr& wsDataGuarded, const chat::RenameRoomRequest& req, IChatRoomService& room_service);
    drogon::Task<chat::DeleteRoomResponse> handleDeleteRoom(const WsDataPtr& wsDataGuarded, const chat::DeleteRoomRequest& req, IChatRoomService& room_service);
private:

    std::optional<std::string> validateUtf8String(const std::string_view& textToValidate, size_t maxLength, const std::string_view& fieldName) const;
    drogon::Task<std::optional<chat::UserRights>> getUserRights(int32_t user_id, int32_t room_id) const;
    drogon::Task<std::optional<chat::UserRights>> getUserRights(int32_t user_id, int32_t room_id, const drogon_model::drogon_test::Rooms& room) const;
    drogon::Task<std::optional<chat::UserRights>> findStoredUserRole(int32_t user_id, int32_t room_id) const;

    drogon::orm::DbClientPtr m_dbClient;

};

} // namespace server
