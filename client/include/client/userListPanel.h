#pragma once

#include <wx/wx.h>

#include <client/user.h>

class UserListPanel : public wxPanel {
public:
    UserListPanel(wxWindow* parent);

    void SetUserList(std::vector<User> users);
    void AddUser(const User& user);
    void RemoveUser(int32_t userId);
    void Clear();
    void UpdateUserRole(int32_t userId, chat::UserRights newRights);

private:

    wxScrolledWindow* m_userContainer;
    wxBoxSizer* m_userSizer;
};
