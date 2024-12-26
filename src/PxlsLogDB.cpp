//
// PxlsLogDB implementation
//

#include "PxlsLogDB.h"

bool PxlsLogDB::OpenLogRaw(const std::string &filename) {
    if (!std::filesystem::exists(filename) || std::filesystem::is_directory(filename)) return false;
    std::ifstream file(filename);
    auto db_path = std::filesystem::path(filename).replace_extension("logdb").generic_string();
    // delete old logdb and reconstruct it
    if (std::filesystem::exists(db_path) && !std::filesystem::is_directory(db_path))
        std::filesystem::remove(db_path);
    if (sqlite3_open_v2(db_path.c_str(), &log_db,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) != SQLITE_OK) return false;
    // init logdb by creating the log table
    const std::string init_sql  = "CREATE TABLE log("
                            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                            "date TEXT NOT NULL,"
                            "hash TEXT NOT NULL,"
                            "x INTEGER NOT NULL,"
                            "y INTEGER NOT NULL,"
                            "color_index INTEGER NOT NULL,"
                            "action TEXT NOT NULL"
                            ");";
    if (sqlite3_exec(log_db, init_sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
        CloseLogDB();
        std::filesystem::remove(db_path);
        return false;
    }
    std::string record_line;
    std::vector<std::string> record;
    const std::string insert_sql_prefix = "INSERT INTO log(date,hash,x,y,color_index,action) VALUES ";
    std::stringstream sql_ss { insert_sql_prefix };
    unsigned short int record_num = 0;
    while (std::getline(file, record_line)) {
        split(record, record_line, boost::is_any_of("\t"));
        /*
         * record format
         * [date, random_hash, x, y, color_index, action]
         */
        if (record.size() != 6) {
            CloseLogDB();
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
            << "'),";
        // insert INSERT_RECORDS_MAX_COUNT of records a time
        if (++record_num == INSERT_RECORDS_MAX_COUNT) {
            std::string insert_sql(sql_ss.str());
            insert_sql.back() = ';';
            if (sqlite3_exec(log_db, insert_sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
                CloseLogDB();
                std::filesystem::remove(db_path);
                return false;
            }
            record_num = 0;
            sql_ss.str(insert_sql_prefix);
        }
    }
    if (!sql_ss.str().empty()) {
        std::string insert_sql(sql_ss.str());
        insert_sql.back() = ';';
        if (sqlite3_exec(log_db, insert_sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
            CloseLogDB();
            std::filesystem::remove(db_path);
            return false;
        }
    }
    return true;
}

bool PxlsLogDB::OpenLogDB(const std::string &filename) {
    if (!std::filesystem::exists(filename) || std::filesystem::is_directory(filename)) return false;
    if (sqlite3_open_v2(filename.c_str(), &log_db,
        SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) return false;
    return true;
}

void PxlsLogDB::CloseLogDB() {
    if (log_db)
        sqlite3_close(log_db);
    log_db = nullptr;
}

bool PxlsLogDB::QueryDimension(void (*callback)(unsigned width, unsigned height)) const {
    if (!log_db) return false;
    const std::string sql = "SELECT MAX(x),MAX(y) FROM log";
    // pass the callback to the lambda function wrapper and invoke it inside
    if (sqlite3_exec(log_db, sql.c_str(), [](void* cb, int, char **argv, char**) -> int {
        reinterpret_cast<void (*)(unsigned, unsigned)>(cb)(std::stoul(argv[0]) + 1, std::stoul(argv[1]) + 1);
        return 0;
    }, reinterpret_cast<void*>(callback), nullptr) != SQLITE_OK)
        return false;
    return true;
}

PxlsLogDB::~PxlsLogDB() {
    CloseLogDB();
}
