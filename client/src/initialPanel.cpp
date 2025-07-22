#include <client/initialPanel.h>
#include <client/mainWidget.h>
#include <client/wsClient.h>
#include <client/app.h>
#include <client/appConfig.h>
#include <client/textUtil.h>

namespace client {

InitialPanel::InitialPanel(MainWidget* parent)
 : wxPanel(parent), m_parent(parent) {
    // 1) Create controls
    m_listBox      = new wxListBox   (this, wxID_ANY);
    m_connectButton= new wxButton    (this, wxID_ANY, "Connect");
    m_addButton    = new wxButton    (this, wxID_ANY, "Add");
    m_deleteButton = new wxButton    (this, wxID_ANY, "Delete");

    // 2) Build the right‑hand button column with stretch spacers
    auto* buttonSizer = new wxBoxSizer(wxVERTICAL);
    buttonSizer->AddStretchSpacer(1);
    buttonSizer->Add(m_connectButton, 0, wxALL | wxEXPAND, 5);
    buttonSizer->Add(m_addButton,     0, wxALL | wxEXPAND, 5);
    buttonSizer->Add(m_deleteButton,  0, wxALL | wxEXPAND, 5);
    buttonSizer->AddStretchSpacer(1);

    // 3) Build the main sizer: list on left (proportion 1), buttons on right (proportion 0)
    auto* mainSizer = new wxBoxSizer(wxHORIZONTAL);
    mainSizer->Add(m_listBox,     1, wxALL | wxEXPAND, 10);
    mainSizer->Add(buttonSizer,   0, wxALL | wxEXPAND, 10);

    // 4) Install sizer on this panel – parent is responsible for sizing/fitting
    SetSizer(mainSizer);

    // 5) Populate & hook up events

    for(const auto& server : wxGetApp().GetConfig().getServers()) {
        m_listBox->Append(wxString::FromUTF8(server));
    }

    m_connectButton->Bind(wxEVT_BUTTON, &InitialPanel::OnConnect, this);
    m_addButton    ->Bind(wxEVT_BUTTON, &InitialPanel::OnAdd,     this);
    m_deleteButton ->Bind(wxEVT_BUTTON, &InitialPanel::OnDelete,  this);
    m_listBox      ->Bind(wxEVT_LISTBOX,&InitialPanel::OnListSelect, this);

    UpdateButtonsState();
}

void InitialPanel::OnConnect([[maybe_unused]] wxCommandEvent& event) {
    int selection = m_listBox->GetSelection();
    if (selection != wxNOT_FOUND) {
        wxString selectedItem = m_listBox->GetString(selection);
        m_parent->wsClient->start(selectedItem.utf8_string());
    }
}

void InitialPanel::OnAdd([[maybe_unused]] wxCommandEvent& event) {
    wxTextEntryDialog dialog(this, "Enter a new item:", "Add Item");

    // Show the dialog and check if the user clicked OK
    if(dialog.ShowModal() == wxID_OK) {
        wxString newItem = dialog.GetValue();

        if (newItem.IsEmpty()) {
            wxMessageBox(
                "Cannot add an empty hostname/URI.",
                "Input Error",
                wxOK | wxICON_ERROR,
                this
            );
            return;
        }

        auto validated = TextUtil::ValidateUrl(newItem);

        if(!validated) {
            wxMessageBox(
                "Invalid URI.",
                "Input Error",
                wxOK | wxICON_ERROR,
                this
            );
            return;
        }

        if (wxGetApp().GetConfig().addServer(*validated)) {
            m_listBox->Append(wxString::FromUTF8(*validated));
        } else {
            wxMessageBox(
                "This URI already exists.",
                "Warning",
                wxOK | wxICON_WARNING,
                this
            );
        }
    }
}

void InitialPanel::OnDelete([[maybe_unused]] wxCommandEvent& event) {
    int selection = m_listBox->GetSelection();
    
    // Check if an item is actually selected
    if(selection != wxNOT_FOUND) {
        wxGetApp().GetConfig().removeServer(std::string(m_listBox->GetStringSelection().utf8_str()));
        m_listBox->Delete(selection);

        // Explicitly clear the selection to force the control to reset its internal state.
        // This ensures the next click will be registered as a new selection event.
        m_listBox->SetSelection(wxNOT_FOUND);

        // After deleting, no item is selected, so update the button states
        UpdateButtonsState();
    }
}

void InitialPanel::OnListSelect([[maybe_unused]] wxCommandEvent& event) {
    // Whenever the selection changes, update the button states
    UpdateButtonsState();
}

void InitialPanel::UpdateButtonsState() {
    //defer to avoid weird race conditions
    CallAfter([this]{
        // Check if any item is selected in the list box
        bool isItemSelected = (m_listBox->GetSelection() != wxNOT_FOUND);

        // Enable or disable buttons based on the selection
        m_connectButton->Enable(isItemSelected);
        m_deleteButton->Enable(isItemSelected);
    });
}

} // namespace client
