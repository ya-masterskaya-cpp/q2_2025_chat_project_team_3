#pragma once

#include <wx/wx.h>
#include <client/user.h>

namespace client {

class MainWidget;

class AccountSettingsPanel : public wxPanel {
public:
    AccountSettingsPanel(wxWindow* parent);

    void OnPasswordChangeContinue(const std::string& currentSalt);
    void OnPasswordChangeFailed();

    void UpdateCurrentUsername(const wxString& newUsername);

private:
    void OnChangeUsername(wxCommandEvent& event);
    void OnChangePassword(wxCommandEvent& event);
    void OnBack(wxCommandEvent& event);
    void OnNewUsernameText(wxCommandEvent& event);

    MainWidget* mainWin;

    wxString m_oldPassword;
    wxString m_newPassword;

    wxStaticText* m_currentUsernameLabel;
    wxTextCtrl* m_newUsernameCtrl;
    wxButton* m_changeUsernameButton;
    wxTextCtrl* m_oldPasswordCtrl;
    wxTextCtrl* m_newPasswordCtrl;
    wxTextCtrl* m_confirmPasswordCtrl;
    wxButton* m_changePasswordButton;
    wxButton* m_backButton;

    wxDECLARE_EVENT_TABLE();
};

} // namespace client
