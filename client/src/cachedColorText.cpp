#include <client/cachedColorText.h>
#include <client/graphicsContextManager.h>

#include <wx/graphics.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/tokenzr.h>

namespace client {

wxBEGIN_EVENT_TABLE(CachedColorText, wxControl)
    EVT_PAINT(CachedColorText::OnPaint)
    EVT_SIZE(CachedColorText::OnSize)
    EVT_SYS_COLOUR_CHANGED(CachedColorText::OnSysColourChanged)
wxEND_EVENT_TABLE()

CachedColorText::CachedColorText(wxWindow* parent, wxWindowID id, const wxString& label,
    const wxPoint& pos, const wxSize& size, long style, const wxString& name, bool stretchToParentWidth)
    : wxControl(parent, id, pos, size, style | wxBORDER_NONE, wxDefaultValidator, name) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    m_stretchesToParentWidth = stretchToParentWidth;
    m_label = label;
    InvalidateLayoutCaches();
}

void CachedColorText::SetLabel(const wxString& label) {
    if(m_label == label) {
        return;
    }
    m_label = label;
    InvalidateLayoutCaches();
    InvalidateCache();
}

wxString CachedColorText::GetLabel() const {
    return m_label;
}

bool CachedColorText::SetFont(const wxFont& font) {
    bool ret = wxControl::SetFont(font);
    if(ret) {
        InvalidateLayoutCaches();
        InvalidateCache();
    }
    return ret;
}

void CachedColorText::InvalidateCache() {
    m_cache = wxBitmap();
}

void CachedColorText::InvalidateCacheAndRefresh() {
    m_cache = wxBitmap();
    Refresh();
}

void CachedColorText::InvalidateLayoutCaches() {
    m_cachedLineHeight = -1.0;
    m_cachedBestSize.Set(wxDefaultCoord, wxDefaultCoord);
    InvalidateBestSize();
}

wxSize CachedColorText::DoGetBestSize() const {
    if(m_cachedBestSize.IsFullySpecified()) {
        return m_cachedBestSize;
    }

    // Handle empty label case for both modes first.
    if(m_label.IsEmpty()) {
        int width = 0;
        if (m_stretchesToParentWidth) {
            wxWindow* parent = GetParent();
            if (parent) {
                width = parent->GetClientSize().GetWidth();
            }
        }
        m_cachedBestSize = wxSize(width, 0);
        return m_cachedBestSize;
    }

    const wxDouble lineHeight = GetLineHeight();
    int totalWidth = 0;
    int totalHeight = 0;

    if(m_stretchesToParentWidth) {
        wxWindow* parent = GetParent();
        if(!parent) {
            return wxSize(20, 20);
        }
        totalWidth = parent->GetClientSize().GetWidth();

        wxStringTokenizer tokenizer(m_label, "\n");
        int numLines = tokenizer.CountTokens();
        // A trailing newline means an extra empty line.
        if(!m_label.IsEmpty() && m_label.EndsWith("\n")) {
            numLines++;
        }
        // If the string is not empty but has no newlines, it's still one line.
        if(numLines == 0 && !m_label.IsEmpty()) {
            numLines = 1;
        }
        totalHeight = ceil(numLines * lineHeight);

    } else {
        totalHeight = ceil(lineHeight);

        wxMemoryDC dc;
        wxBitmap tempBitmap(1, 1);
        dc.SelectObject(tempBitmap);
        dc.SetFont(GetFont());

        GraphicsContextManager ctx(dc);
        wxGraphicsContext* gc = ctx.GetContext();

        wxDouble measuredWidth = 0.0;
        if(gc) {
            gc->SetFont(GetFont(), GetForegroundColour());
            gc->GetTextExtent(m_label, &measuredWidth, nullptr, nullptr, nullptr);
        } else {
            wxCoord w;
            dc.GetTextExtent(m_label, &w, nullptr);
            measuredWidth = w;
        }
        totalWidth = ceil(measuredWidth);
    }

    m_cachedBestSize = wxSize(totalWidth, totalHeight);
    return m_cachedBestSize;
}

