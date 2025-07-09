#pragma once

#include <wx/wx.h>

namespace client {

class MainWidget;

class ServersPanel : public wxPanel {
public:
    ServersPanel(MainWidget* parent);

    void SetServers(const std::vector<std::string>& servers);
private:
    // --- Event Handlers ---
    void OnConnect(wxCommandEvent& event);
    void OnBack(wxCommandEvent& event);
    void OnListSelect(wxCommandEvent& event);

    // --- Helper function to manage button states ---
    void UpdateButtonsState();

    MainWidget* m_parent = nullptr;

    // --- Controls ---
    wxListBox* m_listBox;
    wxButton* m_connectButton;
    wxButton* m_backButton;
};

} // namespace client
