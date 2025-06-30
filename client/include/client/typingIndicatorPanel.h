#pragma once

#include <wx/wx.h>
#include <vector>

namespace client {

    class TypingIndicatorPanel : public wxPanel {
    public:
        TypingIndicatorPanel(wxWindow* parent);

        void AddTypingUser(const wxString& username);
        void RemoveTypingUser(const wxString& username);
        void Clear();

    private:
        void OnAnimationTimer(wxTimerEvent& event);
        void UpdateLabel();

        wxStaticText* m_label;
        wxTimer m_animationTimer;
        std::vector<wxString> m_typingUsers;
        int m_dotCount = 1;
    };

} // namespace client
