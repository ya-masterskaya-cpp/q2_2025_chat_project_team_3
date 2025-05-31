#include <wx/wx.h>
#include <thread>
#include <drogon/HttpAppFramework.h>
#include <client/mainWidget.h>

std::thread drogonThread;

class MyApp : public wxApp {
public:
    MainWidget* mw = nullptr;
    virtual bool OnInit() override {
        // Start Drogon (networking) in a background thread
        drogon::app().setLogLevel(trantor::Logger::kTrace);
        
        drogonThread = std::thread([] {
            drogon::app().run();
        });

        // Now start the GUI
        mw = new MainWidget();
        return true;
    }
    virtual int OnExit() override {
        drogon::app().quit();
        if (drogonThread.joinable()) drogonThread.join();
        return 0;
    }
};

wxIMPLEMENT_APP(MyApp);