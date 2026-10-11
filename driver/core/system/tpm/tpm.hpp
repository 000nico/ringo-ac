#pragma once
#include "../system.hpp"

struct TPM_CHECK {
    SOURCE_RESULT tpm_2_0;
    SOURCE_RESULT tpm_1_2;
};

static_assert(sizeof(TPM_CHECK) == sizeof(SOURCE_RESULT) * 2, "TPM_CHECK layout changed");

TPM_CHECK get_tpm_version();