#include <client/roomSettingsPanel.h>
#include <client/textUtil.h>
#include <common/utils/limits.h>

enum {
    ID_RENAME = wxID_HIGHEST + 100,
    ID_DELETE,
    ID_CLOSE
};

wxDEFINE_EVENT(wxEVT_ROOM_RENAME, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_ROOM_DELETE, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_ROOM_CLOSE, wxCommandEvent);

wxBEGIN_EVENT_TABLE(RoomSettingsPanel, wxPanel)
    EVT_BUTTON(ID_RENAME, RoomSettingsPanel::OnRename)
    EVT_BUTTON(ID_DELETE, RoomSettingsPanel::OnDelete)
    EVT_BUTTON(ID_CLOSE, RoomSettingsPanel::OnClose)
wxEND_EVENT_TABLE()

RoomSettingsPanel::RoomSettingsPanel(wxWindow* parent, const wxString& roomName, int32_t roomId)
    : wxPanel(parent, wxID_ANY), m_roomId(roomId) {
    
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
    
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    
    // Room name edit field
    m_roomNameCtrl = new wxTextCtrl(this, wxID_ANY, roomName, 
                                   wxDefaultPosition, wxDefaultSize);
    m_roomNameCtrl->Bind(wxEVT_TEXT, &RoomSettingsPanel::OnInputRename, this);
    sizer->Add(m_roomNameCtrl, 0, wxEXPAND | wxALL, FromDIP(5));
    
    // Buttons
    auto* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    
    m_renameButton = new wxButton(this, ID_RENAME, "Rename");
    buttonSizer->Add(m_renameButton, 0, wxALL, FromDIP(5));
    
    m_deleteButton = new wxButton(this, ID_DELETE, "Delete");
    buttonSizer->Add(m_deleteButton, 0, wxALL, FromDIP(5));
    
    m_closeButton = new wxButton(this, ID_CLOSE, "Close");
    buttonSizer->Add(m_closeButton, 0, wxALL, FromDIP(5));
    
    sizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxTOP | wxBOTTOM, FromDIP(10));
    
    SetSizer(sizer);
}

void RoomSettingsPanel::OnRename(wxCommandEvent& event) {
    wxString newName = m_roomNameCtrl->GetValue();
    if (newName.IsEmpty()) {
        wxMessageBox("Room name cannot be empty.", "Error", wxOK | wxICON_ERROR, this);
        return;
    }
    wxString confirmationMessage = wxString::Format("Are you sure you want to rename this room to \"%s\"?", newName);
    int response = wxMessageBox(confirmationMessage, "Confirm Rename", wxYES_NO | wxICON_QUESTION, this);
    if (response == wxYES) {
        wxCommandEvent evt(wxEVT_ROOM_RENAME);
        evt.SetEventObject(this);
        evt.SetString(newName);
        evt.SetInt(m_roomId);
        
        GetParent()->GetEventHandler()->ProcessEvent(evt);
    }
}

void RoomSettingsPanel::OnDelete(wxCommandEvent& event) {
    wxString confirmationMessage = "Are you sure you want to permanently delete this room? This action cannot be undone.";
    int response = wxMessageBox(confirmationMessage, "Confirm Deletion", wxYES_NO | wxICON_WARNING, this);
    if (response == wxYES) {
        wxCommandEvent evt(wxEVT_ROOM_DELETE);
        evt.SetEventObject(this);
        evt.SetInt(m_roomId);
        
        GetParent()->GetEventHandler()->ProcessEvent(evt);
    }
}

void RoomSettingsPanel::OnClose(wxCommandEvent& event) {
    wxCommandEvent evt(wxEVT_ROOM_CLOSE);
    evt.SetEventObject(this);
    
    GetParent()->GetEventHandler()->ProcessEvent(evt);
}

void RoomSettingsPanel::OnInputRename(wxCommandEvent& event) {
    event.Skip();
    TextUtil::LimitTextLength(m_roomNameCtrl, limits::MAX_ROOMNAME_LENGTH);
}
