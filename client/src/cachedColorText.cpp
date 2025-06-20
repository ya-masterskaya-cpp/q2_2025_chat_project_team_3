#include <client/cachedColorText.h>
#include <wx/graphics.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>

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
    return GetTextExtent(m_label);
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
