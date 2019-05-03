// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

/*++

Module Name:

scsi.c

Abstract:

This file SCSI support for Service Fabric BlockStore Device's interaction with the BlockStore Service.

Environment:

Kernel mode only.

--*/   

#define MPScsiFile     "2.025"

#include "inc\SFBDVMiniport.h"
#include "Microsoft-ServiceFabric-BlockStoreMiniportEvents.h"


/**************************************************************************************************/     
/*                                                                                                */     
/**************************************************************************************************/     
UCHAR
ScsiExecuteMain(
                IN pHW_HBA_EXT          pHBAExt,    // Adapter device-object extension from StorPort.
                IN PSCSI_REQUEST_BLOCK  pSrb,
                IN PUCHAR               pResult
               )
{
    pHW_LU_EXTENSION pLUExt;
    UCHAR            status = SRB_STATUS_INVALID_REQUEST;

    *pResult = ResultDone;

    // Get the LU extension from StorPort to determine if the device is present or not.
    pLUExt = StorPortGetLogicalUnit(pHBAExt,          
                                    pSrb->PathId,
                                    pSrb->TargetId,
                                    pSrb->Lun 
                                   );
    if (!pLUExt) {
        status = SRB_STATUS_NO_DEVICE;
        goto Done;
    }

    // Handle sufficient opcodes to support a LUN suitable for a file system. Other opcodes are failed.
    // TODO: Evaluate what other CDB commands need to be supported and go over the following implementations.

    switch (pSrb->Cdb[0]) {

        case SCSIOP_TEST_UNIT_READY:
        case SCSIOP_SYNCHRONIZE_CACHE:
        case SCSIOP_START_STOP_UNIT:
        case SCSIOP_VERIFY:
            status = SRB_STATUS_SUCCESS;
            break;

        case SCSIOP_INQUIRY:
            status = ScsiOpInquiry(pHBAExt, pLUExt, pSrb);
            break;

        case SCSIOP_READ_CAPACITY:
        case SCSIOP_READ_CAPACITY16:
            status = ScsiOpReadCapacity(pHBAExt, pLUExt, pSrb);
            break;

        case SCSIOP_READ:
        case SCSIOP_READ16:
            status = ScsiReadWriteSetup(pHBAExt, pLUExt, pSrb, ActionRead, pResult);
            break;

        case SCSIOP_WRITE:
        case SCSIOP_WRITE16:
            status = ScsiReadWriteSetup(pHBAExt, pLUExt, pSrb, ActionWrite, pResult);
            break;

        case SCSIOP_MODE_SENSE:
            status = ScsiOpModeSense(pHBAExt, pLUExt, pSrb);
            break;

        case SCSIOP_REPORT_LUNS:                      
            status = ScsiOpReportLuns(pHBAExt, pLUExt, pSrb);
            break;

        default:
            EventWriteScsiExecuteMainUnsupported(NULL, pSrb->Cdb[0]);
            KdPrint(("ScsiExecuteMain: Unsupported CDB code %08X", pSrb->Cdb[0]));
            status = SRB_STATUS_INVALID_REQUEST;
            break;

    } // switch (pSrb->Cdb[0])

Done:
    return status;
}                                                     

/**************************************************************************************************/     
/*                                                                                                */     
/* Initialize the maximum block size for the disk.                                                */     
/*                                                                                                */     
/**************************************************************************************************/     
VOID
ScsiInitDisk(
    IN pHW_LU_EXTENSION pLUExt
)
{
    pLUExt->MaxBlocks = pLUExt->DeviceInfo->DiskSize / SECTOR_SIZE;
}                                                    

