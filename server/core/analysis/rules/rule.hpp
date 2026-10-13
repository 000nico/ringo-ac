#pragma once
#include <string>
#include <map>
#include "../../models/reports.hpp"
#include "../../models/player_risk.hpp"

struct SIGNAL {
    float weight; // how much it adds to the risk score
    bool definitive; // immediate ban
};

// this is the interface for the analysis
struct RULE_RESULT {
    SIGNAL signal;
    std::string signal_reason; // ex. "debugger_with_impossible_stats"
    std::string reason; // legible description, ex. "debugger present with KDA > 10"
};

// if didn't found nothing, use this
inline RULE_RESULT no_finding() {
    return {};
}   

// 0-39 -> clean
// 40-74 -> flagged
// 75-100 -> ban
static const std::map<std::string, SIGNAL> SIGNAL_WEIGHTS = {
    // weak
    { "hypervisor_detected",          { 15.0f, false } },
    { "debugger_present",             { 20.0f, false } },
    
    // mid
    { "byovd_loaded", { 35.0f, false } },
    { "big_pool_detected", { 35.0f, false } },

    // strong - almost ban with only one of things
    { "kernel_thread_outside_valid_module", { 70.0f, false } },
    { "thread_inside_big_pool", { 70.0f, false } },
    { "spoofing_detection", {70.0f, false } },
    { "unsigned_dll", {70.0f, false} },
    { "remote_thread", {70.0f, false}},
    
    // immediate ban
    { "test_signing_enabled",         { 10.0f, true } },
    { "hwid_previously_banned", { 100.0f, true } },
    { "game_hash_mismatch",  { 100.0f, true } },
    { "invalid_ioctl_sent",  { 100.0f, true } },
};