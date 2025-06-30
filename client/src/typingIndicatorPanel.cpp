#include <client/typingIndicatorPanel.h>

namespace client {

    enum {
        ID_ANIMATION_TIMER = wxID_HIGHEST + 200
    };

    TypingIndicatorPanel::TypingIndicatorPanel(wxWindow* parent)
        : wxPanel(parent, wxID_ANY), m_animationTimer(this, ID_ANIMATION_TIMER)
    {
        SetBackgroundColour(parent->GetBackgroundColour());
        auto* sizer = new wxBoxSizer(wxHORIZONTAL);
        m_label = new wxStaticText(this, wxID_ANY, "");
        m_label->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
        sizer->Add(m_label, 1, wxEXPAND | wxLEFT, FromDIP(5));
        SetSizer(sizer);

        Bind(wxEVT_TIMER, &TypingIndicatorPanel::OnAnimationTimer, this, ID_ANIMATION_TIMER);
    }

    void TypingIndicatorPanel::AddTypingUser(const wxString& username) {
        auto it = std::find(m_typingUsers.begin(), m_typingUsers.end(), username);
        if (it == m_typingUsers.end()) {
            m_typingUsers.push_back(username);
        }

        UpdateLabel();

        if (!m_animationTimer.IsRunning()) {
            m_animationTimer.Start(600);
        }
    }

    void TypingIndicatorPanel::RemoveTypingUser(const wxString& username) {
        m_typingUsers.erase(std::remove(m_typingUsers.begin(), m_typingUsers.end(), username), m_typingUsers.end());

        if (m_typingUsers.empty()) {
            m_animationTimer.Stop();
        }
        UpdateLabel();
    }

    void TypingIndicatorPanel::Clear() {
        m_typingUsers.clear();
        m_animationTimer.Stop();
        UpdateLabel();
    }

    void TypingIndicatorPanel::OnAnimationTimer(wxTimerEvent&) {
        m_dotCount = (m_dotCount % 3) + 1;
        UpdateLabel();
    }

    void TypingIndicatorPanel::UpdateLabel() {
        if (m_typingUsers.empty()) {
            m_label->SetLabel("");
            return;
        }
        wxString text;
        if (m_typingUsers.size() == 1) {
            text = m_typingUsers[0] + " is typing";
        }
        else if (m_typingUsers.size() == 2) {
            text = m_typingUsers[0] + " and " + m_typingUsers[1] + " are typing";
        }
        else {
            text = "Several users are typing";
        }

        text += wxString('.', m_dotCount);
        m_label->SetLabel(text);
    }

} // namespace client
