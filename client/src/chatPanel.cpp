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

    m_messageContainer->Bind(wxEVT_SCROLLWIN_LINEUP,      &ChatPanel::OnScroll, this);
    m_messageContainer->Bind(wxEVT_SCROLLWIN_LINEDOWN,    &ChatPanel::OnScroll, this);
    m_messageContainer->Bind(wxEVT_SCROLLWIN_PAGEUP,      &ChatPanel::OnScroll, this);
    m_messageContainer->Bind(wxEVT_SCROLLWIN_PAGEDOWN,    &ChatPanel::OnScroll, this);
    m_messageContainer->Bind(wxEVT_SCROLLWIN_THUMBTRACK,  &ChatPanel::OnScroll, this);
    m_messageContainer->Bind(wxEVT_SCROLLWIN_THUMBRELEASE,&ChatPanel::OnScroll, this);
    //m_messageContainer->Bind(wxEVT_MOUSEWHEEL,            &ChatPanel::OnScroll, this);
}

void ChatPanel::AppendMessages(const std::vector<Message>& messages, bool isHistoryResponse) {
    if(messages.empty()) {
        m_loadingOlder = false;
        m_loadingNewer = false;
        return;
    }

    m_messageContainer->Freeze();

    if(!isHistoryResponse) {
        int ppuX, ppuY;
        m_messageContainer->GetScrollPixelsPerUnit(&ppuX, &ppuY);
        
        bool wasAtBottom = (m_messageContainer->GetViewStart().y * ppuY + m_messageContainer->GetClientSize().y) >= m_messageContainer->GetVirtualSize().y - 5;
        
        for(const auto& m : messages) {
            AddMessageInternal(m, false);
        }

        int overflow = m_messageSizer->GetItemCount() - MAX_MESSAGES;

        if(overflow > 0) {
            RemoveMessages(overflow, true);
        }

        if(wasAtBottom) {
            ScrollToBottom();
        }

    } else {
        int currentScrollPosInUnits = m_messageContainer->GetScrollPos(wxVERTICAL);
        int ppuX, pixelsPerUnit;
        m_messageContainer->GetScrollPixelsPerUnit(&ppuX, &pixelsPerUnit);
        if(pixelsPerUnit == 0) {
            pixelsPerUnit = 1;
        }

        bool was_empty = m_messageSizer->GetChildren().empty();

        if(m_loadingOlder) {
            // --- Process as OLDER messages (scrolling UP) ---
            int addedHeight = 0;
            for(auto it = messages.begin(); it != messages.end(); ++it) {
                AddMessageInternal(*it, true);
            }
            
            m_messageSizer->Layout();
            for(size_t i = 0; i < messages.size(); ++i) {
                addedHeight += m_messageSizer->GetItem((size_t)i)->GetSize().y;
            }
            
            int overflow = m_messageSizer->GetItemCount() - MAX_MESSAGES;
            if(overflow > 0) {
                RemoveMessages(overflow, false);
            }
            
            m_messageSizer->Layout();
            m_messageContainer->FitInside();
            
            if(!was_empty) {
                int scrollAdjustmentInUnits = addedHeight / pixelsPerUnit;
                m_messageContainer->Scroll(-1, currentScrollPosInUnits + scrollAdjustmentInUnits);
            } else {
                ScrollToBottom();
            }
            m_loadingOlder = false;

        } else if(m_loadingNewer) {
            // --- LOGIC for NEWER messages (scrolling DOWN) ---
            
            // First, append the new messages.
            for(const auto& m : messages) {
                AddMessageInternal(m, false);
            }
            
            // Now, determine if we are over the limit.
            int overflow = m_messageSizer->GetItemCount() - MAX_MESSAGES;
            int removedHeight = 0;

            if(overflow > 0) {
                // If we are over, measure the height of the items we are about to remove from the top.
                for (size_t i = 0; i < (size_t)overflow; ++i) {
                    removedHeight += m_messageSizer->GetItem(i)->GetSize().y;
                }
                // Now, remove that exact number of overflowing items from the top.
                RemoveMessages(overflow, true);
            }
            
            m_messageSizer->Layout();
            m_messageContainer->FitInside();
            
            // Only adjust the scroll position if we actually removed items.
            if(removedHeight > 0) {
                int scrollAdjustmentInUnits = removedHeight / pixelsPerUnit;
                m_messageContainer->Scroll(-1, currentScrollPosInUnits - scrollAdjustmentInUnits);
            }
            
            m_loadingNewer = false;
        }
    }

    m_messageContainer->Thaw();
}

