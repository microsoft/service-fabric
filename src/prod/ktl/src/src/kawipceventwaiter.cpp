/*++

Module Name:

    KAWIpc.cpp

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
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/eventfd.h>
#include <palio.h>
#endif

NTSTATUS KAWIpcEventWaiter::CreateEventWaiter(
    __in_opt PWCHAR Name,
    __out EVENTHANDLEFD& EventHandleFd
    )
{
#if defined(PLATFORM_UNIX)
    UNREFERENCED_PARAMETER(Name);
#endif

    NTSTATUS status = STATUS_SUCCESS;
    
#if !defined(PLATFORM_UNIX)
    EventHandleFd = CreateEvent(NULL,
                    FALSE,
                    FALSE,
                    Name);
    if (EventHandleFd == NULL)
    {
//      DWORD error = GetLastError();
        status = STATUS_UNSUCCESSFUL;
    }
#else
    EventHandleFd = eventfd(0, EFD_CLOEXEC);
    if (EventHandleFd == -1)
    {
        status = LinuxError::LinuxErrorToNTStatus(EventHandleFd.Get());
    }
#endif

    return(status);
}

NTSTATUS KAWIpcEventWaiter::CleanupEventWaiter(
    __in EVENTHANDLEFD EventHandleFd
    )
{
#if !defined(PLATFORM_UNIX)
    CloseHandle(EventHandleFd.Get());
#else
    close(EventHandleFd.Get());
#endif

    return(STATUS_SUCCESS);
}


KAWIpcEventWaiter::KAWIpcEventWaiter()
{
    OnReuse();
}

KAWIpcEventWaiter::~KAWIpcEventWaiter()
{
}

NTSTATUS KAWIpcEventWaiter::Create(
    __in KAllocator& Allocator,
    __in ULONG AllocationTag,
    __out KAWIpcEventWaiter::SPtr& Context
)
{
    KAWIpcEventWaiter::SPtr context;
    
    context = _new(AllocationTag, Allocator) KAWIpcEventWaiter();
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

ktl::Awaitable<NTSTATUS> KAWIpcEventWaiter::WaitAsync(
    __in EVENTHANDLEFD EventHandleFd,
    __in_opt KAsyncContextBase* const ParentAsyncContext

    )
{   
    NTSTATUS status = STATUS_SUCCESS;

    Reuse();
    
    _EventHandleFd = EventHandleFd;
    _Cancel = FALSE;
    _LastError = 0;

    StartAwaiter::SPtr awaiter;

    status = StartAwaiter::Create(
        GetThisAllocator(),
        GetThisAllocationTag(),
        *this,
        awaiter,
        ParentAsyncContext,
        nullptr);               // GlobalContext
    
    if (! NT_SUCCESS(status))
    {
        co_return status ;
    }

    status = co_await *awaiter;
    co_return status;   
}

VOID KAWIpcEventWaiter::Execute(
    )
{
    //
    // TODO: Replace blocking thread with syncfile support when KXM is
    //       ready.
    //
    
    NTSTATUS status;

    //
    // For safety, keep a ref on this for the lifetime of the whole
    // routine.
    //
    KAWIpcEventWaiter::SPtr thisPtr = this;
    
#if defined(PLATFORM_UNIX)
    int err;
    int epollFileDescriptor;
    struct epoll_event evnt = {0};
    struct epoll_event evnts[1];

    epollFileDescriptor = epoll_create1(EPOLL_CLOEXEC);
    if (epollFileDescriptor == -1)
    {
        _LastError = errno;
        status = LinuxError::LinuxErrorToNTStatus(_LastError);
        KTraceFailedAsyncRequest(status, nullptr, (ULONGLONG)_LastError, (ULONGLONG)_EventHandleFd.Get());
        Complete(status);
        return;
    }
    KFinally([&](){  close(epollFileDescriptor); });
        
    evnt.data.fd = _EventHandleFd.Get();
    evnt.events = EPOLLIN;
    err = epoll_ctl(epollFileDescriptor, EPOLL_CTL_ADD, _EventHandleFd.Get(), &evnt);
    if (err == -1)
    {
        _LastError = errno;
        status = LinuxError::LinuxErrorToNTStatus(_LastError);
        KTraceFailedAsyncRequest(status, nullptr, (ULONGLONG)_LastError, (ULONGLONG)_EventHandleFd.Get());
        Complete(status);
        return;
    }
#endif
    
    while (TRUE)
    {
        if (_Cancel)
        {
            Complete(STATUS_CANCELLED);
            return;
        }

#if !defined(PLATFORM_UNIX)
        status = WaitForSingleObject(_EventHandleFd.Get(), _PollIntervalInMs);
        if (status == WAIT_OBJECT_0)
        {
            Complete(STATUS_SUCCESS);
            return;
        }

        if (status != STATUS_TIMEOUT)
        {
            _LastError = GetLastError();
            Complete(STATUS_UNSUCCESSFUL);
            return;
        }
#else       
        err = epoll_wait(epollFileDescriptor, evnts, 1, _PollIntervalInMs);
        if (err == 0)
        {
            continue;
        }
        
        if (err == -1)
        {
            _LastError = errno;
            if (_LastError == EINTR)
            {
                continue;
            }
            status = LinuxError::LinuxErrorToNTStatus(_LastError);
            KTraceFailedAsyncRequest(status, nullptr, (ULONGLONG)_LastError, (ULONGLONG)_EventHandleFd.Get());
            Complete(status);
            return;
        }

        eventfd_t val;
        err = eventfd_read(_EventHandleFd.Get(), &val);
        if (err == -1)
        {
            _LastError = errno;
            status = LinuxError::LinuxErrorToNTStatus(_LastError);
            KTraceFailedAsyncRequest(status, nullptr, (ULONGLONG)_LastError, (ULONGLONG)_EventHandleFd.Get());
            Complete(status);
            return;
        }
        
        Complete(STATUS_SUCCESS);
        return;
#endif

    }
}

VOID KAWIpcEventWaiter::OnReuse(
    )
{
    _EventHandleFd = 0;
    _Cancel = FALSE;
}

VOID KAWIpcEventWaiter::OnCancel(
    )
{
    _Cancel = TRUE;
}


VOID KAWIpcEventWaiter::StartWait(
    __in EVENTHANDLEFD EventHandleFd,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt CompletionCallback CallbackPtr
)
{
    _EventHandleFd = EventHandleFd;
    Start(ParentAsyncContext, CallbackPtr);
}

        
KAWIpcEventKicker::KAWIpcEventKicker(
    )
{
    // Should not be used
    KInvariant(FALSE);
}

KAWIpcEventKicker::KAWIpcEventKicker(
    __in EVENTHANDLEFD EventHandleFd
    )
{
    _EventHandleFd = EventHandleFd;
}

KAWIpcEventKicker::~KAWIpcEventKicker()
{
}

NTSTATUS KAWIpcEventKicker::Create(
    __in KAllocator& Allocator,
    __in ULONG AllocationTag,
    __in EVENTHANDLEFD EventHandleFd,
    __out KAWIpcEventKicker::SPtr& Context
)
{
    KAWIpcEventKicker::SPtr context;
    
    context = _new(AllocationTag, Allocator) KAWIpcEventKicker(EventHandleFd);
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

NTSTATUS KAWIpcEventKicker::Kick()
{
    NTSTATUS status = STATUS_SUCCESS;
    
#if !defined(PLATFORM_UNIX)
    BOOL b;

    b = SetEvent(_EventHandleFd.Get());
    if (! b)
    {
        status = STATUS_UNSUCCESSFUL;
    }
#else
    int err;

    err = eventfd_write(_EventHandleFd.Get(), 1);
    if (err != 0)
    {
        status = LinuxError::LinuxErrorToNTStatus(err);
    }
#endif
    
    return(status);
}
