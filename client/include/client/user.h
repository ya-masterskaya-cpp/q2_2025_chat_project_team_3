#pragma once

#include <wx/string.h> // For wxString

// Define the User struct with server-provided details
struct User {
    int32_t id;
    wxString username;
    chat::UserRights role;

    // Default constructor (important for standard containers)
    User() : id(0), username(wxEmptyString), role(chat::UserRights::REGULAR) {}

    // Parameterized constructor
    User(int32_t userId, const wxString& name, chat::UserRights userRole)
        : id(userId), username(name), role(userRole) {} // Corrected member initialization

    // Sorts by: 1. Role (lower enum value first), 2. Username (case-insensitive), 3. ID
    bool operator<(const User& other) const {
        // 1. Sort by Role
        if (role != other.role) {
            return static_cast<int>(role) < static_cast<int>(other.role);
        }

        // 2. If roles are the same, sort by Username (case-insensitive)
        int usernameCmp = username.CmpNoCase(other.username);
        if (usernameCmp != 0) {
            return usernameCmp < 0;
        }

        // 3. If roles and usernames are the same, sort by ID (tie-breaker)
        return id < other.id;
    }

    // Equality operator for finding users by ID
    bool operator==(const User& other) const {
        return id == other.id;
    }
};
