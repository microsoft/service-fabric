// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

/*++

Module Name:

SFBDVMiniport.c

Abstract:

This file includes core driver implementation for Service Fabric BlockStore Device's interaction with the BlockStore Service.

Environment:

Kernel mode only.

--*/

#include "inc\SFBDVMiniport.h"
#include "inc\SFBDWmi.h"
#include "inc\BlockStore.h"
#include "Microsoft-ServiceFabric-BlockStoreMiniportEvents.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, DriverEntry )
#endif // ALLOC_PRAGMA

/**************************************************************************************************/ 
/*                                                                                                */ 
/* Globals.                                                                                       */ 
/*                                                                                                */ 
/**************************************************************************************************/ 

pSFBDDriverInfo g_pSFBDDriver = NULL;

#if defined(USE_DMA_FOR_SRB_IO)
// Our service needs to share user mode buffer with the kernel driver
// so that we can perform R/W to/from the buffer using Direct IO. for this,
// it needs to use custom IOCTLs.
//
// However, Storport, after initialization, registers its own callback for IRP_MJ_DEVICE_CONTROL
// which only responds to IOCTL_MINIPORT_PROCESS_SERVICE_IRP and does Buffered IO. 
//
// To address this, we will save StorPort's callback and register our own
// callback instead. Our callback will take care off the custom IOCTLs and also
// invoke StorPort's registered callback as well.
VOID SFBDFixupDeviceIoControlCallback(pSFBDDriverInfo pDriverInfo)
{
    pDriverInfo->pFuncDeviceIoControl = (FuncDeviceIoControl)pDriverInfo->pDriverObj->MajorFunction[IRP_MJ_DEVICE_CONTROL];
    pDriverInfo->pDriverObj->MajorFunction[IRP_MJ_DEVICE_CONTROL] = SFBDDeviceIoControl;
}
#endif // defined(USE_DMA_FOR_SRB_IO)

// Ensure DriverEntry entry point visible to WinDbg even without a matching .pdb. 
ULONG                                                                                                                                              
DriverEntry(
            _In_ PVOID pDrvObj,
            _In_ PVOID pRegistryPath
           )
{
    EventRegisterMicrosoft_ServiceFabric_BlockStoreMiniport();
    NTSTATUS                       status = STATUS_SUCCESS;
    VIRTUAL_HW_INITIALIZATION_DATA hwInitData;
    pSFBDDriverInfo                pMPDrvInfo;

    // Allocate basic miniport driver object (shared across instances of miniport). 
    // This will be freed when the last adapter is released.

    pMPDrvInfo = ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(SFBDDriverInfo), SFBD_TAG_GENERAL);
    if (!pMPDrvInfo) 
    { 
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    g_pSFBDDriver = pMPDrvInfo;                    // Save pointer in binary's storage.

    RtlZeroMemory(pMPDrvInfo, sizeof(SFBDDriverInfo));  // Set pMPDrvInfo's storage to a known state.

    pMPDrvInfo->pDriverObj = pDrvObj;                 // Save pointer to driver object.

    KeInitializeSpinLock(&pMPDrvInfo->DrvInfoLock);   // Initialize spin lock.
    
    InitializeListHead(&pMPDrvInfo->ListMPHBAObj);    // Initialize list head.
    
    // Get registry parameters.

    SFBDQueryRegParameters(pRegistryPath, &pMPDrvInfo->pRegistryInfo);

    // Set up information for StorPortInitialize().

    RtlZeroMemory(&hwInitData, sizeof(VIRTUAL_HW_INITIALIZATION_DATA));

    hwInitData.HwInitializationDataSize = sizeof(VIRTUAL_HW_INITIALIZATION_DATA);

    hwInitData.HwInitialize             = SFBDHwInitialize;       // Required.
    hwInitData.HwStartIo                = SFBDHwStartIo;          // Required.
    hwInitData.HwFindAdapter            = SFBDHwFindAdapter;      // Required.
    hwInitData.HwResetBus               = SFBDHwResetBus;         // Required.
    hwInitData.HwAdapterControl         = SFBDHwAdapterControl;   // Required.
    hwInitData.HwFreeAdapterResources   = SFBDHwFreeAdapterResources;
    hwInitData.HwInitializeTracing      = SFBDTracingInit;
    hwInitData.HwCleanupTracing         = SFBDTracingCleanup;
    hwInitData.HwProcessServiceRequest  = SFBDHwProcessIrp;
    hwInitData.HwCompleteServiceIrp     = SFBDHwCompleteServiceIrp;
    
    hwInitData.AdapterInterfaceType     = Internal;

    hwInitData.DeviceExtensionSize      = sizeof(HW_HBA_EXT);
    hwInitData.SpecificLuExtensionSize  = sizeof(HW_LU_EXTENSION);
    hwInitData.SrbExtensionSize         = sizeof(HW_SRB_EXTENSION);

    status =  StorPortInitialize(                     // Tell StorPort we're here.
                                 pDrvObj,
                                 pRegistryPath,
                                 (PHW_INITIALIZATION_DATA)&hwInitData,     // Note: Have to override type!
                                 NULL
                                );

    if (status == STATUS_SUCCESS)
    {
#if defined(USE_DMA_FOR_SRB_IO)
        // Fixup DriverObj->MajorFunction[IRP_MJ_DEVICE_CONTROL] to point to our DeviceIoControl callback instead of StorPort implementation.
        SFBDFixupDeviceIoControlCallback(pMPDrvInfo);
#endif // defined(USE_DMA_FOR_SRB_IO)
    }

Done:    
    if (status != STATUS_SUCCESS)
    { 
      if (NULL!=pMPDrvInfo) 
      {
        ExFreePoolWithTag(pMPDrvInfo, SFBD_TAG_GENERAL);
      }
    }
    
    return status;
}                                                     

/**************************************************************************************************/ 
/*                                                                                                */ 
/* Callback for a new HBA.                                                                        */ 
/*                                                                                                */ 
/**************************************************************************************************/ 
ULONG                                                 
SFBDHwFindAdapter(
                IN       PVOID                           pVoidHBAExt,       // Adapter device-object extension from StorPort.
                IN       PVOID                           pHwContext,        // Pointer to the PDO.
                IN       PVOID                           pBusInformation,   // Miniport's FDO.
                IN       PVOID                           pLowerDO,          // Device object beneath FDO.
                IN       PCHAR                           pArgumentString,
                IN OUT PPORT_CONFIGURATION_INFORMATION   pConfigInfo,
                IN       PBOOLEAN                        pBAgain            
               )
{
    ULONG              i,
                       len,
                       status = SP_RETURN_FOUND;
    PCHAR              pChar;

    KLOCK_QUEUE_HANDLE LockHandle;

    UNREFERENCED_PARAMETER(pHwContext);
    UNREFERENCED_PARAMETER(pBusInformation);
    UNREFERENCED_PARAMETER(pLowerDO);
    UNREFERENCED_PARAMETER(pArgumentString);

    pHW_HBA_EXT  pHBAExt = pVoidHBAExt;

    // Save the reference to the global driver info into the HBA Extension.
    pHBAExt->pDriverInfo = g_pSFBDDriver;           
    pHBAExt->ThreadObject = NULL;
    pHBAExt->WskCachedSecurityDescriptor = NULL;

    // Before proceeding ahead, register the device interface  for user-mode to talk with us.
    // It is registered once and can be used to enumerate HBAs, Buses, Targets and Luns.
    pHBAExt->DeviceSymbolicName.Length = 0;
    pHBAExt->DeviceSymbolicName.MaximumLength = sizeof(pHBAExt->DeviceSymbolicNameBuffer);
    pHBAExt->DeviceSymbolicName.Buffer = pHBAExt->DeviceSymbolicNameBuffer;
    NTSTATUS ntStatus = IoRegisterDeviceInterface((PDEVICE_OBJECT)pHwContext, &GUID_ServiceFabricBlockDeviceInterface, NULL, &pHBAExt->DeviceSymbolicName);
    if (!NT_SUCCESS(ntStatus) && (ntStatus != STATUS_OBJECT_NAME_EXISTS))
    {
        EventWriteRegisterDeviceInterfaceFailed(NULL, ntStatus);
        KdPrint(("Unable to register device interface due to NTSTATUS %08X", ntStatus));
        return SP_RETURN_ERROR;
    }

    // Initialize the list, and supporting lock, for tracking the LUs associated with this HBA.
    InitializeListHead(&pHBAExt->LUList);
    KeInitializeSpinLock(&pHBAExt->LUListLock);     

    pHBAExt->pDrvObj = g_pSFBDDriver->pDriverObj;

    pConfigInfo->VirtualDevice                  = TRUE;                        // Inidicate no real hardware.
    pConfigInfo->WmiDataProvider                = TRUE;                        // Indicate WMI provider.
    pConfigInfo->MaximumTransferLength          = SP_UNINITIALIZED_VALUE;      // Indicate unlimited.
    pConfigInfo->AlignmentMask                  = 0x3;                         // Indicate DWORD alignment.
    pConfigInfo->CachesData                     = FALSE;                       // Indicate miniport wants flush and shutdown notification.
    pConfigInfo->MaximumNumberOfTargets         = SCSI_MAXIMUM_TARGETS;        // Indicate maximum targets/devices per bus.
    pConfigInfo->NumberOfBuses                  = SCSI_MAXIMUM_BUSES;          // Indicate number of buses.
    pConfigInfo->SynchronizationModel           = StorSynchronizeFullDuplex;   // Indicate full-duplex.
    pConfigInfo->ScatterGather                  = TRUE;                        // Indicate scatter-gather (explicit setting needed for Win2003 at least).
    pConfigInfo->MaximumNumberOfLogicalUnits    = SCSI_MAXIMUM_LOGICAL_UNITS;  // Indicate number of LUs per Device/Target
   
    // Save off the Bus/Target/LU details in the HBA Extension.
    pHBAExt->MaximumNumberOfBuses = pConfigInfo->NumberOfBuses;
    pHBAExt->MaximumNumberOfTargets = pConfigInfo->MaximumNumberOfTargets;
    pHBAExt->MaximumNumberOfLogicalUnits = pConfigInfo->MaximumNumberOfLogicalUnits;
    
#if defined(DBG)
    // Init the active socket connection count and the event used to synchronize shutdown
    pHBAExt->ActiveConnections = 0;
    KeInitializeEvent(&pHBAExt->ShutdownSyncEvent, SynchronizationEvent, FALSE);
    pHBAExt->IsShutdownInProgress = 0;
#endif // defined(DBG)

    // Compute the LUN per HBA
    g_pSFBDDriver->pRegistryInfo.LUNPerHBA = (pConfigInfo->NumberOfBuses) * (pConfigInfo->MaximumNumberOfTargets) * (pConfigInfo->MaximumNumberOfLogicalUnits);

    // And save it in HBA Extension as well.
    pHBAExt->LUNPerHBA = g_pSFBDDriver->pRegistryInfo.LUNPerHBA;

    // Create a device list for the adapter with all the deviced marked disabled. 
    SFBDCreateDeviceList(pHBAExt);
    
    // Save Vendor Id, Product Id, Revision in device extension.
    pChar = (PCHAR)pHBAExt->pDriverInfo->pRegistryInfo.VendorId.Buffer;
    len = min(MAXLEN_VENDOR_ID, (pHBAExt->pDriverInfo->pRegistryInfo.VendorId.Length/2));
    for ( i = 0; i < len; i++, pChar+=2)
      pHBAExt->VendorId[i] = *pChar;

    pChar = (PCHAR)pHBAExt->pDriverInfo->pRegistryInfo.ProductId.Buffer;
    len = min(MAXLEN_PRODUCT_ID, (pHBAExt->pDriverInfo->pRegistryInfo.ProductId.Length/2));
    for ( i = 0; i < len; i++, pChar+=2)
      pHBAExt->ProductId[i] = *pChar;

    pChar = (PCHAR)pHBAExt->pDriverInfo->pRegistryInfo.ProductRevision.Buffer;
    len = min(MAXLEN_PRODUCT_REV, (pHBAExt->pDriverInfo->pRegistryInfo.ProductRevision.Length/2));
    for ( i = 0; i < len; i++, pChar+=2)
      pHBAExt->ProductRevision[i] = *pChar;

    // Add HBA extension to master driver object's linked list.

    KeAcquireInStackQueuedSpinLock(&g_pSFBDDriver->DrvInfoLock, &LockHandle);

    InsertTailList(&g_pSFBDDriver->ListMPHBAObj, &pHBAExt->List);
    g_pSFBDDriver->DrvInfoNbrMPHBAObj++;

    KeReleaseInStackQueuedSpinLock(&LockHandle);

    InitializeWmiContext(pHBAExt);

    *pBAgain = FALSE;    
    
    return status;
}                                                     

