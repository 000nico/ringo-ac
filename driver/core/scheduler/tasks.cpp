#include "scheduler.hpp"
#include "core/detection/integrity/hash_sections/hash_sections.hpp"
#include "core/detection/manual_mapped_drivers/big_pool_track/big_pool_track.hpp"
#include "core/detection/manual_mapped_drivers/kernel_space_walking/kernel_space_walking.hpp"
#include "core/global.hpp"
#include <ntdef.h>
#include <ntifs.h>
#include <wdm.h>
#include "core/reporting/report_hash_diff/report_hash_diff.hpp"
#include "sdk/crypto/hashes.hpp"
#include "sdk/pe/pe.hpp"
#include "../detection/byovd/byovd_scan.hpp"
#include "tasks.hpp"

void hash_compare() {
    UCHAR process_image2[HASH_SIZE_SHA256];
    UCHAR disk_image[HASH_SIZE_SHA256];

    if (!g_ringo_context.game.is_game_open) return;

    if (!hash_disk_game_image(disk_image) || !hash_process_image(process_image2))
        return;

    if (!g_ringo_context.session.first_hash_catched) {
        RtlCopyMemory(g_ringo_context.session.last_hash, process_image2, HASH_SIZE_SHA256);
        g_ringo_context.session.first_hash_catched = TRUE;
    }

    if (!compare_hash(process_image2, g_ringo_context.session.last_hash))
        report_hash_mismatch(PROCESS_WITH_PROCESS);

    if (!compare_hash(process_image2, disk_image))
        report_hash_mismatch(FILE_WITH_PROCESS);

    RtlCopyMemory(g_ringo_context.session.last_hash, process_image2, HASH_SIZE_SHA256);
}