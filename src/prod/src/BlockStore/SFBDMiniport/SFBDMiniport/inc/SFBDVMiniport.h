// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

/*++

Module Name:

SFBDVMiniport.h

Abstract:

This file includes data declarations for Service Fabric BlockStore Device's interaction with the BlockStore Service.

Environment:

Kernel mode only.

--*/

#pragma once

#ifndef _MP_H_
#define _MP_H_

#define NTSTRSAFE_LIB

#include <ntddk.h>
#include <ntdddisk.h>
#include <ntdef.h> 
#include <wdf.h>
#include <storport.h>  
#include <devioctl.h>
#include <ntddscsi.h>
#include <scsiwmi.h>
#include <wmistr.h>
#include <wsk.h>
#include <winerror.h>
#include <ntstrsafe.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#define DRIVER_BUILD
#include "..\..\SFBDCtlLib\SFBDCtlLib.h"
#include "common.h"

// Needs to be 8 characters in length
#define MAXLEN_VENDOR_ID            8
#define VENDOR_ID                   L"Service"
#define VENDOR_ID_ascii             "Service"

// Needs to be 16 characters in length
#define MAXLEN_PRODUCT_ID           16
#define PRODUCT_ID                  L" Fabric Block "
#define PRODUCT_ID_ascii            " Fabric Block "

// Needs to be 4 characters in length
#define MAXLEN_PRODUCT_REV          4
#define PRODUCT_REV                 L"1"
#define PRODUCT_REV_ascii           "1"

#define SFBD_TAG_GENERAL            'DBFS'  // "SFBD"

#define DEVICE_NOT_FOUND            0xFF

#define SECTOR_SIZE                     (512)
#define BLOCK_SIZE_MANAGEMENT_REQUEST   (16384)

#define DEFAULT_BREAK_ON_ENTRY      0                // No break
#define DEFAULT_DEBUG_LEVEL         2   

#define DEFAULT_LUNPerHBA           1

// Define the (default) Queue depth for the LU.
// Also, helps control maximum outgoing connections to the service.
#define QUEUE_DEPTH_PER_LU          20

#define InstName                    L"vHBA"
#define MAX_DEVICE_SYMBOLICNAME_LEN 100

#define MAX_LU_REGISTRATIONS        100

#define GET_FLAG(Flags, Bit)        ((Flags) & (Bit))
#define SET_FLAG(Flags, Bit)        ((Flags) |= (Bit))
#define CLEAR_FLAG(Flags, Bit)      ((Flags) &= ~(Bit))

typedef struct _DEVICE_LIST          DEVICE_LIST, *pDEVICE_LIST;
typedef struct _SFBDDriverInfo       SFBDDriverInfo, *pSFBDDriverInfo;
typedef struct _SFBDRegistryInfo     SFBDRegistryInfo, *pSFBDRegistryInfo;
typedef struct _HW_LU_EXTENSION      HW_LU_EXTENSION, *pHW_LU_EXTENSION;
typedef struct _LBA_LIST             LBA_LIST, *PLBA_LIST;

extern pSFBDDriverInfo g_pSFBDDriver;  

// Function prototype for DeviceIoControl callback
typedef NTSTATUS(*FuncDeviceIoControl) (PDEVICE_OBJECT, PIRP);

typedef struct _SFBDRegistryInfo {
    UNICODE_STRING   VendorId;
    UNICODE_STRING   ProductId;
    UNICODE_STRING   ProductRevision;
    ULONG            BreakOnEntry;       // Break into debugger
    ULONG            DebugLevel;         // Debug log level
    ULONG            LUNPerHBA;          // Number of LUNs per HBA.
} SFBDRegistryInfo, * pSFBDRegistryInfo;

// The master miniport object. In effect, an extension of the driver object for the miniport.
typedef struct _SFBDDriverInfo {                        
    SFBDRegistryInfo               pRegistryInfo;
    KSPIN_LOCK                     DrvInfoLock;
    LIST_ENTRY                     ListMPHBAObj;            // Header of list of HW_HBA_EXT objects.
    PDRIVER_OBJECT                 pDriverObj;
    ULONG                          DrvInfoNbrMPHBAObj;      // Count of items in ListMPHBAObj.
    FuncDeviceIoControl            pFuncDeviceIoControl;    // Reference to DeviceIoControl callback setup by StorPort initializtion in MajorFunction array of DRIVER_OBJECT
} SFBDDriverInfo, * pSFBDDriverInfo;

