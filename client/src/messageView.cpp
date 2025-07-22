#include <client/messageView.h>
#include <client/chatPanel.h>
#include <client/mainWidget.h> // Include full definition for wsClient access
#include <client/messageWidget.h>
#include <client/wsClient.h>
#include <wx/dcbuffer.h> // Include for wxAutoBufferedPaintDC
#include <numeric>
#include <limits>

namespace client {

MessageView::MessageView(ChatPanel* parent)
    : wxVScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE),
      m_chatPanelParent(parent),
      m_loadingOlder(false),
      m_loadingNewer(false),
      m_lastKnownWrapWidth(-1),
      m_lastKnownSnapState(true)
{
    SetBackgroundStyle(wxBG_STYLE_ERASE);
    SetDoubleBuffered(true);
    Bind(wxEVT_SIZE, &MessageView::OnSize, this);
    Bind(wxEVT_MOUSEWHEEL, &MessageView::OnMouseWheel, this);

    for (int i = 0; i < MAX_MESSAGES + CHUNK_SIZE; ++i) {
        m_widgetsPool.return_instance(new MessageWidget(this, Message{}, m_lastKnownWrapWidth));
        wxYieldIfNeeded();
    }
}

void MessageView::Start() {
    Clear();
    LoadOlderMessages();
}

void MessageView::InvalidateCaches() {
    for(auto* widget : m_messageWidgets) {
        if(widget->IsShown()) {
            widget->InvalidateCaches();
        }
    }
}

void MessageView::DoSetSize(int x, int y, int width, int height, int sizeFlags) {
    // Capture state BEFORE resize.
    wxSize oldSize = GetClientSize();
    wxCoord oldScrollY = GetVisibleRowsBegin();

    // Let the base class perform the resize.
    wxVScrolledWindow::DoSetSize(x, y, width, height, sizeFlags);

    wxSize newSize = GetClientSize();

    // Guard against calls that result in no actual change.
    if (oldSize == newSize) {
        return;
    }

    // If width changed, your existing code must run.
    if (oldSize.x != newSize.x) {
        ReWrapAllMessages(newSize.x);
    }

    // ALWAYS apply the height adjustment. This is the final, authoritative scroll.
    int heightDifference = oldSize.y - newSize.y;
    ScrollToRow(oldScrollY + heightDifference);
    UpdateWidgetPositions();
}

void MessageView::OnSize(wxSizeEvent& event) {
    event.Skip();
}

void MessageView::OnScrolled() {
    UpdateWidgetPositions();
    if (m_loadingOlder || m_loadingNewer) return;
    wxCoord firstVisiblePixel = GetVisibleRowsBegin();
    wxCoord totalHeight = GetUnitCount();
    wxCoord visibleHeight = GetClientSize().y;
    const int pixelThreshold = FromDIP(500);
    if (firstVisiblePixel <= pixelThreshold) LoadOlderMessages();
    else if (firstVisiblePixel + visibleHeight >= totalHeight - pixelThreshold) LoadNewerMessages();
}

void MessageView::OnMouseWheel(wxMouseEvent& event) {
    // Handle vertical wheel scrolling only
    if(event.GetWheelAxis() != wxMOUSE_WHEEL_VERTICAL) {
        event.Skip();
        return;
    }
    // Calculate scroll lines
    int rotation = event.GetWheelRotation();
    int delta = event.GetWheelDelta();
    int linesPerWheel = FromDIP(15);//event.GetLinesPerAction();

    // Calculate lines to scroll
    int lines = -(rotation * linesPerWheel) / delta;
    if(lines == 0) {
        lines = (rotation > 0) ? -1 : 1;
    }

    if(ScrollRows(lines)) {
        OnScrolled();
    }
}

void MessageView::OnMessagesReceived(const std::vector<Message>& messages, bool isHistoryResponse) {
    if (!messages.empty()) {
        UpdateLayoutAndScroll(messages, isHistoryResponse);
    }
    m_loadingOlder = false;
    m_loadingNewer = false;
}

