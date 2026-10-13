#pragma once
#include <string>
#include <cstdint>

enum class VERDICT_STATUS {
    CLEAN   = 0,
    FLAGGED = 1,
    BANNED  = 2
};

struct VERDICT {
    std::string hwid;
    VERDICT_STATUS status;
    std::string reason; // top reason
    float confidence;  // 0.0 - 1.0
    uint64_t issued_at;   // unix timestamp
};