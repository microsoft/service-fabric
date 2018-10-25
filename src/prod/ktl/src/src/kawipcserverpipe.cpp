/*++

Module Name:

    KAWIpcServerPipe.cpp

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

ktl::Awaitable<NTSTATUS> KAWIpcServerPipe::OpenAsync(
    __in KAWIpcServer& Server,
    __in SOCKETHANDLEFD SocketHandleFd,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt KAsyncGlobalContext* const GlobalContextOverride)
{
    NTSTATUS status;
    KAWIpcServer::SPtr server = &Server;
    
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

    _Server = server;
    _SocketHandleFd = SocketHandleFd;

    co_return co_await *awaiter;
}

VOID KAWIpcServerPipe::OnServiceOpen()
{
    SetDeferredCloseBehavior();
    OpenTask();
}

Task KAWIpcServerPipe::OpenTask()
{
    NTSTATUS status = STATUS_SUCCESS;

    //
    // For safety, keep a ref on this for the lifetime of the whole
    // routine.
    //
    KAWIpcServerPipe::SPtr thisPtr = this;
    
    KDbgCheckpointWDataInformational(0,
                        "KAWIpcServerPipe::OpenTask", status,
                        (ULONGLONG)this,
                        (ULONGLONG)_SocketHandleFd.Get(),
                        0,
                        0);

    CompleteOpen(status);   
    co_return;
}

ktl::Awaitable<NTSTATUS> KAWIpcServerPipe::CloseAsync()
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

VOID KAWIpcServerPipe::OnDeferredClosing()
{
    RundownAllOperations();
}

VOID KAWIpcServerPipe::OnServiceClose()
{
    CloseTask();
}

Task KAWIpcServerPipe::CloseTask()
{
    //
    // For safety, keep a ref on this for the lifetime of the whole
    // routine.
    //
    KAWIpcServerPipe::SPtr thisPtr = this;
    
    KDbgCheckpointWDataInformational(0,
                        "KAWIpcServerPipe::CloseTask", STATUS_SUCCESS,
                        (ULONGLONG)this,
                        (ULONGLONG)_SocketHandleFd.Get(),
                        0,
                        0);

    if (_SocketHandleFd != (SOCKETHANDLEFD)0)
    {
        CloseSocketHandleFd(_SocketHandleFd);
        _SocketHandleFd = (SOCKETHANDLEFD)0;
    }

    _Server = nullptr;
    
    CompleteClose(STATUS_SUCCESS);
    co_return;
}

KAWIpcServerPipe::KAWIpcServerPipe()
{
}

KAWIpcServerPipe::~KAWIpcServerPipe()
{
}

