#include <client/messageWidget.h>
#include <client/textUtil.h>
#include <client/message.h>
#include <client/wsClient.h>

enum {
    ID_COPY = wxID_HIGHEST + 40
};

MessageWidget::MessageWidget(wxWindow* parent,
                             const Message& msg,
                             int lastKnownWrapWidth)
    : wxPanel(parent, wxID_ANY), m_originalMessage{msg.msg}, m_timestamp_val{msg.timestamp} {
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
    SetDoubleBuffered(true);

    auto* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Header line (user + timestamp)
    auto* headerSizer = new wxBoxSizer(wxHORIZONTAL);


    // Username (bold)
    m_userText = new wxStaticText(this, wxID_ANY, msg.user);
    wxFont userFont = m_userText->GetFont();
    userFont.SetWeight(wxFONTWEIGHT_BOLD);
    m_userText->SetFont(userFont);
    m_userText->Wrap(-1);
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
    wxString wrapped = TextUtil::WrapText(this, m_originalMessage, lastKnownWrapWidth - FromDIP(10), wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));

    m_messageStaticText = new wxStaticText(this, wxID_ANY, wrapped,
                                          wxDefaultPosition, wxDefaultSize,
                                          wxALIGN_LEFT); // Ensure left alignment
    m_messageStaticText->Wrap(-1);
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

void MessageWidget::Update(wxWindow* parent, const Message& msg, int lastKnownWrapWidth) {
    m_originalMessage = msg.msg;
    m_timestamp_val = msg.timestamp;
    m_messageStaticText->SetLabelText(TextUtil::WrapText(this, m_originalMessage, lastKnownWrapWidth - FromDIP(10), m_messageStaticText->GetFont()));
    m_userText->SetLabelText(msg.user);
    m_timeText->SetLabelText(wxString::FromUTF8(WebSocketClient::formatMessageTimestamp(msg.timestamp)));
    InvalidateBestSize();
    Layout();
    Fit();
    Refresh();
}

// Method to set the wrapped message based on a given width
void MessageWidget::SetWrappedMessage(int wrapWidth) {
    if (!m_messageStaticText) return;

    // Get the font of the static text control for accurate measurement
    wxFont messageFont = m_messageStaticText->GetFont();

    // Wrap the original message using our utility function
    wxString wrapped = TextUtil::WrapText(this, m_originalMessage, wrapWidth - FromDIP(10), messageFont);

    // Only update the label and re-layout if the wrapped text has actually changed
    // This prevents unnecessary redraws and layout passes.
    if (m_messageStaticText->GetLabel() != wrapped) {
        m_messageStaticText->SetLabelText(wrapped);
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
    wxMenu menu;
    menu.Append(ID_COPY, "Copy Message");
    PopupMenu(&menu);
    event.Skip();
}

void MessageWidget::OnMouseEnter(wxMouseEvent& event) {
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
    Refresh();
    event.Skip();
}

void MessageWidget::OnMouseLeave(wxMouseEvent& event) {
    // Check if mouse left entire widget
    wxPoint screenPos = wxGetMousePosition();
    wxPoint clientPos = ScreenToClient(screenPos);
    if (!GetClientRect().Contains(clientPos)) {
        SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
        Refresh();
    }
    event.Skip();
}

