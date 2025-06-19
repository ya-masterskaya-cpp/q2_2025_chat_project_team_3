#include <client/authPanel.h>
#include <client/mainWidget.h>
#include <client/wsClient.h>
#include <client/passwordUtil.h>
#include <optional>

void AuthPanel::SetButtonsEnabled(bool enabled) {
    m_usernameInput->Enable(enabled);
    m_passwordInput->Enable(enabled);
    m_loginButton->Enable(enabled);
    m_registerButton->Enable(enabled);
}

AuthPanel::AuthPanel(MainWidget* parent) : wxPanel(parent), mainWin(parent) {
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->AddStretchSpacer();
    sizer->Add(new wxStaticText(this, wxID_ANY, "Username:"), 0, wxALL | wxALIGN_CENTER, 5);

    m_usernameInput = new wxTextCtrl(this, wxID_ANY);
    sizer->Add(m_usernameInput, 0, wxALL | wxEXPAND, 5);
    sizer->Add(new wxStaticText(this, wxID_ANY, "Password:"), 0, wxALL | wxALIGN_CENTER, 5);
    m_passwordInput = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
    sizer->Add(m_passwordInput, 0, wxALL | wxEXPAND, 5);

    auto* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    m_loginButton = new wxButton(this, wxID_ANY, "Login");
    m_registerButton = new wxButton(this, wxID_ANY, "Register");
    m_backButton = new wxButton(this, wxID_ANY, "Back");
    btnSizer->Add(m_loginButton, 1, wxALL, 5);
    btnSizer->Add(m_registerButton, 1, wxALL, 5);
    btnSizer->Add(m_backButton, 1, wxALL, 5);

    sizer->Add(btnSizer, 0, wxALIGN_CENTER);
    sizer->AddStretchSpacer();
    SetSizer(sizer);

    m_loginButton   ->Bind(wxEVT_BUTTON, &AuthPanel::OnLogin,    this);
    m_registerButton->Bind(wxEVT_BUTTON, &AuthPanel::OnRegister, this);
    m_backButton    ->Bind(wxEVT_BUTTON, &AuthPanel::OnBack,     this);
    m_usernameInput ->Bind(wxEVT_TEXT, &AuthPanel::OnInputLogin, this);

    //SetButtonsEnabled(false); // Disable by default until connection is ready
}

void AuthPanel::OnLogin(wxCommandEvent&) {
    m_password = m_passwordInput->GetValue();
    mainWin->wsClient->requestInitialAuth(std::string(m_usernameInput->GetValue().ToUTF8()));
}

void AuthPanel::OnRegister(wxCommandEvent&) {
    wxString password = m_passwordInput->GetValue();
    if (password.IsEmpty()) {
        wxMessageBox("Password can't be empty",
            "Registration error",
            wxOK | wxICON_ERROR, this
        );
        m_passwordInput->SetFocus();
        return;
    }
    m_password = password;
    mainWin->wsClient->requestInitialRegister(m_usernameInput->GetValue().utf8_string());
}

void AuthPanel::OnBack(wxCommandEvent&) {
    mainWin->ShowInitial();
}

void AuthPanel::OnInputLogin(wxCommandEvent& event) {
    event.Skip();
    wxString val = m_usernameInput->GetValue();
    if (val.length() > MAX_USERNAME_LENGTH) {
        size_t pos = static_cast<size_t>(m_usernameInput->GetInsertionPoint());
        wxString cated_str = val.Left(MAX_USERNAME_LENGTH);
        CallAfter([this, cated_str, pos]() {
            m_usernameInput->SetValue(cated_str);
            m_usernameInput->SetInsertionPoint((std::min)(pos, MAX_USERNAME_LENGTH));
            wxBell();
        });
    }
}

void AuthPanel::HandleRegisterContinue() {
    try {
        std::string salt = password::generate_salt();
        std::string hash = password::hash_password(std::string(m_password.ToUTF8()), salt);
        mainWin->wsClient->completeRegister(hash, salt);
    } catch (const std::exception& ex) {
        mainWin->ShowPopup(ex.what(), wxICON_ERROR);
    }
    m_password.clear();
}

void AuthPanel::HandleAuthContinue(const std::string &salt) {
    try {
        if (salt.empty()) {
            std::string new_salt = password::generate_salt();
            std::string hash = password::hash_password(std::string(m_password.ToUTF8()), new_salt);
            mainWin->wsClient->completeAuth(hash, std::string(m_password.ToUTF8()), new_salt);
        } else {
            std::string hash = password::hash_password(std::string(m_password.ToUTF8()), salt);
            mainWin->wsClient->completeAuth(hash, std::nullopt, std::nullopt);
        }
    } catch (const std::exception& ex) {
        mainWin->ShowPopup(ex.what(), wxICON_ERROR);
    }
    m_password.clear();
}