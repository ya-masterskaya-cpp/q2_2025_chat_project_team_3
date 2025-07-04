#include <client/userListPanel.h>
#include <client/userNameWidget.h>
#include <client/chatPanel.h>
#include <client/user.h>

namespace client {

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

void UserListPanel::UpdateUserRole(int32_t userId, chat::UserRights newRole) {
    auto it = m_userWidgets.find(userId);
    if (it != m_userWidgets.end()) {
        User user = it->second->GetUser();
        user.role = newRole;
        it->second->UpdateDisplay(user);

        SortUserWidgets();
    }
}

void UserListPanel::SetUserList(std::vector<User> users) {
    Clear();
    m_userContainer->Freeze();

    for (const auto& user : users) {
        auto currUser = static_cast<ChatPanel*>(GetParent())->GetCurrentUser();
            if (currUser.id == user.id) {
                static_cast<ChatPanel*>(GetParent())->SetCurrentUser(user);
            }
        auto* userWidget = new UserNameWidget(m_userContainer, user);
        userWidget->Bind(wxEVT_RIGHT_DOWN, &UserListPanel::OnUserRightClick, this);
        m_userSizer->Add(userWidget, 0, wxEXPAND | wxALL, FromDIP(2));
        m_userWidgets[user.id] = userWidget;
    }
    SortUserWidgets();
    m_userContainer->Thaw();
}

void UserListPanel::AddUser(const User& user) {
    if (m_userWidgets.count(user.id)) {
        UpdateUserRole(user.id, user.role);
        return;
    }
    m_userContainer->Freeze();

    auto* userWidget = new UserNameWidget(m_userContainer, user);
    userWidget->Bind(wxEVT_RIGHT_DOWN, &UserListPanel::OnUserRightClick, this);
    m_userWidgets[user.id] = userWidget;
    m_userSizer->Add(userWidget, 0, wxEXPAND | wxALL, FromDIP(2));

    SortUserWidgets();
    m_userContainer->Thaw();
}

void UserListPanel::RemoveUser(int userId) {
    auto it = m_userWidgets.find(userId);
    if (it != m_userWidgets.end()) {
        m_userContainer->Freeze();

        it->second->Destroy();
        m_userWidgets.erase(it);

        SortUserWidgets();
        m_userContainer->Thaw();
    }
}

void UserListPanel::Clear() {
    m_userContainer->Freeze();
    m_userSizer->Clear(true);
    m_userWidgets.clear();
    m_userContainer->Thaw();
}

void UserListPanel::OnUserRightClick(wxMouseEvent& event) {
    event.Skip();

    auto* clickedWidget = dynamic_cast<UserNameWidget*>(event.GetEventObject());
    if(!clickedWidget) {
        return;
    }
    const User& targetUser = clickedWidget->GetUser();

    if(targetUser.role >= chat::UserRights::OWNER) {
        return;
    }

    auto currentUser = static_cast<ChatPanel*>(GetParent())->GetCurrentUser();

    if(currentUser.id == targetUser.id) {
        return;
    }
    if(currentUser.role <= targetUser.role) {
        return;
    }

    wxMenu menu;

    if(currentUser.role > chat::UserRights::MODERATOR) {
        // MODERATOR role assing\unassign
        if(targetUser.role == chat::UserRights::REGULAR) {
            auto* item = menu.Append(wxID_ANY, "Assign Moderator");
            menu.Bind(wxEVT_MENU, [this, targetUser](wxCommandEvent&) {
                wxCommandEvent newEvent(wxEVT_ASSIGN_MODERATOR, GetId());
                newEvent.SetEventObject(this);
                newEvent.SetInt(targetUser.id);
                ProcessEvent(newEvent);
            }, item->GetId());
        } else if(targetUser.role == chat::UserRights::MODERATOR) {
            auto* item = menu.Append(wxID_ANY, "Unassign Moderator");
            menu.Bind(wxEVT_MENU, [this, targetUser](wxCommandEvent&) {
                wxCommandEvent newEvent(wxEVT_UNASSIGN_MODERATOR, GetId());
                newEvent.SetEventObject(this);
                newEvent.SetInt(targetUser.id);
                ProcessEvent(newEvent);
            }, item->GetId());
        }
    }

    // OWNER transfer
    if(currentUser.role >= chat::UserRights::OWNER) {
        auto* item = menu.Append(wxID_ANY, "Transfer Ownership");
        menu.Bind(wxEVT_MENU, [this, targetUser](wxCommandEvent&) {
            wxCommandEvent newEvent(wxEVT_TRANSFER_OWNERSHIP, GetId());
            newEvent.SetEventObject(this);
            newEvent.SetInt(targetUser.id);
            ProcessEvent(newEvent);
        }, item->GetId());
    }

    if(menu.GetMenuItemCount() > 0) {
        PopupMenu(&menu);
    }
}

void UserListPanel::SortUserWidgets() {
    m_userContainer->Freeze();

    wxSizerItemList& items = m_userSizer->GetChildren();

    std::vector<UserNameWidget*> widgetsToSort;
    widgetsToSort.reserve(items.size());
    for (wxSizerItem* item : items) {
        if (auto* widget = dynamic_cast<UserNameWidget*>(item->GetWindow())) {
            widgetsToSort.push_back(widget);
        }
    }

    std::sort(widgetsToSort.begin(), widgetsToSort.end(),
        [](const UserNameWidget* lhs, const UserNameWidget* rhs) {
            return !(lhs->GetUser() < rhs->GetUser());
        });

    for (UserNameWidget* widget : widgetsToSort) {
        m_userSizer->Detach(widget);
    }

    for (UserNameWidget* widget : widgetsToSort) {
        m_userSizer->Add(widget, 0, wxEXPAND | wxALL, FromDIP(2));
    }

    m_userSizer->Layout();
    m_userContainer->SetVirtualSize(m_userSizer->GetMinSize());
    m_userContainer->Scroll(0, 0);
    m_userContainer->Thaw();
}

} // namespace client
