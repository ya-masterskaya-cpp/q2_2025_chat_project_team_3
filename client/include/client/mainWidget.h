#pragma once
#include <wx/wx.h>
#include <client/wsClient.h>
#include <client/authPanel.h>
#include <client/roomsPanel.h>
#include <client/chatPanel.h>

class MainWidget : public wxFrame {
public:
    MainWidget();
    void ShowPopup(const wxString& msg, long icon = wxICON_INFORMATION);

    void ShowAuth();
    void ShowRooms();
    void ShowChat();

    AuthPanel* authPanel;
    RoomsPanel* roomsPanel;
    ChatPanel* chatPanel;
    WebSocketClient* wsClient;
};
