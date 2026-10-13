#include "../../sdk/helpers.hpp"
#pragma once
#include <string>
#include <cstdint>
#include <cstring>
#include <nlohmann/json.hpp>
#include "core/system/system.hpp"
#include "core/reporting/report_byovd/report_byovd.hpp"
#include "core/reporting/report_manual_mapped_driver/report_manual_mapped_driver.hpp"
#include "core/reporting/report_flagged_dll/report_flagged_dll.hpp"
#include "core/reporting/report_thread_detection/report_thread_detection.hpp"
#include "core/reporting/report_hash_diff/report_hash_diff.hpp"
struct ENVIRONMENT_REPORT {
    std::string            hwid;
    SECURE_BOOT_CHECK      secure_boot;
    HVCI_CHECK             hvci;
    VBS_CHECK              vbs;
    HYPERVISOR_CHECK       hypervisor;
    KERNEL_DEBUGGING_CHECK kernel_debugging;
    TEST_SIGNING_CHECK     test_signing;
    TPM_CHECK              tpm;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SOURCE_RESULT, valid, value)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SECURE_BOOT_CHECK, nt_query_enabled, nt_query_capable, uefi_var, registry)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(HVCI_CHECK, code_integrity)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(VBS_CHECK, nt_query_secure_kernel, nt_query_hvci)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(HYPERVISOR_CHECK, nt_query, cpuid)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(KERNEL_DEBUGGING_CHECK, nt_query_enabled, nt_query_not_present)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TEST_SIGNING_CHECK, code_integrity)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TPM_CHECK, tpm_2_0, tpm_1_2)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ENVIRONMENT_REPORT,
    hwid, secure_boot, hvci, vbs, hypervisor, kernel_debugging, test_signing, tpm)
struct MODULE_REPORT {
    std::string                           hwid;
    uint64_t                              timestamp;
    BYOVD_DETECTION_REPORT                byovd;
    MANUAL_MAPPED_DRIVER_DETECTION_REPORT manual_mapped;
    FLAGGED_DLL_REPORT                    flagged_dll;
    THREAD_DETECTION_REPORT               thread;
    HASH_DIFF_REPORT                      hash_diff;
};
template <typename T, size_t N>
inline size_t sdk::array_count(const T (&)[N]) { return N; }

inline void sdk::string_to_wchar(const std::string& s, WCHAR* buffer, size_t length) {
    size_t n = s.size() < length - 1 ? s.size() : length - 1;
    for (size_t i = 0; i < n; i++)
        buffer[i] = (WCHAR)(unsigned char)s[i];
    buffer[n] = 0;
}

inline void sdk::string_to_uchar(const std::string& s, UCHAR* buffer, size_t length) {
    size_t n = s.size() < length - 1 ? s.size() : length - 1;
    for (size_t i = 0; i < n; i++)
        buffer[i] = (UCHAR)s[i];
    buffer[n] = 0;
}

inline void* sdk::number_to_pointer(uint64_t number) {
    return (void*)(uintptr_t)number;
}

inline LONG json_count(const nlohmann::json& j, size_t max_entries) {
    LONG count = j.value("count", 0);
    if (count < 0) count = 0;
    if (count > (LONG)max_entries) count = (LONG)max_entries;
    return count;
}
inline void set_full_image_name(const nlohmann::json& e, WCHAR* name_buffer, size_t length, UNICODE_STRING& full_image_name) {
    const std::string name = e.value("name_buffer", std::string());
    const std::string full = e.value("full_image_name", std::string());

    sdk::string_to_wchar(name, name_buffer, length);

    const size_t full_length = full.size() < length - 1 ? full.size() : length - 1;
    full_image_name.Length        = (USHORT)(full_length * sizeof(WCHAR));
    full_image_name.MaximumLength = (USHORT)(length * sizeof(WCHAR));
    full_image_name.Buffer        = name_buffer;
}

