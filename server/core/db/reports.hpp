#pragma once
#include "../../sdk/helpers.hpp"
#include <string>
#include <cstdint>

namespace db {
    void save_environment_report(const ENVIRONMENT_REPORT& report);
    void save_module_report(const std::string& hwid, uint64_t timestamp, const std::string& payload);
}


