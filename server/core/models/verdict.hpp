#pragma once
#include <string>
#include <cstdint>

enum class VerdictStatus {
    Clean   = 0,
    Flagged = 1,
    Banned  = 2
};

struct Verdict {
    std::string hwid;
    VerdictStatus status;
    std::string reason; // top reason
    float confidence;  // 0.0 - 1.0
    uint64_t issued_at;   // unix timestamp
};