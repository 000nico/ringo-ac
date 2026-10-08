#pragma once
#include <string>
#include <map>
#include "../../models/reports.hpp"
#include "../../models/player_risk.hpp"

struct Signal {
    float weight; // how much it adds to the risk score
    bool definitive; // immediate ban
};

// this is the interface for the analysis
struct RuleResult {
    Signal signal;
    std::string signal_reason; // ex. "debugger_with_impossible_stats"
    std::string reason; // legible description, ex. "debugger present with KDA > 10"
};

// if didn't found nothing, use this
inline RuleResult no_finding() {
    return { 0, 0, 0};
}   

// 0-39 -> clean
// 40-74 -> flagged
// 75-100 -> ban
static const std::map<std::string, Signal> SIGNAL_WEIGHTS = {
    // weak
    { "hypervisor_detected",          { 15.0f, false } },
    { "test_signing_enabled",         { 10.0f, false } },
    { "debugger_present",             { 20.0f, false } },
    { "big_pool_detected",             { 25.0f, false } },
    
    // mid
    { "byovd_loaded", { 35.0f, false } },
    { "byovd_loaded_and_unloaded_fast", { 45.0f, false } },

    // strong - almost ban with only one of things
    { "byovd_loaded_with_strong_import", { 65.0f, false } },
    { "kernel_thread_outside_valid_module", { 70.0f, false } },
    { "overlay_detected", { 70.0f, false } },

    // immediate ban
    { "hwid_previously_banned", { 100.0f, true } },
    { "game_hash_mismatch",  { 100.0f, true } },
    { "invalid_ioctl_sent",  { 100.0f, true } },
};