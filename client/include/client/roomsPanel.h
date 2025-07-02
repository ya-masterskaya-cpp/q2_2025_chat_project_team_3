#pragma once

#include <wx/wx.h>

namespace client {

struct Room : public wxClientData {
    int32_t room_id;
    wxString room_name;

    Room(int32_t id, const wxString& name)
    : room_id(id), room_name(name) {}
};

wxDECLARE_EVENT(wxEVT_ROOM_JOIN_REQUESTED, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_ROOM_CREATE_REQUESTED, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_LOGOUT_REQUESTED, wxCommandEvent);

class RoomsPanel : public wxPanel {
public:
    RoomsPanel(wxWindow* parent);
    void UpdateRoomList(const std::vector<Room*>& rooms);
    void AddRoom(Room* room);
    void RemoveRoom(int32_t room_id);
    void RenameRoom(int32_t room_id, const wxString& name);
    std::optional<Room> GetSelectedRoom();
    std::optional<Room> FindRoomInListById(int32_t room_id);

private:
    void OnJoin(wxCommandEvent&);
    void OnCreate(wxCommandEvent&);
    void OnLogout(wxCommandEvent&);

    wxListBox* m_roomList;
    wxButton* m_createButton;
    wxButton* m_logoutButton;

    wxDECLARE_EVENT_TABLE();
};

} // namespace client
