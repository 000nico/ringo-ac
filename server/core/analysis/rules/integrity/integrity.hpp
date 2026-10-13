#pragma once
#include "core/models/reports.hpp"
#include "core/db/verdict.hpp"
#include <cmath>
#include <ctime>
#include <vector>
#include "core/system/system.hpp"
#include "core/analysis/engine.hpp"

RULE_RESULT rule_integrity_compromised(const MODULE_REPORT* r);