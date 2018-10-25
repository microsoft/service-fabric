/*++

Module Name:

    KAWIpcClient.cpp

Abstract:

    This file contains the implementation for the KAWIpc mechanism.
    This is a cross process IPC mechanism.

Author:

    Alan Warwick 05/2/2017

Environment:

    User mode

Notes:

Revision History:

--*/

#include <ktl.h>
#include <ktrace.h>
#include <KAsyncService.h>
#include <kawipc.h>

#if defined(PLATFORM_UNIX)
#include <palio.h>
#endif

ktl::Awaitable<NTSTATUS> KAWIpcClient::OpenAsync(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt KAsyncGlobalContext* const GlobalContextOverride)
{
    KDbgCheckpointInformational(PtrToActivityId(this), "OpenAsync");

    NTSTATUS status;
    
    OpenAwaiter::SPtr awaiter;
    status = OpenAwaiter::Create(
        GetThisAllocator(),
        GetThisAllocationTag(),
        *this, 
        awaiter,
        ParentAsync,
        GlobalContextOverride);

    if (!NT_SUCCESS(status))
    {
        KDbgErrorWStatus(PtrToActivityId(this), "ktl::kservice::OpenAwaiter::Create", status);
        co_return status;
    }

    co_return co_await *awaiter;
}

VOID KAWIpcClient::OnServiceOpen()
{
    SetDeferredCloseBehavior();
    OpenTask();
}

Task KAWIpcClient::OpenTask()
{
    NTSTATUS status = STATUS_SUCCESS;

    //
    // For safety, keep a ref on this for the lifetime of the whole
    // routine.
    //
    KAWIpcClient::SPtr thisPtr = this;
    
    CompleteOpen(status);
    co_return;
}

ktl::Awaitable<NTSTATUS> KAWIpcClient::CloseAsync()
{
    NTSTATUS status;

    CloseAwaiter::SPtr awaiter;
    status = CloseAwaiter::Create(
        GetThisAllocator(),
        GetThisAllocationTag(),
        *this, 
        awaiter,
        nullptr,
        nullptr);

    if (!NT_SUCCESS(status))
    {
        KDbgErrorWStatus(PtrToActivityId(this), "CloseAwaiter::Create", status);
        co_return status;
    }

    co_return co_await *awaiter;
}

VOID KAWIpcClient::OnServiceClose()
{
    CloseTask();
}

Task KAWIpcClient::CloseTask()
{   
    //
    // For safety, keep a ref on this for the lifetime of the whole
    // routine.
    //
    KAWIpcClient::SPtr thisPtr = this;
    
    co_await RundownAllClientServerConnections();
    CompleteClose(STATUS_SUCCESS);
}

ktl::Awaitable<NTSTATUS> KAWIpcClient::ConnectPipe(
    __in ULONGLONG SocketAddress,
    __in KAWIpcClientServerConnection& Connection,
    __in_opt KAsyncContextBase* const ParentAsync
    )
{
    KCoService$ApiEntry(TRUE);
    
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN pipeConnected = FALSE;
    BOOLEAN connectionStarted = FALSE;
    KAWIpcClientPipe::SPtr pipe = nullptr;
    KAWIpcClientServerConnection::SPtr connection = &Connection;
    
    status = CreateClientPipe(pipe);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, SocketAddress, 0);
        goto done;
    }

    status = co_await pipe->OpenAsync(SocketAddress);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, SocketAddress, 0);
        goto done;
    }
    pipeConnected = TRUE;

    status = co_await connection->OpenAsync(*pipe);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, SocketAddress, 0);
        goto done;
    }
    connectionStarted = TRUE;

    AddClientServerConnection(*connection);

    KDbgCheckpointWDataInformational(0,
                        "KAWIpcClient::ConnectPipe", status,
                        (ULONGLONG)SocketAddress,
                        (ULONGLONG)pipe.RawPtr(),
                        (ULONGLONG)connection.RawPtr(),
                        0);

    
done:
    if (! NT_SUCCESS(status))
    {
        NTSTATUS statusDontCare;

        if (pipeConnected)
        {           
            statusDontCare = co_await pipe->CloseAsync();
            if (! NT_SUCCESS(statusDontCare))
            {
                KTraceFailedAsyncRequest(statusDontCare, this, SocketAddress, 0);
            }
        }
        
        if (connectionStarted)
        {
            statusDontCare = co_await connection->CloseAsync();
            if (! NT_SUCCESS(statusDontCare))
            {
                KTraceFailedAsyncRequest(statusDontCare, this, SocketAddress, 0);
            }
            connectionStarted = FALSE;
        }       
    }
    
    co_return status;
}


ktl::Awaitable<NTSTATUS> KAWIpcClient::DisconnectPipe(
    __in KAWIpcClientServerConnection& Connection,
    __in_opt KAsyncContextBase* const ParentAsync
    )
{
    KCoService$ApiEntry(TRUE);
    KAWIpcClientServerConnection::SPtr connection = &Connection;
    co_return co_await DisconnectPipeInternal(Connection, TRUE, ParentAsync);
}

