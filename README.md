ringo is a modular anticheat for Windows

ringo is separed in three parts
  - kernel-mode: the core of the anti-cheat, collecting data from the system, setting up callbacks, etc
  - user-mode: the client that loads the driver and communicates with it, acts as a bridge of communication within kernel-mode and server
  - server: gets the encrypted data, analyzes it and decides what happens next

ringo lacks from a module (dll) loaded to the game on purpose, since its development started being meant for a CS2 server, and injecting a module could be detected from VAC. not having a module makes the anti-cheat not only for whole games, it also makes it work for community managed servers from games

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

  - vulnerable drivers:
    - ringo downloads a list of loldrivers.io with hashes, signatures and names, and uses two type of checks
    - at start, iterates through every driver loaded and check hash, signature and name
    - with callbacks checks every time an image is loaded

ringo checks its own and game integrity through:
  - periodically hashing and comparing executable sections
