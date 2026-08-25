// ioctl.cpp
#include <ntifs.h>
#include "ioctl.hpp"
#include "../../global.hpp"
#include "codes.hpp"

NTSTATUS IoctlHandler(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);

    PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
    ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;
    ULONG inputSize = stack->Parameters.DeviceIoControl.InputBufferLength;
    ULONG outputSize = stack->Parameters.DeviceIoControl.OutputBufferLength;
    UNREFERENCED_PARAMETER(outputSize);
    PVOID buffer = Irp->AssociatedIrp.SystemBuffer;

    NTSTATUS status = STATUS_SUCCESS;
    ULONG_PTR bytesReturned = 0;

    switch (code) {
        // getters
        case IOCTL_GET_HWID: {
            break;
        }

        case IOCTL_GET_GAME_PID: {
            break;
        }

        case IOCTL_GET_GAME_STATUS: {
            break;
        }

        // (enviroment)
        case IOCTL_GET_HVCI_INFORMATION: {
            break;
        }

        case IOCTL_GET_HYPERVISOR_STATUS: {
            break;
        }

        case IOCTL_GET_KERNEL_DEBUGGING_STATUS: {
            break;
        }

        case IOCTL_GET_SECURE_BOOT_INFORMATION: {
            break;
        }

        case IOCTL_GET_TEST_SIGNING_MODE: {
            break;
        }

        case IOCTL_GET_VBS_INFORMATION: {
            break;
        }

        case IOCTL_GET_TPM_INFORMATION: {
            break;
        }

        // (callbacks)
        case IOCTL_IS_CREATE_PROCESS_CALLBACK_SET: {
            break;
        }

        case IOCTL_IS_CREATE_THREAD_CALLBACK_SET: {
            break;
        }

        case IOCTL_IS_IMAGE_CALLBACK_SET: {
            break;
        }

        case IOCTL_IS_OBCALLBACK_SET: {
            break;
        }

        // setters 
        case IOCTL_SET_GAME_NAME: {
            if (inputSize == 0) { status = STATUS_INVALID_PARAMETER; break; }
            ringo_context.ProtectedGameProcessName = (PUCHAR)buffer;
            break;
        }

        // driver
        case IOCTL_UNLOAD_DRIVER: {
            break;
        }

        default:
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = bytesReturned;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}