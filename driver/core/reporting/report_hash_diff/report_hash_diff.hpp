#pragma once
#include "sdk/sdk.hpp"

enum HASH_MISMATCH_TYPE {
    PROCESS_WITH_PROCESS,
    FILE_WITH_PROCESS
};

struct HASH_DIFF_ENTRY {
    HASH_MISMATCH_TYPE mismatch_type;
};

struct HASH_DIFF_REPORT {
    LONG           count;
    HASH_DIFF_ENTRY entries[256];
};

VOID report_hash_mismatch(HASH_MISMATCH_TYPE type);
