#pragma once

#include <drogon/orm/CoroMapper.h>

#include <server/chat/WsData.h>
#include <server/chat/IRoomService.h>

#include <server/models/Users.h>
#include <server/models/Rooms.h>
#include <server/models/Messages.h>

namespace models = drogon_model::drogon_test;

class MessageHandlers {
public:
    static drogon::Task<chat::InitialAuthResponse> handleAuthInitial(const std::shared_ptr<WsData>& wsData, const chat::InitialAuthRequest& req);

    static drogon::Task<chat::InitialRegisterResponse> handleRegisterInitial(const std::shared_ptr<WsData>& wsData, const chat::InitialRegisterRequest& req);

    static drogon::Task<chat::AuthResponse> handleAuth(const std::shared_ptr<WsData>& wsData, const chat::AuthRequest& req);

    static drogon::Task<chat::RegisterResponse> handleRegister(const std::shared_ptr<WsData>& wsData, const chat::RegisterRequest& req);

    static drogon::Task<chat::SendMessageResponse> handleSendMessage(const std::shared_ptr<WsData>& wsData, const chat::SendMessageRequest& req);

    static drogon::Task<chat::JoinRoomResponse> handleJoinRoom(const std::shared_ptr<WsData>& wsData, const chat::JoinRoomRequest& req, IRoomService& room_service);

    static drogon::Task<chat::LeaveRoomResponse> handleLeaveRoom(const std::shared_ptr<WsData>& wsData, const chat::LeaveRoomRequest&, IRoomService& room_service);

    static drogon::Task<chat::GetRoomsResponse> handleGetRooms(const std::shared_ptr<WsData>& wsData, const chat::GetRoomsRequest&);

    static drogon::Task<chat::CreateRoomResponse> handleCreateRoom(const std::shared_ptr<WsData>& wsData, const chat::CreateRoomRequest& req);

    static drogon::Task<chat::GetMessagesResponse> handleGetMessages(const std::shared_ptr<WsData>& wsData, const chat::GetMessagesRequest& req);

    static drogon::Task<chat::LogoutResponse> handleLogoutUser(const std::shared_ptr<WsData>& wsData, IRoomService& room_service);
};
