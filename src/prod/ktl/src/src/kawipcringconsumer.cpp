/*++

Module Name:

    KAWIpcRingConsumer.cpp

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

KAWIpcRingConsumer::KAWIpcRingConsumer()
{
}

KAWIpcRingConsumer::~KAWIpcRingConsumer()
{
}

NTSTATUS KAWIpcRingConsumer::Create(
    __in KAllocator& Allocator,
    __in ULONG AllocatorTag,
    __out KAWIpcRingConsumer::SPtr& Context
    )
{
    KAWIpcRingConsumer::SPtr context;
    
    context = _new(AllocatorTag, Allocator) KAWIpcRingConsumer();
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

ktl::Awaitable<NTSTATUS> KAWIpcRingConsumer::OpenAsync(
    __in KAWIpcRing& Ring,
    __in EVENTHANDLEFD EventHandleFd,
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

    _Ring = &Ring;
    _EventHandleFd = EventHandleFd;

    co_return co_await *awaiter;
}

VOID KAWIpcRingConsumer::OnServiceOpen()
{
    SetDeferredCloseBehavior();
    OpenTask();
}

Task KAWIpcRingConsumer::OpenTask()
{
    NTSTATUS status = STATUS_SUCCESS;

    //
    // For safety, keep a ref on this for the lifetime of the whole
    // routine.
    //
    KAWIpcRingConsumer::SPtr thisPtr = this;

    status = KAWIpcEventWaiter::Create(GetThisAllocator(),
                                       GetThisAllocationTag(),
                                       _EventWaiter);
    
    
    KDbgCheckpointWDataInformational(0,
                        "KAWIpcRingConsumer::OpenTask", status,
                        (ULONGLONG)this,
                        (ULONGLONG)_Ring.RawPtr(),
                        (ULONGLONG)_EventWaiter.RawPtr(),
                        (ULONGLONG)_EventHandleFd.Get());

    CompleteOpen(status);   
    co_return;
}

ktl::Awaitable<NTSTATUS> KAWIpcRingConsumer::CloseAsync()
{
    NTSTATUS status;

    //
    // Ring must be locked for close and drained before closing
    //
    KInvariant(_Ring->IsLockedForClose());
    KInvariant(_Ring->IsEmpty());
    
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

VOID KAWIpcRingConsumer::OnDeferredClosing()
{
    _EventWaiter->Cancel();
}

VOID KAWIpcRingConsumer::OnServiceClose()
{
    CloseTask();
}

Task KAWIpcRingConsumer::CloseTask()
{
    //
    // For safety, keep a ref on this for the lifetime of the whole
    // routine.
    //
    KAWIpcRingConsumer::SPtr thisPtr = this;
    
    KDbgCheckpointWDataInformational(0,
                        "KAWIpcRingConsumer::CloseTask", STATUS_SUCCESS,
                        (ULONGLONG)this,
                        (ULONGLONG)_Ring.RawPtr(),
                        (ULONGLONG)_EventWaiter.RawPtr(),
                        (ULONGLONG)_EventHandleFd.Get());
    
    CompleteClose(STATUS_SUCCESS);
    co_return;
}

ktl::Awaitable<NTSTATUS> KAWIpcRingConsumer::ConsumeAsync(
    __out KAWIpcMessage*& Message
    )
{
    KCoService$ApiEntry(TRUE);
    KAWIpcRingConsumer::SPtr thisPtr = this;
    
    NTSTATUS status = STATUS_SUCCESS;
        
    Message = nullptr;
    do
    {
        status = _Ring->Dequeue(Message);
        if (status == STATUS_NOT_FOUND)
        {
            KDbgCheckpointWDataInformational(0,
                                "RingConsumerWait", STATUS_SUCCESS,
                                (ULONGLONG)this,
                                (ULONGLONG)_Ring.RawPtr(),
                                (ULONGLONG)0,
                                (ULONGLONG)_EventHandleFd.Get());
            if (! IsLockedForClose())
            {
                status = co_await _EventWaiter->WaitAsync(_EventHandleFd, nullptr);
                if (! NT_SUCCESS(status))
                {
                    KTraceFailedAsyncRequest(status, nullptr, (ULONGLONG)this, (ULONGLONG)_EventHandleFd.Get());
                    break;
                }
            } else {
                //
                // If the consumer ring is shutting down then go ahead
                // and return without waiting
                //
                status = K_STATUS_API_CLOSED;
                KTraceFailedAsyncRequest(status, nullptr, (ULONGLONG)this, (ULONGLONG)_EventHandleFd.Get());
                break;
            }
        } else if (! NT_SUCCESS(status)) {
            KTraceFailedAsyncRequest(status, nullptr, (ULONGLONG)this, (ULONGLONG)_EventHandleFd.Get());
            co_return status;
        }
    } while (Message == nullptr);
    
    co_return status;
}
