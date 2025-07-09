#pragma once

#include <wx/graphics.h>
#include <wx/dcmemory.h>

namespace client {

/**
 * @class GraphicsContextManager
 * @brief An RAII helper to create and manage a wxGraphicsContext from a wxMemoryDC.
 *
 * This class encapsulates the platform-specific logic for creating the best
 * available wxGraphicsContext from an in-memory device context. On Windows,
 * it attempts to use the Direct2D renderer for superior text and emoji support.
 * On other platforms, or if Direct2D is unavailable, it falls back to the
 * default renderer (e.g., CoreGraphics or Cairo).
 *
 * It is designed as an RAII wrapper,
 * ensuring that the created context is properly deleted when the manager instance
 * goes out of scope. It also provides a fallback to the original wxMemoryDC if a
 * graphics context cannot be created, allowing calling code to function robustly.
 */
class GraphicsContextManager {
public:
    /**
     * @brief Constructs the manager and attempts to create a wxGraphicsContext.
     * @param dc The wxMemoryDC to create the context from. This DC must remain
     *           valid for the entire lifetime of the GraphicsContextManager object.
     */
    explicit GraphicsContextManager(wxMemoryDC& dc);

    /**
     * @brief Destroys the manager and cleans up the created wxGraphicsContext.
     */
    ~GraphicsContextManager();

    /**
     * @brief Retrieves the created wxGraphicsContext.
     * @return A pointer to the wxGraphicsContext, or nullptr if creation failed.
     *         The returned pointer is owned by this manager and should not be
     *         deleted by the caller.
     */
    wxGraphicsContext* GetContext() const;

    /**
     * @brief Retrieves the original wxMemoryDC passed during construction.
     * @return A reference to the original wxMemoryDC. This is useful as a fallback
     *         if GetContext() returns nullptr.
     */
    wxMemoryDC& GetDC() const;

private:
    wxMemoryDC& m_dc;
    wxGraphicsContext* m_gc;

    // This class manages a unique resource (m_gc) and is non-copyable.
    GraphicsContextManager(const GraphicsContextManager&) = delete;
    GraphicsContextManager& operator=(const GraphicsContextManager&) = delete;
};

} // namespace client
