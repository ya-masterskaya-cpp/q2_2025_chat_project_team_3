#include <client/roomHeaderPanel.h>
#include <client/roomsPanel.h>

namespace client {

RoomHeaderPanel::RoomHeaderPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY) {

    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
    auto* headerSizer = new wxBoxSizer(wxHORIZONTAL);
    
    m_roomName = new wxStaticText(this, wxID_ANY, "",
                                        wxDefaultPosition, wxDefaultSize,
                                        wxALIGN_LEFT);
    m_roomName->SetFont(m_roomName->GetFont().Bold());
    m_roomName->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));

    headerSizer->Add(m_roomName, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(10));

    SetSizer(headerSizer);
    SetCursor(wxCURSOR_OPEN_HAND);

    Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent& event) {
        event.Skip();
    });

    m_roomName->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent& event) {
        wxPostEvent(this, event);
        event.Skip();
    });

    Bind(wxEVT_ENTER_WINDOW, &RoomHeaderPanel::OnEnterWindow, this);
    m_roomName->Bind(wxEVT_ENTER_WINDOW, &RoomHeaderPanel::OnEnterWindow, this);

    Bind(wxEVT_LEAVE_WINDOW, &RoomHeaderPanel::OnLeaveWindow, this);
    m_roomName->Bind(wxEVT_LEAVE_WINDOW, &RoomHeaderPanel::OnLeaveWindow, this);

    m_roomId = 0;
}
    
void RoomHeaderPanel::SetRoom(const Room& room) {
    m_roomName->SetLabel(room.room_name);
    m_roomId = room.room_id;
    Layout();
}

void RoomHeaderPanel::OnEnterWindow(wxMouseEvent& event) {
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
    Refresh();
    event.Skip();
}

void RoomHeaderPanel::OnLeaveWindow(wxMouseEvent& event) {
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
    Refresh();
    event.Skip();
}

} // namespace client
