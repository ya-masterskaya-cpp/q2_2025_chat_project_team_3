#pragma once

#include <wx/string.h>
#include <set>

class wxSingleInstanceChecker;

namespace client {

class AppConfig {
public:
    // The constructor takes app's name to create a unique directory
    // and instance checker name.
    AppConfig(const wxString& appName);

    // The destructor handles saving the config and releasing the instance lock.
    ~AppConfig();

    // Expose the root JSON object for reading and writing.
    Json::Value& getRoot();
    const Json::Value& getRoot() const;

    // Check if this is the first instance of the application.
    bool IsFirstInstance() const;

    // Returns a set of unique server strings.
    std::set<std::string> getServers() const;

    // Adds a server if it's not already present.
    bool addServer(const std::string& server);

    // Removes all occurrences of a server.
    void removeServer(const std::string& server);

private:
    // Helper to initialize the path. Called from the constructor.
    void InitPath();
    // Helper to read the config file. Called from the constructor.
    void ReadConfig();

    Json::Value m_root;
    wxString m_configFilePath;
    wxSingleInstanceChecker* m_instanceChecker;
    bool m_isFirstInstance;

    // In-memory cache for the server list. Synced on load and save.
    std::set<std::string> m_servers;

    // Disable copy constructor and assignment operator
    AppConfig(const AppConfig&) = delete;
    AppConfig& operator=(const AppConfig&) = delete;
};

} // namespace client
