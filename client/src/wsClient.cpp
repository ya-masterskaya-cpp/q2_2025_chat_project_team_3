#include <client/wsClient.h>
#include <client/mainWidget.h>
#include <client/authPanel.h>
#include <client/roomsPanel.h>
#include <client/chatPanel.h>
#include <drogon/WebSocketClient.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpAppFramework.h>
#include <json/json.h>
#include <mutex>
#include <thread>
#include <chrono>

WebSocketClient::WebSocketClient(MainWidget* ui_) : ui(ui_) {}

void WebSocketClient::start() {

    LOG_INFO << "WebSocketClient::start()";

    std::string server = "ws://localhost:8848";
    std::string path = "/ws";

    client = drogon::WebSocketClient::newWebSocketClient(server);
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setPath(path);

    client->setMessageHandler([this](const std::string &message,
                                     const drogon::WebSocketClientPtr &,
                                     const drogon::WebSocketMessageType &type) {
        if(type == drogon::WebSocketMessageType::Text) {
            this->handleMessage(message);
        }
    });

    client->setConnectionClosedHandler([this](const drogon::WebSocketClientPtr &) {
        this->connected = false;
        wxTheApp->CallAfter([this]{ ui->authPanel->SetButtonsEnabled(false); });
        wxTheApp->CallAfter([this]{ ui->ShowPopup("Disconnected!", wxICON_ERROR); });
    });

    LOG_INFO << "Connecting to WebSocket at " << server;
    client->connectToServer(
        req,
        [this](drogon::ReqResult r, const drogon::HttpResponsePtr &, const drogon::WebSocketClientPtr &wsPtr) {
            if(r != drogon::ReqResult::Ok) {
                this->connected = false;
                wxTheApp->CallAfter([this]{ ui->authPanel->SetButtonsEnabled(false); });
                wxTheApp->CallAfter([this]{ ui->ShowPopup("Connection failed!", wxICON_ERROR); });
                this->conn.reset();
                return;
            }
            this->conn = wsPtr->getConnection();
        }
    );
}

void WebSocketClient::registerUser(const std::string& username, const std::string& password) {
    Json::Value j;
    j["channel"] = "client2server";
    j["type"] = "register";
    j["data"]["username"] = username;
    j["data"]["password"] = password;
    sendJson(j);
}

void WebSocketClient::loginUser(const std::string& username, const std::string& password) {
    Json::Value j;
    j["channel"] = "client2server";
    j["type"] = "auth";
    j["data"]["username"] = username;
    j["data"]["password"] = password;
    sendJson(j);
}

void WebSocketClient::getRooms() {
    Json::Value j;
    j["channel"] = "client2server";
    j["type"] = "getRooms";
    sendJson(j);
}

void WebSocketClient::createRoom(const std::string& roomName) {
    Json::Value j;
    j["channel"] = "client2server";
    j["type"] = "createRoom";
    j["data"]["roomName"] = roomName;
    sendJson(j);
}

void WebSocketClient::joinRoom(const std::string& roomName) {
    Json::Value j;
    j["channel"] = "client2server";
    j["type"] = "joinRoom";
    j["data"]["room"] = roomName;
    sendJson(j);
}

void WebSocketClient::leaveRoom() {
    Json::Value j;
    j["channel"] = "client2server";
    j["type"] = "leaveRoom";
    sendJson(j);
}

void WebSocketClient::sendMessage(const std::string& message) {
    Json::Value j;
    j["channel"] = "client2server";
    j["type"] = "sendMessage";
    j["data"]["message"] = message;
    sendJson(j);
}

void WebSocketClient::sendJson(const Json::Value& val) {
    if(this->conn && this->conn->connected()) {
        this->conn->send(val.toStyledString());
    } else {
        wxTheApp->CallAfter([this]{ ui->ShowPopup("Not connected to server!", wxICON_ERROR); });
    }
}

void WebSocketClient::scheduleRoomListRefresh() {
    std::thread([this]() {
        while (ui->roomsPanel->IsShown()) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            this->getRooms();
        }
    }).detach();
}

void WebSocketClient::requestRoomList() {
    getRooms();
    scheduleRoomListRefresh();
}

void WebSocketClient::handleMessage(const std::string& msg) {
    Json::CharReaderBuilder rbuilder;
    Json::Value root;
    std::string errs;
    std::istringstream iss(msg);
    if(!Json::parseFromStream(rbuilder, iss, &root, &errs)) {
        wxTheApp->CallAfter([this]{ ui->ShowPopup("Invalid JSON received!", wxICON_ERROR); });
        return;
    }
    std::string type = root.get("type", "").asString();
    std::string channel = root.get("channel", "").asString();

    if(channel != "server2client") {
        return;
    }

    if(type == "serverHello") {
        this->connected = true;
        wxTheApp->CallAfter([this]{ ui->authPanel->SetButtonsEnabled(true); });
        wxTheApp->CallAfter([this]{ ui->ShowPopup("Connected!", wxICON_INFORMATION); });
        return;
    }

    if(type == "roomMessage") {
        std::string user = root["data"]["username"].asString();
        std::string text = root["data"]["message"].asString();
        wxTheApp->CallAfter([this, user, text] {
            ui->chatPanel->AppendMessage(wxString::Format("%s: %s",
                wxString(user.c_str(), wxConvUTF8),
                wxString(text.c_str(), wxConvUTF8)));
        });
        return;
    }

    if(!root["data"]["success"].asBool()) {
        wxTheApp->CallAfter([this] {
            ui->ShowPopup("Operation failed!", wxICON_ERROR);
        });
    } else {
        if(type == "getRooms") {
            std::vector<std::string> rooms;
            for (const auto& r : root["data"]["rooms"]) rooms.push_back(r.asString());
            wxTheApp->CallAfter([this, rooms] {
                ui->roomsPanel->UpdateRoomList(rooms);
            });
        } else if(type == "joinRoom") {
            wxTheApp->CallAfter([this] { ui->ShowChat(); });
        } else if(type == "leaveRoom") {
            wxTheApp->CallAfter([this] { ui->ShowRooms(); });
        } else if(type == "register") {
            wxTheApp->CallAfter([this] {
                ui->ShowPopup("Registration successful!", wxICON_INFORMATION);
            });
        } else if(type == "auth") {
            wxTheApp->CallAfter([this] {
                ui->ShowPopup("Login successful!", wxICON_INFORMATION);
                ui->ShowRooms();
            });
        }
    }

}