ULONG64 SFBDGetDiskSizeInBytesFromSizeUnit(ULONG sizeInUnits, DiskSizeUnit sizeUnit)
{
    // Compute the disk size
    ULONG64 diskSize = 1024 * 1024;
    switch (sizeUnit)
    {
    case MB:
        // Nothing to do since the default multplier is already set.
        break;
    case GB:
        diskSize *= 1024;
        break;
    case TB:
        diskSize *= (1024 * 1024);
        break;
    }

    // Copy the disk size and LUID to device info
    diskSize *= sizeInUnits;

    return diskSize;
}

// This will initialize the Device list with the enumerated LUs that were registered with the service.
//
// We do not persist the PathId/TargetId/Lun for the LU when it is registered during creation. As a result,
// when we are initializing here, we could get a different LU for the PathId/TargetId/Lun and not the one
// used when the volume was originally created.
//
// Currently, this is because of the order in which Reliable Dictionary returned the registration entries for various
// volumes, returning them in random order.
//
// That said, this behaviour does not affect correctness in any manner.
NTSTATUS SFBDInitializeDeviceListWithRegisteredLU(
    IN pHW_HBA_EXT pHBAExt,
    IN PLU_REGISTRATION_LIST pRegistrationList,
    IN ULONG ServicePort
)
{
    ULONG numEntries = pRegistrationList->countEntries;
    NTSTATUS Status = STATUS_SUCCESS;
    PUCHAR pEntryStart = (PUCHAR)&pRegistrationList->registrationInfo;
    WCHAR LUID[MAX_WCHAR_SIZE_LUID];
    UCHAR LastPathIdToReport = 0;
    BOOLEAN ReportBusChangeNotification = FALSE;

    for (ULONG curIndex = 0; curIndex < numEntries; curIndex++)
    {
        // Get the length of LUID in bytes (including the terminating NULL)
        ULONG lengthLUID = *((PULONG)pEntryStart);
        pEntryStart += sizeof(ULONG);

        // Get the DiskSize
        ULONG DiskSizeInSizeUnit = *((PULONG)pEntryStart);
        pEntryStart += sizeof(ULONG);

        // Get the DiskSize unit
        ULONG SizeUnit = *((PULONG)pEntryStart);
        pEntryStart += sizeof(ULONG);

        // Get the mount status
        BOOLEAN Mounted = (*((PULONG)pEntryStart) == 1) ? TRUE : FALSE;
        pEntryStart += sizeof(ULONG);

        // Finally, fetch the LUID
        RtlZeroMemory(LUID, sizeof(LUID));
        RtlCopyMemory(LUID, pEntryStart, lengthLUID);

        pEntryStart += lengthLUID; // This will move to the next entry, if any.

        // Compute the disk size in bytes
        ULONG64 DiskSize = SFBDGetDiskSizeInBytesFromSizeUnit(DiskSizeInSizeUnit, SizeUnit);

        // Get the next available device.
        UCHAR PathId = 0, TargetId = 0, Lun = 0;

        // First, reserve a free DeviceInfo
        pSFBD_DEVICE_INFO pDeviceInfo = SFBDGetNextAvailableDeviceInfo(pHBAExt, &PathId, &TargetId, &Lun);

        // Every LUN supported by this device *must* have a unique ID.
        //
        // Second, check if we already have a DeviceInfo for the specified LUID. If it exists,
        // free it since the LUID is expected to be unique across all service instances connected
        // to this driver.
        pSFBD_DEVICE_INFO pExistingDeviceInfo = SFBDGetDeviceForLUID(pHBAExt, LUID, FALSE);
        if (pExistingDeviceInfo)
        {
            UCHAR devicePathId = pExistingDeviceInfo->PathId;

            // TODO: Log this
            //
            // Mark the device as free
            KdPrint(("[SFBDInitializeDeviceListWithRegisteredLU] Marking DeviceInfo [for LUID: %s] free", LUID));
            if (!SFBDFreeDeviceInfo(pHBAExt, pExistingDeviceInfo))
            {
                KdPrint(("[SFBDInitializeDeviceListWithRegisteredLU] Unable to free DeviceInfo [for LUID: %s]", LUID));
            }

            // Report the change to the storage stack
            StorPortNotification(BusChangeDetected, pHBAExt, devicePathId);
        }

        // Now set the DeviceInfo
        if (pDeviceInfo)
        {
            // Set the DiskSize
            pDeviceInfo->DiskSizeInSizeUnit = DiskSizeInSizeUnit;

            // Set the DiskSize unit
            pDeviceInfo->SizeUnit = SizeUnit;

            // Set the mount status
            pDeviceInfo->Mounted = Mounted;

            // Finally, set the LUID
            RtlCopyMemory(pDeviceInfo->LUID, LUID, lengthLUID);
            
            // Set the disk size in bytes
            pDeviceInfo->DiskSize = DiskSize;

            // Update the port associated with the service backing this LUN
            pDeviceInfo->ServicePort = ServicePort;

            // DeviceInfo is already marked Enabled
            NT_ASSERT(pDeviceInfo->Enabled == TRUE);

            if (pDeviceInfo->PathId != LastPathIdToReport)
            {
                if (ReportBusChangeNotification)
                {
                    StorPortNotification(BusChangeDetected, pHBAExt, LastPathIdToReport);
                }

                LastPathIdToReport = pDeviceInfo->PathId;
            }

            // We have atleast one volume to report to storage stack
            ReportBusChangeNotification = TRUE;
        }
        else
        {
            // We have LU registrations to process but not enough available devices!
            //
            // TODO: Log this.
            KdPrint(("[SFBDInitializeDeviceListWithRegisteredLU] DeviceInfo not available to support new device [LUID: %s]!", LUID));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }
    }

    // Report any outstanding bus change notification to the storage stack
    if (ReportBusChangeNotification)
    {
        StorPortNotification(BusChangeDetected, pHBAExt, LastPathIdToReport);
    }

    return Status;
}

#if !defined(USE_NP_TRANSPORT)
BOOLEAN 
StartSockets(IN pHW_HBA_EXT pHBAExt)
{
    BOOLEAN fInitialized = FALSE;

    NTSTATUS ntStatus = InitSockets(pHBAExt);
    if (NT_SUCCESS(ntStatus) && pHBAExt->InitializedWsk)
    {
        ntStatus = CacheSecurityContext(pHBAExt);
        if (NT_SUCCESS(ntStatus))
        {           
            ntStatus = ResolveNameToIp(pHBAExt);
        }
        else
        {
            EventWriteCacheSecurityContextFailed(NULL, ntStatus);
        }
    }

    if (NT_SUCCESS(ntStatus))
    {
       fInitialized = TRUE;
       EventWriteSocketInitialized(NULL);
       KdPrint(("Sockets successfully initialized"));
    }

    return fInitialized;
}

#endif // !defined(USE_NP_TRANSPORT)

