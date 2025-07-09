#include <client/mainWidget.h>
#include <client/authPanel.h>
#include <client/roomsPanel.h>
#include <client/chatPanel.h>
#include <client/wsClient.h>
#include <client/userListPanel.h>
#include <client/messageView.h>
#include <client/initialPanel.h>
#include <client/serversPanel.h>
#include <client/roomHeaderPanel.h>
#include <client/chatInterface.h>
#include <client/accountSettings.h>
#include <client/user.h>

namespace client {

MainWidget::MainWidget() : wxFrame(NULL, wxID_ANY, "Slightly Pretty Chat", wxDefaultPosition, wxSize(900, 600)) {
    wsClient = new WebSocketClient(this);
    initialPanel = new InitialPanel(this);
    serversPanel = new ServersPanel(this);
    authPanel = new AuthPanel(this);
    chatInterface = new ChatInterface(this);
    accountSettingsPanel = new AccountSettingsPanel(this);

    auto* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(initialPanel, 1, wxEXPAND);
    sizer->Add(serversPanel, 1, wxEXPAND);
    sizer->Add(authPanel, 1, wxEXPAND);
    sizer->Add(chatInterface, 1, wxEXPAND);
    sizer->Add(accountSettingsPanel, 1, wxEXPAND);
    SetSizer(sizer);

    this->Bind(wxEVT_ACTIVATE, &MainWidget::OnActivate, this);
}

void MainWidget::OnActivate([[maybe_unused]] wxActivateEvent& event) {
    if(chatInterface->m_chatPanel->IsShown()) {
        chatInterface->m_chatPanel->InvalidateCaches();
    }
}

void MainWidget::ShowPopup(const wxString& msg, long icon) {
    wxMessageBox(msg, "Info", wxOK | icon, this);
}

void MainWidget::ShowInitial() {
    wsClient->stop();
    if (chatInterface->m_chatPanel->IsShown()) {
        chatInterface->m_chatPanel->ResetState();
    }
    initialPanel->Show();
    serversPanel->Hide();
    authPanel->Hide();
    chatInterface->Hide();
    chatInterface->m_chatPanel->Hide();
    accountSettingsPanel->Hide();
    Layout();
}

void MainWidget::ShowServers() {
    if (chatInterface->m_chatPanel->IsShown()) {
        chatInterface->m_chatPanel->ResetState();
    }
    initialPanel->Hide();
    serversPanel->Show();
    authPanel->Hide();
    chatInterface->Hide();
    chatInterface->m_chatPanel->Hide();
    accountSettingsPanel->Hide();
    Layout();
}

void MainWidget::ShowAuth() {
    if (chatInterface->m_chatPanel->IsShown()) {
        chatInterface->m_chatPanel->ResetState();
    }
    initialPanel->Hide();
    serversPanel->Hide();
    authPanel->Show();
    chatInterface->Hide();
    chatInterface->m_chatPanel->Hide();
    accountSettingsPanel->Hide();
    Layout();
}

void MainWidget::ShowRooms() {
    initialPanel->Hide();
    serversPanel->Hide();
    authPanel->Hide();
    chatInterface->Show();
    chatInterface->m_chatPanel->Hide();
    accountSettingsPanel->Hide();
    Layout();
}

void MainWidget::ShowChat(std::vector<User> users) {
    initialPanel->Hide();
    serversPanel->Hide();
    authPanel->Hide();
    accountSettingsPanel->Hide();

    chatInterface->m_chatPanel->ResetState();
    chatInterface->m_chatPanel->Show();
    chatInterface->Show();
    chatInterface->m_chatPanel->m_roomHeaderPanel->SetRoom(chatInterface->m_roomsPanel->GetSelectedRoom().value());
    chatInterface->m_chatPanel->m_userListPanel->SetUserList(std::move(users));
    chatInterface->m_chatPanel->m_messageView->Start();
    Layout();
}

void MainWidget::ShowAccountSettings(bool show = true) {
    initialPanel->Hide();
    serversPanel->Hide();
    authPanel->Hide();
    if (show) {
        chatInterface->Hide();
        accountSettingsPanel->Show();
    } else {
        chatInterface->Show();
        accountSettingsPanel->Hide();
    }
    Layout();
}

} // namespace client
