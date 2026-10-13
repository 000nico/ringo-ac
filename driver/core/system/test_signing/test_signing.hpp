#pragma once
#include "../system.hpp"

#ifdef _KERNEL_MODE
#include "sdk/sdk.hpp"
#else
#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS
#endif


#define SystemCodeIntegrityInformation 103
#define CODEINTEGRITY_OPTION_ENABLED     0x01
#define CODEINTEGRITY_OPTION_TESTSIGN    0x02

typedef struct _SYSTEM_CODEINTEGRITY_INFORMATION {
    ULONG Length;
    ULONG CodeIntegrityOptions;
} SYSTEM_CODEINTEGRITY_INFORMATION, * PSYSTEM_CODEINTEGRITY_INFORMATION;

struct TEST_SIGNING_CHECK {
    SOURCE_RESULT code_integrity;
};

static_assert(sizeof(TEST_SIGNING_CHECK) == sizeof(SOURCE_RESULT), "TEST_SIGNING_CHECK layout changed");

TEST_SIGNING_CHECK is_test_signing_enabled();