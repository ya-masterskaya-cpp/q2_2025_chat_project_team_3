#pragma once

#include <fstream>
#include <filesystem>

#include <server/models/Migrations.h>

struct MigrationFile {
    uint64_t    timestamp;
    std::string name;
    std::string filename;
    std::filesystem::path fullPath;
};

inline std::pair<std::string_view, std::string_view>
parse_sql_filename(std::string_view input) noexcept {
    if(input.ends_with(".sql")) input.remove_suffix(4);
    if(auto pos = input.find('-'); pos != std::string_view::npos) {
        return { input.substr(0, pos), input.substr(pos+1) };
    }
    return {};
}

inline std::optional<uint64_t>
parseTimestamp(std::string_view s) {
    uint64_t v;
    auto [ptr, ec] = std::from_chars(s.data(), s.data()+s.size(), v);
    return ec==std::errc{} ? std::optional{v} : std::nullopt;
}

inline std::vector<MigrationFile>
scanMigrationFiles(const std::filesystem::path &dir) {
    std::vector<MigrationFile> out;
    if(!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir)) {
        LOG_ERROR << "Migrations dir missing: " << dir;
        return out;
    }
    for (auto &e: std::filesystem::directory_iterator(dir)) {
        if(!e.is_regular_file()) continue;
        auto fn = e.path().filename().string();
        auto [tsStr,nameStr] = parse_sql_filename(fn);
        if(tsStr.empty() || nameStr.empty()) continue;
        if(auto ts = parseTimestamp(tsStr)) {
            out.push_back({*ts, std::string{nameStr}, fn, e.path()});
        }
    }
    std::ranges::sort(out, {}, &MigrationFile::timestamp);
    return out;
}

//–– Coroutine to apply migrations –––––––––––––––––––––––––––––––––––––––––
inline drogon::Task<>
applyMigrations(drogon::orm::DbClientPtr db, const std::vector<MigrationFile> &files) {
    namespace models = drogon_model::drogon_test;
    using namespace drogon::orm;

    auto rc = co_await db->execSqlCoro("SELECT to_regclass('public.migrations')");
    if(rc.empty() || rc[0][0].isNull()) {
        LOG_INFO << "No migrations table; running main.sql";

        std::ifstream mf("db/main.sql");
        if(!mf) {
            LOG_FATAL << "Cannot open db/main.sql";
            drogon::app().quit();
            co_return;
        }
        co_await db->execSqlCoro(std::string(std::istreambuf_iterator<char>(mf), 
                                          std::istreambuf_iterator<char>()));
    }

    auto latest = co_await CoroMapper<models::Migrations>(db)
                    .orderBy(models::Migrations::Cols::_timestamp, SortOrder::DESC)
                    .limit(1)
                    .findAll();

    uint64_t lastTs = latest.empty() ? 0 : latest[0].getValueOfTimestamp();

    for (auto &f : files) {
        if(f.timestamp <= lastTs) {
            LOG_DEBUG << "Skipping: " << f.filename;
            continue;
        }

        LOG_INFO << "Applying: " << f.filename;

        std::ifstream sf(f.fullPath);
        if(!sf) {
            LOG_FATAL << "Cannot open " << f.fullPath;
            drogon::app().quit();
            co_return;
        }

        auto err = co_await WithTransaction(
            [&](auto tx) -> drogon::Task<ScopedTransactionResult> {
                co_await tx->execSqlCoro(std::string(std::istreambuf_iterator<char>(sf),
                                         std::istreambuf_iterator<char>()));
                models::Migrations m;
                m.setTimestamp(f.timestamp);
                m.setName(f.name);
                co_await CoroMapper<models::Migrations>(tx).insert(m);
                co_return std::nullopt;
            });

        if(err) {
            LOG_FATAL << "Migration failed [" << f.name << "]: " << *err;
            drogon::app().quit();
            co_return;
        }

        LOG_INFO << "Migration succeeded: " << f.name;
    }

    co_return;
}
