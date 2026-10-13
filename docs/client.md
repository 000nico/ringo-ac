# ringo client

The user-mode component of ringo (`ringo_client.exe`). It loads the kernel driver, asks it for data and forwards that data to the server. It contains no detection logic.

## Contents

1. [Introduction](#1-introduction)
2. [Layout](#2-layout)
3. [Loading the driver](#3-loading-the-driver)
4. [Talking to the driver](#4-talking-to-the-driver)
   - 4.1. [Information classes](#41-information-classes)
   - 4.2. [Querying a struct](#42-querying-a-struct)
   - 4.3. [Setters](#43-setters)
   - 4.4. [Detection reports](#44-detection-reports)
5. [Talking to the server](#5-talking-to-the-server)
   - 5.1. [Environment report](#51-environment-report)
   - 5.2. [Module report](#52-module-report)
   - 5.3. [Serialization](#53-serialization)
   - 5.4. [Sending](#54-sending)
6. [Scheduler](#6-scheduler)
7. [Configuration](#7-configuration)
8. [Trust model](#8-trust-model)
9. [Known limitations and planned work](#9-known-limitations-and-planned-work)
10. [Building](#10-building)
11. [Acronyms](#11-acronyms)

## 1. Introduction

The client sits between the driver and the server and does three jobs:

1. **Load** the driver as a kernel service and open its device.
2. **Collect** what the driver knows (the environment and the detection reports).
3. **Forward** it to the server as JSON.

```
driver (kernel)                client (user)                      server
      |                             |                               |
      |<--- DeviceIoControl --------|                               |
      |---- C structs ------------->|                               |
      |                             |-- serialize to JSON --------->|
      |                             |      POST /telemetry/...      |
```

*Figure 1. Data flow.*

The client does not interpret what it carries. It never decides that a machine is suspicious, and it never edits a value on the way through. This is deliberate: the client runs on the machine of the player, so it is the least trustworthy part of the chain, and everything it carries is analysed by the server as untrusted input (see 8).

## 2. Layout

```
client/
  main.cpp                             entry point
  server_info.hpp                      server host and port
  core/
    driver/                            loads the driver service, opens the device
    communication_with_driver/         IOCTL wrappers, setters, report readers
    communication_with_server/         collect, serialize and send
    scheduler/                         periodic loop
```

The client includes headers from `driver/` directly (`codes.hpp`, `system/`, `identity/`, `reporting/`). The structs are therefore defined once, and the driver and the client can never disagree on a layout.

## 3. Loading the driver

`open_driver()` does everything needed to reach the device:

```
resolve the path of rngdrv.sys   (same folder as the executable)
OpenSCManager
remove an old "RingoDriver" service if it exists (stop it first, then delete)
CreateService(SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START)
StartService
CreateFile(\\.\ringo)
```

Removing the previous service first means a new build always replaces an old one, which keeps iteration simple during development. The client needs administrator rights both to create the service and to open the device, whose ACL only allows `SYSTEM` and administrators.

If `CreateFile` fails with access denied, the driver refused the caller: its hash does not match the expected one, or the process was banned (see the driver documentation, section 4).

## 4. Talking to the driver

### 4.1. Information classes

Instead of exposing IOCTL codes everywhere, the client uses an enum and one function that maps it:

```cpp
enum ENVIRONMENT_INFO_CLASS { INFO_HWID = 1, INFO_GAME_PID, INFO_HVCI, INFO_SECURE_BOOT, ... };

BOOLEAN get_environment_information(HANDLE h_driver, ENVIRONMENT_INFO_CLASS cls,
                                    PVOID buffer, ULONG size, DWORD* bytes_returned);
```

`get_information_class` translates the class to its `IOCTL_*` code, and `get_environment_information` is a thin `DeviceIoControl` around it. An unknown class returns `FALSE` without touching the driver.

### 4.2. Querying a struct

Every check returns a plain struct, and the client reads it straight into the same type the driver wrote:

```cpp
template <typename T>
static bool query(HANDLE drv, ENVIRONMENT_INFO_CLASS cls, T& out) {
    DWORD returned = 0;
    return get_environment_information(drv, cls, &out, sizeof(out), &returned) != FALSE;
}

SECURE_BOOT_CHECK sb = {};
query(drv, INFO_SECURE_BOOT, sb);
```

Zero-initialising the struct first matters: a field that was not filled stays `valid = 0`, which is exactly what "this source could not be queried" means.

### 4.3. Setters

| Function | IOCTL | Purpose |
|---|---|---|
| `set_game_process_name(drv, "cs2.exe")` | `IOCTL_SET_GAME_NAME` | Tells the driver which process to protect |
| `set_file_hash(drv, hash)` | `IOCTL_SET_FILE_HASH` | Provisions the `.text` hash the driver expects from the client |

### 4.4. Detection reports

`get_reports()` reads the five detection reports of the driver into one `DETECTION_REPORTS`:

```cpp
struct DETECTION_REPORTS {
    BYOVD_DETECTION_REPORT                byovd;
    MANUAL_MAPPED_DRIVER_DETECTION_REPORT manual_mapped;
    HASH_DIFF_REPORT                      hash_diff;
    FLAGGED_DLL_REPORT                    flagged_dll;
    THREAD_DETECTION_REPORT               threads;
};
```

These structs are large (each holds 256 entries with paths), so the client writes them directly into the destination instead of copying them through an intermediate buffer.

## 5. Talking to the server

### 5.1. Environment report

`collect_environment()` fills an `ENVIRONMENT_DATA` with one query per check:

```cpp
struct ENVIRONMENT_DATA {
    SECURE_BOOT_CHECK secure_boot;
    HVCI_CHECK hvci;
    VBS_CHECK vbs;
    HYPERVISOR_CHECK hypervisor;
    KERNEL_DEBUGGING_CHECK kernel_debugging;
    TEST_SIGNING_CHECK test_signing;
    TPM_CHECK tpm;
    HWID hwid;
};
```

The report is sent **whole**. Every check carries the answer of every source, and the server needs all of them from the same moment in order to compare them. Sending one boolean per check, or one request per check, would throw away the information the server uses to detect spoofing, and it would allow values from different instants to be compared against each other.

### 5.2. Module report

`collect_module_data()` builds a `MODULE_DATA`: the HWID as a string, a timestamp, and the five reports exactly as the driver returned them.

### 5.3. Serialization

The structs come from the kernel project, which cannot depend on a JSON library. All the JSON code lives in `serialize.hpp`, in the client.

Plain checks use the macro that only generates the struct-to-JSON direction (the client never parses these):

```cpp
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_ONLY_SERIALIZE(SOURCE_RESULT, valid, value)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_ONLY_SERIALIZE(SECURE_BOOT_CHECK,
    nt_query_enabled, nt_query_capable, uefi_var, registry)
```

Adding a new check is one line with its fields, plus its name in `ENVIRONMENT_DATA`.

Three cases need hand written code:

- **HWID** becomes a string: `<cpu_hash>-<disk_serial>-<smbios_uuid in hex>`.
- **Detection reports** hold fixed arrays with a `count`. Only the first `count` entries (clamped to the array size) are written, as `{ "count": n, "entries": [ ... ] }`.
- **Pointers** (`suspicious_address`, `device_object`, ...) are written as 64-bit numbers. They are identifiers for the server to compare ranges, not addresses meant to be dereferenced.

`BOOLEAN` is an `unsigned char`, so `valid` and `value` appear as `0` / `1`.

Example of an environment report (abbreviated):

```json
{
  "secure_boot": {
    "nt_query_enabled": { "valid": 1, "value": 1 },
    "nt_query_capable": { "valid": 1, "value": 1 },
    "uefi_var":         { "valid": 1, "value": 1 },
    "registry":         { "valid": 1, "value": 1 }
  },
  "hvci":       { "code_integrity": { "valid": 1, "value": 0 } },
  "hypervisor": { "nt_query": { "valid": 1, "value": 1 }, "cpuid": { "valid": 1, "value": 1 } },
  "hwid": "3472910-88213-A1B2C3D4E5F60718293A4B5C6D7E8F90"
}
```

### 5.4. Sending

```
POST /telemetry/environment   body: ENVIRONMENT_DATA as JSON
POST /telemetry/module        body: MODULE_DATA as JSON
```

Both use `httplib::Client` (the same library the server uses) with a 3 second connection timeout and a 5 second read timeout, and succeed only on HTTP 200. A failed send is only logged: the next iteration of the scheduler collects a fresh snapshot and tries again, so nothing needs to be queued.

## 6. Scheduler

`run_scheduler()` is the main loop of the client:

```
every ~120 seconds (+/- 20 seconds of jitter):
    collect_environment  -> send_environment_report
    collect_module_data  -> send_module_report   only if any report has count > 0
    Sleep
```

- **Jitter.** The interval is random inside a window so the traffic has no regular pattern.
- **Module reports only when there is something to say.** A clean machine sends an environment snapshot and nothing else.
- Driver-side reports are cumulative (the driver never clears them), so while a detection exists the same data is sent again every iteration. The server deduplicates (see [server.md](server.md)).

## 7. Configuration

The server address is a compile-time constant in `server_info.hpp`:

```cpp
constexpr const char* SERVER_HOST = "127.0.0.1";
constexpr int         SERVER_PORT = 8080;
```

`constexpr` gives the constants internal linkage, so including the header from several files does not violate the one definition rule. The defaults point to a local server, which is what is used while developing.

The game process name (`cs2.exe`) is currently hardcoded in `main.cpp`.

## 8. Trust model

The client is **not trusted**. It runs in the player's session, and the player controls it. It can be debugged, patched, replaced by a script that speaks the same protocol, or simply killed. The design accepts that and limits what it can do:

- It cannot grant itself access to the driver: the driver verifies the hash of the process on every request.
- It cannot change what the driver observed in a way that would not be inconsistent: the server cross-checks the sources of every report.
- It can drop or delay reports. That is why liveness (the heartbeat) is analysed by the server.

The client never makes a decision from the data it carries. If a check were done here, a modified client would simply skip it.

## 9. Known limitations and planned work

- **No transport security yet.** The client talks plain HTTP to the server. HTTPS (with a pinned certificate) is the next step before any non-local use.
- **Payloads are not authenticated.** Reports are not signed, so a replaced client can send any content. The planned fix is authenticating the data between the driver and the server end to end, with this client as an opaque relay.
- **The heartbeat is not forwarded** to the server yet.
- **Blocking sends.** Requests are made from the scheduler thread, so a slow server delays the next collection.
- **Configuration.** Host, port and game name are compile-time, and could move to a config file.
- **`set_file_hash` is not called** by the client yet, so the driver accepts any administrator until a hash is provisioned.

## 10. Building

See [build-windows.md](build-windows.md). The client is built with MSVC and CMake, with `nlohmann/json` and `cpp-httplib` fetched by `FetchContent`:

```powershell
cmake --build build --config Release --target client
```

The output is `build/bin/ringo_client.exe`. It must be placed next to `rngdrv.sys` and run as administrator.

## 11. Acronyms

- **IOCTL** - I/O control code.
- **HWID** - hardware identifier.
- **ACL** - access control list.
- **HTTP / HTTPS** - hypertext transfer protocol (secure).
- **JSON** - JavaScript object notation.
- **ODR** - one definition rule.
