//
// Provide classes and methods to convert pxls log to sqlite-based database and query the database
//

#ifndef PXLSLOGDB_H
#define PXLSLOGDB_H
#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <format>
#include <map>
#include <utility>
#include <optional>
#include <sqlite3.h>
#include <boost/algorithm/string.hpp>

enum QueryDirection { FORWARD, BACKWARD };
using record_query_callback = std::function<void (std::optional<std::string> date, std::optional<std::string> hash,
        unsigned x, unsigned y, std::optional<unsigned> color_index, std::optional<std::string> action, QueryDirection direction)>;

class PxlsLogDB {
public:
    // open pxls log and convert it to logdb
    bool OpenLogRaw(const std::string &filename);
    // open existing logdb
    bool OpenLogDB(const std::string &filename);
    // close logdb
    void CloseLogDB();
    // methods for getting logdb metadata
    unsigned Width() const { return db_width; }
    unsigned Height() const { return db_height; }
    unsigned long RecordCount() const { return db_record_count; }
    // query records, from current_id to dest_id. when querying backwards, it queries the record with prev_id instead
    bool QueryRecords(unsigned long dest_id, record_query_callback callback);
    // adjust current id pointer
    bool Seek(unsigned long id);
    // get current id pointer
    unsigned long Seek() const { return current_id; }
    ~PxlsLogDB();
private:
    bool QueryLogDBMetadata();
    sqlite3 *log_db = nullptr;
    // maximum count of records inserted a time
    const unsigned short INSERT_RECORDS_MAX_COUNT = 150;
    unsigned long current_id = 0;
    // dimension based on maximum x coordinate and y coordinate
    unsigned db_width { 0 }, db_height { 0 };
    // record count
    unsigned long db_record_count { 0 };
};

#endif //PXLSLOGDB_H
