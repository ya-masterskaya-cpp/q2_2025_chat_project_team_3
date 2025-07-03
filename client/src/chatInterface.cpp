#include <client/chatInterface.h>
#include <client/roomsPanel.h>
#include <client/chatPanel.h>

namespace client {

ChatInterface::ChatInterface(wxWindow* parent): wxPanel(parent, wxID_ANY) {
    m_roomsPanel = new RoomsPanel(this);
    m_chatPanel = new ChatPanel(this);

    // Create a horizontal box sizer
    wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);

    // Add Element A:
    // Proportion = 0: Don't grow horizontally.
    // Flag = wxEXPAND: Make the control actually expand to fill the allocated space.
    // Border = 5: Add a 5-pixel border on all sides.
    hbox->Add(m_roomsPanel, 0, wxEXPAND | wxALL, 5);

    // Add Element B:
    // Proportion = 1: Give it all the extra horizontal space.
    // Flag = wxEXPAND: Make the control actually expand to fill the allocated space.
    // Border = 5: Add a 5-pixel border on all sides.
    hbox->Add(m_chatPanel, 1, wxEXPAND | wxALL, 5);

    // --- Finalize Layout ---

    // Set the panel's sizer. The panel will now be managed by the sizer.
    SetSizer(hbox);
}

} // namespace client