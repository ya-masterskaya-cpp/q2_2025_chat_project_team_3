#pragma once

#include <wx/wx.h>

namespace client {

class MainWidget;

struct Room : public wxClientData {
    int32_t room_id;
    wxString room_name;

    Room(int32_t id, const wxString& name)
    : room_id(id), room_name(name) {}
};

class RoomsPanel : public wxPanel {
public:
    RoomsPanel(wxWindow* parent);
    void UpdateRoomList(const std::vector<Room*>& rooms);
    void AddRoom(Room* room);
    void RemoveRoom(int32_t room_id);
    void RenameRoom(int32_t room_id, const wxString& name);
    std::optional<Room> GetSelectedRoom();

    wxListBox* roomList;
    wxButton* createButton;
    wxButton* logoutButton;
private:
    void OnJoin(wxCommandEvent&);
    void OnCreate(wxCommandEvent&);
    void OnLogout(wxCommandEvent&);
    MainWidget* mainWin;
    wxDECLARE_EVENT_TABLE();
};

} // namespace client
