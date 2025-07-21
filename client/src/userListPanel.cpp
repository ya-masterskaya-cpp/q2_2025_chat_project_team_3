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
    std::for_each(m_users.begin(), m_users.end(), [userId, newRole](User& u) {
        if(u.id == userId) {
            u.role = newRole;
        }
    });

    SetUserList(std::move(m_users));
}

void UserListPanel::UpdateUsername(int32_t userId, const wxString& newUsername) {
    std::for_each(m_users.begin(), m_users.end(), [userId, newUsername](User& u) {
        if (u.id == userId) {
            u.username = newUsername;
        }
    });

    SetUserList(std::move(m_users));
}

void UserListPanel::SetUserList(std::vector<User> users) {
    m_userContainer->Freeze();

    wxSizerItemList itemsToDestroy = m_userSizer->GetChildren();
    for (wxSizerItem* item : itemsToDestroy) {
        if (item->GetWindow()) {
            item->GetWindow()->Destroy(); // Schedules deletion, modifies the original sizer's list
        }
    }
    m_userSizer->Clear(false); // Detach anything remaining

    // Sort the incoming user list by the User::operator<
    std::sort(users.begin(), users.end(), [] (const User& lhs, const User& rhs) {
        return !(lhs < rhs);
    });

    m_users = std::move(users);

    auto currUser = static_cast<ChatPanel*>(GetParent())->GetCurrentUser();

    for (const auto& user : m_users) {
        if (currUser.id == user.id) {
            static_cast<ChatPanel*>(GetParent())->SetCurrentUser(user);
        }

        auto* userWidget = new UserNameWidget(m_userContainer, user);
        userWidget->Bind(wxEVT_RIGHT_DOWN, &UserListPanel::OnUserRightClick, this);
        m_userSizer->Add(userWidget, 0, wxEXPAND | wxALL, FromDIP(2));
    }

    m_userSizer->Layout();
    m_userContainer->SetVirtualSize(m_userSizer->GetMinSize());
    m_userContainer->Scroll(0, 0); // Scroll to top on full list update
    m_userContainer->Thaw();
}

void UserListPanel::AddUser(const User& user) {
    auto it = std::find_if(m_users.begin(), m_users.end(), [userId = user.id](const User& u) {
		return u.id == userId; });
    if (it != m_users.end()) {
        ++it->count;
    } else {
		m_users.push_back(user);
    }
    SetUserList(std::move(m_users));
}

void UserListPanel::RemoveUser(int userId) {
    auto it = std::find_if(m_users.begin(), m_users.end(), [userId](const User& u) {
        return u.id == userId; });

    if (it != m_users.end()) {
        if (it->count > 0) {
            --it->count;
        } else {
            it = m_users.erase(it);
		}
        SetUserList(std::move(m_users));
    }
}

void UserListPanel::Clear() {
    m_users.clear();
    SetUserList({});
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

} // namespace client