void MessageView::UpdateLayoutAndScroll(const std::vector<Message>& messages, bool isHistoryResponse) {
    int oldScrollY = GetVisibleRowsBegin();
    bool wasEmpty = m_messageWidgets.empty();

    if (m_loadingOlder) {
        int addedHeight = 0;
        for (const auto& msg : messages) {
            AddMessageWidget(msg, true);
            addedHeight += m_messageWidgets.front()->GetBestSize().y;
        }
        TrimWidgets(false);
        SetUnitCount(CalculateTotalHeight());
        if(wasEmpty) {
            ScrollToRow(GetUnitCount() - GetClientSize().y);
        } else {
            ScrollToRow(oldScrollY + addedHeight);
        }
    } else {
        bool wasAtBottom = GetVisibleRowsBegin() + GetClientSize().y >= GetUnitCount() - 5;
        int removedHeight = 0;
        
        if(m_messageWidgets.size() + messages.size() > MAX_MESSAGES) {
            auto numToRemove = m_messageWidgets.size() + messages.size() - MAX_MESSAGES;

            for(std::size_t i = 0; i < numToRemove; ++i) {
                if(i == m_messageWidgets.size()) {
                    break;
                }
                removedHeight += m_messageWidgets[i]->GetBestSize().y;
            }
        }
        for(const auto& msg : messages) {
            AddMessageWidget(msg, false);
        }
        TrimWidgets(true);
        SetUnitCount(CalculateTotalHeight());
        if(!isHistoryResponse && wasAtBottom) {
            ScrollToRow(GetUnitCount());
        } else {
            int newScrollUnits = (oldScrollY - removedHeight);
            ScrollToRow((std::max)(0, newScrollUnits));
        }
    }

    UpdateWidgetPositions();
}

void MessageView::UpdateWidgetPositions() {
    wxCoord scrollY = GetVisibleRowsBegin();
    wxCoord clientHeight = GetClientSize().y;
    wxCoord containerWidth = GetClientSize().x;
    const wxCoord padding = FromDIP(100); 
    wxCoord currentY = 0;
    for (auto* widget : m_messageWidgets) {
        wxCoord h = widget->GetBestSize().y;
        auto wasHidden = !widget->IsShown();
        if ((currentY + h) > (scrollY - padding) && (currentY < scrollY + clientHeight + padding)) {
            wxCoord physicalY = currentY - scrollY;
            widget->SetSize(0, physicalY, containerWidth, h);
            if(wasHidden) {
                widget->InvalidateCaches();
                widget->Show();
            }
        } else {
            if(!wasHidden) {
                widget->Hide();
            }
        }
        currentY += h;
    }

    CheckAndUpdateSnapState();
}

void MessageView::ReWrapAllMessages(int wrapWidth) {
    if (wrapWidth <= 0 || m_lastKnownWrapWidth == wrapWidth) return;
    m_lastKnownWrapWidth = wrapWidth;
    wxCoord oldScrollY = GetVisibleRowsBegin();
    for(auto* widget : m_messageWidgets) {
        //if(widget->IsShown()) {
            widget->SetWrappedMessage(m_lastKnownWrapWidth);
        //}
    }
    SetUnitCount(CalculateTotalHeight());
    ScrollToRow(oldScrollY);
    UpdateWidgetPositions();
}

void MessageView::Clear() {
    for(auto& m : m_messageWidgets) {
        m_widgetsPool.return_instance(std::move(m));
    }
    m_messageWidgets.clear();
    SetUnitCount(0);
    m_loadingOlder = false;
    m_loadingNewer = false;
}

wxCoord MessageView::OnGetRowHeight([[maybe_unused]] size_t row) const {
    return 1;
}

