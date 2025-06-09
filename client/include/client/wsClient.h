#pragma once

#include <wx/string.h>
#include <wx/datetime.h>
#include <wx/longlong.h>
#include <drogon/WebSocketClient.h>

class MainWidget;
struct Room;
struct Message;

class WebSocketClient {
public:
    explicit WebSocketClient(MainWidget* ui);

    void start();
    void registerUser(const std::string& username, const std::string& password);
    void loginUser(const std::string& username, const std::string& password);
    void getRooms();
    void createRoom(const std::string& roomName);
    void joinRoom(int32_t room_id);
    void leaveRoom();
    void sendMessage(const std::string& message);
    void requestRoomList();
    void scheduleRoomListRefresh();
    void getMessages(int32_t limit, int64_t offset_ts);

    static std::string formatMessageTimestamp(uint64_t timestamp);

private:
    void sendEnvelope(const chat::Envelope& env);
    void handleMessage(const std::string& msg);

    // UI helpers
    void showError(const wxString& msg);
    void showInfo(const wxString& msg);
    void updateRoomsPanel(const std::vector<Room>& rooms);
    void showChat();
    void showRooms();
    void showRoomMessage(const chat::MessageInfo& mi);
    void showMessageHistory(const std::vector<Message>& messages);


    MainWidget* ui;
    std::shared_ptr<drogon::WebSocketConnection> conn;
    drogon::WebSocketClientPtr client;
    bool connected = false;
};
