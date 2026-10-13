#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <nlohmann/json.hpp>

struct ENVIRONMENT_REPORT {
    std::string hwid;
    uint64_t    timestamp;
    bool        debugger_present;
    bool        hypervisor_detected;
    bool        secure_boot_enabled;
    bool        hvci_enabled;
    bool        test_signing_enabled;
    bool        vbs_enabled;
};

struct MODULE_REPORT {
    std::string              hwid;
    uint64_t                 timestamp;
    bool                     process_hash_mismatch;
    std::vector<std::string> unsigned_modules;
    std::vector<std::string> suspicious_module_names;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ENVIRONMENT_REPORT,
    hwid, timestamp,
    debugger_present, hypervisor_detected,
    secure_boot_enabled, hvci_enabled,
    test_signing_enabled, vbs_enabled)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MODULE_REPORT,
    hwid, timestamp,
    process_hash_mismatch,
    unsigned_modules,
    suspicious_module_names)