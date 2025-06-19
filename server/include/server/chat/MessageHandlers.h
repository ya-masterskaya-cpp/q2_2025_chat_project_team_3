#pragma once

#include <drogon/orm/DbClient.h>

class IChatRoomService;
struct WsData;

class MessageHandlers {
public:
    explicit MessageHandlers(drogon::orm::DbClientPtr dbClient);

    drogon::Task<chat::InitialAuthResponse> handleAuthInitial(const std::shared_ptr<WsData>& wsData, const chat::InitialAuthRequest& req) const;
    drogon::Task<chat::InitialRegisterResponse> handleRegisterInitial(const std::shared_ptr<WsData>& wsData, const chat::InitialRegisterRequest& req) const;
    drogon::Task<chat::AuthResponse> handleAuth(const std::shared_ptr<WsData>& wsData, const chat::AuthRequest& req, IChatRoomService& room_service) const;
    drogon::Task<chat::RegisterResponse> handleRegister(const std::shared_ptr<WsData>& wsData, const chat::RegisterRequest& req) const;
    drogon::Task<chat::SendMessageResponse> handleSendMessage(const std::shared_ptr<WsData>& wsData, const chat::SendMessageRequest& req) const;
    drogon::Task<chat::JoinRoomResponse> handleJoinRoom(const std::shared_ptr<WsData>& wsData, const chat::JoinRoomRequest& req, IChatRoomService& room_service) const;
    drogon::Task<chat::LeaveRoomResponse> handleLeaveRoom(const std::shared_ptr<WsData>& wsData, const chat::LeaveRoomRequest&, IChatRoomService& room_service) const;
    drogon::Task<chat::CreateRoomResponse> handleCreateRoom(const std::shared_ptr<WsData>& wsData, const chat::CreateRoomRequest& req) const;
    drogon::Task<chat::GetMessagesResponse> handleGetMessages(const std::shared_ptr<WsData>& wsData, const chat::GetMessagesRequest& req) const;
    drogon::Task<chat::LogoutResponse> handleLogoutUser(const std::shared_ptr<WsData>& wsData, IChatRoomService& room_service) const;
private:

    drogon::Task<std::optional<chat::UserRights>> GetRoleType(uint32_t user_id, uint32_t room_id) const;
    std::optional<std::string> validateUtf8String(const std::string& textToValidate, size_t maxLength, const std::string& fieldName) const;

    drogon::orm::DbClientPtr m_dbClient;
};
