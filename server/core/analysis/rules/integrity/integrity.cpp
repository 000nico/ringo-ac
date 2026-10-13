#include "integrity.hpp"
#include "core/analysis/rules/rule.hpp"
#include "core/reporting/report_hash_diff/report_hash_diff.hpp"

RULE_RESULT rule_integrity_compromised(const MODULE_REPORT* r) {
    const HASH_DIFF_REPORT* rep = &r->hash_diff;

    for(unsigned long i = 0; i < rep->count; i++){
        if(rep->entries[i].mismatch_type == PROCESS_WITH_PROCESS)
            return found("game_hash_mismatch", "process diff with process");
        if(rep->entries[i].mismatch_type == FILE_WITH_PROCESS)
            return found("game_hash_mismatch", "file diff with process");
    }

    return no_finding();
 }