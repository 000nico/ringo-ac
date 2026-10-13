#pragma once
#include "sdk/sdk.hpp"
#include "core/reporting/report_byovd/report_byovd.hpp"
#include "core/reporting/report_manual_mapped_driver/report_manual_mapped_driver.hpp"
#include "core/reporting/report_hash_diff/report_hash_diff.hpp"
#include "core/reporting/report_flagged_dll/report_flagged_dll.hpp"
#include "core/reporting/report_thread_detection/report_thread_detection.hpp"

typedef struct RINGO_CONTEXT {
    struct {
        HANDLE protected_game_pid;
        UCHAR protected_game_process_name[64];
        BOOLEAN is_game_open;

        BOOLEAN first_hash_catched;
        UCHAR last_image_hash[HASH_SIZE_SHA256];
    } game;

    struct {
        struct {
            PVOID callback_handle;
        } ob;

        struct {
            BOOLEAN is_registered;
            PLOAD_IMAGE_NOTIFY_ROUTINE callback_ptr;
        } load_image;

        struct {
            BOOLEAN is_registered;
            PCREATE_THREAD_NOTIFY_ROUTINE callback_ptr;
        } create_thread;

        struct {
            BOOLEAN is_registered;
            PCREATE_PROCESS_NOTIFY_ROUTINE_EX callback_ptr;
        } create_process;
    } callbacks;

    // each one has both entries and a index counter
    struct {
        MANUAL_MAPPED_DRIVER_DETECTION_REPORT driver_detection_report;
        BYOVD_DETECTION_REPORT byovd_detection_report;
        FLAGGED_DLL_REPORT dll_detection_report;
        THREAD_DETECTION_REPORT thread_detection_report;
        HASH_DIFF_REPORT hash_diff_detection_report;
    } detections;

    struct {
        BOOLEAN active;
        ULONGLONG process_start_key;
        PEPROCESS process;

        UCHAR last_hash[HASH_SIZE_SHA256];

        struct {
            PEPROCESS banned_peprocess[128];
            INT index;
        } banned;
    } session;

    struct {
        volatile LONG scheduler_stop;
        HANDLE scheduler_thread_handle;
        ULONG random_seed;
    } scheduler;
} RINGO_CONTEXT;

extern RINGO_CONTEXT g_ringo_context;
extern PDRIVER_OBJECT g_driver_object;

extern UCHAR g_expected_file_hash[HASH_SIZE_SHA256];

extern KEVENT g_stop_event;
