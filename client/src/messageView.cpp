#include <client/messageView.h>
#include <client/chatPanel.h>
#include <client/mainWidget.h> // Include full definition for wsClient access
#include <client/messageWidget.h>
#include <client/wsClient.h>
#include <wx/dcbuffer.h> // Include for wxAutoBufferedPaintDC
#include <numeric>

MessageView::MessageView(ChatPanel* parent)
    : wxVScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER),
      m_chatPanelParent(parent),
      m_loadingOlder(false),
      m_loadingNewer(false),
      m_lastKnownWrapWidth(-1)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    Bind(wxEVT_PAINT, &MessageView::OnPaint, this);
    Bind(wxEVT_SIZE, &MessageView::OnSize, this);
    Bind(wxEVT_SCROLLWIN_LINEUP, &MessageView::OnScroll, this);
    Bind(wxEVT_SCROLLWIN_LINEDOWN, &MessageView::OnScroll, this);
    Bind(wxEVT_SCROLLWIN_PAGEUP, &MessageView::OnScroll, this);
    Bind(wxEVT_SCROLLWIN_PAGEDOWN, &MessageView::OnScroll, this);
    Bind(wxEVT_SCROLLWIN_THUMBTRACK, &MessageView::OnScroll, this);
    Bind(wxEVT_SCROLLWIN_THUMBRELEASE, &MessageView::OnScroll, this);
}

void MessageView::Start() {
    Clear();
    LoadOlderMessages();
}

void MessageView::OnSize(wxSizeEvent& event) {
    event.Skip();
    ReWrapAllMessages(GetClientSize().x);
}

void MessageView::OnMessagesReceived(const std::vector<Message>& messages, bool isHistoryResponse) {
    if (messages.empty()) {
        m_loadingOlder = false;
        m_loadingNewer = false;
        return;
    }
    UpdateLayoutAndScroll(messages, isHistoryResponse);
    m_loadingOlder = false;
    m_loadingNewer = false;
}

void MessageView::UpdateLayoutAndScroll(const std::vector<Message>& messages, bool isHistoryResponse) {
    int oldScrollY = GetVisibleRowsBegin() * SCROLL_STEP;
    bool wasEmpty = m_messageWidgets.empty();

    if (m_loadingOlder) {
        int addedHeight = 0;
        for (const auto& msg : messages) {
            AddMessageWidget(msg, true);
            addedHeight += m_messageWidgets.front()->GetBestSize().y;
        }
        TrimWidgets(false);
        SetUnitCount(CalculateTotalHeight() / SCROLL_STEP);
        if (wasEmpty) ScrollToRow(GetUnitCount());
        else ScrollToRow((oldScrollY + addedHeight) / SCROLL_STEP);
    } else {
        bool wasAtBottom = (GetVisibleRowsBegin() * SCROLL_STEP) + GetClientSize().y >= (GetUnitCount() * SCROLL_STEP) - 5;
        int removedHeight = 0;
        int numToRemove = m_messageWidgets.size() + messages.size() - MAX_MESSAGES;
        if (numToRemove > 0) {
            for (int i = 0; i < numToRemove; ++i)
                if (i < m_messageWidgets.size()) removedHeight += m_messageWidgets[i]->GetBestSize().y;
        }
        for (const auto& msg : messages) {
            AddMessageWidget(msg, false);
        }
        TrimWidgets(true);
        SetUnitCount(CalculateTotalHeight() / SCROLL_STEP);
        if (!isHistoryResponse && wasAtBottom) ScrollToRow(GetUnitCount());
        else {
            int newScrollUnits = (oldScrollY - removedHeight) / SCROLL_STEP;
            if (newScrollUnits < 0) newScrollUnits = 0;
            ScrollToRow(newScrollUnits);
        }
    }
    
    UpdateWidgetPositions();
}

void MessageView::OnPaint(wxPaintEvent& event) {
    wxAutoBufferedPaintDC dc(this);
    //UpdateWidgetPositions();
}

void MessageView::OnScroll(wxScrollWinEvent& event) {
    event.Skip();
    if (m_loadingOlder || m_loadingNewer) return;
    UpdateWidgetPositions();
    wxCoord firstVisiblePixel = GetVisibleRowsBegin() * SCROLL_STEP;
    wxCoord totalHeight = GetUnitCount() * SCROLL_STEP;
    wxCoord visibleHeight = GetClientSize().y;
    const int pixelThreshold = FromDIP(500); 
    if (firstVisiblePixel <= pixelThreshold) LoadOlderMessages();
    else if (firstVisiblePixel + visibleHeight >= totalHeight - pixelThreshold) LoadNewerMessages();
}

void MessageView::UpdateWidgetPositions() {
    wxCoord scrollY = GetVisibleRowsBegin() * SCROLL_STEP;
    wxCoord clientHeight = GetClientSize().y;
    wxCoord containerWidth = GetClientSize().x;
    const wxCoord padding = SCROLL_STEP * 5; 
    wxCoord currentY = 0;
    for (auto* widget : m_messageWidgets) {
        wxCoord h = widget->GetBestSize().y;
        if ((currentY + h) > (scrollY - padding) && (currentY < scrollY + clientHeight + padding)) {
            wxCoord physicalY = currentY - scrollY;
            widget->SetSize(0, physicalY, containerWidth, -1);
            //widget->Layout();
            widget->Show();
        } else {
            widget->Hide();
        }
        currentY += h;
    }
}

void MessageView::ReWrapAllMessages(int wrapWidth) {
    if (wrapWidth <= 0 || m_lastKnownWrapWidth == wrapWidth) return;
    m_lastKnownWrapWidth = wrapWidth;
    wxCoord oldScrollY = GetVisibleRowsBegin() * SCROLL_STEP;
    for (auto* widget : m_messageWidgets) {
        widget->SetWrappedMessage(m_lastKnownWrapWidth);
    }
    SetUnitCount(CalculateTotalHeight() / SCROLL_STEP);
    ScrollToRow(oldScrollY / SCROLL_STEP);
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

// This now correctly overrides the pure virtual function.
wxCoord MessageView::OnGetRowHeight(size_t row) const {
    return SCROLL_STEP;
}

void MessageView::AddMessageWidget(const Message& msg, bool prepend) {
    
    auto* msgWidget = m_widgetsPool.get_instance(this, msg, m_lastKnownWrapWidth);
    //msgWidget->SetClientSize(GetClientSize().x, -1);
    //msgWidget->SetSize(0, 0, GetClientSize().x, -1);
    //msgWidget->Layout();
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
    for (const auto* widget : m_messageWidgets) {
        totalHeight += widget->GetBestSize().y;
    }
    return totalHeight;
}

void MessageView::LoadOlderMessages() {
    if (m_loadingOlder) return;
    m_loadingOlder = true;
    int64_t timestamp = m_messageWidgets.empty() ? std::numeric_limits<int64_t>::max() : m_messageWidgets.front()->GetTimestampValue();
    m_chatPanelParent->GetMainWidget()->wsClient->getMessages(CHUNK_SIZE, timestamp);
}

void MessageView::LoadNewerMessages() {
    if (m_loadingNewer || m_messageWidgets.empty()) return;
    if ((GetVisibleRowsBegin() * SCROLL_STEP) + GetClientSize().y >= (GetUnitCount() * SCROLL_STEP)) return;
    m_loadingNewer = true;
    int64_t bottomTimestamp = m_messageWidgets.back()->GetTimestampValue();
    m_chatPanelParent->GetMainWidget()->wsClient->getMessages(-CHUNK_SIZE, bottomTimestamp);
}
