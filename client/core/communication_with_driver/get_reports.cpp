#include "get_reports.hpp"

static BOOLEAN query_report(HANDLE h_driver, ENVIRONMENT_INFO_CLASS information_class, PVOID buffer, ULONG size) {
    DWORD bytes_returned = 0;
    return get_environment_information(h_driver, information_class, buffer, size, &bytes_returned);
}

BOOLEAN get_reports(HANDLE h_driver, DETECTION_REPORTS* reports) {
    if (!h_driver || h_driver == INVALID_HANDLE_VALUE || !reports)
        return FALSE;

    *reports = {};

    return query_report(h_driver, INFO_BYOVD_REPORT,         &reports->byovd,         sizeof(reports->byovd))
        && query_report(h_driver, INFO_MANUAL_MAPPED_REPORT, &reports->manual_mapped, sizeof(reports->manual_mapped))
        && query_report(h_driver, INFO_HASH_DIFF_REPORT,     &reports->hash_diff,     sizeof(reports->hash_diff))
        && query_report(h_driver, INFO_FLAGGED_DLL_REPORT,   &reports->flagged_dll,   sizeof(reports->flagged_dll))
        && query_report(h_driver, INFO_THREAD_REPORT,        &reports->threads,       sizeof(reports->threads));
}
