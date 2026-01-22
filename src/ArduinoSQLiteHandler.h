//
// Created by armeldemarsac on 1/22/26.
//

#ifndef ARDUINOSQLITE_MAIN_H
#define ARDUINOSQLITE_MAIN_H
#include <vector>

#include "dbTypes.h"
#include "sqlite3.h"


sqlite3* createOpenSQLConnection(const char* databaseName);
void closeSQLiteConnection(sqlite3* sqliteConnection);
bool createSQLTable(sqlite3* sqliteConnection, const DBTable& table);
bool executeSQLTransaction(sqlite3* sqliteConnection, const std::vector<std::string>& sqlStatement);
std::string buildSQLInsertStatement(const DBTable &table, const std::vector<std::string> &dataToInsert);

void testSQLite();


#endif //ARDUINOSQLITE_MAIN_H