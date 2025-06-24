#include <client/initialPanel.h>
#include <client/mainWidget.h>
#include <client/wsClient.h>
#include <client/app.h>
#include <client/appConfig.h>
#include <wx/uri.h>
#include <wx/tokenzr.h>

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

void InitialPanel::OnConnect(wxCommandEvent& event) {
    int selection = m_listBox->GetSelection();
    if (selection != wxNOT_FOUND) {
        wxString selectedItem = m_listBox->GetString(selection);
        m_parent->wsClient->start(selectedItem.utf8_string());
    }
}

void InitialPanel::OnAdd(wxCommandEvent& event) {
    wxTextEntryDialog dialog(this, "Enter a new item:", "Add Item");

    // Show the dialog and check if the user clicked OK
    if(dialog.ShowModal() == wxID_OK) {
        wxString newItem = dialog.GetValue();
        if (auto error = ValidateUrl(newItem)) {
            wxMessageBox(
                *error,
                "Invalid URL format",
                wxOK | wxICON_ERROR,
                this
            );
            return;
        }

        if (wxGetApp().GetConfig().addServer(std::string(newItem.utf8_str()))) {
            m_listBox->Append(newItem);
        } else {
            wxMessageBox(
                "This hostname/URI already exists.",
                "Warning",
                wxOK | wxICON_WARNING,
                this
            );
        }
    }
}

void InitialPanel::OnDelete(wxCommandEvent& event) {
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

void InitialPanel::OnListSelect(wxCommandEvent& event) {
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

std::optional<wxString> InitialPanel::ValidateUrl(wxString url) {
    // 1. Sanitize user input by removing leading/trailing whitespace for a better UX.
    url.Trim(true);
    url.Trim(false);

    if (url.empty()) {
        return "URL cannot be empty";
    }

    wxURI uri(url);
    // 2. Check for the required components of a valid WebSocket URL.
    if (!uri.HasScheme()) {
        return "URL must start with 'ws://' or 'wss://'";
    }
    if (uri.GetScheme() != "ws" && uri.GetScheme() != "wss") {
        return "The scheme must be 'ws' or 'wss'";
    }

    if (!uri.HasServer() || uri.GetServer().empty()) {
        return "The host name cannot be empty";
    }

    if (!uri.HasPort()) {
        return "You must specify a port (e.g., ws://example.com:8840)";
    }

    // 3. Validate that the port is a number within the valid range.
    long port = 0;
    if (!uri.GetPort().ToLong(&port) || port <= 0 || port > 65535) {
        return "The port must be a number in the range 1–65535";
    }

    // 4. Perform a stricter, syntax-only check on the host's format.
    wxURIHostType hostType = uri.GetHostType();
    wxString host = uri.GetServer();

    if (hostType == wxURI_REGNAME) {
        // The host is a registered name (e.g., 'localhost', 'example.com').

        // Rule 1: A valid hostname cannot contain consecutive dots.
        if (host.Contains("..")) {
            return "Invalid hostname format: cannot contain consecutive dots.";
        }

        // Rule 2: A valid hostname cannot start or end with a dot or a hyphen.
        if (host.StartsWith(".") || host.EndsWith(".") || host.StartsWith("-") || host.EndsWith("-")) {
            return "Invalid hostname format: cannot start or end with a dot or a hyphen.";
        }

        // Rule 3: Check each part of the hostname (label).
        wxStringTokenizer tokenizer(host, ".");
        while (tokenizer.HasMoreTokens()) {
            wxString token = tokenizer.GetNextToken();
            // Check for invalid characters in each label.
            for (const auto& c : token) {
                // Valid characters are letters, numbers, and hyphens.
                if (!wxIsalnum(c) && c != '-') {
                    return wxString::Format("Invalid character '%c' in hostname.", c);
                }
            }
        }
    } else {
        // The host format is unrecognized or unsupported (e.g., wxURI_IPVFUTURE).
        return "Unsupported or invalid host format.";
    }

    // All syntax checks passed.
    return std::nullopt;
}
