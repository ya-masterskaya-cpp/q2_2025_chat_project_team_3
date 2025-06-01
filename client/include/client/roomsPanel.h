#pragma once
#include <wx/wx.h>
#include <vector>
#include <string>
class MainWidget;

class RoomsPanel : public wxPanel {
public:
    RoomsPanel(MainWidget* parent);
    void UpdateRoomList(const std::vector<std::string>& rooms);

    wxListBox* roomList;
    wxButton* joinButton;
    wxButton* createButton;
private:
    void OnJoin(wxCommandEvent&);
    void OnCreate(wxCommandEvent&);
    MainWidget* mainWin;
    wxDECLARE_EVENT_TABLE();
};
