#include <client/initialPanel.h>
#include <client/mainWidget.h>
#include <client/wsClient.h>
#include <json/json.h>
#include <fstream>
#include <wx/stdpaths.h>

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
    //m_listBox->Append("ws://localhost:8848/ws");
    //m_listBox->Append("ws://localhost:8849/ws");

    // 6) Get Server List
    GetServerList();

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

    if (m_parent->IsAnotherInstanceRunning()) {
        wxTheApp->CallAfter([this] { m_parent->ShowPopup("Error, another instance is exist!", wxICON_ERROR); });
        return;
    }

    wxTextEntryDialog dialog(this, "Enter a new item:", "Add Item");

    // Show the dialog and check if the user clicked OK
    if(dialog.ShowModal() == wxID_OK) {
        wxString newItem = dialog.GetValue();
        // Don't add empty items
        if(!newItem.IsEmpty()) {
            m_listBox->Append(newItem);
            ServerListFileUpdate();
        }
    }
}

void InitialPanel::OnDelete(wxCommandEvent& event) {

    if (m_parent->IsAnotherInstanceRunning()) {
        wxTheApp->CallAfter([this] { m_parent->ShowPopup("Error, another instance is exist!", wxICON_ERROR); });
        return;
    }

    int selection = m_listBox->GetSelection();
    
    // Check if an item is actually selected
    if(selection != wxNOT_FOUND) {
        m_listBox->Delete(selection);
        ServerListFileUpdate();
        
        // After deleting, no item is selected, so update the button states
        UpdateButtonsState();
    }
}

void InitialPanel::OnListSelect(wxCommandEvent& event) {
    // Whenever the selection changes, update the button states
    UpdateButtonsState();
}

void InitialPanel::UpdateButtonsState() {
    // Check if any item is selected in the list box
    bool isItemSelected = (m_listBox->GetSelection() != wxNOT_FOUND);

    // Enable or disable buttons based on the selection
    m_connectButton->Enable(isItemSelected);
    m_deleteButton->Enable(isItemSelected);
}

void InitialPanel::GetServerList() {

    std::ifstream server_list_file(GetFilePath().GetAbsolutePath());

    if (!server_list_file.is_open()) {
        //wxTheApp->CallAfter([this] { m_parent->ShowPopup("Failed to open server list!", wxICON_ERROR); });
        ServerListFileUpdate();
        return;
    }

    Json::Value server_list;
    Json::CharReaderBuilder reader_builder;
    std::string parse_errors;

    if (!Json::parseFromStream(reader_builder, server_list_file, &server_list, &parse_errors)) {
        wxTheApp->CallAfter([this] { m_parent->ShowPopup("Failed to parse server list!", wxICON_ERROR); });
        return;
    }

    for (const auto& server : server_list["servers"]) {
        m_listBox->Append(server.asString());
    }
}

void InitialPanel::ServerListFileUpdate() {
    
    std::ofstream server_list_file(GetFilePath().GetAbsolutePath());

    if (!server_list_file.is_open()) {
        wxTheApp->CallAfter([this] { m_parent->ShowPopup("Failed to open server list!", wxICON_ERROR); });
        return;
    }

    Json::Value server_list;
    server_list["servers"] = Json::arrayValue;

    for (const auto& server : m_listBox->GetStrings()) {
        server_list["servers"].append(server.ToStdString());
    }

    Json::StreamWriterBuilder writer;
    std::unique_ptr<Json::StreamWriter> json_writer(writer.newStreamWriter());

    json_writer->write(server_list, &server_list_file);
    server_list_file.close();
}

wxFileName InitialPanel::GetFilePath() {

    wxStandardPaths& paths = wxStandardPaths::Get();
    wxString userConfigDir = paths.GetUserConfigDir();

    //CHECK PROJECT NAME ---
    #if defined(__APPLE__)
    // IMPORTANT: This must match the name set by SetAppName() and your root project() name.
    userConfigDir = userConfigDir + "/Slightly_Pretty_Chat";
    #endif

    return {userConfigDir, "server_list.json"};
}