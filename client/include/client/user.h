#pragma once

#include <wx/string.h> // For wxString

namespace client {

// Define the User struct with server-provided details
struct User {
    int32_t id;
    wxString username;
    chat::UserRights role;
    int count = 0;

    // Default constructor (important for standard containers)
    User() : id(0), username(wxEmptyString), role(chat::UserRights::REGULAR) {}

    // Parameterized constructor
    User(int32_t userId, const wxString& name, chat::UserRights userRole)
        : id(userId), username(name), role(userRole) {} // Corrected member initialization

    // Sorts by: 1. Online, 2. Role (lower enum value first), 3. Username (case-insensitive), 4. ID
    bool operator<(const User& other) const {
		// 1. Sort by online status (online users first)
        if (count == 0 || other.count == 0) {
            // Current user is online and other is offline -> current should appear first
            if (count == 0 && other.count != 0) {
                return true;
            }

            // Current user is offline and other is online -> current should appear later
            if (count != 0 && other.count == 0) {
                return false;
            }
        }
        
        // 2. Sort by Role
        if (role != other.role) {
            return static_cast<int>(role) < static_cast<int>(other.role);
        }

        // 3. If roles are the same, sort by Username (case-insensitive)
        int usernameCmp = username.CmpNoCase(other.username);
        if (usernameCmp != 0) {
            return usernameCmp > 0;
        }

        // 4. If roles and usernames are the same, sort by ID (tie-breaker)
        return id < other.id;
    }

    // Equality operator for finding users by ID
    bool operator==(const User& other) const {
        return id == other.id;
    }
};

} // namespace client
