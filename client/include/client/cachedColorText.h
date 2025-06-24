#pragma once

#include <wx/control.h>
#include <wx/bitmap.h>
#include <wx/stattext.h>

// This class behaves like a wxStaticText for layout and API purposes,
// but is built on a reliable, paintable base.
class CachedColorText : public wxControl {
public:
    CachedColorText(wxWindow* parent, wxWindowID id, const wxString& label,
        const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
        long style = 0, const wxString& name = wxStaticTextNameStr);

    virtual void SetLabel(const wxString& label) override;
    virtual wxString GetLabel() const override;
    virtual bool SetFont(const wxFont& font) override;
    void InvalidateCache();
    void InvalidateCacheAndRefresh();

protected:
    virtual wxSize DoGetBestSize() const override;

private:
    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnSysColourChanged(wxSysColourChangedEvent& event);
    void RenderToCache();
    wxDouble GetLineHeight() const;

    wxString m_label;
    wxBitmap m_cache;
    mutable double m_cachedLineHeight;

    wxDECLARE_EVENT_TABLE();
};
