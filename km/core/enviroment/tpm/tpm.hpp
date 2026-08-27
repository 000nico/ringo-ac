#pragma once

typedef enum _TPM_VERSION {
	TpmVersion1_2,
	TpmVersion2_0,
	TpmVersionNone
} TPM_VERSION;

struct TPM {
	TPM_VERSION TpmVersion;
};


TPM getTpmVersion();