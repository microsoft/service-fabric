/*++

Module Name:

    KAWIpcServerClientConnection.cpp

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

ktl::Awaitable<NTSTATUS> KAWIpcServerClientConnection::OpenAsync(
    __in KAWIpcServerPipe& ServerPipe,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt KAsyncGlobalContext* const GlobalContextOverride)
{
    NTSTATUS status;
    KAWIpcServerPipe::SPtr serverPipe = &ServerPipe;
    
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

    _ServerPipe = serverPipe;

    co_return co_await *awaiter;
}

VOID KAWIpcServerClientConnection::OnServiceOpen()
{
    SetDeferredCloseBehavior();
    OpenTask();
}

Task KAWIpcServerClientConnection::OpenTask()
{
    NTSTATUS status = STATUS_SUCCESS;

    //
    // For safety, keep a ref on this for the lifetime of the whole
    // routine.
    //
    KAWIpcServerClientConnection::SPtr thisPtr = this;
    
    CompleteOpen(status);
    co_return;
}

ktl::Awaitable<NTSTATUS> KAWIpcServerClientConnection::CloseAsync()
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

VOID KAWIpcServerClientConnection::OnServiceClose()
{
    CloseTask();
}

Task KAWIpcServerClientConnection::CloseTask()
{
    NTSTATUS statusDontCare;
    KAWIpcServerPipe::SPtr pipe;

    //
    // For safety, keep a ref on this for the lifetime of the whole
    // routine.
    //
    KAWIpcServerClientConnection::SPtr thisPtr = this;
    
    pipe = _ServerPipe;
    statusDontCare = co_await pipe->CloseAsync();
    if (! NT_SUCCESS(statusDontCare))
    {
        KTraceFailedAsyncRequest(statusDontCare, this, 0, 0);
        // TODO: What else can we do ?
    }
    
    CompleteClose(STATUS_SUCCESS);
}

KAWIpcServerClientConnection::KAWIpcServerClientConnection()
{
}

KAWIpcServerClientConnection::~KAWIpcServerClientConnection()
{
}

