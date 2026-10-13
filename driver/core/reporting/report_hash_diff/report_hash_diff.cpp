#include "report_hash_diff.hpp"
#include "core/global.hpp"

VOID report_hash_mismatch(HASH_MISMATCH_TYPE type) {
    LONG index = InterlockedIncrement(&g_ringo_context.detections.hash_diff_detection_report.count) - 1;
    if (index >= 256) return;

    g_ringo_context.detections.hash_diff_detection_report.entries[index].mismatch_type = type;
}
