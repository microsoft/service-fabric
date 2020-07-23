// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

/*++

Module Name:

wskhelpers.c

Abstract:

This file includes WinSock Kernel helper functions for Service Fabric BlockStore Device's interaction with the BlockStore Service.

Environment:

Kernel mode only.

--*/

#include "inc\SFBDVMiniport.h"
#include "inc\BlockStore.h"
#include "Microsoft-ServiceFabric-BlockStoreMiniportEvents.h"

#if !defined(USE_NP_TRANSPORT)
// Helper to initialize sockets

NTSTATUS CacheSecurityContext(
    IN pHW_HBA_EXT pHBAExt
)
{
    NTSTATUS  Status;
    SECURITY_SUBJECT_CONTEXT SubjectContext;
    PSECURITY_DESCRIPTOR newSD = NULL;

    SeCaptureSubjectContext(&SubjectContext);

    Status = SeAssignSecurity(NULL,
        NULL,
        &newSD,
        FALSE,
        &SubjectContext,
        IoGetFileObjectGenericMapping(),
        PagedPool);

    SeReleaseSubjectContext(&SubjectContext);

    if (NT_SUCCESS(Status))
    {
        Status = pHBAExt->WskProviderNpi.Dispatch->WskControlClient(pHBAExt->WskProviderNpi.Client,
            WSK_CACHE_SD,
            sizeof(PSECURITY_DESCRIPTOR),
            &newSD,
            sizeof(pHBAExt->WskCachedSecurityDescriptor),
            &pHBAExt->WskCachedSecurityDescriptor,
            NULL,
            NULL);

        SeDeassignSecurity(&newSD);
    }

    if (!NT_SUCCESS(Status))
    {
        pHBAExt->WskCachedSecurityDescriptor = NULL;
    }
    return Status;
}

NTSTATUS ResolveNameToIp(
    IN pHW_HBA_EXT pHBAExt
)
{
    NTSTATUS  Status;
#if defined(USE_BLOCKSTORE_SERVICE_ON_NODE)
    //Service is running on the local host, resolving name is time cosuming process as network stack and name server has to be done configuring, so
    //decided to hardcode the local hostip address 127.00.00.01.
    pHBAExt->ResolvedServerAddress.sin_family = AF_INET;
    pHBAExt->ResolvedServerAddress.sin_port = 0;
    pHBAExt->ResolvedServerAddress.sin_addr.S_un.S_un_b.s_b1 = 0x7f;
    pHBAExt->ResolvedServerAddress.sin_addr.S_un.S_un_b.s_b2 = 0x00;
    pHBAExt->ResolvedServerAddress.sin_addr.S_un.S_un_b.s_b3 = 0x00;
    pHBAExt->ResolvedServerAddress.sin_addr.S_un.S_un_b.s_b4 = 0x01;
    Status = STATUS_SUCCESS;
#else // defined(USE_BLOCKSTORE_SERVICE_ON_NODE)
    UNICODE_STRING serviceFQDN;
    RtlInitUnicodeString(&serviceFQDN, SERVER_FQDN);
    // With sockets initialized, resolve the FQDN here since this function is invoked at Passive Level.
    // and resolution APIs of WinSock Kernel expect to be at Passive level.
    Status = ResolveFQDNToIpV4Address(pHBAExt, &serviceFQDN, (PSOCKADDR)&pHBAExt->ResolvedServerAddress);
    if (!NT_SUCCESS(Status))
    {
        pHBAExt->InitializedWsk = FALSE;
        EventWriteResolveFQDNToIpV4AddressFailed(NULL, Status);
        KdPrint(("ResolveFQDNToIpV4Address failed"));
    }
#endif // !defined(USE_BLOCKSTORE_SERVICE_ON_NODE)
    return Status;
}


VOID
InitWSKOnThread(
    _In_ PVOID StartContext
)
{
    pHW_HBA_EXT pHBAExt = StartContext;
    NTSTATUS    Status;
    Status = WskCaptureProviderNPI(&pHBAExt->WskRegistration, WSK_INFINITE_WAIT, &pHBAExt->WskProviderNpi);
    if (NT_SUCCESS(Status))
    {      
        pHBAExt->InitializedWsk = TRUE;
        Status = CacheSecurityContext(pHBAExt);
        if (NT_SUCCESS(Status))
        {
            Status = ResolveNameToIp(pHBAExt);
        }
        else
        {
            EventWriteCacheSecurityContextFailed(NULL, Status);
        }
    }
    else
    { 
        EventWriteWskCaptureProviderNPIFalied(NULL, Status);
    }
}

