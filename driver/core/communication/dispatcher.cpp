// ioctl.cpp
#include <ntifs.h>

#include "communication.hpp"
#include "codes.hpp"

#include "../global.hpp"

#include "../system/system.hpp"
#include "../identity/identity.hpp"
#include "../heartbeat/heartbeat.hpp"
#include "ntstatus.h"
#include "protection.hpp"
#include "sdk/crypto/hashes.hpp"
#include "wdm.h"

NTSTATUS ioctl_handler(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);

    Irp->IoStatus.Information = 0;
    NTSTATUS status = STATUS_ACCESS_DENIED;

    PEPROCESS caller = IoGetRequestorProcess(Irp);

    // verify caller integrity
    UCHAR process_hash[HASH_SIZE_SHA256] = {};

    if (!g_ringo_context.session.active ||
        g_ringo_context.session.process != caller ||
        PsGetProcessStartKey(caller) != g_ringo_context.session.process_start_key)
        goto complete;

    if (!get_caller_process_hash(caller, process_hash))
        goto complete;

    if (!compare_sha256(g_ringo_context.session.last_hash, process_hash))
        goto complete;

    if(is_peprocess_banned(caller))
        goto complete;

    RtlCopyMemory(g_ringo_context.session.last_hash, process_hash, HASH_SIZE_SHA256);

    {
        PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
        ULONG code       = stack->Parameters.DeviceIoControl.IoControlCode;
        ULONG input_size  = stack->Parameters.DeviceIoControl.InputBufferLength;
        ULONG output_size = stack->Parameters.DeviceIoControl.OutputBufferLength;
        PVOID buffer     = Irp->AssociatedIrp.SystemBuffer;

        ULONG_PTR bytes_returned = 0;

        if ((input_size > 0 || output_size > 0) && buffer == NULL) {
            status = STATUS_INVALID_PARAMETER;
            goto complete;
        }

        status = STATUS_SUCCESS;

        switch (code) {
            case IOCTL_GET_HWID: {
                if (output_size < sizeof(HWID)) { status = STATUS_BUFFER_TOO_SMALL; break; }
                HWID hwid = build_hwid();
                RtlCopyMemory(buffer, &hwid, sizeof(hwid));
                bytes_returned = sizeof(hwid);
                break;
            }

            case IOCTL_GET_GAME_PID: {
                if (output_size < sizeof(HANDLE)) { status = STATUS_BUFFER_TOO_SMALL; break; }
                *(HANDLE*)buffer = g_ringo_context.game.protected_game_pid;
                bytes_returned = sizeof(HANDLE);
                break;
            }

            case IOCTL_GET_GAME_STATUS: {
                if (output_size < sizeof(BOOLEAN)) { status = STATUS_BUFFER_TOO_SMALL; break; }
                *(BOOLEAN*)buffer = g_ringo_context.game.is_game_open;
                bytes_returned = sizeof(BOOLEAN);
                break;
            }

            case IOCTL_GET_HVCI_INFORMATION: {
                if (output_size < sizeof(HVCI_CHECK)) { status = STATUS_BUFFER_TOO_SMALL; break; }
                HVCI_CHECK hc = is_hvci_enabled();
                RtlCopyMemory(buffer, &hc, sizeof(hc));
                bytes_returned = sizeof(hc);
                break;
            }

            case IOCTL_GET_HYPERVISOR_STATUS: {
                if (output_size < sizeof(HYPERVISOR_CHECK)) { status = STATUS_BUFFER_TOO_SMALL; break; }
                HYPERVISOR_CHECK hc = get_hypervisor_info();
                RtlCopyMemory(buffer, &hc, sizeof(hc));
                bytes_returned = sizeof(hc);
                break;
            }

            case IOCTL_GET_KERNEL_DEBUGGING_STATUS: {
                if (output_size < sizeof(KERNEL_DEBUGGING_CHECK)) { status = STATUS_BUFFER_TOO_SMALL; break; }
                KERNEL_DEBUGGING_CHECK kdc = is_kernel_debugger_present();
                RtlCopyMemory(buffer, &kdc, sizeof(kdc));
                bytes_returned = sizeof(kdc);
                break;
            }

            case IOCTL_GET_SECURE_BOOT_INFORMATION: {
                if (output_size < sizeof(SECURE_BOOT_CHECK)) { status = STATUS_BUFFER_TOO_SMALL; break; }
                SECURE_BOOT_CHECK sbc = is_secure_boot_enabled();
                RtlCopyMemory(buffer, &sbc, sizeof(sbc));
                bytes_returned = sizeof(sbc);
                break;
            }

            case IOCTL_GET_HEARTBEAT: {
                if (output_size < sizeof(HEARTBEAT)) { status = STATUS_BUFFER_TOO_SMALL; break; }
                add_heartbeat_counter();
                HEARTBEAT hb = get_heartbeat();
                RtlCopyMemory(buffer, &hb, sizeof(hb));
                bytes_returned = sizeof(hb);
                break;
            }
            
            case IOCTL_GET_TEST_SIGNING_MODE: {
                if (output_size < sizeof(TEST_SIGNING_CHECK)) { status = STATUS_BUFFER_TOO_SMALL; break; }
                TEST_SIGNING_CHECK tsc = is_test_signing_enabled();
                RtlCopyMemory(buffer, &tsc, sizeof(tsc));
                bytes_returned = sizeof(tsc);
                break;
            }

            case IOCTL_GET_VBS_INFORMATION: {
                if (output_size < sizeof(VBS_CHECK)) { status = STATUS_BUFFER_TOO_SMALL; break; }
                VBS_CHECK vc = is_vbs_enabled();
                RtlCopyMemory(buffer, &vc, sizeof(vc));
                bytes_returned = sizeof(vc);
                break;
            }

            case IOCTL_GET_TPM_INFORMATION: {
                if (output_size < sizeof(TPM_CHECK)) { status = STATUS_BUFFER_TOO_SMALL; break; }
                TPM_CHECK tpm = get_tpm_version();
                RtlCopyMemory(buffer, &tpm, sizeof(tpm));
                bytes_returned = sizeof(tpm);
                break;
            }

            case IOCTL_IS_CREATE_PROCESS_CALLBACK_SET: {
                if (output_size < sizeof(BOOLEAN)) { status = STATUS_BUFFER_TOO_SMALL; break; }
                *(BOOLEAN*)buffer = g_ringo_context.callbacks.create_process.is_registered;
                bytes_returned = sizeof(BOOLEAN);
                break;
            }

            case IOCTL_IS_CREATE_THREAD_CALLBACK_SET: {
                if (output_size < sizeof(BOOLEAN)) { status = STATUS_BUFFER_TOO_SMALL; break; }
                *(BOOLEAN*)buffer = g_ringo_context.callbacks.create_thread.is_registered;
                bytes_returned = sizeof(BOOLEAN);
                break;
            }

            case IOCTL_IS_IMAGE_CALLBACK_SET: {
                if (output_size < sizeof(BOOLEAN)) { status = STATUS_BUFFER_TOO_SMALL; break; }
                *(BOOLEAN*)buffer = g_ringo_context.callbacks.load_image.is_registered;
                bytes_returned = sizeof(BOOLEAN);
                break;
            }

            case IOCTL_IS_OBCALLBACK_SET: {
                if (output_size < sizeof(BOOLEAN)) { status = STATUS_BUFFER_TOO_SMALL; break; }
                *(BOOLEAN*)buffer = g_ringo_context.callbacks.ob.callback_handle != NULL;
                bytes_returned = sizeof(BOOLEAN);
                break;
            }

            case IOCTL_SET_GAME_NAME: {
                if (input_size == 0 || input_size >= sizeof(g_ringo_context.game.protected_game_process_name)) {
                    status = STATUS_INVALID_PARAMETER;
                    break;
                }

                if(g_ringo_context.game.protected_game_process_name[0] == '\0') {
                    RtlZeroMemory(g_ringo_context.game.protected_game_process_name, sizeof(g_ringo_context.game.protected_game_process_name));
                    RtlCopyMemory(g_ringo_context.game.protected_game_process_name, buffer, input_size);
                    g_ringo_context.game.protected_game_process_name[input_size] = '\0';
                }
                
                break;
            }

            default:
                g_ringo_context.session.banned.banned_peprocess[g_ringo_context.session.banned.index] = g_ringo_context.session.process;
                g_ringo_context.session.banned.index++;
                
                g_ringo_context.session.active = FALSE;
                g_ringo_context.session.process = NULL;
                g_ringo_context.session.process_start_key = 0;
                RtlCopyMemory(g_ringo_context.session.last_hash, 0, HASH_SIZE_SHA256);
                status = STATUS_INVALID_DEVICE_REQUEST;
                break;
        }

        Irp->IoStatus.Information = bytes_returned;
    }

complete:
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}