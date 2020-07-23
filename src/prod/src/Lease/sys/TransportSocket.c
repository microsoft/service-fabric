// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "LeasLayr.h"

//
// Forward declarations.
//
PWSK_PROVIDER_NPI
GetWskProviderNpi(
    );

PVOID TransportAlloc(ULONG BufferSize, ULONG Tag)
{
    PVOID Buffer;

    Buffer = ExAllocatePoolWithTag(
        NonPagedPool, 
        BufferSize, 
        Tag
        );

    if (NULL != Buffer) 
    {
        RtlZeroMemory(Buffer, BufferSize);
    }

    return Buffer;
}

void TransportFree(PVOID Buffer, ULONG Tag)
{
    ExFreePoolWithTag(Buffer, Tag);
}

NTSTATUS
TransportSocketSyncIrpCompletionRoutine(
    __in PDEVICE_OBJECT Reserved,
    __in PIRP Irp,
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Context
    )
    
/*++

Routine Description:

    Irp completion routine used for synchronously waiting for socket operation completion.

Parameters Description:

    Reserved - n/a

    Irp - completion Irp.

    Context - event to signal.

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED

--*/

{    
    PKEVENT CompletionEvent = (PKEVENT)Context;
    
    UNREFERENCED_PARAMETER(Reserved);

    if (Irp->PendingReturned) 
    {
        KeSetEvent(CompletionEvent, 0, FALSE);  
    }
    
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
TransportSocketCloseIrpCompletionRoutine(
    __in PDEVICE_OBJECT Reserved,
    __in PIRP Irp,
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Context
    )
    
/*++

Routine Description:

    Irp completion routine used for asynchronously socket close.

Parameters Description:

    Reserved - n/a

    Irp - completion Irp.

    Context - n/a.

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED

--*/

{    
    PLEASE_TRANSPORT Transport;
    
    UNREFERENCED_PARAMETER(Reserved);
    UNREFERENCED_PARAMETER(Context);

    Transport = (PLEASE_TRANSPORT)Context;
    Transport->CloseStatus = Irp->IoStatus.Status;
    TransportRelease(Transport);

    //
    // The Irp is not needed anymore.
    //
    IoFreeIrp(Irp);
    
    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID TransportSocketCloseWorkItem(
    __in PVOID IoObject,
    __in PVOID Context,
    __in PIO_WORKITEM WorkItem)
{
    PTRANSPORT_CONNECTION Connection = (PTRANSPORT_CONNECTION)Context;
    PLEASE_TRANSPORT Transport = Connection->Target->Transport;

    UNREFERENCED_PARAMETER(IoObject);
    UNREFERENCED_PARAMETER(WorkItem);

    TransportConnectionRelease(Connection);
    TransportRelease(Transport);
}

NTSTATUS
TransportConnectionSocketCloseIrpCompletionRoutine(
    __in PDEVICE_OBJECT Reserved,
    __in PIRP Irp,
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Context
    )
    
/*++

Routine Description:

    Irp completion routine used for asynchronously socket close.

Parameters Description:

    Reserved - n/a

    Irp - completion Irp.

    Context - n/a.

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED

--*/

{    
    PTRANSPORT_CONNECTION Connection = (PTRANSPORT_CONNECTION)Context;

    UNREFERENCED_PARAMETER(Reserved);

    IoQueueWorkItemEx(Connection->CloseCompleteWorkItem, TransportSocketCloseWorkItem, DelayedWorkQueue, Connection);

    //
    // The Irp is not needed anymore.
    //
    IoFreeIrp(Irp);
    
    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID
TransportSocketCloseConnection(
    __in PTRANSPORT_CONNECTION Connection
    )
    
/*++

Routine Description:

    Closes a socket. The close operation is asynchronous.

Arguments:

    socket - socket context that holds the socket.

Return Value:

    n/a
   
--*/

{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PWSK_SOCKET wskSocket = NULL;
    PIRP Irp = NULL;

    //
    // Check if the socket has not yet been closed.
    //
    if (NULL != Connection->Socket) 
    {
        //
        // Get the socket and the socket close Irp.
        //
        wskSocket = Connection->Socket;
        Irp = Connection->SocketCloseIrp;
        
        //
        // Reset socket and its close Irp to NULL.
        //
        Connection->Socket = NULL;
        Connection->SocketCloseIrp = NULL;

        if (NULL != wskSocket) 
        {
            ASSERT(NULL != Irp);

            //
            // Set the completion routine for the Irp.
            //
            IoSetCompletionRoutine(
                Irp,
                TransportConnectionSocketCloseIrpCompletionRoutine,
                Connection,
                TRUE,
                TRUE,
                TRUE
                );

            //
            // Initiate close socket.
            //
            Status = ((PWSK_PROVIDER_BASIC_DISPATCH)wskSocket->Dispatch)->WskCloseSocket(
                wskSocket,
                Irp
                );
            ASSERT(STATUS_SUCCESS == Status || STATUS_PENDING == Status);
        }
    }
}

VOID
TransportSocketClose(PWSK_SOCKET wskSocket, PIRP Irp, PLEASE_TRANSPORT Transport)
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    //
    // Check if the socket has not yet been closed.
    //
    if (NULL != wskSocket) 
    {
        ASSERT(NULL != Irp);
        
        //
        // Set the completion routine for the Irp.
        //
        IoSetCompletionRoutine(
            Irp,
            TransportSocketCloseIrpCompletionRoutine,
            Transport,
            TRUE,
            TRUE,
            TRUE
            );

        //
        // Initiate close socket.
        //
        Status = ((PWSK_PROVIDER_BASIC_DISPATCH)wskSocket->Dispatch)->WskCloseSocket(
            wskSocket,
            Irp
            );
        ASSERT(STATUS_SUCCESS == Status || STATUS_PENDING == Status);
    }
}

VOID TransportSocketSendWorkItem(
    __in PVOID IoObject,
    __in PVOID Context,
    __in PIO_WORKITEM WorkItem)
{
    PTRANSPORT_CONNECTION Connection = (PTRANSPORT_CONNECTION)Context;
    PLEASE_TRANSPORT Transport = Connection->Target->Transport;

    UNREFERENCED_PARAMETER(IoObject);
    UNREFERENCED_PARAMETER(WorkItem);
    
    TransportSendComplete(Connection, Connection->SendStatus);
    TransportRelease(Transport);
}


NTSTATUS
TransportSocketSendComplete(
    __in PDEVICE_OBJECT DeviceObject,
    __in PIRP Irp,
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Context
    )
    
/*++

Routine Description:

    Send I/O completion routine.

Arguments:

    DeviceObject - n/a.

    Irp - Irp for completion.

    Context - lease socket context where the send operation was posted.

Return Value:

   STATUS_MORE_PROCESSING_REQUIRED

--*/

{
    PTRANSPORT_CONNECTION Connection = (PTRANSPORT_CONNECTION)Context;

    UNREFERENCED_PARAMETER(DeviceObject);
  
    //
    // Make sure the message fragment was processed successfully.
    //
    Connection->SendStatus = Irp->IoStatus.Status;
    IoQueueWorkItemEx(Connection->SendCompleteWorkItem, TransportSocketSendWorkItem, DelayedWorkQueue, Connection);

    //
    // Free the MDL.
    //
    IoFreeMdl(Irp->MdlAddress);

    //
    // Free the Irp.
    //
    IoFreeIrp(Irp);

    //
    // Always return STATUS_MORE_PROCESSING_REQUIRED to
    // terminate the completion processing of the IRP.
    //
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
TransportSocketSend(
    __in PTRANSPORT_CONNECTION Connection,
    __in PVOID Buffer,
    __in ULONG BufferSize
    )
    
/*++

Routine Description:

    Routine to send data over a connection-oriented socket.

Arguments:

    socket - lease socket context to use.

    Buffer - buffer to send (pre-allocated by caller).

    BufferSize - buffer size.

Return Value:

   STATUS_DEVICE_NOT_READY if the socket is closing/closed
   STATUS_INSUFFICIENT_RESOURCES if cannot allocate MDL
   STATUS_PENDING otherwise

--*/

{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PMDL MessageMdl = NULL;
    WSK_BUF WskBuffer;
    PIRP Irp = NULL;
    
    //
    // Allocate the IRP.
    //
    Irp = IoAllocateIrp(1, FALSE);
    
    if (NULL == Irp) 
    {
        EventWriteAllocateFail18(
            NULL
            ); 
        Status = STATUS_INSUFFICIENT_RESOURCES;

        goto Error;
    }

    //
    // Set the completion routine for the Irp.
    //
    IoSetCompletionRoutine(
        Irp,
        TransportSocketSendComplete,
        Connection,
        TRUE,
        TRUE,
        TRUE
        );

    //
    // Allocate MDL for the socket buffer and associate it with the Irp.
    //
    MessageMdl = IoAllocateMdl(
        Buffer,
        BufferSize, 
        FALSE, 
        FALSE, 
        Irp
        );
    
    if (NULL == MessageMdl) 
    {
        //
        // Do not have resources to complete this send operation.
        //
        Status = STATUS_INSUFFICIENT_RESOURCES;

        goto Error;
    }
    
    MmBuildMdlForNonPagedPool(MessageMdl);

    //
    // Set up the Wsk buffer with the MDL.
    //
    WskBuffer.Offset = 0;
    WskBuffer.Length = BufferSize;
    WskBuffer.Mdl = MessageMdl;

    if (NULL != Connection->Socket) 
    {
        // add a reference for the send operation
        TransportConnectionAddref(Connection);
        TransportAddRef(Connection->Target->Transport);

        //
        // Initiate sending the buffer.
        //
        Status = ((PWSK_PROVIDER_CONNECTION_DISPATCH)Connection->Socket->Dispatch)->WskSend(
            Connection->Socket,
            &WskBuffer,
            0,
            Irp
            );
    } 
    else 
    {
        //
        // The socket was closed previously.
        //
        Status = STATUS_FILE_FORCED_CLOSED;
    }

Error:
    if (!NT_SUCCESS(Status)) 
    {
        if (NULL != MessageMdl) 
        {
            //
            // Free the MDL.
            //
            IoFreeMdl(MessageMdl);
            
        }

        if (NULL != Irp) 
        {
            //
            // Free the Irp.
            //
            IoFreeIrp(Irp);
        }
    }

    return Status;
}

VOID TransportSocketReceiveWorkItem(
    __in PVOID IoObject,
    __in PVOID Context,
    __in PIO_WORKITEM WorkItem)
{
    PTRANSPORT_CONNECTION Connection = (PTRANSPORT_CONNECTION)Context;
    PLEASE_TRANSPORT Transport = Connection->Target->Transport;

    UNREFERENCED_PARAMETER(IoObject);
    UNREFERENCED_PARAMETER(WorkItem);
    
    TransportReceiveComplete(Connection, Connection->ReceiveStatus, Connection->ReceiveBuffer, Connection->ReceiveBufferSize);
    TransportRelease(Transport);
}

NTSTATUS
TransportSocketReceiveComplete(
    __in PDEVICE_OBJECT DeviceObject,
    __in PIRP Irp,
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Context
    )
    
/*++

Routine Description:

    Receive I/O completion routine.

Arguments:

    DeviceObject - n/a.

    Irp - Irp for completion.

    Context - lease socket context where the receive operation was posted.

Return Value:

   STATUS_MORE_PROCESSING_REQUIRED

--*/

{
    PVOID Buffer = NULL;
    PTRANSPORT_CONNECTION Connection = (PTRANSPORT_CONNECTION)Context;
    ULONG BufferSize = 0;

    UNREFERENCED_PARAMETER(DeviceObject);

    if (NT_SUCCESS(Irp->IoStatus.Status)) 
    {
        //
        // Retrieve the buffer.
        //
        Buffer = MmGetMdlVirtualAddress(Irp->MdlAddress);

        //
        // Retrieve the buffer size.
        //
        BufferSize = (ULONG)Irp->IoStatus.Information;
    }
 
    //
    // Make sure the message fragment was processed successfully.
    //
    Connection->ReceiveStatus = Irp->IoStatus.Status;
    Connection->ReceiveBuffer = Buffer;
    Connection->ReceiveBufferSize = BufferSize;
    IoQueueWorkItemEx(Connection->ReceiveCompleteWorkItem, TransportSocketReceiveWorkItem, DelayedWorkQueue, Connection);

    //
    // Free the MDL for the buffer.
    //
    IoFreeMdl(Irp->MdlAddress);
    
    //
    // Free the Irp.
    //
    IoFreeIrp(Irp);
    
    //
    // Always return STATUS_MORE_PROCESSING_REQUIRED to
    // terminate the completion processing of the IRP.
    //
    return STATUS_MORE_PROCESSING_REQUIRED;
} 

NTSTATUS
TransportSocketReceive(
    __in PTRANSPORT_CONNECTION Connection,
    __in PVOID Buffer,
    __in ULONG BufferSize
    )
    
/*++

Routine Description:

    Routine to receive data over the connection-oriented socket.

Arguments:

    LeaseChannelContext - lease channel context where the receive operation is posted.

    BufferSize - buffer size to receive.
    
    CompletionIrp - completion Irp.

Return Value:

   STATUS_INSUFFICIENT_RESOURCES if cannot allocate receive buffer
   STATUS_PENDING otherwise

--*/

{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PMDL MessageMdl = NULL;
    WSK_BUF WskBuffer;
    PIRP Irp = NULL;
    
    //
    // Allocate the Irp if needed.
    //
    Irp = IoAllocateIrp(1, FALSE);
    
    if (NULL == Irp) {
        EventWriteAllocateFail19(
            NULL
            ); 
        Status = STATUS_INSUFFICIENT_RESOURCES;

        goto End;
    }
    
    RtlZeroMemory(Buffer, BufferSize);

    //
    // Allocate MDL for the socket buffer and associate it with the Irp.
    //
    MessageMdl = IoAllocateMdl(
        Buffer,
        BufferSize, 
        FALSE, 
        FALSE, 
        Irp
        );
    
    if (NULL == MessageMdl) {

        //
        // Do not have resources to complete this send operation.
        //
        Status = STATUS_INSUFFICIENT_RESOURCES;
        
        goto End;
    }
    
    MmBuildMdlForNonPagedPool(MessageMdl);

    //
    // Set the completion routine for the Irp.
    //
    IoSetCompletionRoutine(
        Irp,
        TransportSocketReceiveComplete,
        Connection,
        TRUE,
        TRUE,
        TRUE
        );

    //
    // Set up the Wsk buffer with the MDL.
    //
    WskBuffer.Offset = 0;
    WskBuffer.Length = BufferSize;
    WskBuffer.Mdl = MessageMdl;

    if (NULL != Connection->Socket)
    {
        // add a reference for the receive operation
        TransportConnectionAddref(Connection);
        TransportAddRef(Connection->Target->Transport);

        //
        // Initiate the buffer receive on the socket.
        //
        Status = ((PWSK_PROVIDER_CONNECTION_DISPATCH)Connection->Socket->Dispatch)->WskReceive(
            Connection->Socket,
            &WskBuffer,
            WSK_FLAG_WAITALL, // return when all bytes are in
            Irp
            );
    }
    else
    {
        //
        // The socket was closed previously.
        //
        Status = STATUS_FILE_FORCED_CLOSED;
    }

End:
    if (!NT_SUCCESS(Status)) 
    {
        if (NULL != MessageMdl) 
        {
            //
            // Free the MDL.
            //
            IoFreeMdl(MessageMdl);
        }

        if (NULL != Irp) 
        {
            //
            // Free the Irp.
            //
            IoFreeIrp(Irp);
        }
    }
    
    return Status;
}

/*++

Routine Description:

    Disconnect event callback in case the remote socket disconnects/closes 
    its side of the connection. Clean up the channel on this side of the
    connection. It is guaranteed that all receive operations that are
    still in completion will complete before this event is delivered.

Arguments:

    SocketContext - initial socket context (from the listening socket).

    Flags - n/a

Return Value:

    STATUS_SUCCESS
   
--*/

VOID CompleteSocketDisconnect(__in PVOID SocketContext)
{
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext(WdfGetDriver());
    PTRANSPORT_CONNECTION Connection = (PTRANSPORT_CONNECTION)SocketContext;
    PVOID IterLeaseAgent = NULL;
    PLEASE_AGENT_CONTEXT LeaseAgentContext = NULL;
    KIRQL OldIrql;
    PLIST_ENTRY TargetEntry;
    PLIST_ENTRY ConnectionEntry;
    PTRANSPORT_SENDTARGET Target;
    BOOL ConnectionValid = FALSE;

    // don't do this work if we're unloading: all sockets will get closed anyway
    // and we don't want to get stuck on the mutex
    if (DriverContext->Unloading)
    {
        return;
    }

    // disconnect can race with socket close and can rarely bugcheck under stress
    // find the connection before we touch it

    ExAcquireFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);

    // iterate through transports associated with lease agents
    IterLeaseAgent = RtlEnumerateGenericTable(
        &DriverContext->LeaseAgentContextHashTable,
        TRUE);

    while (!ConnectionValid && (NULL != IterLeaseAgent))
    {
        LeaseAgentContext = *((PLEASE_AGENT_CONTEXT*)IterLeaseAgent);

        // now look at transport send targets to see if the connection still exists
        KeAcquireSpinLock(&LeaseAgentContext->Transport->Lock, &OldIrql);

        for (TargetEntry = LeaseAgentContext->Transport->TargetList.Flink;
             !ConnectionValid && (TargetEntry != &LeaseAgentContext->Transport->TargetList);
             TargetEntry = TargetEntry->Flink)
        {
            Target = CONTAINING_RECORD(TargetEntry, TRANSPORT_SENDTARGET, List);

            for (ConnectionEntry = Target->ConnectionList.Flink;
                 ConnectionEntry != &Target->ConnectionList;
                 ConnectionEntry = ConnectionEntry->Flink) 
            {
                if (Connection == CONTAINING_RECORD(ConnectionEntry, TRANSPORT_CONNECTION, List))
                {
                    // found it
                    ConnectionValid = TRUE;
                    break;
                }
            }
        }

        if (ConnectionValid)
        {
            EventWriteSocketDisconnect(NULL, Connection->Target->Transport->Identifier, Connection->Target->Identifier);

            TransportConnectionAddref(Connection);
            TransportConnectionShutdown(Connection);
            TransportConnectionReleaseInternal(Connection);
        }

        KeReleaseSpinLock(&LeaseAgentContext->Transport->Lock, OldIrql);

        IterLeaseAgent = RtlEnumerateGenericTable(
            &DriverContext->LeaseAgentContextHashTable,
            FALSE);
    }

    ExReleaseFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);
}

VOID SocketDisconnectCallback(
    __in PVOID IoObject,
    __in PVOID Context,
    __in PIO_WORKITEM WorkItem)
{
    UNREFERENCED_PARAMETER(IoObject);

    CompleteSocketDisconnect(Context);
    IoFreeWorkItem(WorkItem);
}

NTSTATUS
TransportSocketDisconnectEvent(
    __in PVOID SocketContext,
    __in ULONG Flags
    )
{
    if (Flags & WSK_FLAG_AT_DISPATCH_LEVEL)
    {
        PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext(WdfGetDriver());
        PIO_WORKITEM WorkItem = IoAllocateWorkItem((PDEVICE_OBJECT)(DriverContext->DriverObject));

        if (WorkItem != NULL) 
        {
            IoQueueWorkItemEx(WorkItem, SocketDisconnectCallback, DelayedWorkQueue, SocketContext);
        }
    }
    else
    {
        CompleteSocketDisconnect(SocketContext);
    }

    return STATUS_SUCCESS;
}

//
// Dispatch routines for a connection oriented socket.
//
const WSK_CLIENT_CONNECTION_DISPATCH WskTransportConnectionDispatch = {
    NULL, // Ignoring when the socket has received data.
    &TransportSocketDisconnectEvent, // Remote disconnect event.
    NULL // Ignoring send backlog event.
};

//
// Dispatch routines for a listening socket.
//
const WSK_CLIENT_LISTEN_DISPATCH WskTransportListenDispatch = {
    &TransportSocketAcceptEvent, // Accept connections event.
    NULL, // WskInspectEvent is required only if conditional-accept is used.
    NULL  // WskAbortEvent is required only if conditional-accept is used.
};

NTSTATUS
TransportSocketAcceptEvent(
    __in PVOID SocketContext,
    __in ULONG Flags,
    __in PSOCKADDR LocalAddress,
    __in PSOCKADDR RemoteAddress,
    __in_opt PWSK_SOCKET AcceptSocket,
    __out_opt PVOID* AcceptSocketContext,
    __out_opt CONST WSK_CLIENT_CONNECTION_DISPATCH** AcceptSocketDispatch
    )
    
/*++

Routine Description:

    Listening socket accept callback which is invoked whenever a new connection arrives.

Arguments:

    SocketContext - initial socket context (from the listening socket).

    Flags - n/a

    LocalAddress - n/a

    RemoteAddress - n/a

    AcceptSocket - the newly created socket (connection-oriented).

    AcceptSocketContext - the new lease socket context.

    AcceptSocketDispatch - n/a

Return Value:

   STATUS_SUCCESS or STATUS_REQUEST_NOT_ACCEPTED
   
--*/

{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PLEASE_TRANSPORT Transport;
    SOCKADDR_STORAGE RemoteSocketAddress;

    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(LocalAddress);

    if (NULL == AcceptSocket || 
        NULL == SocketContext || 
        NULL == AcceptSocketContext || 
        NULL == AcceptSocketDispatch ||
        NULL == RemoteAddress) {

        // If WSK provider makes a WskAcceptEvent callback with NULL 
        // AcceptSocket, this means that the listening socket is no longer
        // functional. We may handle this situation by trying
        // to create a new listening socket. Here, we will not attempt 
        // to create a new listening socket. 
        return STATUS_REQUEST_NOT_ACCEPTED;
    }

    RtlZeroMemory(
        &RemoteSocketAddress, 
        sizeof(SOCKADDR_STORAGE)
        );

#pragma prefast(push)
#pragma prefast(disable: 24002, "IPv4 and IPv6 branches present")

    RtlCopyMemory(
        &RemoteSocketAddress,
        RemoteAddress,
        (AF_INET == RemoteAddress->sa_family) ? sizeof(SOCKADDR_IN) : sizeof(SOCKADDR_IN6)
        );

#pragma prefast(pop)

    if (!IsValidSockAddr(&RemoteSocketAddress)) 
    {
        return STATUS_REQUEST_NOT_ACCEPTED;
    }

    Transport = (PLEASE_TRANSPORT)SocketContext;

    TransportAddRef(Transport);
    Status = TransportAcceptSocket(Transport, AcceptSocket, &RemoteSocketAddress);

    if (NT_SUCCESS(Status))
    {
        return STATUS_SUCCESS;
    }
    else
    {
        TransportRelease(Transport);
        return STATUS_REQUEST_NOT_ACCEPTED;
    }
}

NTSTATUS
TransportSocketEnableKeepAliveListeningSocket(
    __in PLEASE_TRANSPORT Transport,
    __in ULONG KeepAlive,
    __in PIRP Irp
)

/*++

Routine Description:

    Enable keep alive option on the listening socket. Each incoming
    socket connection will now have this options set.
    
    This function waits synchronously for the socket operation to complete.

Arguments:

    connection - connection                                         

    KeepAlive - 0 to disable, 1 to enable.
    
    Irp - Irp to use for completion.

Return Value:

    STATUS_SUCCESS if the option has been successfully set.
    
--*/

{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    
    //
    // Event used for synchronous completion.
    //
    KEVENT CompletionEvent;

    //
    // Initialize the completion event.
    //
    KeInitializeEvent(
        &CompletionEvent, 
        SynchronizationEvent, 
        FALSE
        );

    //
    // Set the completion routine for the Irp.
    //
    IoSetCompletionRoutine(
        Irp,
        TransportSocketSyncIrpCompletionRoutine,
        &CompletionEvent,
        TRUE,
        TRUE,
        TRUE
        );

    //
    // Set the socket option to enable keep alive.
    //
    Status = ((PWSK_PROVIDER_BASIC_DISPATCH)Transport->ListenSocket->Dispatch)->WskControlSocket(
        Transport->ListenSocket,
        WskSetOption,
        SO_KEEPALIVE,
        SOL_SOCKET,
        sizeof(ULONG),
        &KeepAlive,
        0,
        NULL,
        NULL,
        Irp
        );

    if (STATUS_PENDING == Status) {
        
        //
        // Wait for completion or the Irp.
        //
        KeWaitForSingleObject(
            &CompletionEvent, 
            Executive, 
            KernelMode, 
            FALSE, 
            NULL
            );

        //
        // Retrieve the Irp status.
        //
        Status = Irp->IoStatus.Status;
    }

    //
    // Free the Irp.
    //
    IoFreeIrp(Irp);

    return Status;

}

NTSTATUS
TransportSocketEnableAcceptEventListeningSocket(
    __in PLEASE_TRANSPORT Transport,
    __in PIRP Irp
    )

/*++

Routine Description:

    Enable accept event on the listening socket.

    This function waits synchronously for the socket operation to complete.
    
Arguments:

    Transprt - socket context that holds the socket.

    Irp - Irp to use for completion.

Return Value:

    STATUS_SUCCESS if the option has been successfully set.
   
--*/

{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    //
    // Event control structure.
    //
    WSK_EVENT_CALLBACK_CONTROL EventCallbackControl;
    
    //
    // Event used for synchronous completion.
    //
    KEVENT CompletionEvent;

    //
    // Initialize the completion event.
    //
    KeInitializeEvent(
        &CompletionEvent, 
        SynchronizationEvent, 
        FALSE
        );

    //
    // Set the completion routine for the Irp.
    //
    IoSetCompletionRoutine(
        Irp,
        TransportSocketSyncIrpCompletionRoutine,
        &CompletionEvent,
        TRUE,
        TRUE,
        TRUE
        );

    //
    // Specify the WSK NPI identifier.
    //
    EventCallbackControl.NpiId = &NPI_WSK_INTERFACE_ID;
    //
    // Set the event flags for the event callback functions that
    // are to be enabled on the socket.
    //
    EventCallbackControl.EventMask = WSK_EVENT_ACCEPT;

    //
    // Set the socket option to enable accept callback events.
    //
    Status = ((PWSK_PROVIDER_BASIC_DISPATCH)Transport->ListenSocket->Dispatch)->WskControlSocket(
        Transport->ListenSocket,
        WskSetOption,
        SO_WSK_EVENT_CALLBACK,
        SOL_SOCKET,
        sizeof(WSK_EVENT_CALLBACK_CONTROL),
        &EventCallbackControl,
        0,
        NULL,
        NULL,
        Irp
        );

    if (STATUS_PENDING == Status) {
        
        //
        // Wait for completion or the Irp.
        //
        KeWaitForSingleObject(
            &CompletionEvent, 
            Executive, 
            KernelMode, 
            FALSE, 
            NULL
            );

        //
        // Retrieve the Irp status.
        //
        Status = Irp->IoStatus.Status;
    }
    
    //
    // Check status.
    //
    if (NT_SUCCESS(Status)) {

        //
        // Reuse the Irp.
        //
        IoReuseIrp(Irp, STATUS_UNSUCCESSFUL);

        //
        // Enable keep alive on the listening socket.
        //
        Status = TransportSocketEnableKeepAliveListeningSocket(
            Transport, 
            1, // enable
            Irp
            );

    } else {

        //
        // Free the Irp.
        //
        IoFreeIrp(Irp);
    }

    return Status;
}

NTSTATUS
TransportSocketBindListeningSocket(
    __in PLEASE_TRANSPORT Transport,
    __in PIRP Irp
    )
    
/*++

Routine Description:

    Function to bind a listening socket to a local transport address.

    This function waits synchronously for the socket operation to complete.
    
Arguments:

    Transport - transport context that holds the socket.

    Irp - Irp to use for completion.

Return Value:

    STATUS_SUCCESS if the socket has been successfully bound to the
    given address. STATUS_SHARING_VIOLATION if the address is in use.
   
--*/

{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    //
    // Event used for synchronous completion.
    //
    KEVENT CompletionEvent;

    //
    // Initialize the completion event.
    //
    KeInitializeEvent(
        &CompletionEvent, 
        SynchronizationEvent, 
        FALSE
        );

    //
    // Set the completion routine for the Irp.
    //
    IoSetCompletionRoutine(
        Irp,
        TransportSocketSyncIrpCompletionRoutine,
        &CompletionEvent,
        TRUE,
        TRUE,
        TRUE
        );

    //
    // Bind the socket to the local network address.
    //
    Status = ((PWSK_PROVIDER_LISTEN_DISPATCH)Transport->ListenSocket->Dispatch)->WskBind(
        Transport->ListenSocket,
        (SOCKADDR*)&Transport->ListenAddress,
        0,
        Irp
        );

    if (STATUS_PENDING == Status) {
        
        //
        // Wait for completion or the Irp.
        //
        KeWaitForSingleObject(
            &CompletionEvent, 
            Executive, 
            KernelMode, 
            FALSE, 
            NULL
            );

        //
        // Retrieve the Irp status.
        //
        Status = Irp->IoStatus.Status;
    }
    
    //
    // Check status.
    //
    if (NT_SUCCESS(Status)) {

        //
        // Reuse the Irp.
        //
        IoReuseIrp(Irp, STATUS_UNSUCCESSFUL);

        //
        // Enable accept event on the listening socket.
        //
        Status = TransportSocketEnableAcceptEventListeningSocket(
            Transport, 
            Irp
            );

    } else {

        //
        // Free the Irp.
        //
        IoFreeIrp(Irp);
    }

    return Status;
}

NTSTATUS
TransportSocketCreateListeningSocket(
    __in PLEASE_TRANSPORT Transport
    )
    
/*++

Routine Description:

    Creates a listening socket.

    This function waits synchronously for the socket operation to complete.
    
Arguments:

    Transport - lease agent context to hold the socket.

Return Value:

    STATUS_SUCCESS if a listening WSK socket was successfully created.
   
--*/

{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    //
    // Completion Irp.
    //
    PIRP Irp = NULL;
    //
    // Event used for synchronous completion.
    //
    KEVENT CompletionEvent;
    //
    // Get WSK NPI.
    //
    PWSK_PROVIDER_NPI WskNpi = GetWskProviderNpi();
    
    if (NULL == WskNpi) 
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Allocate Irp.
    //
    Irp = IoAllocateIrp(1, FALSE);
    
    if (NULL == Irp) 
    {
        EventWriteAllocateFail20(
            NULL
            ); 
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Initialize the completion event.
    //
    KeInitializeEvent(
        &CompletionEvent, 
        SynchronizationEvent, 
        FALSE
        );

    //
    // Set the completion routine for the Irp.
    //
    IoSetCompletionRoutine(
        Irp,
        TransportSocketSyncIrpCompletionRoutine,
        &CompletionEvent,
        TRUE,
        TRUE,
        TRUE
        );

    //
    // Initiate create socket.
    //
    Status = WskNpi->Dispatch->WskSocket(
        WskNpi->Client,
        Transport->ListenAddress.ss_family,
        SOCK_STREAM,
        IPPROTO_TCP,
        WSK_FLAG_LISTEN_SOCKET,
        Transport, // listening socket context is the transport
        &WskTransportListenDispatch,
        NULL,
        NULL,
        NULL,
        Irp
        );

    if (STATUS_PENDING == Status) 
    {
        //
        // Wait for completion or the Irp.
        //
        KeWaitForSingleObject(
            &CompletionEvent, 
            Executive, 
            KernelMode, 
            FALSE, 
            NULL
            );

        //
        // Retrieve the Irp status.
        //
        Status = Irp->IoStatus.Status;
    }

    //
    // Check Irp status.
    //
    if (NT_SUCCESS(Status)) 
    {
        //
        // Retrieve the socket object for the new socket.
        //
        Transport->ListenSocket = (PWSK_SOCKET)(Irp->IoStatus.Information);
        TransportAddRef(Transport);

        //
        // Reuse the Irp.
        //
        IoReuseIrp(Irp, STATUS_UNSUCCESSFUL);

        //
        // Bind the listening socket.
        //
        Status = TransportSocketBindListeningSocket(
            Transport,
            Irp
            );
    } 
    else 
    {
        //
        // Free the Irp.
        //
        IoFreeIrp(Irp);
    }

    return Status;
}

NTSTATUS
TransportSocketEnableDisconnectEventConnectionSocket(
    __in PTRANSPORT_CONNECTION Connection,
    __in PIRP Irp
    )

/*++

Routine Description:

    Enable disconnect event on the connecting socket.

    This function waits synchronously for the socket operation to complete.
    
Arguments:

    socket - socket context that holds the socket.

    Irp - Irp to use for completion.

Return Value:

    STATUS_SUCCESS if the disconenct callback has been successfully set.
   
--*/

{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    //
    // Event control structure.
    //
    WSK_EVENT_CALLBACK_CONTROL EventCallbackControl;
    
    //
    // Event used for synchronous completion.
    //
    KEVENT CompletionEvent;

    //
    // Initialize the completion event.
    //
    KeInitializeEvent(
        &CompletionEvent, 
        SynchronizationEvent, 
        FALSE
        );

    //
    // Set the completion routine for the Irp.
    //
    IoSetCompletionRoutine(
        Irp,
        TransportSocketSyncIrpCompletionRoutine,
        &CompletionEvent,
        TRUE,
        TRUE,
        TRUE
        );

    //
    // Specify the WSK NPI identifier.
    //
    EventCallbackControl.NpiId = &NPI_WSK_INTERFACE_ID;
    //
    // Set the event flags for the event callback functions that
    // are to be enabled on the socket.
    //
    EventCallbackControl.EventMask = WSK_EVENT_DISCONNECT;

    //
    // Set the socket option to enable accept callback events.
    //
    Status = ((PWSK_PROVIDER_BASIC_DISPATCH)Connection->Socket->Dispatch)->WskControlSocket(
        Connection->Socket,
        WskSetOption,
        SO_WSK_EVENT_CALLBACK,
        SOL_SOCKET,
        sizeof(WSK_EVENT_CALLBACK_CONTROL),
        &EventCallbackControl,
        0,
        NULL,
        NULL,
        Irp
        );

    if (STATUS_PENDING == Status) 
    {
        //
        // Wait for completion or the Irp.
        //
        KeWaitForSingleObject(
            &CompletionEvent, 
            Executive, 
            KernelMode, 
            FALSE, 
            NULL
            );

        //
        // Retrieve the Irp status.
        //
        Status = Irp->IoStatus.Status;
    }

    //
    // Free the Irp.
    //
    IoFreeIrp(Irp);

    return Status;
}

VOID TransportSocketConnectWorkItem(
    __in PVOID IoObject,
    __in PVOID Context,
    __in PIO_WORKITEM WorkItem)
{
    PTRANSPORT_CONNECTION Connection = (PTRANSPORT_CONNECTION)Context;
    PLEASE_TRANSPORT Transport = Connection->Target->Transport;
    NTSTATUS Status = Connection->SendStatus;

    UNREFERENCED_PARAMETER(IoObject);
    UNREFERENCED_PARAMETER(WorkItem);

    ASSERT(Connection->Target != NULL);
    
    TransportConnectComplete(Connection, Status);

    // -1 ref for completing connection operation,
    // +1 ref for socket if it succeeded (net 0)
    //
    if (!NT_SUCCESS(Status)) 
    {
        TransportRelease(Transport);
    }
}

NTSTATUS
TransportSocketConnectCompletionRoutine(
    __in PDEVICE_OBJECT Reserved,
    __in PIRP Irp,
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Context
    )
{
    NTSTATUS status = Irp->IoStatus.Status;
    PTRANSPORT_CONNECTION Connection = (PTRANSPORT_CONNECTION)Context;

    UNREFERENCED_PARAMETER(Reserved);

    //
    // Check status.
    //
    if (NT_SUCCESS(status)) 
    {
        //
        // Save the socket object for the new channel.
        //
        ASSERT(Connection->Socket == NULL);

        //
        // count the connection for the socket
        //
        TransportConnectionAddref(Connection);

        Connection->Socket = (PWSK_SOCKET)(Irp->IoStatus.Information);

        //
        // Reuse the Irp.
        //
        IoReuseIrp(Irp, STATUS_UNSUCCESSFUL);
        
        //
        // Enable disconnect event on the connecting socket.
        //
        status = TransportSocketEnableDisconnectEventConnectionSocket(
            Connection, 
            Irp
            );
    } 
    else 
    {
        //
        // Free the Irp.
        //
        IoFreeIrp(Irp);
    }

    Connection->SendStatus = status;
    ASSERT(Connection->Target != NULL);
    
    IoQueueWorkItemEx(Connection->SendCompleteWorkItem, TransportSocketConnectWorkItem, DelayedWorkQueue, Connection);

    //
    // Always return STATUS_MORE_PROCESSING_REQUIRED to
    // terminate the completion processing of the IRP.
    //
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
TransportSocketCreateConnectionSocket(
    __in PTRANSPORT_CONNECTION Connection
    )
    
/*++

Routine Description:

    Creates a connection oriented socket and connects it remotely.

Arguments:

    LeaseChannelContext - lease channel.

Return Value:

    STATUS_SUCCESS if the connection WSK socket has been successfully created.
   
--*/

{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PSOCKADDR LocalAddress = NULL;

#pragma prefast(push)
#pragma prefast(disable: 24002, "IPv4 and IPv6 branches present")

    //          
    // Local addresses for IPv4 and IPv6.
    //
    SOCKADDR_IN IPv4ConnectAddress = {
        AF_INET, 
        0,
        0, 
        IN6ADDR_ANY_INIT,
        0
        };
    SOCKADDR_IN6 IPv6ConnectAddress = {
        AF_INET6, 
        0,
        0, 
        IN6ADDR_ANY_INIT,
        0
        };

#pragma prefast(pop)

    //
    // Completion Irp.
    //
    PIRP Irp = NULL;

    //
    // Get WSK NPI.
    //
    PWSK_PROVIDER_NPI WskNpi = GetWskProviderNpi();
    
    if (NULL == WskNpi) 
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ASSERT(Connection->ConnectionState == CONNECTIONSTATE_CREATED);
    Connection->ConnectionState = CONNECTIONSTATE_CONNECTING;

    Irp = IoAllocateIrp(1, FALSE);
    
    if (NULL == Irp) 
    {
        EventWriteAllocateFail21(
            NULL
            ); 
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Set local network address using the same family as the remote network address. 
    //
    switch (Connection->Target->PeerAddress.ss_family) {

        case AF_INET:
            LocalAddress = (PSOCKADDR)&IPv4ConnectAddress;
            break;

        case AF_INET6:
            LocalAddress = (PSOCKADDR)&IPv6ConnectAddress;
            break;

        default:
            ASSERT(FALSE); // this was already enforced before getting here
    }

    //
    // Set the completion routine for the Irp.
    //
    IoSetCompletionRoutine(
        Irp,
        TransportSocketConnectCompletionRoutine,
        Connection,
        TRUE,
        TRUE,
        TRUE
        );

    TransportAddRef(Connection->Target->Transport);

    // add a reference for the connection operation
    TransportConnectionAddref(Connection);

    ASSERT(Connection->Target != NULL);
    
    //
    // Initiate create/bind/connect socket.
    //
    Status = WskNpi->Dispatch->WskSocketConnect(
        WskNpi->Client,
        SOCK_STREAM,
        IPPROTO_TCP,
        LocalAddress,
        (PSOCKADDR)&Connection->Target->PeerAddress,
        0,
        Connection,
        &WskTransportConnectionDispatch,
        NULL,
        NULL,
        NULL,
        Irp
        );

    if (!NT_SUCCESS(Status)) 
    {
        TransportRelease(Connection->Target->Transport);
        TransportConnectionShutdown(Connection);
        TransportConnectionRelease(Connection);
        IoFreeIrp(Irp);
    }

    return Status;
}

/*++

Routine Description:

    Gets the remote address associated with a socket.
    
    This function blocks synchronously waiting for the socket operation.
    
Arguments:

    socket - socket to obtain peer address for
    
    Address - storage in which to return address

Return Value:

    Error code

--*/

NTSTATUS TransportGetRemoteAddress(__in PWSK_SOCKET Socket, __out SOCKADDR * Address)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    KEVENT CompletionEvent;
    PIRP Irp = NULL;
    
    Irp = IoAllocateIrp(1, FALSE);
    
    if (NULL == Irp) 
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }

    KeInitializeEvent(
        &CompletionEvent, 
        SynchronizationEvent, 
        FALSE
        );

    IoSetCompletionRoutine(
        Irp,
        TransportSocketSyncIrpCompletionRoutine,
        &CompletionEvent,
        TRUE,
        TRUE,
        TRUE
        );

    status = (*(((PWSK_PROVIDER_CONNECTION_DISPATCH)Socket->Dispatch)->WskGetRemoteAddress))(Socket, Address, Irp);

    if (STATUS_PENDING == status) 
    {
        //
        // Wait for completion or the Irp.
        //
        KeWaitForSingleObject(
            &CompletionEvent, 
            Executive, 
            KernelMode, 
            FALSE, 
            NULL
            );

        //
        // Retrieve the Irp status.
        //
        status = Irp->IoStatus.Status;
    }

Error:
    if (Irp != NULL) 
    {
        IoFreeIrp(Irp);
    }

    return status;
}

LONG TransportConnectionAddref(__in PTRANSPORT_CONNECTION Connection)
{
    return InterlockedIncrement(&Connection->ActivityCount);
}

LONG TransportConnectionRelease(__in PTRANSPORT_CONNECTION Connection)
{
    LONG refCount = InterlockedDecrement(&Connection->ActivityCount);

    ASSERT(refCount >= 0);

    if (refCount == 0)
    {
        ASSERT(Connection->ConnectionState == CONNECTIONSTATE_FAULTED);
        TransportReleaseTarget(Connection->Target);
        TransportConnectionDestructor(Connection);
    }

    return refCount;
}

LONG TransportConnectionReleaseInternal(__in PTRANSPORT_CONNECTION Connection)
{
    LONG refCount = InterlockedDecrement(&Connection->ActivityCount);

    ASSERT(refCount >= 0);

    if (refCount == 0)
    {
        ASSERT(Connection->ConnectionState == CONNECTIONSTATE_FAULTED);
        TransportReleaseTargetInternal(Connection->Target);
        TransportConnectionDestructor(Connection);
    }

    return refCount;
}
