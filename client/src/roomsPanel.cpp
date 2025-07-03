#include <client/roomsPanel.h>
#include <client/mainWidget.h>
#include <client/wsClient.h>
#include <common/utils/limits.h>
#include <client/textUtil.h>

namespace client {

enum {
    ID_LIST_MY_ROOMS = wxID_HIGHEST + 20,
    ID_LIST_PUBLIC_ROOMS,
    ID_JOIN,
    ID_CREATE,
    ID_LOGOUT
};

wxBEGIN_EVENT_TABLE(RoomsPanel, wxPanel)
    //EVT_BUTTON(ID_JOIN, RoomsPanel::OnJoin)
    EVT_BUTTON(ID_CREATE, RoomsPanel::OnCreate)
    EVT_BUTTON(ID_LOGOUT, RoomsPanel::OnLogout)
    EVT_BUTTON(ID_JOIN, RoomsPanel::OnJoin) // Move impls
    EVT_LISTBOX_DCLICK(ID_LIST_MY_ROOMS, RoomsPanel::OnMyRoomSelected)
    EVT_LISTBOX(ID_LIST_PUBLIC_ROOMS, RoomsPanel::OnPublicRoomSelected)

wxEND_EVENT_TABLE()

RoomsPanel::RoomsPanel(wxWindow* parent) : wxPanel(parent), mainWin(static_cast<MainWidget*>(parent->GetParent())) {
    auto* sizer = new wxBoxSizer(wxVERTICAL);

    m_notebook = new wxNotebook(this, wxID_ANY);

    auto* myRoomsPage = new wxPanel(m_notebook);
    auto* myRoomsSizer = new wxBoxSizer(wxVERTICAL);
    m_myRoomsList = new wxListBox(myRoomsPage, ID_LIST_MY_ROOMS);
    myRoomsSizer->Add(m_myRoomsList, 1, wxEXPAND | wxALL, 5);
    myRoomsPage->SetSizer(myRoomsSizer);
    m_notebook->AddPage(myRoomsPage, "My rooms");

    auto* publicRoomsPage = new wxPanel(m_notebook);
    auto* publicRoomsSizer = new wxBoxSizer(wxVERTICAL);
    m_publicRoomsList = new wxListBox(publicRoomsPage, ID_LIST_PUBLIC_ROOMS);
    publicRoomsSizer->Add(m_publicRoomsList, 1, wxEXPAND | wxALL, 5);

    m_joinButton = new wxButton(publicRoomsPage, ID_JOIN, "Join");
    m_joinButton->Disable();
    publicRoomsSizer->Add(m_joinButton, 0, wxALIGN_CENTER | wxALL, 5);

    publicRoomsPage->SetSizer(publicRoomsSizer);
    m_notebook->AddPage(publicRoomsPage, "Public rooms");

    sizer->Add(m_notebook, 1, wxEXPAND | wxALL, 5);

    auto* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    m_createButton = new wxButton(this, ID_CREATE, "Создать");
    btnSizer->Add(m_createButton, 1, wxALL, 5);
    sizer->Add(btnSizer, 0, wxALIGN_CENTER);

    m_logoutButton = new wxButton(this, ID_LOGOUT, "Выход");
    sizer->Add(m_logoutButton, 0, wxALL | wxALIGN_CENTER, 10);
    SetSizer(sizer);
}

void RoomsPanel::UpdateRoomList(const std::vector<Room*>& rooms) {
    m_myRoomsList->Clear();
    m_publicRoomsList->Clear();
    for (auto* room : rooms){
        if (room->is_member) {
            m_myRoomsList->Append(room->room_name, room);
        }
        else {
            m_publicRoomsList->Append(room->room_name, room);
        }
    }
}

void RoomsPanel::AddRoom(Room* room) {
    if (room->is_member) {
        for (unsigned int i = 0; i < m_publicRoomsList->GetCount(); ++i) {
            Room* publicRoomData = dynamic_cast<Room*>(m_publicRoomsList->GetClientObject(i));
            if (publicRoomData && publicRoomData->room_id == room->room_id) {
                m_publicRoomsList->Delete(i);
                break;
            }
        }
        int newIndex = m_myRoomsList->Append(room->room_name, room);
        m_myRoomsList->SetSelection(newIndex);
        m_notebook->SetSelection(0);
    }
    else {
        m_publicRoomsList->Append(room->room_name, room);
    }
}

void RoomsPanel::RemoveRoom(int32_t room_id) {
    for (unsigned int i = 0; i < m_myRoomsList->GetCount(); ++i) {
        Room* roomData = dynamic_cast<Room*>(m_myRoomsList->GetClientObject(i));
        if (roomData && roomData->room_id == room_id) {
            m_myRoomsList->Delete(i);
            return;
        }
    }
    for (unsigned int i = 0; i < m_publicRoomsList->GetCount(); ++i) {
        Room* roomData = dynamic_cast<Room*>(m_publicRoomsList->GetClientObject(i));
        if (roomData && roomData->room_id == room_id) {
            m_publicRoomsList->Delete(i);
            return;
        }
    }
}

void RoomsPanel::RenameRoom(int32_t room_id, const wxString& name) {
    for (unsigned int i = 0; i < m_myRoomsList->GetCount(); ++i) {
        Room* roomData = dynamic_cast<Room*>(m_myRoomsList->GetClientObject(i));
        if (roomData && roomData->room_id == room_id) {
            roomData->room_name = name;
            m_myRoomsList->SetString(i, name);
            break;
        }
    }
    for (unsigned int i = 0; i < m_publicRoomsList->GetCount(); ++i) {
        Room* roomData = dynamic_cast<Room*>(m_publicRoomsList->GetClientObject(i));
        if (roomData && roomData->room_id == room_id) {
            roomData->room_name = name;
            m_publicRoomsList->SetString(i, name);
            break;
        }
    }
}

std::optional<Room> RoomsPanel::GetSelectedRoom() {
    wxListBox* activeList = nullptr;
    if (m_notebook->GetSelection() == 0) {
        activeList = m_myRoomsList;
    }
    else {
        activeList = m_publicRoomsList;
    }

    int sel = activeList->GetSelection();
    if(sel != wxNOT_FOUND) {
        Room* roomData = dynamic_cast<Room*>(activeList->GetClientObject(sel));
        if (roomData){
            return *roomData;
        }
    }
    return std::nullopt;
}

void RoomsPanel::OnMyRoomSelected(wxCommandEvent&) {
    auto selectedRoom = GetSelectedRoom();
    if (selectedRoom.has_value()) {
        mainWin->wsClient->joinRoom(selectedRoom->room_id);
    }
}

void RoomsPanel::OnPublicRoomSelected(wxCommandEvent& event) {
    m_joinButton->Enable(m_publicRoomsList->GetSelection() != wxNOT_FOUND);
    event.Skip();
}

void RoomsPanel::OnJoin(wxCommandEvent&) {
    auto selectedRoom = GetSelectedRoom();
    if (selectedRoom.has_value()) {
        mainWin->wsClient->joinRoom(selectedRoom->room_id);
    }
}

void RoomsPanel::OnCreate(wxCommandEvent&) {
    wxTextEntryDialog dlg(this, "Room name?", "Create Room");
    dlg.SetMaxLength(common::limits::MAX_ROOMNAME_LENGTH);
    if(dlg.ShowModal() == wxID_OK) {
        wxString roomName = dlg.GetValue();
        if (roomName.IsEmpty()) {
            wxMessageBox("Room name cannot be empty!", "Error", wxOK | wxICON_ERROR);
            return;
        }

        roomName = TextUtil::SanitizeInput(roomName);

        if (roomName.IsEmpty()) {
            wxMessageBox("Room name must consist of at least 1 non special character",
                "Room creation error",
                wxOK | wxICON_ERROR, this
            );
            return;
        }

        mainWin->wsClient->createRoom(roomName.utf8_string());
    }
}

void RoomsPanel::OnLogout(wxCommandEvent &) {
    mainWin->wsClient->logout();
}

} // namespace client
