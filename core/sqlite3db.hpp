#pragma once

#include <sqlite3.h>

#include <string>
#include <vector>

// SQLite3 database connection wrapper
class SQLite3DB {
private:
    sqlite3* db = nullptr;
    bool connected = false;
    static SQLite3DB* instance;
    std::string dbPath;

    // Private constructor for singleton pattern
    SQLite3DB();
    bool initializeSchema();

public:
    // Singleton instance getter
    static SQLite3DB* getInstance();

    // Connection management
    bool connect(const std::string& databasePath);
    void disconnect();
    bool isConnected() const;

    // Execute query (INSERT, UPDATE, DELETE)
    bool executeUpdate(const std::string& sql);

    // Execute SELECT query and return results
    std::vector<std::vector<std::string>> executeQuery(const std::string& sql);

    // Destructor
    ~SQLite3DB();

    // Prevent copy
    SQLite3DB(const SQLite3DB&) = delete;
    SQLite3DB& operator=(const SQLite3DB&) = delete;
};
