#pragma once
#include "../system.hpp"
#define CODEINTEGRITY_OPTION_HVCI_KMCI_ENABLED 0x400
#define SystemCodeIntegrityInformation 103

#include "../test_signing/test_signing.hpp"

struct HVCI_CHECK {
    SOURCE_RESULT code_integrity;
};

static_assert(sizeof(HVCI_CHECK) == sizeof(SOURCE_RESULT), "HVCI_CHECK layout changed");

HVCI_CHECK is_hvci_enabled();