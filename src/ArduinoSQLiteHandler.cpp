#include <Arduino.h>

#include "ArduinoSQLite.hpp"
#include "MemoryInfo.hpp"

#include <SD.h>
#include <stdexcept>
#include <string>
#include "dbTypes.h"

namespace memInfo = halvoe::memoryInfo;

const char* dbName;
const char* dbJournalName = "test.db-journal";

void setupSerial(long in_serialBaudrate, unsigned long in_timeoutInSeconds = 15)
{
  Serial.begin(in_serialBaudrate);

  unsigned long timeoutInMilliseconds = in_timeoutInSeconds * 1000;
  elapsedMillis initialisationTime;
  while (not Serial && initialisationTime < timeoutInMilliseconds);

  if (Serial)
  {
    Serial.println();
    Serial.println("Serial logging is ready (initialisationTime: " + String(initialisationTime) + " ms)");
  }
}

void errorLogCallback(void* pArg, int iErrCode, const char* zMsg)
{
  Serial.printf("(%d) %s\n", iErrCode, zMsg);
}

void checkSQLiteError(sqlite3* in_db, int in_rc)
{
  if (in_rc == SQLITE_OK)
  {
    Serial.println(">>>> testSQLite - operation - success <<<<");
  }
  else
  {
    int ext_rc = sqlite3_extended_errcode(in_db);
    Serial.print(ext_rc);
    Serial.print(": ");
    Serial.println(sqlite3_errstr(ext_rc));
  }
}

std::string buildSQLInsertStatement(const DBTable &table, const std::vector<std::string> &dataToInsert) {

  int expectedColumns = 0;
  for(const auto& col : table.columns) {
    if (!col.isPrimaryKey) expectedColumns++;
  }

  if (expectedColumns != dataToInsert.size()) {
    Serial.printf("Error: Table has %d insertable columns, but you provided %d values.\n", expectedColumns, dataToInsert.size());
    return "";
  }

  std::string sql = "INSERT INTO " + table.tableName + " (";
  std::string values = "VALUES (";

  int dataIndex = 0;

  for (size_t i = 0; i < table.columns.size(); i++) {
    if (table.columns[i].isPrimaryKey) {
      continue;
    }

    sql += table.columns[i].name;

    if (table.columns[i].type.find("TEXT") != std::string::npos) {
      values += "'" + dataToInsert[dataIndex] + "'";
    } else {
      values += dataToInsert[dataIndex];
    }

    bool isLast = (dataIndex == dataToInsert.size() - 1);
    if (!isLast) {
      sql += ", ";
      values += ", ";
    }

    dataIndex++;
  }

  sql += ") " + values + ");";

  return sql;
}

void printMemoryInfo()
{
  Serial.printf("getUsedStackInBytes(): %d\n", memInfo::getUsedStackInBytes());
  Serial.printf("getUsedHeapInBytes(): %d\n", memInfo::getUsedHeapInBytes());
  //Serial.printf("getDynamicUsedPsramInBytes(): %d\n", memInfo::getDynamicUsedPsramInBytes()); // expensive opration
}

sqlite3* createOpenSQLConnection(const char* dbName) {
  sqlite3* sqliteConnection;
  Serial.println("---- testSQLite - sqlite3_open - begin ----");
  int connectionResult = sqlite3_open(dbName, &sqliteConnection);
  checkSQLiteError(sqliteConnection, connectionResult);
  printMemoryInfo();
  Serial.println("---- testSQLite - sqlite3_open - end ----");
  return sqliteConnection;
}

void closeSQLiteConnection(sqlite3* sqliteConnection) {
  Serial.println("---- testSQLite - sqlite3_close - begin ----");
  sqlite3_close(sqliteConnection);
  Serial.println("---- testSQLite - sqlite3_close - end ----");
}

bool createSQLTable(sqlite3* sqliteConnection, const DBTable& table) {
  Serial.println("---- creating sql table - begin ----");

  const char* tableName = table.tableName.c_str();

  const std::vector<DBColumn>& cols = table.columns;

  std::string sqlStatement = "CREATE TABLE IF NOT EXISTS ";
  sqlStatement += tableName;
  sqlStatement += " (";

  for (size_t i = 0; i < table.columns.size(); i++) {
    sqlStatement += cols[i].name;
    sqlStatement += " ";
    sqlStatement += cols[i].type;

    if (i < cols.size() - 1) {
      sqlStatement += ", ";
    }
  }

  sqlStatement += ");";

  Serial.print("Executing: ");
  Serial.println(sqlStatement.c_str());

  char* errMsg = nullptr;
  int commandResult = sqlite3_exec(sqliteConnection, sqlStatement.c_str(), NULL, NULL, &errMsg);

  if (commandResult != SQLITE_OK) {
    Serial.printf("SQL Error: %s\n", errMsg);
    sqlite3_free(errMsg);
    Serial.println("---- failed creating sql table - end ----");
    return false;
  }

  Serial.println("---- success creating sql table - end ----");
  return true;

}

