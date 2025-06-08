#include <client/wsClient.h>
#include <client/mainWidget.h>
#include <client/authPanel.h>
#include <client/roomsPanel.h>
#include <client/chatPanel.h>
#include <client/message.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpAppFramework.h>
#include <chrono>
#include <thread>

WebSocketClient::WebSocketClient(MainWidget* ui_) : ui(ui_) {}

void WebSocketClient::start() {
    LOG_INFO << "WebSocketClient::start()";
    std::string server = "ws://localhost:8848";
    std::string path = "/ws";
    client = drogon::WebSocketClient::newWebSocketClient(server);
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setPath(path);

    client->setMessageHandler([this](const std::string& message,
                                     const drogon::WebSocketClientPtr&,
                                     const drogon::WebSocketMessageType& type) {
        if(type == drogon::WebSocketMessageType::Binary) {
            handleMessage(message);
        }
    });

    client->setConnectionClosedHandler([this](const drogon::WebSocketClientPtr&) {
        connected = false;
        wxTheApp->CallAfter([this] { ui->authPanel->SetButtonsEnabled(false); });
        showError("Disconnected!");
    });

    LOG_INFO << "Connecting to WebSocket at " << server;
    client->connectToServer(
        req,
        [this](drogon::ReqResult r, const drogon::HttpResponsePtr&, const drogon::WebSocketClientPtr& wsPtr) {
            if(r != drogon::ReqResult::Ok) {
                connected = false;
                wxTheApp->CallAfter([this] { ui->authPanel->SetButtonsEnabled(false); });
                showError("Connection failed!");
                conn.reset();
                return;
            }
            conn = wsPtr->getConnection();
        }
    );
}

void WebSocketClient::registerUser(const std::string& username, const std::string& password) {
    chat::Envelope env;
    env.mutable_register_request()->set_username(username);
    env.mutable_register_request()->set_password(password);
    sendEnvelope(env);
}

void WebSocketClient::loginUser(const std::string& username, const std::string& password) {
    chat::Envelope env;
    env.mutable_auth_request()->set_username(username);
    env.mutable_auth_request()->set_password(password);
    sendEnvelope(env);
}

void WebSocketClient::getRooms() {
    chat::Envelope env;
    env.mutable_get_rooms_request();
    sendEnvelope(env);
}

void WebSocketClient::createRoom(const std::string& roomName) {
    chat::Envelope env;
    env.mutable_create_room_request()->set_room_name(roomName);
    sendEnvelope(env);
}

void WebSocketClient::joinRoom(uint32_t room_id) {
    chat::Envelope env;
    env.mutable_join_room_request()->set_room_id(room_id);
    sendEnvelope(env);
}

void WebSocketClient::leaveRoom() {
    chat::Envelope env;
    env.mutable_leave_room_request();
    sendEnvelope(env);
}

void WebSocketClient::sendMessage(const std::string& message) {
    chat::Envelope env;
    env.mutable_send_message_request()->set_message(message);
    sendEnvelope(env);
}

void WebSocketClient::sendEnvelope(const chat::Envelope& env) {
    if(conn && conn->connected()) {
        std::string out;
        if(env.SerializeToString(&out)) {
            conn->send(out, drogon::WebSocketMessageType::Binary);
        } else {
            showError("Failed to serialize message!");
        }
    } else {
        showError("Not connected to server!");
    }
}

void WebSocketClient::scheduleRoomListRefresh() {
    std::thread([this]() {
        while(ui->roomsPanel->IsShown()) {
            std::this_thread::sleep_for(std::chrono::seconds(5)); // TODO: create method
            getRooms();
        }
    }).detach();
}

void WebSocketClient::getMessages(int limit, int offset){
chat::Envelope env;
    auto* request = env.mutable_get_messages_request();
    request->set_limit(limit);
    request->set_offset(offset);
    sendEnvelope(env);
}

void WebSocketClient::requestRoomList()
{
    getRooms();
    scheduleRoomListRefresh();
}

