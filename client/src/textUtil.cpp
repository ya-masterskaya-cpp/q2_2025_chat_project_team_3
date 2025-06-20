#include <client/textUtil.h>
#include <wx/string.h>
#include <wx/dcclient.h>
#include <wx/dcbuffer.h>
#include <wx/settings.h>
#include <wx/font.h>
#include <wx/utils.h>

namespace TextUtil {
    wxString WrapText(wxWindow* targetWindow, const wxString& text, int wrapWidth, const wxFont& font) {
        // Basic validation to prevent crashes.
        if (wrapWidth <= 0 || text.IsEmpty() || !targetWindow) {
            return text;
        }

        wxString wrappedText;
        wxString currentLine;
        int currentLineWidth = 0; // Tracks accumulated width of characters on the current line.

        // Use wxMemoryDC for text measurement. It works independently of a visible window.
        wxMemoryDC dc;
        dc.SetFont(font);

        // Iterate through the input text character by character. wxUniChar correctly handles Unicode code points.
        for (wxUniChar ch : text) {
            if (ch == '\n') { // Handle explicit newline characters.
                wrappedText += currentLine.Trim(true) + "\n";
                currentLine.Clear();
                currentLineWidth = 0;
                continue;
            }

            // Measure the width of the *single* character.
            wxCoord charWidth;
            dc.GetTextExtent(wxString(ch), &charWidth, nullptr);

            // Check if adding this character makes the current line exceed the wrapWidth.
            // We only break if the current line is not empty (to avoid an infinite loop if a single char > wrapWidth).
            if (currentLineWidth + charWidth > wrapWidth && !currentLine.IsEmpty()) {
                wrappedText += currentLine.Trim(true) + "\n"; // Add the completed line.
                currentLine.Clear();                          // Start a new line.
                currentLineWidth = 0;                         // Reset line width.
            }

            // Append the character to the current line and add its width.
            currentLine += ch;
            currentLineWidth += charWidth;
        }

        // After the loop, append any remaining text on the last line.
        if (!currentLine.IsEmpty()) {
            wrappedText += currentLine.Trim(true);
        }

        return wrappedText;
    }

    void LimitTextLength(wxTextCtrl* textEntry, size_t maxLength) {
        wxString val = textEntry->GetValue();
        if (val.length() > maxLength) {
            size_t pos = static_cast<size_t>(textEntry->GetInsertionPoint());
            wxString cated_str = val.Left(maxLength);
            textEntry->CallAfter([=]() {
                textEntry->SetValue(cated_str);
                textEntry->SetInsertionPoint(std::min(pos, maxLength));
                wxBell();
            });
        }
    }
} // namespace TextUtil
