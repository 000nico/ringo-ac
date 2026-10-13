#include "global.hpp"


RINGO_CONTEXT g_ringo_context = { 0 };

PDRIVER_OBJECT g_driver_object = NULL;
KEVENT g_stop_event = { 0 };