
#include <client/messageWidget.h>
#include <client/textUtil.h>

enum {
    ID_COPY = wxID_HIGHEST + 40
};

MessageWidget::MessageWidget(wxWindow* parent,
                             const wxString& sender,
                             const wxString& message, // Original message passed here initially
                             const wxString& timestamp)
    : wxPanel(parent, wxID_ANY),
      m_originalMessage(message), // Store the original message directly in constructor
      m_messageStaticText(nullptr) // Initialize pointer to null
{
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));

    auto* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Header line (user + timestamp)
    auto* headerSizer = new wxBoxSizer(wxHORIZONTAL);

    // Username (bold)
    auto* userText = new wxStaticText(this, wxID_ANY, sender);
    wxFont userFont = userText->GetFont();
    userFont.SetWeight(wxFONTWEIGHT_BOLD);
    userText->SetFont(userFont);
    userText->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
    headerSizer->Add(userText, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(5));

    headerSizer->AddStretchSpacer();

    // Timestamp (smaller, gray)
    auto* timeText = new wxStaticText(this, wxID_ANY, timestamp);
    timeText->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
    wxFont timeFont = timeText->GetFont();
    timeFont.SetPointSize(timeFont.GetPointSize() - 1); // Make it slightly smaller
    timeText->SetFont(timeFont);
    headerSizer->Add(timeText, 0, wxALIGN_CENTER_VERTICAL);

    mainSizer->Add(headerSizer, 0, wxEXPAND | wxBOTTOM, FromDIP(2));

    // Message text (initially set with the original, unwrapped message)
    m_messageStaticText = new wxStaticText(this, wxID_ANY, message,
                                          wxDefaultPosition, wxDefaultSize,
                                          wxALIGN_LEFT); // Ensure left alignment
    //m_messageStaticText->SetBackgroundColour(*wxYELLOW); // For debugging the text control width

    // Add wxStaticText to the sizer. Proportion 1 allows it to expand vertically
    // within the MessageWidget if its content's height changes.
    mainSizer->Add(m_messageStaticText, 1, wxALL | wxEXPAND, FromDIP(5));

    SetSizer(mainSizer); // Set the sizer for the panel
    //mainSizer->Layout(); // Ensure it calculates its initial size
    // Do NOT call SetSizerAndFit here, as that might cause issues with initial sizing
    // in the parent sizer. Let the parent's sizer manage fitting.

    // Bind event handlers (keep your existing implementations)
    Bind(wxEVT_RIGHT_DOWN, &MessageWidget::OnRightClick, this);
    userText->Bind(wxEVT_RIGHT_DOWN, &MessageWidget::OnRightClick, this);
    timeText->Bind(wxEVT_RIGHT_DOWN, &MessageWidget::OnRightClick, this);
    m_messageStaticText->Bind(wxEVT_RIGHT_DOWN, &MessageWidget::OnRightClick, this);

    Bind(wxEVT_ENTER_WINDOW, &MessageWidget::OnMouseEnter, this);
    Bind(wxEVT_LEAVE_WINDOW, &MessageWidget::OnMouseLeave, this);
}

// Method to store the original, unwrapped message string.
// This is useful if the MessageWidget is reused or if you pass the message later.
void MessageWidget::SetOriginalMessage(const wxString& message) {
    m_originalMessage = message;
    // Set the label initially with the original message (it will be wrapped later by SetWrappedMessage)
    if (m_messageStaticText) {
        m_messageStaticText->SetLabel(m_originalMessage);
    }
}

// Method to set the wrapped message based on a given width
void MessageWidget::SetWrappedMessage(int wrapWidth) {
    if (!m_messageStaticText) return;

    // Get the font of the static text control for accurate measurement
    wxFont messageFont = m_messageStaticText->GetFont();

    // Wrap the original message using our utility function
    wxString wrapped = TextUtil::WrapText(this, m_originalMessage, wrapWidth, messageFont);

    // Only update the label and re-layout if the wrapped text has actually changed
    // This prevents unnecessary redraws and layout passes.
    if (m_messageStaticText->GetLabel() != wrapped) {
        m_messageStaticText->SetLabel(wrapped);
        // After changing the label, force the MessageWidget's internal sizer to re-layout.
        // This recalculates the MessageWidget's height based on the new wrapped text.
        //if (GetSizer()) {
        //    GetSizer()->Layout();
        //}
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
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
    Refresh();
    event.Skip();
}

