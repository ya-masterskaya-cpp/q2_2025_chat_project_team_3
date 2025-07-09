#include <client/appConfig.h>

#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/filefn.h>
#include <wx/snglinst.h>
#include <wx/wfstream.h>
#include <wx/txtstrm.h>

#include <fstream>

namespace client {

AppConfig::AppConfig(const wxString& appName)
    : m_instanceChecker(nullptr),
      m_isFirstInstance(false)
{
    m_instanceChecker = new wxSingleInstanceChecker(appName);
    if(m_instanceChecker->IsAnotherRunning()) {
        m_isFirstInstance = false;
        LOG_WARN << "Another instance of the application is already running.";
    } else {
        m_isFirstInstance = true;
    }

    InitPath();
    ReadConfig();
}

AppConfig::~AppConfig() {
    if(m_isFirstInstance) {
        // Sync the in-memory server set back to the JSON object before saving.
        Json::Value serversArray(Json::arrayValue);
        for(const auto& server : m_servers) {
            serversArray.append(server);
        }
        m_root["servers"] = serversArray;

        // Use a stringstream to get the JSON content as a string.
        std::stringstream ss;
        Json::StreamWriterBuilder builder;
        builder["commentStyle"] = "None";
        builder["indentation"] = "    ";
        std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
        writer->write(m_root, &ss);
        
        // Convert the content to a wxString, telling it the source is UTF-8.
        wxString file_contents = wxString::FromUTF8(ss.str());

        // Use wxFile for direct, cross-platform file access.
        wxFile configFile(m_configFilePath, wxFile::write);

        if(!configFile.IsOpened()) {
            LOG_ERROR << "Failed to open config file for writing: " << std::string(m_configFilePath.utf8_str());
        } else {
            // Write the wxString content to the file, encoding the bytes as UTF-8.
            if(!configFile.Write(file_contents, wxConvUTF8)) {
                LOG_ERROR << "Failed to write content to config file: " << std::string(m_configFilePath.utf8_str());
            }
        }
    }
    delete m_instanceChecker;
}

void AppConfig::InitPath() {
    // 1. Get the full path to app's config directory as a string.
    wxStandardPaths& paths = wxStandardPaths::Get();
    wxString configDir = paths.GetUserDataDir();

    // 2. Use that string to check if the directory exists.
    if (!wxDirExists(configDir)) {
        // 3. If it doesn't exist, create it.
        if (!wxMkdir(configDir, wxS_DIR_DEFAULT)) {
            LOG_ERROR << "Could not create configuration directory: " << configDir.utf8_str();
            return;
        }
    }

    // 4. Now that the directory is guaranteed to exist, use wxFileName
    // to correctly and safely build the final file path.
    wxFileName finalPath(configDir, "config.json");

    m_configFilePath = finalPath.GetFullPath();
}

void AppConfig::ReadConfig() {
    // 1. Use wxFile for direct, cross-platform file access.
    if(wxFileName::FileExists(m_configFilePath)) {
        wxFile configFile(m_configFilePath, wxFile::read);
        wxString file_contents;

        if(!configFile.IsOpened()) {
            LOG_INFO << "Configuration file not found. Creating a new one with default values.";
        } else {
            // 2. Read the entire file's content into a wxString.
            //    We explicitly tell it to interpret the file's bytes as UTF-8.
            if(!configFile.ReadAll(&file_contents, wxConvUTF8)) {
                LOG_ERROR << "Failed to read content from config file: " << std::string(m_configFilePath.utf8_str());
            }
        }
        // configFile is automatically closed when it goes out of scope.

        // 3. If we have content, parse it.
        if(!file_contents.IsEmpty()) {
            Json::Reader reader;
            // Use the safe utf8_str() conversion to pass a UTF-8 std::string to the parser.
            if(!reader.parse(std::string(file_contents.utf8_str()), m_root)) {
                LOG_ERROR << "Error parsing config.json: " << reader.getFormattedErrorMessages()
                        << ". Will reset to default.";
                m_root = Json::Value(Json::objectValue);
            }
        }
    }

    // Ensure the root is a valid object, even if the file was empty or parsing failed.
    if(!m_root.isObject()) {
        m_root = Json::Value(Json::objectValue);
    }

    // Now, populate the in-memory server cache from the JSON object.
    m_servers.clear();
    if(m_root.isMember("servers") && m_root["servers"].isArray()) {
        const Json::Value& serversArray = m_root["servers"];
        for(const auto& serverEntry : serversArray) {
            if(serverEntry.isString()) {
                m_servers.insert(serverEntry.asString());
            }
        }
    }
}

std::set<std::string> AppConfig::getServers() const {
    return m_servers;
}

bool AppConfig::addServer(const std::string& server) {
    if(server.empty()) {
        return false; // Cannot add an empty server.
    }

    return m_servers.insert(server).second;
}

void AppConfig::removeServer(const std::string& server) {
    // std::set::erase is safe to call even if the element doesn't exist.
    m_servers.erase(server);
}

Json::Value& AppConfig::getRoot() {
    return m_root;
}

const Json::Value& AppConfig::getRoot() const {
    return m_root;
}

bool AppConfig::IsFirstInstance() const {
    return m_isFirstInstance;
}

} // namespace client