ktl::Awaitable<NTSTATUS> KAWIpcClient::DisconnectPipeInternal(
    __in KAWIpcClientServerConnection& Connection,
    __in BOOLEAN RemoveFromList,
    __in_opt KAsyncContextBase* const ParentAsync
    )
{
    //
    // Do not take a service activity since this runs during client
    // shutdown
    //
    
    NTSTATUS status = STATUS_SUCCESS;
    KAWIpcClientPipe::SPtr pipe = nullptr;
    KAWIpcClientServerConnection::SPtr connection = &Connection;

    if (RemoveFromList)
    {
        status = RemoveClientServerConnection(*connection);
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, (ULONGLONG)(Connection.GetClientPipe()->GetSocketHandleFd().Get()), (ULONGLONG)&Connection);
            co_return status;
        }
    }

    pipe = connection->GetClientPipe(); 
    status = co_await pipe->CloseAsync();

    KDbgCheckpointWDataInformational(0,
                        "ClosedPipe", status,
                        (RemoveFromList ? 1 : 0),
                        (ULONGLONG)(pipe->GetSocketHandleFd().Get()),
                        (ULONGLONG)pipe.RawPtr(),
                        (ULONGLONG)connection);
    
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
    }

    status = co_await connection->CloseAsync();
    KDbgCheckpointWDataInformational(0,
                        "ClosedConnection", status,
                        (RemoveFromList ? 1 : 0),
                        connection->GetClientPipe() ? (ULONGLONG)(connection->GetClientPipe()->GetSocketHandleFd().Get()) : -1,
                        (ULONGLONG)connection->GetClientPipe(),
                        (ULONGLONG)connection);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
    }

    co_return STATUS_SUCCESS;
}


const ULONG KAWIpcClient::_ConnectionLinkageOffset = FIELD_OFFSET(KAWIpcClientServerConnection,
                                                                 _ListEntry);

VOID KAWIpcClient::AddClientServerConnection(
    __in KAWIpcClientServerConnection& Connection
    )
{
    //
    // Take ref count when adding to the list. The ref count will be
    // removed when the item is removed from the list.
    //
    Connection.AddRef();
    
    K_LOCK_BLOCK(_ConnectionsListLock)
    {
        _ConnectionsList.AppendTail(&Connection);
    }

}

NTSTATUS KAWIpcClient::RemoveClientServerConnection(
    __in KAWIpcClientServerConnection& Connection
    )
{
    KAWIpcClientServerConnection* connection = nullptr;
    
    K_LOCK_BLOCK(_ConnectionsListLock)
    {
        connection = _ConnectionsList.Remove(&Connection);
    }
    if (connection == nullptr)
    {
        KTraceFailedAsyncRequest(STATUS_NOT_FOUND,
                                 this,
                                 (ULONGLONG)(Connection.GetClientPipe()->GetSocketHandleFd().Get()),
                                 (ULONGLONG)&Connection);
        return(STATUS_NOT_FOUND);
    }

    //
    // Release ref count taken when added to list
    //
    Connection.Release();
    return(STATUS_SUCCESS);
}

ktl::Awaitable<VOID> KAWIpcClient::RundownAllClientServerConnections(
    __in_opt KAsyncContextBase* const ParentAsync
    )
{
    NTSTATUS statusDontCare = STATUS_SUCCESS;
    
    KAWIpcClientServerConnection* connection = nullptr;

    do
    {
        K_LOCK_BLOCK(_ConnectionsListLock)
        {
            connection = _ConnectionsList.RemoveHead();
        }

        if (connection)
        {
            statusDontCare = co_await DisconnectPipeInternal(*connection, FALSE, ParentAsync);            

            //
            // Release ref count taken when added to list
            //
            connection->Release();      
        }
    } while (connection != nullptr);        
}


KAWIpcClient::KAWIpcClient() :
   _ConnectionsList(_ConnectionLinkageOffset)
{
}

KAWIpcClient::~KAWIpcClient()
{
}

NTSTATUS KAWIpcClient::Create(
    __in KAllocator& Allocator,
    __in ULONG AllocationTag,
    __out KAWIpcClient::SPtr& Context
)
{
    KAWIpcClient::SPtr context;
    
    context = _new(AllocationTag, Allocator) KAWIpcClient();
    if (! context)
    {
        KTraceOutOfMemory(0, STATUS_INSUFFICIENT_RESOURCES, nullptr, 0, 0);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    if (! NT_SUCCESS(context->Status()))
    {
        return(context->Status());
    }

    Context = Ktl::Move(context);
    return(STATUS_SUCCESS); 
}

NTSTATUS KAWIpcClient::CreateClientPipe(
    __out KAWIpcClientPipe::SPtr& Context
)
{
    KAWIpcClientPipe::SPtr context;
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) KAWIpcClientPipe(*this);
    if (! context)
    {
        KTraceOutOfMemory(0, STATUS_INSUFFICIENT_RESOURCES, nullptr, 0, 0);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    if (! NT_SUCCESS(context->Status()))
    {
        return(context->Status());
    }

    Context = Ktl::Move(context);
    return(STATUS_SUCCESS); 
}

NTSTATUS KAWIpcClient::CreateClientServerConnection(
    __out KAWIpcClientServerConnection::SPtr& Context
)
{
    KService$ApiEntry(TRUE);
    
    KAWIpcClientServerConnection::SPtr context;
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) KAWIpcClientServerConnection();
    if (! context)
    {
        KTraceOutOfMemory(0, STATUS_INSUFFICIENT_RESOURCES, nullptr, 0, 0);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    if (! NT_SUCCESS(context->Status()))
    {
        return(context->Status());
    }

    Context = Ktl::Move(context);
    return(STATUS_SUCCESS); 
}

