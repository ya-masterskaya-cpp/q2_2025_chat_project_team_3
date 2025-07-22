#pragma once

#include <wx/string.h>
#include <wx/font.h>
#include <wx/window.h> // Required for wxClientDC in the implementation
#include <wx/textctrl.h>

namespace client {

namespace TextUtil {
    // Function to manually wrap text based on a given width and font.
    // 'targetWindow' is needed for the wxClientDC to measure text.
    // This minimalist version iterates character by character and will break words mid-way if needed.
    wxString WrapText(wxWindow* targetWindow, const wxString& text, int wrapWidth, const wxFont& font);

    void LimitTextLength(wxTextCtrl* textEntry, size_t maxLength);

    /**
     * @brief Sanitizes an input string according to specific rules.
     *
     * This function performs the following sanitization steps:
     * 1. Trims all leading and trailing whitespace from the input string.
     * 2. On Windows (where wxString is UTF-16), it validates the string to ensure that
     *    every high surrogate is immediately followed by a low surrogate. If an invalid
     *    or "lone" surrogate is found, the function fails.
     * 3. It checks that the resulting string contains at least one character from the
     *    Basic Multilingual Plane (BMP), i.e., a code point U+0000 to U+FFFF.
     *
     * @param input The wxString to be sanitized.
     * @return The sanitized string if all checks pass. Returns an empty wxString if
     *         the input is empty after trimming, if the surrogate validation fails,
     *         or if no BMP characters are found.
     */
    wxString SanitizeInput(const wxString& input);

    /**
     * @brief Validates a URI, ensuring it has a 'ws' or 'wss' scheme.
     *
     * This function uses ada-url to check for syntactical validity and then
     * specifically checks if the scheme is 'ws' or 'wss'.
     *
     * @param url The URI to validate, as a wxString.
     * @return std::optional<std::string> containing the URI as a UTF-8 encoded
     *         std::string if it's valid, or std::nullopt otherwise.
     */
    std::optional<std::string> ValidateUrl(wxString url);

} // namespace TextUtil

} // namespace client
