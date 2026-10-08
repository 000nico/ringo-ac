#pragma once
#include "../models/reports.hpp"

namespace db {
    void save_environment_report(const EnvironmentReport& report);
    void save_module_report(const ModuleReport& report);
}