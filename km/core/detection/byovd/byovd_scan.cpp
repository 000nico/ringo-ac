#include "sdk/sdk.hpp"
#include "../manual_mapped_drivers/driver_scan.hpp"
#include "../../reporting/report_byovd/report_byovd.hpp"
#include "blocked_drivers.hpp"
#include "byovd_scan.hpp"
#include "../../callbacks/image_notify/image_notify.hpp"

BOOLEAN is_hash_from_vulnerable_driver(DRIVER_HASHES hashes) {
    for (int i = 0; i < VulnerableDriverCount; i++) {
        if (hashes.SHA256Valid       && RtlCompareMemory(hashes.SHA256, VulnerableDriverHashes[i].SHA256, 32) == 32) return TRUE;
        if (hashes.AuthentihashValid && RtlCompareMemory(hashes.Authentihash, VulnerableDriverHashes[i].Authentihash, 32) == 32) return TRUE;
    }

    return FALSE;
}

BOOLEAN is_driver_vulnerable(PUNICODE_STRING FullDriverName) {
    DRIVER_HASHES driver_hashes = { 0 };
    if (NT_SUCCESS(compute_driver_hashes(FullDriverName, &driver_hashes))) {
        return is_hash_from_vulnerable_driver(driver_hashes);
    }
    else return FALSE;
}

// detects vulnerable drivers in memory when called, the idea is to call this one time at startup and
// the rest of drivers are getting detected by callbacks
VOID detect_byovd_drivers_at_runtime() {
    LOADED_LEGIT_DRIVERS* lld = get_legit_driver_list();
    if (lld == NULL) return;

    for (ULONG i = 0; i < lld->Count; i++) {
        ANSI_STRING ansiName;
        UNICODE_STRING unicodeName;
        RtlInitAnsiString(&ansiName, (PCHAR)lld->Drivers[i].full_path_name);
        if (!NT_SUCCESS(RtlAnsiStringToUnicodeString(&unicodeName, &ansiName, TRUE))) continue;

        DRIVER_HASHES hashes = {};
        NTSTATUS status = compute_driver_hashes(&unicodeName, &hashes);

        if (NT_SUCCESS(status)) {
            BOOLEAN vulnerable = FALSE;

            if (hashes.AuthentihashValid && is_hash_from_vulnerable_driver(hashes))
                vulnerable = TRUE;

            if (vulnerable) {
                PDRIVER_OBJECT driverObj = NULL;
                if (NT_SUCCESS(get_driver_object_from_image_name(&unicodeName, &driverObj)) && driverObj != NULL) {
                    PDEVICE_OBJECT deviceObj = driverObj->DeviceObject;
                    if (deviceObj != NULL) {
                        ObReferenceObject(deviceObj);
                        add_byovd_report_entry(&unicodeName, driverObj, deviceObj, METHOD_RUNTIME_SCAN);
                    }
                    ObDereferenceObject(driverObj);
                }
            }
        }

        RtlFreeUnicodeString(&unicodeName);
    }
}