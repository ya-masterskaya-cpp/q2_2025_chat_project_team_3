#include <client/userListPanel.h>

UserListPanel::UserListPanel(wxWindow* parent) 
    : wxPanel(parent, wxID_ANY) {
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
    
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    
    auto* header = new wxStaticText(this, wxID_ANY, "Users in room:");
    header->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
    sizer->Add(header, 0, wxALL, FromDIP(5));
    
    userListBox = new wxListBox(this, wxID_ANY, 
                              wxDefaultPosition, 
                              wxSize(FromDIP(150), -1),
                              0, nullptr, 
                              wxLB_SINGLE | wxLB_NEEDED_SB);
    userListBox->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
    userListBox->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
    
    sizer->Add(userListBox, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(5));
    SetSizer(sizer);

    userListBox->Bind(wxEVT_RIGHT_DOWN, &UserListPanel::OnRightClick, this);
}

void UserListPanel::UpdateUserList(const std::vector<wxString>& users) {
    userListBox->Clear();
    for (const auto& user : users) {
       userListBox->Append(user);
    }
}

void UserListPanel::OnRightClick(wxMouseEvent& event) {
    wxPoint ptClient = event.GetPosition();
    int idx = userListBox->HitTest(ptClient);
    if(idx == wxNOT_FOUND) { 
        event.Skip();
        return;
    }

    userListBox->SetSelection(idx);

    wxMenu menu;
    menu.Append(wxID_ANY, "Private message");
    menu.AppendSeparator();
    menu.Append(wxID_ANY, "Kick user");
    menu.Append(wxID_ANY, "Ban user");

    userListBox->PopupMenu(&menu, ptClient);

    userListBox->DeselectAll();
    userListBox->Refresh();

    //event.Skip();
}
