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

static bool disagree(const SOURCE_RESULT& a, const SOURCE_RESULT& b);
RULE_RESULT found(const std::string& signal, const std::string& reason);