/**************************************************************************************************/     
/*                                                                                                */     
/**************************************************************************************************/     
UCHAR
ScsiOpInquiry(
              IN pHW_HBA_EXT          pHBAExt,      // Adapter device-object extension from StorPort.
              IN pHW_LU_EXTENSION     pLUExt,       // LUN device-object extension from StorPort.
              IN PSCSI_REQUEST_BLOCK  pSrb
             )
{
    PINQUIRYDATA          pInqData = pSrb->DataBuffer;// Point to Inquiry buffer.
    pSFBD_DEVICE_INFO       pDeviceInfo = NULL;
    UCHAR                 status = SRB_STATUS_SUCCESS;
    PCDB                  pCdb;

    KLOCK_QUEUE_HANDLE    LockHandle;

    RtlZeroMemory((PUCHAR)pSrb->DataBuffer, pSrb->DataTransferLength);

    pCdb = (PCDB)pSrb->Cdb;
    if (pCdb->CDB6INQUIRY3.EnableVitalProductData == 1) 
    {
        status = ScsiOpVPD(pHBAExt, pLUExt, pSrb);
        goto done;
    }

    // Fetch the device info reference for an enabled LU corresponding to PathId/TargetId/Lun
    pDeviceInfo = SFBDGetDevice(pHBAExt, pSrb->PathId, pSrb->TargetId, pSrb->Lun, FALSE);
    if (!pDeviceInfo || (pDeviceInfo->DeviceType == DEVICE_NOT_FOUND))
    {
        pSrb->DataTransferLength = 0;
        status = SRB_STATUS_INVALID_LUN;
        goto done;
    }

    pInqData->DeviceType = pDeviceInfo->DeviceType;
    pInqData->RemovableMedia = FALSE;
    pInqData->CommandQueue = TRUE;

    RtlMoveMemory(pInqData->VendorId, pHBAExt->VendorId, MAXLEN_VENDOR_ID);
    RtlMoveMemory(pInqData->ProductId, pHBAExt->ProductId, MAXLEN_PRODUCT_ID);
    RtlMoveMemory(pInqData->ProductRevisionLevel, pHBAExt->ProductRevision, MAXLEN_PRODUCT_REV);

    if (pDeviceInfo->DeviceType != DIRECT_ACCESS_DEVICE) {
        
        // Shouldn't happen.
        NT_ASSERT(FALSE);
        goto done;
    }

    // Check if the device has already been seen.
    if (GET_FLAG(pLUExt->LUFlags, LU_DEVICE_INITIALIZED)) {
        
        // This is an existing device and nothing needs to be done.
        goto done;
    }

    //
    // A new LUN.
    //

    // Save the LU Extension reference in DeviceInfo and vice-versa
    pDeviceInfo->pLUExt = pLUExt;
    pLUExt->DeviceInfo = pDeviceInfo;
    
    // Initialize the disk params for the new LUN.
    ScsiInitDisk(pLUExt);

    // Flag the LU as initialized. 
    SET_FLAG(pLUExt->LUFlags, LU_DEVICE_INITIALIZED);

    // Set the Queue depth for the LU
    BOOLEAN fSetupDepth = StorPortSetDeviceQueueDepth(pHBAExt, pDeviceInfo->PathId, pDeviceInfo->TargetId, pDeviceInfo->Lun, QUEUE_DEPTH_PER_LU);
    if (!fSetupDepth)
    {
        NT_ASSERT(fSetupDepth);
    }

    KeAcquireInStackQueuedSpinLock(                   // Serialize the linked list of LUN extensions.              
                                   &pHBAExt->LUListLock, &LockHandle);

    InsertTailList(&pHBAExt->LUList, &pLUExt->List);  // Add LUN extension to list in HBA extension.

    KeReleaseInStackQueuedSpinLock(&LockHandle);      

done:
    return status;
}                                                     

