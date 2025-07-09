#pragma once

#include <wx/wx.h>

namespace client {

class MainWidget;

class AuthPanel : public wxPanel {
public:
    AuthPanel(MainWidget* parent);

    wxTextCtrl* m_usernameInput;
    wxTextCtrl* m_passwordInput;
    wxButton* m_loginButton;
    wxButton* m_registerButton;
    wxButton* m_backButton;

    void SetButtonsEnabled(bool enabled);
    void HandleRegisterContinue();
    void HandleAuthContinue(const std::string& salt);

private:
    void OnLogin(wxCommandEvent&);
    void OnRegister(wxCommandEvent&);
    void OnBack(wxCommandEvent&);
    void OnInputLogin(wxCommandEvent& event);

    MainWidget* mainWin;
    wxString m_password;
};

} // namespace client
