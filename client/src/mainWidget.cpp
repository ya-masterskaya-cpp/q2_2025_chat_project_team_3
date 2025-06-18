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

MainWidget::MainWidget() : wxFrame(NULL, wxID_ANY, "Slightly Pretty Chat", wxDefaultPosition, wxSize(450, 600)) {
    wsClient = new WebSocketClient(this);
    initialPanel = new InitialPanel(this);
    serversPanel = new ServersPanel(this);
    authPanel = new AuthPanel(this);
    roomsPanel = new RoomsPanel(this);
    chatPanel = new ChatPanel(this);

    auto* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(initialPanel, 1, wxEXPAND);
    sizer->Add(serversPanel, 1, wxEXPAND);
    sizer->Add(authPanel, 1, wxEXPAND);
    sizer->Add(roomsPanel, 1, wxEXPAND);
    sizer->Add(chatPanel, 1, wxEXPAND);
    SetSizer(sizer);
}

void MainWidget::ShowPopup(const wxString& msg, long icon) {
    wxMessageBox(msg, "Info", wxOK | icon, this);
}

void MainWidget::ShowInitial() {
    wsClient->stop();
    initialPanel->Show();
    serversPanel->Hide();
    authPanel->Hide();
    roomsPanel->Hide();
    chatPanel->Hide();
    Layout();
}

void MainWidget::ShowServers() {
    initialPanel->Hide();
    serversPanel->Show();
    authPanel->Hide();
    roomsPanel->Hide();
    chatPanel->Hide();
    Layout();
}

void MainWidget::ShowAuth() {
    initialPanel->Hide();
    serversPanel->Hide();
    authPanel->Show();
    roomsPanel->Hide();
    chatPanel->Hide();
    Layout();
}

void MainWidget::ShowRooms() {
    initialPanel->Hide();
    serversPanel->Hide();
    authPanel->Hide();
    roomsPanel->Show();
    chatPanel->Hide();
    Layout();
}

void MainWidget::ShowChat(std::vector<User> users) {
    initialPanel->Hide();
    serversPanel->Hide();
    authPanel->Hide();
    roomsPanel->Hide();
    chatPanel->Show();
    chatPanel->m_roomHeaderPanel->SetRoom(roomsPanel->GetSelectedRoom().value());
    chatPanel->m_userListPanel->SetUserList(std::move(users));
    chatPanel->m_messageView->Start();
    Layout();
}
