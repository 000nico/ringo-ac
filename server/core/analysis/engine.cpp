#include "engine.hpp"
#include "../db/verdict.hpp"
#include <cmath>
#include <ctime>
#include "core/analysis/rules/manual_mapped_drivers/manual_mapped_drivers.hpp"
#include "core/analysis/rules/byovd/byovd.hpp"
#include "rules/integrity/integrity.hpp"
#include "rules/system/system.hpp"
#include <vector>

using Evidence = std::vector<EVIDENCE_ENTRY>;
static const float DECAY_PER_DAY = 0.9f;

RULE_RESULT found(const std::string& signal, const std::string& reason) {
    return { SIGNAL_WEIGHTS.at(signal), signal, reason };
}

static float decayed(float score, uint64_t last_updated, uint64_t now) {
    if (now <= last_updated) return score;
    double days = (double)(now - last_updated) / 86400.0;
    return score * (float)std::pow(DECAY_PER_DAY, days);
}

static void apply_results(const std::string& hwid, const std::vector<RULE_RESULT>& results) {
    PLAYER_RISK p = db::get_or_create(hwid);
    uint64_t now = (uint64_t)time(nullptr);

    // lower the old score
    p.risk_score = decayed(p.risk_score, p.last_updated, now);

    static const uint64_t DEDUP_WINDOW = 24 * 3600;   // same finding counts once every 24 h

    for (const auto& res : results) {
        if (res.signal.weight == 0) continue;   // no_finding

        // a ban is applied even if the signal was already counted
        if (res.signal.definitive) {
            p.status = VERDICT_STATUS::BANNED;
            p.ban_reason = res.reason;
        }

        // already counted recently: don't add again
        if (db::has_recent_evidence(hwid, res.reason, now - DEDUP_WINDOW))
            continue;

        p.risk_score += res.signal.weight;
        db::save_evidence(hwid, res.reason, res.signal.weight, now);
    }

    // status with the score
    if (p.status != VERDICT_STATUS::BANNED) {
        if (p.risk_score >= 75)      p.status = VERDICT_STATUS::BANNED;
        else if (p.risk_score >= 40) p.status = VERDICT_STATUS::FLAGGED;
        else                         p.status = VERDICT_STATUS::CLEAN;
    }

    p.last_updated = now;
    db::save_player_risk(p);
}

void engine::analyze_environment(const ENVIRONMENT_REPORT& r) {
    std::vector<RULE_RESULT> results = {
        rule_secure_boot(r),
        rule_hvci(r),
        rule_vbs(r),
        rule_hypervisor(r),
        rule_test_signing(r),
        rule_kernel_debugging(r),
        rule_tpm(r),
    };
    
    apply_results(r.hwid, results);
}

void engine::analyze_modules(const MODULE_REPORT& r) {
    std::vector<RULE_RESULT> results = {
        rule_manual_mapped(&r),
        rule_integrity_compromised(&r),
        rule_byovd(&r),
    };

    apply_results(r.hwid, results);
}