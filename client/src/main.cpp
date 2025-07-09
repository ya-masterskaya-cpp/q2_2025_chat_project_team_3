#include <wx/wx.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/font.h>
#include <wx/dir.h>
#include <wx/fontmap.h>

#include <drogon/HttpAppFramework.h>
#include <client/mainWidget.h>
#include <client/appConfig.h>
#include <client/app.h>

wxIMPLEMENT_APP(client::MyApp);

namespace client {

MyApp::MyApp() = default;
MyApp::~MyApp() = default;

AppConfig& MyApp::GetConfig() {
    return *m_config;
}

bool MyApp::OnInit() {
    this->SetVendorName("0xCAFEBABE");
    this->SetAppName("SlightlyPrettyChat");
    this->SetAppDisplayName("Slightly Pretty Chat™");

    // 1. Determine the base directory where our resources are located.
    //    This is different on macOS (.app bundle) vs. other platforms.
    wxFileName resourceDir;
#ifdef __WXMAC__
    // On macOS, resources are in YourApp.app/Contents/Resources/
    // wxStandardPaths correctly finds this for us.
    resourceDir.SetPath(wxStandardPaths::Get().GetResourcesDir());
#else
    // On Windows and Linux/AppImage, we placed the fonts relative to the executable.
    // So, our base directory is the executable's directory.
    resourceDir.SetPath(wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath());
#endif

    // 2. Append the "fonts" subdirectory to the base path.
    //    wxFileName handles the path separator ('/' or '\') automatically.
    resourceDir.AppendDir("fonts");
    wxString fontDirPath = resourceDir.GetPath();

    if (wxDirExists(fontDirPath)) {
        wxArrayString fontFiles;
        wxDir::GetAllFiles(fontDirPath, &fontFiles, "*.ttf", wxDIR_FILES);
        
        for (const wxString& fontFile : fontFiles) {
            LOG_INFO << "Loading font " << fontFile.utf8_string();
            wxFont::AddPrivateFont(fontFile);
        }
    } else {
        LOG_ERROR << "Font directory does not exist: " << fontDirPath.ToStdString();
    }

    // Start Drogon (networking) in a background thread
    drogon::app().setLogLevel(trantor::Logger::kTrace);
    drogon::app().setDocumentRoot(std::string(wxStandardPaths::Get().GetUserDataDir().ToUTF8()));
    drogon::app().setUploadPath("uploads");
    drogonThread = std::thread([] {
        drogon::app().run();
    });
    
    m_config = std::make_unique<AppConfig>(GetAppName());

    // Now start the GUI
    mw = new MainWidget();
    mw->ShowInitial();
    mw->Show();

    return true;
}

int MyApp::OnExit() {
    drogon::app().quit();
    if(drogonThread.joinable()) drogonThread.join();
    return 0;
}

} // namespace client