bool executeSQLTransaction(sqlite3* sqliteConnection, const std::vector<std::string>& sqlStatement) {
  Serial.println("---- preparing sql transaction - begin ----");

  char* errMsg = nullptr;
  int commandResult = sqlite3_exec(sqliteConnection, "BEGIN TRANSACTION;", NULL, NULL, &errMsg);

  if (commandResult != SQLITE_OK) {
    Serial.printf("SQL Error: %s\n", errMsg);
    sqlite3_free(errMsg);
    Serial.println("---- failed preparing sql transaction - end ----");
    return false;
  }

  for (size_t i = 0; i < sqlStatement.size(); i++) {
    Serial.println("---- executing sql satement ----");

    commandResult = sqlite3_exec(sqliteConnection, sqlStatement[i].c_str(), NULL, NULL, &errMsg);

    if (commandResult != SQLITE_OK) {
      Serial.printf("SQL Error: %s\n", errMsg);
      sqlite3_free(errMsg);
      Serial.println("---- failed preparing sql transaction - end ----");
      sqlite3_exec(sqliteConnection, "ROLLBACK;", NULL, NULL, NULL);
      return false;
    }
  }

  commandResult = sqlite3_exec(sqliteConnection, "COMMIT;", NULL, NULL, &errMsg);

  if (commandResult != SQLITE_OK) {
    Serial.printf("SQL Error: %s\n", errMsg);
    sqlite3_free(errMsg);
    Serial.println("---- failed preparing sql transaction - end ----");
    sqlite3_exec(sqliteConnection, "ROLLBACK;", NULL, NULL, NULL);
    return false;
  }

  Serial.println("---- success executing sql transaction - end ----");
  return true;
}

void testSQLite()
{
  sqlite3* db;
  Serial.println("---- testSQLite - sqlite3_open - begin ----");
  int rc = sqlite3_open("test.db", &db);
  checkSQLiteError(db, rc);
  printMemoryInfo();
  Serial.println("---- testSQLite - sqlite3_open - end ----");

  if (rc == SQLITE_OK)
  {
    Serial.println("---- testSQLite - sqlite3_exec - begin ----");
    const char* sqlCreateTable = "CREATE TABLE Persons(PersonID INT);";
    Serial.print("SQL to exec: \"");
    Serial.print(sqlCreateTable);
    Serial.println("\"");
    rc = sqlite3_exec(db, sqlCreateTable, NULL, 0, NULL);
    checkSQLiteError(db, rc);
    printMemoryInfo();
    Serial.println("---- testSQLite - sqlite3_exec - end ----");

    Serial.println("---- testSQLite - sqlite3_exec - begin ----");
    const char* sqlInsert = "INSERT INTO Persons (PersonID) VALUES (127);";
    Serial.print("SQL to exec: \"");
    Serial.print(sqlInsert);
    Serial.println("\"");
    rc = sqlite3_exec(db, sqlInsert, NULL, 0, NULL);
    checkSQLiteError(db, rc);
    printMemoryInfo();
    Serial.println("---- testSQLite - sqlite3_exec - end ----");

    Serial.println("---- testSQLite - sqlite3_prepare_v2 - begin ----");
    const char* sqlSelect = "SELECT * FROM Persons;";
    Serial.print("SQL to prepare: \"");
    Serial.print(sqlSelect);
    Serial.println("\"");
    sqlite3_stmt* stmt;
    rc = sqlite3_prepare_v2(db, "SELECT * FROM Persons;", -1, &stmt, 0); // create SQL statement
    checkSQLiteError(db, rc);
    printMemoryInfo();
    Serial.println("---- testSQLite - sqlite3_prepare_v2 - end ----");
    Serial.println("---- testSQLite - sqlite3_step - begin ----");
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
      Serial.print("Result from SQL (");
      Serial.print(sqlSelect);
      Serial.println("): ");
      Serial.println(sqlite3_column_int(stmt, 0));
    }
    else
    {
      checkSQLiteError(db, rc);
    }
    printMemoryInfo();
    Serial.println("---- testSQLite - sqlite3_step - end ----");
    Serial.println("---- testSQLite - sqlite3_finalize - begin ----");
    rc = sqlite3_finalize(stmt);
    checkSQLiteError(db, rc);
    printMemoryInfo();
    Serial.println("---- testSQLite - sqlite3_finalize - end ----");
  }

  Serial.println("---- testSQLite - sqlite3_close - begin ----");
  rc = sqlite3_close(db);
  checkSQLiteError(db, rc);
  printMemoryInfo();
  Serial.println("---- testSQLite - sqlite3_close - end ----");
}

void setupSQLite(const char* dbName)
{
  setupSerial(115200);

  std::string journalPath = std::string(dbName) + "-journal";

  if (CrashReport)
  {
    Serial.println(CrashReport);
  }

  Serial.print("SQLite Version: ");
  Serial.println(SQLITE_VERSION);

  printMemoryInfo();

  if (not SD.begin(BUILTIN_SDCARD))
  {
    Serial.println("SD.begin() failed! - Halting!");
    while (true) { delay(1000); }
  }

  if (SD.exists(dbName)) { if (not SD.remove(dbName)) { Serial.printf("Remove %s failed!\n", dbName); } }
  if (SD.exists(dbJournalName)) { if (not SD.remove(dbJournalName)) { Serial.printf("Remove %s failed!\n", dbJournalName); } }

  T41SQLite::getInstance().setLogCallback(errorLogCallback);
  int resultBegin = T41SQLite::getInstance().begin(&SD, false);

  if (resultBegin == SQLITE_OK)
  {
    Serial.println("T41SQLite::getInstance().begin() succeded!");
    printMemoryInfo();

    testSQLite();

    int resultEnd = T41SQLite::getInstance().end();

    if (resultEnd == SQLITE_OK)
    {
      Serial.println("T41SQLite::getInstance().end() succeded!");
    }
    else
    {
      Serial.print("T41SQLite::getInstance().end() failed! result code: ");
      Serial.println(resultEnd);
    }

    printMemoryInfo();
  }
  else
  {
    Serial.println("T41SQLite::getInstance().begin() failed!");
  }
}


