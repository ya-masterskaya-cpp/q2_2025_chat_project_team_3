#include <wx/graphics.h>
#include <wx/clipbrd.h>
#include <client/messageWidget.h>
#include <client/textUtil.h>
#include <client/message.h>
#include <client/wsClient.h>
#include <client/cachedColorText.h>
#include <client/chatPanel.h>
#include <client/mainWidget.h>
#include <client/user.h>

namespace client {

enum {
    ID_COPY = wxID_HIGHEST + 40,
    ID_DELETE_MESSAGE
};

MessageWidget::MessageWidget(wxWindow* parent,
                             const Message& msg,
                             int lastKnownWrapWidth)
    : wxPanel(parent, wxID_ANY), m_originalMessage{ msg.msg }, m_timestamp_val{ msg.timestamp }, m_messageId{ msg.messageId }, m_userId{ msg.userId } {
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
    SetDoubleBuffered(true);

    auto* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Header line (user + timestamp)
    auto* headerSizer = new wxBoxSizer(wxHORIZONTAL);

    // Username (bold)
    m_userText = new CachedColorText(this, wxID_ANY, msg.user, wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT, wxStaticTextNameStr, false);
    wxFont userFont = m_userText->GetFont();
    userFont.SetWeight(wxFONTWEIGHT_BOLD);
    m_userText->SetFont(userFont);
    //m_userText->Wrap(-1);
    m_userText->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
    headerSizer->Add(m_userText, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(5));

    headerSizer->AddStretchSpacer();

    wxString timestamp_str = wxString::FromUTF8(WebSocketClient::formatMessageTimestamp(msg.timestamp));

    // Timestamp (smaller, gray)
    m_timeText = new wxStaticText(this, wxID_ANY, timestamp_str);
    m_timeText->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
    wxFont timeFont = m_timeText->GetFont();
    timeFont.SetPointSize(timeFont.GetPointSize() - 1); // Make it slightly smaller
    m_timeText->SetFont(timeFont);
    m_timeText->Wrap(-1);
    headerSizer->Add(m_timeText, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(5));

    mainSizer->Add(headerSizer, 0, wxEXPAND | wxBOTTOM, FromDIP(2));

    // Wrap the original message using our utility function
    wxString wrapped = TextUtil::WrapText(this, m_originalMessage, lastKnownWrapWidth - FromDIP(9), this->GetFont());

    m_messageStaticText = new CachedColorText(this, wxID_ANY, wrapped,
                                          wxDefaultPosition, wxDefaultSize,
                                          wxALIGN_LEFT, wxStaticTextNameStr, true); // Ensure left alignment
    //m_messageStaticText->Wrap(-1);
    //m_messageStaticText->SetBackgroundColour(*wxYELLOW); // For debugging the text control width

    // Add wxStaticText to the sizer. Proportion 1 allows it to expand vertically
    // within the MessageWidget if its content's height changes.
    mainSizer->Add(m_messageStaticText, 1, wxALL | wxEXPAND, FromDIP(5));

    SetSizer(mainSizer); // Set the sizer for the panel

    Bind(wxEVT_RIGHT_DOWN, &MessageWidget::OnRightClick, this);
    m_userText->Bind(wxEVT_RIGHT_DOWN, &MessageWidget::OnRightClick, this);
    m_timeText->Bind(wxEVT_RIGHT_DOWN, &MessageWidget::OnRightClick, this);
    m_messageStaticText->Bind(wxEVT_RIGHT_DOWN, &MessageWidget::OnRightClick, this);

    Bind(wxEVT_ENTER_WINDOW, &MessageWidget::OnMouseEnter, this);

    Bind(wxEVT_LEAVE_WINDOW, &MessageWidget::OnMouseLeave, this);
    m_userText->Bind(wxEVT_LEAVE_WINDOW, &MessageWidget::OnMouseLeave, this);
    m_timeText->Bind(wxEVT_LEAVE_WINDOW, &MessageWidget::OnMouseLeave, this);
    m_messageStaticText->Bind(wxEVT_LEAVE_WINDOW, &MessageWidget::OnMouseLeave, this);
}

void MessageWidget::InvalidateCaches() {
    m_messageStaticText->InvalidateCache();
    m_userText->InvalidateCache();
}

void MessageWidget::Update([[maybe_unused]] wxWindow* parent, const Message& msg, int lastKnownWrapWidth) {
    m_originalMessage = msg.msg;
    m_timestamp_val = msg.timestamp;
    m_messageId = msg.messageId;
    m_messageStaticText->SetLabelText(TextUtil::WrapText(this, m_originalMessage, lastKnownWrapWidth - FromDIP(9), this->GetFont()));
    m_userText->SetLabelText(msg.user);
    m_userId = msg.userId;
    m_timeText->SetLabelText(wxString::FromUTF8(WebSocketClient::formatMessageTimestamp(msg.timestamp)));
    //InvalidateBestSize();
    Layout();
    //Fit();
    //Refresh();
}

// Method to set the wrapped message based on a given width
void MessageWidget::SetWrappedMessage(int wrapWidth) {
    if (!m_messageStaticText) return;

    // Wrap the original message using our utility function
    wxString wrapped = TextUtil::WrapText(this, m_originalMessage, wrapWidth - FromDIP(9), this->GetFont());

    // Only update the label and re-layout if the wrapped text has actually changed
    // This prevents unnecessary redraws and layout passes.
    if (m_messageStaticText->GetLabel() != wrapped) {
        m_messageStaticText->SetLabel(wrapped);
    }
}

// Method to retrieve the font of the message's wxStaticText.
wxFont MessageWidget::GetMessageTextFont() const {
    if (m_messageStaticText) {
        return m_messageStaticText->GetFont();
    }
    return wxFont(); // Return a default (invalid) font if the control isn't ready
}

void MessageWidget::OnRightClick(wxMouseEvent& event) {
    auto* chatPanel = dynamic_cast<ChatPanel*>(GetParent()->GetParent());
    const User& currentUser = chatPanel->GetCurrentUser();
    bool canDelete = (currentUser.role > chat::UserRights::REGULAR);

    wxMenu menu;
    menu.Append(ID_COPY, "Copy Message");

    if (canDelete) {
        menu.AppendSeparator();
        menu.Append(ID_DELETE_MESSAGE, "Delete Message");
    }

    menu.Bind(wxEVT_MENU, [this](wxCommandEvent&) {
        wxTheApp->CallAfter([msg = m_originalMessage]() mutable {
            wxClipboardLocker locker;
            if (!locker) {
                return;
            }
            wxTheClipboard->SetData(new wxTextDataObject(msg));
        });
    }, ID_COPY);

    if (canDelete) {
        menu.Bind(wxEVT_MENU, [this](wxCommandEvent&) {
            int response = wxMessageBox(
                "Are you sure you want to permanently delete this message?",
                "Confirm Deletion",
                wxYES_NO | wxNO_DEFAULT | wxICON_WARNING,
                this
            );
            if (response == wxYES) {
                wxCommandEvent deleteEvent(wxEVT_DELETE_MESSAGE, GetId());
                deleteEvent.SetEventObject(this);
                deleteEvent.SetInt(m_messageId);
                wxPostEvent(GetParent()->GetParent(), deleteEvent);
            }
        }, ID_DELETE_MESSAGE);
    }

    PopupMenu(&menu);
    event.Skip();
}

void MessageWidget::OnMouseEnter(wxMouseEvent& event) {
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
    InvalidateCaches();
    Refresh();
    event.Skip();
}

void MessageWidget::OnMouseLeave(wxMouseEvent& event) {
    // Check if mouse left entire widget
    wxPoint screenPos = wxGetMousePosition();
    wxPoint clientPos = ScreenToClient(screenPos);
    if (!GetClientRect().Contains(clientPos)) {
        SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
        InvalidateCaches();
        Refresh();
    }
    event.Skip();
}

} // namespace client
