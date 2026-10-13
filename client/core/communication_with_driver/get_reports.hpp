#pragma once
#include <windows.h>

#include "communication_with_driver.hpp"
#include "../../../driver/core/reporting/report_byovd/report_byovd.hpp"
#include "../../../driver/core/reporting/report_manual_mapped_driver/report_manual_mapped_driver.hpp"
#include "../../../driver/core/reporting/report_hash_diff/report_hash_diff.hpp"
#include "../../../driver/core/reporting/report_flagged_dll/report_flagged_dll.hpp"
#include "../../../driver/core/reporting/report_thread_detection/report_thread_detection.hpp"

struct DETECTION_REPORTS {
    BYOVD_DETECTION_REPORT                byovd;
    MANUAL_MAPPED_DRIVER_DETECTION_REPORT manual_mapped;
    HASH_DIFF_REPORT                      hash_diff;
    FLAGGED_DLL_REPORT                    flagged_dll;
    THREAD_DETECTION_REPORT               threads;
};

BOOLEAN get_reports(HANDLE h_driver, DETECTION_REPORTS* reports);
