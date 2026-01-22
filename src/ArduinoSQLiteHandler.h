//
// Created by armeldemarsac on 1/22/26.
//

#ifndef ARDUINOSQLITE_MAIN_H
#define ARDUINOSQLITE_MAIN_H
#include <vector>

#include "dbTypes.h"
#include "sqlite3.h"


void setupSQLite(const char* databaseName);
sqlite3* createOpenSQLite(const char* databaseName);
void closeSQLiteConnection(sqlite3* sqliteConnection);
bool createSQLTable(sqlite3* sqliteConnection, const char* tableName, const std::vector<DBColumn>& columns);
bool executeSQLTransaction(sqlite3* sqliteConnection, const std::vector<std::string>& sqlStatement);

void testSQLite();


#endif //ARDUINOSQLITE_MAIN_H