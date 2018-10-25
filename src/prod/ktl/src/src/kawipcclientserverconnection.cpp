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

ktl::Awaitable<NTSTATUS> KAWIpcClientServerConnection::OpenAsync(
    __in KAWIpcClientPipe& ClientPipe,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt KAsyncGlobalContext* const GlobalContextOverride)
{
    NTSTATUS status;
    KAWIpcClientPipe::SPtr clientPipe = &ClientPipe;
    
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

    _ClientPipe = &ClientPipe;

    co_return co_await *awaiter;
}

VOID KAWIpcClientServerConnection::OnServiceOpen()
{
    SetDeferredCloseBehavior();
    OpenTask();
}

Task KAWIpcClientServerConnection::OpenTask()
{
    NTSTATUS status = STATUS_SUCCESS;

    //
    // For safety, keep a ref on this for the lifetime of the whole
    // routine.
    //
    KAWIpcClientServerConnection::SPtr thisPtr = this;
    
    KDbgCheckpointWDataInformational(0, "KAWIpcClientServerConnection::OpenTask",
                                     STATUS_SUCCESS,
                                     (ULONGLONG)this,
                                     (ULONGLONG)_ClientPipe->GetSocketHandleFd().Get(),
                                     (ULONGLONG)GetClientPipe(),
                                     (ULONGLONG)this);

    CompleteOpen(status);
    co_return;
}

ktl::Awaitable<NTSTATUS> KAWIpcClientServerConnection::CloseAsync()
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

VOID KAWIpcClientServerConnection::OnServiceClose()
{
    CloseTask();
}

Task KAWIpcClientServerConnection::CloseTask()
{
    //
    // For safety, keep a ref on this for the lifetime of the whole
    // routine.
    //
    KAWIpcClientServerConnection::SPtr thisPtr = this;
    
    KDbgCheckpointWDataInformational(0, "KAWIpcClientServerConnection::CloseTask",
                                     STATUS_SUCCESS,
                                     (ULONGLONG)this,
                                     (ULONGLONG)_ClientPipe->GetSocketHandleFd().Get(),
                                     (ULONGLONG)GetClientPipe(),
                                     (ULONGLONG)this);

    CompleteClose(STATUS_SUCCESS);
    co_return;
}

KAWIpcClientServerConnection::KAWIpcClientServerConnection()
{
}

KAWIpcClientServerConnection::~KAWIpcClientServerConnection()
{
}

