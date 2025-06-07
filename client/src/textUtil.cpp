#include <client/textUtil.h>
#include <wx/dcclient.h> // For wxClientDC

namespace TextUtil {
    wxString WrapText(wxWindow* targetWindow, const wxString& text, int wrapWidth, const wxFont& font) {
        if (wrapWidth <= 0 || text.IsEmpty() || !targetWindow) {
            return text; // Basic validation: return original text if width is invalid or text is empty.
        }

        wxString wrappedText;
        wxString currentLine;

        wxClientDC dc(targetWindow);
        dc.SetFont(font);

        // Iterate through the input text character by character (Unicode-aware iteration).
        for (wxUniChar ch : text) {
            if (ch == '\n') { // If an explicit newline character is encountered
                wrappedText += currentLine.Trim(true) + "\n"; // Add the current line (trimmed) and a newline
                currentLine.Clear(); // Start a new line
                continue;
            }

            // Measure the width if the current character is added to the current line.
            wxCoord testWidth;
            dc.GetTextExtent(currentLine + ch, &testWidth, nullptr);

            // If adding this character makes the line exceed the wrapWidth:
            if (testWidth > wrapWidth) {
                // Scenario: The current line would become too long.
                // We add the current accumulated line to wrappedText,
                // then this character starts a new line.
                wrappedText += currentLine.Trim(true) + "\n"; // Add previous line (trimmed) and newline
                currentLine.Clear(); // Clear current line
                currentLine += ch;   // Add the character to the new line
            } else {
                // Scenario: The character fits on the current line.
                currentLine += ch; // Append the character to the current line.
            }
        }

        // After the loop, add any remaining text from the last line.
        if (!currentLine.IsEmpty()) {
            wrappedText += currentLine.Trim(true); // Add the final line (trimmed)
        }
        return wrappedText;
    }
} // namespace TextUtil
