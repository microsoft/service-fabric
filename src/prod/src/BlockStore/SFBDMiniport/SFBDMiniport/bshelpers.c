// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

/*++

Module Name:

bshelpers.c

Abstract:

This file contains BlockStore helper functions for Service Fabric BlockStore Device's interaction with the BlockStore Service.

Environment:

Kernel mode only.

--*/

#include "inc\SFBDVMiniport.h"
#include "inc\BlockStore.h"
#include "Microsoft-ServiceFabric-BlockStoreMiniportEvents.h"

#if !defined(USE_NP_TRANSPORT)
// Frees the specified RW_REQUEST instance, including applicable contained buffers
VOID FreeRWRequest(PRW_REQUEST pRWRequest)
{
    if (pRWRequest)
    {
        if (pRWRequest->IsIrpAllocated)
        {
            IoFreeIrp(pRWRequest->pIrp);
        }

        if (pRWRequest->IsSocketBufferAllocated)
        {
            FreeSocketBuffer(&pRWRequest->SocketBuffer);
        }

        if ((*(pRWRequest->ppSocket)) && pRWRequest->CanCloseSocket)
        {
            // We should never be here in the current implementation
            NT_ASSERT(FALSE);
            CloseSocket(*(pRWRequest->ppSocket));
        }

        // Finally, free the request buffer
        ExFreePoolWithTag(pRWRequest, SFBD_TAG_GENERAL);
    }
}

// Creates and initializes a new RW_REQUEST instance, alongwith associating the completion routine with the
// Irp for the instance.
NTSTATUS GetNewRWRequestInstance(PRW_REQUEST *ppOutRWRequest, PIO_COMPLETION_ROUTINE pCompletionRoutine,

#if defined(USE_SRB_MDL)    
    PMDL pSrcMdl
#else // !defined(USE_SRB_MDL)
    ULONG lengthToRW
#endif // defined(USE_SRB_MDL)
)
{
    NTSTATUS status = STATUS_SUCCESS;

    *ppOutRWRequest = NULL;
    PRW_REQUEST pRWRequest = ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(RW_REQUEST), SFBD_TAG_GENERAL);
    if (!pRWRequest)
    {
        // Insufficient memory for us to work with.
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    RtlZeroMemory(pRWRequest, sizeof(RW_REQUEST));

    // Allocate Buffer for this payload
    ULONG sizeSocketBuffer = GET_PAYLOAD_HEADER_SIZE;

#if !defined(USE_SRB_MDL)
    // If we are not using SRB's MDL for R/W to buffer, then we will allocate the memory for it
    // in the socket buffer.
    sizeSocketBuffer += lengthToRW;
#else // defined(USE_SRB_MDL)
    // Save reference to SRB's MDL
    pRWRequest->pSrbDataBufferMdl = pSrcMdl;
#endif // !defined(USE_SRB_MDL)

    pRWRequest->IsSocketBufferAllocated = FALSE;
    status = AllocateSocketBuffer(&pRWRequest->SocketBuffer, sizeSocketBuffer);
    if (!NT_SUCCESS(status))
    {
        goto Cleanup;
    }
    else
    {
        pRWRequest->IsSocketBufferAllocated = TRUE;
    }

    // Initialize the Irp that we will use
    pRWRequest->IsIrpAllocated = FALSE;
    status = AllocAndInitIrp(&pRWRequest->pIrp, FALSE, pCompletionRoutine, pRWRequest);
    if (!NT_SUCCESS(status))
    {
        goto Cleanup;
    }
    else
    {
        pRWRequest->IsIrpAllocated = TRUE;
    }

Cleanup:

    if (!NT_SUCCESS(status))
    {
        FreeRWRequest(pRWRequest);
    }
    else
    {
        *ppOutRWRequest = pRWRequest;
    }

    return status;
}
#endif // !defined(USE_NP_TRANSPORT)

NTSTATUS WaitForOutstandingSubrequests(PKEVENT pSyncEvent)
{
    LARGE_INTEGER timeout;
    timeout.QuadPart = RELATIVE_TIMEOUT_IN_SECONDS(TIMEOUT_IN_SECONDS_PROCESS_SRB);
    
    NTSTATUS status = KeWaitForSingleObject(pSyncEvent, Executive, KernelMode, FALSE, &timeout);

    return status;
}

