#include <wx/wx.h>
#include <wx/stdpaths.h>
#include <drogon/HttpAppFramework.h>
#include <client/mainWidget.h>
#include <client/appConfig.h>
#include <client/app.h>

wxIMPLEMENT_APP(MyApp);

MyApp::MyApp() = default;
MyApp::~MyApp() = default;

AppConfig& MyApp::GetConfig() {
    return *m_config;
}

bool MyApp::OnInit() {
    this->SetVendorName("0xCAFEBABE");
    this->SetAppName("SlightlyPrettyChat");
    this->SetAppDisplayName("Slightly Pretty Chat™");
    
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
    return true;
}

int MyApp::OnExit() {
    drogon::app().quit();
    if(drogonThread.joinable()) drogonThread.join();
    return 0;
}


