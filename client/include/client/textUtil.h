#pragma once

#include <wx/string.h>
#include <wx/font.h>
#include <wx/window.h> // Required for wxClientDC in the implementation
#include <wx/textctrl.h>

namespace TextUtil {
    // Function to manually wrap text based on a given width and font.
    // 'targetWindow' is needed for the wxClientDC to measure text.
    // This minimalist version iterates character by character and will break words mid-way if needed.
    wxString WrapText(wxWindow* targetWindow, const wxString& text, int wrapWidth, const wxFont& font);

    void LimitTextLength(wxTextCtrl* textEntry, size_t maxLength);
} // namespace TextUtil