NTSTATUS InitSockets(
    IN pHW_HBA_EXT pHBAExt
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    WSK_CLIENT_NPI wskClientNpi;

    if (pHBAExt->InitializedWsk)
    {
        // Already initialized
        return Status;
    }

    // WSK Client Dispatch table that denotes the WSK version
    // that we want to use and optionally a pointer
    // to the WskClientEvent callback function
    pHBAExt->WskAppDispatch.Version = MAKE_WSK_VERSION(1, 0);
    pHBAExt->WskAppDispatch.Reserved = 0;
    pHBAExt->WskAppDispatch.WskClientEvent = NULL;

    // Register the WSK application
    wskClientNpi.ClientContext = NULL;
    wskClientNpi.Dispatch = &pHBAExt->WskAppDispatch;
    Status = WskRegister(&wskClientNpi, &pHBAExt->WskRegistration);
    if (!NT_SUCCESS(Status))
    {
        EventWriteWskRegisterFailed(NULL, Status);
        KdPrint(("Service Fabric BlockDevice - WskRegister failed with %x.\n", Status));
    }
    else
    {
        Status = WskCaptureProviderNPI(&pHBAExt->WskRegistration, WSK_NO_WAIT, &pHBAExt->WskProviderNpi);
      
        if (!NT_SUCCESS(Status)) 
        {   //if failed, we will init WSK on a thread.

            EventWriteCaptureProviderNPIFaliedTryingOnThread(NULL);
            HANDLE threadHandle;
            Status = PsCreateSystemThread(&threadHandle,
                (ACCESS_MASK)0,
                NULL,
                (HANDLE)0,
                NULL,
                InitWSKOnThread,
                pHBAExt);

            if (!NT_SUCCESS(Status))
            {
                EventWritePsCreateSystemThreadFailed(NULL, Status);
                KdPrint(("PsCreateSystemThread failed with %x.\n", Status));
                WskDeregister(&pHBAExt->WskRegistration);
                return Status;
            }

            //Convert the Thread object handle into a pointer to the Thread object itself. Then close the handle.
            ObReferenceObjectByHandle(threadHandle,
                THREAD_ALL_ACCESS,
                NULL,
                KernelMode,
                &pHBAExt->ThreadObject,
                NULL);

            ZwClose(threadHandle);           
        }
        else
        {
            pHBAExt->InitializedWsk = TRUE;
        }
    }

    return Status;
}

VOID ShutdownSockets(
    IN pHW_HBA_EXT pHBAExt
)
{
    if (pHBAExt->ThreadObject)
    {// Wait for the thread to terminate
        KeWaitForSingleObject(pHBAExt->ThreadObject,
            Executive,
            KernelMode,
            FALSE,
            NULL);

        ObDereferenceObject(pHBAExt->ThreadObject);
        pHBAExt->ThreadObject = NULL;
    }

    if (pHBAExt->InitializedWsk)
    {
#if defined(DBG)
        // Set the flag indicating we are shutting down
        InterlockedOr((volatile LONG*)&pHBAExt->IsShutdownInProgress, 1);

        if (InterlockedOr64((volatile LONG64 *)&pHBAExt->ActiveConnections, 0) > 0)
        {
            // There are active connections, so wait for them to close.
            KeWaitForSingleObject(&pHBAExt->ShutdownSyncEvent, Executive, KernelMode, FALSE, NULL);
        }
#endif // defined(DBG)

        if (pHBAExt->WskCachedSecurityDescriptor)
        {
            pHBAExt->WskProviderNpi.Dispatch->WskControlClient(pHBAExt->WskProviderNpi.Client,
            WSK_RELEASE_SD,
            sizeof(pHBAExt->WskCachedSecurityDescriptor),
            &pHBAExt->WskCachedSecurityDescriptor,
            0,
            NULL,
            NULL,
            NULL);
        }

        // Cleanup the WSK NPI resources used by the driver.
        WskReleaseProviderNPI(&pHBAExt->WskRegistration);
        WskDeregister(&pHBAExt->WskRegistration);
    }

    pHBAExt->InitializedWsk = FALSE;
}

NTSTATUS AllocateSocketBuffer(
    IN OUT PUSER_BUFFER UserBuffer,
    IN ULONG Size
)
{
    NTSTATUS Status = STATUS_NO_MEMORY;

    if (!UserBuffer)
        return STATUS_INVALID_PARAMETER_1;

    UserBuffer->payloadNPBuffer = ExAllocatePoolWithTag(NonPagedPoolNx, Size, USER_BUFFER_POOL_TAG);
    if (UserBuffer->payloadNPBuffer) {
        UserBuffer->SocketBuffer.Mdl = IoAllocateMdl(UserBuffer->payloadNPBuffer, Size, FALSE, FALSE, NULL);
        if (!UserBuffer->SocketBuffer.Mdl) {
            // Free the buffer since Mdl could not be allocated
            ExFreePool(UserBuffer->payloadNPBuffer);
        }
        else {
            MmBuildMdlForNonPagedPool(UserBuffer->SocketBuffer.Mdl);
            RtlZeroMemory(UserBuffer->payloadNPBuffer, Size);

            // Set the parameters for the Socket buffer - how much to send/receive, size of 
            // non-paged pool buffer and offset to send/receive data from.
            UserBuffer->SocketBuffer.Length = Size;
            UserBuffer->payloadNPBufferSize = Size;
            UserBuffer->SocketBuffer.Offset = 0;

            Status = STATUS_SUCCESS;
        }
    }

    return Status;
}

NTSTATUS FreeSocketBuffer(
    IN OUT PUSER_BUFFER UserBuffer
)
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER_1;

    // Proceed to cleanup the buffer only if it looks valid
    if (!(UserBuffer && UserBuffer->payloadNPBuffer && UserBuffer->SocketBuffer.Mdl))
        return Status;

    // Free the memory and Mdl
    ExFreePoolWithTag(UserBuffer->payloadNPBuffer, SFBD_TAG_GENERAL);
    IoFreeMdl(UserBuffer->SocketBuffer.Mdl);

    return STATUS_SUCCESS;

}

