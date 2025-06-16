#include <client/mainWidget.h>
#include <client/authPanel.h>
#include <client/roomsPanel.h>
#include <client/chatPanel.h>
#include <client/wsClient.h>
#include <client/userListPanel.h>
#include <client/messageView.h>

MainWidget::MainWidget() : wxFrame(NULL, wxID_ANY, "Drogon wxWidgets Chat Client", wxDefaultPosition, wxSize(450, 600)) {
    wsClient = new WebSocketClient(this);
    wsClient->start();
    authPanel = new AuthPanel(this);
    roomsPanel = new RoomsPanel(this);
    chatPanel = new ChatPanel(this);

    auto* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(authPanel, 1, wxEXPAND);
    sizer->Add(roomsPanel, 1, wxEXPAND);
    sizer->Add(chatPanel, 1, wxEXPAND);
    SetSizer(sizer);

    ShowAuth();
    Show();
}

void MainWidget::ShowPopup(const wxString& msg, long icon) {
    wxMessageBox(msg, "Info", wxOK | icon, this);
}

void MainWidget::ShowAuth() {
    authPanel->Show();
    roomsPanel->Hide();
    chatPanel->Hide();
    Layout();
}

void MainWidget::ShowRooms() {
    authPanel->Hide();
    roomsPanel->Show();
    chatPanel->Hide();
    Layout();
}

void MainWidget::ShowChat(std::vector<User> users) {
    //users.emplace_back(1000, "admin", chat::UserRights::ADMIN);
    //users.emplace_back(1001, "moderator", chat::UserRights::MODERATOR);
    authPanel->Hide();
    roomsPanel->Hide();
    chatPanel->Show();
    chatPanel->m_userListPanel->SetUserList(std::move(users));
    chatPanel->m_messageView->Start();
    Layout();
}
