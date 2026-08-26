#pragma once
#include <windows.h>
#include <ntstatus.h>

NTSTATUS setGameProcessName(HANDLE hDriver, const char* name);