VOID ResetSocketBuffer(
    IN PUSER_BUFFER UserBuffer
)
{
    // Zero out the buffer
    RtlZeroMemory(UserBuffer->payloadNPBuffer, UserBuffer->payloadNPBufferSize);

    // Set the parameters for the Socket buffer.
    UserBuffer->SocketBuffer.Offset = 0;
}

BOOLEAN IsEndOfPayloadMarkerPresent(
    IN PUSER_BUFFER UserBuffer
)
{
    // Set the end of payload marker
    PUCHAR pMarkerReadFromLocation = (PUCHAR)(UserBuffer->payloadNPBuffer) + (UserBuffer->SocketBuffer.Length - SIZEOF_END_OF_PAYLOAD_MARKER);
    ULONG endOfPayloadMarker = 0;
    RtlCopyMemory(&endOfPayloadMarker, pMarkerReadFromLocation, SIZEOF_END_OF_PAYLOAD_MARKER);

    return (endOfPayloadMarker == END_OF_PAYLOAD_MARKER) ? TRUE : FALSE;
}

VOID SetEndOfPayloadMarker(
    IN PUSER_BUFFER UserBuffer
)
{
    // Set the end of payload marker
    PUCHAR pMarkerWriteToLocation = (PUCHAR)(UserBuffer->payloadNPBuffer) + (UserBuffer->SocketBuffer.Length - SIZEOF_END_OF_PAYLOAD_MARKER);
    ULONG endOfPayloadMarker = END_OF_PAYLOAD_MARKER;
    RtlCopyMemory(pMarkerWriteToLocation, &endOfPayloadMarker, SIZEOF_END_OF_PAYLOAD_MARKER);
}

NTSTATUS WorkCompletionRoutine(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Context
)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);

    PKEVENT syncEvent = (PKEVENT)(Context);

    if (syncEvent)
    {
        KeSetEvent(syncEvent, IO_NO_INCREMENT, FALSE);
    }
    else
    {
        EventWriteUnexpectedContextinWorkCompletionRoutine(NULL);
        HandleInvariantFailure();
    }

    return STATUS_MORE_PROCESSING_REQUIRED;
}

PWSK_SOCKET ConnectTo(
    IN pHW_HBA_EXT pHBAExt,
    IN PUNICODE_STRING server,
    IN ULONG servicePort
)
{
    UNREFERENCED_PARAMETER(server);

    PWSK_SOCKET retVal = NULL;
    PIRP Irp = NULL;
    KEVENT SyncEvent = { 0 };
    SOCKADDR_IN local, target;
   
    if (!pHBAExt->InitializedWsk)
    {
        return NULL;
    }

    RtlZeroMemory(&local, sizeof(local));
    RtlZeroMemory(&target, sizeof(target));

    // Setup for localaddress to be bound to.
    local.sin_family = AF_INET;
    local.sin_port = RtlUshortByteSwap(0);
    local.sin_addr.S_un.S_addr = RtlUlongByteSwap(INADDR_ANY);

    target.sin_family = AF_INET;
    target.sin_port = RtlUshortByteSwap(servicePort);

    target.sin_addr = pHBAExt->ResolvedServerAddress.sin_addr;

    // Initialize the Irp that we will use
    NTSTATUS Status = AllocAndInitSyncIrp(&Irp, &SyncEvent, FALSE);
    if (!NT_SUCCESS(Status))
        return NULL;

#if defined(DBG)
    // Increment the active connection count
    InterlockedIncrement64((volatile LONG64*)&pHBAExt->ActiveConnections);

    // If we are in the process of shutdown, fail the connection.
    if (InterlockedOr((volatile LONG*)&pHBAExt->IsShutdownInProgress, 0) == 0)
#endif // defined(DBG)
    {
        // Attempt the connection
        Status = pHBAExt->WskProviderNpi.Dispatch->WskSocketConnect(
            pHBAExt->WskProviderNpi.Client,
            SOCK_STREAM,
            IPPROTO_TCP,
            (PSOCKADDR)&local,
            (PSOCKADDR)&target,
            0,
            NULL,
            NULL,
            NULL,
            NULL,
            pHBAExt->WskCachedSecurityDescriptor,
            Irp);

        if (Status == STATUS_PENDING) {
            
            // Wait for the WSK subsystem to complete the work
            KeWaitForSingleObject(&SyncEvent, Executive, KernelMode, FALSE, NULL);
            Status = Irp->IoStatus.Status;
        }

        if (NT_SUCCESS(Status)) {

            // Return the created socket
            retVal = (PWSK_SOCKET)Irp->IoStatus.Information;
        }
    }

#if defined(DBG)
    if (retVal == NULL)
    {
        // Decrement the active connection count
        SFBDDecrementActiveConnections(pHBAExt);
    }
#endif // defined(DBG)

    // Free the Irp since we are not going to use it
    IoFreeIrp(Irp);

    return retVal;
}

NTSTATUS DisconnectSocket(
    IN PWSK_SOCKET socket
)
{
    PIRP Irp = NULL;
    KEVENT SyncEvent = { 0 };

    // Initialize the Irp that we will use
    NTSTATUS Status = AllocAndInitSyncIrp(&Irp, &SyncEvent, FALSE);
    if (!NT_SUCCESS(Status))
        return Status;

    // Disconnect the Connection.
    Status = ((PWSK_PROVIDER_CONNECTION_DISPATCH)(socket->Dispatch))->WskDisconnect(
        socket,
        NULL,
        0,
        Irp);

    if (Status == STATUS_PENDING) {
        // Wait for the WSK subsystem to complete the work
        KeWaitForSingleObject(&SyncEvent, Executive, KernelMode, FALSE, NULL);
        Status = Irp->IoStatus.Status;
    }

    // Free the Irp since we are not going to use it
    IoFreeIrp(Irp);

    return Status;
}

