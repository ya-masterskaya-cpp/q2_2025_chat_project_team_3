#include <client/chatPanel.h>
#include <client/mainWidget.h>
#include <client/messageWidget.h>
#include <client/userListPanel.h>
#include <client/wsClient.h>
#include <client/message.h>
#include <client/messageView.h>

enum { ID_SEND = wxID_HIGHEST+30, ID_LEAVE, ID_RESIZE_TIMER };

wxBEGIN_EVENT_TABLE(ChatPanel, wxPanel)
    EVT_BUTTON(ID_SEND, ChatPanel::OnSend)
    EVT_BUTTON(ID_LEAVE, ChatPanel::OnLeave)
    EVT_TIMER(ID_RESIZE_TIMER, ChatPanel::OnResizeTimerTick)
    EVT_SIZE(ChatPanel::OnChatPanelSize)
wxEND_EVENT_TABLE()

ChatPanel::ChatPanel(MainWidget* parent)
    : wxPanel(parent, wxID_ANY),
      m_parent(parent),
      m_resizeTimer(this, ID_RESIZE_TIMER) // Initialize the timer with 'this' as the owner
{
    // Main sizer for the ChatPanel itself (horizontal layout: chat area | user list)
    m_mainSizer = new wxBoxSizer(wxHORIZONTAL);
    SetSizer(m_mainSizer);

    // --- Left side: Chat area (messages + input) ---
    m_chatSizer = new wxBoxSizer(wxVERTICAL);
    // Add the chat area to the main sizer, expanding horizontally and taking all available vertical space.
    m_mainSizer->Add(m_chatSizer, 1, wxEXPAND | wxALL, FromDIP(5));

    // --- NEW: Create the self-contained MessageView ---
    // The constructor takes 'this' (a ChatPanel*) so MessageView can call back
    // to get the wsClient.
    m_messageView = new MessageView(this);
    // Add the new message view directly to the chat sizer. It will fill the available space.
    m_chatSizer->Add(m_messageView, 1, wxEXPAND | wxALL, FromDIP(5));

    // --- REMOVED ---
    // The old wxScrolledWindow and its sizer are no longer needed.
    // m_messageContainer = new wxScrolledWindow(this, wxID_ANY, ...);
    // m_messageContainer->SetScrollRate(0, FromDIP(10));
    // m_messageContainer->SetBackgroundColour(...);
    // m_messageSizer = new wxBoxSizer(wxVERTICAL);
    // m_messageContainer->SetSizer(m_messageSizer);
    // m_chatSizer->Add(m_messageContainer, 1, wxEXPAND | wxALL, FromDIP(5));

    // --- Input area (text control + send/leave buttons) ---
    // This part remains exactly the same.
    auto* inputSizer = new wxBoxSizer(wxHORIZONTAL);
    m_input_ctrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
                                           wxDefaultPosition, wxDefaultSize,
                                           wxTE_PROCESS_ENTER | wxTE_MULTILINE);
    inputSizer->Add(m_input_ctrl, 1, wxEXPAND | wxRIGHT, FromDIP(5));

    wxButton* sendButton = new wxButton(this, ID_SEND, "Send");
    inputSizer->Add(sendButton, 0, wxALIGN_CENTER_VERTICAL);

    wxButton* leaveButton = new wxButton(this, ID_LEAVE, "Leave");
    inputSizer->Add(leaveButton, 0, wxALIGN_CENTER_VERTICAL);

    m_chatSizer->Add(inputSizer, 0, wxEXPAND | wxBOTTOM | wxLEFT | wxRIGHT, FromDIP(5));

    // --- Right side: User list ---
    // This part remains exactly the same.
    m_userListPanel = new UserListPanel(this);
    m_mainSizer->Add(m_userListPanel, 0, wxEXPAND | wxTOP | wxBOTTOM | wxRIGHT, FromDIP(5));

    // Final layout of the main sizer for the ChatPanel
    m_mainSizer->Layout();

    // --- REMOVED ---
    // The scroll events are now handled internally by MessageView. ChatPanel no longer
    // needs to listen for them.
    // m_messageContainer->Bind(wxEVT_SCROLLWIN_LINEUP,      &ChatPanel::OnScroll, this);
    // ... all other m_messageContainer->Bind calls ...
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
void ChatPanel::OnResizeTimerTick(wxTimerEvent& event) {
    // The timer has fired, meaning resizing has stopped (or paused) for 200ms.
    // Now, perform the re-wrapping for all messages using the last known accurate width.
    if (m_lastKnownWrapWidth > 20) {
        LOG_DEBUG << "Rewrapped all";
        //m_messageView->ReWrapAllMessages(m_lastKnownWrapWidth);
    }
}

void ChatPanel::OnSend(wxCommandEvent&) {
    if (!m_input_ctrl->IsEmpty()) {
        m_parent->wsClient->sendMessage(std::string(m_input_ctrl->GetValue().ToUTF8()));
        m_input_ctrl->Clear();
    }
}

void ChatPanel::OnLeave(wxCommandEvent&) {
    m_messageView->Clear();
    m_parent->wsClient->leaveRoom();
}
