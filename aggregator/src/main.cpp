#include <aggregator/controller/WsController.h>

int main() {
    std::filesystem::create_directory("logs");
    LOG_INFO << "Starting Drogon application...";
    drogon::app().loadConfigFile("config.json");
    LOG_INFO << "Entering main loop...";
    drogon::app().run();
    LOG_INFO << "Drogon stopped.";
    return 0;
}
