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
    if (!parent) {
        return wxSize(20, 20);
    }

    const int availableWidth = parent->GetClientSize().GetWidth();

    if (m_label.IsEmpty()) {
        return wxSize(availableWidth, 0);
    }

#ifdef __WXMSW__
    // --- Windows-Specific Path with Caching ---

    // Check if the line height for the current font is already cached.
    if (m_cachedLineHeight < 0) {
        // CACHE MISS: We must perform the expensive one-time calculation.
        // This will only happen once per font change.

        wxBitmap temp_bmp(1, 1);
        wxMemoryDC temp_memDC(temp_bmp);
        wxGraphicsContext* gc = nullptr;
        wxGraphicsRenderer* d2dRenderer = wxGraphicsRenderer::GetDirect2DRenderer();
        if (d2dRenderer) {
            gc = d2dRenderer->CreateContext(temp_memDC);
        }
        if (!gc) {
            gc = wxGraphicsContext::Create(temp_memDC);
        }

        if (gc) {
            gc->SetFont(GetFont(), GetForegroundColour());
            wxDouble w;
            // Use a non-const version of ourself to update the cache
            auto* self = const_cast<CachedColorText*>(this);
            gc->GetTextExtent("Tg", &w, &self->m_cachedLineHeight);
            delete gc;
        }
        else {
            // Fallback for catastrophic GC failure
            wxClientDC dc(const_cast<CachedColorText*>(this));
            dc.SetFont(GetFont());
            return wxSize(availableWidth, dc.GetMultiLineTextExtent(m_label).GetHeight());
        }
    }

    // CACHE HIT: The line height is known. The rest is super fast.
    // Count the lines in the string (this is very cheap).
    wxStringTokenizer tokenizer(m_label, "\n");
    int numLines = tokenizer.CountTokens();
    if (m_label.EndsWith("\n")) numLines++;
    if (numLines == 0 && !m_label.IsEmpty()) numLines = 1;

    const int totalHeight = ceil(numLines * m_cachedLineHeight);

    return wxSize(availableWidth, totalHeight);

#else
    // --- Linux/macOS/Other Path (already fast) ---
    wxClientDC dc(const_cast<CachedColorText*>(this));
    dc.SetFont(GetFont());
    const int requiredHeight = dc.GetMultiLineTextExtent(m_label).GetHeight();
    return wxSize(availableWidth, requiredHeight);
#endif
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
    gc = wxGraphicsContext::Create(memDC);
#endif

    if(gc) {
        gc->SetFont(GetFont(), GetForegroundColour());
        gc->DrawText(GetLabel(), 0, 0);
        delete gc;
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
