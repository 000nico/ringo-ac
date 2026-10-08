#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "verdict.hpp"

struct EvidenceEntry {
    std::string signal; // "debugger_detected", "unsigned_module", etc
    float weight; // how much is added to the score
    uint64_t timestamp; // unix timestamp
};

struct PlayerRisk {
    std::string hwid;
    float risk_score;      // accumulated
    VerdictStatus status;
    std::string ban_reason;      // 0 if not banned
    uint64_t last_updated;    // to calculate decay
    uint64_t first_seen;      // first report 
    std::vector<EvidenceEntry> evidence; // historial de señales
};