NTSTATUS CloseSocket(
    IN PWSK_SOCKET socket
)
{
    PIRP Irp = NULL;
    KEVENT SyncEvent = { 0 };

    // Gracefully disconnect socket first
    // DisconnectSocket(socket);

    // Initialize the Irp that we will use
    NTSTATUS Status = AllocAndInitSyncIrp(&Irp, &SyncEvent, FALSE);
    if (!NT_SUCCESS(Status))
        return Status;

    // Close the Connection.
    Status = ((PWSK_PROVIDER_BASIC_DISPATCH)(socket->Dispatch))->WskCloseSocket(
        socket,
        Irp);

    if (Status == STATUS_PENDING) {
        // Wait for the WSK subsystem to complete the work
        KeWaitForSingleObject(&SyncEvent, Executive, KernelMode, FALSE, NULL);
        Status = Irp->IoStatus.Status;
    }

    // Free the Irp since we are not going to use it
    IoFreeIrp(Irp);

    return Status;
}

// Helper function to copy the user payload to the IO buffer
VOID CopyPayloadToIOBuffer(PGATEWAY_PAYLOAD pPayload, PUSER_BUFFER pIOBuffer)
{
    PUCHAR pCopyPayloadToAddress = pIOBuffer->payloadNPBuffer;

#if defined(USE_SRB_MDL)
    BOOLEAN IsReadOrWriteRequest = ((pPayload->mode == Read) || (pPayload->mode == Write));
#endif // defined(USE_SRB_MDL)

    // 
    // First, write the header that precedes the payload
    //
    CopyPayloadHeaderToBuffer(pPayload, pCopyPayloadToAddress);

    //
    // Second, write the actual payload after the end of header's buffer. 
    //

#if defined(USE_SRB_MDL)
    // Payload for Write request is already chained in the MDL of SocketBuffer (and will be chained
    // for Read request when we are receiving it), so we have no work to do for them.
    if (!IsReadOrWriteRequest)
#endif // defined(USE_SRB_MDL)
    {
        pCopyPayloadToAddress += GET_PAYLOAD_HEADER_SIZE;

        CopyPayloadToBuffer(pPayload, pCopyPayloadToAddress);
    }
}

// This routine is invoked when the IO operation to send the request was deemed completed (independent of
// whether it was successful or not) by the WinSock Kernel.
NTSTATUS SendRequestCompletionRoutine(
    IN PDEVICE_OBJECT pDevice,
    IN PIRP pIrp,
    IN PVOID pContext
)
{
    UNREFERENCED_PARAMETER(pDevice);

    NTSTATUS Status = pIrp->IoStatus.Status;
    PRW_REQUEST pRWRequest = (PRW_REQUEST)pContext;
    
    NT_ASSERT(pRWRequest != NULL);
    if (pRWRequest)
    {       
    // Proceed to initiate the response if request was sent successfully.
        if (NT_SUCCESS(Status))
        {
            // We should have sent the expected number of bytes
            ULONG bytesSent = (ULONG)(pIrp->IoStatus.Information);
            if (bytesSent != (pRWRequest->SocketBuffer.SocketBuffer.Length))
            {
                EventWriteUnableToSendEntireRequest(NULL, bytesSent, pRWRequest->SocketBuffer.SocketBuffer.Length);
                KdPrint(("Unable to send entire request to the service - sent %08X of %08X bytes.", bytesSent, pRWRequest->SocketBuffer.SocketBuffer.Length));
                Status = GET_RW_ERROR_STATUS(pRWRequest->IsReadRequest);
            }
            else
            {
                // For a successful send of Write request, we expect the EndOfPayload marker to be not present
                // since the buffer would have been overwritten with the actual payload.
                if (pRWRequest->CheckForPayloadConsistency)
                {
                    // Reset the payload consistency marker flag
                    pRWRequest->CheckForPayloadConsistency = FALSE;

                    if (IsEndOfPayloadMarkerPresent(&pRWRequest->SocketBuffer))
                    {
                        // Inconsistent payload was sent. Fail the request.
                        NT_ASSERT(FALSE);
                        EventWriteInconsistentPayloadSent(NULL);
                        KdPrint(("Inconsistent payload sent to the service."));
                        Status = GET_RW_ERROR_STATUS(pRWRequest->IsReadRequest);
                    }
                }

                if (NT_SUCCESS(Status))
                {
                    // Since the request was sent successfully, initiate the receive operation.
                    Status = ReceiveBlockResponseFromService(pRWRequest);
                }
            }
        }

        if (!NT_SUCCESS(Status))
        {
            // If we got failure while sending or initiating a response operation, reflect it in the overall status
            // and exit out.
            InterlockedCompareExchange(pRWRequest->pSubrequestsStatus, Status, STATUS_SUCCESS);
            if (InterlockedDecrement(pRWRequest->pSubrequestsActiveCount) == 0)
            {
                KeSetEvent(pRWRequest->pSubrequestsEventsync, IO_NO_INCREMENT, FALSE);
            }
            // Free the buffer
            FreeRWRequest(pRWRequest);
        }
    }
    else
    {
        EventWriteUnexpectedContextinSendRequestCompletionRoutine(NULL);
        HandleInvariantFailure();
    }

    // All IO completion routines need to return this (see https://docs.microsoft.com/en-us/windows-hardware/drivers/network/using-irps-with-winsock-kernel-functions).
    return STATUS_MORE_PROCESSING_REQUIRED;
}

