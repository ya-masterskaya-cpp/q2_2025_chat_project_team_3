#pragma once

#include <wx/wx.h>
#include <client/user.h>

class UserNameWidget : public wxPanel {
public:
    UserNameWidget(wxWindow* parent, const User& user);

    const User& GetUser() const {
        return m_user;
    }

private:
    wxStaticText* m_usernameText;
    User m_user;
};
