# Project Status and Roadmap

This document describes what Ringo currently implements, where the implementation is incomplete or has known gaps, and what the planned development trajectory looks like.

---

## Current Capabilities

The driver's active protection layer is largely functional. Object callbacks on process and thread types strip dangerous handle access rights before they reach any caller, and the same callback mechanism extends to device objects belonging to known-vulnerable drivers. The four kernel notify routines — process, thread, image-load, and (implicitly) the BYOVD real-time path — are all registered at startup and collectively give the driver visibility into most normal cheat injection vectors.

The BYOVD detection system is fully operational. The blocklist ships as a compile-time array of roughly 2,000 drivers, hash-checked against both raw SHA-256 and Authentihash. Detection runs at startup against already-loaded modules and in real time as new drivers load. Detected drivers have their device objects reference-locked and tracked so the object callback layer can block handle acquisition.

The environment probe suite covers the seven platform security properties that matter most for trust assessment: HVCI, VBS, Secure Boot, test-signing, hypervisor presence (with spoof detection), kernel debugger attachment, and TPM version. The hardware identification system reads SMBIOS, CPUID, and physical disk serial number from kernel context, bypassing the user-mode paths cheats commonly spoof.

---

## Known Gaps

**Kernel-mode cheat bypass.** If a cheat operates from kernel mode — whether through a signed but exploitable driver (BYOVD) not in the current blocklist, or through a manually mapped driver that evades the big pool scanner — it can call `MmCopyVirtualMemory` directly or manipulate `EPROCESS` structures without ever touching the Object Manager. Object callbacks provide no defense against this.

**No in-memory signature scanning.** The driver does not scan game process memory for known cheat patterns, shellcode signatures, or inline hooks. This is a significant gap compared to production anti-cheat systems. Detection of injected code that successfully bypasses the image-load callback relies entirely on the thread-start-address signal and the big pool scanner.

**Unauthenticated IOCTL channel.** Any process that can open a handle to `\\.\ringo` can issue all IOCTL commands, including `IOCTL_SET_GAME_NAME`. There is no challenge-response authentication, no session token, and no DACL restricting access to the anti-cheat client process. In a production deployment this needs to be addressed before the channel can be considered secure.

**No HMAC or encryption on IOCTL traffic.** All data crosses the kernel/user boundary as plaintext structs. A malicious user-mode process with a handle to the device could issue queries or attempt to replay traffic.

**VAD and page-table blind spots.** Manually mapped code injected into the game process that does not call `NtMapViewOfSection` or `LdrLoadDll` does not trigger the image-load callback and is not currently detected. The planned VAD scanner and page-table walker would cover this.

---

## Incomplete Components

| Component | Location | Status |
|---|---|---|
| **Big pool scanner** | `manual_mapped_drivers/big_pool_track/big_pool_track.cpp` | Identifies unaccounted non-paged allocations but does not verify whether they contain executable code. PE header inspection and page-permission checks are needed to reduce false positives. |
| **Kernel space walk** | `manual_mapped_drivers/kernel_space_walking/kernel_space_walking.cpp` | Empty stub. Intended to traverse PML4 page tables to find unbacked executable kernel pages — a complementary method to the big pool approach. |
| **Suspicious path filter** | `callbacks/image_notify/image_notify.hpp` | The `EsRutaSospechosa` function (checking common temp/download paths) is defined but commented out and not connected to the detection pipeline. |
| **TPM direct probing** | `system/tpm/tpm.cpp` | Works on some Windows versions but may fail with `STATUS_INVALID_DEVICE_REQUEST` on newer builds that require routing TPM commands through TBS. |
| **Process callback heuristics** | `callbacks/create_process_notify/` | Currently only tracks the target game process. Could be extended to flag suspicious processes (debuggers, known injection tools) that appear while the game is running, though this may belong in a packer rather than the kernel driver. |

---

## Roadmap

**Usermode memory scanning.** A VAD (Virtual Address Descriptor) walker would enumerate allocations in the game process and flag `PAGE_EXECUTE_READWRITE` regions and regions not backed by a file on disk. Thread call stack walking would verify that return addresses on game threads point into legitimate module `.text` sections.

**Kernel space walk.** The PML4 traversal in `kernel_space_walking.cpp` would provide a second, independent path for detecting manually mapped kernel code — one that is harder to evade than big pool analysis alone.

**Big pool scanner hardening.** After enumerating suspicious allocations, the scanner should inspect each one for PE headers, executable section flags, and valid import tables. This would substantially reduce false positives and make the scanner actionable for automated enforcement rather than just logging.

**IOCTL channel hardening.** The device object should be created with a restrictive SDDL that limits access to the anti-cheat client's process token. A challenge-response handshake over the IOCTL channel would authenticate the client before accepting any commands, and an HMAC over response payloads would prevent replay. These are prerequisites before the system can be considered hardened for production use.

**BYOVD blocklist updates.** The current blocklist is a point-in-time snapshot of the loldrivers dataset. A mechanism to update it without rebuilding the driver — for example, reading a signed supplemental list from disk at load time — would allow the blocklist to be maintained without driver redeployment.
