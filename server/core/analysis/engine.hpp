#pragma once

#include "../models/reports.hpp"
#include "rules/rule.hpp"
#include "../db/verdict.hpp"
#include <cmath>
#include <ctime>
#include <vector>

using Evidence = std::vector<EVIDENCE_ENTRY>;

namespace engine {
    void analyze_environment(const ENVIRONMENT_REPORT& r);
    void analyze_modules(const MODULE_REPORT& r);
}

inline bool disagree(const SOURCE_RESULT& a, const SOURCE_RESULT& b) {
    return a.valid && b.valid && a.value != b.value;
}

RULE_RESULT found(const std::string& signal, const std::string& reason);