void WebSocketClient::handleMessage(const std::string& msg) {
    chat::Envelope env;
    if(!env.ParseFromString(msg)) {
        showError("Invalid protobuf message received!");
        return;
    }
    using SC = chat::StatusCode;
    auto statusOk = [](const chat::Status& s) { return s.code() == SC::STATUS_SUCCESS; };

    switch(env.payload_case()) {
        case chat::Envelope::kServerHello: {
            connected = true;
            wxTheApp->CallAfter([this] { ui->authPanel->SetButtonsEnabled(true); });
            showInfo("Connected!");
            break;
        }
        case chat::Envelope::kRoomMessage: {
            showRoomMessage(env.room_message().message());
            break;
        }
        case chat::Envelope::kGetRoomsResponse: {
            if(statusOk(env.get_rooms_response().status())) {
                std::vector<Room> rooms;
                for (const auto& proto_room : env.get_rooms_response().rooms()){
                    rooms.emplace_back(Room{proto_room.room_id(), proto_room.room_name()});
                }
                updateRoomsPanel(rooms);
            } else {
                showError("Failed to get rooms.");
            }
            break;
        }
        case chat::Envelope::kJoinRoomResponse: {
            if(statusOk(env.join_room_response().status())) {
                getMessages(LAST_MESSAGES, 0);
                showChat();
            } else {
                showError("Failed to join room.");
            }
            break;
        }
        case chat::Envelope::kLeaveRoomResponse: {
            if(statusOk(env.leave_room_response().status())) {
                showRooms();
            } else {
                showError("Failed to leave room.");
            }
            break;
        }
        case chat::Envelope::kCreateRoomResponse: {
            if (!statusOk(env.create_room_response().status())) {
                showError("Failed to create room");
            }
            break;
        }
        case chat::Envelope::kRegisterResponse: {
            if(statusOk(env.register_response().status())) {
                showInfo("Registration successful!");
            } else {
                showError("Registration failed!");
            }
            break;
        }
        case chat::Envelope::kAuthResponse: {
            if(statusOk(env.auth_response().status())) {
                showInfo("Login successful!");
                showRooms();
            } else {
                showError("Login failed!");
            }
            break;
        }
        case chat::Envelope::kSendMessageResponse: {
            if(!statusOk(env.send_message_response().status())) {
                showError("Failed to send message!");
            }
            break;
        }
        case chat::Envelope::kGenericError: {
            showError(wxString::Format("Server error: %s",
                wxString(env.generic_error().status().message().c_str(), wxConvUTF8)));
            break;
        }
        case chat::Envelope::kGetMessagesResponse: {
            if(!statusOk(env.get_messages_response().status())) {
                showError("Failed to get messages!");
            }
            std::vector<Message> messages;
            for(const auto& proto_message : env.get_messages_response().message()) {
                messages.emplace_back(Message{wxString::FromUTF8(proto_message.from())
                    , wxString::FromUTF8(proto_message.message())
                    , proto_message.timestamp()});
            }
            showMessageHistory(messages);
            break;
        }
        default: {
            showError("Unknown message received from server!");
            break;
        }
    }
}

void WebSocketClient::showError(const wxString& msg) {
    wxTheApp->CallAfter([this, msg] { ui->ShowPopup(msg, wxICON_ERROR); });
}

void WebSocketClient::showInfo(const wxString& msg) {
    wxTheApp->CallAfter([this, msg] { ui->ShowPopup(msg, wxICON_INFORMATION); });
}

void WebSocketClient::updateRoomsPanel(const std::vector<Room>& rooms) {
    wxTheApp->CallAfter([this, rooms] { ui->roomsPanel->UpdateRoomList(rooms); });
}

void WebSocketClient::showChat() {
    wxTheApp->CallAfter([this] { ui->ShowChat(); });
}

void WebSocketClient::showRooms() {
    wxTheApp->CallAfter([this] { ui->ShowRooms(); });
}

void WebSocketClient::showRoomMessage(const chat::MessageInfo& mi) {
    wxTheApp->CallAfter([this, user = mi.from(), text = mi.message(), time = mi.timestamp()] {
        std::string timeStr = formatMessageTimestamp(time);
        ui->chatPanel->AppendMessage(
            wxString(timeStr.c_str(), wxConvUTF8),
            wxString(user.c_str(), wxConvUTF8),
            wxString(text.c_str(), wxConvUTF8));
    });
}

void WebSocketClient::showMessageHistory(const std::vector<Message> &messages) {
    std::vector<Message> sorted_messages = messages;
    std::sort(sorted_messages.begin(), sorted_messages.end(),
        [](const Message &lhs, const Message &rhs) {
            return lhs.timestamp < rhs.timestamp;
        });

    wxTheApp->CallAfter([this, sorted_messages = std::move(sorted_messages)] {
        LOG_DEBUG << "Stared bulk add";
        ui->chatPanel->AppendMessages(sorted_messages);
        LOG_DEBUG << "Finished bulk add";
    });
}

std::string WebSocketClient::formatMessageTimestamp(uint64_t timestamp) {
    trantor::Date msgDate(timestamp);
    auto now = trantor::Date::now();
    auto zeroTime = [](const trantor::Date& dt) {
        time_t t = dt.microSecondsSinceEpoch() / 1000000ULL;
        struct tm local_tm{};
        localtime_r(&t, &local_tm);
        local_tm.tm_hour = 0;
        local_tm.tm_min = 0;
        local_tm.tm_sec = 0;
        time_t zero_t = mktime(&local_tm);
        return trantor::Date(static_cast<uint64_t>(zero_t) * 1000000ULL);
    };
    trantor::Date todayZero = zeroTime(now);
    trantor::Date msgZero = zeroTime(msgDate);

    if (msgZero == todayZero) {
        return "[" + msgDate.toCustomFormattedStringLocal("%H:%M") + "]";
    }
    int msgYear = std::stoi(msgDate.toCustomFormattedStringLocal("%Y"));
    int nowYear = std::stoi(now.toCustomFormattedStringLocal("%Y"));
    if (msgYear == nowYear) {
        return "[" + msgDate.toCustomFormattedStringLocal("%d.%m %H:%M") + "]";
    }
    else {
        return "[" + msgDate.toCustomFormattedStringLocal("%d.%m.%Y %H:%M") + "]";
    }
}
