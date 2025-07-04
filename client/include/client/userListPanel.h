#pragma once

#include <wx/wx.h>

namespace client {

struct User;
class UserNameWidget;

class UserListPanel : public wxPanel {
public:
    UserListPanel(wxWindow* parent);

    void SetUserList(std::vector<User> users);
    void AddUser(const User& user);
    void RemoveUser(int32_t userId);
    void Clear();
    void UpdateUserRole(int32_t userId, chat::UserRights newRole);

private:
    void OnUserRightClick(wxMouseEvent& event);

    void SortUserWidgets();

    wxScrolledWindow* m_userContainer;
    wxBoxSizer* m_userSizer;
    std::unordered_map<int32_t, UserNameWidget*> m_userWidgets;
};

} // namespace client
