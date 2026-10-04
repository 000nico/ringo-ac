// ioctl.cpp
#include <ntifs.h>

#include "communication.hpp"
#include "codes.hpp"

#include "../global.hpp"

#include "../system/system.hpp"
#include "../identity/identity.hpp"

NTSTATUS ioctl_handler(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);

    PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
    ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;
    ULONG inputSize = stack->Parameters.DeviceIoControl.InputBufferLength;
    ULONG outputSize = stack->Parameters.DeviceIoControl.OutputBufferLength;
    PVOID buffer = Irp->AssociatedIrp.SystemBuffer;

    NTSTATUS status = STATUS_SUCCESS;
    ULONG_PTR bytesReturned = 0;

    if ((inputSize > 0 || outputSize > 0) && buffer == NULL) {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INVALID_PARAMETER;
    }

    switch (code) {
        // getters
        case IOCTL_GET_HWID: {
            if (outputSize < sizeof(HWID)) { status = STATUS_BUFFER_TOO_SMALL; break; }

            HWID hwid = build_hwid();
            RtlCopyMemory(buffer, &hwid, sizeof(hwid));
            bytesReturned = sizeof(hwid);

            break;
        }

        case IOCTL_GET_GAME_PID: {
            if (outputSize < sizeof(HANDLE)) { status = STATUS_BUFFER_TOO_SMALL; break; }
            *(HANDLE*)buffer = g_ringo_context.Game.protected_game_pid;
            bytesReturned = sizeof(HANDLE);
            break;
        }

        case IOCTL_GET_GAME_STATUS: {
            if (outputSize < sizeof(BOOLEAN)) { status = STATUS_BUFFER_TOO_SMALL; break; }
            *(BOOLEAN*)buffer = g_ringo_context.Game.is_game_open;
            bytesReturned = sizeof(BOOLEAN);
            break;
        }

        // (enviroment)
        case IOCTL_GET_HVCI_INFORMATION: {
            if (outputSize < sizeof(HVCI_CHECK)) { status = STATUS_BUFFER_TOO_SMALL; break; }

            HVCI_CHECK hc = is_hvci_enabled();
            RtlCopyMemory(buffer, &hc, sizeof(hc));
            bytesReturned = sizeof(hc);

            break;
        }

        case IOCTL_GET_HYPERVISOR_STATUS: {
            if (outputSize < sizeof(HYPERVISOR_CHECK)) { status = STATUS_BUFFER_TOO_SMALL; break; }

            HYPERVISOR_CHECK hc = get_hypervisor_info();
            RtlCopyMemory(buffer, &hc, sizeof(hc));
            bytesReturned = sizeof(hc);

            break;
        }

        case IOCTL_GET_KERNEL_DEBUGGING_STATUS: {
            if(outputSize < sizeof(KERNEL_DEBUGGING_CHECK)) { status = STATUS_BUFFER_TOO_SMALL; break; }

            KERNEL_DEBUGGING_CHECK kdc = is_kernel_debugger_present();
            RtlCopyMemory(buffer, &kdc, sizeof(kdc));
            bytesReturned = sizeof(kdc);

            break;
        }

        case IOCTL_GET_SECURE_BOOT_INFORMATION: {
            if (outputSize < sizeof(SECURE_BOOT_CHECK)) { status = STATUS_BUFFER_TOO_SMALL; break; }

            SECURE_BOOT_CHECK sbc = is_secure_boot_enabled();
            RtlCopyMemory(buffer, &sbc, sizeof(sbc));
            bytesReturned = sizeof(sbc);

            break;
        }

        case IOCTL_GET_TEST_SIGNING_MODE: {
            if (outputSize < sizeof(TEST_SIGNING_CHECK)) { status = STATUS_BUFFER_TOO_SMALL; break; }

            TEST_SIGNING_CHECK tsc = is_test_signing_enabled();
            RtlCopyMemory(buffer, &tsc, sizeof(tsc));
            bytesReturned = sizeof(tsc);

            break;
        }

        case IOCTL_GET_VBS_INFORMATION: {
            if (outputSize < sizeof(VBS_CHECK)) { status = STATUS_BUFFER_TOO_SMALL; break; }

            VBS_CHECK vc = isVBSEnabled();
            RtlCopyMemory(buffer, &vc, sizeof(vc));
            bytesReturned = sizeof(vc);

            break;
        }

        case IOCTL_GET_TPM_INFORMATION: {
            if (outputSize < sizeof(TPM_CHECK)) { status = STATUS_BUFFER_TOO_SMALL; break; }

            TPM_CHECK tpm = get_tpm_version();
            RtlCopyMemory(buffer, &tpm, sizeof(tpm));
            bytesReturned = sizeof(tpm);

            break;
        }

        // (callbacks)
        case IOCTL_IS_CREATE_PROCESS_CALLBACK_SET: {
            if (outputSize < sizeof(BOOLEAN)) { status = STATUS_BUFFER_TOO_SMALL; break; }
            *(BOOLEAN*)buffer = g_ringo_context.Callbacks.CreateProcess.is_registered;
            bytesReturned = sizeof(BOOLEAN);
            break;
        }

        case IOCTL_IS_CREATE_THREAD_CALLBACK_SET: {
            if (outputSize < sizeof(BOOLEAN)) { status = STATUS_BUFFER_TOO_SMALL; break; }
            *(BOOLEAN*)buffer = g_ringo_context.Callbacks.CreateThread.is_registered;
            bytesReturned = sizeof(BOOLEAN);
            break;
        }

        case IOCTL_IS_IMAGE_CALLBACK_SET: {
            if (outputSize < sizeof(BOOLEAN)) { status = STATUS_BUFFER_TOO_SMALL; break; }
            *(BOOLEAN*)buffer = g_ringo_context.Callbacks.LoadImage.is_registered;
            bytesReturned = sizeof(BOOLEAN);
            break;
        }

        case IOCTL_IS_OBCALLBACK_SET: {
            if (outputSize < sizeof(BOOLEAN)) { status = STATUS_BUFFER_TOO_SMALL; break; }
            *(BOOLEAN*)buffer = g_ringo_context.Callbacks.Ob.callback_handle != NULL;
            bytesReturned = sizeof(BOOLEAN);
            break;
        }

        // setters 
        case IOCTL_SET_GAME_NAME: {
            if (inputSize == 0 || inputSize >= sizeof(g_ringo_context.Game.protected_game_process_name)) { status = STATUS_INVALID_PARAMETER; break; }

            RtlZeroMemory(g_ringo_context.Game.protected_game_process_name, sizeof(g_ringo_context.Game.protected_game_process_name));
            RtlCopyMemory(g_ringo_context.Game.protected_game_process_name, buffer, inputSize);
            g_ringo_context.Game.protected_game_process_name[inputSize] = '\0';

            break;
        }

        // driver
        /*
        case IOCTL_UNLOAD_DRIVER: {
            if (g_DriverObject && g_DriverObject->DriverUnload)
                g_DriverObject->DriverUnload(g_DriverObject);
            break;
        }
        */

        default:
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = bytesReturned;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}
