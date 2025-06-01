#include <client/chatPanel.h>
#include <client/mainWidget.h>
#include <client/wsClient.h>

enum { ID_SEND = wxID_HIGHEST+30, ID_LEAVE };

wxBEGIN_EVENT_TABLE(ChatPanel, wxPanel)
    EVT_BUTTON(ID_SEND, ChatPanel::OnSend)
    EVT_BUTTON(ID_LEAVE, ChatPanel::OnLeave)
wxEND_EVENT_TABLE()

ChatPanel::ChatPanel(MainWidget* parent) : wxPanel(parent), mainWin(parent) {
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    chatBox = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 300),
                             wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    sizer->Add(chatBox, 1, wxALL | wxEXPAND, 5);

    auto* hSizer = new wxBoxSizer(wxHORIZONTAL);
    messageInput = new wxTextCtrl(this, wxID_ANY);
    sendButton = new wxButton(this, ID_SEND, "Send");
    leaveButton = new wxButton(this, ID_LEAVE, "Leave");
    hSizer->Add(messageInput, 1, wxALL, 2);
    hSizer->Add(sendButton, 0, wxALL, 2);
    hSizer->Add(leaveButton, 0, wxALL, 2);
    sizer->Add(hSizer, 0, wxEXPAND);
    SetSizer(sizer);
}

void ChatPanel::AppendMessage(const wxString& msg) {
    chatBox->AppendText(msg + "\n");
}

void ChatPanel::OnSend(wxCommandEvent&) {
    if (mainWin->wsClient && !messageInput->IsEmpty()) {
        mainWin->wsClient->sendMessage(std::string(messageInput->GetValue().ToUTF8()));
        messageInput->Clear();
    }
}
void ChatPanel::OnLeave(wxCommandEvent&) {
    mainWin->wsClient->leaveRoom();
}
