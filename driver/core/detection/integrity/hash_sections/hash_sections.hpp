#pragma once
#include "ntdef.h"
#include "ntifs.h"
#include "sdk/crypto/hashes.hpp"
#include "sdk/pe/pe.hpp"
#include "sdk/sdk.hpp"
#include "core/global.hpp"
#include "wdm.h"

extern "C" PVOID NTAPI PsGetProcessSectionBaseAddress(PEPROCESS process);

bool hash_disk_game_image(UCHAR sha256[HASH_SIZE_SHA256]);
bool compare_hash(const UCHAR hash_1[HASH_SIZE_SHA256], const UCHAR hash_2[HASH_SIZE_SHA256]);
bool hash_process_image(UCHAR sha256[HASH_SIZE_SHA256]);