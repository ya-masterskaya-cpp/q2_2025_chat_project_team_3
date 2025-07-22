#include <client/graphicsContextManager.h>

namespace client {

GraphicsContextManager::GraphicsContextManager(wxMemoryDC& dc)
    : m_dc(dc), m_gc(nullptr)
{
#ifdef __WXMSW__
    // On MSW, the Direct2D renderer provides the best support for modern
    // text features. It can be created directly from a wxMemoryDC.
    if(wxGraphicsRenderer* d2dRenderer = wxGraphicsRenderer::GetDirect2DRenderer()) {
        m_gc = d2dRenderer->CreateContext(m_dc);
    }
#endif

    // If the platform-specific attempt was not applicable or failed,
    // use the default wxGraphicsContext::Create() factory method. This overload
    // for wxMemoryDC is guaranteed to exist and will select the best default
    // renderer (e.g., GDI+ on Windows, CoreGraphics on macOS, Cairo on Linux).
    if(!m_gc) {
        m_gc = wxGraphicsContext::Create(m_dc);
    }
}

GraphicsContextManager::~GraphicsContextManager() {
    delete m_gc;
}

wxGraphicsContext* GraphicsContextManager::GetContext() const {
    return m_gc;
}

wxMemoryDC& GraphicsContextManager::GetDC() const {
    return m_dc;
}

} // namespace client
