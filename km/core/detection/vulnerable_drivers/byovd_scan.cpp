#include "../../../sdk/imports/imports.hpp"
#include "../../../sdk/crypto/hashes.hpp"
#include "../manual_map_driver_scan/driver_scan.hpp"
#include "vulnerable_driver_detections.hpp"
#include "blocked_drivers.hpp"
#include "byovd_scan.hpp"
#include "../callbacks/image_notify/image_notify.hpp"

BOOLEAN isHashFromVulnerableDriver(DRIVER_HASHES hashes) {
    for (int i = 0; i < VulnerableDriverCount; i++) {
        if (hashes.SHA256Valid       && RtlCompareMemory(hashes.SHA256, VulnerableDriverHashes[i].SHA256, 32) == 32) return TRUE;
        if (hashes.AuthentihashValid && RtlCompareMemory(hashes.Authentihash, VulnerableDriverHashes[i].Authentihash, 32) == 32) return TRUE;
    }

    return FALSE;
}

BOOLEAN isDriverVulnerable(PUNICODE_STRING FullDriverName) {
    DRIVER_HASHES driver_hashes = { 0 };
    if (NT_SUCCESS(ComputeDriverHashes(FullDriverName, &driver_hashes))) {
        return isHashFromVulnerableDriver(driver_hashes);
    }
    else return FALSE;
}

// detects vulnerable drivers in memory when called, the idea is to call this one time at startup and
// the rest of drivers are getting detected by callbacks
VOID DetectVulnerableDriversAtRuntime() {
    LOADED_LEGIT_DRIVERS* lld = GetLegitDriverList();
    if (lld == NULL) return;

    for (ULONG i = 0; i < lld->Count; i++) {
        ANSI_STRING ansiName;
        UNICODE_STRING unicodeName;
        RtlInitAnsiString(&ansiName, (PCHAR)lld->Drivers[i].FullPathName);
        if (!NT_SUCCESS(RtlAnsiStringToUnicodeString(&unicodeName, &ansiName, TRUE))) continue;

        DRIVER_HASHES hashes = {};
        NTSTATUS status = ComputeDriverHashes(&unicodeName, &hashes);

        if (NT_SUCCESS(status)) {
            BOOLEAN vulnerable = FALSE;

            if (hashes.AuthentihashValid && isHashFromVulnerableDriver(hashes))
                vulnerable = TRUE;

            if (vulnerable) {
                PDRIVER_OBJECT driverObj = NULL;
                if (NT_SUCCESS(GetDriverObjectFromImageName(&unicodeName, &driverObj)) && driverObj != NULL) {
                    PDEVICE_OBJECT deviceObj = driverObj->DeviceObject;
                    if (deviceObj != NULL) {
                        ObReferenceObject(deviceObj);
                        AddVulnerableDriverDetectionEntry(&unicodeName, driverObj, deviceObj, METHOD_RUNTIME_SCAN);
                    }
                    ObDereferenceObject(driverObj);
                }
            }
        }

        RtlFreeUnicodeString(&unicodeName);
    }
}