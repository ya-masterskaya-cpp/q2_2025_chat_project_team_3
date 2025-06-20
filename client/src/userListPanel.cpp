#include <client/userListPanel.h>
#include <client/userNameWidget.h>

UserListPanel::UserListPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY) {
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
    
    auto* sizer = new wxBoxSizer(wxVERTICAL);

    auto* header = new wxStaticText(this, wxID_ANY, "Users in room:");
    header->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
    sizer->Add(header, 0, wxALL, FromDIP(5));

    m_userContainer = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL);
    m_userContainer->SetScrollRate(0, FromDIP(10));
    m_userContainer->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));

    m_userSizer = new wxBoxSizer(wxVERTICAL);
    m_userContainer->SetSizer(m_userSizer);

    sizer->Add(m_userContainer, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(5));
    SetSizer(sizer);
}

void UserListPanel::SetUserList(std::vector<User> users) {
    m_userContainer->Freeze();
    m_userSizer->Clear(true); // Destroy existing widgets

    // Sort the incoming user list by the User::operator<
    std::sort(users.begin(), users.end(), [] (const User& lhs, const User& rhs) {
        return !(lhs < rhs);
    });

    for (const auto& user : users) {
        auto* userWidget = new UserNameWidget(m_userContainer, user);
        m_userSizer->Add(userWidget, 0, wxEXPAND | wxALL, FromDIP(2));
    }

    m_userSizer->Layout();
    m_userContainer->SetVirtualSize(m_userSizer->GetMinSize());
    m_userContainer->Scroll(0, 0); // Scroll to top on full list update
    m_userContainer->Thaw();
}

void UserListPanel::AddUser(const User& user) {
    m_userContainer->Freeze();

    // Find the correct insertion index to maintain sorted order
    int insertIndex = 0;
    for (auto* item : m_userSizer->GetChildren()) {
        if (item->IsWindow()) {
            UserNameWidget* existingWidget = static_cast<UserNameWidget*>(item->GetWindow());
            // Use the User::operator< to compare and find the insertion point
            if (existingWidget && user < existingWidget->GetUser()) {
                break; // Found the spot where the new user should be inserted
            }
            insertIndex++;
        }
    }

    auto* userWidget = new UserNameWidget(m_userContainer, user);
    m_userSizer->Insert(insertIndex, userWidget, 0, wxEXPAND | wxALL, FromDIP(2));

    m_userSizer->Layout();
    m_userContainer->SetVirtualSize(m_userSizer->GetMinSize());
    m_userContainer->Thaw();
}

void UserListPanel::RemoveUser(int userId) { // Now accepts user ID
    m_userContainer->Freeze();

    UserNameWidget* widgetToRemove = nullptr;
    int removeIndex = wxNOT_FOUND;

    // Iterate through current widgets to find the one matching the ID
    for (size_t i = 0; i < m_userSizer->GetChildren().size(); ++i) {
        wxSizerItem* item = m_userSizer->GetChildren()[i];
        if (item->IsWindow()) {
            UserNameWidget* userWidget = static_cast<UserNameWidget*>(item->GetWindow());
            if (userWidget && userWidget->GetUser().id == userId) { // Compare by ID
                widgetToRemove = userWidget;
                removeIndex = i;
                break;
            }
        }
    }

    if (widgetToRemove) {
        m_userSizer->Remove(removeIndex); // Remove from sizer
        widgetToRemove->Destroy();         // Destroy the widget (important for memory management)
    }

    m_userSizer->Layout();
    m_userContainer->SetVirtualSize(m_userSizer->GetMinSize());
    m_userContainer->Thaw();
}
