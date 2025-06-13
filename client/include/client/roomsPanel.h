#pragma once

#include <wx/wx.h>

class MainWidget;

struct Room {
    int32_t room_id;
    std::string room_name;
};

class RoomsPanel : public wxPanel {
public:
    RoomsPanel(MainWidget* parent);
    void UpdateRoomList(const std::vector<Room>& rooms);

    wxListBox* roomList;
    wxButton* joinButton;
    wxButton* createButton;
    wxButton* logoutButton;
private:
    std::unordered_map<int, uint32_t> room_list_index_to_id_;
    void OnJoin(wxCommandEvent&);
    void OnCreate(wxCommandEvent&);
    void OnLogout(wxCommandEvent&);
    MainWidget* mainWin;
    wxDECLARE_EVENT_TABLE();
};
