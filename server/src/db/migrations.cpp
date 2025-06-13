#include <server/db/migrations.h>
#include <server/models/Migrations.h>
#include <server/utils/scoped_coro_transaction.h>

using namespace std::literals;
using namespace drogon::orm;
using namespace drogon_model::drogon_test;

struct MigrationFile {
    int64_t    timestamp;
    std::string name;
    std::string filename;
    std::filesystem::path fullPath;
};

std::pair<std::string_view, std::string_view> parse_sql_filename(std::string_view input) noexcept {
    if(input.ends_with(".sql"sv)) {
        input.remove_suffix(4);
    }
    if(auto pos = input.find("-"sv); pos != std::string_view::npos) {
        return {input.substr(0, pos), input.substr(pos+1)};
    }
    return {};
}

std::optional<int64_t> parseTimestamp(std::string_view s) noexcept {
    int64_t v;
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
    return ec==std::errc{} ? std::optional{v} : std::nullopt;
}

std::string trim(std::string_view str) {
    auto start = str.find_first_not_of(" \t\n\r\f\v"sv);
    if(start == std::string_view::npos) {
        return "";
    }
    auto end = str.find_last_not_of(" \t\n\r\f\v"sv);
    return std::string(str.substr(start, end - start + 1));
}

auto split_and_trim(const std::string& content) {
    return content 
        | std::views::split(';')
        | std::views::transform([](auto&& range) {
            return std::string_view(range.begin(), range.end());
        })
        | std::views::transform(trim);
}

std::optional<std::vector<MigrationFile>> scanMigrationFiles(const std::filesystem::path &dir) {
    std::vector<MigrationFile> out;
    if(!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir)) {
        LOG_FATAL << "Migrations dir missing: "sv << dir;
        return std::nullopt;
    }
    for(auto &e: std::filesystem::directory_iterator(dir)) {
        LOG_INFO << "Trying migration "sv << e.path();
        if(!e.is_regular_file()) {
            LOG_FATAL << "Had to skip this migration, not a regular file"sv;
            return std::nullopt;
        }
        auto fn = e.path().filename().string();
        auto [tsStr,nameStr] = parse_sql_filename(fn);
        if(tsStr.empty() || nameStr.empty()) {
            LOG_FATAL << "Had to skip this migration, invalid file name format"sv;
            return std::nullopt;
        }
        auto ts = parseTimestamp(tsStr);
        if(!ts) {
            LOG_FATAL << "Had to skip this migration, Could not parse timestamp"sv;
            return std::nullopt;
        }
        out.emplace_back(*ts, std::string(nameStr), fn, e.path());
    }
    std::ranges::sort(out, {}, &MigrationFile::timestamp);
    return out;
}

drogon::Task<bool> applyMigration(drogon::orm::DbClientPtr db, const MigrationFile& f) {
    std::ifstream file(f.fullPath);
    if (!file) {
        LOG_FATAL << "Error opening file: "sv << f.fullPath;
        co_return false;
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    file.close();

    auto sql_commands = split_and_trim(content);

    auto err = co_await WithTransaction(
        [&](auto tx) -> drogon::Task<ScopedTransactionResult> {

            for(const auto& command : sql_commands) {
                co_await tx->execSqlCoro(command);
            }

            Migrations m;
            m.setTimestamp(f.timestamp);
            m.setName(f.name);
            co_await CoroMapper<Migrations>(tx).insert(m);
            co_return std::nullopt;
        });

    if(err) {
        LOG_FATAL << "Could not apply transaction: "sv << *err;
        co_return false;
    }

    co_return true;
}

drogon::Task<bool> applyMigrationFile(drogon::orm::DbClientPtr db, const MigrationFile& f) {
    LOG_INFO << "Applying: "sv << f.filename;

    bool success = co_await applyMigration(db, f);

    if(!success) {
        LOG_FATAL << "Migration failed ["sv << f.name << "]"sv;
        co_return false;
    }

    LOG_INFO << "Migration succeeded: "sv << f.name;
    co_return true;
}

drogon::Task<bool> applyMigrationFiles(drogon::orm::DbClientPtr db) {
    auto latest = co_await CoroMapper<Migrations>(db)
                    .orderBy(Migrations::Cols::_timestamp, SortOrder::DESC)
                    .limit(1)
                    .findAll();

    int64_t lastTs = latest.empty() ? 0 : latest[0].getValueOfTimestamp();

    auto migrationFiles = scanMigrationFiles("db/migrations"sv);

    if(!migrationFiles) {
        LOG_FATAL << "Could not enumerate migrations."sv;
        co_return false;
    } else {
        LOG_INFO << "Found "sv << migrationFiles->size() << " migration files"sv;
    }

    for (const auto &f : *migrationFiles) {
        if(f.timestamp <= lastTs) {
            LOG_INFO << "Skipping: "sv << f.filename;
            continue;
        }
        bool success = co_await applyMigrationFile(db, f);

        if(!success){
            LOG_FATAL << "Could not apply file"sv;
            drogon::app().quit();
            co_return false;
        }
    }

    co_return true;
}

drogon::Task<bool> MigrateDatabase(drogon::orm::DbClientPtr db) {
    bool exists = true;
    try {
        db->execSqlSync("SELECT * FROM migrations"s);
    } catch(std::exception&) {
        exists = false;
    }

    if(!exists) {
        LOG_INFO << "No migrations table; running main.sql"sv;
        auto success = co_await applyMigrationFile(db, {0, "main"s, "main.sql"s, "db/main.sql"sv});
        if(!success) {
            LOG_FATAL << "Could not apply main.sql"sv;
            co_return false;
        }
    }

    auto success = co_await applyMigrationFiles(db);

    if(!success) {
        LOG_FATAL << "Could not migrate db"sv;
        co_return false;
    }

    LOG_INFO << "Migrated db"sv;
    co_return true;
}
