#pragma once

#include <wx/wx.h>

wxDECLARE_EVENT(wxEVT_ROOM_RENAME, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_ROOM_DELETE, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_ROOM_BACK, wxCommandEvent);

class RoomSettingsPanel : public wxPanel {
public:
    RoomSettingsPanel(wxWindow* parent, const wxString& roomName, int32_t roomId);
    
    wxString GetNewRoomName() const { return m_roomNameCtrl->GetValue(); }
    int32_t GetRoomId() const { return m_roomId; }

private:
    wxTextCtrl* m_roomNameCtrl = nullptr;
    wxButton* m_renameButton = nullptr;
    wxButton* m_deleteButton = nullptr;
    wxButton* m_backButton = nullptr;
    int32_t m_roomId;

    void OnRename(wxCommandEvent& event);
    void OnDelete(wxCommandEvent& event);
    void OnBack(wxCommandEvent& event);

    wxDECLARE_EVENT_TABLE();
};