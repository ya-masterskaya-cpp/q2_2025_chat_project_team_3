#pragma once

#include <wx/wx.h>

namespace client {

class InitialPanel;
class ServersPanel;
class AuthPanel;
class ChatInterface;
class AccountSettingsPanel;
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
    void ShowAccountSettings(bool show);

    InitialPanel* initialPanel;
    ServersPanel* serversPanel;
    AuthPanel* authPanel;
    ChatInterface* chatInterface;
    AccountSettingsPanel* accountSettingsPanel;
    WebSocketClient* wsClient;

private:
    void OnActivate(wxActivateEvent& event);
};

} // namespace client
