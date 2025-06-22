#include <client/textUtil.h>
#include <wx/string.h>
#include <wx/dcclient.h>
#include <wx/dcbuffer.h>
#include <wx/settings.h>
#include <wx/font.h>
#include <wx/utils.h>

namespace TextUtil {

    // Helper functions to avoid repeating magic numbers for surrogate checks.
#ifdef __WXMSW__
    /**
     * @brief Checks if a wchar_t is a high surrogate for a UTF-16 pair.
     * High surrogates are in the range U+D800 to U+DBFF.
     */
    inline bool IsHighSurrogate(wxChar ch) {
        return (ch >= 0xD800 && ch <= 0xDBFF);
    }

    /**
     * @brief Checks if a wchar_t is a low surrogate for a UTF-16 pair.
     * Low surrogates are in the range U+DC00 to U+DFFF.
     */
    inline bool IsLowSurrogate(wxChar ch) {
        return (ch >= 0xDC00 && ch <= 0xDFFF);
    }
#else
    // On non-Windows platforms, these concepts are not used in this context.
    inline bool IsHighSurrogate(wxChar ch) { (void)ch; return false; }
    inline bool IsLowSurrogate(wxChar ch) { (void)ch; return false; }
#endif

    /**
     * @brief Gets the number of Unicode code points in a wxString.
     *
     * This function correctly counts surrogate pairs as a single character on Windows.
     * On other platforms, it is equivalent to calling str.length().
     */
    size_t GetUnicodeLength(const wxString& str) {
#ifdef __WXMSW__
        size_t count = 0;
        const size_t len = str.length();
        for(size_t i = 0; i < len; ++i) {
            if(IsHighSurrogate(str[i])) {
                if((i + 1) < len && IsLowSurrogate(str[i + 1])) {
                    // Valid surrogate pair found, skip the second half.
                    i++;
                }
            }
            // Each loop pass represents one full Unicode code point.
            count++;
        }
        return count;
#else
        // On non-Windows platforms, length() is correct.
        return str.length();
#endif
    }

    /**
     * @brief Truncates a wxString to a specified number of Unicode code points.
     *
     * This function is a Unicode-aware replacement for wxString::Left(). On Windows,
     * it correctly handles surrogate pairs to avoid splitting a character.
     * @param str The string to truncate.
     * @param count The maximum number of Unicode code points to keep.
     * @return The truncated string.
     */
    wxString UnicodeLeft(const wxString& str, size_t count) {
#ifdef __WXMSW__
        size_t unicode_pos = 0;
        for(size_t i = 0; i < str.length(); ++i) {
            if(unicode_pos >= count) {
                // We have reached the desired number of code points.
                // Truncate the string at the current wchar_t index 'i'.
                return str.Left(i);
            }

            // Advance past the current code point.
            unicode_pos++;
            if(IsHighSurrogate(str[i]) && (i + 1) < str.length() && IsLowSurrogate(str[i + 1])) {
                // It's a surrogate pair, so skip the second half in the next loop iteration.
                i++;
            }
        }
        // If we complete the loop, the entire string is within the limit.
        return str;
#else
        // On non-Windows platforms, Left() operates on code points correctly.
        return str.Left(count);
#endif
    }

