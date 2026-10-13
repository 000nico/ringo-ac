// client/core/communication_with_server/serialize.hpp
#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <cstdint>
#include "../../../driver/core/system/system.hpp"
#include "send_environment_report.hpp"
#include "send_module_report.hpp"
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
template <typename T, size_t N>
inline size_t array_count(const T (&)[N]) { return N; }

inline std::string wchar_to_string(const WCHAR* buffer, size_t length) {
    std::string s;
    for (size_t i = 0; i < length && buffer[i]; i++)
        s += (char)buffer[i];
    return s;
}

inline std::string uchar_to_string(const UCHAR* buffer, size_t length) {
    std::string s;
    for (size_t i = 0; i < length && buffer[i]; i++)
        s += (char)buffer[i];
    return s;
}

inline std::string bytes_to_string(const UCHAR* buffer, size_t length) {
    return std::string((const char*)buffer, length);
}

inline uint64_t as_number(const void* p) {
    return (uint64_t)(uintptr_t)p;
}

inline void to_json(nlohmann::json& j, const BYOVD_DETECTION_REPORT& r) {
    j["count"] = r.count;
    j["entries"] = nlohmann::json::array();

    for (LONG i = 0; i < r.count && i < (LONG)array_count(r.entries); i++) {
        const BYOVD_DETECTION_ENTRY& e = r.entries[i];

        j["entries"].push_back({
            { "name_buffer",     wchar_to_string(e.name_buffer, array_count(e.name_buffer)) },
            { "full_image_name", wchar_to_string(e.name_buffer, array_count(e.name_buffer)) },
            { "device_object",   as_number(e.device_object) },
            { "driver_object",   as_number(e.driver_object) },
            { "method",          (int)e.method }
        });
    }
}

inline void to_json(nlohmann::json& j, const MANUAL_MAPPED_DRIVER_DETECTION_REPORT& r) {
    j["count"] = r.count;
    j["entries"] = nlohmann::json::array();

    for (LONG i = 0; i < r.count && i < (LONG)array_count(r.entries); i++) {
        const MANUAL_MAPPED_DRIVER_DETECTION_ENTRY& e = r.entries[i];
        j["entries"].push_back({
            { "suspicious_address", as_number(e.suspicious_address) },
            { "size",               (uint64_t)e.size },
            { "method",             (int)e.method },
            { "tag",                bytes_to_string(e.tag, array_count(e.tag)) },
            { "module_path",        uchar_to_string(e.module_path, array_count(e.module_path)) }
        });
    }
}

inline void to_json(nlohmann::json& j, const FLAGGED_DLL_REPORT& r) {
    j["count"] = r.count;
    j["entries"] = nlohmann::json::array();

    for (LONG i = 0; i < r.count && i < (LONG)array_count(r.entries); i++) {
        const LOADED_MODULE& e = r.entries[i];
        nlohmann::json image = {
            { "image_base", as_number(e.mem.image.image_base) },
            { "image_size", (uint64_t)e.mem.image.image_size }
        };
        nlohmann::json mem = { { "image", image } };

        j["entries"].push_back({
            { "name_buffer",     wchar_to_string(e.name_buffer, array_count(e.name_buffer)) },
            { "full_image_name", wchar_to_string(e.name_buffer, array_count(e.name_buffer)) },
            { "is_signed",       (int)e.is_signed },
            { "mem",             mem }
        });
    }
}

inline void to_json(nlohmann::json& j, const THREAD_DETECTION_REPORT& r) {
    j["count"] = r.count;
    j["entries"] = nlohmann::json::array();

    for (LONG i = 0; i < r.count && i < (LONG)array_count(r.entries); i++) {
        const CREATED_THREAD& e = r.entries[i];
        j["entries"].push_back({
            { "start_address",       as_number(e.start_address) },
            { "is_created_remotely", (int)e.is_created_remotely },
            { "thread_id",           (uint64_t)(uintptr_t)e.thread_id },
            { "creator_id",          (uint64_t)(uintptr_t)e.creator_id }
        });
    }
}

inline void to_json(nlohmann::json& j, const HASH_DIFF_REPORT& r) {
    j["count"] = r.count;
    j["entries"] = nlohmann::json::array();

    for (LONG i = 0; i < r.count && i < (LONG)array_count(r.entries); i++)
        j["entries"].push_back({ { "mismatch_type", (int)r.entries[i].mismatch_type } });
}

inline void to_json(nlohmann::json& j, const MODULE_DATA& m) {
    j["hwid"] = m.hwid;
    j["timestamp"] = m.timestamp;
    j["byovd"] = m.reports.byovd;
    j["manual_mapped"] = m.reports.manual_mapped;
    j["flagged_dll"] = m.reports.flagged_dll;
    j["thread"] = m.reports.threads;
    j["hash_diff"] = m.reports.hash_diff;
}
