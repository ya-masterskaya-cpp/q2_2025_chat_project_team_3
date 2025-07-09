#include <client/chatPanel.h>
#include <client/mainWidget.h>
#include <client/messageWidget.h>
#include <client/userListPanel.h>
#include <client/wsClient.h>
#include <client/message.h>
#include <client/messageView.h>
#include <client/textUtil.h>
#include <common/utils/limits.h>
#include <client/roomHeaderPanel.h>
#include <client/roomSettingsPanel.h>
#include <client/user.h>
#include <client/typingIndicatorPanel.h>

namespace client {

enum { ID_SEND = wxID_HIGHEST+30,
//  ID_LEAVE,
    ID_JUMP_TO_PRESENT,
    ID_RENAME_ROOM,
    ID_DELETE_ROOM,
    ID_BACK_FROM_SETTINGS,
    ID_TYPING_TIMER
};

wxDEFINE_EVENT(wxEVT_SNAP_STATE_CHANGED, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_ASSIGN_MODERATOR, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_UNASSIGN_MODERATOR, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_TRANSFER_OWNERSHIP, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_DELETE_MESSAGE, wxCommandEvent);

wxBEGIN_EVENT_TABLE(ChatPanel, wxPanel)
    EVT_BUTTON(ID_SEND, ChatPanel::OnSend)
    //EVT_BUTTON(ID_LEAVE, ChatPanel::OnLeave)
    EVT_BUTTON(ID_JUMP_TO_PRESENT, ChatPanel::JumpToPresent)
    //EVT_TIMER(ID_RESIZE_TIMER, ChatPanel::OnResizeTimerTick)
    //EVT_SIZE(ChatPanel::OnChatPanelSize)
    //EVT_LEFT_DOWN(ChatPanel::OnRoomHeaderClicked)
wxEND_EVENT_TABLE()

ChatPanel::ChatPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY),
      m_parent(static_cast<MainWidget*>(parent->GetParent())),//m_resizeTimer(this, ID_RESIZE_TIMER) // Initialize the timer with 'this' as the owner
      m_typingTimer(this, ID_TYPING_TIMER)
{
    m_currentUser = std::make_unique<User>();

    // Main sizer for the ChatPanel itself (horizontal layout: chat area | user list)
    m_mainSizer = new wxBoxSizer(wxHORIZONTAL);
    SetSizer(m_mainSizer);

    // --- Left side: Chat area (header + messages + input) ---
    m_chatSizer = new wxBoxSizer(wxVERTICAL);
    // Add the chat area to the main sizer, expanding horizontally and taking all available vertical space.
    m_mainSizer->Add(m_chatSizer, 1, wxEXPAND | wxALL, FromDIP(5));

    // Add Room Header Panel
    m_roomHeaderPanel = new RoomHeaderPanel(this);

    m_chatSizer->Add(m_roomHeaderPanel, 0, wxEXPAND | wxALL, FromDIP(5));

    m_messageView = new MessageView(this);
    // Add the new message view directly to the chat sizer. It will fill the available space.
    m_chatSizer->Add(m_messageView, 1, wxEXPAND | wxALL, FromDIP(5));

    // --- Typing Indicator ---
    m_typingIndicator = new TypingIndicatorPanel(this);
    m_chatSizer->Add(m_typingIndicator, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(5));

    // --- Input area (text control + send/leave buttons) ---
    auto* inputSizer = new wxBoxSizer(wxHORIZONTAL);
    m_input_ctrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
                                           wxDefaultPosition, wxDefaultSize,
                                           wxTE_PROCESS_ENTER | wxTE_MULTILINE);
    m_input_ctrl->Bind(wxEVT_TEXT, &ChatPanel::OnInputText, this);
    m_input_ctrl->Bind(wxEVT_KEY_DOWN, &ChatPanel::OnInputKeyDown, this);
    inputSizer->Add(m_input_ctrl, 1, wxEXPAND | wxRIGHT, FromDIP(5));

    wxButton* sendButton = new wxButton(this, ID_SEND, "Send");
    inputSizer->Add(sendButton, 0, wxALIGN_CENTER_VERTICAL);

    //wxButton* leaveButton = new wxButton(this, ID_LEAVE, "Leave");
    //inputSizer->Add(leaveButton, 0, wxALIGN_CENTER_VERTICAL);

    m_chatSizer->Add(inputSizer, 0, wxEXPAND | wxBOTTOM | wxLEFT | wxRIGHT, FromDIP(5));

    // --- Right side: User list and jump button ---
    auto* rightSizer = new wxBoxSizer(wxVERTICAL);
    
    m_userListPanel = new UserListPanel(this);
    rightSizer->Add(m_userListPanel, 1, wxEXPAND, 0);

    m_jumpToPresentButton = new wxButton(this, ID_JUMP_TO_PRESENT, "Jump to present");
    rightSizer->Add(m_jumpToPresentButton, 0, wxEXPAND | wxTOP | wxRESERVE_SPACE_EVEN_IF_HIDDEN, FromDIP(5));

    m_mainSizer->Add(rightSizer, 0, wxEXPAND | wxTOP | wxBOTTOM | wxRIGHT, FromDIP(5));

    // Final layout of the main sizer for the ChatPanel
    m_mainSizer->Layout();
    m_jumpToPresentButton->Hide();

    Bind(wxEVT_SNAP_STATE_CHANGED, &ChatPanel::OnSnapStateChanged, this);
    
    m_roomHeaderPanel->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent& event) {
        ShowSettingsPanel();
        event.Skip();
    });

    Bind(wxEVT_ROOM_RENAME, &ChatPanel::OnRoomRename, this);
    Bind(wxEVT_ROOM_DELETE, &ChatPanel::OnRoomDelete, this);
    Bind(wxEVT_ROOM_CLOSE, &ChatPanel::OnRoomClose, this);
    Bind(wxEVT_ASSIGN_MODERATOR, &ChatPanel::OnAssignModerator, this);
    Bind(wxEVT_UNASSIGN_MODERATOR, &ChatPanel::OnUnassignModerator, this);
    Bind(wxEVT_TRANSFER_OWNERSHIP, &ChatPanel::OnTransferOwnership, this);
    Bind(wxEVT_DELETE_MESSAGE, &ChatPanel::OnDeleteMessage, this);
    Bind(wxEVT_TIMER, &ChatPanel::OnTypingTimer, this, ID_TYPING_TIMER);
}

