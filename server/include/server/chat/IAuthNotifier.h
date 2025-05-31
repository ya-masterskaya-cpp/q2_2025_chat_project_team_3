#pragma once
#include <string>

class IAuthNotifier {
public:
    virtual ~IAuthNotifier() = default;
    virtual void onUserAuthenticated(const std::string &username) = 0;
};
