#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <nlohmann/json.hpp>

// this mirrors what the client sends (see client/core/communication_with_server/serialize.hpp):
// every source is serialized as { "valid": bool, "value": bool }
struct SOURCE_RESULT {
    bool valid;
    bool value;
};

struct SECURE_BOOT_REPORT {
    SOURCE_RESULT nt_query_enabled;
    SOURCE_RESULT nt_query_capable;
    SOURCE_RESULT uefi_var;
    SOURCE_RESULT registry;
};

struct HVCI_REPORT {
    SOURCE_RESULT code_integrity;
};

struct VBS_REPORT {
    SOURCE_RESULT nt_query_secure_kernel;
    SOURCE_RESULT nt_query_hvci;
};

struct HYPERVISOR_REPORT {
    SOURCE_RESULT nt_query;
    SOURCE_RESULT cpuid;
};

struct KERNEL_DEBUGGING_REPORT {
    SOURCE_RESULT nt_query_enabled;
    SOURCE_RESULT nt_query_not_present;
};

struct TEST_SIGNING_REPORT {
    SOURCE_RESULT code_integrity;
};

struct TPM_REPORT {
    SOURCE_RESULT tpm_2_0;
    SOURCE_RESULT tpm_1_2;
};

struct ENVIRONMENT_REPORT {
    std::string             hwid;
    SECURE_BOOT_REPORT      secure_boot;
    HVCI_REPORT             hvci;
    VBS_REPORT              vbs;
    HYPERVISOR_REPORT       hypervisor;
    KERNEL_DEBUGGING_REPORT kernel_debugging;
    TEST_SIGNING_REPORT     test_signing;
    TPM_REPORT              tpm;
};

struct MODULE_REPORT {
    std::string              hwid;
    uint64_t                 timestamp;
    bool                     process_hash_mismatch;
    std::vector<std::string> unsigned_modules;
    std::vector<std::string> suspicious_module_names;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SOURCE_RESULT, valid, value)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SECURE_BOOT_REPORT, nt_query_enabled, nt_query_capable, uefi_var, registry)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(HVCI_REPORT, code_integrity)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(VBS_REPORT, nt_query_secure_kernel, nt_query_hvci)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(HYPERVISOR_REPORT, nt_query, cpuid)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(KERNEL_DEBUGGING_REPORT, nt_query_enabled, nt_query_not_present)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TEST_SIGNING_REPORT, code_integrity)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TPM_REPORT, tpm_2_0, tpm_1_2)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ENVIRONMENT_REPORT,
    hwid, secure_boot, hvci, vbs, hypervisor, kernel_debugging, test_signing, tpm)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MODULE_REPORT,
    hwid, timestamp,
    process_hash_mismatch,
    unsigned_modules,
    suspicious_module_names)
