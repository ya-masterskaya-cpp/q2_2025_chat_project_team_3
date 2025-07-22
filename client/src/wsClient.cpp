#include <client/wsClient.h>
#include <client/mainWidget.h>
#include <client/authPanel.h>
#include <client/roomsPanel.h>
#include <client/chatPanel.h>
#include <client/userListPanel.h>
#include <client/serversPanel.h>
#include <client/message.h>
#include <client/messageView.h>
#include <client/user.h>
#include <client/chatInterface.h>
#include <client/accountSettings.h>
#include <common/utils/utils.h>
#include <common/version.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpAppFramework.h>
#include <ada.h>
#include <time.h>

namespace client {

WebSocketClient::WebSocketClient(MainWidget* ui_) : ui(ui_) {}

void WebSocketClient::stop() {
    drogon::app().getLoop()->runInLoop([this]{
        LOG_INFO << "WebSocketClient::stop()";
        conn.reset();
        client.reset();
    });
}

void WebSocketClient::start(const std::string& address) {
    stop();

    drogon::app().getLoop()->runInLoop([this, address]{

        LOG_INFO << "WebSocketClient::start()";

        auto result = ada::parse<ada::url_aggregator>(address);

        auto server = std::string(result->get_protocol()) + "//" + std::string(result->get_hostname());

        auto port = result->get_port();
        if (!port.empty()) {
            server += std::string(":") + std::string(port);
        }

        client = drogon::WebSocketClient::newWebSocketClient(server);
        auto req = drogon::HttpRequest::newHttpRequest();
        req->setPath(std::string(result->get_pathname()));

        client->setMessageHandler([this](const std::string& message,
                                        const drogon::WebSocketClientPtr&,
                                        const drogon::WebSocketMessageType& type) {
            if(type == drogon::WebSocketMessageType::Binary) {
                handleMessage(message);
            }
        });

        client->setConnectionClosedHandler([this](const drogon::WebSocketClientPtr&) {
        });

        LOG_INFO << "Connecting to WebSocket at " << server;
        client->connectToServer(
            req,
            [this](drogon::ReqResult r, const drogon::HttpResponsePtr&, const drogon::WebSocketClientPtr& wsPtr) {
                if(r != drogon::ReqResult::Ok) {
                    conn.reset();
                    return;
                }
                conn = wsPtr->getConnection();
            }
        );

    });
}

void WebSocketClient::requestInitialRegister(const std::string &username) {
    chat::Envelope env;
    env.mutable_initial_register_request()->set_username(username);
    sendEnvelope(env);
}

void WebSocketClient::requestInitialAuth(const std::string &username) {
    chat::Envelope env;
    env.mutable_initial_auth_request()->set_username(username);
    sendEnvelope(env);
}

void WebSocketClient::completeRegister(const std::string &hash, const std::string& salt) {
    chat::Envelope env;
    auto* req = env.mutable_register_request();
    req->set_hash(hash);
    req->set_salt(salt);
    sendEnvelope(env);
}

void WebSocketClient::completeAuth(const std::string& hash, const std::optional<std::string>& password, const std::optional<std::string>& salt) {
    chat::Envelope env;
    auto* req = env.mutable_auth_request();
    req->set_hash(hash);
    if (password) req->set_password(*password);
    if (salt) req->set_salt(*salt);
    sendEnvelope(env);
}

void WebSocketClient::createRoom(const std::string& roomName) {
    chat::Envelope env;
    env.mutable_create_room_request()->set_room_name(roomName);
    sendEnvelope(env);
}

void WebSocketClient::joinRoom(int32_t room_id) {
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
    drogon::app().getLoop()->runInLoop([this, env]{ // TODO: this copies whole envolope
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
    });
}

void WebSocketClient::getMessages(int32_t limit, int64_t offset_ts) {
    chat::Envelope env;
    auto* request = env.mutable_get_messages_request();
    request->set_limit(limit);
    request->set_offset_ts(offset_ts);
    sendEnvelope(env);
}

void WebSocketClient::logout() {
    chat::Envelope env;
    env.mutable_logout_request();
    sendEnvelope(env);
}

void WebSocketClient::getServers() {
    chat::Envelope env;
    env.mutable_get_servers_request();
    sendEnvelope(env);
}

void WebSocketClient::renameRoom(int32_t roomId, const std::string& newName) {
    chat::Envelope env;
    auto* request = env.mutable_rename_room_request();
    request->set_room_id(roomId);
    request->set_name(newName);
    sendEnvelope(env);
}

void WebSocketClient::deleteRoom(int32_t roomId) {
    chat::Envelope env;
    auto* request = env.mutable_delete_room_request();
    request->set_room_id(roomId);
    sendEnvelope(env);
}

void WebSocketClient::assignRole(int32_t roomId, int32_t userId, chat::UserRights role) {
    chat::Envelope env;
    auto* req = env.mutable_assign_role_request();
    req->set_room_id(roomId);
    req->set_user_id(userId);
    req->set_new_role(role);
    sendEnvelope(env);
}

void WebSocketClient::deleteMessage(int32_t messageId) {
    chat::Envelope env;
    auto* request = env.mutable_delete_message_request();
    request->set_message_id(messageId);
    sendEnvelope(env);
}

void WebSocketClient::sendTypingStart() {
    chat::Envelope env;
    env.mutable_user_typing_start_request();
    sendEnvelope(env);
}

void WebSocketClient::sendTypingStop() {
    chat::Envelope env;
    env.mutable_user_typing_stop_request();
    sendEnvelope(env);
}

void WebSocketClient::becomeMember(int32_t roomId) {
    chat::Envelope env;
    env.mutable_become_member_request()->set_room_id(roomId);
    sendEnvelope(env);
}

void WebSocketClient::changeUsername(const std::string& username) {
    chat::Envelope env;
    auto* request = env.mutable_change_username_request();
    request->set_new_username(username);
    sendEnvelope(env);
}

void WebSocketClient::requestMySalt() {
    chat::Envelope env;
    env.mutable_get_my_salt_request();
    sendEnvelope(env);
}

void WebSocketClient::changePassword(const std::string& old_hash, const std::string& new_hash, const std::string& new_salt) {
    chat::Envelope env;
    auto* request = env.mutable_change_password_request();
    request->set_old_password_hash(old_hash);
    request->set_new_password_hash(new_hash);
    request->set_new_salt(new_salt);
	sendEnvelope(env);
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
            //wxTheApp->CallAfter([this] { ui->authPanel->SetButtonsEnabled(true); });
            //showInfo("Connected!");

            if(env.server_hello().protocol_version() != common::version::PROTOCOL_VERSION) {
                showError("Version mismatch, update your client");
                showInitial();
            } else if(env.server_hello().type() == chat::ServerType::TYPE_AGGREGATOR) {
                LOG_TRACE << "Aggregator, getting initial list of servers";
                getServers();
                showServers();
            } else {
                showAuth();
            }
            break;
        }
        case chat::Envelope::kRoomMessage: {
            showRoomMessage(env.room_message().message());
            break;
        }
        case chat::Envelope::kJoinRoomResponse: {
            if (statusOk(env.join_room_response().status())) {
                std::vector<User> all_users;
                all_users.reserve(env.join_room_response().all_users().size());
                
                for (const auto& user : env.join_room_response().all_users()) {
                    all_users.emplace_back(user.user_id(), wxString::FromUTF8(user.user_name()), user.user_room_rights());
                }
                wxTheApp->CallAfter([this] {
                    ui->chatInterface->m_roomsPanel->OnJoinRoom();
                });
                showChat(std::move(all_users));

                for (const auto& user : env.join_room_response().active_users()) {
                    addUser({ user.user_id(), wxString::FromUTF8(user.user_name()), user.user_room_rights() });
                }
            } else {
                showError("Failed to join room.");
            }
            break;
        }
        case chat::Envelope::kUserJoined: {
            addUser({env.user_joined().user().user_id(), wxString::FromUTF8(env.user_joined().user().user_name()), env.user_joined().user().user_room_rights()});
            break;
        }
        case chat::Envelope::kUserLeft: {
            removeUser({env.user_left().user().user_id(), wxString::FromUTF8(env.user_left().user().user_name()), env.user_left().user().user_room_rights()});
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
            if(!statusOk(env.create_room_response().status())) {
                showError("Failed to create room");
            }
            break;
        }
        case chat::Envelope::kInitialRegisterResponse: {
            if (statusOk(env.initial_register_response().status())){
                wxTheApp->CallAfter([this] {
                    ui->authPanel->HandleRegisterContinue();
                });
            } else {
                showError("Failed to register user: " + wxString(env.initial_register_response().status().message()));
            }
            break;
        }
        case chat::Envelope::kInitialAuthResponse: {
            if (statusOk(env.initial_auth_response().status())){
                wxTheApp->CallAfter([this, env = std::move(env)] {
                    const auto& response = env.initial_auth_response();
                    std::string salt;
                    if(response.has_salt()) {
                        salt = response.salt();
                    }
                    ui->authPanel->HandleAuthContinue(salt);
                });
            } else {
                showError("Failed to login");
            }
            break;
        }
        case chat::Envelope::kLogoutResponse: {
            if(statusOk(env.logout_response().status())) {
                wxTheApp->CallAfter([this] { ui->ShowAuth(); });
            } else {
                showError("Failed to logout");
            }
            break;
        }
        case chat::Envelope::kAuthResponse: {
            if(statusOk(env.auth_response().status())) {
                showInfo("Login successful!");
                std::vector<Room*> rooms;
                for (const auto& proto_room : env.auth_response().rooms()){
                    rooms.emplace_back(new Room{proto_room.room_id(), wxString::FromUTF8(proto_room.room_name()), proto_room.is_joined()});
                }
                client::User user;
                user.id = env.auth_response().authenticated_user().user_id();
                user.username = wxString::FromUTF8(env.auth_response().authenticated_user().user_name());
                user.role = chat::UserRights::REGULAR;
                wxTheApp->CallAfter([this, user]() {
                    ui->chatInterface->m_chatPanel->SetCurrentUser(user);
                    ui->accountSettingsPanel->UpdateCurrentUsername(user.username);
                });
                updateRoomsPanel(rooms);
                showRooms();
            } else {
                showError("Login failed! " + wxString(env.auth_response().status().message()));
            }
            break;
        }
        case chat::Envelope::kRegisterResponse: {
            if(statusOk(env.register_response().status())) {
                showInfo("Registration successful!");
            } else {
                showError("Registration failed! " + wxString(env.register_response().status().message()));
            }
            break;
        }
        case chat::Envelope::kSendMessageResponse: {
            if(!statusOk(env.send_message_response().status())) {
                showError("Failed to send message!");
            }
            break;
        }
        case chat::Envelope::kGetServersResponse: {
            std::vector<std::string> servers;

            LOG_TRACE << "got servers resp";
            for(const auto& server : env.get_servers_response().servers()) {
                LOG_TRACE << server.host();
                servers.emplace_back(server.host());
            }
            SetServers(servers);
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
                messages.emplace_back(Message{wxString::FromUTF8(proto_message.from().user_name())
                    , proto_message.from().user_id()
                    , wxString::FromUTF8(proto_message.message())
                    , proto_message.timestamp()
                    , proto_message.message_id()});
            }
            showMessageHistory(messages);
            break;
        }
        case chat::Envelope::kNewRoomCreated: {
            wxTheApp->CallAfter([this, env = std::move(env)] {
                auto& response = env.new_room_created().room();
                const auto& curr_usr = ui->chatInterface->m_chatPanel->GetCurrentUser();
                auto is_joined = response.owner().user_id() == curr_usr.id;
                ui->chatInterface->m_roomsPanel->AddRoom(new Room{response.room_id(), wxString::FromUTF8(response.room_name()), is_joined});
                if(is_joined) {
                    joinRoom(response.room_id());
                }
            });
            break;
        }
        case chat::Envelope::kRenameRoomResponse: {
            if (!statusOk(env.rename_room_response().status())) {
                    showError("Failed to rename room: " + wxString(env.rename_room_response().status().message()));
            }
            break;
        }
        case chat::Envelope::kNewRoomName: {
            wxTheApp->CallAfter([this, env = std::move(env)] {
                const auto& response = env.new_room_name();
                if (ui->chatInterface->m_chatPanel->IsShown() && ui->chatInterface->m_chatPanel->GetRoomId() == response.room_id()) {
                    ui->chatInterface->m_chatPanel->SetRoomName(wxString::FromUTF8(response.name()));
                }
                ui->chatInterface->m_roomsPanel->RenameRoom(response.room_id(), wxString::FromUTF8(response.name()));
            });
            break;
        }
        case chat::Envelope::kDeleteRoomResponse: {
            if (statusOk(env.delete_room_response().status())) {
                wxTheApp->CallAfter([this] { ui->ShowRooms(); });
            } else {
                showError("Failed to delete room: " + wxString(env.delete_room_response().status().message()));
            }
            break;
        }
        case chat::Envelope::kRoomDeleted: {
            wxTheApp->CallAfter([this, env = std::move(env)] {
                auto roomId = env.room_deleted().room_id();
                if (ui->chatInterface->m_chatPanel->IsShown() && ui->chatInterface->m_chatPanel->GetRoomId() == roomId) {
                ui->ShowRooms();
            }
            ui->chatInterface->m_roomsPanel->RemoveRoom(roomId);
            });
            break;
        }
        case chat::Envelope::kAssignRoleResponse: {
            if (!statusOk(env.assign_role_response().status())) {
                showError("Failed to assign role: " + wxString(env.assign_role_response().status().message()));
            }
            break;
        }
        case chat::Envelope::kUserRoleChanged: {
            const auto& roleChange = env.user_role_changed();
            updateUserRole(roleChange.user_id(), roleChange.new_role());
            break;
        }
        case chat::Envelope::kDeleteMessageResponse: {
            if (!statusOk(env.delete_message_response().status())) {
                showError("Failed to delete message: " + wxString(env.delete_message_response().status().message()));
            }
            break;
        }
        case chat::Envelope::kMessageDeleted: {
            removeMessageFromView(env.message_deleted().message_id());
            break;
        }
        case chat::Envelope::kUserTypingStartResponse: {
            if (!statusOk(env.user_typing_start_response().status())) {
                showError("Error when requesting \"User typing start\": " + wxString(env.user_typing_start_response().status().message()));
            }
            break;
        }
        case chat::Envelope::kUserTypingStopResponse: {
            if (!statusOk(env.user_typing_stop_response().status())) {
                showError("Error when requesting \"User typing stop\": " + wxString(env.user_typing_stop_response().status().message()));
            }
            break;
        }
        case chat::Envelope::kUserStartedTyping: {
            const auto& user_info = env.user_started_typing().user();
			User user{ user_info.user_id(), wxString::FromUTF8(user_info.user_name()), user_info.user_room_rights() };
            wxTheApp->CallAfter([this, user] {
                if (ui->chatInterface->m_chatPanel->IsShown()) {
                    ui->chatInterface->m_chatPanel->UserStartedTyping(user);
                }
                });
            break;
        }
        case chat::Envelope::kUserStoppedTyping: {
            const auto& user_info = env.user_stopped_typing().user();
            User user{ user_info.user_id(), wxString::FromUTF8(user_info.user_name()), user_info.user_room_rights() };
            wxTheApp->CallAfter([this, user] {
                if (ui->chatInterface->m_chatPanel->IsShown()) {
                    ui->chatInterface->m_chatPanel->UserStoppedTyping(user);
                }
                });
            break;
        }
        case chat::Envelope::kBecomeMemberResponse: {
            if (!statusOk(env.become_member_response().status())) {
                showError("Error when attempting to become a member: " + wxString(env.become_member_response().status().message()));
            } else {
                becameMember();
            }
            break;
        }
        case chat::Envelope::kChangeUsernameResponse: {
            if (statusOk(env.change_username_response().status())) {
                showInfo("Username has been successfully changed!");
            }
            else {
                showError("Error when attempting to change username : " + wxString(env.change_username_response().status().message()));
            }
            break;
        }
        case chat::Envelope::kUsernameChanged: {
            updateUsername(env.username_changed().user_id(), env.username_changed().new_username());
            break;
        }
        case chat::Envelope::kGetMySaltResponse: {
            if (statusOk(env.get_my_salt_response().status())) {
                wxTheApp->CallAfter([this, salt = env.get_my_salt_response().salt()]() {
                    ui->accountSettingsPanel->OnPasswordChangeContinue(salt);
                });
            }
            else {
                showError("Failed to get user data for password change.");
                wxTheApp->CallAfter([this]() {
                    ui->accountSettingsPanel->OnPasswordChangeFailed();
                });
            }
            break;
        }
        case chat::Envelope::kChangePasswordResponse: {
            if (statusOk(env.change_password_response().status())) {
                showInfo("Password has been successfully changed!");
            }
            else {
                showError("Error when attempting to change password : " + wxString(env.change_password_response().status().message()));
            }
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

void WebSocketClient::updateRoomsPanel(const std::vector<Room*> &rooms)
{
    wxTheApp->CallAfter([this, rooms] { ui->chatInterface->m_roomsPanel->UpdateRoomList(rooms); });
}

void WebSocketClient::showChat(std::vector<User> users) {
    wxTheApp->CallAfter([this, users = std::move(users)]() mutable { ui->ShowChat(std::move(users)); });
}

void WebSocketClient::showRooms() {
    wxTheApp->CallAfter([this] { ui->ShowRooms(); });
}

void WebSocketClient::showInitial() {
    wxTheApp->CallAfter([this] { ui->ShowInitial(); });
}

void WebSocketClient::showAuth() {
    wxTheApp->CallAfter([this] { ui->ShowAuth(); });
}

void WebSocketClient::showServers() {
    wxTheApp->CallAfter([this] { ui->ShowServers(); });
}

void WebSocketClient::addUser(User user) {
    wxTheApp->CallAfter([this, user = std::move(user)] {
        ui->chatInterface->m_chatPanel->UserJoin(user);
    });
}

void WebSocketClient::removeUser(User user) {
    wxTheApp->CallAfter([this, user = std::move(user)] {
        ui->chatInterface->m_chatPanel->UserLeft(user);
    });
}

void WebSocketClient::showRoomMessage(const chat::MessageInfo& mi) {
    std::vector<Message> messages;
    messages.emplace_back(Message{wxString::FromUTF8(mi.from().user_name())
        , mi.from().user_id()
        , wxString::FromUTF8(mi.message())
        , mi.timestamp()
        , mi.message_id()});

    wxTheApp->CallAfter([this, messages] {
        LOG_DEBUG << "Stared singular add";
        ui->chatInterface->m_chatPanel->m_messageView->OnMessagesReceived(messages, false);
        LOG_DEBUG << "Finished singular add";
    });
}

void WebSocketClient::showMessageHistory(const std::vector<Message> &messages) {
    wxTheApp->CallAfter([this, messages] {
        LOG_DEBUG << "Stared bulk add";
        ui->chatInterface->m_chatPanel->m_messageView->OnMessagesReceived(messages, true);
        LOG_DEBUG << "Finished bulk add";
    });
}

void WebSocketClient::SetServers(const std::vector<std::string> &servers) {
    wxTheApp->CallAfter([this, servers] {ui->serversPanel->SetServers(servers);});
}

void WebSocketClient::updateUserRole(int32_t userId, chat::UserRights newRole) {
    wxTheApp->CallAfter([this, userId, newRole] {
        ui->chatInterface->m_chatPanel->m_userListPanel->UpdateUserRole(userId, newRole);
    });
}

void WebSocketClient::removeMessageFromView(int32_t messageId) {
    wxTheApp->CallAfter([this, messageId] {
        if (ui->chatInterface->m_chatPanel->IsShown()) {
            ui->chatInterface->m_chatPanel->m_messageView->DeleteMessageById(messageId);
        }
    });
}

void WebSocketClient::addRoom(Room* room) {
    wxTheApp->CallAfter([this, room] {
        ui->chatInterface->m_roomsPanel->AddRoom(room);
    });
}

void WebSocketClient::becameMember() {
    wxTheApp->CallAfter([this] {
        ui->chatInterface->m_roomsPanel->OnBecameMember();
    });
}

void WebSocketClient::updateUsername(int32_t userId, const std::string& username) {
    wxTheApp->CallAfter([this, userId, username] {
        wxString newUsername = wxString::FromUTF8(username);
        const auto& currentUser = ui->chatInterface->m_chatPanel->GetCurrentUser();

        if (currentUser.id == userId) {
            User updatedUser = currentUser;
            updatedUser.username = newUsername;

            ui->chatInterface->m_chatPanel->SetCurrentUser(updatedUser);
            ui->accountSettingsPanel->UpdateCurrentUsername(newUsername);
        }
        if (ui->chatInterface->m_chatPanel->IsShown()) {
            ui->chatInterface->m_chatPanel->UpdateUsername(userId, newUsername);
        }
    });
}

std::string WebSocketClient::formatMessageTimestamp(int64_t timestamp)
{
    trantor::Date msgDate(timestamp);
    auto now = trantor::Date::now();
    auto zeroTime = [](const trantor::Date& dt) {
        time_t t = dt.microSecondsSinceEpoch() / 1000000LL;
        struct tm local_tm;
        // Cross-platform local time conversion
#if defined(_WIN32)
        localtime_s(&local_tm, &t);
#else
        localtime_r(&t, &local_tm);
#endif
        local_tm.tm_hour = 0;
        local_tm.tm_min = 0;
        local_tm.tm_sec = 0;
        time_t zero_t = mktime(&local_tm);
        return trantor::Date(static_cast<int64_t>(zero_t) * 1000000LL);
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

} // namespace client