void ChatPanel::InvalidateCaches() {
    m_messageView->InvalidateCaches();
}

// Event handler for when the message container (wxScrolledWindow) changes size.
void ChatPanel::OnChatPanelSize(wxSizeEvent& event) {
    // Get the current visible client width of the scrolled window.
    int newClientWidth = m_messageView->GetClientSize().x;
    
    // Calculate the actual width available for text wrapping.
    // This involves subtracting all horizontal padding and margins between the messageContainer's
    // client edge and the actual text content within wxStaticText.
    // FromDIP(3) * 2: Left/right padding for MessageWidget itself in m_messageSizer.
    // FromDIP(5) * 2: Left/right padding for m_messageStaticText inside MessageWidget's sizer.
    int calculatedWrapWidth = newClientWidth - (FromDIP(3) * 2) - (FromDIP(5) * 2);

    // Basic sanity check: ensure wrapWidth is positive.
    if (calculatedWrapWidth < 1) {
        calculatedWrapWidth = 1; 
    }

    if(m_lastKnownWrapWidth != calculatedWrapWidth) {
        m_lastKnownWrapWidth = calculatedWrapWidth;
        // --- Debounce Logic ---
        m_resizeTimer.Stop();
        m_resizeTimer.StartOnce(200);
    }

    event.Skip(); // Important: allow the event to propagate further if needed.
}

