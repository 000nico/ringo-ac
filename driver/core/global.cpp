#include "global.hpp"


RINGO_CONTEXT g_ringo_context = { 0 };

PDRIVER_OBJECT g_driver_object = NULL;
KEVENT g_stop_event = { 0 };

UCHAR g_expected_file_hash[HASH_SIZE_SHA256] = { 0 };