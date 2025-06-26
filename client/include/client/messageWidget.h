#pragma once

#include <wx/wx.h>
#include <wx/popupwin.h>

namespace client {

struct Message;
class CachedColorText;

class MessageWidget : public wxPanel {
public:
    MessageWidget(wxWindow* parent,
                  const Message&,
                  int lastKnownWrapWidth);

    // Method to update the wxStaticText with wrapped content based on a given width.
    // It takes the wrapWidth determined by the parent.
    void SetWrappedMessage(int wrapWidth);

    // Method to retrieve the font used by the message's wxStaticText.
    wxFont GetMessageTextFont() const;

    int64_t GetTimestampValue() const { return m_timestamp_val; }
    bool m_hasBeenPositionedCorrectly = false;

    void Update(wxWindow* parent, const Message& msg, int lastKnownWrapWidth);

private:
    wxString m_originalMessage;        // Stores the original, unwrapped message.
    CachedColorText* m_messageStaticText; // Pointer to the actual text control.
    int64_t m_timestamp_val;           // Stores the raw timestamp for sorting/querying
    wxStaticText* m_timeText;
    CachedColorText* m_userText;

    void OnRightClick(wxMouseEvent& event);
    void OnMouseEnter(wxMouseEvent& event);
    void OnMouseLeave(wxMouseEvent& event);
};

} // namespace client
