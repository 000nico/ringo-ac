#include "report_hash_diff.hpp"
#include "core/global.hpp"

void add_hash_mismatch_detection_entry(HASH_MISMATCH_TYPE type) {
    HASH_DIFF_ENTRY entry;
    entry.mismatch_type = type;
    g_ringo_context.Detections.hash_diff_detection_report.Entries[g_ringo_context.Detections.hash_diff_detection_report.count] = entry;
    g_ringo_context.Detections.hash_diff_detection_report.count++;
}