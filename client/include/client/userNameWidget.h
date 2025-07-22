#pragma once

#include <wx/wx.h>
#include <client/user.h>

namespace client {

class CachedColorText;

class UserNameWidget : public wxPanel {
public:
    UserNameWidget(wxWindow* parent, const User& user);

    const User& GetUser() const {
        return m_user;
    }

private:
    void PropagateRightClick(wxMouseEvent& event);
    
    CachedColorText* m_usernameText;
    User m_user;
};

} // namespace client
