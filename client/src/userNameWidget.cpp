#include <client/userNameWidget.h>

UserNameWidget::UserNameWidget(wxWindow* parent, const User& user)
    : wxPanel(parent, wxID_ANY), m_user(user) {
    auto* sizer = new wxBoxSizer(wxHORIZONTAL);
    m_usernameText = new wxStaticText(this, wxID_ANY, user.username);
    m_usernameText->Wrap(-1);
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
    m_usernameText->SetForegroundColour(textColor);

    sizer->Add(m_usernameText, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    SetSizer(sizer);

    // Bind(wxEVT_RIGHT_DOWN, &UserNameWidget::OnRightClick, this);
    // m_usernameText->Bind(wxEVT_RIGHT_DOWN, &UserNameWidget::OnRightClick, this);
}

void UserNameWidget::UpdateRole(chat::UserRights newRole) {
    wxColour textColor;
    switch (newRole) {
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
    m_usernameText->SetForegroundColour(textColor);
    Refresh();
}

void UserNameWidget::OnRightClick(wxMouseEvent& event) {
    UserNameWidget* clickedWidget = static_cast<UserNameWidget*>(event.GetEventObject());
    if (!clickedWidget) {
        event.Skip();
        return;
    }
    
    // Store the selected user for menu actions
    //m_selectedUser = clickedWidget->GetUser();
    
    wxMenu menu;
    menu.Append(wxID_ANY, "Private message");
    menu.AppendSeparator();
    menu.Append(wxID_ANY, "Kick user");
    menu.Append(wxID_ANY, "Ban user");
    
    // Bind menu events
    //menu.Bind(wxEVT_MENU, &UserListPanel::OnPrivateMessage, this, ID_PRIVATE_MESSAGE);
    //menu.Bind(wxEVT_MENU, &UserListPanel::OnKickUser, this, ID_KICK_USER);
    //menu.Bind(wxEVT_MENU, &UserListPanel::OnBanUser, this, ID_BAN_USER);
    
    PopupMenu(&menu, event.GetPosition());
}
