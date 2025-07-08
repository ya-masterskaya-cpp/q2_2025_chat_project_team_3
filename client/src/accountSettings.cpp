#include <client/accountSettings.h>
#include <client/mainWidget.h>
#include <client/wsClient.h>
#include <client/chatInterface.h>
#include <client/chatPanel.h>
#include <client/passwordUtil.h>
#include <client/textUtil.h>
#include <common/utils/limits.h>

namespace client {

enum {
    ID_CHANGE_USERNAME_BTN = wxID_HIGHEST + 300,
    ID_CHANGE_PASSWORD_BTN,
    ID_NEW_USERNAME_TEXT,
    ID_BACK
};

wxBEGIN_EVENT_TABLE(AccountSettingsPanel, wxPanel)
    EVT_BUTTON(ID_CHANGE_USERNAME_BTN, AccountSettingsPanel::OnChangeUsername)
    EVT_BUTTON(ID_CHANGE_PASSWORD_BTN, AccountSettingsPanel::OnChangePassword)
    EVT_TEXT(ID_NEW_USERNAME_TEXT, AccountSettingsPanel::OnNewUsernameText)
    EVT_BUTTON(ID_BACK, AccountSettingsPanel::OnBack)
wxEND_EVENT_TABLE()


AccountSettingsPanel::AccountSettingsPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY), mainWin(static_cast<MainWidget*>(parent))
{
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);

    // --- Change username box ---
    auto* usernameBox = new wxStaticBoxSizer(wxVERTICAL, this, "Username");

    auto* currentUsernameSizer = new wxBoxSizer(wxHORIZONTAL);
    currentUsernameSizer->Add(new wxStaticText(this, wxID_ANY, "Current username:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    m_currentUsernameLabel = new wxStaticText(this, wxID_ANY, "");
    m_currentUsernameLabel->SetFont(GetFont().Bold());
    currentUsernameSizer->Add(m_currentUsernameLabel, 1, wxALIGN_CENTER_VERTICAL);
    usernameBox->Add(currentUsernameSizer, 0, wxEXPAND | wxALL, 10);

    usernameBox->Add(new wxStaticText(this, wxID_ANY, "New username:"), 0, wxLEFT | wxRIGHT | wxTOP, 10);
    m_newUsernameCtrl = new wxTextCtrl(this, ID_NEW_USERNAME_TEXT);
    usernameBox->Add(m_newUsernameCtrl, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

    m_changeUsernameButton = new wxButton(this, ID_CHANGE_USERNAME_BTN, "Change Username");
    usernameBox->Add(m_changeUsernameButton, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT | wxBOTTOM, 10);

    mainSizer->Add(usernameBox, 2, wxEXPAND | wxALL, 10);

    // --- Change password box ---
    auto* passwordBox = new wxStaticBoxSizer(wxVERTICAL, this, "Password");

    passwordBox->Add(new wxStaticText(this, wxID_ANY, "Old password:"), 0, wxLEFT | wxRIGHT | wxTOP, 10);
    m_oldPasswordCtrl = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
    passwordBox->Add(m_oldPasswordCtrl, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

    passwordBox->Add(new wxStaticText(this, wxID_ANY, "New password:"), 0, wxLEFT | wxRIGHT | wxTOP, 10);
    m_newPasswordCtrl = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
    passwordBox->Add(m_newPasswordCtrl, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

    passwordBox->Add(new wxStaticText(this, wxID_ANY, "Confirm new password:"), 0, wxLEFT | wxRIGHT | wxTOP, 10);
    m_confirmPasswordCtrl = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
    passwordBox->Add(m_confirmPasswordCtrl, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

    m_changePasswordButton = new wxButton(this, ID_CHANGE_PASSWORD_BTN, "Change Password");
    passwordBox->Add(m_changePasswordButton, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT | wxBOTTOM, 10);

    mainSizer->Add(passwordBox, 3, wxEXPAND | wxALL, 10);
    

    // --- Button "Back" ---
    m_backButton = new wxButton(this, ID_BACK, "Back");
    mainSizer->Add(m_backButton, 0, wxALIGN_LEFT | wxALL, 15);

    SetSizerAndFit(mainSizer);
    CentreOnParent();
}

void AccountSettingsPanel::OnChangeUsername(wxCommandEvent& event) {
    wxString newUsernameRaw = m_newUsernameCtrl->GetValue();
    wxString newUsername = TextUtil::SanitizeInput(newUsernameRaw);
    const User& currentUser = mainWin->chatInterface->m_chatPanel->GetCurrentUser();

    if (newUsername.IsEmpty()) {
        wxMessageBox("Username must consist of at least 1 non-special character.", "Error", wxOK | wxICON_ERROR, this);
        m_newUsernameCtrl->SetFocus();
        return;
    }

    if (newUsername == currentUser.username) {
        wxMessageBox("The new username is the same as the current one.", "Information", wxOK | wxICON_INFORMATION, this);
        return;
    }

    mainWin->wsClient->changeUsername(newUsername.utf8_string());
    m_newUsernameCtrl->Clear();
}

void AccountSettingsPanel::OnChangePassword(wxCommandEvent& event) {
    wxString oldPassword = m_oldPasswordCtrl->GetValue();
    wxString newPassword = m_newPasswordCtrl->GetValue();
    wxString confirmPassword = m_confirmPasswordCtrl->GetValue();

    if (oldPassword.IsEmpty() || newPassword.IsEmpty() || confirmPassword.IsEmpty()) {
        wxMessageBox("All password fields must be filled.", "Error", wxOK | wxICON_ERROR, this);
        return;
    }

    if (newPassword != confirmPassword) {
        wxMessageBox("The new password and confirmation do not match.", "Error", wxOK | wxICON_ERROR, this);
        m_newPasswordCtrl->Clear();
        m_confirmPasswordCtrl->Clear();
        m_newPasswordCtrl->SetFocus();
        return;
    }

    m_oldPassword = oldPassword;
    m_newPassword = newPassword;

    m_changePasswordButton->Disable();
    m_changeUsernameButton->Disable();
    m_oldPasswordCtrl->Clear();
    m_newPasswordCtrl->Clear();
    m_confirmPasswordCtrl->Clear();

    mainWin->wsClient->requestMySalt();
}

void AccountSettingsPanel::OnBack(wxCommandEvent& event) {
    mainWin->ShowAccountSettings(false);
}

void AccountSettingsPanel::OnPasswordChangeContinue(const std::string& currentSalt) {
    if (currentSalt.empty()) {
        mainWin->ShowPopup("Failed to retrieve data for password change. Please try again.", wxICON_ERROR);
        m_changePasswordButton->Enable();
        m_changeUsernameButton->Enable();
        return;
    }

    try {
        std::string oldPasswordHash = password::hash_password(m_oldPassword.utf8_string(), currentSalt);

        std::string newSalt = password::generate_salt();
        std::string newPasswordHash = password::hash_password(m_newPassword.utf8_string(), newSalt);

        mainWin->wsClient->changePassword(oldPasswordHash, newPasswordHash, newSalt);

        m_oldPassword.clear();
        m_newPassword.clear();
    }
    catch (const std::exception& ex) {
        mainWin->ShowPopup(ex.what(), wxICON_ERROR);
    }

    m_changePasswordButton->Enable();
    m_changeUsernameButton->Enable();
}

void AccountSettingsPanel::OnPasswordChangeFailed() {
    m_changePasswordButton->Enable();
    m_changeUsernameButton->Enable();
}

void AccountSettingsPanel::UpdateCurrentUsername(const wxString& newUsername) {
    m_currentUsernameLabel->SetLabel(newUsername);
    Layout();
}

void AccountSettingsPanel::OnNewUsernameText(wxCommandEvent& event) {
    event.Skip();
    TextUtil::LimitTextLength(m_newUsernameCtrl, common::limits::MAX_USERNAME_LENGTH);
}

} // namespace client
