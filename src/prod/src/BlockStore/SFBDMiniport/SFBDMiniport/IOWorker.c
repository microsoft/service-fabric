// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

/*++

Module Name:

IOWorker.c

Abstract:

This file includes IO worker routine implementation for Service Fabric BlockStore Device's interaction with the BlockStore Service.

Environment:

Kernel mode only.

--*/

#define WkRtnVer     "1.013"

#include "inc\SFBDVMiniport.h"
#include "inc\BlockStore.h"
#include "Microsoft-ServiceFabric-BlockStoreMiniportEvents.h"

/**************************************************************************************************/ 
/*                                                                                                */ 
/* This is the work routine, which always runs in System under an OS-supplied worker thread.      */ 
/*                                                                                                */ 
/**************************************************************************************************/ 
VOID                                                                                                                                               
SFBDGeneralWkRtn(
               _In_ PDEVICE_OBJECT DeviceObject,   // Not used.
               _In_opt_ PVOID Context              // Parm list pointer.
              )
{
    PSCSI_REQUEST_BLOCK pSrb = (PSCSI_REQUEST_BLOCK)Context;
    if (pSrb)
    {
        PHW_SRB_EXTENSION pSrbExt = (PHW_SRB_EXTENSION)pSrb->SrbExtension;

        UNREFERENCED_PARAMETER(DeviceObject);

        // Free queue item.
        IoFreeWorkItem(pSrbExt->WorkItem);
        pSrbExt->WorkItem = NULL;

        // Do the actual work.
        SFBDWorker(pSrb);
    }
    else
    {
        EventWriteUnexpectedContextinSFBDGeneralWkRtn(NULL);
        HandleInvariantFailure();
    }
}                                                     

/**************************************************************************************************/ 
/*                                                                                                */ 
/* This routine does the "read"/"write" work by working with the BlockStore service.              */ 
/*                                                                                                */ 
/**************************************************************************************************/ 
VOID                                                                                                                                               
SFBDWorker(IN PSCSI_REQUEST_BLOCK pSrb)
{
    PHW_SRB_EXTENSION       pSrbExt = (PHW_SRB_EXTENSION)pSrb->SrbExtension;
    PCDB                    pCdb = (PCDB)pSrb->Cdb;
    ULONG64                 startingSector = 0, sectorOffset = 0;
    UCHAR                   status;
    NTSTATUS                ntStatus = STATUS_SUCCESS;

    startingSector = ScsiGetStartingSectorFromCDB(pCdb, pSrb->CdbLength);
    sectorOffset = startingSector * SECTOR_SIZE;

    // Try to get the System Address for the SRB's DataBuffer.
    ULONG lclStatus = StorPortGetSystemAddress(pSrbExt->HBAExt, pSrb, &pSrbExt->SystemAddressDataBuffer);
    if ((STOR_STATUS_SUCCESS != lclStatus) || !pSrbExt->SystemAddressDataBuffer)
    {
        NT_ASSERT(FALSE);

        EventWriteStorPortGetSystemAddressFailed(NULL, (ActionRead == pSrbExt->Action) ? L"Read" : L"Write", pSrb->PathId, pSrb->TargetId, pSrb->Lun, sectorOffset, pSrb->DataTransferLength, lclStatus);
        KdPrint(("Failed to get system buffer address to process SRB of the type %s for Bus: %d, Target: %d, Lun: %d at sector offset %08X for length: %08X due to error code: %08X",
            (ActionRead == pSrbExt->Action) ? "Read" : "Write", pSrb->PathId, pSrb->TargetId, pSrb->Lun, sectorOffset, pSrb->DataTransferLength, lclStatus));

        status = SRB_STATUS_ERROR;
        goto Cleanup;
    }

    // Note:  Obviously there's going to be a problem if pSrb->DataBuffer points to something in user space, since the correct user space
    //        is probably not that of the System process.  Less obviously, in the paging path at least, even an address in kernel space 
    //        proved not valid; that is, not merely not backed by real storage but actually not valid.  The reason for this behavior is
    //        still under investigation.  For now, in all cases observed, it has been found sufficient to get a new kernel-space address 
    //        to use.

    EventWriteProcessingSRB(NULL, (ActionRead == pSrbExt->Action) ? L"Read" : L"Write", pSrb->PathId, pSrb->TargetId, pSrb->Lun, sectorOffset, pSrb->DataTransferLength);
    KdPrint(("Processing SRB of the type %s for Bus: %d, Target: %d, Lun: %d at sector offset %08X for length: %08X",
        (ActionRead == pSrbExt->Action) ? "Read" : "Write", pSrb->PathId, pSrb->TargetId, pSrb->Lun, sectorOffset, pSrb->DataTransferLength));

    ntStatus = ProcessRWRequests(sectorOffset, pSrb);
    if (NT_SUCCESS(ntStatus))
        status = SRB_STATUS_SUCCESS;
    else
        status = SRB_STATUS_ERROR;

Cleanup:
    pSrb->SrbStatus = status;   
    EventWriteProcessingCompletedForSRB(NULL, (ActionRead == pSrbExt->Action) ? L"Read" : L"Write", pSrb->PathId, pSrb->TargetId, pSrb->Lun, sectorOffset, pSrb->DataTransferLength, ntStatus, pSrb->SrbStatus);
    KdPrint(("Processing completed for SRB of the type %s for Bus: %d, Target: %d, Lun: %d at sector offset %08X for length: %08X with NTSTATUS: %08X and SrbStatus: %08X",
        (ActionRead == pSrbExt->Action) ? "Read" : "Write", pSrb->PathId, pSrb->TargetId, pSrb->Lun, sectorOffset, pSrb->DataTransferLength, ntStatus, pSrb->SrbStatus));

    // Tell StorPort this action has been completed.
    StorPortNotification(RequestComplete, pSrbExt->HBAExt, pSrb);     
}                                                    

