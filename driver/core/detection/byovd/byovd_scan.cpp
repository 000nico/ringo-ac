#include "sdk/sdk.hpp"
#include "../manual_mapped_drivers/driver_scan.hpp"
#include "../../reporting/report_byovd/report_byovd.hpp"
#include "blocked_drivers.hpp"
#include "byovd_scan.hpp"
#include "../../callbacks/image_notify/image_notify.hpp"

BOOLEAN is_hash_from_vulnerable_driver(DRIVER_HASHES hashes) {
    for (int i = 0; i < VULNERABLE_DRIVER_COUNT; i++) {
        if (hashes.sha256_valid       && RtlCompareMemory(hashes.sha256, VULNERABLE_DRIVER_HASHES[i].sha256, 32) == 32) return TRUE;
        if (hashes.authentihash_valid && RtlCompareMemory(hashes.authentihash, VULNERABLE_DRIVER_HASHES[i].authentihash, 32) == 32) return TRUE;
    }

    return FALSE;
}

BOOLEAN is_driver_vulnerable(PUNICODE_STRING full_driver_name) {
    DRIVER_HASHES driver_hashes = { 0 };
    if (NT_SUCCESS(compute_driver_hashes(full_driver_name, &driver_hashes))) {
        return is_hash_from_vulnerable_driver(driver_hashes);
    }
    else return FALSE;
}

// detects vulnerable drivers in memory when called, the idea is to call this one time at startup and
// the rest of drivers are getting detected by callbacks
VOID detect_byovd_drivers_at_runtime() {
    LOADED_LEGIT_DRIVERS* lld = get_legit_driver_list();
    if (lld == NULL) return;

    for (ULONG i = 0; i < lld->count; i++) {
        ANSI_STRING ansi_name;
        UNICODE_STRING unicode_name;
        RtlInitAnsiString(&ansi_name, (PCHAR)lld->drivers[i].full_path_name);
        if (!NT_SUCCESS(RtlAnsiStringToUnicodeString(&unicode_name, &ansi_name, TRUE))) continue;

        DRIVER_HASHES hashes = {};
        NTSTATUS status = compute_driver_hashes(&unicode_name, &hashes);

        if (NT_SUCCESS(status)) {
            BOOLEAN vulnerable = FALSE;

            if (hashes.authentihash_valid && is_hash_from_vulnerable_driver(hashes))
                vulnerable = TRUE;

            if (vulnerable) {
                PDRIVER_OBJECT driver_obj = NULL;
                if (NT_SUCCESS(get_driver_object_from_image_name(&unicode_name, &driver_obj)) && driver_obj != NULL) {
                    PDEVICE_OBJECT device_obj = driver_obj->DeviceObject;
                    if (device_obj != NULL) {
                        ObReferenceObject(device_obj);
                        report_byovd_detection(&unicode_name, driver_obj, device_obj, METHOD_RUNTIME_SCAN);
                    }
                    ObDereferenceObject(driver_obj);
                }
            }
        }

        RtlFreeUnicodeString(&unicode_name);
    }
}