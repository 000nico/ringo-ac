#include <windows.h>

#include "get_env_info.hpp"

#include "../../../../km/core/comunication/ioctl/codes.hpp"

#include "../../../../km/core/enviroment/hvci/hvci.hpp"
#include "../../../../km/core/enviroment/hypervisor/hypervisor.hpp"
#include "../../../../km/core/enviroment/kernel_debugging/kernel_debugging.hpp"
#include "../../../../km/core/enviroment/secure_boot/secure_boot.hpp"
#include "../../../../km/core/enviroment/test_signing/test_signing.hpp"
#include "../../../../km/core/enviroment/vbs/vbs.hpp"
#include "../../../../km/core/enviroment/tpm/tpm.hpp"


DWORD getInformationClass(ENVIRONMENT_INFO_CLASS informationClass) {
    DWORD ioctlCode = 0;

    switch (informationClass) {
        case InfoHwid:
            ioctlCode = IOCTL_GET_HWID;
            break;
        case InfoGamePid:
            ioctlCode = IOCTL_GET_GAME_PID;
            break;
        case InfoGameStatus:
            ioctlCode = IOCTL_GET_GAME_STATUS;
            break;
        case InfoHvci:
            ioctlCode = IOCTL_GET_HVCI_INFORMATION;
            break;
        case InfoHypervisorStatus:
            ioctlCode = IOCTL_GET_HYPERVISOR_STATUS;
            break;
        case InfoKernelDebuggingStatus:
            ioctlCode = IOCTL_GET_KERNEL_DEBUGGING_STATUS;
            break;
        case InfoSecureBoot:
            ioctlCode = IOCTL_GET_SECURE_BOOT_INFORMATION;
            break;
        case InfoTestSigningMode:
            ioctlCode = IOCTL_GET_TEST_SIGNING_MODE;
            break;
        case InfoVbs:
            ioctlCode = IOCTL_GET_VBS_INFORMATION;
            break;
        case InfoTpm:
            ioctlCode = IOCTL_GET_TPM_INFORMATION;
            break;
        case InfoCreateProcessCallbackSet:
            ioctlCode = IOCTL_IS_CREATE_PROCESS_CALLBACK_SET;
            break;
        case InfoCreateThreadCallbackSet:
            ioctlCode = IOCTL_IS_CREATE_THREAD_CALLBACK_SET;
            break;
        case InfoImageCallbackSet:
            ioctlCode = IOCTL_IS_IMAGE_CALLBACK_SET;
            break;
        case InfoObCallbackSet:
            ioctlCode = IOCTL_IS_OBCALLBACK_SET;
            break;
        
        default:
            ioctlCode = 0;
            break;
    }

    return ioctlCode;
}

BOOLEAN GetEnvironmentInformation(HANDLE hDriver, ENVIRONMENT_INFO_CLASS informationClass, PVOID buffer, ULONG size, DWORD* bytesReturned) {
	if (!hDriver || hDriver == INVALID_HANDLE_VALUE || !buffer) return STATUS_INVALID_PARAMETER;

	DWORD ioctlCode = getInformationClass(informationClass);

	BOOL result = DeviceIoControl(hDriver, ioctlCode, NULL, 0, buffer, size, bytesReturned, NULL);

	return (BOOLEAN)result;
}