/**************************************************************************************************/     
/*                                                                                                */     
/**************************************************************************************************/     
UCHAR
ScsiOpVPD(
          IN pHW_HBA_EXT          pHBAExt,          // Adapter device-object extension from StorPort.
          IN pHW_LU_EXTENSION     pLUExt,           // LUN device-object extension from StorPort.
          IN PSCSI_REQUEST_BLOCK  pSrb
         )
{
    UCHAR                  status;
    ULONG                  len;
    struct _CDB6INQUIRY3 * pVpdInquiry = (struct _CDB6INQUIRY3 *)&pSrb->Cdb;;

    ASSERT(pLUExt != NULL);
    ASSERT(pSrb->DataTransferLength>0);

    if (0==pSrb->DataTransferLength) {
        return SRB_STATUS_DATA_OVERRUN;
      }

    RtlZeroMemory((PUCHAR)pSrb->DataBuffer,           // Clear output buffer.
                  pSrb->DataTransferLength);

    if (VPD_SUPPORTED_PAGES==pVpdInquiry->PageCode) { // Inquiry for supported pages?
      PVPD_SUPPORTED_PAGES_PAGE pSupportedPages;

      len = sizeof(VPD_SUPPORTED_PAGES_PAGE) + 8;

      if (pSrb->DataTransferLength < len) {
        return SRB_STATUS_DATA_OVERRUN;
      }

      pSupportedPages = pSrb->DataBuffer;             // Point to output buffer.

      pSupportedPages->DeviceType = DIRECT_ACCESS_DEVICE;
      pSupportedPages->DeviceTypeQualifier = 0;
      pSupportedPages->PageCode = VPD_SERIAL_NUMBER;
      pSupportedPages->PageLength = 8;                // Enough space for 4 VPD values.
      pSupportedPages->SupportedPageList[0] =         // Show page 0x80 supported.
        VPD_SERIAL_NUMBER;
      pSupportedPages->SupportedPageList[1] =         // Show page 0x83 supported.
        VPD_DEVICE_IDENTIFIERS;

      status = SRB_STATUS_SUCCESS;
    }
    else
    if (VPD_SERIAL_NUMBER==pVpdInquiry->PageCode) {   // Inquiry for serial number?
      PVPD_SERIAL_NUMBER_PAGE pVpd;

      len = sizeof(VPD_SERIAL_NUMBER_PAGE) + 8 + 32;
      if (pSrb->DataTransferLength < len) {
        return SRB_STATUS_DATA_OVERRUN;
      }

      pVpd = pSrb->DataBuffer;                        // Point to output buffer.

      pVpd->DeviceType = DIRECT_ACCESS_DEVICE;
      pVpd->DeviceTypeQualifier = 0;
      pVpd->PageCode = VPD_SERIAL_NUMBER;                
      pVpd->PageLength = 8 + 32;

      status = SRB_STATUS_SUCCESS;
    }
    else
    if (VPD_DEVICE_IDENTIFIERS==pVpdInquiry->PageCode) { // Inquiry for device ids?
        PVPD_IDENTIFICATION_PAGE pVpid;
        PVPD_IDENTIFICATION_DESCRIPTOR pVpidDesc;

        #define VPIDNameSize 32
        #define VPIDName     "PSSLUNxxx"

        len = sizeof(VPD_IDENTIFICATION_PAGE) + sizeof(VPD_IDENTIFICATION_DESCRIPTOR) + VPIDNameSize;

        if (pSrb->DataTransferLength < len) {
          return SRB_STATUS_DATA_OVERRUN;
        }

        pVpid = pSrb->DataBuffer;                     // Point to output buffer.

        pVpid->PageCode = VPD_DEVICE_IDENTIFIERS;

        pVpidDesc =                                   // Point to first (and only) descriptor.
            (PVPD_IDENTIFICATION_DESCRIPTOR)pVpid->Descriptors;

        pVpidDesc->CodeSet = VpdCodeSetAscii;         // Identifier contains ASCII.
        pVpidDesc->IdentifierType =                   // 
            VpdIdentifierTypeFCPHName;

        
        /* Generate a changing serial number. */
        //32 is the length, we require below.
        RtlStringCbPrintfA((char *)pVpidDesc->Identifier, 32, "%03d%02d%03d0123456789abcdefghij\n",
                (int)pHBAExt->pDriverInfo->DrvInfoNbrMPHBAObj, pLUExt->DeviceInfo->TargetId, pLUExt->DeviceInfo->Lun);
        
        pVpidDesc->IdentifierLength =                 // Size of Identifier.
            (UCHAR)strlen((const char *)pVpidDesc->Identifier) - 1;
        pVpid->PageLength =                           // Show length of remainder.
            (UCHAR)(FIELD_OFFSET(VPD_IDENTIFICATION_PAGE, Descriptors) + 
                    FIELD_OFFSET(VPD_IDENTIFICATION_DESCRIPTOR, Identifier) + 
                    pVpidDesc->IdentifierLength);

        status = SRB_STATUS_SUCCESS;
    }
    else {
      status = SRB_STATUS_INVALID_REQUEST;
      len = 0;
    }

    pSrb->DataTransferLength = len;

    return status;
}                                                     

