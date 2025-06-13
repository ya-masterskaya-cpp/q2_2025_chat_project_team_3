#pragma once

#include <wx/wx.h>

class AuthPanel;
class RoomsPanel;
class ChatPanel;
class WebSocketClient;
struct User;

class MainWidget : public wxFrame {
public:
    MainWidget();
    void ShowPopup(const wxString& msg, long icon = wxICON_INFORMATION);

    void ShowAuth();
    void ShowRooms();
    void ShowChat(std::vector<User> users);

    AuthPanel* authPanel;
    RoomsPanel* roomsPanel;
    ChatPanel* chatPanel;
    WebSocketClient* wsClient;
};
