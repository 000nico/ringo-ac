#include "../../../driver/core/communication/codes.hpp"
#include "communication_with_driver.hpp"

#define FILE_HASH_SIZE 32 // same as HASH_SIZE_SHA256 in driver/sdk/crypto/hashes.hpp

NTSTATUS set_file_hash(HANDLE h_driver, const UCHAR* hash) {
	if (!h_driver || h_driver == INVALID_HANDLE_VALUE || !hash) return STATUS_INVALID_PARAMETER;

	DWORD bytes_returned = 0;
	bool result = DeviceIoControl(h_driver, IOCTL_SET_FILE_HASH, const_cast<UCHAR*>(hash), FILE_HASH_SIZE, NULL, 0, &bytes_returned, NULL);

	return result ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

NTSTATUS set_game_process_name(HANDLE h_driver, const char* name) {
	if (!h_driver || h_driver == INVALID_HANDLE_VALUE) return STATUS_INVALID_PARAMETER;
	
	DWORD name_length = static_cast<DWORD>(strlen(name) + 1);

	DWORD bytes_returned = 0;
	bool result = DeviceIoControl(h_driver, IOCTL_SET_GAME_NAME, const_cast<char*>(name), name_length, NULL, 0, &bytes_returned, NULL);

	return result ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}