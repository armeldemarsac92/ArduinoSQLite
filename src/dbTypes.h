#ifndef ARDUINOSQLITE_DBTYPES_H
#define ARDUINOSQLITE_DBTYPES_H
#include <string>

struct DBColumn {
    std::string name;
    std::string type;
    bool isPrimaryKey = false;
};

#endif //ARDUINOSQLITE_DBTYPES_H