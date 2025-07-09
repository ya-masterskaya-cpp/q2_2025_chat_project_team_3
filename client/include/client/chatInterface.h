#pragma once

#include <wx/wx.h>

namespace client {

class RoomsPanel;
class ChatPanel;

class ChatInterface : public wxPanel {
public:
    ChatInterface(wxWindow* parent);

    RoomsPanel* m_roomsPanel = nullptr;
    ChatPanel*  m_chatPanel = nullptr;
};

} // namespace client
