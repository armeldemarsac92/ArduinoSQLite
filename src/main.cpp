#include <Arduino.h>

#include "ArduinoSQLite.hpp"
#include "MemoryInfo.hpp"

#include <SD.h>

namespace memInfo = halvoe::memoryInfo;

const char* dbName = "test.db";
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

void printMemoryInfo()
{
  Serial.printf("getUsedStackInBytes(): %d\n", memInfo::getUsedStackInBytes());
  Serial.printf("getUsedHeapInBytes(): %d\n", memInfo::getUsedHeapInBytes());
  //Serial.printf("getDynamicUsedPsramInBytes(): %d\n", memInfo::getDynamicUsedPsramInBytes()); // expensive opration
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

void setup()
{
  setupSerial(115200);

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

void loop()
{
  // nothing to do here
}

