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

private:

    wxScrolledWindow* m_userContainer;
    wxBoxSizer* m_userSizer;
};

} // namespace client
