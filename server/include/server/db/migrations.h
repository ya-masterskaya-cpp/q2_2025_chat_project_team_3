#pragma once

namespace server {

//–– Coroutine to apply migrations –––––––––––––––––––––––––––––––––––––––––
drogon::Task<bool> MigrateDatabase(drogon::orm::DbClientPtr db);

} // namespace server