inline void from_json(const nlohmann::json& j, BYOVD_DETECTION_REPORT& r) {
    memset(&r, 0, sizeof(r));
    r.count = json_count(j, sdk::array_count(r.entries));

    const nlohmann::json& entries = j.at("entries");
    for (LONG i = 0; i < r.count; i++) {
        const nlohmann::json& e = entries.at(i);
        BYOVD_DETECTION_ENTRY& entry = r.entries[i];

        set_full_image_name(e, entry.name_buffer, sdk::array_count(entry.name_buffer), entry.full_image_name);
        entry.device_object = (PDEVICE_OBJECT)sdk::number_to_pointer(e.value("device_object", (uint64_t)0));
        entry.driver_object = (PDRIVER_OBJECT)sdk::number_to_pointer(e.value("driver_object", (uint64_t)0));
        entry.method        = (BYOVD_DETECTION_METHOD)e.value("method", 0);
    }
}

inline void from_json(const nlohmann::json& j, MANUAL_MAPPED_DRIVER_DETECTION_REPORT& r) {
    memset(&r, 0, sizeof(r));
    r.count = json_count(j, sdk::array_count(r.entries));

    const nlohmann::json& entries = j.at("entries");
    for (LONG i = 0; i < r.count; i++) {
        const nlohmann::json& e = entries.at(i);
        MANUAL_MAPPED_DRIVER_DETECTION_ENTRY& entry = r.entries[i];

        entry.suspicious_address = sdk::number_to_pointer(e.value("suspicious_address", (uint64_t)0));
        entry.size               = (SIZE_T)e.value("size", (uint64_t)0);
        entry.method             = (DETECTION_METHOD)e.value("method", 0);

        const std::string tag = e.value("tag", std::string());
        const size_t tag_length = tag.size() < sdk::array_count(entry.tag) ? tag.size() : sdk::array_count(entry.tag);
        memcpy(entry.tag, tag.data(), tag_length);

        sdk::string_to_uchar(e.value("module_path", std::string()), entry.module_path, sdk::array_count(entry.module_path));
    }
}

inline void from_json(const nlohmann::json& j, FLAGGED_DLL_REPORT& r) {
    memset(&r, 0, sizeof(r));
    r.count = json_count(j, sdk::array_count(r.entries));

    const nlohmann::json& entries = j.at("entries");
    for (LONG i = 0; i < r.count; i++) {
        const nlohmann::json& e = entries.at(i);
        LOADED_MODULE& entry = r.entries[i];

        set_full_image_name(e, entry.name_buffer, sdk::array_count(entry.name_buffer), entry.full_image_name);
        entry.is_signed = (BOOLEAN)e.value("is_signed", 0);

        const nlohmann::json& image = e.at("mem").at("image");
        entry.mem.image.image_base = sdk::number_to_pointer(image.value("image_base", (uint64_t)0));
        entry.mem.image.image_size = (SIZE_T)image.value("image_size", (uint64_t)0);
    }
}

inline void from_json(const nlohmann::json& j, THREAD_DETECTION_REPORT& r) {
    memset(&r, 0, sizeof(r));
    r.count = json_count(j, sdk::array_count(r.entries));

    const nlohmann::json& entries = j.at("entries");
    for (LONG i = 0; i < r.count; i++) {
        const nlohmann::json& e = entries.at(i);
        CREATED_THREAD& entry = r.entries[i];

        entry.start_address       = sdk::number_to_pointer(e.value("start_address", (uint64_t)0));
        entry.is_created_remotely = (BOOLEAN)e.value("is_created_remotely", 0);
        entry.thread_id           = (HANDLE)sdk::number_to_pointer(e.value("thread_id", (uint64_t)0));
        entry.creator_id          = (HANDLE)sdk::number_to_pointer(e.value("creator_id", (uint64_t)0));
    }
}

inline void from_json(const nlohmann::json& j, HASH_DIFF_REPORT& r) {
    memset(&r, 0, sizeof(r));
    r.count = json_count(j, sdk::sdk::array_count(r.entries));

    const nlohmann::json& entries = j.at("entries");
    for (LONG i = 0; i < r.count; i++)
        r.entries[i].mismatch_type = (HASH_MISMATCH_TYPE)entries.at(i).value("mismatch_type", 0);
}

inline void from_json(const nlohmann::json& j, MODULE_REPORT& r) {
    r.hwid      = j.value("hwid", std::string());
    r.timestamp = j.value("timestamp", (uint64_t)0);

    j.at("byovd").get_to(r.byovd);
    j.at("manual_mapped").get_to(r.manual_mapped);
    j.at("flagged_dll").get_to(r.flagged_dll);
    j.at("thread").get_to(r.thread);
    j.at("hash_diff").get_to(r.hash_diff);
}