    wxString WrapText(wxWindow* targetWindow, const wxString& text, int wrapWidth, const wxFont& font) {
        // Basic validation to prevent crashes.
        if(wrapWidth <= 0 || text.IsEmpty() || !targetWindow) {
            return text;
        }

        wxString wrappedText;
        wxString currentLine;
        int currentLineWidth = 0; // Tracks accumulated width of characters on the current line.

        // Use wxMemoryDC for text measurement. It works independently of a visible window.
        wxMemoryDC dc;
        dc.SetFont(font);

        const size_t len = text.length();

        for(size_t i = 0; i < len; ++i) {
            wxString currentCharStr;

#ifdef __WXMSW__
            // On Windows, wxString is UTF-16, so we must manually handle surrogate pairs
            // to correctly process Unicode code points outside the Basic Multilingual Plane.
            wxChar ch1 = text[i];
            
            // Check if the current character is a high surrogate and is followed by a low surrogate.
            if(IsHighSurrogate(ch1) && (i + 1) < len && IsLowSurrogate(text[i + 1])) {
                // A valid surrogate pair represents a single Unicode code point. Append both wchar_t values.
                currentCharStr.Append(ch1);
                currentCharStr.Append(text[i + 1]);
                // Manually advance the index past the low surrogate.
                i++; 
            } else {
                // Handle a regular BMP character or an invalid/standalone surrogate.
                currentCharStr.Append(ch1);
            }
#else
            // On non-Windows platforms, the default wxString encoding is UTF-32.
            // In this mode, text.at(i) correctly returns the i-th full Unicode code point.
            // This also works for the non-default UTF-8 build configuration.
            currentCharStr = text.at(i);
#endif

            if(currentCharStr == "\n") { // Handle explicit newline characters.
                wrappedText += currentLine.Trim(true) + "\n";
                currentLine.Clear();
                currentLineWidth = 0;
                continue;
            }

            // Measure the width of the *single* character.
            wxCoord charWidth;
            dc.GetTextExtent(currentCharStr, &charWidth, nullptr);

            // Check if adding this character makes the current line exceed the wrapWidth.
            // We only break if the current line is not empty (to avoid an infinite loop if a single char > wrapWidth).
            if(currentLineWidth + charWidth > wrapWidth && !currentLine.IsEmpty()) {
                wrappedText += currentLine.Trim(true) + "\n"; // Add the completed line.
                currentLine.Clear();                          // Start a new line.
                currentLineWidth = 0;                         // Reset line width.
            }

            // Append the character to the current line and add its width.
            currentLine += currentCharStr;
            currentLineWidth += charWidth;

        }

        // After the loop, append any remaining text on the last line.
        if(!currentLine.IsEmpty()) {
            wrappedText += currentLine.Trim(true);
        }

        return wrappedText;
    }

    /**
     * @brief Limits the text in a wxTextCtrl to a maximum number of Unicode code points.
     *
     * If the current text exceeds the specified maximum length, this function truncates it.
     * The check and truncation are Unicode-aware, correctly handling multi-byte characters
     * and surrogate pairs. The UI update is deferred via CallAfter() for safety,
     * which is necessary when modifying a control during its own event handler.
     * An audible bell is sounded to notify the user when the limit is enforced.
     *
     * @param textEntry The wxTextCtrl to monitor and modify.
     * @param maxLength The maximum allowed number of Unicode code points.
     */
    void LimitTextLength(wxTextCtrl* textEntry, size_t maxLength) {
        auto val = textEntry->GetValue();

        // Check if the number of Unicode code points exceeds the maximum allowed length.
        if(GetUnicodeLength(val) > maxLength) {
            // Store the original cursor position. GetInsertionPoint() returns a physical
            // index into the string's internal wchar_t array, not a logical Unicode code point index.
            auto original_pos = textEntry->GetInsertionPoint();

            // Truncate the string to the maximum number of code points in a Unicode-safe way.
            auto truncated_str = UnicodeLeft(val, maxLength);

            // Defer the UI update. Modifying a control's value directly from within an
            // event handler that it generated (like EVT_TEXT) can lead to instability.
            textEntry->CallAfter([=]() {
                textEntry->SetValue(truncated_str);

                // Restore the cursor's position, ensuring it's not placed beyond the end
                // of the new, shorter string. The physical length of the truncated string
                // is the new boundary.
                textEntry->SetInsertionPoint(std::min(original_pos, (long)truncated_str.length()));
                
                // Provide audible feedback that the input was automatically shortened.
                wxBell();
            });
        }
    }
} // namespace TextUtil
