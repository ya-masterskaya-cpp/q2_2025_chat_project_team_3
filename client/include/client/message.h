#pragma once

#include <wx/wx.h>

namespace client {

struct Message {
    wxString user;
    int32_t userId;
    wxString msg;
    int64_t timestamp;
    int32_t messageId;
};

} // namespace client
