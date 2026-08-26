#pragma once
#define CODEINTEGRITY_OPTION_HVCI_KMCI_ENABLED 0x400
#define SystemCodeIntegrityInformation 103

#include "../test_signing/test_signing.hpp"

struct HVCI_CHECK {
    BOOLEAN isEnabled;
};

HVCI_CHECK isHVCIEnabled();