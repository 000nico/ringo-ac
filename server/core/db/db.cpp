#include "db.hpp"
#include <iostream>

std::unique_ptr<SQLite::Database> g_db;

void db_init(const std::string& path) {
    g_db = std::make_unique<SQLite::Database>(path, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);

    g_db->exec(R"(
        CREATE TABLE IF NOT EXISTS environment_reports (
            id                   INTEGER PRIMARY KEY AUTOINCREMENT,
            hwid                 TEXT NOT NULL,
            timestamp            INTEGER NOT NULL,
            debugger_present     INTEGER NOT NULL,
            hypervisor_detected  INTEGER NOT NULL,
            secure_boot_enabled  INTEGER NOT NULL,
            hvci_enabled         INTEGER NOT NULL,
            test_signing_enabled INTEGER NOT NULL,
            vbs_enabled          INTEGER NOT NULL
        );
    )");

    g_db->exec(R"(
        CREATE TABLE IF NOT EXISTS module_reports (
            id        INTEGER PRIMARY KEY AUTOINCREMENT,
            hwid      TEXT    NOT NULL,
            timestamp INTEGER NOT NULL,
            payload   TEXT    NOT NULL
        );
    )");

    g_db->exec(R"(
        CREATE TABLE IF NOT EXISTS player_risk (
            hwid         TEXT PRIMARY KEY,
            risk_score   REAL    NOT NULL DEFAULT 0,
            status       INTEGER NOT NULL DEFAULT 0,
            ban_reason   TEXT    NOT NULL DEFAULT '',
            last_updated INTEGER NOT NULL DEFAULT 0,
            first_seen   INTEGER NOT NULL DEFAULT 0
        );
    )");

    g_db->exec(R"(
        CREATE TABLE IF NOT EXISTS evidence (
            id        INTEGER PRIMARY KEY AUTOINCREMENT,
            hwid      TEXT    NOT NULL,
            signal    TEXT    NOT NULL,
            weight    REAL    NOT NULL,
            timestamp INTEGER NOT NULL
        );
    )");

    std::cout << "[db] initialized at " << path << "\n";
}
