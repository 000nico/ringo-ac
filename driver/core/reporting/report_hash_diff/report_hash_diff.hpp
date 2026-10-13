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
    HASH_DIFF_ENTRY Entries[256];
    LONG count;
};

void add_hash_mismatch_detection_entry(HASH_MISMATCH_TYPE type);