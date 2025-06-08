#include <client/chatPanel.h>
#include <client/mainWidget.h>
#include <client/messageWidget.h>
#include <client/userListPanel.h>
#include <client/wsClient.h>
#include <client/message.h>

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

    // Messages container: A wxScrolledWindow to hold all individual message widgets.
    m_messageContainer = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                             wxVSCROLL); // Only vertical scrollbar needed for chat history
    m_messageContainer->SetScrollRate(0, FromDIP(10)); // Set vertical scroll speed
    //m_messageContainer->SetBackgroundColour(*wxBLUE); // For debugging the scrolled window's width
    m_messageContainer->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));

    m_messageSizer = new wxBoxSizer(wxVERTICAL); // Sizer inside the scrolled window for messages
    m_messageContainer->SetSizer(m_messageSizer);

    // Add the message container to the chat sizer, expanding to fill available space.
    m_chatSizer->Add(m_messageContainer, 1, wxEXPAND | wxALL, FromDIP(5));

    // --- Input area (text control + send\leave buttons) ---
    auto* inputSizer = new wxBoxSizer(wxHORIZONTAL);
    m_input_ctrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
                                           wxDefaultPosition, wxDefaultSize,
                                           wxTE_PROCESS_ENTER | wxTE_MULTILINE); // Process ENTER to send
    inputSizer->Add(m_input_ctrl, 1, wxEXPAND | wxRIGHT, FromDIP(5)); // Input text control expands horizontally

    wxButton* sendButton = new wxButton(this, ID_SEND, "Send");
    inputSizer->Add(sendButton, 0, wxALIGN_CENTER_VERTICAL); // Send button does not expand

    wxButton* leaveButton = new wxButton(this, ID_LEAVE, "Leave");
    inputSizer->Add(leaveButton, 0, wxALIGN_CENTER_VERTICAL); // Leave button does not expand

    // Add the input sizer to the chat sizer, expanding horizontally.
    m_chatSizer->Add(inputSizer, 0, wxEXPAND | wxBOTTOM | wxLEFT | wxRIGHT, FromDIP(5));

    // --- Right side: User list ---
    m_userListPanel = new UserListPanel(this);
    m_mainSizer->Add(m_userListPanel, 0, wxEXPAND | wxTOP | wxBOTTOM | wxRIGHT, FromDIP(5));

    // Final layout of the main sizer for the ChatPanel
    m_mainSizer->Layout();
}

void ChatPanel::AppendMessages(const std::vector<Message>& messages) {
    // Freeze the scrolled window to prevent immediate redraws and layout recalculations
    m_messageContainer->Freeze();

    for(const auto& m : messages) {
        // Create a new MessageWidget instance within the scrolled window.
        auto* msgWidget = new MessageWidget(m_messageContainer, m.user, m.msg, wxString::FromUTF8(WebSocketClient::formatMessageTimestamp(m.timestamp)), m_lastKnownWrapWidth);
 
        // Add the new message widget to the message sizer.
        // Use wxEXPAND to ensure it takes full horizontal width. Proportion 0 as it shouldn't grow vertically itself.
        m_messageSizer->Add(msgWidget, 0, wxEXPAND | wxALL, FromDIP(3));
    }

    m_messageContainer->Thaw();
}

void ChatPanel::AppendMessage(const wxString& timestamp, const wxString& user, const wxString& msg) {
    // Freeze the scrolled window to prevent immediate redraws and layout recalculations
    //m_messageContainer->Freeze();

    // Create a new MessageWidget instance within the scrolled window.
    auto* msgWidget = new MessageWidget(m_messageContainer, user, msg, timestamp, m_lastKnownWrapWidth);

    // Add the new message widget to the message sizer.
    // Use wxEXPAND to ensure it takes full horizontal width. Proportion 0 as it shouldn't grow vertically itself.
    m_messageSizer->Add(msgWidget, 0, wxEXPAND | wxALL, FromDIP(3));

    // --- Initial Wrapping Trigger (Important for first message) ---
    // If m_lastKnownWrapWidth is -1, it means this is likely the first message
    // or the window hasn't been properly sized yet.
    // Manually trigger the resize logic to ensure the first message gets wrapped correctly.
    // if (m_lastKnownWrapWidth == -1) {
    //     wxSizeEvent dummyEvent(m_messageContainer->GetSize(), wxEVT_SIZE); // Create a dummy size event
    //     OnChatPanelSize(dummyEvent); // Call the handler to initiate the debounced wrap
    // } else {
    //     // If we already have a known accurate wrap width, apply it immediately to the new message.
    //     // This ensures new messages are wrapped correctly without waiting for a resize event.
    //     msgWidget->SetWrappedMessage(m_lastKnownWrapWidth);
    // }

    // Force the message sizer to re-layout its children (including the new message).
    //m_messageSizer->Layout();
    // Update the wxScrolledWindow's virtual size based on the sizer's new minimum size.
    //m_messageContainer->SetVirtualSize(m_messageSizer->GetMinSize());

    // Thaw the scrolled window to allow it to redraw with the new content.
    //m_messageContainer->Thaw();

    // Scroll to the bottom to show the newest message.
    m_messageContainer->Scroll(0, m_messageContainer->GetVirtualSize().y);
}

// Event handler for when the message container (wxScrolledWindow) changes size.
void ChatPanel::OnChatPanelSize(wxSizeEvent& event) {
    // Get the current visible client width of the scrolled window.
    int newClientWidth = m_messageContainer->GetClientSize().x;
    
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
        ReWrapAllMessages(m_lastKnownWrapWidth);
    }
}

// Helper function to iterate through all messages and re-wrap their text.
void ChatPanel::ReWrapAllMessages(int wrapWidth) {
    if (wrapWidth <= 0) {
        return;
    }

    LOG_DEBUG << "Rewrapping text";

    // Iterate through all items (MessageWidgets) in the messageSizer.
    for (auto* item : m_messageSizer->GetChildren()) {
        if (item->IsWindow()) { // Ensure the sizer item represents a window (our MessageWidget)
            MessageWidget* msgWidget = static_cast<MessageWidget*>(item->GetWindow());
            if (msgWidget) {
                msgWidget->SetWrappedMessage(wrapWidth);
            }
        }
    }

    // Force the messageSizer to recalculate its total size after all children have potentially resized.
    //m_messageSizer->Layout();

    // Update the virtual size of the wxScrolledWindow based on the new total size of its content.
    m_messageContainer->SetVirtualSize(m_messageSizer->GetMinSize());

    // Scroll to the bottom again, as content height might have changed significantly.
    m_messageContainer->Scroll(0, m_messageContainer->GetVirtualSize().y);
}


void ChatPanel::OnSend(wxCommandEvent&) {
    if (!m_input_ctrl->IsEmpty()) {
        m_parent->wsClient->sendMessage(std::string(m_input_ctrl->GetValue().ToUTF8()));
        m_input_ctrl->Clear();
    }
}

void ChatPanel::OnLeave(wxCommandEvent&) {
    m_messageSizer->Clear(true);
    m_parent->wsClient->leaveRoom();
}
