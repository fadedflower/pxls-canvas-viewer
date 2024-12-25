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
#include <sqlite3.h>
#include <boost/algorithm/string.hpp>

class PxlsLogDB {
public:
  //open pxls log and convert it to logdb
  bool OpenLogRaw(const std::string &filename);
  //open existing logdb
  bool OpenLogDB(const std::string &filename);
  //close logdb
  void CloseLogDB();
  ~PxlsLogDB();
private:
  sqlite3 *log_db = nullptr;
  // maximum count of insert records buffered at the same time
  const unsigned short INSERT_RECORDS_MAX_COUNT = 150;
};

#endif //PXLSLOGDB_H