/**************************************************************************************************/ 
/*                                                                                                */ 
/**************************************************************************************************/ 
BOOLEAN
SFBDHwInitialize(IN PVOID pVoidHBAExt)
{
    // Enable the device interface so that it can be enumerated and user mode can communicate with us.
    BOOLEAN fInitialized = FALSE;
    pHW_HBA_EXT pHBAExt = pVoidHBAExt;

    NTSTATUS ntStatus = IoSetDeviceInterfaceState(&pHBAExt->DeviceSymbolicName, TRUE);
    if (!NT_SUCCESS(ntStatus))
    {
        EventWriteDeviceInterfaceActivationFailed(NULL, ntStatus);
        KdPrint(("Unable to activate the device interface due to NTSTATUS %08X", ntStatus));
        NT_ASSERT(FALSE);
        return fInitialized;
    }

#if !defined(USE_NP_TRANSPORT)
    fInitialized = StartSockets(pHBAExt);
#endif // !defined(USE_NP_TRANSPORT)

    if (fInitialized)
    {
        EventWriteAdapterInitialized(NULL);
        KdPrint(("Adapter initialization successful!"));
    }

    return fInitialized;
}                                                    

/**************************************************************************************************/ 
/*                                                                                                */ 
/**************************************************************************************************/ 
VOID
SFBDHwReportAdapter(IN pHW_HBA_EXT pHBAExt)
{
    NTSTATUS               status;
    PWNODE_SINGLE_INSTANCE pWnode;
    ULONG                  WnodeSize,
                           WnodeSizeInstanceName,
                           WnodeSizeDataBlock,
                           length,
                           size;
    GUID                   lclGuid = MSFC_AdapterEvent_GUID;
    UNICODE_STRING         lclInstanceName;
    UCHAR                  myPortWWN[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    PMSFC_AdapterEvent     pAdapterArr;

    // With the instance name used here and with the rounding-up to 4-byte alignment of the data portion used here,
    // 0x34 (52) bytes are available for the actual data of the WMI event.  (The 0x34 bytes result from the fact that
    // StorPort at present (August 2008) allows 0x80 bytes for the entire WMIEvent (header, instance name and data);
    // the header is 0x40 bytes; the instance name used here results in 0xA bytes, and the rounding up consumes 2 bytes;
    // in other words, 0x80 - (0x40 + 0x0A + 0x02)).

    RtlInitUnicodeString(&lclInstanceName, InstName); // Set Unicode descriptor for instance name.

    // A WMIEvent structure consists of header, instance name and data block.

    WnodeSize             = sizeof(WNODE_SINGLE_INSTANCE);

    // Because the first field in the data block, EventType, is a ULONG, ensure that the data block begins on a
    // 4-byte boundary (as will be calculated in DataBlockOffset).

    WnodeSizeInstanceName = sizeof(USHORT) +          // Size of USHORT at beginning plus
                            lclInstanceName.Length;   //   size of instance name.
    WnodeSizeInstanceName =                           // Round length up to multiple of 4 (if needed).
      (ULONG)WDF_ALIGN_SIZE_UP(WnodeSizeInstanceName, sizeof(ULONG));

    WnodeSizeDataBlock    = MSFC_AdapterEvent_SIZE;   // Size of data block.

    size = WnodeSize             +                    // Size of WMIEvent.         
           WnodeSizeInstanceName + 
           WnodeSizeDataBlock;

    pWnode = ExAllocatePoolWithTag(NonPagedPoolNx, size, SFBD_TAG_GENERAL);

    if (NULL!=pWnode) {                               // Good?
        RtlZeroMemory(pWnode, size);
        
        // Fill out most of header. StorPort will set the ProviderId and TimeStamp in the header.

        pWnode->WnodeHeader.BufferSize = size;
        pWnode->WnodeHeader.Version    = 1;
        RtlCopyMemory(&pWnode->WnodeHeader.Guid, &lclGuid, sizeof(lclGuid));  
        pWnode->WnodeHeader.Flags      = WNODE_FLAG_EVENT_ITEM |
                                         WNODE_FLAG_SINGLE_INSTANCE;

        // Say where to find instance name and the data block and what is the data block's size.

        pWnode->OffsetInstanceName     = WnodeSize;
        pWnode->DataBlockOffset        = WnodeSize + WnodeSizeInstanceName;
        pWnode->SizeDataBlock          = WnodeSizeDataBlock;

        // Copy the instance name.
                   
        size -= WnodeSize;                            // Length remaining and available.
        status = WDF_WMI_BUFFER_APPEND_STRING(        // Copy WCHAR string, preceded by its size.
            WDF_PTR_ADD_OFFSET(pWnode, pWnode->OffsetInstanceName),
            size,                                     // Length available for copying.
            &lclInstanceName,                         // Unicode string whose WCHAR buffer is to be copied.
            &length                                   // Variable to receive size needed.
            );

        if (STATUS_SUCCESS!=status) {                 // A problem?
            ASSERT(FALSE);
        }

        pAdapterArr =                                 // Point to data block.
          WDF_PTR_ADD_OFFSET(pWnode, pWnode->DataBlockOffset);

        // Copy event code and WWN.

        pAdapterArr->EventType = HBA_EVENT_ADAPTER_ADD;

        RtlCopyMemory(pAdapterArr->PortWWN, myPortWWN, sizeof(myPortWWN));

        // Ask StorPort to announce the event.

        StorPortNotification(WMIEvent, 
                             pHBAExt, 
                             pWnode, 
                             0xFF);                   // Notification pertains to an HBA.

        ExFreePoolWithTag(pWnode, SFBD_TAG_GENERAL);
    }
    else {
    }
}                                                     

/**************************************************************************************************/ 
/*                                                                                                */ 
/**************************************************************************************************/ 
VOID
SFBDHwReportLink(IN pHW_HBA_EXT pHBAExt)
{
    NTSTATUS               status;
    PWNODE_SINGLE_INSTANCE pWnode;
    PMSFC_LinkEvent        pLinkEvent;
    ULONG                  WnodeSize,
                           WnodeSizeInstanceName,
                           WnodeSizeDataBlock,
                           length,
                           size;
    GUID                   lclGuid = MSFC_LinkEvent_GUID;
    UNICODE_STRING         lclInstanceName;
             
    #define RLIRBufferArraySize 0x10                  // Define 16 entries in MSFC_LinkEvent.RLIRBuffer[].
             
    UCHAR                  myAdapterWWN[8] = {1, 2, 3, 4, 5, 6, 7, 8},
                           myRLIRBuffer[RLIRBufferArraySize] = {10, 11, 12, 13, 14, 15, 16, 17, 20, 21, 22, 23, 24, 25, 26, 0xFF};

    RtlInitUnicodeString(&lclInstanceName, InstName); // Set Unicode descriptor for instance name.

    WnodeSize             = sizeof(WNODE_SINGLE_INSTANCE);
    WnodeSizeInstanceName = sizeof(USHORT) +          // Size of USHORT at beginning plus
                            lclInstanceName.Length;   //   size of instance name.
    WnodeSizeInstanceName =                           // Round length up to multiple of 4 (if needed).
      (ULONG)WDF_ALIGN_SIZE_UP(WnodeSizeInstanceName, sizeof(ULONG));
    WnodeSizeDataBlock    =                           // Size of data.
                            FIELD_OFFSET(MSFC_LinkEvent, RLIRBuffer) +
                            sizeof(myRLIRBuffer);

    size = WnodeSize             +                    // Size of WMIEvent.         
           WnodeSizeInstanceName + 
           WnodeSizeDataBlock;

    pWnode = ExAllocatePoolWithTag(NonPagedPoolNx, size, SFBD_TAG_GENERAL);

    if (NULL!=pWnode) {                               // Good?
        RtlZeroMemory(pWnode, size);
        
        // Fill out most of header. StorPort will set the ProviderId and TimeStamp in the header.

        pWnode->WnodeHeader.BufferSize = size;
        pWnode->WnodeHeader.Version    = 1;
        RtlCopyMemory(&pWnode->WnodeHeader.Guid, &lclGuid, sizeof(lclGuid));  
        pWnode->WnodeHeader.Flags      = WNODE_FLAG_EVENT_ITEM |
                                         WNODE_FLAG_SINGLE_INSTANCE;

        // Say where to find instance name and the data block and what is the data block's size.

        pWnode->OffsetInstanceName     = WnodeSize;
        pWnode->DataBlockOffset        = WnodeSize + WnodeSizeInstanceName;
        pWnode->SizeDataBlock          = WnodeSizeDataBlock;

        // Copy the instance name.
                   
        size -= WnodeSize;                            // Length remaining and available.
        status = WDF_WMI_BUFFER_APPEND_STRING(        // Copy WCHAR string, preceded by its size.
            WDF_PTR_ADD_OFFSET(pWnode, pWnode->OffsetInstanceName),
            size,                                     // Length available for copying.
            &lclInstanceName,                         // Unicode string whose WCHAR buffer is to be copied.
            &length                                   // Variable to receive size needed.
            );

        if (STATUS_SUCCESS!=status) {                 // A problem?
            ASSERT(FALSE);
        }

        pLinkEvent =                                  // Point to data block.
          WDF_PTR_ADD_OFFSET(pWnode, pWnode->DataBlockOffset);

        // Copy event code, WWN, buffer size and buffer contents.

        pLinkEvent->EventType = HBA_EVENT_LINK_INCIDENT;

        RtlCopyMemory(pLinkEvent->AdapterWWN, myAdapterWWN, sizeof(myAdapterWWN));

        pLinkEvent->RLIRBufferSize = sizeof(myRLIRBuffer);

        RtlCopyMemory(pLinkEvent->RLIRBuffer, myRLIRBuffer, sizeof(myRLIRBuffer));

        StorPortNotification(WMIEvent, 
                             pHBAExt, 
                             pWnode, 
                             0xFF);                   // Notification pertains to an HBA.

        ExFreePoolWithTag(pWnode, SFBD_TAG_GENERAL);
    }
    else {
    }
}                                                     

/**************************************************************************************************/ 
/*                                                                                                */ 
/**************************************************************************************************/ 
VOID
SFBDHwReportLog(IN pHW_HBA_EXT pHBAExt)
{
    NTSTATUS               status;
    PWNODE_SINGLE_INSTANCE pWnode;
    ULONG                  WnodeSize,
                           WnodeSizeInstanceName,
                           WnodeSizeDataBlock,
                           length,
                           size;
    UNICODE_STRING         lclInstanceName;
    PIO_ERROR_LOG_PACKET   pLogError;

    RtlInitUnicodeString(&lclInstanceName, InstName); // Set Unicode descriptor for instance name.

    WnodeSize             = sizeof(WNODE_SINGLE_INSTANCE);
    WnodeSizeInstanceName = sizeof(USHORT) +          // Size of USHORT at beginning plus
                            lclInstanceName.Length;   //   size of instance name.
    WnodeSizeInstanceName =                           // Round length up to multiple of 4 (if needed).
      (ULONG)WDF_ALIGN_SIZE_UP(WnodeSizeInstanceName, sizeof(ULONG));
    WnodeSizeDataBlock    = sizeof(IO_ERROR_LOG_PACKET);       // Size of data.

    size = WnodeSize             +                    // Size of WMIEvent.         
           WnodeSizeInstanceName + 
           WnodeSizeDataBlock;

    pWnode = ExAllocatePoolWithTag(NonPagedPoolNx, size, SFBD_TAG_GENERAL);

    if (NULL!=pWnode) {                               // Good?
        RtlZeroMemory(pWnode, size);
        
        // Fill out most of header. StorPort will set the ProviderId and TimeStamp in the header.

        pWnode->WnodeHeader.BufferSize = size;
        pWnode->WnodeHeader.Version    = 1;
        pWnode->WnodeHeader.Flags      = WNODE_FLAG_EVENT_ITEM |
                                         WNODE_FLAG_LOG_WNODE;

        pWnode->WnodeHeader.HistoricalContext = 9;

        // Say where to find instance name and the data block and what is the data block's size.

        pWnode->OffsetInstanceName     = WnodeSize;
        pWnode->DataBlockOffset        = WnodeSize + WnodeSizeInstanceName;
        pWnode->SizeDataBlock          = WnodeSizeDataBlock;

        // Copy the instance name.
                   
        size -= WnodeSize;                            // Length remaining and available.
        status = WDF_WMI_BUFFER_APPEND_STRING(        // Copy WCHAR string, preceded by its size.
            WDF_PTR_ADD_OFFSET(pWnode, pWnode->OffsetInstanceName),
            size,                                     // Length available for copying.
            &lclInstanceName,                         // Unicode string whose WCHAR buffer is to be copied.
            &length                                   // Variable to receive size needed.
            );

        if (STATUS_SUCCESS!=status) {                 // A problem?
            ASSERT(FALSE);
        }

        pLogError =                                    // Point to data block.
          WDF_PTR_ADD_OFFSET(pWnode, pWnode->DataBlockOffset);

        pLogError->UniqueErrorValue = 0x40;
        pLogError->FinalStatus = 0x41;
        pLogError->ErrorCode = 0x42;

        StorPortNotification(WMIEvent, 
                             pHBAExt, 
                             pWnode, 
                             0xFF);                   // Notification pertains to an HBA.

        ExFreePoolWithTag(pWnode, SFBD_TAG_GENERAL);
    }
    else {
    }
}                                                    

/**************************************************************************************************/ 
/*                                                                                                */ 
/**************************************************************************************************/ 
BOOLEAN
SFBDHwResetBus(
             IN PVOID                pHBAExt,       // Adapter device-object extension from StorPort.
             IN ULONG                BusId
            )
{
    UNREFERENCED_PARAMETER(pHBAExt);
    UNREFERENCED_PARAMETER(BusId);

    return TRUE;
}                                                    

/**************************************************************************************************/ 
/* PnP manager delivers this as the device is about to be removed.                                */ 
/**************************************************************************************************/ 
NTSTATUS                                              
SFBDHandleRemoveDevice(
                     IN pHW_HBA_EXT             pHBAExt,// Adapter device-object extension from StorPort.
                     IN PSCSI_PNP_REQUEST_BLOCK pSrb
                    )
{
    UNREFERENCED_PARAMETER(pHBAExt);

    pSrb->SrbStatus = SRB_STATUS_SUCCESS;

    return STATUS_SUCCESS;
}                                                    

/**************************************************************************************************/ 
/*                                                                                                */ 
/**************************************************************************************************/ 
NTSTATUS                                           
SFBDHandleQueryCapabilities(
                          IN pHW_HBA_EXT             pHBAExt,// Adapter device-object extension from StorPort.
                          IN PSCSI_PNP_REQUEST_BLOCK pSrb
                         )
{
    NTSTATUS                  status = STATUS_SUCCESS;
    PSTOR_DEVICE_CAPABILITIES pStorageCapabilities = (PSTOR_DEVICE_CAPABILITIES)pSrb->DataBuffer;

    UNREFERENCED_PARAMETER(pHBAExt);

    RtlZeroMemory(pStorageCapabilities, pSrb->DataTransferLength);

    pStorageCapabilities->Removable = TRUE;
    pStorageCapabilities->SurpriseRemovalOK = TRUE;

    pSrb->SrbStatus = SRB_STATUS_SUCCESS;

    return status;
}                                                     

/**************************************************************************************************/ 
/*                                                                                                */ 
/**************************************************************************************************/ 
NTSTATUS                                              
SFBDHwHandlePnP(
              IN pHW_HBA_EXT              pHBAExt,  // Adapter device-object extension from StorPort.
              IN PSCSI_PNP_REQUEST_BLOCK  pSrb
             )
{
    NTSTATUS status = STATUS_SUCCESS;

    switch(pSrb->PnPAction) {

      case StorRemoveDevice:
      {
          EventWriteSFBDHwHandlePnPRemoveDevice(NULL);
          status = SFBDHandleRemoveDevice(pHBAExt, pSrb);
          break;
      }
      case StorStopDevice:
      {
          EventWriteSFBDHwHandlePnPStopDevice(NULL);
          pSrb->SrbStatus = SRB_STATUS_SUCCESS;
          status = STATUS_SUCCESS;
          break;
      }
      case StorQueryCapabilities:
      {
          EventWriteSFBDHwHandlePnPQueryCapabilities(NULL);
          status = SFBDHandleQueryCapabilities(pHBAExt, pSrb);
          break;
      }
      default:
        pSrb->SrbStatus = SRB_STATUS_SUCCESS;         // Do nothing.
    }

    return status;
}                                                   

/**************************************************************************************************/ 
/*                                                                                                */ 
/**************************************************************************************************/ 
BOOLEAN
SFBDHwStartIo(
            IN       PVOID              pVoidHBAExt,  // Adapter device-object extension from StorPort.
            IN OUT PSCSI_REQUEST_BLOCK  pSrb
           )
{
    UCHAR                     srbStatus = SRB_STATUS_INVALID_REQUEST;
    BOOLEAN                   bFlag;
    NTSTATUS                  status;
    UCHAR                     Result = ResultDone;
    pHW_HBA_EXT               pHBAExt = pVoidHBAExt;

    // Bump count of SRBs encountered.
    _InterlockedExchangeAdd((volatile LONG *)&pHBAExt->SRBsSeen, 1);   

    // TODO: Review these (and any additional SRBs that need to be supported)
    switch (pSrb->Function) {

        case SRB_FUNCTION_EXECUTE_SCSI:
            srbStatus = ScsiExecuteMain(pHBAExt, pSrb, &Result);
            break;

        case SRB_FUNCTION_WMI:
            _InterlockedExchangeAdd((volatile LONG *)&pHBAExt->WMISRBsSeen, 1);
            bFlag = HandleWmiSrb(pHBAExt, (PSCSI_WMI_REQUEST_BLOCK)pSrb);
            srbStatus = TRUE==bFlag ? SRB_STATUS_SUCCESS : SRB_STATUS_INVALID_REQUEST;
            break;

        case SRB_FUNCTION_RESET_LOGICAL_UNIT:
            StorPortCompleteRequest(
                                    pHBAExt,
                                    pSrb->PathId,
                                    pSrb->TargetId,
                                    pSrb->Lun,
                                    SRB_STATUS_BUSY
                                   );
            srbStatus = SRB_STATUS_SUCCESS;
            break;
            
        case SRB_FUNCTION_RESET_DEVICE:
            StorPortCompleteRequest(
                                    pHBAExt,
                                    pSrb->PathId,
                                    pSrb->TargetId,
                                    SP_UNTAGGED,
                                    SRB_STATUS_TIMEOUT
                                   );
            srbStatus = SRB_STATUS_SUCCESS;
            break;
            
        case SRB_FUNCTION_PNP:                        
            status = SFBDHwHandlePnP(pHBAExt, (PSCSI_PNP_REQUEST_BLOCK)pSrb);
            srbStatus = pSrb->SrbStatus;
            
            break;

        case SRB_FUNCTION_POWER:                      
            // Do nothing.
            srbStatus = SRB_STATUS_SUCCESS;

            break;

        case SRB_FUNCTION_SHUTDOWN:                   
            // Do nothing.
            srbStatus = SRB_STATUS_SUCCESS;

            break;

        default:
            EventWriteSFBDHwStartIoUnsupported(NULL, pSrb->Function);
            KdPrint(("SFBDHwStartIo: SRB Function unsupported: %08X", pSrb->Function));
            srbStatus = SRB_STATUS_INVALID_REQUEST;
            break;

    } 

    // ResultDone is event set incase of a failure srbStatus.
    if (ResultDone==Result) 
    {                         
      pSrb->SrbStatus = srbStatus;

      // Note:  A miniport with real hardware would not always be calling RequestComplete from HwStorStartIo.  Rather,
      //        the miniport would typically be doing real I/O and would call RequestComplete only at the end of that
      //        real I/O, in its HwStorInterrupt or in a DPC routine.

      StorPortNotification(RequestComplete, pHBAExt, pSrb);
    }
    
    return TRUE;
}                                                     

/**************************************************************************************************/ 
/*                                                                                                */ 
/**************************************************************************************************/ 
SCSI_ADAPTER_CONTROL_STATUS
SFBDHwAdapterControl(
                   IN PVOID                     pVoidHBAExt, // Adapter device-object extension from StorPort.
                   IN SCSI_ADAPTER_CONTROL_TYPE ControlType,
                   IN PVOID                     pParameters
                  )
{
    PSCSI_SUPPORTED_CONTROL_TYPE_LIST pCtlTypList;
    ULONG                             i;
    pHW_HBA_EXT               pHBAExt = pVoidHBAExt;

    pHBAExt->AdapterState = ControlType;

    switch (ControlType) {
        case ScsiQuerySupportedControlTypes:
            
            // Get pointer to control type list
            pCtlTypList = (PSCSI_SUPPORTED_CONTROL_TYPE_LIST)pParameters;

            // Cycle through list to set TRUE for each type supported
            // making sure not to go past the MaxControlType
            for (i = 0; i < pCtlTypList->MaxControlType; i++)
                if ( i == ScsiQuerySupportedControlTypes ||
                     i == ScsiStopAdapter || i == ScsiRestartAdapter)
                {
                    pCtlTypList->SupportedTypeList[i] = TRUE;
                }
            break;

        case ScsiStopAdapter:
            
            // This is invoked for shutdown due to removal, resource reconfiguration and power management.
            SFBDStopAdapter(pHBAExt);
            break;

        case ScsiRestartAdapter:
            
            // This is only invoked when the adapter is restarted as part of power management.
            SFBDRestartAdapter(pHBAExt);
            break;

        default:
            break;
    } 

    return ScsiAdapterControlSuccess;
}                                                     

// This will send the request to the service to disconnect from the driver
// and shutdown the transport primitives.
VOID SFBDShutdownServiceTransport(pHW_HBA_EXT pHBAExt)
{

#if !defined(USE_NP_TRANSPORT)
    // Shutdown sockets
    ShutdownSockets(pHBAExt);
#else // defined(USE_NP_TRANSPORT)
    UNREFERENCED_PARAMETER(pHBAExt);
#endif // !defined(USE_NP_TRANSPORT)

}

// Performs required shutdown when AdapterControl gets ScsiStopAdapter request.
VOID
SFBDStopAdapter(IN pHW_HBA_EXT pHBAExt)               
{
    UNREFERENCED_PARAMETER(pHBAExt);
}                                                     

// Performs required initialization when AdapterControl gets ScsiRestartAdapter request.
VOID
SFBDRestartAdapter(IN pHW_HBA_EXT pHBAExt)
{
    UNREFERENCED_PARAMETER(pHBAExt);
}

// Resets the DeviceInfo to uninitialized state, excluding the Enabled Status
// that is set based upon the policy of the caller.
VOID ResetDeviceInfo(
    IN pSFBD_DEVICE_INFO    pDeviceInfo,
    IN UCHAR                PathId,
    IN UCHAR                TargetId,
    IN UCHAR                Lun    
)
{
    pDeviceInfo->DeviceType = DIRECT_ACCESS_DEVICE;
    pDeviceInfo->Lun = Lun;
    pDeviceInfo->TargetId = TargetId;
    pDeviceInfo->PathId = PathId;
    pDeviceInfo->DiskSize = 0;
    pDeviceInfo->LUID[0] = L'\0';
    pDeviceInfo->DiskSizeInSizeUnit = 0;
    pDeviceInfo->pLUExt = NULL;
    pDeviceInfo->SizeUnit = 0;
    pDeviceInfo->Mounted = 0;
    pDeviceInfo->ServicePort = 0;    
}

/**************************************************************************************************/ 
/*                                                                                                */ 
/* Create list of LUNs for specified HBA extension.                                               */ 
/*                                                                                                */ 
/**************************************************************************************************/ 
NTSTATUS
SFBDCreateDeviceList(IN       pHW_HBA_EXT    pHBAExt)
{
    NTSTATUS status = STATUS_SUCCESS;
    
    // FIELD_OFFSET gives us the cumulative size of the set of fields preceding it.
    ULONG  len = FIELD_OFFSET(SFBD_DEVICE_LIST, DeviceInfo) + (pHBAExt->LUNPerHBA * sizeof(SFBD_DEVICE_INFO));

    if (pHBAExt->pDeviceList) {
        // When will we reach here?
        NT_ASSERT(FALSE);
        ExFreePoolWithTag(pHBAExt->pDeviceList, SFBD_TAG_GENERAL);
    }

    pHBAExt->pDeviceList = ExAllocatePoolWithTag(NonPagedPoolNx, len, SFBD_TAG_GENERAL);
    if (!pHBAExt->pDeviceList) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    RtlZeroMemory(pHBAExt->pDeviceList, len);

    pHBAExt->pDeviceList->DeviceCount = pHBAExt->LUNPerHBA;

    // Initialize the LUs for each target and bus associated with the adapter.
    for (UCHAR iBusIndex = 0; iBusIndex < pHBAExt->MaximumNumberOfBuses; iBusIndex++)
    {
        for (UCHAR iTargetIndex = 0; iTargetIndex < pHBAExt->MaximumNumberOfTargets; iTargetIndex++)
        {
            for (UCHAR iLUIndex = 0; iLUIndex < pHBAExt->MaximumNumberOfLogicalUnits; iLUIndex++) 
            {
                int iDeviceIndex = SFBDGetDeviceIndex(pHBAExt, iBusIndex, iTargetIndex, iLUIndex);
                ResetDeviceInfo(&pHBAExt->pDeviceList->DeviceInfo[iDeviceIndex], iBusIndex, iTargetIndex, iLUIndex);
                pHBAExt->pDeviceList->DeviceInfo[iDeviceIndex].Enabled = FALSE;
            }
        }
    }

done:
    return status;
}                                                     

// Returns the reference to the next availabel DeviceInfo instance, or NULL if none is available.
pSFBD_DEVICE_INFO SFBDGetNextAvailableDeviceInfo(
    IN       pHW_HBA_EXT pHBAExt,
    PUCHAR                 pPathId,
    PUCHAR                 pTargetId,
    PUCHAR                 pLun
)
{
    // Init the outgoing data
    *pPathId = (UCHAR)-1;
    *pTargetId = (UCHAR)-1;
    *pLun = (UCHAR)-1;

    // Initialize the LUs for each target and bus associated with the adapter.
    for (UCHAR iBusIndex = 0; iBusIndex < pHBAExt->MaximumNumberOfBuses; iBusIndex++)
    {
        for (UCHAR iTargetIndex = 0; iTargetIndex < pHBAExt->MaximumNumberOfTargets; iTargetIndex++)
        {
            for (UCHAR iLUIndex = 0; iLUIndex < pHBAExt->MaximumNumberOfLogicalUnits; iLUIndex++)
            {
                int iDeviceIndex = SFBDGetDeviceIndex(pHBAExt, iBusIndex, iTargetIndex, iLUIndex);
                if (InterlockedCompareExchange(&pHBAExt->pDeviceList->DeviceInfo[iDeviceIndex].Enabled, TRUE, FALSE) == FALSE)
                {
                    // Found a reference to return
                    pSFBD_DEVICE_INFO pRetVal = &pHBAExt->pDeviceList->DeviceInfo[iDeviceIndex];

                    // Ensure that returned instance is initialized
                    ResetDeviceInfo(pRetVal, iBusIndex, iTargetIndex, iLUIndex);

                    *pPathId = iBusIndex;
                    *pTargetId = iTargetIndex;
                    *pLun = iLUIndex;
                    return pRetVal;
                }
            }
        }
    }

    return NULL;
}

ULONG SFBDGetDeviceIndex(
    IN       pHW_HBA_EXT pHBAExt,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun
)
{
    return Lun + (TargetId * pHBAExt->MaximumNumberOfLogicalUnits) + (PathId * pHBAExt->MaximumNumberOfTargets * pHBAExt->MaximumNumberOfLogicalUnits);
}
/**************************************************************************************************/ 
/*                                                                                                */ 
/* Return device info for specified device on specified HBA extension.                            */ 
/*                                                                                                */ 
/**************************************************************************************************/ 
pSFBD_DEVICE_INFO 
SFBDGetDevice(
                IN pHW_HBA_EXT          pHBAExt,    // Adapter device-object extension from StorPort.
                IN UCHAR                PathId,
                IN UCHAR                TargetId,
                IN UCHAR                Lun,
                IN BOOLEAN              IncludeUnmountedLU
               )
{
    pSFBD_DEVICE_INFO pRetVal = NULL;

    ULONG iDeviceIndex = SFBDGetDeviceIndex(pHBAExt, PathId, TargetId, Lun);
    pSFBD_DEVICE_INFO pDeviceInfo = SFBDGetDeviceForLUIndex(pHBAExt, iDeviceIndex);

    if (pDeviceInfo)
    {
        if ((SFBDIsDeviceEnabled(pDeviceInfo) == TRUE) && ((pDeviceInfo->Mounted == TRUE) || IncludeUnmountedLU))
        {
            pRetVal = pDeviceInfo;
        }
    }

    return pRetVal;
}                                                   

// Returns the DeviceInfo instance with the specified LU ID.
pSFBD_DEVICE_INFO
SFBDGetDeviceForLUID(
    IN pHW_HBA_EXT          pHBAExt,    
    IN PWCHAR               pLUID,
    IN BOOLEAN              IncludeUnmountedLU
)
{
    pSFBD_DEVICE_LIST pDevList = pHBAExt->pDeviceList;
    pSFBD_DEVICE_INFO pRetVal = NULL;

    if (!pDevList || (pDevList->DeviceCount == 0))
    {
        return pRetVal;
    }


    // Locate the device corresponding to the PathId/TargetId/Lun to report its type.
    for (UCHAR iBusIndex = 0; iBusIndex < pHBAExt->MaximumNumberOfBuses; iBusIndex++)
    {
        for (UCHAR iTargetIndex = 0; iTargetIndex < pHBAExt->MaximumNumberOfTargets; iTargetIndex++)
        {
            for (UCHAR iLUIndex = 0; iLUIndex < pHBAExt->MaximumNumberOfLogicalUnits; iLUIndex++)
            {
                int iDeviceIndex = SFBDGetDeviceIndex(pHBAExt, iBusIndex, iTargetIndex, iLUIndex);
                if ((SFBDIsDeviceEnabled(&pHBAExt->pDeviceList->DeviceInfo[iDeviceIndex]) == TRUE) &&
                    ((pHBAExt->pDeviceList->DeviceInfo[iDeviceIndex].Mounted == TRUE) || IncludeUnmountedLU) &&
                    (wcsncmp(pHBAExt->pDeviceList->DeviceInfo[iDeviceIndex].LUID, pLUID, wcslen(pLUID)) == 0))
                {
                    pRetVal = &pHBAExt->pDeviceList->DeviceInfo[iDeviceIndex];
                    break;
                }
            }
        }
    }

    return pRetVal;
}

// Returns the DeviceInfo instance for the specified LU index.
pSFBD_DEVICE_INFO
SFBDGetDeviceForLUIndex(
    IN pHW_HBA_EXT          pHBAExt,
    IN ULONG                indexLu
)
{
    pSFBD_DEVICE_LIST pDevList = pHBAExt->pDeviceList;
    pSFBD_DEVICE_INFO pRetVal = NULL;

    if (!pDevList || (pDevList->DeviceCount == 0) || (indexLu >= pHBAExt->LUNPerHBA))
    {
        return pRetVal;
    }


    pRetVal = &pHBAExt->pDeviceList->DeviceInfo[indexLu];

    return pRetVal;
}

BOOLEAN SFBDIsDeviceEnabled(
    IN pSFBD_DEVICE_INFO pDeviceInfo
)
{
    NT_ASSERT(pDeviceInfo != NULL);

    return (BOOLEAN)InterlockedOr(&pDeviceInfo->Enabled, 0);
}

BOOLEAN SFBDFreeDeviceInfo(
    IN pHW_HBA_EXT pHBAExt,
    IN pSFBD_DEVICE_INFO pDeviceInfo
)
{
    NT_ASSERT(pDeviceInfo != NULL);

    // Remove the LUExt from the HBAList
    pHW_LU_EXTENSION pLUExt = (pHW_LU_EXTENSION)pDeviceInfo->pLUExt;
    BOOLEAN fRemovedLUExt = pLUExt?SFBDRemoveLUExtFromHBAList(pHBAExt, pLUExt):TRUE;
    
    // Mark the device as free only if we were able to remove it from LUExt
    if (fRemovedLUExt)
    {
        InterlockedAnd(&pDeviceInfo->Enabled, 0);
    }

    return fRemovedLUExt;
}

// Marks the device info associated with the service port as disabled
// and trigger notification for the bus so that storage stack reflects the change.
VOID SFBDShutdownVolumesForServicePort(
    IN pHW_HBA_EXT pHBAExt,
    IN DWORD dwServicePort)
{
    // No need to cleanup DeviceInfo if driver is shutting down since the memory would get freed up
    // anyways.
    BOOLEAN fCleanupAllDevicesInfo = (dwServicePort == (DWORD)-1) ? TRUE : FALSE;

    if (fCleanupAllDevicesInfo == TRUE)
    {
        EventWriteVolumeShutdownAll(NULL);
    }

    // Locate the device corresponding to the PathId/TargetId/Lun to report its type.
    for (UCHAR iBusIndex = 0; iBusIndex < pHBAExt->MaximumNumberOfBuses; iBusIndex++)
    {
        BOOLEAN fReportChangeForBus = FALSE;

        for (UCHAR iTargetIndex = 0; iTargetIndex < pHBAExt->MaximumNumberOfTargets; iTargetIndex++)
        {
            for (UCHAR iLUIndex = 0; iLUIndex < pHBAExt->MaximumNumberOfLogicalUnits; iLUIndex++)
            {
                int iDeviceIndex = SFBDGetDeviceIndex(pHBAExt, iBusIndex, iTargetIndex, iLUIndex);
                if ((pHBAExt->pDeviceList->DeviceInfo[iDeviceIndex].ServicePort == dwServicePort) ||
                    (fCleanupAllDevicesInfo == TRUE))
                {
                    BOOLEAN fVolumeShutdown = FALSE;
                    if ((fCleanupAllDevicesInfo == TRUE) || SFBDFreeDeviceInfo(pHBAExt, &pHBAExt->pDeviceList->DeviceInfo[iDeviceIndex]))
                    {
                        fVolumeShutdown = TRUE;
                        fReportChangeForBus = TRUE;
                    }
                    
                    if (fCleanupAllDevicesInfo == FALSE)
                    {
                        EventWriteVolumeShutdown(NULL, fVolumeShutdown ? L"Successfully" : L"Unable to", pHBAExt->pDeviceList->DeviceInfo[iDeviceIndex].LUID, iBusIndex, iTargetIndex, iLUIndex);
                    }
                }
            }
        }

        if (fReportChangeForBus)
        {
             StorPortNotification(BusChangeDetected, pHBAExt, iBusIndex);    
        }
    }
}
/**************************************************************************************************/                         
/*                                                                                                */                         
/* SFBDTracingInit.                                                                                 */                         
/*                                                                                                */                         
/**************************************************************************************************/                         
VOID                                                                                                                         
SFBDTracingInit(
              IN PVOID pArg1,                                                                                  
              IN PVOID pArg2
             )                                                                                                            
{   
    UNREFERENCED_PARAMETER(pArg1);
    UNREFERENCED_PARAMETER(pArg2);
}                                                     

/**************************************************************************************************/                         
/*                                                                                                */                         
/* SFBDTracingCleanUp.                                                                              */                         
/*                                                                                                */                         
/* This is called when the driver is being unloaded.                                              */                         
/*                                                                                                */                         
/**************************************************************************************************/                         
VOID                                                                                                                         
SFBDTracingCleanup(IN PVOID pArg1)
{              
    UNREFERENCED_PARAMETER(pArg1);
    EventUnregisterMicrosoft_ServiceFabric_BlockStoreMiniport();
}                                                     

/**************************************************************************************************/                         
/*                                                                                                */                         
/* SFBDHwFreeAdapterResources.                                                                      */                         
/*                                                                                                */                         
/**************************************************************************************************/                         
VOID
SFBDHwFreeAdapterResources(IN PVOID pVoidHBAExt)
{
    PLIST_ENTRY           pNextEntry; 
    pHW_HBA_EXT           pLclHBAExt;
    KLOCK_QUEUE_HANDLE    LockHandle;
    BOOLEAN               ReleaseGlobalAllocations = FALSE;
    pHW_HBA_EXT           pHBAExt = pVoidHBAExt;

    SFBDShutdownServiceTransport(pHBAExt);

    // Shutdown all the active volumes
    SFBDShutdownVolumesForServicePort(pHBAExt, (DWORD)-1);

    // Take the global lock before freeing the adapter resources.
    KeAcquireInStackQueuedSpinLock(&g_pSFBDDriver->DrvInfoLock, &LockHandle);

    // Go through linked list of HBA extensions.
    for (                                             
         pNextEntry = g_pSFBDDriver->ListMPHBAObj.Flink;
         pNextEntry != &g_pSFBDDriver->ListMPHBAObj;
         pNextEntry =  pNextEntry->Flink
        ) 
    {
        pLclHBAExt = CONTAINING_RECORD(pNextEntry, HW_HBA_EXT, List);

        // Did we find the HBA Storport wants to free up?
        if (pLclHBAExt == pHBAExt) 
        {                    
            RemoveEntryList(pNextEntry);
            g_pSFBDDriver->DrvInfoNbrMPHBAObj--;

            // If this is the last adapter being removed, then driver is getting unloaded.
            // This is our change to release global resources.
            if (g_pSFBDDriver->DrvInfoNbrMPHBAObj == 0)
            {
                ReleaseGlobalAllocations = TRUE;
            }

            break;
        }
    }

    KeReleaseInStackQueuedSpinLock(&LockHandle);

    ExFreePoolWithTag(pHBAExt->pDeviceList, SFBD_TAG_GENERAL);

    if (ReleaseGlobalAllocations)
    {
        ExFreePoolWithTag(g_pSFBDDriver, SFBD_TAG_GENERAL);
    }
}                                                     

// Removes the specified LUExtension from the HBA's LUExt list.
BOOLEAN
SFBDRemoveLUExtFromHBAList(IN pHW_HBA_EXT pHBAExt, IN pHW_LU_EXTENSION pLUExtToRemove)
{
    PLIST_ENTRY           pNextEntry = NULL;
    pHW_LU_EXTENSION      pLUExt = NULL;
    BOOLEAN               fRemovedLUExt = FALSE;

    KLOCK_QUEUE_HANDLE    LockHandle;

    KeAcquireInStackQueuedSpinLock(                   // Serialize the linked list of LUN extensions.              
        &pHBAExt->LUListLock, &LockHandle);

    // Go through linked list of HBA extensions.
    for (
        pNextEntry = pHBAExt->LUList.Flink;
        pNextEntry != &pHBAExt->LUList;
        pNextEntry = pNextEntry->Flink
        )
    {
        pLUExt = CONTAINING_RECORD(pNextEntry, HW_LU_EXTENSION, List);

        // Did we find the HBA Storport wants to free up?
        if (pLUExt == pLUExtToRemove)
        {
            RemoveEntryList(pNextEntry);
            CLEAR_FLAG(pLUExt->LUFlags, LU_DEVICE_INITIALIZED);
            fRemovedLUExt = TRUE;
            break;
        }
    }

    KeReleaseInStackQueuedSpinLock(&LockHandle);

    return fRemovedLUExt;
}

// Required since we have implemented ProcessIrp per the Virtual miniport contract.
// 
// Does not have anything to do for now.
VOID
SFBDHwCompleteServiceIrp(
    IN PVOID pHBAExt
)
{
    UNREFERENCED_PARAMETER(pHBAExt);
}

// Processes the IOCTL codes received from the user mode controller.   
VOID
SFBDHwProcessIrp(
              IN PVOID         pVoidHBAExt,             // Adapter device-object extension from StorPort.
              IN PVOID         pVoidIrp
             )
{
    pHW_HBA_EXT   pHBAExt = pVoidHBAExt;
    PIRP pIrp = pVoidIrp;
    if (pIrp) 
    {
        NTSTATUS           Status = STATUS_SUCCESS;
        PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
        ULONG              inputBufferLength;
        DWORD              returnDataLength = sizeof(SFBDCommand);

        // We only support DeviceIoControl IOCTLs.
        if (pIrpStack->MajorFunction == IRP_MJ_DEVICE_CONTROL)
        {
            inputBufferLength = pIrpStack->Parameters.DeviceIoControl.InputBufferLength;

            switch (pIrpStack->Parameters.DeviceIoControl.IoControlCode) 
            {
                case IOCTL_MINIPORT_PROCESS_SERVICE_IRP:

                    // Cast the buffer as SFBDCommand
                    PSFBDCommand pCommand = (PSFBDCommand)pIrp->AssociatedIrp.SystemBuffer;
                    if (!pCommand || (inputBufferLength < sizeof(SFBDCommand)))
                    {
                        // Invalid request came in.
                        Status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        EventWriteCommandReceived(NULL, pCommand->command);
                        // Default response status is success
                        pCommand->cmdStatus = Success;

                        // Parse the command and return the details.
                        if (pCommand->command == GetAdapterInfo)
                        {
                            pCommand->basicInfo.BusCount = pHBAExt->MaximumNumberOfBuses;
                            pCommand->basicInfo.TargetCount = pHBAExt->MaximumNumberOfTargets;
                            pCommand->basicInfo.LUCount = pHBAExt->MaximumNumberOfLogicalUnits;
                        }
                        else if (pCommand->command == CreateLU)
                        {
                            // Find next available DeviceInfo for creating the LU
                            pSFBD_DEVICE_INFO pDeviceInfo = SFBDGetNextAvailableDeviceInfo(pHBAExt, &pCommand->luInfo.PathId, &pCommand->luInfo.TargetId, &pCommand->luInfo.Lun);
                            if (!pDeviceInfo)
                            {
                                pCommand->cmdStatus = DeviceNotAvailable;
                            }
                            else
                            {
                                if ((pCommand->luInfo.diskSize < 1) || (wcslen(pCommand->luInfo.wszLUID) == 0))
                                {
                                    // Bail out on invalid arguments.
                                    pCommand->cmdStatus = InvalidArguments;
                                }
                                else
                                {
                                    // Register the LU with the BlockStore Service
                                    NTSTATUS status = ProcessManagementRequest(RegisterLU, pHBAExt, pCommand->luInfo.wszLUID, pCommand->luInfo.diskSize, pCommand->luInfo.sizeUnit, 1, NULL, pCommand->servicePort);
                                    if (NT_SUCCESS(status))
                                    {
                                        pDeviceInfo->ServicePort = pCommand->servicePort;

                                        pDeviceInfo->DiskSizeInSizeUnit = pCommand->luInfo.diskSize;
                                        pDeviceInfo->SizeUnit = pCommand->luInfo.sizeUnit;

                                        // Copy the disk size and LUID to device info
                                        pDeviceInfo->DiskSize = SFBDGetDiskSizeInBytesFromSizeUnit(pCommand->luInfo.diskSize, pCommand->luInfo.sizeUnit);
                                        wcscpy_s(pDeviceInfo->LUID, _countof(pDeviceInfo->LUID), pCommand->luInfo.wszLUID);

                                        // Mark the device as mounted
                                        pDeviceInfo->Mounted = TRUE;

                                        // DeviceInfo is already marked Enabled
                                        NT_ASSERT(pDeviceInfo->Enabled == TRUE);

                                        // Deliver bus change notification so that OpInquiry comes in and update the
                                        // storage stack.
                                        StorPortNotification(BusChangeDetected, pHBAExt, pDeviceInfo->PathId);
                                    }
                                    else
                                    {
                                        pCommand->cmdStatus = FailedToRegisterLU;
                                    }
                                }

                                // Free the DeviceInfo structure if command failed.
                                if (pCommand->cmdStatus != Success)
                                {
                                    if (pDeviceInfo != NULL)
                                    {
                                        SFBDFreeDeviceInfo(pHBAExt, pDeviceInfo);
                                    }    
                                }
                            }
                        }
                        else if (pCommand->command == RefreshLUList)
                        {
                            // Service has requested us to connect to it and refresh the storage stack
                            // with any LUs active with the service.
                            PLU_REGISTRATION_LIST pRegistrationList = ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(LU_REGISTRATION_LIST), SFBD_TAG_GENERAL);
                            if (pRegistrationList)
                            {
                                RtlZeroMemory(pRegistrationList, sizeof(LU_REGISTRATION_LIST));

                                // Fetch the list of LUs registered with the service and initialize the DeviceList with it.
                                NTSTATUS ntStatus = ProcessManagementRequest(FetchRegisteredLUList, pHBAExt, NULL, 0, 0, 0, pRegistrationList, pCommand->servicePort);
                                if (NT_SUCCESS(ntStatus))
                                {
                                    ntStatus = SFBDInitializeDeviceListWithRegisteredLU(pHBAExt, pRegistrationList, pCommand->servicePort);
                                    if (!NT_SUCCESS(ntStatus))
                                    {
                                        EventWriteDeviceListWithRegisteredLUFailed(NULL, ntStatus);
                                        KdPrint(("DeviceListWithRegisteredLU initialization failed due to NTSTATUS %08X", ntStatus));
                                        pCommand->cmdStatus = DeviceListInitializedFailed;
                                    }
                                }
                                else
                                {
                                    EventWriteFetchRegisteredLUListFailed(NULL, ntStatus);
                                    KdPrint(("FetchRegisteredLUList failed due to NTSTATUS %08X", ntStatus));
                                    pCommand->cmdStatus = UnableToFetchLUList;
                                }
                                ExFreePoolWithTag(pRegistrationList, SFBD_TAG_GENERAL);
                            }
                            else
                            {
                                // Insufficient memory for us to work with.
                                EventWriteRefreshLUListAllocationFailed(NULL);
                                Status = STATUS_INSUFFICIENT_RESOURCES;
                            }
                        }
                        else if (pCommand->command == ShutdownServiceVolumes)
                        {
                            // Service has requested us to shutdown the volumes associated with it and refresh the storage stack.
                            SFBDShutdownVolumesForServicePort(pHBAExt, pCommand->servicePort);
                        }
                        else if (pCommand->command == GetInfoForLu)
                        {
                            // Locate the LU for which the information is requested.
                            pSFBD_DEVICE_INFO pDeviceInfo = SFBDGetDeviceForLUID(pHBAExt, pCommand->luInfo.wszLUID, TRUE);
                            if (!pDeviceInfo)
                            {
                                pCommand->cmdStatus = DeviceNotFound;
                            }
                            else
                            {
                                wcscpy_s(pCommand->luInfo.wszLUID, _countof(pCommand->luInfo.wszLUID), pDeviceInfo->LUID);
                                pCommand->luInfo.diskSize = pDeviceInfo->DiskSizeInSizeUnit;
                                pCommand->luInfo.sizeUnit = pDeviceInfo->SizeUnit;
                                pCommand->luInfo.Enabled = (BOOLEAN)pDeviceInfo->Enabled;
                                pCommand->luInfo.Mounted = pDeviceInfo->Mounted;
                                pCommand->luInfo.PathId = pDeviceInfo->PathId;
                                pCommand->luInfo.TargetId = pDeviceInfo->TargetId;
                                pCommand->luInfo.Lun = pDeviceInfo->Lun;
                                pCommand->servicePort = pDeviceInfo->ServicePort;
                            }
                        }
                        else if (pCommand->command == DeleteLu)
                        {
                            // Locate the LU for which the information is requested.
                            pSFBD_DEVICE_INFO pDeviceInfo = SFBDGetDeviceForLUID(pHBAExt, pCommand->luInfo.wszLUID, TRUE);
                            if (!pDeviceInfo)
                            {
                                pCommand->cmdStatus = DeviceNotFound;
                            }
                            else
                            {
                                NTSTATUS status = ProcessManagementRequest(UnregisterLU, pHBAExt, pDeviceInfo->LUID, 0, 0, 0, NULL, pDeviceInfo->ServicePort);
                                if (NT_SUCCESS(status))
                                {
                                    UCHAR deviceBusId = pDeviceInfo->PathId;

                                    // Free up the DeviceInfo
                                    if (!SFBDFreeDeviceInfo(pHBAExt, pDeviceInfo))
                                    {
                                        pCommand->cmdStatus = FailedToUpdateDeviceList;
                                    }
                                    else
                                    {
                                        // Deliver bus change notification so that OpInquiry comes in and update the
                                        // storage stack.
                                        StorPortNotification(BusChangeDetected, pHBAExt, deviceBusId);
                                    }
                                }
                                else
                                {
                                    pCommand->cmdStatus = FailedToUnregisterLU;
                                }
                            }
                        }
                        else if (pCommand->command == UnmountLu)
                        {
                            // Locate the LU for which the information is requested.
                            pSFBD_DEVICE_INFO pDeviceInfo = SFBDGetDeviceForLUID(pHBAExt, pCommand->luInfo.wszLUID, TRUE);
                            if (!pDeviceInfo)
                            {
                                pCommand->cmdStatus = DeviceNotFound;
                            }
                            else
                            {
                                if (pDeviceInfo->Mounted == FALSE)
                                {
                                    // LU is not mounted, so cannot unmount it.
                                    pCommand->cmdStatus = LUNotMounted;
                                }
                                else
                                {
                                    NTSTATUS status = ProcessManagementRequest(UnmountLU, pHBAExt, pDeviceInfo->LUID, 0, 0, 0, NULL, pDeviceInfo->ServicePort);
                                    if (NT_SUCCESS(status))
                                    {
                                        pHW_LU_EXTENSION pLUExt = (pHW_LU_EXTENSION)pDeviceInfo->pLUExt;

                                        // Remove the LUExt from the HBAList
                                        if (!SFBDRemoveLUExtFromHBAList(pHBAExt, pLUExt))
                                        {
                                            pCommand->cmdStatus = FailedToUpdateDeviceList;
                                        }
                                        else
                                        {
                                            pDeviceInfo->Mounted = FALSE;

                                            // Deliver bus change notification so that OpInquiry comes in and update the
                                            // storage stack.
                                            StorPortNotification(BusChangeDetected, pHBAExt, pDeviceInfo->PathId);
                                        }
                                    }
                                    else
                                    {
                                        pCommand->cmdStatus = FailedToUnmountLU;
                                    }
                                }
                            }

                        }
                        else if (pCommand->command == MountLu)
                        {
                            // Locate the LU for which the information is requested.
                            pSFBD_DEVICE_INFO pDeviceInfo = SFBDGetDeviceForLUID(pHBAExt, pCommand->luInfo.wszLUID, TRUE);
                            if (!pDeviceInfo)
                            {
                                pCommand->cmdStatus = DeviceNotFound;
                            }
                            else
                            {
                                if (pDeviceInfo->Mounted == TRUE)
                                {
                                    // LU is already mounted.
                                    pCommand->cmdStatus = LUAlreadyMounted;
                                }
                                else
                                {
                                    NTSTATUS status = ProcessManagementRequest(MountLU, pHBAExt, pDeviceInfo->LUID, 0, 0, 1, NULL, pDeviceInfo->ServicePort);
                                    if (NT_SUCCESS(status))
                                    {
                                        pDeviceInfo->Mounted = TRUE;

                                        // Deliver bus change notification so that OpInquiry comes in and update the
                                        // storage stack.
                                        StorPortNotification(BusChangeDetected, pHBAExt, pDeviceInfo->PathId);
                                    }
                                    else
                                    {
                                        pCommand->cmdStatus = FailedToMountLU;
                                    }
                                }
                            }

                        }
                        else if (pCommand->command == GetLuCount)
                        {
                            // Return the total number of LUs supported by the adapter.
                            pCommand->numLu = pHBAExt->LUNPerHBA;
                        }
                        else if (pCommand->command == GetInfoForLuIndex)
                        {
                            // For the specified LU Index, get the LU Info.
                            pSFBD_DEVICE_INFO pDeviceInfo = SFBDGetDeviceForLUIndex(pHBAExt, pCommand->luIndex);
                            if (!pDeviceInfo)
                            {
                                pCommand->cmdStatus = InvalidLuIndex;
                            }
                            else
                            {
                                wcscpy_s(pCommand->luInfo.wszLUID, _countof(pCommand->luInfo.wszLUID), pDeviceInfo->LUID);
                                pCommand->luInfo.diskSize = pDeviceInfo->DiskSizeInSizeUnit;
                                pCommand->luInfo.sizeUnit = pDeviceInfo->SizeUnit;
                                pCommand->luInfo.Enabled = (BOOLEAN)pDeviceInfo->Enabled;
                                pCommand->luInfo.Mounted = pDeviceInfo->Mounted;
                                pCommand->luInfo.PathId = pDeviceInfo->PathId;
                                pCommand->luInfo.TargetId = pDeviceInfo->TargetId;
                                pCommand->luInfo.Lun = pDeviceInfo->Lun;
                                pCommand->servicePort = pDeviceInfo->ServicePort;
                            }
                        }
                        else
                        {
                            // Unsupported command - should not be here.
                            NT_ASSERT(FALSE);
                            pCommand->cmdStatus = UnsupportedCommand;
                        }
                    }
                    break;
#if defined(USE_DMA_FOR_SRB_IO)
                case IOCTL_SFBD_READ_FROM_USERMODE:
                case IOCTL_SFBD_READ_FROM_SRB:

                    // We have no data to return.
                    returnDataLength = 0;
                    if (!pIrp->MdlAddress)
                    {
                        Status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        // Fetch the input buffer to get access to the SRB
                        PSFBDCommand pCommandRW = (PSFBDCommand)pIrp->AssociatedIrp.SystemBuffer;
                        if (!pCommandRW || (inputBufferLength < sizeof(SFBDCommand)))
                        {
                            // Invalid request came in.
                            Status = STATUS_INVALID_PARAMETER_1;
                        }
                        else
                        {
                            // Fetch the Srb for which R/W needs to be performed.
                            PSCSI_REQUEST_BLOCK pSrb = (PSCSI_REQUEST_BLOCK)pCommandRW->srbInfo.SRBAddress;
                            PHW_SRB_EXTENSION pSrbExt = (PHW_SRB_EXTENSION)pSrb->SrbExtension;

                            // First, check if the length matches the DataBuffer length for the SRB
                            if (pCommandRW->srbInfo.DataBufferLength != pSrb->DataTransferLength)
                            {
                                Status = STATUS_INVALID_PARAMETER_2;
                                EventWriteDataBufferLengthMismatched(NULL, pSrb, pCommandRW->srbInfo.DataBufferLength, pSrb->DataTransferLength);
                                KdPrint(("IOCTL_SFB_READBLOCK: DataBufferLengthMismatch for SRB %p; Incoming Length: %08X, Expected Length: %08X", pSrb, pCommandRW->srbInfo.DataBufferLength, pSrb->DataTransferLength));
                            }
                            else
                            {
                                // TODO: When doing actual DMA, we do not need to fetch the kernel virtual address
                                //       and should do a MapTransfer instead.
                                PVOID pUserModeBuffer = MmGetSystemAddressForMdlSafe(pIrp->MdlAddress, NormalPagePriority | MdlMappingNoExecute);
                                if (!pUserModeBuffer)
                                {
                                    Status = STATUS_INSUFFICIENT_RESOURCES;
                                }
                                else
                                {
                                    // Perform the R/W operation
                                    if (pIrpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_SFBD_READ_FROM_USERMODE)
                                    {
                                        // Copy the block data from the user buffer to the SRB DataBuffer
                                        RtlCopyMemory(pSrbExt->SystemAddressDataBuffer, pUserModeBuffer, pSrb->DataTransferLength);
                                    }
                                    else
                                    {
                                        // Copy the block data from the SRB DataBuffer to the user buffer
                                        RtlCopyMemory(pUserModeBuffer, pSrbExt->SystemAddressDataBuffer, pSrb->DataTransferLength);
                                    }
                                }
                            }
                        }
                    }
                    break;
#endif // defined(USE_DMA_FOR_SRB_IO)
                default:
                    NT_ASSERT(FALSE);
                    Status = STATUS_INVALID_DEVICE_REQUEST;
                    break;
            }
        }
        else
        {
            //Treat the unsupported request as invalid. 
            NT_ASSERT(FALSE);
            Status = STATUS_INVALID_DEVICE_REQUEST;
        }

        // Save the Status and complete the request
        pIrp->IoStatus.Status = Status;
        pIrp->IoStatus.Information = returnDataLength;
        
#if defined(USE_DMA_FOR_SRB_IO)
        // We will complete the Irp only if it came via StorPort which always sends the Adapter extension reference.
        // This will not be set when our DeviceIoControl callback invokes this function for our custom IOCTL. 
        if (pHBAExt)
#endif // defined(USE_DMA_FOR_SRB_IO)
        {
            StorPortCompleteServiceIrp(pHBAExt, pIrp);
        }
    }
}                                                     

