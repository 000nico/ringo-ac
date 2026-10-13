#include "core/callbacks/ob_callback/ob_callback.hpp"
#include "core/callbacks/image_notify/image_notify.hpp"
#include "core/callbacks/create_thread_notify/create_thread_notify.hpp"
#include "core/callbacks/create_process_notify/create_process_notify.hpp"
#include "core/global.hpp"
#include "core/communication/communication.hpp"
#include "core/scheduler/scheduler.hpp"
#include <wdm.h>

void driver_unload(PDRIVER_OBJECT DriverObject) {
    UNREFERENCED_PARAMETER(DriverObject);

    if (g_ringo_context.callbacks.ob.callback_handle != NULL)
        ObUnRegisterCallbacks(g_ringo_context.callbacks.ob.callback_handle);

    if (g_ringo_context.callbacks.load_image.is_registered == TRUE)
        PsRemoveLoadImageNotifyRoutine(g_ringo_context.callbacks.load_image.callback_ptr);

    if (g_ringo_context.callbacks.create_thread.is_registered == TRUE)
        PsRemoveCreateThreadNotifyRoutine(g_ringo_context.callbacks.create_thread.callback_ptr);

    if (g_ringo_context.callbacks.create_process.is_registered == TRUE)
        PsSetCreateProcessNotifyRoutineEx(g_ringo_context.callbacks.create_process.callback_ptr, TRUE);

    cleanup_device();

    KdPrint(("Ringo Driver Unloaded\n"));
}

extern "C"
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
    UNREFERENCED_PARAMETER(RegistryPath);
    KdPrint(("Ringo Driver Loaded\n"));

    g_ringo_context.scheduler.scheduler_stop = FALSE;
    
    // load callbacks
    g_ringo_context.callbacks.ob.callback_handle = ob_register_callbacks_open_process();
    if (g_ringo_context.callbacks.ob.callback_handle == NULL) {
        KdPrint(("[-] ob_callback_handle not loaded.\n"));
    }

    set_load_image_callback();
    if (g_ringo_context.callbacks.load_image.is_registered == FALSE) {
        KdPrint(("[-] SetLoadImage Callback not loaded.\n"));
    }

    set_create_thread_callback();
    if (g_ringo_context.callbacks.create_thread.is_registered == FALSE) {
        KdPrint(("[-] create_thread_callback not loaded.\n"));
    }

    set_create_process_callback();
    if (g_ringo_context.callbacks.create_process.is_registered == FALSE) {
        KdPrint(("[-] CreateProcessCallback not loaded.\n"));
    }

    if (!NT_SUCCESS(setup_device(DriverObject))) {
        KdPrint(("[-] IOCTL setup not loaded\n"));
    }

    KeInitializeEvent(&g_stop_event, NotificationEvent, FALSE);
    
    PsCreateSystemThread(
        &g_ringo_context.scheduler.scheduler_thread_handle,
        THREAD_ALL_ACCESS, 
        NULL, 
        NULL, 
        NULL, 
        scheduler, 
        NULL
    );

    ZwClose(g_ringo_context.scheduler.scheduler_thread_handle);
    g_ringo_context.scheduler.scheduler_thread_handle = NULL;
    
    DriverObject->DriverUnload = driver_unload;

    return STATUS_SUCCESS;
}