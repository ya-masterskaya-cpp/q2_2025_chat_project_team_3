#include <client/cachedColorText.h>
#include <wx/graphics.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/tokenzr.h>

wxBEGIN_EVENT_TABLE(CachedColorText, wxControl)
    EVT_PAINT(CachedColorText::OnPaint)
    EVT_SIZE(CachedColorText::OnSize)
    EVT_SYS_COLOUR_CHANGED(CachedColorText::OnSysColourChanged)
wxEND_EVENT_TABLE()

CachedColorText::CachedColorText(wxWindow* parent, wxWindowID id, const wxString& label,
    const wxPoint& pos, const wxSize& size, long style, const wxString& name)
    : wxControl(parent, id, pos, size, style | wxBORDER_NONE, wxDefaultValidator, name) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    m_label = label;
    m_cachedLineHeight = -1.0;
}

void CachedColorText::SetLabel(const wxString& label) {
    if(m_label == label) {
        return;
    }
    m_label = label;
    InvalidateBestSize();
    InvalidateCache();
}

wxString CachedColorText::GetLabel() const {
    return m_label;
}

bool CachedColorText::SetFont(const wxFont& font) {
    bool ret = wxControl::SetFont(font);
    if(ret) {
        m_cachedLineHeight = -1.0;
        InvalidateBestSize();
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

wxSize CachedColorText::DoGetBestSize() const {
    wxWindow* parent = GetParent();
    if(!parent) {
        return wxSize(20, 20);
    }

    const int availableWidth = parent->GetClientSize().GetWidth();

    if(m_label.IsEmpty()) {
        return wxSize(availableWidth, 0);
    }

    const wxDouble lineHeight = GetLineHeight();

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

    const int totalHeight = ceil(numLines * lineHeight);

    return wxSize(availableWidth, totalHeight);
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

    wxGraphicsContext* gc = nullptr;
#ifdef __WXMSW__
    // Use Direct2D renderer on Windows for emoji support
    wxGraphicsRenderer* d2dRenderer = wxGraphicsRenderer::GetDirect2DRenderer();
    if(d2dRenderer) {
        gc = d2dRenderer->CreateContext(memDC);
    }
#else
    // On macOS and Linux, Create() will use the best available renderer (CoreGraphics/Cairo).
    gc = wxGraphicsContext::Create(memDC);
#endif

    if(gc) {
        gc->SetFont(GetFont(), GetForegroundColour());
        
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

        delete gc;
    } else {
        // Fallback to wxDC if GraphicsContext fails (though this would lose color emoji support).
        memDC.SetFont(GetFont());
        memDC.SetTextForeground(GetForegroundColour());
        memDC.DrawLabel(GetLabel(), wxRect(0, 0, size.x, size.y));
    }
}

void CachedColorText::OnPaint(wxPaintEvent& event) {
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
    wxClientDC dc(const_cast<CachedColorText*>(this));
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
