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
