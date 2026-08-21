#include "../../../sdk/imports/imports.hpp"
#include "test_signing.hpp"

BOOLEAN checkTestSigning(BOOLEAN* pTestSigningEnabled) {
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

TEST_SIGNING_CHECK isTestSigningEnabled() {
	TEST_SIGNING_CHECK tsc = { 0 };

	BOOLEAN testSigningEnabled;
	if (checkTestSigning(&testSigningEnabled)) {
		tsc.isEnabled = testSigningEnabled;
	}

	return tsc;
}