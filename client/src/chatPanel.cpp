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
#include <client/roomsPanel.h>

namespace client {

enum { ID_SEND = wxID_HIGHEST+30,
    ID_LEAVE,
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
    EVT_BUTTON(ID_SEND, ChatPanel::OnSendClicked)
    EVT_BUTTON(ID_LEAVE, ChatPanel::OnLeaveClicked)
    EVT_BUTTON(ID_JUMP_TO_PRESENT, ChatPanel::OnJumpToPresentClicked)
    //EVT_TIMER(ID_RESIZE_TIMER, ChatPanel::OnResizeTimerTick)
    //EVT_SIZE(ChatPanel::OnChatPanelSize)
    //EVT_LEFT_DOWN(ChatPanel::OnRoomHeaderClicked)
    EVT_COMMAND(wxID_ANY, wxEVT_ROOM_JOIN_REQUESTED, ChatPanel::OnRoomJoinRequested)
    EVT_COMMAND(wxID_ANY, wxEVT_ROOM_CREATE_REQUESTED, ChatPanel::OnRoomCreateRequested)
    EVT_COMMAND(wxID_ANY, wxEVT_LOGOUT_REQUESTED, ChatPanel::OnLogoutRequested)
wxEND_EVENT_TABLE()

ChatPanel::ChatPanel(MainWidget* parent)
    : wxPanel(parent, wxID_ANY),
      m_parent(parent),//m_resizeTimer(this, ID_RESIZE_TIMER) // Initialize the timer with 'this' as the owner
      m_typingTimer(this, ID_TYPING_TIMER),
      m_pendingJoinRoomId(std::nullopt),
      m_pendingLeaveRoomId(std::nullopt)
{
    m_currentUser = std::make_unique<User>();

    // 1. Главный горизонтальный сайзер для всей панели
    m_mainSizer = new wxBoxSizer(wxHORIZONTAL);
    SetSizer(m_mainSizer);

    // 2. Левая колонка: RoomsPanel
    m_roomsPanel = new RoomsPanel(this);
    m_mainSizer->Add(m_roomsPanel, 0, wxEXPAND | wxALL, FromDIP(5));

    // 3. Правая колонка: Панель-заглушка "Выберите комнату"
    m_placeholderPanel = new wxPanel(this, wxID_ANY);
    auto* placeholderSizer = new wxBoxSizer(wxVERTICAL);
    placeholderSizer->Add(new wxStaticText(m_placeholderPanel, wxID_ANY, "Select a room to start."), 0, wxALIGN_CENTER | wxALL, 20);
    m_placeholderPanel->SetSizer(placeholderSizer);
    m_mainSizer->Add(m_placeholderPanel, 1, wxEXPAND | wxALL, FromDIP(5));

    // 4. Правая колонка: Панель для области чата (изначально скрыта)
    m_chatAreaPanel = new wxPanel(this, wxID_ANY);

    // --- Заполняем m_chatAreaPanel виджетами ---
    auto* chatAreaMainSizer = new wxBoxSizer(wxHORIZONTAL);
    m_chatAreaPanel->SetSizer(chatAreaMainSizer);

    // Левая часть области чата (сообщения и ввод)
    auto* chatContentSizer = new wxBoxSizer(wxVERTICAL);
    chatAreaMainSizer->Add(chatContentSizer, 1, wxEXPAND | wxALL, FromDIP(5));

    m_roomHeaderPanel = new RoomHeaderPanel(m_chatAreaPanel);
    chatContentSizer->Add(m_roomHeaderPanel, 0, wxEXPAND | wxALL, FromDIP(5));

    m_messageView = new MessageView(m_chatAreaPanel, this);
    chatContentSizer->Add(m_messageView, 1, wxEXPAND | wxALL, FromDIP(5));

    m_typingIndicator = new TypingIndicatorPanel(m_chatAreaPanel);
    chatContentSizer->Add(m_typingIndicator, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(5));

    auto* inputSizer = new wxBoxSizer(wxHORIZONTAL);
    m_input_ctrl = new wxTextCtrl(m_chatAreaPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER | wxTE_MULTILINE);
    m_input_ctrl->Bind(wxEVT_TEXT, &ChatPanel::OnInputText, this);
    m_input_ctrl->Bind(wxEVT_KEY_DOWN, &ChatPanel::OnInputKeyDown, this);
    inputSizer->Add(m_input_ctrl, 1, wxEXPAND | wxRIGHT, FromDIP(5));
    inputSizer->Add(new wxButton(m_chatAreaPanel, ID_SEND, "Send"), 0, wxALIGN_CENTER_VERTICAL);
    inputSizer->Add(new wxButton(m_chatAreaPanel, ID_LEAVE, "Leave"), 0, wxALIGN_CENTER_VERTICAL);
    chatContentSizer->Add(inputSizer, 0, wxEXPAND | wxBOTTOM | wxLEFT | wxRIGHT, FromDIP(5));

    // Правая часть области чата (список пользователей)
    auto* userListSizer = new wxBoxSizer(wxVERTICAL);
    m_userListPanel = new UserListPanel(m_chatAreaPanel, this);
    userListSizer->Add(m_userListPanel, 1, wxEXPAND, 0);
    m_jumpToPresentButton = new wxButton(m_chatAreaPanel, ID_JUMP_TO_PRESENT, "Jump to present");
    userListSizer->Add(m_jumpToPresentButton, 0, wxEXPAND | wxTOP | wxRESERVE_SPACE_EVEN_IF_HIDDEN, FromDIP(5));
    chatAreaMainSizer->Add(userListSizer, 0, wxEXPAND | wxTOP | wxBOTTOM | wxRIGHT, FromDIP(5));
    m_jumpToPresentButton->Hide();

    // FromDIP(5). Добавляем панель чата в главный сайзер и скрываем ее
    m_mainSizer->Add(m_chatAreaPanel, 1, wxEXPAND | wxALL, FromDIP(5));
    m_mainSizer->Show(m_chatAreaPanel, false);

    // Событие от MessageView
    Bind(wxEVT_SNAP_STATE_CHANGED, &ChatPanel::OnSnapStateChanged, this);

    // События от RoomSettingsPanel
    Bind(wxEVT_ROOM_RENAME, &ChatPanel::OnRoomRename, this);
    Bind(wxEVT_ROOM_DELETE, &ChatPanel::OnRoomDelete, this);
    Bind(wxEVT_ROOM_CLOSE, &ChatPanel::OnRoomCloseSettings, this);

    // События от UserListPanel
    Bind(wxEVT_ASSIGN_MODERATOR, &ChatPanel::OnAssignModerator, this);
    Bind(wxEVT_UNASSIGN_MODERATOR, &ChatPanel::OnUnassignModerator, this);
    Bind(wxEVT_TRANSFER_OWNERSHIP, &ChatPanel::OnTransferOwnership, this);

    // Событие от MessageWidget (через MessageView)
    Bind(wxEVT_DELETE_MESSAGE, &ChatPanel::OnDeleteMessage, this);

    // Таймер
    Bind(wxEVT_TIMER, &ChatPanel::OnTypingTimer, this, ID_TYPING_TIMER);

    // Клик по заголовку
    m_roomHeaderPanel->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent& event) {
        ShowSettingsPanel();
        event.Skip();
        });
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

void ChatPanel::OnSendClicked(wxCommandEvent&) {
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

void ChatPanel::OnLeaveClicked(wxCommandEvent&) {
    if (m_roomSettingsPanel) {
        ShowChatPanel();
    }
    m_pendingLeaveRoomId = GetCurrentRoomId();
    m_pendingJoinRoomId = std::nullopt;

    m_roomsPanel->Enable(false);
    m_parent->wsClient->leaveRoom();
    ShowPlaceholder();
}

void ChatPanel::OnJumpToPresentClicked(wxCommandEvent&) {
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

    m_roomSettingsPanel = new RoomSettingsPanel(m_chatAreaPanel,
        m_roomHeaderPanel->GetLabel(),
        m_roomHeaderPanel->GetRoomId());

    auto* chatContentSizer = dynamic_cast<wxBoxSizer*>(m_messageView->GetContainingSizer());
    if (chatContentSizer) {
        chatContentSizer->Replace(m_roomHeaderPanel, m_roomSettingsPanel);
        m_roomHeaderPanel->Hide();
        m_roomSettingsPanel->Show();
        m_chatAreaPanel->Layout();
    }
}

void ChatPanel::ShowChatPanel() {
    if (!m_roomSettingsPanel) return;

    auto* chatContentSizer = dynamic_cast<wxBoxSizer*>(m_messageView->GetContainingSizer());
    if (chatContentSizer) {
        chatContentSizer->Replace(m_roomSettingsPanel, m_roomHeaderPanel);
        m_roomHeaderPanel->Show();
        m_roomSettingsPanel->Destroy();
        m_roomSettingsPanel = nullptr;
        m_chatAreaPanel->Layout();
    }
}

int32_t ChatPanel::GetCurrentRoomId() {
    if (m_roomHeaderPanel && m_roomHeaderPanel->IsShown()) {
        return m_roomHeaderPanel->GetRoomId();
    }
    return -1;
}

void ChatPanel::ResetState() {
    ShowPlaceholder();
    m_roomsPanel->UpdateRoomList({});
    m_currentUser = std::make_unique<User>();
    m_typingTimer.Stop();
    m_isTyping = false;
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

void ChatPanel::PopulateInitialRoomList(const std::vector<Room*>& rooms) {
    m_roomsPanel->UpdateRoomList(rooms);
}

void ChatPanel::DisplayChatForRoom(const Room& room, std::vector<User> users) {
    if (!m_chatAreaPanel->IsShown()) {
        m_mainSizer->Show(m_placeholderPanel, false);
        m_placeholderPanel->Hide();

        m_mainSizer->Show(m_chatAreaPanel, true);
        m_chatAreaPanel->Show();
    }
    m_roomHeaderPanel->SetRoom(room);
    m_userListPanel->SetUserList(std::move(users));
    m_messageView->Start();
    Layout();
}

void ChatPanel::ShowPlaceholder() {
    if (m_placeholderPanel->IsShown()) {
        return;
    }
    m_mainSizer->Show(m_chatAreaPanel, false);
    m_chatAreaPanel->Hide();
    m_mainSizer->Show(m_placeholderPanel, true);
    m_placeholderPanel->Show();

    m_messageView->Clear();
    m_userListPanel->Clear();
    m_typingIndicator->Clear();
    m_input_ctrl->Clear();
    m_roomHeaderPanel->Clear();
    Layout();
}

void ChatPanel::AddRoomToList(Room* room) {
    m_roomsPanel->AddRoom(room);
}

void ChatPanel::RemoveRoomFromList(int32_t room_id) {
    m_roomsPanel->RemoveRoom(room_id);
}

void ChatPanel::RenameRoomInList(int32_t room_id, const wxString& name) {
    m_roomsPanel->RenameRoom(room_id, name);
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

void ChatPanel::OnRoomCloseSettings([[maybe_unused]] wxCommandEvent& event) {
    ShowChatPanel();
}

void ChatPanel::OnAssignModerator(wxCommandEvent& event) {
    if (!m_parent || !m_parent->wsClient) return;

    int32_t roomId = GetCurrentRoomId();
    int32_t userId = event.GetInt();
    m_parent->wsClient->assignRole(roomId, userId, chat::UserRights::MODERATOR);
}

void ChatPanel::OnUnassignModerator(wxCommandEvent& event) {
    if (!m_parent || !m_parent->wsClient) return;

    int32_t roomId = GetCurrentRoomId();
    int32_t userId = event.GetInt();
    m_parent->wsClient->assignRole(roomId, userId, chat::UserRights::REGULAR);
}

void ChatPanel::OnTransferOwnership(wxCommandEvent& event) {
    if (!m_parent || !m_parent->wsClient) return;

    int32_t roomId = GetCurrentRoomId();
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
            OnSendClicked(sendEvent);
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

void ChatPanel::OnRoomJoinRequested(wxCommandEvent& event) {
    if (m_pendingLeaveRoomId.has_value() || m_pendingJoinRoomId.has_value()) {
        return;
    }
    int32_t targetRoomId = event.GetInt();
    int32_t currentRoomId = GetCurrentRoomId();
    if (m_chatAreaPanel->IsShown() && currentRoomId == targetRoomId) {
        return;
    }
    m_pendingJoinRoomId = targetRoomId;

    ShowPlaceholder();
    m_roomsPanel->Enable(false);

    if (m_chatAreaPanel->IsShown()) {
        m_pendingLeaveRoomId = currentRoomId;
        m_parent->wsClient->leaveRoom();
    }
    else {
        m_parent->wsClient->joinRoom(*m_pendingJoinRoomId);
    }
}

void ChatPanel::OnJoinRoomSuccess(std::vector<User> users) {
    m_roomsPanel->Enable(true);

    if (!m_pendingJoinRoomId.has_value()) {
        return;
    }

    int32_t joinedRoomId = *m_pendingJoinRoomId;
    m_pendingJoinRoomId = std::nullopt;

    auto roomOpt = FindRoomById(joinedRoomId);
    if (roomOpt) {
        DisplayChatForRoom(roomOpt.value(), std::move(users));
    }
    else {
        m_parent->ShowPopup(wxString::Format("Internal error: Could not find room data for ID %d", joinedRoomId), wxICON_ERROR);
    }
}

void ChatPanel::OnJoinRoomFailure() {
    m_roomsPanel->Enable(true);
    m_pendingJoinRoomId = std::nullopt;
}

void ChatPanel::OnLeaveRoomResponse(bool success) {
    if (!m_pendingLeaveRoomId.has_value()) {
        return;
    }
    m_pendingLeaveRoomId = std::nullopt;

    if (success && m_pendingJoinRoomId.has_value()) {
        m_parent->wsClient->joinRoom(*m_pendingJoinRoomId);
    }
    else {
        m_parent->ShowPopup("Failed to leave previous room.", wxICON_ERROR);
        m_roomsPanel->Enable(true);
        m_pendingJoinRoomId = std::nullopt;
    }
}

std::optional<Room> ChatPanel::FindRoomById(int32_t room_id) {
    return m_roomsPanel->FindRoomInListById(room_id);
}

void ChatPanel::OnRoomCreateRequested(wxCommandEvent& event) {
    m_parent->wsClient->createRoom(event.GetString().utf8_string());
}

void ChatPanel::OnLogoutRequested(wxCommandEvent& event) {
    m_parent->wsClient->logout();
}

} // namespace client
