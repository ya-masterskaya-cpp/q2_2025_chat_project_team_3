#pragma once

#include <wx/wx.h>

struct Message {
    wxString user;
    wxString msg;
    int64_t timestamp;
    int64_t messageId;
};
