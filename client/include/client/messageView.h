#pragma once

#include <wx/wx.h>
#include <wx/vscroll.h>
#include <client/message.h>
#include <deque>
#include <vector>

#include <deque>
#include <utility>

template<typename T>
class GenericCacher {
private:
    std::deque<T*> pool_;

public:
    // Get an instance: return cached one if available, otherwise construct a new one
    template<typename... Args>
    T* get_instance(Args&&... args) {
        if (!pool_.empty()) {
            T* obj = std::move(pool_.front());
            pool_.pop_front();
            obj->Update(std::forward<Args>(args)...);
            return obj;
        } else {
            T* obj = new T(std::forward<Args>(args)...);
            return obj;
        }
    }

    // Return an instance back to the cacher
    void return_instance(T*&& obj) {
        obj->Hide();
        pool_.emplace_back(std::move(obj));
    }

    // Optional: Return an instance without moving (if you need to reuse it in-place)
    void return_instance(const T*& obj) {
        obj->Hide();
        pool_.push_back(obj);
    }

    // Optional: Clear the pool
    void clear() {
        pool_.clear();
    }

    // Optional: Check if the pool is empty
    bool empty() const {
        return pool_.empty();
    }

    // Optional: Get current number of cached instances
    size_t size() const {
        return pool_.size();
    }
};

class ChatPanel;
class MessageWidget;

class MessageView : public wxVScrolledWindow {
public:
    // Constructor no longer takes initialWrapWidth, it's set later.
    MessageView(ChatPanel* parent);

    // --- PUBLIC API ---
    void Start();
    void OnMessagesReceived(const std::vector<Message>& messages, bool isHistoryResponse);
    void ReWrapAllMessages(int wrapWidth);
    void Clear();

protected:
    // --- THE CRITICAL FIX ---
    // This signature must exactly match the pure virtual function in the base class.
    virtual wxCoord OnGetRowHeight(size_t row) const override;
    // ----------------------

private:
    void OnSize(wxSizeEvent& event);
    void OnPaint(wxPaintEvent& event);
    void OnScroll(wxScrollWinEvent& event);
    void UpdateLayoutAndScroll(const std::vector<Message>& messages, bool isHistoryResponse);
    void UpdateWidgetPositions();
    void AddMessageWidget(const Message& msg, bool prepend);
    int TrimWidgets(bool fromTop);
    void LoadOlderMessages();
    void LoadNewerMessages();
    wxCoord CalculateTotalHeight() const;

    ChatPanel* m_chatPanelParent;
    std::deque<MessageWidget*> m_messageWidgets;
    GenericCacher<MessageWidget> m_widgetsPool;

    bool m_loadingOlder;
    bool m_loadingNewer;
    int m_lastKnownWrapWidth;

    static const int MAX_MESSAGES = 100;
    static const int CHUNK_SIZE = 25;
    static const int LOAD_THRESHOLD_ROWS = 10;
    const int SCROLL_STEP = FromDIP(10);
};
