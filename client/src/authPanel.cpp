#include <client/authPanel.h>
#include <client/mainWidget.h>
#include <client/wsClient.h>
#include <client/passwordUtil.h>
#include <optional>

enum { ID_LOGIN = wxID_HIGHEST+10, ID_REGISTER };

wxBEGIN_EVENT_TABLE(AuthPanel, wxPanel)
    EVT_BUTTON(ID_LOGIN, AuthPanel::OnLogin)
    EVT_BUTTON(ID_REGISTER, AuthPanel::OnRegister)
wxEND_EVENT_TABLE()

void AuthPanel::SetButtonsEnabled(bool enabled) {
    usernameInput->Enable(enabled);
    passwordInput->Enable(enabled);
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
    m_password = passwordInput->GetValue();
    mainWin->wsClient->requestInitialAuth(usernameInput->GetValue().ToStdString());
}

void AuthPanel::OnRegister(wxCommandEvent&) {
    m_password = passwordInput->GetValue();
    mainWin->wsClient->requestInitialRegister(usernameInput->GetValue().ToStdString());
}

void AuthPanel::HandleRegisterContinue() {
    std::string salt = password::generate_salt();
    mainWin->wsClient->completeRegister(password::hash_password(m_password.ToStdString(), salt), salt);
    m_password.clear();
}

void AuthPanel::HandleAuthContinue(const std::string &salt) {
    if (salt.empty()){
        std::string new_salt = password::generate_salt();
        std::string hash = password::hash_password(m_password.ToStdString(), new_salt);
        mainWin->wsClient->completeAuth(hash, m_password.ToStdString(), new_salt);
    } else {
        std::string hash = password::hash_password(m_password.ToStdString(), salt);
        mainWin->wsClient->completeAuth(hash, std::nullopt, std::nullopt);
    }
    m_password.clear();
}