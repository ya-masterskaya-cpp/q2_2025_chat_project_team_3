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
    m_usernameText->SetForegroundColour(GetColourByRole(user));

    sizer->Add(m_usernameText, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    SetSizer(sizer);
    m_usernameText->Bind(wxEVT_RIGHT_DOWN, &UserNameWidget::PropagateRightClick, this);
}

void UserNameWidget::UpdateDisplay(const User& user) {
    m_user = user;
    m_usernameText->SetLabel(user.username);
    m_usernameText->SetForegroundColour(GetColourByRole(user));
    m_usernameText->Refresh();
}

void UserNameWidget::PropagateRightClick(wxMouseEvent &event) {
    wxMouseEvent newEvent(event);
    newEvent.SetEventObject(this);
    ProcessWindowEvent(newEvent);
}

wxColour UserNameWidget::GetColourByRole(const User& user) {
    switch (user.role) {
    case chat::UserRights::OWNER:
        return *wxRED;
    case chat::UserRights::ADMIN:
        return *wxGREEN;
    case chat::UserRights::MODERATOR:
        return *wxBLUE;
    case chat::UserRights::REGULAR:
    default:
        return wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
    }
}

} // namespace client
