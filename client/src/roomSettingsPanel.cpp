#include "client/roomSettingsPanel.h"

enum {
    ID_RENAME = wxID_HIGHEST + 100,
    ID_DELETE,
    ID_BACK
};

wxDEFINE_EVENT(wxEVT_ROOM_RENAME, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_ROOM_DELETE, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_ROOM_BACK, wxCommandEvent);

wxBEGIN_EVENT_TABLE(RoomSettingsPanel, wxPanel)
    EVT_BUTTON(ID_RENAME, RoomSettingsPanel::OnRename)
    EVT_BUTTON(ID_DELETE, RoomSettingsPanel::OnDelete)
    EVT_BUTTON(ID_BACK, RoomSettingsPanel::OnBack)
wxEND_EVENT_TABLE()

RoomSettingsPanel::RoomSettingsPanel(wxWindow* parent, const wxString& roomName, int32_t roomId)
    : wxPanel(parent, wxID_ANY), m_roomId(roomId) {
    
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
    
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    
    // Room name edit field
    m_roomNameCtrl = new wxTextCtrl(this, wxID_ANY, roomName, 
                                   wxDefaultPosition, wxDefaultSize);
    sizer->Add(m_roomNameCtrl, 0, wxEXPAND | wxALL, FromDIP(5));
    
    // Buttons
    auto* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    
    m_renameButton = new wxButton(this, ID_RENAME, "Rename");
    buttonSizer->Add(m_renameButton, 0, wxALL, FromDIP(5));
    
    m_deleteButton = new wxButton(this, ID_DELETE, "Delete");
    buttonSizer->Add(m_deleteButton, 0, wxALL, FromDIP(5));
    
    m_backButton = new wxButton(this, ID_BACK, "Back");
    buttonSizer->Add(m_backButton, 0, wxALL, FromDIP(5));
    
    sizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxTOP | wxBOTTOM, FromDIP(10));
    
    SetSizer(sizer);
}

void RoomSettingsPanel::OnRename(wxCommandEvent& event) {
    wxCommandEvent evt(wxEVT_ROOM_RENAME);
    evt.SetEventObject(this);
    evt.SetString(m_roomNameCtrl->GetValue());
    evt.SetInt(m_roomId);
    
    GetParent()->GetEventHandler()->ProcessEvent(evt);
}

void RoomSettingsPanel::OnDelete(wxCommandEvent& event) {
    wxCommandEvent evt(wxEVT_ROOM_DELETE);
    evt.SetEventObject(this);
    evt.SetInt(m_roomId);
    
    GetParent()->GetEventHandler()->ProcessEvent(evt);
}

void RoomSettingsPanel::OnBack(wxCommandEvent& event) {
    wxCommandEvent evt(wxEVT_ROOM_BACK);
    evt.SetEventObject(this);
    
    GetParent()->GetEventHandler()->ProcessEvent(evt);
}
