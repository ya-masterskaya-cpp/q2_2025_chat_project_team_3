#pragma once

#include <wx/wx.h>

namespace client {

struct Room;

class RoomHeaderPanel : public wxPanel {
public:
    RoomHeaderPanel(wxWindow* parent);

    void SetRoom(const Room& room);
    int32_t GetRoomId() const { return m_roomId; }
    wxString GetLabel() const { return m_roomName->GetLabel(); }
    void SetLabel(wxString label) const {m_roomName->SetLabel(label); }

private:
    wxStaticText* m_roomName = nullptr;
    int32_t m_roomId;

    //void OnClick(wxMouseEvent& event);
    void OnEnterWindow(wxMouseEvent& event);
    void OnLeaveWindow(wxMouseEvent& event);
};

} // namespace client
