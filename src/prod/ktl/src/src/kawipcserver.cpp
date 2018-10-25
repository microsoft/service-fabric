/*++

Module Name:

    KAWIpcServer.cpp

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
#include <kawipc.h>

#if defined(PLATFORM_UNIX)
#include <palio.h>
#endif

ktl::Awaitable<NTSTATUS> KAWIpcServer::OpenAsync(
    __in ULONGLONG SocketAddress,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt KAsyncGlobalContext* const GlobalContextOverride)
{
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
        KDbgErrorWStatus(PtrToActivityId(this), "OpenAwaiter::Create", status);
        co_return status;
    }

    _SocketAddress = SocketAddress;

    co_return co_await *awaiter;
}

VOID KAWIpcServer::OnServiceOpen()
{
    SetDeferredCloseBehavior();
    OpenTask();
}

Task KAWIpcServer::OpenTask()
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN isPipeStarted = FALSE;

    //
    // For safety, keep a ref on this for the lifetime of the whole
    // routine.
    //
    KAWIpcServer::SPtr thisPtr = this;
    
    //
    // Create a pipe endpoint and start it up
    //
    status = CreateServerPipeEndpoint(_PipeEndpoint);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, _SocketAddress, 0);
        goto done;
    }

    status = co_await _PipeEndpoint->OpenAsync(*this, _SocketAddress);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, _SocketAddress, 0);
        goto done;
    }
    isPipeStarted = TRUE;

    KDbgCheckpointWDataInformational(0,
                        "KAWIpcServer::OpenTask", status,
                        (ULONGLONG)this,
                        _SocketAddress,
                        (ULONGLONG)_PipeEndpoint.RawPtr(),
                        0); 
done:
    //
    // Cleanup on error condition
    //
    if (! NT_SUCCESS(status))
    {
        if ((_PipeEndpoint) && (isPipeStarted))
        {
             co_await _PipeEndpoint->CloseAsync();              
        }
    }

    CompleteOpen(status);   
}

ktl::Awaitable<NTSTATUS> KAWIpcServer::CloseAsync()
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

VOID KAWIpcServer::OnServiceClose()
{
    CloseTask();
}

Task KAWIpcServer::CloseTask()
{
    NTSTATUS status;

    //
    // For safety, keep a ref on this for the lifetime of the whole
    // routine.
    //
    KAWIpcServer::SPtr thisPtr = this;
    
    co_await RundownAllServerClientConnections();
    
    status = co_await _PipeEndpoint->CloseAsync();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, _SocketAddress, (ULONGLONG)_PipeEndpoint.RawPtr());
    }

    KDbgCheckpointWDataInformational(0,
                        "KAWIpcServer::CloseTask", status,
                        (ULONGLONG)this,
                        _SocketAddress,
                        (ULONGLONG)_PipeEndpoint.RawPtr(),
                        0);
    
    CompleteClose(STATUS_SUCCESS);
}

ktl::Awaitable<NTSTATUS> KAWIpcServer::OnIncomingConnection(
    __in KAWIpcServerPipe& Pipe,
    __out KAWIpcServerClientConnection::SPtr& PerClientConnection
    )
{
    KCoService$ApiEntry(TRUE);
    
    NTSTATUS status;
    KAWIpcServerClientConnection::SPtr connection;
    KAWIpcServerPipe::SPtr pipe = &Pipe;
    
    //
    // Default incoming connection handler. Typically the application
    // would override KAWIpcServer and replace this with its own
    // routine
    //
    status = CreateServerClientConnection(connection);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        co_return status;
    }
    
    status = co_await connection->OpenAsync(*pipe);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        co_return status;
    }

    PerClientConnection = Ktl::Move(connection);
    co_return status;
}


NTSTATUS KAWIpcServer::CreateServerPipeEndpoint(
    __out KAWIpcServerPipeEndpoint::SPtr& Context
)
{
    KAWIpcServerPipeEndpoint::SPtr context;
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) KAWIpcServerPipeEndpoint();
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

NTSTATUS KAWIpcServer::CreateServerClientConnection(
    __out KSharedPtr<KAWIpcServerClientConnection>& Context
)
{
    KService$ApiEntry(TRUE);
    
    KAWIpcServerClientConnection::SPtr context;
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) KAWIpcServerClientConnection();
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

const ULONG KAWIpcServer::_ConnectionLinkageOffset = FIELD_OFFSET(KAWIpcServerClientConnection,
                                                                 _ListEntry);

VOID KAWIpcServer::AddServerClientConnection(
    __in KAWIpcServerClientConnection& Connection
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

NTSTATUS KAWIpcServer::RemoveServerClientConnection(
    __in KAWIpcServerClientConnection& Connection
    )
{
    KAWIpcServerClientConnection::SPtr connection = nullptr;
    
    K_LOCK_BLOCK(_ConnectionsListLock)
    {
        connection = _ConnectionsList.Remove(&Connection);
    }
    
    if (! connection)
    {
        return(STATUS_NOT_FOUND);
    }
    
    //
    // Release ref count taken when added to list
    //
    Connection.Release();
    
    return(STATUS_SUCCESS);
}

ktl::Awaitable<NTSTATUS> KAWIpcServer::ShutdownServerClientConnection(
    __in KAWIpcServerClientConnection& Connection,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt KAsyncGlobalContext* const GlobalContextOverride
    )
{
    KCoService$ApiEntry(TRUE);
    
    NTSTATUS status;
    KAWIpcServerClientConnection::SPtr connection = &Connection;
    SOCKETHANDLEFD socketHandleFd;
    KAWIpcServerPipe::SPtr pipe;

    pipe = connection->GetServerPipe();
    socketHandleFd = (pipe ? pipe->GetSocketHandleFd() : (SOCKETHANDLEFD)0);

    status = RemoveServerClientConnection(*connection);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, (ULONGLONG)connection, 0);
        co_return status;
    }
    
    status = co_await connection->CloseAsync();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
    }

    KDbgCheckpointWDataInformational(0,
                        "KAWIpcServer::ShutdownServerClientConnetion", status,
                        (ULONGLONG)this,
                        _SocketAddress,
                        (ULONGLONG)connection.RawPtr(),
                        (ULONGLONG)socketHandleFd.Get());
    
    co_return status;
}


ktl::Awaitable<VOID> KAWIpcServer::RundownAllServerClientConnections()
{
    NTSTATUS statusDontCare;

    KAWIpcServerClientConnection::SPtr connection = nullptr;

    do
    {
        K_LOCK_BLOCK(_ConnectionsListLock)
        {
            connection = _ConnectionsList.RemoveHead();
        }
        
        if (connection)
        {
            SOCKETHANDLEFD socketHandleFd;
            KAWIpcServerPipe::SPtr pipe;

            pipe = connection->GetServerPipe();
            socketHandleFd = (pipe ? pipe->GetSocketHandleFd() : (SOCKETHANDLEFD)0);
            
            KDbgCheckpointWDataInformational(0,
                                "KAWIpcServer::ShutdownServerClientConnetion", STATUS_SUCCESS,
                                (ULONGLONG)this,
                                _SocketAddress,
                                (ULONGLONG)connection.RawPtr(),
                                (ULONGLONG)socketHandleFd.Get());
                                             
            statusDontCare = co_await connection->CloseAsync();
            if (! NT_SUCCESS(statusDontCare))
            {
                KTraceFailedAsyncRequest(statusDontCare, this, 0, 0);
                // TODO: What else can we do ?
            }
            
            //
            // Release ref count taken when added to list
            //
            connection->Release();      
        }
    } while (connection);        
}
        

KAWIpcServer::KAWIpcServer() :
   _ConnectionsList(_ConnectionLinkageOffset)
{
}

KAWIpcServer::~KAWIpcServer()
{
    KInvariant(_ConnectionsList.IsEmpty());
}

NTSTATUS KAWIpcServer::Create(
    __in KAllocator& Allocator,
    __in ULONG AllocationTag,
    __out KAWIpcServer::SPtr& Context
)
{
    KAWIpcServer::SPtr context;
    
    context = _new(AllocationTag, Allocator) KAWIpcServer();
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

