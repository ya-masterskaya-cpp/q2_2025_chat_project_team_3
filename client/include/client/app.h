#pragma once

#include <wx/app.h>
#include <memory>
#include <thread>

namespace client {

class AppConfig;
class MainWidget;

class MyApp : public wxApp {
public:
    MyApp();
    virtual ~MyApp();

    virtual bool OnInit() override;
    virtual int OnExit() override;

    AppConfig& GetConfig();

private:
    std::thread drogonThread;
    std::unique_ptr<AppConfig> m_config;
    MainWidget* mw = nullptr;
};

} // namespace client

// This macro allows us to use wxGetApp() to get a MyApp& instead of wxApp&
wxDECLARE_APP(client::MyApp);
