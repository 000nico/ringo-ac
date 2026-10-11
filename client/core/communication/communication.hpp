#pragma once
#include <windows.h>
#include <ntstatus.h>

NTSTATUS set_game_process_name(HANDLE hDriver, const char* name);