# Project Status and Roadmap

This document describes what Ringo currently implements, where the implementation is incomplete or has known gaps, and what the planned development trajectory looks like.

---

## Incomplete Components

| Component | Location | Status |
|---|---|---|
| **Big pool scanner** | `manual_mapped_drivers/big_pool_track/big_pool_track.cpp` | Identifies unaccounted non-paged allocations but does not verify whether they contain executable code. PE header inspection and page-permission checks are needed to reduce false positives. |
| **Suspicious path filter** | `callbacks/image_notify/image_notify.hpp` | The `EsRutaSospechosa` function (checking common temp/download paths) is defined but commented out and not connected to the detection pipeline. |
| **TPM direct probing** | `system/tpm/tpm.cpp` | Works on some Windows versions but may fail with `STATUS_INVALID_DEVICE_REQUEST` on newer builds that require routing TPM commands through TBS. |

---

## Roadmap

**Usermode memory scanning.** hashing .text sections and other sections

**Big pool scanner hardening.**  check valid import tables. This would substantially reduce false positives and make the scanner actionable for automated enforcement rather than just logging.

**IOCTL channel hardening.** The device object should be created with a restrictive SDDL that limits access to the anti-cheat client's process token. A challenge-response handshake over the IOCTL channel would authenticate the client before accepting any commands, and an HMAC over response payloads would prevent replay. These are prerequisites before the system can be considered hardened for production use.

detect execution outside real kernel with nmi non maskable interrupts
strip lsass and svchost (and others) handles at start by listing handles