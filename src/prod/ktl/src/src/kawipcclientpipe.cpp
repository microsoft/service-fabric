/*++

Module Name:

    KAWIpcClientPipe.cpp

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
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>
#include <unistd.h>

#include <palio.h>
#endif

ktl::Awaitable<NTSTATUS> KAWIpcClientPipe::OpenAsync(
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
        KDbgErrorWStatus(PtrToActivityId(this), "ktl::kservice::OpenAwaiter::Create", status);
        co_return status;
    }

    _SocketAddress = SocketAddress;

    co_return co_await *awaiter;
}

VOID KAWIpcClientPipe::OnServiceOpen()
{
    SetDeferredCloseBehavior();
    OpenTask();
}

Task KAWIpcClientPipe::OpenTask()
{
    NTSTATUS status = STATUS_SUCCESS;

    //
    // For safety, keep a ref on this for the lifetime of the whole
    // routine.
    //
    KAWIpcClientPipe::SPtr thisPtr = this;
    
    status = co_await ConnectToServerAsync(_SocketAddress,
                                           _SocketHandleFd,
                                           nullptr);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, _SocketAddress, 0);
    } else {
        KDbgCheckpointWDataInformational(0,
                            "KAWIpcClientPipe::OpenTask", status,
                            (ULONGLONG)this,
                            (ULONGLONG)_SocketHandleFd.Get(),
                            0,
                            0);
    }

    
    CompleteOpen(status);
}

ktl::Awaitable<NTSTATUS> KAWIpcClientPipe::CloseAsync()
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

VOID KAWIpcClientPipe::OnDeferredClosing()
{
    RundownAllOperations();
}


VOID KAWIpcClientPipe::OnServiceClose()
{
    CloseTask();
}

Task KAWIpcClientPipe::CloseTask()
{   
    //
    // For safety, keep a ref on this for the lifetime of the whole
    // routine.
    //
    KAWIpcClientPipe::SPtr thisPtr = this;
    
    if (_SocketHandleFd != (SOCKETHANDLEFD)0)
    {
        CloseSocketHandleFd(_SocketHandleFd);
        _SocketHandleFd = (SOCKETHANDLEFD)0;
    }
    
    KDbgCheckpointWDataInformational(0, "KAWIpcClientPipe::CloseTask",
                                     STATUS_SUCCESS, (ULONGLONG)this, (ULONGLONG)_SocketHandleFd.Get(), 0, 0);

    CompleteClose(STATUS_SUCCESS);
    co_return;
}

KAWIpcClientPipe::KAWIpcClientPipe()
{
    KInvariant(FALSE);
}

KAWIpcClientPipe::KAWIpcClientPipe(__in KAWIpcClient& Client)
{
    _Client = &Client;
}

KAWIpcClientPipe::~KAWIpcClientPipe()
{
}


KAWIpcClientPipe::AsyncConnectToServer::AsyncConnectToServer()
{
    KInvariant(FALSE);
}

KAWIpcClientPipe::AsyncConnectToServer::AsyncConnectToServer(__in KAWIpcClientPipe& Pipe)
{
    _Cancelled = FALSE;
    _Pipe = &Pipe;
}

KAWIpcClientPipe::AsyncConnectToServer::~AsyncConnectToServer()
{
}

NTSTATUS KAWIpcClientPipe::CreateAsyncConnectToServer(
    __out AsyncConnectToServer::SPtr& Context)
{
    KAWIpcClientPipe::AsyncConnectToServer::SPtr context;
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) KAWIpcClientPipe::AsyncConnectToServer(*this);
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

ktl::Awaitable<NTSTATUS> KAWIpcClientPipe::ConnectToServerAsync(
    __in ULONGLONG SocketAddress,
    __out SOCKETHANDLEFD& SocketHandleFd,
    __in_opt KAsyncContextBase* const ParentAsyncContext
    )
{
    //
    // Do not take a service activity since it is called from within
    // KAWIpcClientPipe OpenTask
    //
    
    NTSTATUS status;
    KAWIpcClientPipe::AsyncConnectToServer::SPtr context;

    status = CreateAsyncConnectToServer(context);
    if (! NT_SUCCESS(status))
    {
        co_return status ;
    }   
    
    context->_SocketAddress = SocketAddress;
    context->_SocketHandleFd = &SocketHandleFd;;

    StartAwaiter::SPtr awaiter;

    status = StartAwaiter::Create(
        GetThisAllocator(),
        GetThisAllocationTag(),
        *context,
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


VOID KAWIpcClientPipe::AsyncConnectToServer::Execute(
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    
    //
    // For safety, keep a ref on this for the lifetime of the whole
    // routine.
    //
    KAWIpcClientPipe::AsyncConnectToServer::SPtr thisPtr = this;
    
#if !defined(PLATFORM_UNIX)
    BOOL b;
    DWORD error = 0;
    HANDLE pipe = nullptr;
    KDynString name(GetThisAllocator(), KAWIpcPipe::_SocketNameLength);

    status = KAWIpcPipe::GetSocketName(_SocketAddress, name);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, _SocketAddress, error);
        Complete(status);
        return;
    }

    // CONSIDER: Retries ?
    while (! _Cancelled)
    {
        pipe = CreateFile((PWCHAR)name,
                          GENERIC_ALL,
                          0,
                          nullptr,
                          OPEN_EXISTING,
                          FILE_FLAG_OVERLAPPED,
                          nullptr);
        if (pipe != INVALID_HANDLE_VALUE)
        {
            //
            // Success !!
            //
            break;
        } else {
            error = GetLastError();
            if (error != ERROR_PIPE_BUSY)
            {
                status = KAWIpcPipe::MapErrorToNTStatus(error);
                KTraceFailedAsyncRequest(status, this, _SocketAddress, error);
                Complete(status);
                return;
            }
        }

        b = WaitNamedPipe((PWCHAR)name, 1000);
        if (! b)
        {
            error = GetLastError();
            status = KAWIpcPipe::MapErrorToNTStatus(error);
            KTraceFailedAsyncRequest(status, this, _SocketAddress, error);
            Complete(status);
            return;
        }
    } 

    DWORD mode = PIPE_READMODE_MESSAGE;
    b = SetNamedPipeHandleState(pipe,
                                &mode,
                                nullptr,
                                nullptr);
    if (! b)
    {
        CloseHandle(pipe);
        error = GetLastError();
        status = KAWIpcPipe::MapErrorToNTStatus(error);
        KTraceFailedAsyncRequest(status, this, _SocketAddress, error);
        Complete(status);
        return;
    }

    *_SocketHandleFd = pipe;
    status = STATUS_SUCCESS;
#else
    struct sockaddr_un serverAddress;
    int error;
    int sx;
    
    sx = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (sx == -1)
    {
        error = errno;
        status = LinuxError::LinuxErrorToNTStatus(error);
        KTraceFailedAsyncRequest(status, this, _SocketAddress, error);
        goto done;
    }
    
    memset(&serverAddress, 0, sizeof(struct sockaddr_un));
    serverAddress.sun_family = AF_UNIX;
    *serverAddress.sun_path = 0;
    *((unsigned long long*)&serverAddress.sun_path[1]) = _SocketAddress;

    error = connect (sx, (const struct sockaddr *) &serverAddress, sizeof(struct sockaddr_un));
    if (error == 0)
    {
        //
        // Instant connect
        //
        status = STATUS_SUCCESS;
        goto done;
    } else {
        KInvariant(error == -1);
        
        error = errno;
        if (error != EINPROGRESS)
        {
            //
            // An error occured
            //
            status = LinuxError::LinuxErrorToNTStatus(error);
            KTraceFailedAsyncRequest(status, this, _SocketAddress, error);
            goto done;
        }

        //
        // Wait for socket to be ready
        //
        while (! _Cancelled)
        {
            struct pollfd p;
            p.fd = sx;
            p.events = POLLOUT;
            p.revents = 0;

            error = poll(&p, 1, 1000);
            if (error == 1)
            {
                //
                // connect completed, harvest the error code
                //
                int error2;
                socklen_t len = sizeof(error);
                error2 = getsockopt(sx, SOL_SOCKET, SO_ERROR, (void *)&error, &len);
                if (error2 == -1)                   
                {
                    error = errno;
                    status = LinuxError::LinuxErrorToNTStatus(error);
                    KTraceFailedAsyncRequest(status, this, _SocketAddress, error);
                    goto done;
                }

                if (error == 0)
                {
                    status = STATUS_SUCCESS;
                    goto done;
                } else {
                    status = LinuxError::LinuxErrorToNTStatus(error);
                    KTraceFailedAsyncRequest(status, this, _SocketAddress, error);
                    goto done;
                }                                   
            } else if (error == 0) {
                //
                // Timeout
                //
                status = STATUS_IO_TIMEOUT;
                KTraceFailedAsyncRequest(status, this, _SocketAddress, error);
                goto done;
            } else {
                //
                // poll error
                //
                error = errno;

                if (error == EINTR)
                {
                    //
                    // Signal happened, let's retry
                    //
                    continue;
                }
                
                status = LinuxError::LinuxErrorToNTStatus(error);
                KTraceFailedAsyncRequest(status, this, _SocketAddress, error);
                goto done;
            }
        }
    }

done:
    if (NT_SUCCESS(status))
    {
        *_SocketHandleFd = sx;
    } else {
        close(sx);
    }   
#endif
    Complete(status);
}


VOID KAWIpcClientPipe::AsyncConnectToServer::OnReuse(
    )
{
    _Cancelled = FALSE;
}

VOID KAWIpcClientPipe::AsyncConnectToServer::OnCancel(
    )
{
    _Cancelled = TRUE;
}