/**************************************************************************************************/     
/* SCSI READ_CAPACITY AND READ_CAPACITY16 implementation.                                         */     
/**************************************************************************************************/     
UCHAR
ScsiOpReadCapacity(
                   IN pHW_HBA_EXT          pHBAExt, // Adapter device-object extension from StorPort.
                   IN pHW_LU_EXTENSION     pLUExt,  // LUN device-object extension from StorPort.
                   IN PSCSI_REQUEST_BLOCK  pSrb
                  )
{
    UNREFERENCED_PARAMETER(pHBAExt);
    NT_ASSERT(pLUExt != NULL);

    UCHAR   Status = SRB_STATUS_SUCCESS;
    PCDB    pCdb = (PCDB)pSrb->Cdb;
    ULONG   blockSize = SECTOR_SIZE;
    ULONG64 highestBlockNumber = pLUExt->MaxBlocks - 1;
    
    // Zero init the buffer
    RtlZeroMemory((PUCHAR)pSrb->DataBuffer, pSrb->DataTransferLength );

    if ((pSrb->CdbLength == 10) && (pCdb->CDB10.OperationCode == SCSIOP_READ_CAPACITY))
    {
        // Handle CDB10 READ_CAPACITY command
        PREAD_CAPACITY_DATA  readCapacity = pSrb->DataBuffer;

        REVERSE_BYTES(&readCapacity->BytesPerBlock, &blockSize);
        
        ULONG maxBlocks = 0;
        if (highestBlockNumber >= MAXULONG)
        {
            // Number of disk blocks have overflown
            // what can be specified in READ_CAPACITY.
            // 
            // Set the block count to be MAXULONG and the caller (FS?)
            // will reinvoke the request with SCSIOP_READ_CAPACITY16. 
            maxBlocks = MAXULONG;
        }
        else
        {
            maxBlocks = (ULONG)highestBlockNumber;
        }

        REVERSE_BYTES(&readCapacity->LogicalBlockAddress, &maxBlocks);
    }
    else if ((pSrb->CdbLength == 16) && (pCdb->CDB16.OperationCode == SCSIOP_READ_CAPACITY16))
    {
        // Handle the CDB16 READ_CAPACITY command
        PREAD_CAPACITY16_DATA  readCapacity = pSrb->DataBuffer;

        REVERSE_BYTES(&readCapacity->BytesPerBlock, &blockSize);

        ULONG64 maxBlocks = highestBlockNumber;
        REVERSE_BYTES_QUAD(&readCapacity->LogicalBlockAddress.QuadPart, &maxBlocks);
    }
    else
    {
        // Unknown READ_CAPACITY
        NT_ASSERT(FALSE);
        Status = SRB_STATUS_ERROR;
    }

    return Status;
}                                                     

