#include <client/authPanel.h>
#include <client/mainWidget.h>
#include <client/wsClient.h>

enum { ID_LOGIN = wxID_HIGHEST+10, ID_REGISTER };

wxBEGIN_EVENT_TABLE(AuthPanel, wxPanel)
    EVT_BUTTON(ID_LOGIN, AuthPanel::OnLogin)
    EVT_BUTTON(ID_REGISTER, AuthPanel::OnRegister)
wxEND_EVENT_TABLE()

void AuthPanel::SetButtonsEnabled(bool enabled) {
    loginButton->Enable(enabled);
    registerButton->Enable(enabled);
}

AuthPanel::AuthPanel(MainWidget* parent) : wxPanel(parent), mainWin(parent) {
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->AddStretchSpacer();
    sizer->Add(new wxStaticText(this, wxID_ANY, "Username:"), 0, wxALL | wxALIGN_CENTER, 5);
    usernameInput = new wxTextCtrl(this, wxID_ANY);
    sizer->Add(usernameInput, 0, wxALL | wxEXPAND, 5);
    sizer->Add(new wxStaticText(this, wxID_ANY, "Password:"), 0, wxALL | wxALIGN_CENTER, 5);
    passwordInput = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
    sizer->Add(passwordInput, 0, wxALL | wxEXPAND, 5);
    auto* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    loginButton = new wxButton(this, ID_LOGIN, "Login");
    registerButton = new wxButton(this, ID_REGISTER, "Register");
    btnSizer->Add(loginButton, 1, wxALL, 5);
    btnSizer->Add(registerButton, 1, wxALL, 5);
    sizer->Add(btnSizer, 0, wxALIGN_CENTER);
    sizer->AddStretchSpacer();
    SetSizer(sizer);

    SetButtonsEnabled(false); // Disable by default until connection is ready
}

void AuthPanel::OnLogin(wxCommandEvent&) {
    mainWin->wsClient->loginUser(
        usernameInput->GetValue().ToStdString(),
        passwordInput->GetValue().ToStdString()
    );
}
void AuthPanel::OnRegister(wxCommandEvent&) {
    mainWin->wsClient->registerUser(
        usernameInput->GetValue().ToStdString(),
        passwordInput->GetValue().ToStdString()
    );
}
