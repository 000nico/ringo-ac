#include "sdk/sdk.hpp"
#include "test_signing.hpp"

BOOLEAN check_test_signing(BOOLEAN* p_test_signing_enabled) {
	*p_test_signing_enabled = FALSE;

	SYSTEM_CODEINTEGRITY_INFORMATION sci = { 0 };
	sci.Length = sizeof(sci);

	ULONG return_length = 0;
	NTSTATUS status = ZwQuerySystemInformation(SystemCodeIntegrityInformation, &sci, sizeof(sci), &return_length);

	if (!NT_SUCCESS(status)) {
		return FALSE;
	}

	*p_test_signing_enabled = (sci.CodeIntegrityOptions & CODEINTEGRITY_OPTION_TESTSIGN) != 0;
	return TRUE;
}

TEST_SIGNING_CHECK is_test_signing_enabled() {
	TEST_SIGNING_CHECK tsc = {};

	BOOLEAN test_signing_enabled;
	if (check_test_signing(&test_signing_enabled)) {
		tsc.code_integrity.valid = TRUE;
		tsc.code_integrity.value = test_signing_enabled;
	}

	return tsc;
}