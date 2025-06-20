#pragma once

#include <wx/wx.h>

class MainWidget;

struct Room : public wxClientData {
    int32_t room_id;
    wxString room_name;

    Room(int32_t id, const wxString& name)
    : room_id(id), room_name(name) {}
};

class RoomsPanel : public wxPanel {
public:
    RoomsPanel(MainWidget* parent);
    void UpdateRoomList(const std::vector<Room>& rooms);
    void AddRoom(const Room& room);
    void RemoveRoom(uint32_t room_id);

    wxListBox* roomList;
    wxButton* joinButton;
    wxButton* createButton;
    wxButton* logoutButton;
private:
    void OnJoin(wxCommandEvent&);
    void OnCreate(wxCommandEvent&);
    void OnLogout(wxCommandEvent&);
    MainWidget* mainWin;
    wxDECLARE_EVENT_TABLE();
};
