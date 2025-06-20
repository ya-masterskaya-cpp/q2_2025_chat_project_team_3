#pragma once

#include <wx/wx.h>
#include <wx/filename.h>

class MainWidget;

class InitialPanel : public wxPanel {
public:
    InitialPanel(MainWidget* parent);

private:
    // --- Event Handlers ---
    void OnConnect(wxCommandEvent& event);
    void OnAdd(wxCommandEvent& event);
    void OnDelete(wxCommandEvent& event);
    void OnListSelect(wxCommandEvent& event);

    // --- Helper function to manage button states ---
    void UpdateButtonsState();

    // --- Server List Read/Write ---
    void GetServerList();
    void ServerListFileUpdate();
    wxFileName GetFilePath();

    MainWidget* m_parent = nullptr;

    // --- Controls ---
    wxListBox* m_listBox;
    wxButton* m_connectButton;
    wxButton* m_addButton;
    wxButton* m_deleteButton;
};