NTSTATUS ProcessRWRequests(
    IN ULONG64 offset,
    IN PSCSI_REQUEST_BLOCK pSrb
)
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG lengthToRW = pSrb->DataTransferLength;
    ULONG64 currentOffset = offset;
    PHW_SRB_EXTENSION pSrbExt = (PHW_SRB_EXTENSION)pSrb->SrbExtension;
    BOOLEAN IsReadRequest = TRUE;
    
    // Init the FQDN to connect to the service
    UNICODE_STRING serviceFQDN;
    RtlInitUnicodeString(&serviceFQDN, SERVER_FQDN);

    if (pSrbExt->Action == ActionWrite)
    {
        IsReadRequest = FALSE;
    }

#if !defined(USE_NP_TRANSPORT)
    
    NTSTATUS statusSubrequests = STATUS_SUCCESS;
    KEVENT eventSyncSubrequests = { 0 };
    LONG countSubrequests = 0;
    PWSK_SOCKET pSocket = NULL;
    
    // Sockets must be initialized
    if (!pSrbExt->HBAExt->InitializedWsk)
    {
        EventWriteSocketNotinitialized(NULL, IsReadRequest ? L"Read" : L"Write", pSrb->PathId, pSrb->TargetId, pSrb->Lun, offset, lengthToRW);
        KdPrint(("Unable to process %s SRB request (Bus: %d Target: %d Lun: %d) for offset %08X and Length: %08X since network is not initialized.", IsReadRequest ? "Read" : "Write", pSrb->PathId, pSrb->TargetId, pSrb->Lun, offset, lengthToRW));
        return STATUS_NETWORK_UNREACHABLE;
    }

    // Fetch the service port to connect to
    pSFBD_DEVICE_INFO pDeviceInfo = SFBDGetDevice(pSrbExt->HBAExt, pSrb->PathId, pSrb->TargetId, pSrb->Lun, FALSE);
    if (!pDeviceInfo)
    {
        EventWriteNULLDeviceInfo(NULL, IsReadRequest ? L"Read" : L"Write", pSrb->PathId, pSrb->TargetId, pSrb->Lun, offset, lengthToRW);
        KdPrint(("Unable to locate DeviceInfo for %s SRB request (Bus: %d Target: %d Lun: %d) for offset %08X and Length: %08X.", IsReadRequest ? "Read" : "Write", pSrb->PathId, pSrb->TargetId, pSrb->Lun, offset, lengthToRW));

        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    // Allocate RW_REQUEST buffer for processing the responses.
    PRW_REQUEST pRWSubrequest = NULL;
    
    // Initialize the event used to synchronize subrequests
    KeInitializeEvent(&eventSyncSubrequests, SynchronizationEvent, FALSE);

    pSocket = ConnectTo(pSrbExt->HBAExt, &serviceFQDN, pDeviceInfo->ServicePort);
    if (pSocket)
    {
        ULONG payloadBufferSizeToAllocate = lengthToRW;

#if defined(USE_DMA_FOR_SRB_IO)
        // If we are using DMA, then no additional payload buffer size needs to be allocated
        // aside from the payload header.
        payloadBufferSizeToAllocate = 0;
#endif // defined(USE_DMA_FOR_SRB_IO)

        // We will send the request asynchronously, so create a new instance of 
        // RWRequest for each subrequest.
        status = GetNewRWRequestInstance(&pRWSubrequest, SendRequestCompletionRoutine, 
#if defined(USE_SRB_MDL)
            pSrbDataBufferMdl
#else // !defined(USE_SRB_MDL)
            payloadBufferSizeToAllocate
#endif // defined(USE_SRB_MDL)
        );

        if (NT_SUCCESS(status))
        {
            // Setup the RWRequest

#if defined(USE_DMA_FOR_SRB_IO)
            pRWSubrequest->pSrb = pSrb;
#endif // defined(USE_DMA_FOR_SRB_IO)

            pRWSubrequest->pSubrequestsActiveCount = &countSubrequests;
            pRWSubrequest->pSubrequestsEventsync = &eventSyncSubrequests;
            pRWSubrequest->pSubrequestsStatus = &statusSubrequests;
            pRWSubrequest->CanCloseSocket = FALSE;

            // Initialize the RWRequest data
            pRWSubrequest->lengthToRW = lengthToRW;
            pRWSubrequest->offset = currentOffset;
            pRWSubrequest->CheckForPayloadConsistency = FALSE;

#if !defined(USE_SRB_MDL) && !defined(USE_DMA_FOR_SRB_IO)
            // R/W buffer is specified only when not using SRB's MDL
            pRWSubrequest->pRWBufferAddress = (PUCHAR)pSrbExt->SystemAddressDataBuffer;
#endif // !defined(USE_SRB_MDL) && !defined(USE_DMA_FOR_SRB_IO)

            pRWSubrequest->IsReadRequest = IsReadRequest;
            pRWSubrequest->pLUID = pSrbExt->LUExt->DeviceInfo->LUID;
            pRWSubrequest->ppSocket = &pSocket;
            pRWSubrequest->baseOffset = offset;

            status = SendBlockRequestToService(pRWSubrequest);

            // Wait for any outstanding requests to complete
            if (InterlockedOr(&countSubrequests, 0) > 0)
            {
                status = WaitForOutstandingSubrequests(&eventSyncSubrequests);
                
                // Assert to help break on timeouts
                // NT_ASSERT(status == STATUS_SUCCESS);
            }

            if (NT_SUCCESS(status))
            {
                // Reflect the overall status of the requests that were sent as the primary status.
                status = statusSubrequests;
            }
            else
            {
                FreeRWRequest(pRWSubrequest);
            }
        }
    }
    else
    {
        status = STATUS_HOST_UNREACHABLE;
    }

    // Close the connection
    if (pSocket)
    {
        CloseSocket(pSocket);

#if defined(DBG)
        // Decrement the active connection count
        SFBDDecrementActiveConnections(pSrbExt->HBAExt);
#endif // defined(DBG)
    }

#else // defined(USE_NP_TRANSPORT)

    HANDLE hServicePipe = NULL;

    status = ConnectTo(&serviceFQDN, &hServicePipe);
    if (NT_SUCCESS(status))
    {
        PVOID pipeBufferForRead = NULL;
        ULONG pipeBufferLength = 0;

        // Form the payload to be sent
        GATEWAY_PAYLOAD payload;
        RtlZeroMemory(&payload, sizeof(payload));

        payload.mode = IsReadRequest ? Read : Write;
        payload.offsetToRW = currentOffset;
        payload.sizePayloadBuffer = lengthToRW;

#if defined(USE_DMA_FOR_SRB_IO)

        // If we are using DMA, then setup the reference to the SRB.
        payload.addressSRB = (ULONG64)pSrb;
        payload.payloadData = NULL;

#else // !defined(USE_DMA_FOR_SRB_IO)

        PVOID pRWBufferBaseAddress = pSrbExt->SystemAddressDataBuffer;

        if (payload.mode == Write)
        {
            // Write request contains the header + payload

            // Set the buffer from which to send the data
            payload.payloadData = pRWBufferBaseAddress;
        }
        else
        {
            // Read request only contains the header, so nothing to do here.
            //
            // However, we will allocate the buffer into which the data will be read into
            // before issuing the read request (similar to what happens for Sockets approach).
            // 
            // We saw a BSOD when using the SRB provided buffer directly. It appears that happens
            // because the MDL for the SRB is marked for Read and passing the corresponding virtual address
            // to ZwReadFile results in MM locking the pages so that they can be written to ( in order to write the data
            // being read). Thus, there is a bug check.
            //
            // For now, explicit allocation should help.
            pipeBufferForRead = ExAllocatePoolWithTag(NonPagedPoolNx, payload.sizePayloadBuffer, USER_BUFFER_POOL_TAG_REQUEST);
            if (!pipeBufferForRead)
            {
                EventWritePipeReadBufferAllocationFailed(NULL, IsReadRequest ? L"Read" : L"Write", pSrb->PathId, pSrb->TargetId, pSrb->Lun, offset, lengthToRW);
                KdPrint(("Unable to allocate ReadBuffer for %s SRB request (Bus: %d Target: %d Lun: %d) for offset %08X and Length: %08X", IsReadRequest ? "Read" : "Write", pSrb->PathId, pSrb->TargetId, pSrb->Lun, offset, lengthToRW));
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
            else
            {
                pipeBufferLength = payload.sizePayloadBuffer;
            }
        }

#endif // defined(USE_DMA_FOR_SRB_IO)

        if (NT_SUCCESS(status))
        {
            payload.deviceID = pSrbExt->LUExt->DeviceInfo->LUID;
            payload.lengthDeviceID = (ULONG)(sizeof(wchar_t) * (wcslen(payload.deviceID) + 1));

            // Send the request
            status = SendBuffer(hServicePipe, &payload);
            if (NT_SUCCESS(status))
            {
                // Receive the response
                RtlZeroMemory(&payload, sizeof(payload));
                payload.mode = IsReadRequest ? Read : Write;

                if (IsReadRequest)
                {

#if !defined(USE_DMA_FOR_SRB_IO)
                    // Response of Read request will carry the payload buffer.
                    // Setup the buffer to copy the payload to.
                    payload.payloadData = pRWBufferBaseAddress;
#else // defined(USE_DMA_FOR_SRB_IO)
                    payload.payloadData = NULL;
#endif // !defined(USE_DMA_FOR_SRB_IO)

                }

                status = ReceiveBufferEx(hServicePipe, &payload, pipeBufferForRead, pipeBufferLength);
                if (NT_SUCCESS(status))
                {
                    status = GetNTSTATUSForResponsePayload(IsReadRequest, &payload);
                }
            }

            // Free the pipeBuffer if it was allocated.
            if (pipeBufferForRead)
            {
                ExFreePoolWithTag(pipeBufferForRead, USER_BUFFER_POOL_TAG_REQUEST);
            }
        }

        // Close the connection
        CloseConnection(hServicePipe);
    }

#endif // !defined(USE_NP_TRANSPORT)

    return status;
}

// Manages the registration of the specified LUID with the BlockStore service
NTSTATUS ProcessManagementRequestonInternal(
    IN BlockMode registrationMode,
    IN pHW_HBA_EXT pHBAExt,
    IN PWCHAR pLUUniqueID,
    IN ULONG  DiskSize,
    IN UCHAR  DiskSizeUnit,
    IN ULONG  Mounted,
    IN PVOID  pLURegistrationListBuffer,
    IN ULONG  servicePort
)
{
    NTSTATUS status = STATUS_SUCCESS;
    
    UNICODE_STRING serviceFQDN;
    RtlInitUnicodeString(&serviceFQDN, SERVER_FQDN);

    // Initialize the payload to be sent to the service
    GATEWAY_PAYLOAD payload;
    RtlZeroMemory(&payload, sizeof(payload));

#if !defined(USE_NP_TRANSPORT)
    PIRP Irp = NULL;
    KEVENT SyncEvent = { 0 };

    PWSK_SOCKET pSocket = NULL;
    USER_BUFFER SocketBuffer;
    PUSER_BUFFER pSocketBuffer = &SocketBuffer;
    
    // Sockets must be initialized
    if (!pHBAExt->InitializedWsk)
    {
        return STATUS_NETWORK_UNREACHABLE;
    }

    // Allocate a socket buffer of size that works for all request types.
    status = AllocateSocketBuffer(&SocketBuffer, GET_PAYLOAD_HEADER_SIZE + BLOCK_SIZE_MANAGEMENT_REQUEST);
    if (!NT_SUCCESS(status))
        return status;

    // Initialize the Irp that we will use
    status = AllocAndInitSyncIrp(&Irp, &SyncEvent, FALSE);
    if (!NT_SUCCESS(status))
    {
        goto Cleanup;
    }

    
    pSocket = ConnectTo(pHBAExt, &serviceFQDN, servicePort);
    if (pSocket)
#else // defined(USE_NP_TRANSPORT)
    UNREFERENCED_PARAMETER(pHBAExt);

    // Allocate a buffer of size that works for all request types and will be used to 
    // carry the payload across the pipe.
    ULONG bufferPipeLength = BLOCK_SIZE_MANAGEMENT_REQUEST;
    PVOID bufferPipe = ExAllocatePoolWithTag(NonPagedPoolNx, bufferPipeLength, USER_BUFFER_POOL_TAG);
    if (!bufferPipe)
        return STATUS_INSUFFICIENT_RESOURCES;

    HANDLE hServicePipe = NULL;
    status = ConnectTo(&serviceFQDN, &hServicePipe);
    if (NT_SUCCESS(status))
#endif // !defined(USE_NP_TRANSPORT)
    {
        // Copy the registration information over
        LU_REGISTRATION_INFO registrationInfo;
        ULONG lengthToWrite = sizeof(registrationInfo);

        RtlZeroMemory(&registrationInfo, lengthToWrite);

        // Set the LUID only for applicable LU operations
        if (IS_LU_OPERATION(registrationMode))
        {
            // Set the correct length of the LUID, including the terminating NULL, in bytes
            // as that is copied over by wcscpy_s as well.
            registrationInfo.lenLUID = (ULONG)(sizeof(wchar_t) * (wcslen(pLUUniqueID)+1));
            wcscpy_s(registrationInfo.wszLUID, _countof(registrationInfo.wszLUID), pLUUniqueID);
        }

        // Disk size and Size unit are only required for registration.
        if (registrationMode == RegisterLU)
        {
            registrationInfo.DiskSize = DiskSize;
            registrationInfo.DiskSizeUnit = DiskSizeUnit;
        }

        registrationInfo.Mounted = Mounted;
            
        // Set the payload mode to indicate registration mode
        payload.mode = registrationMode;
        payload.offsetToRW = 0;

        // Set the size of the payload buffer (based upon the SocketBuffer/bufferPipe size allocated earlier).
        payload.sizePayloadBuffer = BLOCK_SIZE_MANAGEMENT_REQUEST;

        payload.deviceID = LU_REGISTER_DEVICE_ID;
        payload.lengthDeviceID = (ULONG)(sizeof(WCHAR) * (wcslen(payload.deviceID) + 1));

        if (IS_LU_OPERATION(registrationMode))
        {
            // Setup the data we wish to register/unregister
            payload.payloadData = &registrationInfo;

            // Since the payload buffer does not contain as much data as
            // sizePayloadBuffer indicates, specify the actual size.
            // This will be used in copying the correct number of bytes to the 
            // outgoing SocketBuffer.
            payload.sizeActualPayload = lengthToWrite;
        }
#if defined(USE_NP_TRANSPORT)
        else
        {
            // Since LU registration list fetch expects a BLOCK_SIZE_MANAGEMENT_REQUEST buffer,
            // set that up in the payload.
            payload.payloadData = bufferPipe;
        }
#endif // defined(USE_NP_TRANSPORT)

        // Send the request to the service
#if !defined(USE_NP_TRANSPORT)
        status = SendBuffer(pSocket, pSocketBuffer, Irp, &SyncEvent, &payload);
#else // defined(USE_NP_TRANSPORT)
        status = SendBufferEx(hServicePipe, &payload, bufferPipe, bufferPipeLength);
#endif // !defined(USE_NP_TRANSPORT)
        if (NT_SUCCESS(status))
        {
            // We sent the request successfully to the server, so now prepare to receive the response.
            RtlZeroMemory(&payload, sizeof(payload));

            if (registrationMode == FetchRegisteredLUList)
            {
                // Setup the buffer to receive LU registration list
                payload.payloadData = pLURegistrationListBuffer;
            }

#if !defined(USE_NP_TRANSPORT)
            status = ReceiveBuffer(pSocket, pSocketBuffer, Irp, &SyncEvent, &payload);
#else // defined(USE_NP_TRANSPORT)
            status = ReceiveBufferEx(hServicePipe, &payload, bufferPipe, bufferPipeLength);
#endif // !defined(USE_NP_TRANSPORT)
            if (NT_SUCCESS(status))
            {
                if (payload.mode == BLOCK_OPERATION_INVALID_PAYLOAD)
                {
                    // We got an invalid payload. Treat that
                    // as disk operation failure.
                    status = NTSTATUS_FROM_WIN32(ERROR_WRITE_FAULT);
                    NT_ASSERT(FALSE);
                }
                else if (payload.mode == BLOCK_OPERATION_INVALID_PAYLOAD_SERVER)
                {
                    // Server got an invalid payload. Treat that
                    // as disk operation failure.
                    status = NTSTATUS_FROM_WIN32(ERROR_WRITE_FAULT);
                    NT_ASSERT(FALSE);
                }
                else if (payload.mode == BLOCK_OPERATION_FAILED)
                {
                    // We had an exception processing the block in the service. Treat that
                    // as registration failure.
                    status = NTSTATUS_FROM_WIN32(ERROR_WRITE_FAULT);
                }
            }
        }
    }
#if !defined(USE_NP_TRANSPORT)
    else
    {
        status = STATUS_HOST_UNREACHABLE;
    }

Cleanup:

    if (pSocket != NULL)
    {
        CloseSocket(pSocket);

#if defined(DBG)
        // Decrement the active connection count
        SFBDDecrementActiveConnections(pHBAExt);
#endif // defined(DBG)
    }

    if (Irp != NULL)
    {
        IoFreeIrp(Irp);
    }

    FreeSocketBuffer(&SocketBuffer);
#else // defined(USE_NP_TRANSPORT)

    // Close the connection
    CloseConnection(hServicePipe);

    // Free the buffer used for pipe IO.
    if (bufferPipe != NULL)
    {
        ExFreePoolWithTag(bufferPipe, USER_BUFFER_POOL_TAG);
    }

#endif // !defined(USE_NP_TRANSPORT)

    return status;
}

#ifdef CONNECT_RS3_WORKAROUND

typedef struct
{
    BlockMode registrationMode;
    pHW_HBA_EXT pHBAExt;
    PWCHAR pLUUniqueID;
    ULONG  DiskSize;
    UCHAR  DiskSizeUnit;
    ULONG  Mounted;
    PVOID  pLURegistrationListBuffer;
    ULONG  servicePort;
    NTSTATUS ReturnStatus;
    KEVENT syncWithWorkerThread;
}THREAD_DATA, pTHREAD_DATA;

VOID
SFBDWorkerfunction(
    _In_ PDEVICE_OBJECT DeviceObject,  // Not used.
    _In_opt_ PVOID Context             // Pointer to THREAD_DATA.
)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    THREAD_DATA* threadData = (THREAD_DATA*)Context;

    if (threadData)
    {
        // Do the actual work.
        threadData->ReturnStatus = ProcessManagementRequestonInternal(threadData->registrationMode,
            threadData->pHBAExt,
            threadData->pLUUniqueID,
            threadData->DiskSize,
            threadData->DiskSizeUnit,
            threadData->Mounted,
            threadData->pLURegistrationListBuffer,
            threadData->servicePort);

        KeSetEvent(&threadData->syncWithWorkerThread, IO_NO_INCREMENT, FALSE);
    }
    else
    {
        EventWriteUnexpectedContextinSFBDWorkerfunction(NULL);
        HandleInvariantFailure();
    }
}
#endif
// Manages the registration of the specified LUID with the BlockStore service
NTSTATUS ProcessManagementRequest(
    IN BlockMode registrationMode,
    IN pHW_HBA_EXT pHBAExt,
    IN PWCHAR pLUUniqueID,
    IN ULONG  DiskSize,
    IN UCHAR  DiskSizeUnit,
    IN ULONG  Mounted,
    IN PVOID  pLURegistrationListBuffer,
    IN ULONG  servicePort
)
{
#ifdef CONNECT_RS3_WORKAROUND 
    //Connect started crashing in RS3 builds, switching on a system thread fixes this issue.
    THREAD_DATA threadData = { 0 };
    threadData.registrationMode = registrationMode;
    threadData.pHBAExt = pHBAExt;
    threadData.pLUUniqueID = pLUUniqueID;
    threadData.DiskSize = DiskSize;
    threadData.DiskSizeUnit = DiskSizeUnit;
    threadData.Mounted = Mounted;
    threadData.pLURegistrationListBuffer = pLURegistrationListBuffer;
    threadData.servicePort = servicePort;
    threadData.ReturnStatus = STATUS_SUCCESS;
    KeInitializeEvent(&threadData.syncWithWorkerThread, SynchronizationEvent, FALSE);
    
    PIO_WORKITEM  WorkItem = IoAllocateWorkItem((PDEVICE_OBJECT)pHBAExt->pDrvObj);
    if (WorkItem)
    {
        IoQueueWorkItem(WorkItem, SFBDWorkerfunction, DelayedWorkQueue, &threadData);
        KeWaitForSingleObject(&threadData.syncWithWorkerThread, Executive, KernelMode, FALSE, NULL);        
        // Free queue item.
        IoFreeWorkItem(WorkItem);
    }   
    else
    {
        threadData.ReturnStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    return threadData.ReturnStatus;

#else
    return ProcessManagementRequestonInternal(registrationMode,
        pHBAExt,
        pLUUniqueID,
        DiskSize,
        DiskSizeUnit,
        Mounted,
        pLURegistrationListBuffer,
        servicePort);

#endif // CONNECT_RS3_WORKAROUND
}
