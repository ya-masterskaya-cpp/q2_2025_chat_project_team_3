#include <client/serversPanel.h>
#include <client/mainWidget.h>
#include <client/wsClient.h>

namespace client {

ServersPanel::ServersPanel(MainWidget* parent)
 : wxPanel(parent), m_parent(parent) {
    // 1) Create controls
    m_listBox       = new wxListBox(this, wxID_ANY);
    m_connectButton = new wxButton (this, wxID_ANY, "Connect");
    m_backButton    = new wxButton (this, wxID_ANY, "Back");

    // 2) Build the right‑hand button column with stretch spacers
    auto* buttonSizer = new wxBoxSizer(wxVERTICAL);
    buttonSizer->AddStretchSpacer(1);
    buttonSizer->Add(m_connectButton, 0, wxALL | wxEXPAND, 5);
    buttonSizer->Add(m_backButton,    0, wxALL | wxEXPAND, 5);
    buttonSizer->AddStretchSpacer(1);

    // 3) Build the main sizer: list on left (proportion 1), buttons on right (proportion 0)
    auto* mainSizer = new wxBoxSizer(wxHORIZONTAL);
    mainSizer->Add(m_listBox,     1, wxALL | wxEXPAND, 10);
    mainSizer->Add(buttonSizer,   0, wxALL | wxEXPAND, 10);

    // 4) Install sizer on this panel – parent is responsible for sizing/fitting
    SetSizer(mainSizer);

    m_connectButton->Bind(wxEVT_BUTTON, &ServersPanel::OnConnect, this);
    m_backButton   ->Bind(wxEVT_BUTTON, &ServersPanel::OnBack, this);
    m_listBox      ->Bind(wxEVT_LISTBOX,&ServersPanel::OnListSelect, this);

    UpdateButtonsState();
}

void ServersPanel::OnConnect([[maybe_unused]] wxCommandEvent& event) {
    int selection = m_listBox->GetSelection();
    if (selection != wxNOT_FOUND) {
        wxString selectedItem = m_listBox->GetString(selection);
        m_parent->wsClient->start(selectedItem.utf8_string());
    }
}

void ServersPanel::OnBack([[maybe_unused]] wxCommandEvent& event) {
    m_parent->ShowInitial();
}

void ServersPanel::OnListSelect([[maybe_unused]] wxCommandEvent& event) {
    // Whenever the selection changes, update the button states
    UpdateButtonsState();
}

void ServersPanel::UpdateButtonsState() {
    // Check if any item is selected in the list box
    bool isItemSelected = (m_listBox->GetSelection() != wxNOT_FOUND);

    // Enable or disable buttons based on the selection
    m_connectButton->Enable(isItemSelected);
}

void ServersPanel::SetServers(const std::vector<std::string>& servers) {
    m_listBox->Clear();
    for(const auto& server : servers) {
        m_listBox->Append(wxString(server));
    }
}

} // namespace client
