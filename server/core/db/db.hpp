#pragma once
#include <SQLiteCpp/SQLiteCpp.h>
#include <memory>

// global instance of db
extern std::unique_ptr<SQLite::Database> g_db;

void db_init(const std::string& path);