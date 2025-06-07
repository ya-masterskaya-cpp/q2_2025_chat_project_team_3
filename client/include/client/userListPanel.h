#include <wx/wx.h>
#include <wx/listbox.h>

class UserListPanel : public wxPanel {
public:
    UserListPanel(wxWindow* parent);
    void UpdateUserList(const std::vector<wxString>& users);
    
private:
    wxListBox* userListBox;
    void OnRightClick(wxMouseEvent& event);
};
