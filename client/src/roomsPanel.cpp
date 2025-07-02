#include <client/roomsPanel.h>
#include <common/utils/limits.h>
#include <client/textUtil.h>

namespace client {

wxDEFINE_EVENT(wxEVT_ROOM_JOIN_REQUESTED, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_ROOM_CREATE_REQUESTED, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_LOGOUT_REQUESTED, wxCommandEvent);

enum { ID_JOIN = wxID_HIGHEST+20, ID_CREATE, ID_LOGOUT };

wxBEGIN_EVENT_TABLE(RoomsPanel, wxPanel)
    EVT_LISTBOX_DCLICK(wxID_ANY, RoomsPanel::OnJoin)
    EVT_LISTBOX(wxID_ANY, RoomsPanel::OnJoin)
    EVT_BUTTON(ID_CREATE, RoomsPanel::OnCreate)
    EVT_BUTTON(ID_LOGOUT, RoomsPanel::OnLogout)
wxEND_EVENT_TABLE()

RoomsPanel::RoomsPanel(wxWindow* parent) : wxPanel(parent) {

    auto* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(new wxStaticText(this, wxID_ANY, "Rooms:"), 0, wxALL, 5);

    m_roomList = new wxListBox(this, wxID_ANY);
    sizer->Add(m_roomList, 1, wxALL | wxEXPAND, 5);

    // Create and logout sizer
    auto* btnSizer = new wxBoxSizer(wxVERTICAL);
    m_createButton = new wxButton(this, ID_CREATE, "Create Room");
    m_logoutButton = new wxButton(this, ID_LOGOUT, "Logout");
    btnSizer->Add(m_createButton, 1, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);
    btnSizer->Add(m_logoutButton, 1, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);

    sizer->Add(btnSizer, 0, wxEXPAND);

    SetSizer(sizer);
}

void RoomsPanel::UpdateRoomList(const std::vector<Room*>& rooms) {
    m_roomList->Clear();
    for (auto* room : rooms){
        m_roomList->Append(room->room_name, room);
    }
}

void RoomsPanel::AddRoom(Room* room) {
    m_roomList->Append(room->room_name, room);
}

void RoomsPanel::RemoveRoom(int32_t room_id) {
    for (unsigned int i = 0; i < m_roomList->GetCount(); ++i) {
        Room* roomData = dynamic_cast<Room*>(m_roomList->GetClientObject(i));
        if (roomData && roomData->room_id == room_id) {
            m_roomList->Delete(i);
            break;
        }
    }
}

void RoomsPanel::RenameRoom(int32_t room_id, const wxString &name) {
    for (unsigned int i = 0; i < m_roomList->GetCount(); ++i) {
        Room* roomData = dynamic_cast<Room*>(m_roomList->GetClientObject(i));
        if (roomData && roomData->room_id == room_id) {
            roomData->room_name = name;
            m_roomList->SetString(i, name);
            break;
        }
    }
}

std::optional<Room> RoomsPanel::GetSelectedRoom() {
    int sel = m_roomList->GetSelection();
    if (sel != wxNOT_FOUND) {
        Room* roomData = dynamic_cast<Room*>(m_roomList->GetClientObject(sel));
        if (roomData){
            return *roomData;
        }
    }
    return std::nullopt;
}

std::optional<Room> RoomsPanel::FindRoomInListById(int32_t room_id) {
    for (unsigned int i = 0; i < m_roomList->GetCount(); ++i) {
        Room* roomData = dynamic_cast<Room*>(m_roomList->GetClientObject(i));
        if (roomData && roomData->room_id == room_id) {
            return *roomData;
        }
    }
    return std::nullopt;
}

void RoomsPanel::OnJoin(wxCommandEvent&) {
    auto selectedRoomOpt = GetSelectedRoom();
    if (selectedRoomOpt) {
        wxCommandEvent event(wxEVT_ROOM_JOIN_REQUESTED, GetId());
        event.SetInt(selectedRoomOpt->room_id);
        ProcessWindowEvent(event);
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

        wxCommandEvent event(wxEVT_ROOM_CREATE_REQUESTED, GetId());
        event.SetString(roomName);
        ProcessWindowEvent(event);
    }
}

void RoomsPanel::OnLogout(wxCommandEvent &) {
    wxCommandEvent event(wxEVT_LOGOUT_REQUESTED, GetId());
    ProcessWindowEvent(event);
}

} // namespace client
