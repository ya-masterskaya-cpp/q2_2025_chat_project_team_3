#pragma once

#include <wx/wx.h>

namespace client {

struct Message {
    wxString user;
    wxString msg;
    int64_t timestamp;
};

} // namespace client