// Event handler for the debouncing timer. This is where the actual re-wrapping happens.
void ChatPanel::OnResizeTimerTick([[maybe_unused]] wxTimerEvent& event) {
    // The timer has fired, meaning resizing has stopped (or paused) for 200ms.
    // Now, perform the re-wrapping for all messages using the last known accurate width.
    if (m_lastKnownWrapWidth > 20) {
        LOG_DEBUG << "Rewrapped all";
        //m_messageView->ReWrapAllMessages(m_lastKnownWrapWidth);
    }
}

void ChatPanel::OnSend(wxCommandEvent&) {
    if (!m_input_ctrl->IsEmpty()) {
        if (m_isTyping) {
            m_typingTimer.Stop();
            m_isTyping = false;
            m_parent->wsClient->sendTypingStop();
        }
        m_parent->wsClient->sendMessage(m_input_ctrl->GetValue().utf8_string());
        m_input_ctrl->Clear();
    }
}

void ChatPanel::OnLeave(wxCommandEvent&) {
    if (m_roomSettingsPanel) {
        ShowChatPanel();
    }
    m_messageView->Clear();
    m_parent->wsClient->leaveRoom();
}

void ChatPanel::JumpToPresent(wxCommandEvent&) {
    m_messageView->JumpToPresent();
}

void ChatPanel::OnSnapStateChanged(wxCommandEvent& event) {
    bool isSnapped = event.GetInt() == 1;
    if(isSnapped) {
        m_jumpToPresentButton->Hide();
    } else {
        m_jumpToPresentButton->Show();
        m_jumpToPresentButton->Refresh();
    }
    
    //m_jumpToPresentButton->Enable(!isSnapped);
    Layout();
}

void ChatPanel::OnInputText(wxCommandEvent& event) {
	// Check if the input control is not empty and the user is not already typing.
    if (!m_input_ctrl->IsEmpty() && !m_isTyping) {
        m_isTyping = true;
        m_parent->wsClient->sendTypingStart();
    }
	// Restart the typing timer to reset the typing state after 2 seconds of inactivity.
    m_typingTimer.StartOnce(2000);

    event.Skip();
    TextUtil::LimitTextLength(m_input_ctrl, common::limits::MAX_MESSAGE_LENGTH);
}

void ChatPanel::ShowSettingsPanel() {
    if (m_roomSettingsPanel) return;
    m_roomSettingsPanel = new RoomSettingsPanel(this,
                                              m_roomHeaderPanel->GetLabel(),
                                              m_roomHeaderPanel->GetRoomId());
    
    m_roomHeaderPanel->Hide();
    m_chatSizer->Replace(m_roomHeaderPanel, m_roomSettingsPanel);
    m_roomSettingsPanel->Show();
    Layout();
    m_messageView->RefreshAll();
}

void ChatPanel::ShowChatPanel() {
    if (!m_roomSettingsPanel) return;

    m_roomHeaderPanel->Show();
    m_roomSettingsPanel->Hide();
    m_chatSizer->Replace(m_roomSettingsPanel, m_roomHeaderPanel);
    m_roomSettingsPanel->Destroy();
    m_roomSettingsPanel = nullptr;
    Layout();
    m_messageView->RefreshAll();
}

void ChatPanel::SetRoomName(wxString name) {
    m_roomHeaderPanel->SetLabel(name);
}

int32_t ChatPanel::GetRoomId() {
    return m_roomHeaderPanel->GetRoomId();
}

void ChatPanel::ResetState() {
    ShowChatPanel();
    m_input_ctrl->Clear();
    m_messageView->Clear();
    m_userListPanel->Clear();
    m_jumpToPresentButton->Hide();
    m_typingTimer.Stop();
    m_isTyping = false;
    m_typingIndicator->Clear();
    Layout();
}

void ChatPanel::SetCurrentUser(const User& user) {
    *m_currentUser = user;
}

const User& ChatPanel::GetCurrentUser() const {
    return *m_currentUser;
}

