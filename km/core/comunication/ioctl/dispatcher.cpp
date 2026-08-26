// ioctl.cpp
#include <ntifs.h>

#include "ioctl.hpp"
#include "../../global.hpp"
#include "codes.hpp"

#include "../../enviroment/test_signing/test_signing.hpp"
#include "../../enviroment/secure_boot/secure_boot.hpp"
#include "../../enviroment/kernel_debugging/kernel_debugging.hpp"
#include "../../enviroment/hypervisor/hypervisor.hpp"
#include "../../enviroment/hvci/hvci.hpp"
#include "../../enviroment/vbs/vbs.hpp"

NTSTATUS IoctlHandler(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
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
            break;
        }

        case IOCTL_GET_GAME_PID: {
            if (outputSize < sizeof(HANDLE)) { status = STATUS_BUFFER_TOO_SMALL; break; }
            *(HANDLE*)buffer = ringo_context.ProtectedGamePid;
            bytesReturned = sizeof(HANDLE);
            break;
        }

        case IOCTL_GET_GAME_STATUS: {
            if (outputSize < sizeof(BOOLEAN)) { status = STATUS_BUFFER_TOO_SMALL; break; }
            *(BOOLEAN*)buffer = ringo_context.isGameOpen;
            bytesReturned = sizeof(BOOLEAN);
            break;
        }

        // (enviroment)
        case IOCTL_GET_HVCI_INFORMATION: {
            if (outputSize < sizeof(HVCI_CHECK)) { status = STATUS_BUFFER_TOO_SMALL; break; }

            HVCI_CHECK hc = isHVCIEnabled();
            RtlCopyMemory(buffer, &hc, sizeof(hc));
            bytesReturned = sizeof(hc);

            break;
        }

        case IOCTL_GET_HYPERVISOR_STATUS: {
            if (outputSize < sizeof(HYPERVISOR_CHECK)) { status = STATUS_BUFFER_TOO_SMALL; break; }

            HYPERVISOR_CHECK hc = getHypervisorInfo();
            RtlCopyMemory(buffer, &hc, sizeof(hc));
            bytesReturned = sizeof(hc);

            break;
        }

        case IOCTL_GET_KERNEL_DEBUGGING_STATUS: {
            if(outputSize < sizeof(KERNEL_DEBUGGING_CHECK)) { status = STATUS_BUFFER_TOO_SMALL; break; }

            KERNEL_DEBUGGING_CHECK kdc = isKernelDebuggerPresent();
            RtlCopyMemory(buffer, &kdc, sizeof(kdc));
            bytesReturned = sizeof(kdc);

            break;
        }

        case IOCTL_GET_SECURE_BOOT_INFORMATION: {
            if (outputSize < sizeof(SECURE_BOOT_CHECK)) { status = STATUS_BUFFER_TOO_SMALL; break; }

            SECURE_BOOT_CHECK sbc = isSecureBootEnabled();
            RtlCopyMemory(buffer, &sbc, sizeof(sbc));
            bytesReturned = sizeof(sbc);

            break;
        }

        case IOCTL_GET_TEST_SIGNING_MODE: {
            if (outputSize < sizeof(TEST_SIGNING_CHECK)) { status = STATUS_BUFFER_TOO_SMALL; break; }

            TEST_SIGNING_CHECK tsc = isTestSigningEnabled();
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
            break;
        }

        // (callbacks)
        case IOCTL_IS_CREATE_PROCESS_CALLBACK_SET: {
            if (outputSize < sizeof(BOOLEAN)) { status = STATUS_BUFFER_TOO_SMALL; break; }
            *(BOOLEAN*)buffer = ringo_context.Callbacks.isCreateProcessCallbackRegistered;
            bytesReturned = sizeof(BOOLEAN);
            break;
        }

        case IOCTL_IS_CREATE_THREAD_CALLBACK_SET: {
            if (outputSize < sizeof(BOOLEAN)) { status = STATUS_BUFFER_TOO_SMALL; break; }
            *(BOOLEAN*)buffer = ringo_context.Callbacks.isCreateThreadCallbackRegistered;
            bytesReturned = sizeof(BOOLEAN);
            break;
        }

        case IOCTL_IS_IMAGE_CALLBACK_SET: {
            if (outputSize < sizeof(BOOLEAN)) { status = STATUS_BUFFER_TOO_SMALL; break; }
            *(BOOLEAN*)buffer = ringo_context.Callbacks.isLoadImageCallbackRegistered;
            bytesReturned = sizeof(BOOLEAN);
            break;
        }

        case IOCTL_IS_OBCALLBACK_SET: {
            if (outputSize < sizeof(BOOLEAN)) { status = STATUS_BUFFER_TOO_SMALL; break; }
            *(BOOLEAN*)buffer = ringo_context.Callbacks.ObCallbackHandle != NULL;
            bytesReturned = sizeof(BOOLEAN);
            break;
        }

        // setters 
        case IOCTL_SET_GAME_NAME: {
            if (inputSize == 0 || inputSize >= sizeof(ringo_context.ProtectedGameProcessName)) { status = STATUS_INVALID_PARAMETER; break; }

            RtlZeroMemory(ringo_context.ProtectedGameProcessName, sizeof(ringo_context.ProtectedGameProcessName));
            RtlCopyMemory(ringo_context.ProtectedGameProcessName, buffer, inputSize);

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