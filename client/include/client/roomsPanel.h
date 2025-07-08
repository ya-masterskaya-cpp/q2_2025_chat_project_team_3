#pragma once

#include <wx/wx.h>
#include <wx/notebook.h>

namespace client {

class MainWidget;

struct Room : public wxClientData {
    int32_t room_id;
    wxString room_name;
    bool is_member;

    Room(int32_t id, const wxString& name, bool member)
        : room_id(id), room_name(name), is_member(member) {}
};

class RoomsPanel : public wxPanel {
public:
    RoomsPanel(wxWindow* parent);
    void UpdateRoomList(const std::vector<Room*>& rooms);
    void AddRoom(Room* room);
    void RemoveRoom(int32_t room_id);
    void RenameRoom(int32_t room_id, const wxString& name);
    std::optional<Room> GetSelectedRoom();
    void OnJoinRoom();
    void OnBecameMember();

private:
    void OnMyRoomSelected(wxCommandEvent&);
    void OnPublicRoomSelected(wxCommandEvent& event);
    void OnJoin(wxCommandEvent&);
    void OnCreate(wxCommandEvent&);
    void OnLogout(wxCommandEvent&);
    void OnAccount(wxCommandEvent&);

    MainWidget* mainWin;
    wxNotebook* m_notebook;
    wxListBox* m_myRoomsList;
    wxListBox* m_publicRoomsList;
    wxButton* m_joinButton;
    wxButton* m_createButton;
    wxButton* m_logoutButton;
    wxButton* m_accountButton;
    wxDECLARE_EVENT_TABLE();
};

} // namespace client
