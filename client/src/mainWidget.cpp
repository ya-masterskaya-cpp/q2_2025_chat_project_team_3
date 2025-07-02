#include <client/mainWidget.h>
#include <client/authPanel.h>
#include <client/chatPanel.h>
#include <client/wsClient.h>
#include <client/initialPanel.h>
#include <client/serversPanel.h>
#include <client/user.h>

namespace client {

const wxSize COMPACT_WINDOW_SIZE(450, 600);
const wxSize FULL_WINDOW_SIZE(600, 750);

MainWidget::MainWidget() : wxFrame(NULL, wxID_ANY, "Slightly Pretty Chat", wxDefaultPosition, COMPACT_WINDOW_SIZE) {
    wsClient = new WebSocketClient(this);
    initialPanel = new InitialPanel(this);
    serversPanel = new ServersPanel(this);
    authPanel = new AuthPanel(this);
    chatPanel = new ChatPanel(this);

    auto* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(initialPanel, 1, wxEXPAND);
    sizer->Add(serversPanel, 1, wxEXPAND);
    sizer->Add(authPanel, 1, wxEXPAND);
    sizer->Add(chatPanel, 1, wxEXPAND);
    SetSizer(sizer);

    this->Bind(wxEVT_ACTIVATE, &MainWidget::OnActivate, this);
}

void MainWidget::OnActivate([[maybe_unused]] wxActivateEvent& event) {
    if(chatPanel->IsShown()) {
        chatPanel->InvalidateCaches();
    }
}

void MainWidget::ShowPopup(const wxString& msg, long icon) {
    wxMessageBox(msg, "Info", wxOK | icon, this);
}

void MainWidget::ShowInitial() {
    if (this->GetSize() != COMPACT_WINDOW_SIZE) {
        this->SetSize(COMPACT_WINDOW_SIZE);
    }
    wsClient->stop();
    if (chatPanel->IsShown()) {
        chatPanel->ResetState();
    }
    initialPanel->Show();
    serversPanel->Hide();
    authPanel->Hide();
    chatPanel->Hide();
    Layout();
}

void MainWidget::ShowServers() {
    if (this->GetSize() != COMPACT_WINDOW_SIZE) {
        this->SetSize(COMPACT_WINDOW_SIZE);
    }
    if (chatPanel->IsShown()) {
        chatPanel->ResetState();
    }
    initialPanel->Hide();
    serversPanel->Show();
    authPanel->Hide();
    chatPanel->Hide();
    Layout();
}

void MainWidget::ShowAuth() {
    if (this->GetSize() != COMPACT_WINDOW_SIZE) {
        this->SetSize(COMPACT_WINDOW_SIZE);
    }
    if (chatPanel->IsShown()) {
        chatPanel->ResetState();
    }
    initialPanel->Hide();
    serversPanel->Hide();
    authPanel->Show();
    chatPanel->Hide();
    Layout();
}

void MainWidget::ShowMainChatView(const std::vector<Room*>& rooms, const User& user) {
    this->SetSize(FULL_WINDOW_SIZE);
    initialPanel->Hide();
    serversPanel->Hide();
    authPanel->Hide();

    chatPanel->Show();
    chatPanel->SetCurrentUser(user);
    chatPanel->PopulateInitialRoomList(rooms);

    Layout();
}

} // namespace client
