#include "sqlite3db.hpp"

#include <iostream>
#include <string>
#include <vector>

SQLite3DB* SQLite3DB::instance = nullptr;

SQLite3DB::SQLite3DB() {
}

SQLite3DB* SQLite3DB::getInstance() {
    if (instance == nullptr) {
        instance = new SQLite3DB();
    }
    return instance;
}

bool SQLite3DB::connect(const std::string& databasePath) {
    try {
        dbPath = databasePath;

        // Open or create database file
        int rc = sqlite3_open(databasePath.c_str(), &db);

        if (rc) {
            std::cout << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
            return false;
        }

        // Enable foreign keys
        char* errMsg = nullptr;
        rc = sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, &errMsg);
        if (rc != SQLITE_OK) {
            std::cout << "Failed to enable foreign keys: " << errMsg << std::endl;
            sqlite3_free(errMsg);
            return false;
        }

        // Initialize database schema
        if (!initializeSchema()) {
            std::cout << "Failed to initialize database schema." << std::endl;
            sqlite3_close(db);
            db = nullptr;
            return false;
        }

        connected = true;
        return true;
    }
    catch (const std::exception& e) {
        std::cout << "Connection Error: " << e.what() << std::endl;
        return false;
    }
}

bool SQLite3DB::initializeSchema() {
    const char* schema = R"(
        CREATE TABLE IF NOT EXISTS flights (
            id TEXT PRIMARY KEY,
            airline TEXT NOT NULL,
            origin TEXT NOT NULL,
            destination TEXT NOT NULL,
            departure_time TEXT NOT NULL,
            arrival_time TEXT NOT NULL,
            terminal TEXT,
            gate TEXT,
            total_seats INTEGER NOT NULL,
            available_seats INTEGER NOT NULL,
            ticket_price REAL NOT NULL,
            status TEXT DEFAULT 'Scheduled'
        );

        CREATE TABLE IF NOT EXISTS seats (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            flight_id TEXT NOT NULL,
            row_number INTEGER NOT NULL,
            letter TEXT NOT NULL,
            cabin TEXT NOT NULL,
            is_booked INTEGER DEFAULT 0,
            booked_by TEXT,
            booked_time TEXT,
            FOREIGN KEY(flight_id) REFERENCES flights(id) ON DELETE CASCADE,
            UNIQUE(flight_id, row_number, letter)
        );

        CREATE TABLE IF NOT EXISTS admin_users (
            username TEXT PRIMARY KEY,
            account_name TEXT NOT NULL,
            password_hash TEXT NOT NULL,
            password_salt TEXT NOT NULL,
            phone_number TEXT,
            gender CHAR(1) DEFAULT NULL,
            address TEXT,
            email TEXT,
            department TEXT,
            admin_level TEXT,
            hire_date TEXT,
            nok_name TEXT,
            nok_relationship TEXT,
            nok_phone TEXT,
            nok_address TEXT
        );

        CREATE TABLE IF NOT EXISTS customer_users (
            username TEXT PRIMARY KEY,
            account_name TEXT NOT NULL,
            password_hash TEXT NOT NULL,
            password_salt TEXT NOT NULL,
            phone_number TEXT,
            gender CHAR(1) DEFAULT NULL,
            address TEXT,
            email TEXT,
            total_bookings INTEGER DEFAULT 0,
            loyalty_points REAL DEFAULT 0.0,
            account_creation_date TEXT,
            account_balance REAL DEFAULT 0.0,
            nok_name TEXT,
            nok_relationship TEXT,
            nok_phone TEXT,
            nok_address TEXT
        );

        CREATE TABLE IF NOT EXISTS customer_transaction_history (
            username TEXT NOT NULL,
            transaction_text TEXT NOT NULL,
            FOREIGN KEY (username) REFERENCES customer_users(username) ON DELETE CASCADE
        );

        CREATE TABLE IF NOT EXISTS customer_booked_flights (
            username TEXT NOT NULL,
            flight_id TEXT NOT NULL,
            FOREIGN KEY (username) REFERENCES customer_users(username) ON DELETE CASCADE
        );
    )";

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, schema, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cout << "Failed to initialize schema: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }

    // Migration: Add booked_time column if it doesn't exist (for existing databases)
    const char* migration = "ALTER TABLE seats ADD COLUMN booked_time TEXT;";
    errMsg = nullptr;
    rc = sqlite3_exec(db, migration, nullptr, nullptr, &errMsg);
    // Ignore error if column already exists (SQLITE_ERROR with "duplicate column name")
    if (rc != SQLITE_OK && std::string(errMsg).find("duplicate column name") == std::string::npos) {
        std::cout << "Migration warning (non-critical): " << errMsg << std::endl;
        sqlite3_free(errMsg);
    } else if (errMsg) {
        sqlite3_free(errMsg);
    }

    return true;
}

void SQLite3DB::disconnect() {
    if (db) {
        sqlite3_close(db);
        db = nullptr;
    }
    connected = false;
}

bool SQLite3DB::isConnected() const {
    return connected;
}

bool SQLite3DB::executeUpdate(const std::string& sql) {
    if (!isConnected()) {
        std::cout << "Database is not connected." << std::endl;
        return false;
    }

    try {
        char* errMsg = nullptr;
        int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);

        if (rc != SQLITE_OK) {
            std::cout << "SQL execution failed: " << errMsg << std::endl;
            std::cout << "Query: " << sql << std::endl;
            sqlite3_free(errMsg);
            return false;
        }

        return true;
    }
    catch (const std::exception& e) {
        std::cout << "SQL Error: " << e.what() << std::endl;
        return false;
    }
}

std::vector<std::vector<std::string>> SQLite3DB::executeQuery(const std::string& sql) {
    std::vector<std::vector<std::string>> results;

    if (!isConnected()) {
        std::cout << "Database is not connected." << std::endl;
        return results;
    }

    try {
        sqlite3_stmt* stmt = nullptr;
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);

        if (rc != SQLITE_OK) {
            std::cout << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
            return results;
        }

        // Execute query and fetch rows
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::vector<std::string> row;
            int colCount = sqlite3_column_count(stmt);

            for (int i = 0; i < colCount; ++i) {
                const unsigned char* value = sqlite3_column_text(stmt, i);
                if (value) {
                    row.push_back(std::string(reinterpret_cast<const char*>(value)));
                }
                else {
                    row.push_back("");
                }
            }

            results.push_back(row);
        }

        sqlite3_finalize(stmt);
        return results;
    }
    catch (const std::exception& e) {
        std::cout << "SQL Query Error: " << e.what() << std::endl;
        return results;
    }
}

SQLite3DB::~SQLite3DB() {
    disconnect();
}
