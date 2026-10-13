#include "byovd.hpp"
#include "core/analysis/rules/rule.hpp"
#include "core/reporting/report_byovd/report_byovd.hpp"

RULE_RESULT rule_byovd(const MODULE_REPORT* r){
    const BYOVD_DETECTION_REPORT* rep = &r->byovd;

    for(unsigned long i = 0; i < rep->count; i++){
        if(rep->entries[i].method == METHOD_RUNTIME_SCAN)
            return found("byovd_loaded", "found byovd when loading ringo");
        if(rep->entries[i].method == METHOD_IMAGE_CALLBACK)
            return found("byovd_loaded", "byovd detected by image callback");
        if(rep->entries[i].method == METHOD_DRIVER_WALKING)
            return found("byovd_loaded", "byovd detected by walking drivers");
    }

    return no_finding();
}