// Returns the starting sector from CDB, based upon the CDB Length in SRB.
ULONG64 ScsiGetStartingSectorFromCDB(PCDB pCdb, UCHAR CdbLength)
{
    ULONG64 startingSector = 0;
    
    if (CdbLength == 10)
    {
        REVERSE_BYTES(&startingSector, &pCdb->CDB10.LogicalBlockByte0);
    }
    else if (CdbLength == 16)
    {
        REVERSE_BYTES_QUAD(&startingSector, &pCdb->CDB16.LogicalBlock);
    }
    else
    {
        // Invalid CdbLength!
        NT_ASSERT(FALSE);
    }

    return startingSector;
}

/**************************************************************************************************/     
/*                                                                                                */     
/* This routine does the setup for reading or writing. The reading/writing could be effected      */     
/* here rather than in SFBDGeneralWkRtn, but in the general case SFBDGeneralWkRtn is going to be the  */     
/* place to do the work since it gets control at PASSIVE_LEVEL and so could do real I/O, could    */     
/* wait, etc, etc.                                                                                */     
/*                                                                                                */     
/**************************************************************************************************/     
UCHAR
ScsiReadWriteSetup(
                   IN pHW_HBA_EXT          pHBAExt, // Adapter device-object extension from StorPort.
                   IN pHW_LU_EXTENSION     pLUExt,  // LUN device-object extension from StorPort.        
                   IN PSCSI_REQUEST_BLOCK  pSrb,
                   IN SFBDWkRtnAction      WkRtnAction,
                   IN PUCHAR               pResult
                  )
{
    PHW_SRB_EXTENSION pSrbExt = (PHW_SRB_EXTENSION)pSrb->SrbExtension;
    PCDB                         pCdb = (PCDB)pSrb->Cdb;
    ULONG64                      startingSector = 0, sectorOffset = 0;
    ASSERT(pLUExt != NULL);

    *pResult = ResultDone;                            

    // Validate the bounds of I/O first.
    startingSector = ScsiGetStartingSectorFromCDB(pCdb, pSrb->CdbLength);
    if (startingSector >= pLUExt->MaxBlocks) {
        NT_ASSERT(FALSE);
        return SRB_STATUS_INVALID_REQUEST;
    }

    sectorOffset = startingSector * SECTOR_SIZE;

    pSrbExt->HBAExt = pHBAExt;
    pSrbExt->Action = WkRtnAction;
    pSrbExt->LUExt  = pLUExt;

    pSrbExt->WorkItem = IoAllocateWorkItem((PDEVICE_OBJECT)pHBAExt->pDrvObj);
    if (pSrbExt->WorkItem == NULL)
    {
        EventWriteScsiReadWriteSetupAllocationFailed(NULL, (ActionRead == WkRtnAction) ? L"Read" : L"Write", pSrb->PathId, pSrb->TargetId, pSrb->Lun, sectorOffset, pSrb->DataTransferLength);
        KdPrint(("Failed to allocate thread worker to process SRB of the type %s for Bus: %d, Target: %d, Lun: %d at sector offset %08X for length: %08X", (ActionRead == WkRtnAction) ? "Read" : "Write", pSrb->PathId, pSrb->TargetId, pSrb->Lun, sectorOffset, pSrb->DataTransferLength));
        return SRB_STATUS_ERROR;
    }

    // TODO: Apply Silo Context to the workitem that will run under System process context.
    //
    // Queue work item, which will run in the System process.
    IoQueueWorkItem(pSrbExt->WorkItem, SFBDGeneralWkRtn, DelayedWorkQueue, pSrb);

    *pResult = ResultQueued;                          // Indicate queuing.

    return SRB_STATUS_SUCCESS;
}                                                    

/**************************************************************************************************/     
/* TODO: Look into this.                                                                          */     
/**************************************************************************************************/     
UCHAR
ScsiOpModeSense(
                IN pHW_HBA_EXT          pHBAExt,    // Adapter device-object extension from StorPort.
                IN pHW_LU_EXTENSION     pLUExt,     // LUN device-object extension from StorPort.
                IN PSCSI_REQUEST_BLOCK  pSrb
               )
{
    UNREFERENCED_PARAMETER(pHBAExt);
    UNREFERENCED_PARAMETER(pLUExt);

    RtlZeroMemory((PUCHAR)pSrb->DataBuffer, pSrb->DataTransferLength);

    return SRB_STATUS_SUCCESS;
}

