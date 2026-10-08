ringo is a modular anticheat for Windows

ringo was made just for learning purposes. its also meant to be modular, with simple & readable code

ringo is separed in three parts
  - kernel-mode: the core of the anti-cheat, collecting data from the system, setting up callbacks, etc
  - user-mode: the client that loads the driver and communicates with it, acts as a bridge of communication within kernel-mode and server
  - server: gets the encrypted data, analyzes it and decides what happens next

ringo lacks from a module (dll) loaded to the game on purpose, since its development started being meant for a CS2 server, and injecting a module could be detected as malicious from VAC. not having a module makes the anti-cheat not only for whole games, it also makes it work for community managed servers from games.
ringo also lacks from self protection checks, since its a modular project and the responsability of anti-debugging, etc, is for a packer

what ringo does:
ringo collects data in multiple ways in order to detect spoofing, and collects:
  - hvci
  - vbs
  - secure boot
  - test signing mode
  - tpm state and version
  - hwid (smbios uuid, cpu hash, disk serial)
  - screen capturing
  - kernel debugging mode

ringo implements multiple red-pill techniques to detect an hypervisor or emulated environment
  - ZwQuerySystemInformation check
  - todo

ringo implements multiple callbacks in order to collect data and act in the fastest way possible
  - created/destroyed process hook
  - create/destroyed thread hook
  - created/destroyed image hook
  - object callbacks hook

ringo lacks from registry keys callbacks in purpose, since its not configured from them, because that would be one extra vector of attack

ringo detects kernel mode cheats, both manual mapped drivers and BYOVD
  - manual mapped drivers:
    - ringo does not focus on which tool was used for mapping the driver, it just focuses on the vectors of detection that raises when a manual mapped driver gets mapped
    - big pool tracking (with a whitelist)
    - kernel threads with a base address outside of a signed driver memory space
    - kernel threads with a return address stored in the stack that does not belong to a known memory space

  - vulnerable drivers:
    - ringo downloads a list of loldrivers.io with hashes, signatures and names, and uses two type of checks
    - at start, iterates through every driver loaded and check hash, signature and name
    - with callbacks checks every time an image is loaded

ringo checks its own and game integrity through:
  - periodically hashing and comparing executable sections (first hash is done to the disk, from there, hashes are done to the process, compared periodically)

ringo has a scheduler that decides randomly when and which functions are raised, in order to make patterns unpredictable, perhaps, having a single thread working makes the attack surface smaller

ringo protects its own IRP managers through:
  - having unique HANDLE, once a process has it, none other can open a HANDLE to the device, this is done via wdmsec.h, IOCreateDeviceSecure, Exclusive=TRUE, FILE_DEVICE_SECURE_OPEN
  - at the moment of opening the handle, ringo verifies the signature of the caller and its integrity
  - at the moment of answering an IRP, ringo verifies the caller, its integrity (so duplicate handle doesn't happen) through hashes and modules
  - before ringo answers any ioctl, usermode has to send through an specific ioctl the HMAC of a random nonce generated with BCryptGenRandom
  - a "Session" gets created: a struct that has info from the caller, a boolean that its true if it already answered to the nonce. the session gets cleaned in IRP_MJ_CLEANUP / IRP_MJ_CLOSE and when usermode process dies (PsSetCreateProcessNotifyRoutineEx)
  - if a wrong ioctl is called, the driver will close the session and refuse to open a new one, to avoid fuzzing.

ringo its not only a driver, it also has its own server, also coded in c++
ringo's server has its own db which its separated from any game db. Its for persistency in user's data. 
reports are separated by
  - reports: raw data that gets in the server, what came, without processing anything. for example:
      - 'the HWID "5vDJD234154nffwFWDA" has a thread that its running outside any valid driver address space'
  - verdicts: the conclusion archieved after processing the data from one or more reports, for example:
    -  'the HWID "5vDJD234154nffwFWDA" has a risk_score 75, its now flagged"

there are many reports per hwid, but only one report that gets modified per hwid
the consumers are different. The analisis engine reads reports to calculate verdicts, the game endpoint only reads veredicts

the analysis is for every data, the server checks the consistency between the different ways to get info of the system (like secure boot, vbs, hvci, etc) to check spoofing, and also analyzes the risk. For example, having a big pool outside a valid driver, its not big enough to flag if the driver didn't found any thread outside of the memory. a big bool and a thread outside valid memory, its already sus, but a big pool and a thread inside of that big pool is so sus.

the risk of a user being a cheater decays with time

POST /telemetry
GET /status/{hwid}

ringo known limitations:
  - a cheat using a byovd that its not listed could not be detected

things planned to add:
while doing this is exciting, and i have a lot ideas of my own to add, i dont have time to implement them right
now, however:
  - detect overlays by enumerating windows that are either TOP_MOST or created via CreateWindowInBand
  - detect communication channels between drivers and um
  - hook IRP_MJ_DEVICE_CONTROL enumerating drivers with PsSetLoadImageNotifyRoutine  and:
      - detect sussy buffer lenghts (for example, { PVOID address; SIZE_T size; PVOID buffer; } is 24 bytes)
      - enumerate and diff constantly PsLoadedModuleList
      - avoid unsigned um process to get handles of recently loaded drivers (filter with not multimedia / hid)
      - monitor MmMapIoSpace or physical memory access
      - inspect IRP stack with IoGetCurrentIrpStackLocation
   - use NMI callbacks to inspect stack of kernel threads

IMPORTANT: ringo is free for non-profit use. if you want to use this for commercial purposes, contact me
