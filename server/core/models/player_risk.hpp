#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "verdict.hpp"

struct EVIDENCE_ENTRY {
    std::string signal; // "debugger_detected", "unsigned_module", etc
    float weight; // how much is added to the score
    uint64_t timestamp; // unix timestamp
};

struct PLAYER_RISK {
    std::string hwid;
    float risk_score;      // accumulated
    VERDICT_STATUS status;
    std::string ban_reason;      // 0 if not banned
    uint64_t last_updated;    // to calculate decay
    uint64_t first_seen;      // first report 
    std::vector<EVIDENCE_ENTRY> evidence; // historial de señales
};