// Initiates the request to send data asynchronously. When the send operation completes (success or fail), our completion routine would be invoked
// for the post-send processing.
NTSTATUS SendRequestAsync(
    IN PRW_REQUEST pRWRequest,
    IN PGATEWAY_PAYLOAD payload
)
{
    // Copy the payload to the I/O buffer
    CopyPayloadToIOBuffer(payload, &pRWRequest->SocketBuffer);

    // Attempt to send the data. Since Irp is already setup to point to SendRequestCompletionRoutine,
    // it will be called when the data has been sent.
    PWSK_SOCKET pSocket = *(pRWRequest->ppSocket);
    NTSTATUS Status = ((PWSK_PROVIDER_CONNECTION_DISPATCH)(pSocket->Dispatch))->WskSend(
        pSocket,
        &(pRWRequest->SocketBuffer.SocketBuffer),
        WSK_FLAG_NODELAY, // Send data immediately
        pRWRequest->pIrp);

    return Status;
}

// This routine is invoked when the IO operation to receive the request was deemed completed (independent of
// whether it was successful or not) by the WinSock Kernel.
NTSTATUS ReceiveResponseCompletionRoutine(
    IN PDEVICE_OBJECT pDevice,
    IN PIRP pIrp,
    IN PVOID pContext
)
{
    UNREFERENCED_PARAMETER(pDevice);

    NTSTATUS Status = pIrp->IoStatus.Status;
    PRW_REQUEST pRWRequest = (PRW_REQUEST)pContext;

    if (NT_SUCCESS(Status))
    {
        NT_ASSERT(pRWRequest != NULL);
        if (pRWRequest)
        {
            Status = STATUS_SUCCESS;

            // We should have received the expected number of bytes
            ULONG bytesReceived = (ULONG)(pIrp->IoStatus.Information);
            if (bytesReceived != (pRWRequest->SocketBuffer.SocketBuffer.Length))
            {
                EventWriteUnableToReceiveEntireRequest(NULL, bytesReceived, pRWRequest->SocketBuffer.SocketBuffer.Length);
                KdPrint(("Unable to receive entire request from the service - received %08X of %08X bytes.", bytesReceived, pRWRequest->SocketBuffer.SocketBuffer.Length));
                Status = GET_RW_ERROR_STATUS(pRWRequest->IsReadRequest);
                goto Exit;
            }

            // For a successful receive of Read request response, we expect the EndOfPayload marker to be not present
            // since the buffer would have been overwritten with the actual payload.
            if (pRWRequest->CheckForPayloadConsistency)
            {
                // Reset payload consistency marker flag
                pRWRequest->CheckForPayloadConsistency = FALSE;

                if (IsEndOfPayloadMarkerPresent(&pRWRequest->SocketBuffer))
                {
                    NT_ASSERT(FALSE);
                    EventWriteInconsistentPayloadReceived(NULL);
                    KdPrint(("Inconsistent payload received from the service."));
                    Status = GET_RW_ERROR_STATUS(pRWRequest->IsReadRequest);
                    goto Exit;
                }
            }

            // Receive the payload from the service
            GATEWAY_PAYLOAD payload;
            RtlZeroMemory(&payload, sizeof(payload));
            payload.mode = pRWRequest->IsReadRequest ? Read : Write;

#if !defined(USE_DMA_FOR_SRB_IO) 
            if (pRWRequest->IsReadRequest)
            {
#if !defined(USE_SRB_MDL)
                // Response of Read request will carry the payload buffer.
                // Setup the buffer to copy the payload to.
                payload.payloadData = pRWRequest->pRWBufferAddress;
#else // defined(USE_SRB_MDL)
                // The data would have already been read into the SRB's MDL
                // so we do not have to do anything here.
#endif // !defined(USE_SRB_MDL)
            }
#else // defined(USE_DMA_FOR_SRB_IO)

            // When using DMA, we only have to do the following:
            //
            // 1) No payload buffer reference (since this is a DMA)

            payload.payloadData = NULL;

#endif // !defined(USE_DMA_FOR_SRB_IO)

            // Copy the received data to the payload buffer user has specified
            CopyPayloadFromIOBuffer(&payload, pRWRequest->SocketBuffer.payloadNPBuffer);

            Status = GetNTSTATUSForResponsePayload(pRWRequest->IsReadRequest, &payload);
        }
        else
        {
            EventWriteUnexpectedContextinReceiveResponseCompletionRoutine(NULL);
            HandleInvariantFailure();
        }
    }
   

Exit:
    if (NT_SUCCESS(Status))
    {
        if (pRWRequest)
        {
            // We are all done with the response - wrap it up now.
            // 
            // Update the overall status
            InterlockedCompareExchange(pRWRequest->pSubrequestsStatus, Status, STATUS_SUCCESS);

            // Decrement the subrequest count and set the event if this is the last subrequest.
            if (InterlockedDecrement(pRWRequest->pSubrequestsActiveCount) == 0)
            {
                KeSetEvent(pRWRequest->pSubrequestsEventsync, IO_NO_INCREMENT, FALSE);
            }

            // Finally, free the buffer
            FreeRWRequest(pRWRequest);
        }
        else
        {
            EventWriteUnexpectedContextinReceiveResponseCompletionRoutine(NULL);
            HandleInvariantFailure();
        }
    }

    // All IO completion routines need to return this (see https://docs.microsoft.com/en-us/windows-hardware/drivers/network/using-irps-with-winsock-kernel-functions).
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS ReceiveResponseAsync(
    IN PRW_REQUEST pRWRequest
)
{
    // Reset the Irp for receive response
    AllocAndInitIrp(&pRWRequest->pIrp, TRUE, ReceiveResponseCompletionRoutine, pRWRequest);

    // Attempt to receive the data
    PWSK_SOCKET pSocket = *(pRWRequest->ppSocket);
    
#if !defined(USE_DMA_FOR_SRB_IO)
    if (!pRWRequest->IsReadRequest)
    {
        // Write response only contains the header

#if defined(USE_SRB_MDL)
        // Unchain the MDL that was used to send the data
        pRWRequest->SocketBuffer.SocketBuffer.Mdl->Next = NULL;
#endif // defined(USE_SRB_MDL)

        // Update the size in the WSK_BUF length since we will be only
        // receiving the payload header.
        pRWRequest->SocketBuffer.SocketBuffer.Length = GET_PAYLOAD_HEADER_SIZE;
    }
    else
    {
        // Read response contains header and payload.

#if defined(USE_SRB_MDL)
        // Chain the MDL that will be used to receive the data
        NT_ASSERT(pRWRequest->SocketBuffer.SocketBuffer.Mdl->Next == NULL);
        pRWRequest->SocketBuffer.SocketBuffer.Mdl->Next = pRWRequest->pSrbDataBufferMdl;

        pRWRequest->CheckForPayloadConsistency = FALSE;
#else // !defined(USE_SRB_MDL)
        // We do not need to do anything here since the payload buffer is already allocated as part of the SocketBuffer.
        // 
        // However, set the payload consistency marker to validate the consistency of the returned payload from the service.
        pRWRequest->CheckForPayloadConsistency = TRUE;
#endif // #endif // defined(USE_SRB_MDL)

        // Update the size in the WSK_BUF length as well to receive the payload
        // header and the data from the service.
        pRWRequest->SocketBuffer.SocketBuffer.Length = GET_PAYLOAD_HEADER_SIZE + pRWRequest->lengthToRW;
    }
#else // defined(USE_DMA_FOR_SRB_IO)

    // When using DMA, we only have to do the following:
    //
    // 1) Socket buffer size to be the payload header size.

    pRWRequest->SocketBuffer.SocketBuffer.Length = GET_PAYLOAD_HEADER_SIZE;
    pRWRequest->CheckForPayloadConsistency = FALSE;

#endif // !defined(USE_DMA_FOR_SRB_IO)

    // Reset the SocketBuffer prior to (re)using it.
    ResetSocketBuffer(&pRWRequest->SocketBuffer);

    if (pRWRequest->CheckForPayloadConsistency)
    {
        SetEndOfPayloadMarker(&pRWRequest->SocketBuffer);
    }

    NTSTATUS Status = ((PWSK_PROVIDER_CONNECTION_DISPATCH)(pSocket->Dispatch))->WskReceive(
        pSocket,
        &(pRWRequest->SocketBuffer.SocketBuffer),
        WSK_FLAG_WAITALL,  // Complete Irp when the buffer is completely full (service does similar thing when receiving requests).
        pRWRequest->pIrp);

    return Status;
}

NTSTATUS SendBuffer(
    IN PWSK_SOCKET socket,
    IN PUSER_BUFFER pSocketBuffer,
    IN PIRP pIrp,
    IN PKEVENT pSyncEvent,
    IN PGATEWAY_PAYLOAD payload
)
{

    // Validate the userBuffer
    if (!payload)
        return STATUS_INVALID_PARAMETER_5;

    // Reset the SocketBuffer prior to (re)using it.
    ResetSocketBuffer(pSocketBuffer);

    // Reset the Irp and SyncEvent for Reuse
    AllocAndInitSyncIrp(&pIrp, pSyncEvent, TRUE);

    // Copy the payload to the I/O buffer
    CopyPayloadToIOBuffer(payload, pSocketBuffer);

    // Attempt to send the data
    NTSTATUS Status = ((PWSK_PROVIDER_CONNECTION_DISPATCH)(socket->Dispatch))->WskSend(
        socket,
        &(pSocketBuffer->SocketBuffer),
        WSK_FLAG_NODELAY, // Send data immediately
        pIrp);

    if (Status == STATUS_PENDING) {
        // Wait for the WSK subsystem to complete the work
        KeWaitForSingleObject(pSyncEvent, Executive, KernelMode, FALSE, NULL);
        Status = pIrp->IoStatus.Status;

        // We should have sent the expected number of bytes. 
        ULONG bytesSent = (ULONG)(pIrp->IoStatus.Information);
        if (bytesSent != (pSocketBuffer->SocketBuffer.Length))
        {
            NT_ASSERT(FALSE);
        }
    }

    return Status;
}

NTSTATUS ReceiveBuffer(
    IN PWSK_SOCKET socket,
    IN PUSER_BUFFER pSocketBuffer,
    IN PIRP pIrp,
    IN PKEVENT pSyncEvent,
    IN PGATEWAY_PAYLOAD payload
)
{
    // Validate the userBuffer
    if (!payload)
        return STATUS_INVALID_PARAMETER_5;

    // Reset the SocketBuffer prior to (re)using it.
    ResetSocketBuffer(pSocketBuffer);

    // Reset the Irp and SyncEvent for Reuse
    AllocAndInitSyncIrp(&pIrp, pSyncEvent, TRUE);

    // Attempt to receive the data
    NTSTATUS Status = ((PWSK_PROVIDER_CONNECTION_DISPATCH)(socket->Dispatch))->WskReceive(
        socket,
        &(pSocketBuffer->SocketBuffer),
        WSK_FLAG_WAITALL,  // Complete Irp when the buffer is completely full (service does similar thing when receiving requests).
        pIrp);

    if (Status == STATUS_PENDING) {
        // Wait for the WSK subsystem to complete the work
        KeWaitForSingleObject(pSyncEvent, Executive, KernelMode, FALSE, NULL);
        Status = pIrp->IoStatus.Status;
    }

    if (NT_SUCCESS(Status))
    {
        // Copy the received data to the payload buffer user has specified
        CopyPayloadFromIOBuffer(payload, pSocketBuffer->payloadNPBuffer);
    }

    return Status;
}

NTSTATUS ResolveFQDNToIpV4Address(
    IN pHW_HBA_EXT pHBAExt,
    IN PUNICODE_STRING fqdnToResolve,
    IN OUT PSOCKADDR resolvedAddress
)
{
    PIRP Irp = NULL;
    KEVENT SyncEvent = { 0 };

    if (!fqdnToResolve)
        return STATUS_INVALID_PARAMETER_1;

    // Initialize the Irp that we will use
    NTSTATUS Status = AllocAndInitSyncIrp(&Irp, &SyncEvent, FALSE);
    if (!NT_SUCCESS(Status))
        return Status;

    PADDRINFOEXW resolutionResult = NULL;
    RtlZeroMemory(resolvedAddress, sizeof(SOCKADDR));

    // The functions below can only be invoked at passive level
    NT_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    // Attempt the resolution
    Status = pHBAExt->WskProviderNpi.Dispatch->WskGetAddressInfo(
        pHBAExt->WskProviderNpi.Client,
        fqdnToResolve,
        NULL,
        NS_ALL,
        NULL,
        NULL,
        &resolutionResult,
        NULL,
        NULL,
        Irp
    );

    if (Status == STATUS_PENDING) {
        // Wait for the WSK subsystem to complete the work
        KeWaitForSingleObject(&SyncEvent, Executive, KernelMode, FALSE, NULL);
        Status = Irp->IoStatus.Status;
    }

    // Free the Irp since we are not going to use it
    IoFreeIrp(Irp);

    if (NT_SUCCESS(Status)) {

        // Extract the IPV4 address we got for it.
        PADDRINFOEXW currentAddrInfo = resolutionResult;

        while (currentAddrInfo) {

            // Only look for IPV4 Addresses
            if (currentAddrInfo->ai_family == AF_INET) {

                // Copy over the information since we are going to release the result set below.
                *resolvedAddress = *(currentAddrInfo->ai_addr);
                Status = STATUS_SUCCESS;
                break;
            }

            // Move to the next entry
            currentAddrInfo = currentAddrInfo->ai_next;
        }

        // Set the status if we didnt find an appropriate resolution result
        if (!currentAddrInfo) {
            Status = STATUS_NOT_FOUND;
        }

        // Free the result array
        currentAddrInfo = resolutionResult;
        while (currentAddrInfo)
        {
            PADDRINFOEXW nextAddrInfo = currentAddrInfo->ai_next;
            pHBAExt->WskProviderNpi.Dispatch->WskFreeAddressInfo(pHBAExt->WskProviderNpi.Client, currentAddrInfo);
            currentAddrInfo = nextAddrInfo;
        }
    }

    return Status;
}


// Sends the block request to the BlockService
NTSTATUS SendBlockRequestToService(PRW_REQUEST pRWRequest)
{
    NTSTATUS status = GET_RW_ERROR_STATUS(pRWRequest->IsReadRequest);

    // Get the latest overall status to see if any request failed or not.
    status = InterlockedOr(pRWRequest->pSubrequestsStatus, 0);
    if (NT_SUCCESS(status))
    {
        // Send the payload to the service
        GATEWAY_PAYLOAD payload;
        RtlZeroMemory(&payload, sizeof(payload));

        payload.mode = pRWRequest->IsReadRequest ? Read : Write;
        payload.offsetToRW = pRWRequest->offset;
        payload.sizePayloadBuffer = pRWRequest->lengthToRW;

#if !defined(USE_DMA_FOR_SRB_IO)
        if (payload.mode == Write)
        {
            // Write request contains the header + payload

#if defined(USE_SRB_MDL)
            // Chain the partial MDL that contains the data to be sent.
            NT_ASSERT(pRWRequest->SocketBuffer.SocketBuffer.Mdl->Next == NULL);
            pRWRequest->SocketBuffer.SocketBuffer.Mdl->Next = pRWRequest->pSrbDataBufferMdl;

            // We cannot check for payload consistency since the payload is already setup and we cannot
            // modify it (which is what setting of marker does).
            // Set the flag to write the marker to be used for consistency of sent data.
            pRWRequest->CheckForPayloadConsistency = FALSE;
#else // !defined(USE_SRB_MDL)
            // Set the buffer from which to send the data
            payload.payloadData = pRWRequest->pRWBufferAddress;

            // Set the flag to write the marker to be used for consistency of sent data.
            // This sets the marker in the buffer that will have payload written to it
            // when sending it via the sockets.
            pRWRequest->CheckForPayloadConsistency = TRUE;
#endif // defined(USE_SRB_MDL)

            // Reflect the overall size in the WSK_BUF length as well.
            pRWRequest->SocketBuffer.SocketBuffer.Length = GET_PAYLOAD_HEADER_SIZE + payload.sizePayloadBuffer;
        }
        else
        {
            // Read request only contains the header

#if defined(USE_SRB_MDL)
            // For Read requests, we only have the payload header being sent. The MDL for it, and the corresponding
            // SocketBuffer length, is already setup as expected.
            NT_ASSERT(pRWRequest->SocketBuffer.SocketBuffer.Length == GET_PAYLOAD_HEADER_SIZE);
#else // !defined(USE_SRB_MDL)
            // Adjust the Socket buffer length so that we only send the payload header.
            pRWRequest->SocketBuffer.SocketBuffer.Length = GET_PAYLOAD_HEADER_SIZE;
#endif // defined(USE_SRB_MDL)
        }
#else // defined(USE_DMA_FOR_SRB_IO) 

        // When using DMA, we only have to do the following:
        //
        // 1) Setup SRB reference
        // 2) No payload buffer reference (since this is a DMA)
        // 3) Socket buffer size to be the payload header size.

        payload.addressSRB = (ULONG64)pRWRequest->pSrb;
        payload.payloadData = NULL;
        pRWRequest->SocketBuffer.SocketBuffer.Length = GET_PAYLOAD_HEADER_SIZE;
        pRWRequest->CheckForPayloadConsistency = FALSE;

#endif // !defined(USE_DMA_FOR_SRB_IO)

        payload.deviceID = pRWRequest->pLUID;
        payload.lengthDeviceID = (ULONG)(sizeof(wchar_t) * (wcslen(payload.deviceID) + 1));

        // Reset the SocketBuffer prior to (re)using it.
        ResetSocketBuffer(&pRWRequest->SocketBuffer);

        // Set the payload consistency marker if applicable.
        if (pRWRequest->CheckForPayloadConsistency)
        {
            SetEndOfPayloadMarker(&pRWRequest->SocketBuffer);
        }

        InterlockedIncrement(pRWRequest->pSubrequestsActiveCount);
        status = SendRequestAsync(pRWRequest, &payload);
        if (!NT_SUCCESS(status))
        {
            // Decrement the count if the request could not be sent.
            InterlockedDecrement(pRWRequest->pSubrequestsActiveCount);
        }
    }

    return status;
}

// Receives a block response from the service.
NTSTATUS ReceiveBlockResponseFromService(PRW_REQUEST pRWRequest)
{
    NTSTATUS status = GET_RW_ERROR_STATUS(pRWRequest->IsReadRequest);

    // Get the latest overall status to see if any request failed or not.
    status = InterlockedOr(pRWRequest->pSubrequestsStatus, 0);
    if (NT_SUCCESS(status))
    {
        status = ReceiveResponseAsync(pRWRequest);
    }

    return status;
}

NTSTATUS
AllocAndInitIrp(
    IN OUT PIRP* IrpForInit,
    IN BOOLEAN fReuseIrp,
    IN PIO_COMPLETION_ROUTINE CompletionRoutine,
    IN PVOID ContextCompletionRoutine
)
{
    if (!fReuseIrp)
    {
        // Attempt to allocate the Irp to be used by the caller
        *IrpForInit = IoAllocateIrp(1, FALSE);
        if (!(*IrpForInit)) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else
    {
        IoReuseIrp(*IrpForInit, STATUS_SUCCESS);
    }

    // Setup the IoCompletionRoutine callback that will set the event when the work is done.
    IoSetCompletionRoutine(*IrpForInit, CompletionRoutine, ContextCompletionRoutine, TRUE, TRUE, TRUE);

    return STATUS_SUCCESS;
}

NTSTATUS
AllocAndInitSyncIrp(
    IN OUT PIRP* IrpForInit,
    IN PKEVENT SyncEvent,
    IN BOOLEAN fReuseIrp
)
{
    if (!fReuseIrp)
    {
        // Attempt to allocate the Irp to be used by the caller
        *IrpForInit = IoAllocateIrp(1, FALSE);
        if (!(*IrpForInit)) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else
    {
        IoReuseIrp(*IrpForInit, STATUS_SUCCESS);
    }

    // Init the synchronization object
    KeInitializeEvent(SyncEvent, SynchronizationEvent, FALSE);

    // Setup the IoCompletionRoutine callback that will set the event when the work is done.
    IoSetCompletionRoutine(*IrpForInit, WorkCompletionRoutine, SyncEvent, TRUE, TRUE, TRUE);

    return STATUS_SUCCESS;
}
#endif // !defined(USE_NP_TRANSPORT)