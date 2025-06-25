#pragma once

#include <wx/wx.h>
#include <client/user.h>

wxDECLARE_EVENT(wxEVT_SNAP_STATE_CHANGED, wxCommandEvent);

class MainWidget;
class UserListPanel;
struct Message;
class MessageView;
class RoomHeaderPanel;
struct Room;
class RoomSettingsPanel;

class ChatPanel : public wxPanel {
public:
    ChatPanel(MainWidget* parent);

    MainWidget* GetMainWidget() { return m_parent; }

    void ShowSettingsPanel();
    void ShowChatPanel();
    void SetRoomName(wxString name);
    int32_t GetRoomId();
    void ResetState();

    UserListPanel* m_userListPanel = nullptr;
    MessageView* m_messageView = nullptr;
    RoomHeaderPanel* m_roomHeaderPanel = nullptr;

private:
    MainWidget* m_parent = nullptr;
    wxTextCtrl* m_input_ctrl = nullptr;
    wxBoxSizer* m_mainSizer = nullptr;         // Horizontal sizer for the entire ChatPanel
    wxBoxSizer* m_chatSizer = nullptr;         // Vertical sizer for messages and input area
    wxButton*   m_jumpToPresentButton = nullptr;
    RoomSettingsPanel* m_roomSettingsPanel = nullptr;

    void OnSend(wxCommandEvent& event);
    void OnLeave(wxCommandEvent& event);
    void JumpToPresent(wxCommandEvent&);

    void OnSnapStateChanged(wxCommandEvent& event);

    int m_lastKnownWrapWidth = -1; // Stores the last calculated wrap width for optimization.
                                   // Initialized to -1 to indicate no width calculated yet.
    wxTimer m_resizeTimer;         // Timer for debouncing resize events for performance.

    // Event handlers for the message container's size and the debouncing timer.
    void OnChatPanelSize(wxSizeEvent& event);
    void OnResizeTimerTick(wxTimerEvent& event);
    void OnInputText(wxCommandEvent& event);
    void OnInputKeyDown(wxKeyEvent& event);
    void OnRoomRename(wxCommandEvent& event);
    void OnRoomDelete(wxCommandEvent& event);
    void OnRoomClose(wxCommandEvent& event);

    wxDECLARE_EVENT_TABLE();
};
