#pragma once
#include <wx/wx.h>
class MainWidget;

constexpr int LAST_MESSAGES = 50;

class ChatPanel : public wxPanel {
public:
    ChatPanel(MainWidget* parent);
    void AppendMessage(const wxString& msg);

    wxTextCtrl* chatBox;
    wxTextCtrl* messageInput;
    wxButton* sendButton;
    wxButton* leaveButton;
private:
    void OnSend(wxCommandEvent&);
    void OnLeave(wxCommandEvent&);
    MainWidget* mainWin;
    wxDECLARE_EVENT_TABLE();
};
