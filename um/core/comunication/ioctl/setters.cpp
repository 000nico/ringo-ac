#include "../../../../km/core/comunication/ioctl/codes.hpp"
#include "ioctl.hpp"

NTSTATUS setGameProcessName(HANDLE hDriver, const char* name) {
	if (!hDriver || hDriver == INVALID_HANDLE_VALUE) return STATUS_INVALID_PARAMETER;
	
	DWORD nameLength = static_cast<DWORD>(strlen(name) + 1);

	DWORD bytesReturned = 0;
	bool result = DeviceIoControl(hDriver, IOCTL_SET_GAME_NAME, const_cast<char*>(name), nameLength, NULL, 0, &bytesReturned, NULL);

	return result ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}