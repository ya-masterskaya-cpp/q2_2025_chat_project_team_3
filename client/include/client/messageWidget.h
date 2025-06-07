#pragma once

#include <wx/wx.h>
#include <wx/popupwin.h>

class MessageWidget : public wxPanel {
public:
    MessageWidget(wxWindow* parent,
                  const wxString& sender,
                  const wxString& message, // Original message (unwrapped)
                  const wxString& timestamp);

    // Method to store the original message (unwrapped).
    void SetOriginalMessage(const wxString& message);

    // Method to update the wxStaticText with wrapped content based on a given width.
    // It takes the wrapWidth determined by the parent.
    void SetWrappedMessage(int wrapWidth);

    // Method to retrieve the font used by the message's wxStaticText.
    wxFont GetMessageTextFont() const;

private:
    wxString m_originalMessage;        // Stores the original, unwrapped message.
    wxStaticText* m_messageStaticText; // Pointer to the actual text control.

    void OnRightClick(wxMouseEvent& event);
    void OnMouseEnter(wxMouseEvent& event);
    void OnMouseLeave(wxMouseEvent& event);
};
