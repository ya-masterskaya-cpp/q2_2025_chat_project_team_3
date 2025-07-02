#pragma once

#include <wx/wx.h>

namespace client {

class InitialPanel;
class ServersPanel;
class AuthPanel;
class ChatPanel;
class WebSocketClient;
struct User;
struct Room;

class MainWidget : public wxFrame {
public:
    MainWidget();
    void ShowPopup(const wxString& msg, long icon = wxICON_INFORMATION);

    void ShowInitial();
    void ShowServers();
    void ShowAuth();
    void ShowMainChatView(const std::vector<Room*>& rooms, const User& user);

    InitialPanel* initialPanel;
    ServersPanel* serversPanel;
    AuthPanel* authPanel;
    ChatPanel* chatPanel;
    WebSocketClient* wsClient;

private:
    void OnActivate(wxActivateEvent& event);
};

} // namespace client
