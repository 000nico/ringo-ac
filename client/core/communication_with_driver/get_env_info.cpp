#include <windows.h>

#include "communication_with_driver.hpp"

#include "../../../driver/core/communication/codes.hpp"

#include "../../../driver/core/system/system.hpp"
#include "../../../driver/core/identity/identity.hpp"


DWORD get_information_class(ENVIRONMENT_INFO_CLASS information_class) {
    DWORD ioctl_code = 0;

    switch (information_class) {
        case INFO_HWID:
            ioctl_code = IOCTL_GET_HWID;
            break;
        case INFO_GAME_PID:
            ioctl_code = IOCTL_GET_GAME_PID;
            break;
        case INFO_GAME_STATUS:
            ioctl_code = IOCTL_GET_GAME_STATUS;
            break;
        case INFO_HVCI:
            ioctl_code = IOCTL_GET_HVCI_INFORMATION;
            break;
        case INFO_HYPERVISOR_STATUS:
            ioctl_code = IOCTL_GET_HYPERVISOR_STATUS;
            break;
        case INFO_KERNEL_DEBUGGING_STATUS:
            ioctl_code = IOCTL_GET_KERNEL_DEBUGGING_STATUS;
            break;
        case INFO_SECURE_BOOT:
            ioctl_code = IOCTL_GET_SECURE_BOOT_INFORMATION;
            break;
        case INFO_TEST_SIGNING_MODE:
            ioctl_code = IOCTL_GET_TEST_SIGNING_MODE;
            break;
        case INFO_VBS:
            ioctl_code = IOCTL_GET_VBS_INFORMATION;
            break;
        case INFO_TPM:
            ioctl_code = IOCTL_GET_TPM_INFORMATION;
            break;
        case INFO_CREATE_PROCESS_CALLBACK_SET:
            ioctl_code = IOCTL_IS_CREATE_PROCESS_CALLBACK_SET;
            break;
        case INFO_CREATE_THREAD_CALLBACK_SET:
            ioctl_code = IOCTL_IS_CREATE_THREAD_CALLBACK_SET;
            break;
        case INFO_IMAGE_CALLBACK_SET:
            ioctl_code = IOCTL_IS_IMAGE_CALLBACK_SET;
            break;
        case INFO_OB_CALLBACK_SET:
            ioctl_code = IOCTL_IS_OBCALLBACK_SET;
            break;

        case INFO_HEARTBEAT:
            ioctl_code = IOCTL_GET_HEARTBEAT;
            break;

        case INFO_BYOVD_REPORT:
            ioctl_code = IOCTL_GET_BYOVD_REPORT;
            break;
        case INFO_MANUAL_MAPPED_REPORT:
            ioctl_code = IOCTL_GET_MANUAL_MAPPED_REPORT;
            break;
        case INFO_HASH_DIFF_REPORT:
            ioctl_code = IOCTL_GET_HASH_DIFF_REPORT;
            break;
        case INFO_FLAGGED_DLL_REPORT:
            ioctl_code = IOCTL_GET_FLAGGED_DLL_REPORT;
            break;
        case INFO_THREAD_REPORT:
            ioctl_code = IOCTL_GET_THREAD_REPORT;
            break;
        
        default:
            ioctl_code = 0;
            break;
    }

    return ioctl_code;
}

BOOLEAN get_environment_information(HANDLE h_driver, ENVIRONMENT_INFO_CLASS information_class, PVOID buffer, ULONG size, DWORD* bytes_returned) {
	if (!h_driver || h_driver == INVALID_HANDLE_VALUE || !buffer) return FALSE;

	DWORD ioctl_code = get_information_class(information_class);
	if (ioctl_code == 0) return FALSE;

	BOOL result = DeviceIoControl(h_driver, ioctl_code, NULL, 0, buffer, size, bytes_returned, NULL);

	return (BOOLEAN)result;
}