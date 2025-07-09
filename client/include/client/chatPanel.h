#pragma once

#include <wx/wx.h>

namespace client {

wxDECLARE_EVENT(wxEVT_SNAP_STATE_CHANGED, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_ASSIGN_MODERATOR, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_UNASSIGN_MODERATOR, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_TRANSFER_OWNERSHIP, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_DELETE_MESSAGE, wxCommandEvent);

class MainWidget;
class UserListPanel;
struct Message;
class MessageView;
class RoomHeaderPanel;
struct Room;
class RoomSettingsPanel;
struct User;
class TypingIndicatorPanel;

class ChatPanel : public wxPanel {
public:
    ChatPanel(wxWindow* parent);

    MainWidget* GetMainWidget() { return m_parent; }

    void ShowSettingsPanel();
    void ShowChatPanel();
    void SetRoomName(wxString name);
    int32_t GetRoomId();
    void ResetState();
    void SetCurrentUser(const User& user);
    const User& GetCurrentUser() const;
    void UserStartedTyping(const User& user);
    void UserStoppedTyping(const User& user);
    void UserJoin(const User& user);
    void UserLeft(const User& user);
    void UpdateUsername(int32_t userId, const wxString& newUsername);

    UserListPanel* m_userListPanel = nullptr;
    MessageView* m_messageView = nullptr;
    RoomHeaderPanel* m_roomHeaderPanel = nullptr;

    void InvalidateCaches();

private:
    MainWidget* m_parent = nullptr;
    wxTextCtrl* m_input_ctrl = nullptr;
    wxBoxSizer* m_mainSizer = nullptr;         // Horizontal sizer for the entire ChatPanel
    wxBoxSizer* m_chatSizer = nullptr;         // Vertical sizer for messages and input area
    wxButton*   m_jumpToPresentButton = nullptr;
    RoomSettingsPanel* m_roomSettingsPanel = nullptr;
    std::unique_ptr<User> m_currentUser;

    void OnSend(wxCommandEvent& event);
    void OnLeave(wxCommandEvent& event);
    void JumpToPresent(wxCommandEvent&);

    void OnSnapStateChanged(wxCommandEvent& event);

    int m_lastKnownWrapWidth = -1; // Stores the last calculated wrap width for optimization.
                                   // Initialized to -1 to indicate no width calculated yet.
    wxTimer m_resizeTimer;         // Timer for debouncing resize events for performance.

    wxTimer m_typingTimer;
    bool m_isTyping = false;
    TypingIndicatorPanel* m_typingIndicator = nullptr;

    // Event handlers for the message container's size and the debouncing timer.
    void OnChatPanelSize(wxSizeEvent& event);
    void OnResizeTimerTick(wxTimerEvent& event);
    void OnInputText(wxCommandEvent& event);
    void OnInputKeyDown(wxKeyEvent& event);
    void OnRoomRename(wxCommandEvent& event);
    void OnRoomDelete(wxCommandEvent& event);
    void OnRoomClose(wxCommandEvent& event);
    void OnAssignModerator(wxCommandEvent& event);
    void OnUnassignModerator(wxCommandEvent& event);
    void OnTransferOwnership(wxCommandEvent& event);
    void OnDeleteMessage(wxCommandEvent& event);
    void OnTypingTimer(wxTimerEvent& event);
    wxDECLARE_EVENT_TABLE();
};

} // namespace client
