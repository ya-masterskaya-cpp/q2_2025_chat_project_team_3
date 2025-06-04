#pragma once
#include <wx/wx.h>
#include <stdint.h>
class MainWidget;

constexpr int LAST_MESSAGES = 50;

struct Message{
    std::string from;
    std::string message;
    uint64_t timestamp;
};

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
