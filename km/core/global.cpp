#include "global.hpp"

LOADED_MODULE g_LoadedDlls[256] = { 0 };
volatile LONG g_LoadedDllsCount = 0;
LOADED_MODULE g_FlaggedDlls[256] = { 0 };
volatile LONG g_FlaggedDllsCount = 0;
CREATED_THREAD g_CreatedThreads[256] = { 0 };
volatile LONG g_CreatedThreadsCount = 0;
RINGO_CONTEXT ringo_context = { 0 };

PDRIVER_OBJECT g_DriverObject = NULL;