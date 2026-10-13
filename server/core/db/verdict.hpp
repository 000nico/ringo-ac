#pragma once
#include "../models/player_risk.hpp"

namespace db {
    // returns existant PlayerRisk or a new one with score 0
    PLAYER_RISK  get_or_create(const std::string& hwid);
    void        save_player_risk(const PLAYER_RISK& player);
    void        save_evidence(const std::string& hwid, const std::string& signal, float weight, uint64_t timestamp);
    bool has_recent_evidence(const std::string& hwid, const std::string& signal, uint64_t since);
}