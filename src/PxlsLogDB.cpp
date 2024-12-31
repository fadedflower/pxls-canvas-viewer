//
// PxlsLogDB implementation
//

#include "PxlsLogDB.h"

bool PxlsLogDB::OpenLogRaw(const std::string &filename) {
    if (!std::filesystem::exists(filename) || std::filesystem::is_directory(filename)) return false;
    std::ifstream file(filename);
    auto db_path = std::filesystem::path(filename).replace_extension("logdb").string();
    // delete old logdb and reconstruct it
    if (std::filesystem::exists(db_path) && !std::filesystem::is_directory(db_path))
        std::filesystem::remove(db_path);
    sqlite3 *new_log_db = nullptr;
    if (sqlite3_open_v2(db_path.c_str(), &new_log_db,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) != SQLITE_OK) return false;
    // enable foreign keys
    if (sqlite3_exec(new_log_db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr) != SQLITE_OK) {
        sqlite3_close(new_log_db);
        std::filesystem::remove(db_path);
        return false;
    }
    // init logdb by creating the log table
    const std::string init_sql  = "CREATE TABLE log("
                            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                            "prev_id INTEGER,"
                            "date TEXT NOT NULL,"
                            "hash TEXT NOT NULL,"
                            "x INTEGER NOT NULL,"
                            "y INTEGER NOT NULL,"
                            "color_index INTEGER NOT NULL,"
                            "action TEXT NOT NULL,"
                            "FOREIGN KEY (prev_id) REFERENCES log(id)"
                            ");";
    if (sqlite3_exec(new_log_db, init_sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
        sqlite3_close(new_log_db);
        std::filesystem::remove(db_path);
        return false;
    }
    // store previous record id
    std::map<std::pair<unsigned, unsigned>, unsigned long> prev_id_map;
    std::string record_line;
    std::vector<std::string> record;
    const std::string insert_sql_prefix = "INSERT INTO log(date,hash,x,y,color_index,action,prev_id) VALUES ";
    std::stringstream sql_ss;
    sql_ss << insert_sql_prefix;
    unsigned long record_id = 1;
    unsigned short record_num = 0;
    unsigned record_x, record_y;
    while (std::getline(file, record_line)) {
        split(record, record_line, boost::is_any_of("\t"));
        /*
         * record format
         * [date, random_hash, x, y, color_index, action]
         */
        if (record.size() != 6) {
            sqlite3_close(new_log_db);
            std::filesystem::remove(db_path);
            return false;
        }
        record[0][record[0].rfind(',')] = '.'; // convert date to compatible format
        // construct insert values
        sql_ss << "('" << record[0]
            << "','" << record[1]
            << "'," << record[2]
            << ',' << record[3]
            << ',' << record[4]
            << ",'" << record[5]
            << "',";
        // fetch
        try {
            record_x = std::stoul(record[2]);
            record_y = std::stoul(record[3]);
        }
        catch (std::invalid_argument&) {
            sqlite3_close(new_log_db);
            std::filesystem::remove(db_path);
            return false;
        }
        if (prev_id_map.contains(std::make_pair(record_x, record_y)))
            sql_ss << prev_id_map[std::make_pair(record_x, record_y)];
        else
            sql_ss << "NULL";
        sql_ss << "),";
        // update prev_id_map
        prev_id_map[std::make_pair(record_x, record_y)] = record_id++;
        // insert INSERT_RECORDS_MAX_COUNT of records a time
        if (++record_num == INSERT_RECORDS_MAX_COUNT) {
            std::string insert_sql { sql_ss.str() };
            insert_sql.back() = ';';
            if (sqlite3_exec(new_log_db, insert_sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
                sqlite3_close(new_log_db);
                std::filesystem::remove(db_path);
                return false;
            }
            record_num = 0;
            sql_ss.str("");
            sql_ss << insert_sql_prefix;
        }
    }
    if (!sql_ss.str().empty()) {
        std::string insert_sql { sql_ss.str() };
        insert_sql.back() = ';';
        if (sqlite3_exec(new_log_db, insert_sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
            sqlite3_close(new_log_db);
            std::filesystem::remove(db_path);
            return false;
        }
    }
    CloseLogDB();
    log_db = new_log_db;
    if (!QueryLogDBMetadata()) {
        CloseLogDB();
        return false;
    }
    return true;
}

bool PxlsLogDB::OpenLogDB(const std::string &filename) {
    if (!std::filesystem::exists(filename) || std::filesystem::is_directory(filename)) return false;
    sqlite3 *new_log_db = nullptr;
    if (sqlite3_open_v2(filename.c_str(), &new_log_db,
        SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) return false;
    // enable foreign keys
    if (sqlite3_exec(new_log_db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr) != SQLITE_OK) {
        sqlite3_close(new_log_db);
        return false;
    }
    CloseLogDB();
    log_db = new_log_db;
    if (!QueryLogDBMetadata()) {
        CloseLogDB();
        return false;
    }
    return true;
}

void PxlsLogDB::CloseLogDB() {
    if (log_db)
        sqlite3_close(log_db);
    log_db = nullptr;
    current_id = 0;
    db_width = db_height = 0;
    db_record_count = 0ul;
}

bool PxlsLogDB::QueryLogDBMetadata() {
    if (!log_db) return false;
    const std::string sql = "SELECT MAX(x),MAX(y),COUNT(*) FROM log";
    if (sqlite3_exec(log_db, sql.c_str(), [](void* db_ptr, int, char **argv, char**) -> int {
        auto *db = static_cast<PxlsLogDB*>(db_ptr);
        db->db_width = std::stoul(argv[0]) + 1;
        db->db_height = std::stoul(argv[1]) + 1;
        db->db_record_count = std::stoul(argv[2]);
        return 0;
    }, this, nullptr) != SQLITE_OK)
        return false;
    return true;
}

bool PxlsLogDB::QueryRecords(unsigned long dest_id, RecordQueryCallback callback) {

    if (!log_db || dest_id > db_record_count) return false;
    if (callback == nullptr || dest_id == current_id) {
        current_id = dest_id;
        return true;
    }
    if (dest_id > current_id) {
        const std::string sql = std::format("SELECT date,hash,x,y,color_index,action "
                                    "FROM log WHERE id > {} and id <= {};", current_id, dest_id);
        if (sqlite3_exec(log_db, sql.c_str(), [](void* cb, int, char **argv, char**) -> int {
            (*static_cast<RecordQueryCallback*>(cb))(
                argv[0],
                argv[1],
                std::stoul(argv[2]),
                std::stoul(argv[3]),
                std::stoul(argv[4]),
                argv[5],
                FORWARD
            );
            return 0;
        }, &callback, nullptr) != SQLITE_OK)
            return false;
    } else {
        const std::string sql = std::format("SELECT prev_log.date,prev_log.hash,cur_log.x,cur_log.y,prev_log.color_index,prev_log.action "
                                    "FROM log cur_log LEFT JOIN log prev_log ON cur_log.prev_id = prev_log.id "
                                    "WHERE cur_log.id > {} and cur_log.id <= {} ORDER BY cur_log.id DESC;", dest_id, current_id);
        if (sqlite3_exec(log_db, sql.c_str(), [](void* cb, int, char **argv, char**) -> int {
            (*static_cast<RecordQueryCallback*>(cb))(
                argv[0] ? std::make_optional(argv[0]) : std::nullopt,
                argv[1] ? std::make_optional(argv[1]) : std::nullopt,
                std::stoul(argv[2]),
                std::stoul(argv[3]),
                argv[4] ? std::make_optional(std::stoul(argv[4])) : std::nullopt,
                argv[5] ? std::make_optional(argv[5]) : std::nullopt,
                BACKWARD
            );
            return 0;
        }, &callback, nullptr) != SQLITE_OK)
            return false;
    }
    current_id = dest_id;
    return true;
}

bool PxlsLogDB::Seek(const unsigned long id) {
    if (!log_db || id > db_record_count) return false;
    current_id = id;
    return true;
}

PxlsLogDB::~PxlsLogDB() {
    CloseLogDB();
}