typedef struct _HW_HBA_EXT {                          // Adapter device-object extension allocated by StorPort.
    LIST_ENTRY                      List;              // Pointers to next and previous HW_HBA_EXT objects.
    LIST_ENTRY                      LUList;            // Pointers to HW_LU_EXTENSION objects.
    pSFBDDriverInfo                 pDriverInfo;
    PDRIVER_OBJECT                  pDrvObj;
    pSFBD_DEVICE_LIST               pDeviceList;
    SCSI_WMILIB_CONTEXT             WmiLibContext;
    PIRP                            pReverseCallIrp;
    KSPIN_LOCK                      LUListLock;   
    ULONG                           SRBsSeen;
    ULONG                           WMISRBsSeen;
    ULONG                           LUNPerHBA; 
    UCHAR                           MaximumNumberOfBuses;
    UCHAR                           MaximumNumberOfTargets;
    UCHAR                           MaximumNumberOfLogicalUnits;
    UCHAR                           AdapterState;
    UCHAR                           VendorId[9];
    UCHAR                           ProductId[17];
    UCHAR                           ProductRevision[5];
    BOOLEAN                         bReportAdapterDone;
    WCHAR                           DeviceSymbolicNameBuffer[MAX_DEVICE_SYMBOLICNAME_LEN];
    UNICODE_STRING                  DeviceSymbolicName;

    // WSK Registration datastructures
    WSK_CLIENT_DISPATCH             WskAppDispatch;
    WSK_REGISTRATION                WskRegistration;
    WSK_PROVIDER_NPI                WskProviderNpi;
    BOOLEAN                         InitializedWsk;
    SOCKADDR_IN                     ResolvedServerAddress;
    PETHREAD                        ThreadObject;
    PSECURITY_DESCRIPTOR            WskCachedSecurityDescriptor;

#if defined(DBG)
    LONG64                          ActiveConnections;
    KEVENT                          ShutdownSyncEvent;
    LONG                            IsShutdownInProgress;
#endif // defined(DBG)

} HW_HBA_EXT, * pHW_HBA_EXT;

// Flag definitions for LUFlags.

#define LU_DEVICE_INITIALIZED   0x0001

typedef enum {
    ActionRead,
    ActionWrite
} SFBDWkRtnAction;

typedef struct _HW_LU_EXTENSION {                     // LUN extension allocated by StorPort.
    LIST_ENTRY            List;                       // Pointers to next and previous HW_LU_EXTENSION objects, used in HW_HBA_EXT.
    ULONG                 LUFlags;
    ULONG64               MaxBlocks;
    pSFBD_DEVICE_INFO       DeviceInfo;

} HW_LU_EXTENSION, * pHW_LU_EXTENSION;

typedef struct _HW_SRB_EXTENSION {
    
    pHW_HBA_EXT             HBAExt;
    pHW_LU_EXTENSION        LUExt;
    SFBDWkRtnAction         Action;
    PIO_WORKITEM            WorkItem;

    // WMI Context for each SRB
    SCSIWMI_REQUEST_CONTEXT WmiRequestContext;

    // System address for the SRB's DataBuffer
    PVOID                   SystemAddressDataBuffer;

} HW_SRB_EXTENSION, * PHW_SRB_EXTENSION;

enum ResultType {
  ResultDone,
  ResultQueued
} ;

//
// ** Core Driver Functions **
//

// Ensure DriverEntry entry point visible to WinDbg even without a matching .pdb.
sp_DRIVER_INITIALIZE DriverEntry;
VIRTUAL_HW_FIND_ADAPTER SFBDHwFindAdapter;
HW_TIMER SFBDHwTimer;
HW_INITIALIZE SFBDHwInitialize;
HW_FREE_ADAPTER_RESOURCES SFBDHwFreeAdapterResources;
HW_STARTIO SFBDHwStartIo;
HW_RESET_BUS SFBDHwResetBus;
HW_ADAPTER_CONTROL SFBDHwAdapterControl;

HW_PROCESS_SERVICE_REQUEST SFBDHwProcessIrp;
HW_COMPLETE_SERVICE_IRP SFBDHwCompleteServiceIrp;

// ** Tracing Functions **
HW_INITIALIZE_TRACING SFBDTracingInit;
HW_CLEANUP_TRACING SFBDTracingCleanup;
// ** SCSI Support Functions **

VOID
SFBDHwReportAdapter(
    IN pHW_HBA_EXT
    );

VOID
SFBDHwReportLink(
    IN pHW_HBA_EXT
    );

VOID
SFBDHwReportLog(
    IN pHW_HBA_EXT);


NTSTATUS 
SFBDDeviceIoControl(
    PDEVICE_OBJECT DeviceObject, 
    PIRP Irp
);

