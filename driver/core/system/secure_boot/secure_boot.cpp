#include <initguid.h> 
#include "secure_boot.hpp"
#include "sdk/sdk.hpp"

DEFINE_GUID(EFI_GLOBAL_VARIABLE_GUID,
	0x8be4df61, 0x93ca, 0x11d2, 0xaa, 0x0d, 0x00, 0xe0, 0x98, 0x03, 0x2b, 0x8c);

BOOLEAN zwQueryCheck(_SYSTEM_SECUREBOOT_INFORMATION* psbi) {
	_SYSTEM_SECUREBOOT_INFORMATION sbi = { 0 };
	ULONG returnLenght = 0;

	NTSTATUS status = ZwQuerySystemInformation(SystemSecureBootInformation, &sbi, sizeof(sbi), &returnLenght);

	if (!NT_SUCCESS(status))
		return FALSE; // not supported

	else {
		*psbi = sbi;
		return TRUE;
	}
}

BOOLEAN uefiCheck(BOOLEAN* pEnabled) {
	UNICODE_STRING varName;
	RtlInitUnicodeString(&varName, L"SecureBoot");

	UCHAR value = 0;
	ULONG valueLenght = sizeof(value);
	ULONG attributes = 0;

	NTSTATUS status = ExGetFirmwareEnvironmentVariable(&varName, (LPGUID)&EFI_GLOBAL_VARIABLE_GUID, &value, &valueLenght, &attributes);

	if (!NT_SUCCESS(status)) {
		return FALSE; // no uefi or not existing variable, or legacy bios
	}
	else {
		*pEnabled = (value != 0);
		return TRUE;
	}
}

BOOLEAN registryKeyCheck(BOOLEAN* pEnabled)
{
	*pEnabled = FALSE;

	UNICODE_STRING keyPath;
	RtlInitUnicodeString(&keyPath, L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\SecureBoot\\State");

	OBJECT_ATTRIBUTES objAttr;
	InitializeObjectAttributes(&objAttr, &keyPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

	HANDLE handle;
	NTSTATUS status = ZwOpenKey(&handle, KEY_READ, &objAttr);
	if (!NT_SUCCESS(status)) {
		return FALSE;
	}

	UNICODE_STRING valueName;
	RtlInitUnicodeString(&valueName, L"UEFISecureBootEnabled");

	UCHAR buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG)];
	PKEY_VALUE_PARTIAL_INFORMATION pInfo = (PKEY_VALUE_PARTIAL_INFORMATION)buffer;
	ULONG resultLength = 0;

	status = ZwQueryValueKey(handle, &valueName, KeyValuePartialInformation, buffer, sizeof(buffer), &resultLength);
	ZwClose(handle);

	if (NT_SUCCESS(status) && pInfo->Type == REG_DWORD && pInfo->DataLength == sizeof(ULONG)) {
		ULONG value = *(PULONG)pInfo->Data;
		*pEnabled = (value != 0);
		return TRUE;
	}

	return FALSE;
}

SECURE_BOOT_CHECK is_secure_boot_enabled() {
	SECURE_BOOT_CHECK sbc = {};

	_SYSTEM_SECUREBOOT_INFORMATION sbi = { 0 };
	if (zwQueryCheck(&sbi)) {
		sbc.nt_query_enabled.valid = TRUE;
		sbc.nt_query_enabled.value = sbi.SecureBootEnabled;
		sbc.nt_query_capable.valid = TRUE;
		sbc.nt_query_capable.value = sbi.SecureBootCapable;
	}

	BOOLEAN uefiCheckEnabled;
	if (uefiCheck(&uefiCheckEnabled)) {
		sbc.uefi_var.valid = TRUE;
		sbc.uefi_var.value = uefiCheckEnabled;
	}

	BOOLEAN regCheckEnabled;
	if (registryKeyCheck(&regCheckEnabled)) {
		sbc.registry.valid = TRUE;
		sbc.registry.value = regCheckEnabled;
	}

	return sbc;
}
