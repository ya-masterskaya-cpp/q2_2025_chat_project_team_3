#include <client/initialPanel.h>
#include <client/mainWidget.h>
#include <client/wsClient.h>
#include <client/app.h>
#include <client/appConfig.h>
#include <drogon/drogon.h>

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
        std::string serverAddress = std::string(newItem.utf8_str());
        if (auto error = ValidateUrl(serverAddress)) {
            wxMessageBox(
                wxString(*error),
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

std::optional<std::string> InitialPanel::ValidateUrl(std::string_view url) {
    auto trim = [](std::string_view s) -> std::string_view {
        while (!s.empty() && std::isspace(s.front())) s.remove_prefix(1);
        while (!s.empty() && std::isspace(s.back())) s.remove_suffix(1);
        return s;
    };
    url = trim(url);

    if (url.empty()) {
        return "URL cannot be empty";
    }

    // 1. Validate the scheme (must be ws:// or wss://)
    constexpr std::string_view ws_scheme = "ws://";
    constexpr std::string_view wss_scheme = "wss://";

    if (url.starts_with(ws_scheme)) {
        url.remove_prefix(ws_scheme.size());
    } else if (url.starts_with(wss_scheme)) {
        url.remove_prefix(wss_scheme.size());
    } else {
        return "URL must start with 'ws://' or 'wss://'";
    }

    // 2. Separate host:port from the path. The path is optional.
    std::string_view host_port;
    auto path_pos = url.find('/');
    if (path_pos != std::string_view::npos) {
        host_port = url.substr(0, path_pos);
    } else {
        host_port = url;
    }

    // 3. Validate the host and port part
    if (host_port.empty()) {
        return "Host and port are missing";
    }

    auto colon_pos = host_port.rfind(':');
    if (colon_pos == std::string_view::npos) {
        return "You must specify a port (e.g., ws://example.com:8840)";
    }

    std::string host(host_port.substr(0, colon_pos));
    std::string port_str(host_port.substr(colon_pos + 1));

    if (host.empty()) {
        return "Host name cannot be empty";
    }

    int port = 0;
    try {
        port = std::stoi(port_str);
    } catch (...) {
        return "Port must be a number";
    }

    if (port <= 0 || port > 65535) {
        return "Port must be in the range 1–65535";
    }

    // 4. Validate the host using trantor's name resolution
    try {
        trantor::InetAddress addr(host, port, false);
    } catch (const std::exception& e) {
        return std::string("Invalid host: ") + e.what();
    }

    // 5. Path validation is no longer needed as its absence is a valid case.

    return std::nullopt; // All checks passed, no error
}
