// src/db/verdicts.cpp
#include "verdict.hpp"
#include "db.hpp"
#include <ctime>

namespace db {

PLAYER_RISK get_or_create(const std::string& hwid) {
    SQLite::Statement stmt(*g_db, "SELECT * FROM player_risk WHERE hwid = ?");
    stmt.bind(1, hwid);

    if (stmt.executeStep()) {
        PLAYER_RISK p;
        p.hwid         = stmt.getColumn(0).getString();
        p.risk_score   = (float)stmt.getColumn(1).getDouble();
        p.status       = (VERDICT_STATUS)stmt.getColumn(2).getInt();
        p.ban_reason   = stmt.getColumn(3).getString();
        p.last_updated = (uint64_t)stmt.getColumn(4).getInt64();
        p.first_seen   = (uint64_t)stmt.getColumn(5).getInt64();
        return p;
    }

    PLAYER_RISK p;
    p.hwid         = hwid;
    p.risk_score   = 0.0f;
    p.status       = VERDICT_STATUS::CLEAN;
    p.ban_reason   = "";
    p.last_updated = (uint64_t)time(nullptr);
    p.first_seen   = (uint64_t)time(nullptr);
    return p;
}

void save_player_risk(const PLAYER_RISK& p) {
    SQLite::Statement stmt(*g_db, R"(
        INSERT INTO player_risk (hwid, risk_score, status, ban_reason, last_updated, first_seen)
        VALUES (?, ?, ?, ?, ?, ?)
        ON CONFLICT(hwid) DO UPDATE SET
            risk_score   = excluded.risk_score,
            status       = excluded.status,
            ban_reason   = excluded.ban_reason,
            last_updated = excluded.last_updated
    )");
    stmt.bind(1, p.hwid);
    stmt.bind(2, (double)p.risk_score);
    stmt.bind(3, (int)p.status);
    stmt.bind(4, p.ban_reason);
    stmt.bind(5, (int64_t)p.last_updated);
    stmt.bind(6, (int64_t)p.first_seen);
    stmt.exec();
}

void save_evidence(const std::string& hwid, const std::string& signal, float weight, uint64_t timestamp) {
    SQLite::Statement stmt(*g_db, R"(
        INSERT INTO evidence (hwid, signal, weight, timestamp)
        VALUES (?, ?, ?, ?)
    )");
    stmt.bind(1, hwid);
    stmt.bind(2, signal);
    stmt.bind(3, (double)weight);
    stmt.bind(4, (int64_t)timestamp);
    stmt.exec();
}

} 