void MessageView::AddMessageWidget(const Message& msg, bool prepend) {
    
    auto* msgWidget = m_widgetsPool.get_instance(this, msg, m_lastKnownWrapWidth);
    if (prepend) {
        m_messageWidgets.push_front(msgWidget);
    } else {
        m_messageWidgets.push_back(msgWidget);
    }
}

int MessageView::TrimWidgets(bool fromTop) {
    int numRemoved = 0;
    while (m_messageWidgets.size() > MAX_MESSAGES) {
        MessageWidget* toDelete = nullptr;
        if (fromTop) {
            toDelete = m_messageWidgets.front();
            m_messageWidgets.pop_front();
        } else {
            toDelete = m_messageWidgets.back();
            m_messageWidgets.pop_back();
        }
        if (toDelete) {
            toDelete->Hide();
            m_widgetsPool.return_instance(std::move(toDelete));
        }
        numRemoved++;
    }
    return numRemoved;
}

wxCoord MessageView::CalculateTotalHeight() const {
    wxCoord totalHeight = 0;
    for(const auto* widget : m_messageWidgets) {
        totalHeight += widget->GetBestSize().y;
    }
    return totalHeight;
}

void MessageView::LoadOlderMessages() {
    if(m_loadingOlder) {
        return;
    }
    m_loadingOlder = true;
    const auto topTimestamp = m_messageWidgets.empty() ? 32517734834000000 : m_messageWidgets.front()->GetTimestampValue();
    m_chatPanelParent->GetMainWidget()->wsClient->getMessages(CHUNK_SIZE, topTimestamp);
}

void MessageView::LoadNewerMessages() {
    if(m_loadingNewer || m_messageWidgets.empty()) {
        return;
    }
    m_loadingNewer = true;
    const auto bottomTimestamp = m_messageWidgets.back()->GetTimestampValue();
    m_chatPanelParent->GetMainWidget()->wsClient->getMessages(-CHUNK_SIZE, bottomTimestamp);
}

bool MessageView::IsSnappedToBottom() const {
    if(m_messageWidgets.empty()) {
        return true;
    }

    const wxCoord currentScrollY = GetVisibleRowsBegin();
    const wxCoord totalHeight = GetUnitCount();
    const wxCoord visibleHeight = GetClientSize().y;

    return (currentScrollY + visibleHeight) >= (totalHeight - 100);
}

void MessageView::CheckAndUpdateSnapState() {
    bool currentlySnapped = IsSnappedToBottom();
    if (currentlySnapped != m_lastKnownSnapState) {
        m_lastKnownSnapState = currentlySnapped;
        UpdateSnapState(currentlySnapped);
    }
}

void MessageView::UpdateSnapState(bool isSnapped) {
    wxCommandEvent evt(wxEVT_SNAP_STATE_CHANGED, GetId());
    evt.SetInt(isSnapped ? 1 : 0);
    ProcessEvent(evt);
}

void MessageView::JumpToPresent() {
    Start();
}

void MessageView::DeleteMessageById(int32_t messageId) {
    auto it = std::find_if(m_messageWidgets.begin(), m_messageWidgets.end(),
        [messageId](const MessageWidget* widget) {
            return widget->GetMessageId() == messageId;
        });

    if (it != m_messageWidgets.end()) {
        MessageWidget* widgetToRemove = *it;
        m_widgetsPool.return_instance(std::move(widgetToRemove));
        m_messageWidgets.erase(it);
        SetUnitCount(CalculateTotalHeight());
        UpdateWidgetPositions();
    }
}

void MessageView::UpdateUsername(int32_t userId, const wxString& newUsername) {
    for (auto message : m_messageWidgets) {
        if (message->GetUserId() == userId) {
            Message msg{
                .user = newUsername,
                .userId = userId,
                .msg = message->GetMessageValue(),
                .timestamp = message->GetTimestampValue(),
                .messageId = message->GetMessageId(),
            };
            message->Update(this, msg, m_lastKnownWrapWidth);
        }
    }
    UpdateWidgetPositions();
    Refresh();
}

} // namespace client
