#pragma once
#include <wx/wx.h>
#include <stdint.h>

class MainWidget;
class UserListPanel;

constexpr int LAST_MESSAGES = 50;

struct Message{
    std::string from;
    std::string message;
    uint64_t timestamp;
};

class ChatPanel : public wxPanel {
public:
    ChatPanel(MainWidget* parent);
    void AppendMessage(const wxString& timestamp, const wxString& user, const wxString& msg);
    void UpdateUserList(const std::vector<wxString>& users);

private:
    MainWidget* m_parent = nullptr;
    wxTextCtrl* m_input_ctrl = nullptr;
    wxBoxSizer* m_mainSizer = nullptr;         // Horizontal sizer for the entire ChatPanel
    wxBoxSizer* m_chatSizer = nullptr;         // Vertical sizer for messages and input area
    wxScrolledWindow* m_messageContainer = nullptr; // Scrolled window that holds all message widgets
    wxBoxSizer* m_messageSizer = nullptr;      // Sizer within m_messageContainer to manage MessageWidgets

    UserListPanel* m_userListPanel = nullptr;

    void OnSend(wxCommandEvent& event);
    void OnLeave(wxCommandEvent& event);
    

    int m_lastKnownWrapWidth = -1; // Stores the last calculated wrap width for optimization.
                                   // Initialized to -1 to indicate no width calculated yet.
    wxTimer m_resizeTimer;         // Timer for debouncing resize events for performance.

    // Event handlers for the message container's size and the debouncing timer.
    void OnChatPanelSize(wxSizeEvent& event);
    void OnResizeTimerTick(wxTimerEvent& event);

    // Helper function to re-wrap and re-layout all messages.
    void ReWrapAllMessages(int wrapWidth);

    wxDECLARE_EVENT_TABLE();
};



