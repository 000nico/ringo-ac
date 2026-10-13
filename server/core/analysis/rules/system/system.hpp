#pragma once
#include "core/models/reports.hpp"
#include "core/db/verdict.hpp"
#include <cmath>
#include <ctime>
#include <vector>
#include "core/system/system.hpp"
#include "core/analysis/engine.hpp"

RULE_RESULT rule_secure_boot(const ENVIRONMENT_REPORT& r);
RULE_RESULT rule_vbs(const ENVIRONMENT_REPORT& r);
RULE_RESULT rule_hvci(const ENVIRONMENT_REPORT& r);
RULE_RESULT rule_test_signing(const ENVIRONMENT_REPORT& r);
RULE_RESULT rule_hypervisor(const ENVIRONMENT_REPORT& r);
RULE_RESULT rule_tpm(const ENVIRONMENT_REPORT& r);
RULE_RESULT rule_kernel_debugging(const ENVIRONMENT_REPORT& r);