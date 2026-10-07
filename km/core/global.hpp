#pragma once
#include "sdk/crypto/hashes.hpp"
#include "sdk/sdk.hpp"
#include "reporting/report_byovd/report_byovd.hpp"
#include "callbacks/image_notify/image_notify.hpp"
#include "callbacks/create_thread_notify/create_thread_notify.hpp"
#include "reporting/report_manual_mapped_driver/report_manual_mapped_driver.hpp"

typedef struct RINGO_CONTEXT {
    struct {
        HANDLE protected_game_pid;
        UCHAR protected_game_process_name[64];
        BOOLEAN is_game_open;
    } Game;

    struct {
        struct {
            PVOID callback_handle;
        } Ob;

        struct {
            BOOLEAN is_registered;
            PLOAD_IMAGE_NOTIFY_ROUTINE callback_ptr;
        } LoadImage;

        struct {
            BOOLEAN is_registered;
            PCREATE_THREAD_NOTIFY_ROUTINE callback_ptr;
        } CreateThread;

        struct {
            BOOLEAN is_registered;
            PCREATE_PROCESS_NOTIFY_ROUTINE_EX callback_ptr;
        } CreateProcess;
    } Callbacks;

    struct {
        MANUAL_MAPPED_DRIVER_DETECTION_REPORT driver_detection_report;
        BYOVD_DETECTION_REPORT byovd_detection_report;
        FLAGGED_DLL_REPORT dll_detection_report;
        THREAD_DETECTION_REPORT thread_detection_report;
    } Detections;

    struct {
        BOOLEAN active;
        ULONGLONG process_start_key;
        PEPROCESS process;
        UCHAR last_hash[HASH_SIZE_SHA256];
    } Session;
} RINGO_CONTEXT;

extern RINGO_CONTEXT g_ringo_context;
extern PDRIVER_OBJECT g_DriverObject;

static const UCHAR g_expected_process_hash[HASH_SIZE_SHA256] = {
    0x00, 0x00,
};

static const UCHAR g_expected_file_hash[HASH_SIZE_SHA256] = {
    0x00, 0x00,
};