void ChatPanel::AppendMessage(const wxString& timestamp, const wxString& user, const wxString& msg, int64_t timestamp_val) {
    Message live_msg = { user, msg, timestamp_val };
    AppendMessages({live_msg}, false);
}

void ChatPanel::LoadOlderMessages() {
    LOG_DEBUG << "LoadOlderMessages()";
    if (m_loadingOlder || m_loadingNewer || m_messageSizer->GetChildren().empty()) {
        return;
    }
    m_loadingOlder = true;
    auto* firstWidget = static_cast<MessageWidget*>(m_messageSizer->GetItem((size_t)0)->GetWindow());
    m_parent->wsClient->getMessages(CHUNK_SIZE, firstWidget->GetTimestampValue());
}

void ChatPanel::LoadNewerMessages() {
    LOG_DEBUG << "LoadNewerMessages()";

    if(m_loadingOlder || m_loadingNewer || m_messageSizer->GetChildren().empty()) {
        return;
    }
    
    int ppuX, ppuY;
    m_messageContainer->GetScrollPixelsPerUnit(&ppuX, &ppuY);
    if((m_messageContainer->GetViewStart().y * ppuY + m_messageContainer->GetClientSize().y) >= m_messageContainer->GetVirtualSize().y - 5) {
        return;
    } 
    
    m_loadingNewer = true;
    auto* lastWidget = static_cast<MessageWidget*>(m_messageSizer->GetItem(m_messageSizer->GetItemCount() - 1)->GetWindow());
    m_parent->wsClient->getMessages(-CHUNK_SIZE, lastWidget->GetTimestampValue());
}

void ChatPanel::ScrollToBottom() {
    LOG_DEBUG << "ScrollToBottom()";
    wxTheApp->CallAfter([this]() {
        if (this && m_messageContainer) {
            m_messageSizer->Layout();
            m_messageContainer->FitInside();
            int range = m_messageContainer->GetScrollRange(wxVERTICAL);
            m_messageContainer->Scroll(-1, range + 1);
        }
    });
}

void ChatPanel::AddMessageInternal(const Message& msg, bool prepend) {
    wxString timestamp_str = wxString::FromUTF8(WebSocketClient::formatMessageTimestamp(msg.timestamp));
    auto* msgWidget = new MessageWidget(m_messageContainer, msg.user, msg.msg,
                                        timestamp_str, msg.timestamp, m_lastKnownWrapWidth);
    m_messageSizer->Insert(prepend ? 0 : m_messageSizer->GetItemCount(),
                           msgWidget, 0, wxEXPAND | wxALL, FromDIP(3));
}

void ChatPanel::RemoveMessages(int count, bool fromTop) {
    for (int i = 0; i < count && !m_messageSizer->GetChildren().empty(); ++i) {
        size_t index = fromTop ? 0 : m_messageSizer->GetItemCount() - 1;
        wxSizerItem* item = m_messageSizer->GetItem(index);
        wxWindow* window = item->GetWindow();
        m_messageSizer->Detach(index);
        window->Destroy();
    }
}

void ChatPanel::LoadInitialMessages() {
    LOG_DEBUG << "LoadInitialMessages()";
    m_loadingOlder = true;

    m_parent->wsClient->getMessages(CHUNK_SIZE * 2, std::numeric_limits<int64_t>::max());
}

void ChatPanel::OnScroll(wxScrollWinEvent& event) {
    event.Skip();

    int ppuX, pixelsPerUnit;
    m_messageContainer->GetScrollPixelsPerUnit(&ppuX, &pixelsPerUnit);
    
    int viewStartPixels = m_messageContainer->GetViewStart().y * pixelsPerUnit;
    int clientHeightPixels = m_messageContainer->GetClientSize().y;
    int virtualHeightPixels = m_messageContainer->GetVirtualSize().y;
    
    if (viewStartPixels < LOAD_THRESHOLD) LoadOlderMessages();
    else if (viewStartPixels + clientHeightPixels > virtualHeightPixels - LOAD_THRESHOLD) LoadNewerMessages();
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
