#pragma once

#include "sdk/sdk.hpp"

static const ULONG KNOWN_LEGIT_POOL_TAGS[] = {

    // ==========================================
    // NDIS / Network Driver Interface
    // ==========================================
    'TDtN',  // NdisMAllocateSharedMemory
    'RDtN',  // NDIS receive
    'BDtN',  // NDIS buffer descriptor
    'MDtN',  // NDIS miniport
    'NDtN',  // NDIS general
    '1DtN',  // NDIS
    '2DtN',  // NDIS
    '3DtN',  // NDIS
    '4DtN',  // NDIS
    '5DtN',  // NDIS
    '6DtN',  // NDIS
    'aDtN',  // NDIS
    'bDtN',  // NDIS
    'cDtN',  // NDIS
    'dDtN',  // NDIS
    'eDtN',  // NDIS
    'fDtN',  // NDIS
    'nDtN',  // NDIS NBL
    'tDtN',  // NDIS timer
    'lDtN',  // NDIS lookaside
    'pDtN',  // NDIS
    'sDtN',  // NDIS
    'FDtN',  // NDIS filter
    'LDtN',  // NDIS
    'TnIL',  // NDIS LWF (Lightweight Filter)
    'RFtN',  // NDIS receive filter
    'NFtN',  // NDIS filter
    'SItN',  // NDIS
    'rFdN',  // NDIS
    'nFdN',  // NDIS

    // ==========================================
    // TCP/IP Stack
    // ==========================================
    'tBpT',  // TCPIP
    'oBpT',  // TCPIP
    'lBpT',  // TCPIP
    'rBpT',  // TCPIP
    'PBpT',  // TCPIP packet
    'nBpT',  // TCPIP NBL context
    'aBpT',  // TCPIP
    'bBpT',  // TCPIP
    'cBpT',  // TCPIP
    'dBpT',  // TCPIP
    'eBpT',  // TCPIP endpoint
    'fBpT',  // TCPIP
    'gBpT',  // TCPIP
    'hBpT',  // TCPIP
    'mBpT',  // TCPIP
    'sBpT',  // TCPIP
    'uBpT',  // TCPIP
    'vBpT',  // TCPIP
    'TpcA',  // TCPIP
    'Tcpc',  // TCPIP connection
    'UdpA',  // UDP
    'Udpc',  // UDP connection
    'RawA',  // Raw socket
    'TCPr',  // TCPIP receive
    'TCPs',  // TCPIP send
    'IPv4',  // IPv4
    'IPv6',  // IPv6
    'IpFl',  // IP filter
    'IpSe',  // IP security
    'IpNb',  // IP neighbor
    'IpRt',  // IP route
    'Arp4',  // ARP
    'Icmp',  // ICMP

    // ==========================================
    // WFP - Windows Filtering Platform
    // ==========================================
    'klfW',  // WFP
    'elfW',  // WFP
    'AlfW',  // WFP ale
    'BlfW',  // WFP
    'ClfW',  // WFP callout
    'DlfW',  // WFP
    'FlfW',  // WFP filter
    'GlfW',  // WFP
    'IlfW',  // WFP
    'KlfW',  // WFP
    'LlfW',  // WFP layer
    'MlfW',  // WFP
    'NlfW',  // WFP
    'PlfW',  // WFP
    'RlfW',  // WFP
    'SlfW',  // WFP sublayer
    'TlfW',  // WFP
    'WlfW',  // WFP
    'fepW',  // WFP endpoint
    'cipW',  // WFP

    // ==========================================
    // Memory Manager (Mm)
    // ==========================================
    'bSmM',  // Mm section
    'lSmM',  // Mm
    'tSmM',  // Mm
    'VSmM',  // Mm VAD
    'ASmM',  // Mm
    'PSmM',  // Mm paging
    'MmSt',  // Mm store
    'MmCi',  // Mm
    'MmPl',  // Mm
    'MmRs',  // Mm
    'MmLd',  // Mm loader
    'MmCa',  // Mm cache
    'Mmxx',  // Mm
    'MmPp',  // Mm prototype PTE
    'MmAd',  // Mm address
    'MmHp',  // Mm heap
    'MmSh',  // Mm shared
    'MmIn',  // Mm
    'MmPa',  // Mm page
    'MmSe',  // Mm section

    // ==========================================
    // Cache Manager (Cc)
    // ==========================================
    'lCcC',  // Cache manager
    'bCcC',  // Cache
    'sCcC',  // Cache
    'vCcC',  // Cache vacb
    'BCcC',  // Cache BCB
    'mCcC',  // Cache
    'wCcC',  // Cache write
    'rCcC',  // Cache read
    'dCcC',  // Cache dirty
    'pCcC',  // Cache

    // ==========================================
    // I/O Manager (Io)
    // ==========================================
    'mNoI',  // IO manager
    'pIoI',  // IO
    'qIoI',  // IO queue
    'rIoI',  // IO
    'IoCo',  // IO completion
    'IoPn',  // IO plug and play
    'IoNm',  // IO name
    'IoDv',  // IO device
    'IoIr',  // IO IRP
    'IoFi',  // IO file object
    'IoBu',  // IO buffer
    'IrpA',  // IRP (allocate)
    'IrpS',  // IRP stack

    // ==========================================
    // NTFS
    // ==========================================
    'NtfF',  // NTFS FCB
    'NtfS',  // NTFS SCB
    'NtfC',  // NTFS CCB
    'NtfI',  // NTFS index
    'NtfL',  // NTFS log
    'NtfB',  // NTFS buffer
    'NtfR',  // NTFS
    'NtfA',  // NTFS attribute
    'NtfN',  // NTFS
    'NtfX',  // NTFS
    'NtfV',  // NTFS volume
    'NtfK',  // NTFS
    'NtfM',  // NTFS MFT

    // ==========================================
    // FAT / exFAT
    // ==========================================
    'FatF',  // FAT FCB
    'FatS',  // FAT SCB
    'FatC',  // FAT CCB
    'FatN',  // FAT
    'FatI',  // FAT
    'FatD',  // FAT directory
    'exFT',  // exFAT
    'exFC',  // exFAT

    // ==========================================
    // Filter Manager (FltMgr)
    // ==========================================
    'FMfn',  // Filter Manager
    'FMic',  // Filter Manager IRP context
    'FMcb',  // Filter Manager callback
    'FMcn',  // Filter Manager
    'FMfl',  // Filter Manager
    'FMin',  // Filter Manager instance
    'FMfr',  // Filter Manager frame
    'FMvl',  // Filter Manager volume
    'FMst',  // Filter Manager stream
    'FMnc',  // Filter Manager name cache

    // ==========================================
    // Object Manager (Ob)
    // ==========================================
    'ObjT',  // Object type
    'ObjD',  // Object directory
    'ObjH',  // Object header
    'ObjN',  // Object name
    'ObjS',  // Object symbolic link
    'ObSq',  // Object security
    'ObDi',  // Object directory
    'ObNm',  // Object name

    // ==========================================
    // Process Manager (Ps)
    // ==========================================
    'PsQb',  // Process quota block
    'PsFP',  // Process
    'PsPh',  // Process handle table
    'PsJb',  // Process job
    'PsCr',  // Process create
    'PsTh',  // Thread
    'PsWs',  // Process working set

    // ==========================================
    // Registry (Cm / Hv)
    // ==========================================
    'CmKb',  // Registry key body
    'CmKn',  // Registry key name
    'CmVb',  // Registry value body
    'CmNb',  // Registry
    'CmSb',  // Registry security
    'HvSb',  // Hive
    'HvBb',  // Hive bin
    'HvCe',  // Hive cell
    'HvLo',  // Hive log

    // ==========================================
    // Security (Se / Lsa)
    // ==========================================
    'SeTk',  // Security token
    'SeAc',  // Security access
    'SeSd',  // Security descriptor
    'SeAu',  // Security audit
    'SeLs',  // Security LSA
    'SeRm',  // Security RM
    'SeSp',  // Security

    // ==========================================
    // PnP / HAL
    // ==========================================
    'PnpI',  // PnP IRP
    'PnpD',  // PnP device
    'PnpE',  // PnP event
    'PnpR',  // PnP resource
    'HalT',  // HAL timer
    'HalA',  // HAL adapter
    'HalD',  // HAL DMA
    'HalI',  // HAL interrupt

    // ==========================================
    // Storage / Disk
    // ==========================================
    'Stor',  // Storage port
    'StBu',  // Storage buffer
    'ScPo',  // SCSI port
    'ScSr',  // SCSI srb
    'DskI',  // Disk driver
    'VoIn',  // Volume manager
    'VPBk',  // VPB
    'FvSr',  // storport
    'FvBf',  // storport buffer

    // ==========================================
    // Display / GPU (WDDM)
    // ==========================================
    'Dxgk',  // DXGK (DirectX kernel)
    'DxgA',  // DX adapter
    'DxgD',  // DX device
    'DxgR',  // DX resource
    'DxgS',  // DX surface
    'DxgV',  // DX vidmem
    'DxAl',  // DX alloc
    'VidM',  // Video memory manager
    'VidP',  // Video port
    'VidS',  // Video scheduler

    // ==========================================
    // Audio (PortCls / HD Audio)
    // ==========================================
    'PcAd',  // PortCls adapter
    'PcDv',  // PortCls device
    'PcFl',  // PortCls
    'HdAu',  // HD Audio

    // ==========================================
    // Crypto / Code Integrity
    // ==========================================
    'CiCa',  // Code integrity
    'CiSi',  // Code integrity signature
    'CiHa',  // Code integrity hash
    'CrBl',  // Crypto block
    'CrKe',  // Crypto key
    'Cryt',  // Cryptography

    // ==========================================
    // ETW / WMI
    // ==========================================
    'EtwB',  // ETW buffer
    'EtwR',  // ETW registration
    'EtwL',  // ETW logger
    'EtwG',  // ETW guid
    'WmiG',  // WMI guid
    'WmiI',  // WMI instance
    'WmiR',  // WMI registration

    // ==========================================
    // USB
    // ==========================================
    'UsbH',  // USB hub
    'UsbD',  // USBDI
    'UsbP',  // USB port
    'UsbR',  // USB request
    'UsbT',  // USB transfer
    'UsbC',  // USB controller
    'UsbE',  // USB endpoint
    'UHCi',  // UHCI
    'EHCi',  // EHCI
    'XHCi',  // XHCI (USB 3)

    // ==========================================
    // Hyper-V / virtualization
    // ==========================================
    'HvVM',  // Hyper-V VM
    'HvPg',  // Hyper-V page
    'HvBu',  // Hyper-V buffer
    'VmbK',  // VMBus kernel
    'VmbC',  // VMBus channel

    // ==========================================
    // Misc Windows internals
    // ==========================================
    'AlPC',  // ALPC (Advanced LPC)
    'AlPo',  // ALPC port
    'AlPm',  // ALPC message
    'LpcZ',  // LPC zone
    'LpcP',  // LPC port
    'ExpA',  // Executive
    'ExpE',  // Executive event
    'ExpS',  // Executive semaphore
    'KiSt',  // Kernel stack
    'KiDp',  // Kernel DPC
    'KiTm',  // Kernel timer
    'KeAT',  // Kernel
    'TmTx',  // Transaction manager
    'TmRm',  // TM resource manager
    'TmEn',  // TM enlistment

};
