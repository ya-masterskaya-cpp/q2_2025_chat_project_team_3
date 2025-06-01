#include <client/roomsPanel.h>
#include <client/mainWidget.h>
#include <client/wsClient.h>

enum { ID_JOIN = wxID_HIGHEST+20, ID_CREATE };

wxBEGIN_EVENT_TABLE(RoomsPanel, wxPanel)
    EVT_BUTTON(ID_JOIN, RoomsPanel::OnJoin)
    EVT_BUTTON(ID_CREATE, RoomsPanel::OnCreate)
wxEND_EVENT_TABLE()

RoomsPanel::RoomsPanel(MainWidget* parent) : wxPanel(parent), mainWin(parent) {
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(new wxStaticText(this, wxID_ANY, "Rooms:"), 0, wxALL, 5);
    roomList = new wxListBox(this, wxID_ANY);
    sizer->Add(roomList, 1, wxALL | wxEXPAND, 5);
    auto* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    joinButton = new wxButton(this, ID_JOIN, "Join");
    createButton = new wxButton(this, ID_CREATE, "Create");
    btnSizer->Add(joinButton, 1, wxALL, 5);
    btnSizer->Add(createButton, 1, wxALL, 5);
    sizer->Add(btnSizer, 0, wxALIGN_CENTER);
    SetSizer(sizer);
}

void RoomsPanel::UpdateRoomList(const std::vector<std::string>& rooms) {
    roomList->Clear();
    for (const auto& r : rooms) roomList->Append(r);
}

void RoomsPanel::OnJoin(wxCommandEvent&) {
    int sel = roomList->GetSelection();
    if(sel != wxNOT_FOUND)
        mainWin->wsClient->joinRoom(roomList->GetString(sel).ToStdString());
}

void RoomsPanel::OnCreate(wxCommandEvent&) {
    wxTextEntryDialog dlg(this, "Room name?", "Create Room");
    if(dlg.ShowModal() == wxID_OK) {
        mainWin->wsClient->createRoom(std::string(dlg.GetValue().ToUTF8()));
    }
}