#pragma once
#include "../models/reports.hpp"

namespace db {
    void save_environment_report(const ENVIRONMENT_REPORT& report);
    void save_module_report(const MODULE_REPORT& report);
}