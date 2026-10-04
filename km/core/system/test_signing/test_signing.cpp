#include "sdk/sdk.hpp"
#include "test_signing.hpp"

BOOLEAN check_test_signing(BOOLEAN* pTestSigningEnabled) {
	*pTestSigningEnabled = FALSE;

	SYSTEM_CODEINTEGRITY_INFORMATION sci = { 0 };
	sci.Length = sizeof(sci);

	ULONG returnLenght = 0;
	NTSTATUS status = ZwQuerySystemInformation(SystemCodeIntegrityInformation, &sci, sizeof(sci), &returnLenght);

	if (!NT_SUCCESS(status)) {
		return FALSE;
	}

	*pTestSigningEnabled = (sci.CodeIntegrityOptions & CODEINTEGRITY_OPTION_TESTSIGN) != 0;
	return TRUE;
}

TEST_SIGNING_CHECK is_test_signing_enabled() {
	TEST_SIGNING_CHECK tsc = {};

	BOOLEAN testSigningEnabled;
	if (check_test_signing(&testSigningEnabled)) {
		tsc.code_integrity.valid = TRUE;
		tsc.code_integrity.value = testSigningEnabled;
	}

	return tsc;
}