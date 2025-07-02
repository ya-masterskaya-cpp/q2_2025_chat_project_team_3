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
class RoomsPanel;

class ChatPanel : public wxPanel {
public:
    ChatPanel(MainWidget* parent);

    MainWidget* GetMainWidget() { return m_parent; }

    // Public API for control by wsClient
    void PopulateInitialRoomList(const std::vector<Room*>& rooms);
    void AddRoomToList(Room* room);
    void RemoveRoomFromList(int32_t room_id);
    void RenameRoomInList(int32_t room_id, const wxString& name);

    // Public API for responses by server
    void OnLeaveRoomResponse(bool success);
    void OnJoinRoomSuccess(std::vector<User> users);
    void OnJoinRoomFailure();
    void ShowPlaceholder();

    // Broadcast notifications
    void UserJoin(const User& user);
    void UserLeft(const User& user);
    void UserStartedTyping(const User& user);
    void UserStoppedTyping(const User& user);

    // State control
    void ResetState();
    void SetCurrentUser(const User& user);
    const User& GetCurrentUser() const;

    // Public members for rights access
    UserListPanel* m_userListPanel = nullptr;
    MessageView* m_messageView = nullptr;
    RoomHeaderPanel* m_roomHeaderPanel = nullptr;
    void InvalidateCaches();


    std::optional<Room> FindRoomById(int32_t room_id);


private:
    // Private methods for controls GUI
    void DisplayChatForRoom(const Room& room, std::vector<User> users);
    void ShowSettingsPanel();
    void ShowChatPanel();
    int32_t GetCurrentRoomId();

    // Handlers by childs
    // From RoomsPanel
    void OnRoomJoinRequested(wxCommandEvent& event);
    void OnRoomCreateRequested(wxCommandEvent& event);
    void OnLogoutRequested(wxCommandEvent& event);
    // From buttons in ChatArea
    void OnSendClicked(wxCommandEvent& event);
    void OnLeaveClicked(wxCommandEvent& event);
    void OnJumpToPresentClicked(wxCommandEvent& event);
    // From input controls
    void OnInputText(wxCommandEvent& event);
    void OnInputKeyDown(wxKeyEvent& event);
    // From MessageView
    void OnSnapStateChanged(wxCommandEvent& event);
    void OnDeleteMessage(wxCommandEvent& event);
    // From RoomSettingsPanel
    void OnRoomRename(wxCommandEvent& event);
    void OnRoomDelete(wxCommandEvent& event);
    void OnRoomCloseSettings(wxCommandEvent& event);
    // From UserListPanel
    void OnAssignModerator(wxCommandEvent& event);
    void OnUnassignModerator(wxCommandEvent& event);
    void OnTransferOwnership(wxCommandEvent& event);
    // From таймеров
    void OnTypingTimer(wxTimerEvent& event);

    // Class members
    MainWidget* m_parent;
    wxBoxSizer* m_mainSizer;

    // Child Panels
    RoomsPanel* m_roomsPanel;
    wxPanel* m_placeholderPanel;
    wxPanel* m_chatAreaPanel;

    // State transition btw rooms
    std::optional<int32_t> m_pendingLeaveRoomId;
    std::optional<int32_t> m_pendingJoinRoomId;

    // Other members
    std::unique_ptr<User> m_currentUser;
    wxTextCtrl* m_input_ctrl = nullptr;
    wxButton* m_jumpToPresentButton = nullptr;
    RoomSettingsPanel* m_roomSettingsPanel = nullptr;
    TypingIndicatorPanel* m_typingIndicator = nullptr;
    wxTimer m_typingTimer;
    bool m_isTyping = false;

    int m_lastKnownWrapWidth = -1; // Stores the last calculated wrap width for optimization.
                                   // Initialized to -1 to indicate no width calculated yet.
    wxTimer m_resizeTimer;         // Timer for debouncing resize events for performance.


    // Event handlers for the message container's size and the debouncing timer.
    void OnChatPanelSize(wxSizeEvent& event);
    void OnResizeTimerTick(wxTimerEvent& event);
    wxDECLARE_EVENT_TABLE();
};

} // namespace client
