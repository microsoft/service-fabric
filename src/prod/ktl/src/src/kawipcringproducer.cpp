/*++

Module Name:

    KAWIpcRingProducer.cpp

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

KAWIpcRingProducer::KAWIpcRingProducer()
{
}

KAWIpcRingProducer::~KAWIpcRingProducer()
{
}

NTSTATUS KAWIpcRingProducer::Create(
    __in KAllocator& Allocator,
    __in ULONG AllocatorTag,
    __out KAWIpcRingProducer::SPtr& Context
    )
{
    KAWIpcRingProducer::SPtr context;
    
    context = _new(AllocatorTag, Allocator) KAWIpcRingProducer();
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


ktl::Awaitable<NTSTATUS> KAWIpcRingProducer::OpenAsync(
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

VOID KAWIpcRingProducer::OnServiceOpen()
{
    SetDeferredCloseBehavior();
    OpenTask();
}

Task KAWIpcRingProducer::OpenTask()
{
    NTSTATUS status = STATUS_SUCCESS;

    //
    // For safety, keep a ref on this for the lifetime of the whole
    // routine.
    //
    KAWIpcRingProducer::SPtr thisPtr = this;

    _IsCancelled = FALSE;
    
    status = KAWIpcEventKicker::Create(GetThisAllocator(),
                                       GetThisAllocationTag(),
                                       _EventHandleFd,
                                       _EventKicker);

    if (NT_SUCCESS(status))
    {
        status = KTimer::Create(_ThrottleTimer, GetThisAllocator(), GetThisAllocationTag());
    }

    
    KDbgCheckpointWDataInformational(0,
                        "KAWIpcRingProducer::OpenTask", status,
                        (ULONGLONG)this,
                        (ULONGLONG)_Ring.RawPtr(),
                        (ULONGLONG)_EventKicker.RawPtr(),
                        (ULONGLONG)_EventHandleFd.Get());

    CompleteOpen(status);   
    co_return;
}

ktl::Awaitable<NTSTATUS> KAWIpcRingProducer::CloseAsync()
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

VOID KAWIpcRingProducer::OnDeferredClosing()
{
    _IsCancelled = TRUE;
}

VOID KAWIpcRingProducer::OnServiceClose()
{
    CloseTask();
}

Task KAWIpcRingProducer::CloseTask()
{
    //
    // For safety, keep a ref on this for the lifetime of the whole
    // routine.
    //
    KAWIpcRingProducer::SPtr thisPtr = this;
    
    KDbgCheckpointWDataInformational(0,
                        "KAWIpcRingProducer::CloseTask", STATUS_SUCCESS,
                        (ULONGLONG)this,
                        (ULONGLONG)_Ring.RawPtr(),
                        (ULONGLONG)_EventKicker.RawPtr(),
                        (ULONGLONG)_EventHandleFd.Get());
    
    CompleteClose(STATUS_SUCCESS);
    co_return;
}

ktl::Awaitable<NTSTATUS> KAWIpcRingProducer::ProduceAsync(
    __in KAWIpcMessage* Message
)
{
    KCoService$ApiEntry(TRUE);

    KAWIpcRingProducer::SPtr thisPtr = this;
    
    static const ULONG startThrottleDelayInMs = 1;
    static const ULONG maxThrottleDelayInMs = 256;
    ULONG throttleDelayInMs = startThrottleDelayInMs;
    NTSTATUS status = STATUS_SUCCESS;
    NTSTATUS statusDontCare;
    BOOLEAN wasEmpty;

    do
    {
        status = _Ring->Enqueue(Message, wasEmpty);
        if (status == STATUS_ARRAY_BOUNDS_EXCEEDED) {
            //
            // Ring is full so we need to first check if the producer is shutting down and
            // then wait for space to free up. We
            // do not have a notification for this; Consider adding
            // one. For now, just wait a ms and try again. Note that
            // this is a natural throttle point
            //
            if (_IsCancelled)
            {
                status = STATUS_CANCELLED;
                break;
            }

            _ThrottleTimer->Reuse();
            statusDontCare = co_await _ThrottleTimer->StartTimerAsync(throttleDelayInMs, nullptr, nullptr);
            if (throttleDelayInMs > maxThrottleDelayInMs)
            {
                throttleDelayInMs = maxThrottleDelayInMs;
            }
        } else if (! NT_SUCCESS(status)) {
            //
            // Most likely the ring is shutdown by the consumer and so
            // we fail back
            //
            KTraceFailedAsyncRequest(status, nullptr, (ULONGLONG)this, (ULONGLONG)_EventHandleFd.Get());
            break;
        }
    } while (! NT_SUCCESS(status));

    //
    // See if we need to awaken the consumer
    //
    if ((NT_SUCCESS(status) && (wasEmpty)))
    {
        KDbgCheckpointWDataInformational(0,
                            "Kick", STATUS_SUCCESS,
                            (ULONGLONG)this,
                            (ULONGLONG)_Ring.RawPtr(),
                            (ULONGLONG)_EventKicker.RawPtr(),
                            (ULONGLONG)_EventHandleFd.Get());
        status = _EventKicker->Kick();
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, nullptr, (ULONGLONG)this, (ULONGLONG)_EventHandleFd.Get());
            //
            // TODO: Not sure what to do here
            //
        }
    }

    co_return status;
}
        
