#pragma once

#include <wx/wx.h>

#include <client/user.h>

namespace client {

class UserListPanel : public wxPanel {
public:
    UserListPanel(wxWindow* parent);

    void SetUserList(std::vector<User> users);
    void AddUser(const User& user);
    void RemoveUser(int32_t userId);
    void Clear();
    void SetCurrentUser(const User& user);
    void UpdateUserRole(int32_t userId, chat::UserRights newRole);

private:
    void OnUserRightClick(wxMouseEvent& event);

    wxScrolledWindow* m_userContainer;
    wxBoxSizer* m_userSizer;
    User m_currentUser;
    std::vector<User> m_users;
};

} // namespace client
