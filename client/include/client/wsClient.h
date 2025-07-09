#pragma once

#include <wx/string.h>
#include <wx/datetime.h>
#include <wx/longlong.h>
#include <drogon/WebSocketClient.h>
#include <optional>

namespace client {

class MainWidget;
struct Room;
struct Message;
struct User;

class WebSocketClient {
public:
    explicit WebSocketClient(MainWidget* ui);

    void start(const std::string& address);
    void stop();
    
    void requestInitialRegister(const std::string& username);
    void requestInitialAuth(const std::string& username);
    void completeRegister(const std::string& hash, const std::string& salt);
    void completeAuth(const std::string& hash, const std::optional<std::string>& password, const std::optional<std::string>& salt);
    void createRoom(const std::string& roomName);
    void joinRoom(int32_t room_id);
    void leaveRoom();
    void sendMessage(const std::string& message);
    void getMessages(int32_t limit, int64_t offset_ts);
    void logout();
    void getServers();
    void renameRoom(int32_t roomId, const std::string& newName);
    void deleteRoom(int32_t roomId);
    void assignRole(int32_t roomId, int32_t userId, chat::UserRights role);
    void deleteMessage(int32_t messageId);
    void sendTypingStart();
    void sendTypingStop();
    void becomeMember(int32_t roomId);
    void changeUsername(const std::string& username);
    void requestMySalt();
    void changePassword(const std::string& old_hash, const std::string& new_hash, const std::string& new_salt);

    static std::string formatMessageTimestamp(int64_t timestamp);

private:
    void sendEnvelope(const chat::Envelope& env);
    void handleMessage(const std::string& msg);

    // UI helpers
    void showError(const wxString& msg);
    void showInfo(const wxString& msg);
    void updateRoomsPanel(const std::vector<Room*>& rooms);
    void showChat(std::vector<User> users);
    void showRooms();
    void showRoomMessage(const chat::MessageInfo& mi);
    void showMessageHistory(const std::vector<Message>& messages);
    void addUser(User user);
    void removeUser(User user);
    void showAuth();
    void showServers();
    void showInitial();
    void SetServers(const std::vector<std::string> &servers);
    void updateUserRole(int32_t userId, chat::UserRights newRole);
    void removeMessageFromView(int32_t messageId);
    void addRoom(Room* room);
    void becameMember();
    void updateUsername(int32_t userId, const std::string& username);
    
    MainWidget* ui;
    std::shared_ptr<drogon::WebSocketConnection> conn;
    drogon::WebSocketClientPtr client;
};

} // namespace client