VOID 
SFBDShutdownServiceTransport(
    pHW_HBA_EXT pHBAExt
);

VOID
SFBDQueryRegParameters(
    IN PUNICODE_STRING,
    IN pSFBDRegistryInfo
);

NTSTATUS
SFBDCreateDeviceList(
    IN pHW_HBA_EXT
);

ULONG SFBDGetDeviceIndex(
    IN pHW_HBA_EXT pHBAExt,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun
);

pSFBD_DEVICE_INFO
SFBDGetDevice(
    IN pHW_HBA_EXT DevExt,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun,
    IN BOOLEAN IncludeUnmountedLU
    );

pSFBD_DEVICE_INFO SFBDGetNextAvailableDeviceInfo(
    IN pHW_HBA_EXT pHBAExt,
    IN PUCHAR pPathId,
    IN PUCHAR pTargetId,
    IN PUCHAR pLun
);

pSFBD_DEVICE_INFO
SFBDGetDeviceForLUIndex(
    IN pHW_HBA_EXT          pHBAExt,
    IN ULONG                indexLu
);

pSFBD_DEVICE_INFO
SFBDGetDeviceForLUID(
    IN pHW_HBA_EXT          pHBAExt,
    IN PWCHAR               pLUID,
    IN BOOLEAN              IncludeUnmountedLU
);

BOOLEAN SFBDIsDeviceEnabled(
    IN pSFBD_DEVICE_INFO pDeviceInfo
);

BOOLEAN SFBDFreeDeviceInfo(
    IN pHW_HBA_EXT pHBAExt,
    IN pSFBD_DEVICE_INFO pDeviceInfo
);

UCHAR SFBDFindRemovedDevice(
    IN pHW_HBA_EXT,
    IN PSCSI_REQUEST_BLOCK
);

VOID SFBDStopAdapter(
    IN pHW_HBA_EXT DevExt
);

VOID SFBDRestartAdapter(
    IN pHW_HBA_EXT DevExt
);

IO_WORKITEM_ROUTINE SFBDGeneralWkRtn;

VOID
SFBDWorker(
    IN PVOID
);

BOOLEAN
SFBDRemoveLUExtFromHBAList(
    IN pHW_HBA_EXT pHBAExt, 
    IN pHW_LU_EXTENSION pLUExtToRemove
);

#if defined(DBG)
VOID
SFBDDecrementActiveConnections(
    IN pHW_HBA_EXT
);
#endif // defined(DBG)
 
UCHAR
ScsiExecuteMain(
    IN pHW_HBA_EXT DevExt,
    IN PSCSI_REQUEST_BLOCK,
    IN PUCHAR
);

UCHAR
ScsiOpInquiry(
    IN pHW_HBA_EXT DevExt,
    IN pHW_LU_EXTENSION LuExt,
    IN PSCSI_REQUEST_BLOCK Srb
);

UCHAR
ScsiOpReadCapacity(
    IN pHW_HBA_EXT DevExt,
    IN pHW_LU_EXTENSION LuExt,
    IN PSCSI_REQUEST_BLOCK Srb
);

UCHAR
ScsiOpModeSense(
    IN pHW_HBA_EXT         DevExt,
    IN pHW_LU_EXTENSION    LuExt,
    IN PSCSI_REQUEST_BLOCK pSrb
);

UCHAR
ScsiOpReportLuns(
    IN pHW_HBA_EXT          DevExt,
    IN pHW_LU_EXTENSION     LuExt,
    IN PSCSI_REQUEST_BLOCK  Srb
);

ULONG64 
ScsiGetStartingSectorFromCDB(
    IN PCDB pCdb, 
    IN UCHAR CdbLength
);

UCHAR
ScsiOpVPD(
    IN pHW_HBA_EXT,
    IN pHW_LU_EXTENSION,
    IN PSCSI_REQUEST_BLOCK
    );

UCHAR
ScsiReadWriteSetup(
    IN pHW_HBA_EXT          pDevExt,
    IN pHW_LU_EXTENSION     pLUExt,
    IN PSCSI_REQUEST_BLOCK  pSrb,
    IN SFBDWkRtnAction        WkRtnAction,
    IN PUCHAR               pResult
);

//
// ** WMI Support Functions **
//

VOID
InitializeWmiContext(
    IN pHW_HBA_EXT
);

BOOLEAN
HandleWmiSrb(
    IN     pHW_HBA_EXT,
    IN OUT PSCSI_WMI_REQUEST_BLOCK
);

void
HandleInvariantFailure();

#endif    // _MP_H_

