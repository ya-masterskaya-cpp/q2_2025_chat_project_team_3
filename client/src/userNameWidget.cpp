#include <client/userNameWidget.h>

UserNameWidget::UserNameWidget(wxWindow* parent, const User& user)
    : wxPanel(parent, wxID_ANY), m_user(user) {
    auto* sizer = new wxBoxSizer(wxHORIZONTAL);
    m_usernameText = new wxStaticText(this, wxID_ANY, user.username);

    // Set color based on role
    wxColour textColor;
    switch (user.role) {
        case UserRole::Owner:
            textColor = *wxRED;
            break;
        case UserRole::Admin:
            textColor = *wxGREEN;
            break;
        case UserRole::Moderator:
            textColor = *wxBLUE;
            break;
        case UserRole::Regular:
        default:
            textColor = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
            break;
    }
    m_usernameText->SetForegroundColour(textColor);

    sizer->Add(m_usernameText, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    SetSizer(sizer);
}
