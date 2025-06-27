#pragma once

#include <wx/wx.h>
#include <client/user.h>

namespace client {

class InitialPanel;
class ServersPanel;
class AuthPanel;
class RoomsPanel;
class ChatPanel;
class WebSocketClient;

class MainWidget : public wxFrame {
public:
    MainWidget();
    void ShowPopup(const wxString& msg, long icon = wxICON_INFORMATION);

    void SetCurrentUser(const User& user);
    const User& GetCurrentUser() const;

    void ShowInitial();
    void ShowServers();
    void ShowAuth();
    void ShowRooms();
    void ShowChat(std::vector<User> users);

    InitialPanel* initialPanel;
    ServersPanel* serversPanel;
    AuthPanel* authPanel;
    RoomsPanel* roomsPanel;
    ChatPanel* chatPanel;
    WebSocketClient* wsClient;

private:
    User m_currentUser;
};

} // namespace client
