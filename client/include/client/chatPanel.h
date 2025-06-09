#pragma once

#include <wx/wx.h>
#include <client/user.h>

class MainWidget;
class UserListPanel;
class Message;

class ChatPanel : public wxPanel {
public:
    ChatPanel(MainWidget* parent);
    void AppendMessage(const wxString& timestamp, const wxString& user, const wxString& msg, int64_t timestamp_val);
    void AppendMessages(const std::vector<Message>& messages, bool isHistoryResponse);
    void LoadInitialMessages();

    UserListPanel* m_userListPanel = nullptr;
private:
    MainWidget* m_parent = nullptr;
    wxTextCtrl* m_input_ctrl = nullptr;
    wxBoxSizer* m_mainSizer = nullptr;         // Horizontal sizer for the entire ChatPanel
    wxBoxSizer* m_chatSizer = nullptr;         // Vertical sizer for messages and input area
    wxScrolledWindow* m_messageContainer = nullptr; // Scrolled window that holds all message widgets
    wxBoxSizer* m_messageSizer = nullptr;      // Sizer within m_messageContainer to manage MessageWidgets

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

    void AddMessageInternal(const Message& msg, bool prepend);
    void RemoveMessages(int count, bool fromTop);
    void LoadOlderMessages();
    void LoadNewerMessages();
    void OnScroll(wxScrollWinEvent& event);
    void ScrollToBottom();

    bool m_loadingOlder = false;
    bool m_loadingNewer = false;

    static const int MAX_MESSAGES = 50;
    static const int LOAD_THRESHOLD = 300;
    static const int CHUNK_SIZE = 20;

    wxDECLARE_EVENT_TABLE();
};
