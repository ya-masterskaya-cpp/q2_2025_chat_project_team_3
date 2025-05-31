#pragma once
#include <wx/wx.h>
class MainWidget;

class AuthPanel : public wxPanel {
public:
    AuthPanel(MainWidget* parent);

    wxTextCtrl* usernameInput;
    wxTextCtrl* passwordInput;
    wxButton* loginButton;
    wxButton* registerButton;

    void SetButtonsEnabled(bool enabled);

private:
    void OnLogin(wxCommandEvent&);
    void OnRegister(wxCommandEvent&);
    MainWidget* mainWin;
    wxDECLARE_EVENT_TABLE();
};