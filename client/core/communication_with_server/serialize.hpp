// client/core/communication_with_server/serialize.hpp
#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include "../../../driver/core/system/system.hpp"
#include "send_environment_report.hpp"
#include "../../../driver/core/identity/identity.hpp"

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_ONLY_SERIALIZE(SOURCE_RESULT, valid, value)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_ONLY_SERIALIZE(SECURE_BOOT_CHECK, nt_query_enabled, nt_query_capable, uefi_var, registry)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_ONLY_SERIALIZE(HVCI_CHECK, code_integrity)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_ONLY_SERIALIZE(VBS_CHECK, nt_query_secure_kernel, nt_query_hvci)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_ONLY_SERIALIZE(HYPERVISOR_CHECK, nt_query, cpuid)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_ONLY_SERIALIZE(KERNEL_DEBUGGING_CHECK, nt_query_enabled, nt_query_not_present)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_ONLY_SERIALIZE(TEST_SIGNING_CHECK, code_integrity)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_ONLY_SERIALIZE(TPM_CHECK, tpm_2_0, tpm_1_2)

inline std::string hwid_to_string(const HWID& h) {
    char buf[128];
    snprintf(buf, sizeof(buf), "%lu-%lu-", h.cpu_hash, h.disk_serial);
    std::string s = buf;

    for (size_t i = 0; i < sizeof(h.smbios_uuid); i++) {
        snprintf(buf, sizeof(buf), "%02X", ((const BYTE*)&h.smbios_uuid)[i]);
        s += buf;
    }
    return s;  
}

inline void to_json(nlohmann::json& j, const HWID& h) {
    j = hwid_to_string(h);
}

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_ONLY_SERIALIZE(ENVIRONMENT_DATA, secure_boot, hvci, vbs, hypervisor, kernel_debugging, test_signing, tpm, hwid)