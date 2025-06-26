#pragma once

#include <wx/wx.h>

namespace client {

class InitialPanel;
class ServersPanel;
class AuthPanel;
class RoomsPanel;
class ChatPanel;
class WebSocketClient;

struct User;

class MainWidget : public wxFrame {
public:
    MainWidget();
    void ShowPopup(const wxString& msg, long icon = wxICON_INFORMATION);

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
};

} // namespace client
