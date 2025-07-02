#pragma once

#include <wx/wx.h>

namespace client {

struct User;
class ChatPanel;

class UserListPanel : public wxPanel {
public:
    UserListPanel(wxWindow* parent, ChatPanel* chatPanel);

    void SetUserList(std::vector<User> users);
    void AddUser(const User& user);
    void RemoveUser(int32_t userId);
    void Clear();
    void UpdateUserRole(int32_t userId, chat::UserRights newRole);

private:
    void OnUserRightClick(wxMouseEvent& event);

    wxScrolledWindow* m_userContainer;
    wxBoxSizer* m_userSizer;
    std::vector<User> m_users;
    ChatPanel* m_chatPanel;
};

} // namespace client
