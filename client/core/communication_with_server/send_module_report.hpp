#pragma once
#include <windows.h>
#include <string>
#include <cstdint>
#include "core/communication_with_driver/get_reports.hpp"

// Lo que se manda al server: hwid + timestamp + los 5 reportes del driver tal cual.
struct MODULE_DATA {
    std::string       hwid;
    uint64_t          timestamp;
    DETECTION_REPORTS reports;
};

bool collect_module_data(HANDLE drv, MODULE_DATA& out);
bool send_module_report(const MODULE_DATA& report);