void ChatPanel::UserStartedTyping(const User& user) {
    if (m_currentUser->id == user.id) {
        return;
	}
    m_typingIndicator->AddTypingUser(user.username);
}

void ChatPanel::UserStoppedTyping(const User& user) {
    if (m_currentUser->id == user.id) {
        return;
    }
	m_typingIndicator->RemoveTypingUser(user.username);
}

void ChatPanel::UserJoin(const User& user) {
    m_userListPanel->AddUser(user);
}

void ChatPanel::UserLeft(const User& user) {
    m_userListPanel->RemoveUser(user.id);
    UserStoppedTyping(user);
}

void ChatPanel::UpdateUsername(int32_t userId, const wxString& newUsername) {
    if (m_userListPanel) {
        m_userListPanel->UpdateUsername(userId, newUsername);
    }
    
    m_messageView->UpdateUsername(userId, newUsername);
}

void ChatPanel::OnRoomRename(wxCommandEvent &event) {
    if (!m_parent || !m_parent->wsClient) return;
    wxString newName = event.GetString();
    int32_t roomId = event.GetInt();
    m_parent->wsClient->renameRoom(roomId, std::string(newName.ToUTF8()));
}

void ChatPanel::OnRoomDelete(wxCommandEvent& event) {
    if (!m_parent || !m_parent->wsClient) return;
    int32_t roomId = event.GetInt();
    m_parent->wsClient->deleteRoom(roomId);
}

void ChatPanel::OnRoomClose([[maybe_unused]] wxCommandEvent& event) {
    ShowChatPanel();
}

void ChatPanel::OnAssignModerator(wxCommandEvent& event) {
    if (!m_parent || !m_parent->wsClient) return;

    int32_t roomId = GetRoomId();
    int32_t userId = event.GetInt();
    m_parent->wsClient->assignRole(roomId, userId, chat::UserRights::MODERATOR);
}

void ChatPanel::OnUnassignModerator(wxCommandEvent& event) {
    if (!m_parent || !m_parent->wsClient) return;

    int32_t roomId = GetRoomId();
    int32_t userId = event.GetInt();
    m_parent->wsClient->assignRole(roomId, userId, chat::UserRights::REGULAR);
}

void ChatPanel::OnTransferOwnership(wxCommandEvent& event) {
    if (!m_parent || !m_parent->wsClient) return;

    int32_t roomId = GetRoomId();
    int32_t userId = event.GetInt();
    m_parent->wsClient->assignRole(roomId, userId, chat::UserRights::OWNER);
}

void ChatPanel::OnDeleteMessage(wxCommandEvent& event) {
    if (!m_parent || !m_parent->wsClient) {
        return;
    }
    int32_t messageId = event.GetInt();
    m_parent->wsClient->deleteMessage(messageId);
}

void ChatPanel::OnInputKeyDown(wxKeyEvent& event) {
    int keyCode = event.GetKeyCode();

    // Check if the pressed key was Enter (on the main keyboard or the numpad).
    if (keyCode == WXK_RETURN || keyCode == WXK_NUMPAD_ENTER) {
        // Check if NO modifiers (Shift, Ctrl, Alt) are being held down.
        if (!event.ShiftDown() && !event.ControlDown() && !event.AltDown()) {
            // --- Case 1: Plain Enter was pressed ---
            wxCommandEvent sendEvent(wxEVT_BUTTON, ID_SEND);
            OnSend(sendEvent);
        } else {
            // --- Case 2: Enter was pressed with a modifier (e.g., Shift+Enter) ---
            // Allow the event to be processed by the default handler.
            event.Skip();
        }
    } else {
        // --- Case 3: Any other key was pressed ---
        // Let the default handler process the event as usual (e.g., typing a character).
        event.Skip();
    }
}

void ChatPanel::OnTypingTimer([[maybe_unused]] wxTimerEvent& event) {
    m_isTyping = false;
    m_parent->wsClient->sendTypingStop();
}

} // namespace client
