#pragma once
#include <wx/string.h>
#include <drogon/WebSocketClient.h>
#include <common/proto/chat.pb.h>
#include <memory>
#include <string>

class MainWidget;
struct Room;

class WebSocketClient {
public:
    explicit WebSocketClient(MainWidget* ui);

    void start();
    void registerUser(const std::string& username, const std::string& password);
    void loginUser(const std::string& username, const std::string& password);
    void getRooms();
    void createRoom(const std::string& roomName);
    void joinRoom(uint32_t room_id);
    void leaveRoom();
    void sendMessage(const std::string& message);
    void requestRoomList();
    void scheduleRoomListRefresh();
    void getMessages(int limit, int offset);

private:
    void sendEnvelope(const chat::Envelope& env);
    void handleMessage(const std::string& msg);

    // UI helpers
    void showError(const wxString& msg);
    void showInfo(const wxString& msg);
    void updateRoomsPanel(const std::vector<Room>& rooms);
    void showChat();
    void showRooms();
    void showRoomMessage(const chat::RoomMessage& rm);

    MainWidget* ui;
    std::shared_ptr<drogon::WebSocketConnection> conn;
    drogon::WebSocketClientPtr client;
    bool connected = false;
};
