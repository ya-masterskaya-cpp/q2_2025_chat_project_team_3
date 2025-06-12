#pragma once

//–– Coroutine to apply migrations –––––––––––––––––––––––––––––––––––––––––
drogon::Task<bool> MigrateDatabase(drogon::orm::DbClientPtr db);