void CachedColorText::RenderToCache() {
    wxSize size = GetClientSize();
    if(size.x <= 0 || size.y <= 0) {
        m_cache = wxBitmap();
        return;
    }

    m_cache.Create(size);
    wxMemoryDC memDC(m_cache);

    wxWindow* parent = GetParent();
    if(parent) {
        memDC.SetBackground(parent->GetBackgroundColour());
    }
    memDC.Clear();

    GraphicsContextManager ctx(memDC);
    wxGraphicsContext* gc = ctx.GetContext();

    if(gc) {
        gc->SetFont(GetFont(), GetForegroundColour());

        if(m_stretchesToParentWidth) {
            // The wxGraphicsContext implementation on macOS does not handle '\n' characters.
            // To ensure consistent behavior on all platforms, we must manually parse the
            // string and draw it line by line.
            const wxDouble lineHeight = GetLineHeight();
            wxStringTokenizer tokenizer(GetLabel(), "\n");
            wxDouble y = 0.0;

            while(tokenizer.HasMoreTokens()) {
                wxString line = tokenizer.GetNextToken();
                gc->DrawText(line, 0, y);
                y += lineHeight;
            }
        } else {
            gc->DrawText(GetLabel(), 0, 0);
        }
    } else {
        // Fallback to wxDC if GraphicsContext fails (though this would lose color emoji support).
        memDC.SetFont(GetFont());
        memDC.SetTextForeground(GetForegroundColour());

        if(m_stretchesToParentWidth) {
            memDC.DrawLabel(GetLabel(), wxRect(0, 0, size.x, size.y));
        } else {
            memDC.DrawText(GetLabel(), 0, 0);
        }
    }
}

void CachedColorText::OnPaint([[maybe_unused]] wxPaintEvent& event) {
    wxPaintDC dc{this};
    if(!m_cache.IsOk()) {
        RenderToCache();
    }
    if(m_cache.IsOk()) {
        dc.DrawBitmap(m_cache, 0, 0);
    }
}

void CachedColorText::OnSize(wxSizeEvent& event) {
    // Only invalidate if the size has actually changed.
    if(m_cache.IsOk() && GetClientSize() != m_cache.GetSize()) {
        InvalidateCache();
    }

    // If the control's best size depends on the parent's width,
    // a size change implies that the parent may have been resized,
    // making our layout cache stale.
    if(m_stretchesToParentWidth) {
        InvalidateLayoutCaches();
    }

    event.Skip();
}

void CachedColorText::OnSysColourChanged(wxSysColourChangedEvent& event) {
    // The system theme changed, so our colors are stale.
    InvalidateCache();
    event.Skip();
}

wxDouble CachedColorText::GetLineHeight() const {
    // CACHE HIT: If the line height is already calculated, return it immediately.
    if (m_cachedLineHeight >= 0) {
        return m_cachedLineHeight;
    }

    // CACHE MISS: We must perform the one-time calculation.
    // Use a temporary wxDC to get the true font metrics, as this is the most
    // reliable way to determine line height for all scripts (Latin, CJK, etc.).
    wxMemoryDC dc;
    // A wxMemoryDC on some platforms requires a bitmap to be selected to have
    // valid characteristics. A 1x1 bitmap is sufficient.
    wxBitmap tempBitmap(1, 1);
    dc.SelectObject(tempBitmap);

    dc.SetFont(GetFont());

    wxFontMetrics metrics = dc.GetFontMetrics();

    // The standard line height is the sum of the height above and below the
    // baseline, plus any recommended inter-line spacing (leading).
    m_cachedLineHeight = metrics.ascent + metrics.descent + metrics.externalLeading;

    // A font might have zero height if it's invalid. Guard against division by zero later.
    if (m_cachedLineHeight <= 0) {
        // Fallback to a reasonable default if metrics are weird.
        m_cachedLineHeight = dc.GetCharHeight();
    }

    return m_cachedLineHeight;
}

} // namespace client
