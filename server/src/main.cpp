#include <drogon/drogon.h>
#include <server/chat/WsChat.h>
#include <server/db/migrations.h>

int main() {
    std::filesystem::create_directory("logs");

    LOG_INFO << "Starting Drogon application...";
    drogon::app().loadConfigFile("config.json");
    drogon::app().setUnicodeEscapingInJson(false); //TODO verify if we need this
                                                   //prevents jsoncpp from turning utf into escaped codepoints

    // Setup and run migrations before the app starts serving
    drogon::app().registerBeginningAdvice([]() {
        LOG_INFO << "Preparing to apply migrations...";

        auto dbClient = drogon::app().getDbClient();
        if (!dbClient) {
            LOG_FATAL << "Failed to get DB client. Check your config.json. Aborting migrations.";
            drogon::app().quit(); // Quit if DB client is not available
            return;
        }

        auto migrationFiles = scanMigrationFiles("db/migrations");
        if (migrationFiles.empty()) {
             LOG_INFO << "No migration .sql files found";
        } else {
            LOG_INFO << "Found " << migrationFiles.size() << " migration files";
        }

        try {
            drogon::sync_wait(applyMigrations(dbClient, migrationFiles));
            LOG_INFO << "Migrations check/apply process completed.";
        } catch (const drogon::orm::DrogonDbException &e) {
            LOG_FATAL << "Database exception during migrations: " << e.base().what() << ". Aborting.";
            drogon::app().quit();
        } catch (const std::exception &e) {
            LOG_FATAL << "Standard exception during migrations: " << e.what() << ". Aborting.";
            drogon::app().quit();
        } catch (...) { //NOTE catch (...) is safe here cuz we will terminate afterwards, or is it not?
            LOG_FATAL << "Unknown error during migrations. Aborting.";
            drogon::app().quit();
        }
    });

    LOG_INFO << "Entering main loop...";
    drogon::app().run();
    LOG_INFO << "Drogon stopped.";
    return 0;
}
