#include <client/messageView.h>
#include <client/chatPanel.h>
#include <client/mainWidget.h> // Include full definition for wsClient access
#include <client/messageWidget.h>
#include <client/wsClient.h>
#include <wx/dcbuffer.h> // Include for wxAutoBufferedPaintDC
#include <numeric>
#include <limits>

MessageView::MessageView(ChatPanel* parent)
    : wxVScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE),
      m_chatPanelParent(parent),
      m_loadingOlder(false),
      m_loadingNewer(false),
      m_lastKnownWrapWidth(-1),
      m_lastKnownSnapState(true)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetDoubleBuffered(true);
    //Bind(wxEVT_PAINT, &MessageView::OnPaint, this);
    Bind(wxEVT_SIZE, &MessageView::OnSize, this);
    Bind(wxEVT_MOUSEWHEEL, &MessageView::OnMouseWheel, this);
    //Bind(wxEVT_SCROLLWIN_LINEUP, &MessageView::OnScroll, this);
    //Bind(wxEVT_SCROLLWIN_LINEDOWN, &MessageView::OnScroll, this);
    //Bind(wxEVT_SCROLLWIN_PAGEUP, &MessageView::OnScroll, this);
    //Bind(wxEVT_SCROLLWIN_PAGEDOWN, &MessageView::OnScroll, this);
    //Bind(wxEVT_SCROLLWIN_THUMBTRACK, &MessageView::OnScroll, this);
    //Bind(wxEVT_SCROLLWIN_THUMBRELEASE, &MessageView::OnScroll, this);

    for (int i = 0; i < MAX_MESSAGES + CHUNK_SIZE; ++i) {
        m_widgetsPool.return_instance(new MessageWidget(this, Message{}, m_lastKnownWrapWidth));
        wxYieldIfNeeded();
    }
}

void MessageView::Start() {
    Clear();
    LoadOlderMessages();
}

void MessageView::OnSize(wxSizeEvent& event) {
    event.Skip();
    ReWrapAllMessages(GetClientSize().x);
}

void MessageView::OnScrolled() {
    UpdateWidgetPositions();
    if (m_loadingOlder || m_loadingNewer) return;
    wxCoord firstVisiblePixel = GetVisibleRowsBegin() * SCROLL_STEP;
    wxCoord totalHeight = GetUnitCount() * SCROLL_STEP;
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
    int lines = (rotation * linesPerWheel) / delta;
    if(lines == 0) {
        lines = (rotation > 0) ? -1 : 1;
    }

    // Perform scrolling
    //int oldPos = GetScrollPos(wxVERTICAL);
    ScrollRows(lines);


    //ScrollLines(lines);  // This updates the view
    //int newPos = GetScrollPos(wxVERTICAL);

    // Generate synthetic scroll event if position changed
    //if (oldPos != newPos) {
    //    wxScrollWinEvent evt(wxEVT_SCROLLWIN_THUMBTRACK, newPos, wxVERTICAL);
    //    evt.SetEventObject(this);
    //    OnScroll(evt);  // Process like other scroll events
    //}

    //event.Skip();
    OnScrolled();
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
    //wxAutoBufferedPaintDC dc(this);
    //UpdateWidgetPositions();
}

void MessageView::OnScroll(wxScrollWinEvent& event) {
    event.Skip();
    OnScrolled();
}

void MessageView::UpdateWidgetPositions() {
    wxCoord scrollY = GetVisibleRowsBegin() * SCROLL_STEP;
    wxCoord clientHeight = GetClientSize().y;
    wxCoord containerWidth = GetClientSize().x;
    const wxCoord padding = SCROLL_STEP * 10; 
    wxCoord currentY = 0;
    for (auto* widget : m_messageWidgets) {
        wxCoord h = widget->GetBestSize().y;
        if ((currentY + h) > (scrollY - padding) && (currentY < scrollY + clientHeight + padding)) {
            wxCoord physicalY = currentY - scrollY;
            widget->SetSize(0, physicalY, containerWidth, -1);
            widget->Show();
            //widget->Layout();
           // widget->Refresh();
        } else {
            widget->Hide();
        }
        currentY += h;
    }

    CheckAndUpdateSnapState();
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
    //msgWidget->Refresh();
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

    const wxCoord currentScrollY = GetVisibleRowsBegin() * SCROLL_STEP;
    const wxCoord totalHeight = GetUnitCount() * SCROLL_STEP;
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
