#include <client/userNameWidget.h>
#include <client/cachedColorText.h>

namespace client {

UserNameWidget::UserNameWidget(wxWindow* parent, const User& user)
    : wxPanel(parent, wxID_ANY), m_user(user) {
    SetBackgroundColour(parent->GetBackgroundColour());

    auto* sizer = new wxBoxSizer(wxHORIZONTAL);
    m_usernameText = new CachedColorText(this, wxID_ANY, user.username, wxDefaultPosition, wxDefaultSize, 0, wxStaticTextNameStr, false);
    //m_usernameText->Wrap(-1);
    // Set color based on role
    wxColour textColor;
    switch (user.role) {
        case chat::UserRights::OWNER:
            textColor = *wxRED;
            break;
        case chat::UserRights::ADMIN:
            textColor = *wxGREEN;
            break;
        case chat::UserRights::MODERATOR:
            textColor = *wxBLUE;
            break;
        case chat::UserRights::REGULAR:
        default:
            textColor = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
            break;
    }
    if (user.count == 0) {
        const unsigned char OFFLINE_ALPHA = 110;
        textColor = wxColour(textColor.Red(), textColor.Green(), textColor.Blue(), OFFLINE_ALPHA);
    }
    m_usernameText->SetForegroundColour(textColor);

    sizer->Add(m_usernameText, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    SetSizer(sizer);
    m_usernameText->Bind(wxEVT_RIGHT_DOWN, &UserNameWidget::PropagateRightClick, this);
}

void UserNameWidget::PropagateRightClick(wxMouseEvent &event) {
    wxMouseEvent newEvent(event);
    newEvent.SetEventObject(this);
    ProcessWindowEvent(newEvent);
}

} // namespace client
