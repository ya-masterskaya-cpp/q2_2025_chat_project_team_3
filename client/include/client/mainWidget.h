#pragma once

#include <wx/wx.h>
#include <wx/snglinst.h>

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
    ~MainWidget();

    void ShowPopup(const wxString& msg, long icon = wxICON_INFORMATION);

    void ShowInitial();
    void ShowServers();
    void ShowAuth();
    void ShowRooms();
    void ShowChat(std::vector<User> users);
    bool IsAnotherInstanceRunning() const; 

    InitialPanel* initialPanel;
    ServersPanel* serversPanel;
    AuthPanel* authPanel;
    RoomsPanel* roomsPanel;
    ChatPanel* chatPanel;
    WebSocketClient* wsClient;
    
private:
    wxSingleInstanceChecker* m_checker = nullptr;
};
