#include "../../global.hpp"
#include "sdk/sdk.hpp"
#include "../../detection/byovd/byovd_scan.hpp"
#include "../../reporting/report_byovd/report_byovd.hpp"
#include "image_notify.hpp"

NTSTATUS get_driver_object_from_image_name(PUNICODE_STRING full_image_name, PDRIVER_OBJECT* out_driver_object) {
    UNICODE_STRING file_name = *full_image_name;

    for (int i = file_name.Length / sizeof(WCHAR) - 1; i >= 0; i--) {
        if (file_name.Buffer[i] == L'\\') {
            file_name.Buffer = &file_name.Buffer[i + 1];
            file_name.Length -= (USHORT)((i + 1) * sizeof(WCHAR));
            break;
        }
    }

    UNICODE_STRING driver_name = file_name;
    for (int i = driver_name.Length / sizeof(WCHAR) - 1; i >= 0; i--) {
        if (driver_name.Buffer[i] == L'.') {
            driver_name.Length = (USHORT)(i * sizeof(WCHAR));
            break;
        }
    }

    WCHAR path_buffer[256];
    UNICODE_STRING driver_path;
    RtlInitEmptyUnicodeString(&driver_path, path_buffer, sizeof(path_buffer));
    RtlAppendUnicodeToString(&driver_path, L"\\Driver\\");
    RtlAppendUnicodeStringToString(&driver_path, &driver_name);

    return ObReferenceObjectByName(
        &driver_path,
        OBJ_CASE_INSENSITIVE,
        NULL,
        0,
        *IoDriverObjectType,
        KernelMode,
        NULL,
        (PVOID*)out_driver_object
    );
}

VOID vulnerable_driver_detection(PUNICODE_STRING full_image_name) {
    if (is_driver_vulnerable(full_image_name)) {
        KdPrint(("Vulnerable driver detected"));

        PDRIVER_OBJECT driver_object = NULL;
        NTSTATUS driver_object_status = get_driver_object_from_image_name(full_image_name, &driver_object);

        if (NT_SUCCESS(driver_object_status) && driver_object != NULL) {
            PDEVICE_OBJECT device_object = driver_object->DeviceObject;

            if (device_object != NULL) {
                ObReferenceObject(device_object);
                // this entry gets monitored by ob_callback, which strips handle rights
                report_byovd_detection(full_image_name, driver_object, device_object, METHOD_IMAGE_CALLBACK);
            }
            ObfDereferenceObject(driver_object);
        }

        // maybe try to unload the driver(?
    }
}

VOID dll_detection(PIMAGE_INFO image_info, PUNICODE_STRING full_image_name) {
    BOOLEAN is_signed = (image_info->ImageSignatureLevel > SE_SIGNING_LEVEL_UNSIGNED);
    if (is_signed) return;

    report_flagged_dll(full_image_name, is_signed, image_info->ImageBase, image_info->ImageSize);
}

VOID load_image_callback(_In_opt_ PUNICODE_STRING full_image_name, _In_ HANDLE process_id, _In_ PIMAGE_INFO image_info) {
    if (full_image_name == NULL || image_info == NULL) return;

    if (process_id == NULL || process_id == (HANDLE)4) {
        vulnerable_driver_detection(full_image_name);
        return;
    }

    if (!g_ringo_context.game.is_game_open || g_ringo_context.game.protected_game_pid == NULL) return;
    if (process_id != (HANDLE)g_ringo_context.game.protected_game_pid) return;
    dll_detection(image_info, full_image_name);
}

VOID set_load_image_callback() {
    NTSTATUS status = PsSetLoadImageNotifyRoutineEx(load_image_callback, 0);

    if (NT_SUCCESS(status)) {
        g_ringo_context.callbacks.load_image.callback_ptr = load_image_callback;
        g_ringo_context.callbacks.load_image.is_registered = TRUE;
    }
    else {
        g_ringo_context.callbacks.load_image.is_registered = FALSE;
    }
}