/**************************************************************************************************/     
/* Report available LUNs to the system.                                                           */     
/**************************************************************************************************/     
UCHAR
ScsiOpReportLuns(                                     
                 IN OUT pHW_HBA_EXT         pHBAExt,   // Adapter device-object extension from StorPort.
                 IN       pHW_LU_EXTENSION    pLUExt,    // LUN device-object extension from StorPort.
                 IN       PSCSI_REQUEST_BLOCK pSrb
                )
{
    UCHAR     status = SRB_STATUS_SUCCESS;
    PLUN_LIST pLunList = (PLUN_LIST)pSrb->DataBuffer; // Point to LUN list.
    ULONG     i, 
              GoodLunIdx;

    UNREFERENCED_PARAMETER(pLUExt);

    if (!pHBAExt->bReportAdapterDone) 
    {         
        // This opcode will be one of the earliest I/O requests for a new HBA (and may be received later, too).
        SFBDHwReportAdapter(pHBAExt);                   // WMIEvent test.

        SFBDHwReportLink(pHBAExt);                      // WMIEvent test.

        SFBDHwReportLog(pHBAExt);                       // WMIEvent test.

        pHBAExt->bReportAdapterDone = TRUE;
    }

    // Enable sparse LUN reporting (see http://www.osronline.com/showThread.CFM?link=94776 for details)
    // for the specified Bus/Target ID.
    RtlZeroMemory((PUCHAR)pSrb->DataBuffer, pSrb->DataTransferLength);

    // Compute the number of available LUNs on the Bus/Target
    int iAvailableLuns = 0;
    BOOLEAN fLunEnabled[SCSI_MAXIMUM_LOGICAL_UNITS];

    for (UCHAR iLunIndex = 0; iLunIndex < SCSI_MAXIMUM_LOGICAL_UNITS; iLunIndex++)
    {
        fLunEnabled[iLunIndex] = FALSE;

        // Report the LUN at the specified index only if it is enabled and mounted.
        pSFBD_DEVICE_INFO pDeviceInfo = SFBDGetDevice(pHBAExt, pSrb->PathId, pSrb->TargetId, iLunIndex, FALSE);
        if (pDeviceInfo == NULL)
        {
            fLunEnabled[iLunIndex] = FALSE;
        }
        else
        {
            if ((SFBDIsDeviceEnabled(pDeviceInfo) == TRUE) && pDeviceInfo->Mounted)
            {
                fLunEnabled[iLunIndex] = TRUE;    
            }
        } 

        if (fLunEnabled[iLunIndex])
        {
            iAvailableLuns++;
        }
    }

    // Set the size corresponding to the number of entries we will have in
    // the LUN List.
    int sizeofLunListEntry = sizeof(pLunList->Lun[0]);
    PULONG pLunListLength = (PULONG)(&pLunList->LunListLength);
    
    // Write the length in reverse byte order.
    *pLunListLength = _byteswap_ulong(sizeofLunListEntry * iAvailableLuns);

    if (iAvailableLuns > 0)
    {
        if (pSrb->DataTransferLength >= (ULONG)((FIELD_OFFSET(LUN_LIST, Lun) + (sizeofLunListEntry*iAvailableLuns))))
        {
            for (i = 0, GoodLunIdx = 0; i < SCSI_MAXIMUM_LOGICAL_UNITS; i++)
            {
                // Report the LUN at the specified index only if it is enabled.
                if (fLunEnabled[i])
                {
                    pLunList->Lun[GoodLunIdx][1] = (UCHAR)i;
                    GoodLunIdx++;
                }
            }
        }
        else
        {
            status = SRB_STATUS_DATA_OVERRUN;
        }
    }

    return status;
}                                                     

