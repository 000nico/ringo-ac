#pragma once

#ifdef _KERNEL_MODE
#include "../../sdk/sdk.hpp"
#else
#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS
#endif

struct SOURCE_RESULT {
    BOOLEAN valid;
    BOOLEAN value;
};

static_assert(sizeof(SOURCE_RESULT) == 2, "SOURCE_RESULT must remain two bytes");

#include "hvci/hvci.hpp"
#include "hypervisor/hypervisor.hpp"
#include "kernel_debugging/kernel_debugging.hpp"
#include "secure_boot/secure_boot.hpp"
#include "test_signing/test_signing.hpp"
#include "tpm/tpm.hpp"
#include "vbs/vbs.hpp"
