#pragma once
#include <drogon/WebSocketClient.h>
#include <json/json.h>
#include <mutex>
#include <vector>
#include <string>
#include <atomic>

class MainWidget;

class WebSocketClient {
public:
    explicit WebSocketClient(MainWidget* ui);

    void start();

    void registerUser(const std::string& username, const std::string& password);
    void loginUser(const std::string& username, const std::string& password);
    void getRooms();
    void createRoom(const std::string& roomName);
    void joinRoom(const std::string& roomName);
    void leaveRoom();
    void sendMessage(const std::string& message);

    void requestRoomList();

    std::atomic<bool> connected{false};

private:
    void handleMessage(const std::string& msg);
    void sendJson(const Json::Value& val);
    void scheduleRoomListRefresh();

    drogon::WebSocketClientPtr client;
    drogon::WebSocketConnectionPtr conn;
    MainWidget* ui;
};