#if defined(USE_DMA_FOR_SRB_IO)
NTSTATUS SFBDDeviceIoControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    //
    // Get the current IRP stack location
    //
    PIO_STACK_LOCATION ioStack = IoGetCurrentIrpStackLocation(Irp);

    ULONG dwControlCode = ioStack->Parameters.DeviceIoControl.IoControlCode;

    if ((dwControlCode == IOCTL_SFBD_READ_FROM_SRB) || (dwControlCode == IOCTL_SFBD_READ_FROM_USERMODE))
    {
        // Since this is our custom IOCTL code, process it.
        SFBDHwProcessIrp(NULL, Irp);

        // Set the status to be the one our helper set.
        if (Irp)
        {
            Status = Irp->IoStatus.Status;
        }

        // Complete the request
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
    else
    {
        // Otherwise, chain back to StorPort's callback for DeviceIoControl.
        Status = pMPDrvInfoGlobal->pFuncDeviceIoControl(DeviceObject, Irp);
    }

    return Status;
}
#endif // defined(USE_DMA_FOR_SRB_IO)

#if defined(DBG)
VOID SFBDDecrementActiveConnections(
    IN pHW_HBA_EXT pHBAExt
)
{
    // Decrement the active connection count
    if (InterlockedDecrement64((volatile LONG64*)&pHBAExt->ActiveConnections) == 0)
    {
        // Are we being shutdown? If so, set the shutdown event.
        if (InterlockedOr((volatile LONG*)&pHBAExt->IsShutdownInProgress, 0) == 1)
        {
            KeSetEvent(&pHBAExt->ShutdownSyncEvent, IO_NO_INCREMENT, FALSE);
        }
    }
}
#endif // defined(DBG)