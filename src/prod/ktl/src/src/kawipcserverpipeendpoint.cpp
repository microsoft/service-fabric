/*++

Module Name:

    KAWIpcServerPipeEndpoint.cpp

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
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <palio.h>
#endif

NTSTATUS KAWIpcServerPipeEndpoint::CreateServerPipe(
    __out KSharedPtr<KAWIpcServerPipe>& Context
)
{
    KAWIpcServerPipe::SPtr context;
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) KAWIpcServerPipe();
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

//
// This is the top level task that spins up the resources for an new
// incoming client. It will start a KAWIpcServerPipe object to
// represent the server end of the pipe and then forward on to the
// Server's OnIncomingConnection. 
//
Task KAWIpcServerPipeEndpoint::ProcessIncomingConnection(
    __in SOCKETHANDLEFD SocketHandleFd
    )
{
    //
    // For safety, keep a ref on this for the lifetime of the whole
    // routine.
    //
    KAWIpcServerPipeEndpoint::SPtr thisPtr = this;

    BOOLEAN b = TryAcquireServiceActivity();
    if (!b) co_return;
    KFinally([&] { ReleaseServiceActivity(); });
    
    NTSTATUS status;
    KAWIpcServerPipe::SPtr pipe;
    KAWIpcServerClientConnection::SPtr connection;
    BOOLEAN pipeStarted, connectionStarted;

    pipeStarted = FALSE;
    connectionStarted = FALSE;

    //
    // Create and start the ServerPipe object
    //
    status = CreateServerPipe(pipe);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, (ULONGLONG)SocketHandleFd.Get(), 0);
        goto done;
    }

    status = co_await pipe->OpenAsync(*_Server, SocketHandleFd);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, (ULONGLONG)SocketHandleFd.Get(), (ULONGLONG)pipe.RawPtr());
        goto done;
    }
    pipeStarted = TRUE;

    status = co_await _Server->OnIncomingConnection(*pipe,
                                                    connection);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, (ULONGLONG)SocketHandleFd.Get(), (ULONGLONG)connection.RawPtr());
        goto done;
    }
    connectionStarted = TRUE;

    _Server->AddServerClientConnection(*connection);

done:
    {
        //
        // Cleanup on error condition
        //
        if (! NT_SUCCESS(status))
        {
            NTSTATUS statusDontCare;
            
            if (connectionStarted)
            {
                statusDontCare = co_await connection->CloseAsync();
                if (! NT_SUCCESS(statusDontCare))
                {
                    KTraceFailedAsyncRequest(statusDontCare, this, (ULONGLONG)SocketHandleFd.Get(), (ULONGLONG)connection.RawPtr());
                }
                pipeStarted = FALSE;
            }

            if (pipeStarted)
            {               
                statusDontCare = co_await pipe->CloseAsync();
                if (! NT_SUCCESS(statusDontCare))
                {
                    KTraceFailedAsyncRequest(statusDontCare, this, (ULONGLONG)SocketHandleFd.Get(), (ULONGLONG)pipe.RawPtr());
                }
                SocketHandleFd = (SOCKETHANDLEFD)0;
            }
            
            if (SocketHandleFd != (SOCKETHANDLEFD)0)
            {
                KAWIpcServerPipe::CloseSocketHandleFd(SocketHandleFd);
            }
        }

    }
}

//
// This is the server pipe endpoint worker routine and it runs on its
// own thread and its purpose is to serve incoming pipe connections and
// establish them. Note that pipes themselves are opened as blocking
// and so this thread will block waiting for incoming connections as
// well as reads and writes to the pipe will be blocking. Note that
// although this is not strictly needed for Windows, we follow a common
// pattern for Linux and Windows.
//
VOID KAWIpcServerPipeEndpoint::ServerWorkerRoutine()
{
    NTSTATUS status;

    //
    // For safety, keep a ref on this for the lifetime of the whole
    // routine.
    //
    KAWIpcServerPipeEndpoint::SPtr thisPtr = this;
    
    //
    // Do not take a request reference on the service as if one is
    // taken then the service close will deadlock as this thread is
    // shutdown by the service close itself. If this thread holds a
    // service reference then the service close routine will never be
    // called. Because of this, it is critical that the owner of the
    // thread not go away until this thread has exited.
    //

#if !defined(PLATFORM_UNIX)
    HANDLE pipeHandle = nullptr;
    OVERLAPPED pipeOverlapped;
    HANDLE pipeEvent;
    BOOLEAN firstInstance;
    BOOLEAN createPipe;
    BOOL b;
    DWORD error;
    HANDLE waitHandles[2];
    KDynString name(GetThisAllocator(), KAWIpcPipe::_SocketNameLength);

    {
        KFinally([&] { if (! NT_SUCCESS(status)) { _WorkerStartAcs->SetResult(status); } });
        
        _CancelThreadEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (_CancelThreadEvent == NULL)
        {
            error = GetLastError();
            status = STATUS_UNSUCCESSFUL;
            KTraceFailedAsyncRequest(status, this, _SocketAddress, error);

            // Fatal error - bring down whole process in debug
            KAssert(FALSE);
            return;
        }

        pipeEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (pipeEvent == NULL)
        {
            error = GetLastError();
            status = STATUS_UNSUCCESSFUL;
            KTraceFailedAsyncRequest(status, this, _SocketAddress, error);

            // Fatal error - bring down whole process in debug
            KAssert(FALSE);
            return;
        }

        status = KAWIpcPipe::GetSocketName(_SocketAddress,
                                           name);
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, _SocketAddress, 0);
            // Fatal error - bring down whole process in debug
            KAssert(FALSE);
            return;
        }
    }
    
    waitHandles[0] = _CancelThreadEvent;
    waitHandles[1] = pipeEvent;

    createPipe = TRUE;
    firstInstance = TRUE;
    do
    {
        //
        // First check if it is time to exit the thread
        //
        status = WaitForSingleObject(_CancelThreadEvent, 0);
        if (status == WAIT_OBJECT_0)
        {
            break;
        }
        
        memset(&pipeOverlapped, 0, sizeof(OVERLAPPED));
        pipeOverlapped.hEvent = pipeEvent;

        if (createPipe)
        {
            pipeHandle = CreateNamedPipe((PWCHAR)name,
                                  PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED |
                                  (firstInstance ? FILE_FLAG_FIRST_PIPE_INSTANCE : 0),
                                  PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_REJECT_REMOTE_CLIENTS,
                                  PIPE_UNLIMITED_INSTANCES,
                                  0, // outBufferSize
                                  0, // inBufferSize
                                  0, // timeout
                                  nullptr);

            if (pipeHandle == INVALID_HANDLE_VALUE)
            {
                pipeHandle = NULL;
                error = GetLastError();
                status = STATUS_UNSUCCESSFUL;
                
                //
                // For some reason creating the named pipe failed. This
                // need not be fatal - lets just wait a bit and try
                // again. Consider giving up after a while.
                //              
                KTraceFailedAsyncRequest(status, this, _SocketAddress, 0);
                Sleep(1000);
                continue;
            }
            
            b = ConnectNamedPipe(pipeHandle, &pipeOverlapped);
            if (! b)
            {
                error = GetLastError();
                if (error == ERROR_PIPE_CONNECTED)
                {
                    //
                    // This is the case where the client connects
                    // before we call connect.
                    //
                    SetEvent(pipeEvent);
                } else if (error != ERROR_IO_PENDING) {
                    //
                    // For some reason connecting the named pipe failed. This
                    // need not be fatal - lets just wait a bit and try
                    // again. Consider giving up after a while.
                    //              
                    status = STATUS_UNSUCCESSFUL;
                    KTraceFailedAsyncRequest(status, this, _SocketAddress, error);

                    CloseHandle(pipeHandle);
                    pipeHandle = NULL;
                    Sleep(1000);
                    continue;
                }
            }

            if (firstInstance)
            {
                _WorkerStartAcs->SetResult(STATUS_SUCCESS);
                firstInstance = FALSE;
            }
            
            createPipe = FALSE;
        }
        
        status = WaitForMultipleObjectsEx(2, waitHandles, FALSE, INFINITE, TRUE);
        if (status == WAIT_OBJECT_0)
        {
            //
            // Thread cancel event
            //
            break;
        } else if (status == WAIT_OBJECT_0 + 1)
        {
            //
            // pipe connected event. Start the processing task and then
            // loop back to create a new pipe instance
            //
            ProcessIncomingConnection(pipeHandle);
            pipeHandle = NULL;
            createPipe = TRUE;
        } else if (status == WAIT_IO_COMPLETION) {
            //
            // APC was serviced on this thread
            //
            continue;
        }
    } while (TRUE);

    //
    // Shutdown this thread
    //
    if (pipeHandle != NULL)
    {
        CloseHandle(pipeHandle);
    }

    if (_CancelThreadEvent != NULL)
    {
        _CancelThreadEvent = nullptr;
    }
    
#else
    struct sockaddr_un serverAddress;
    int epollFileDescriptor = -1;
    struct epoll_event evnts[1];
    int sfd = -1;
    int error;
    int count;

    KFinally([&] {
        if (sfd != -1)
        {
            close(sfd);
        }
        if (epollFileDescriptor != -1)
        {
            close(epollFileDescriptor);
        }
    });
    
    {
        KFinally([&] {
            _WorkerStartAcs->SetResult(status);
        });
        
        _CancelThreadEventFd = eventfd(0, EFD_CLOEXEC);
        if (_CancelThreadEventFd == -1)
        {
            error = errno;
            status = LinuxError::LinuxErrorToNTStatus(error);
            KTraceFailedAsyncRequest(status, this, _SocketAddress, error);
            // Fatal error - bring down whole process in debug
            KAssert(FALSE);
            return;
        }

        //
        // Start our socket listener
        //
        sfd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
        if (sfd == -1)
        {
            error = errno;
            status = LinuxError::LinuxErrorToNTStatus(error);
            KTraceFailedAsyncRequest(status, this, _SocketAddress, error);
            // Fatal error - bring down whole process in debug
            KAssert(FALSE);
            return;
        }

        memset(&serverAddress, 0, sizeof(struct sockaddr_un));
        serverAddress.sun_family = AF_UNIX;
        *serverAddress.sun_path = 0;
        *((unsigned long long*)&serverAddress.sun_path[1]) = _SocketAddress;

        error = bind(sfd, (struct sockaddr *)&serverAddress, sizeof(struct sockaddr_un));
        if (error == -1)
        {
            error = errno;
            status = LinuxError::LinuxErrorToNTStatus(error);
            KTraceFailedAsyncRequest(status, this, sfd, error);
            // Fatal error - bring down whole process in debug
            KAssert(FALSE);
            return;
        }

        int flags = fcntl(sfd, F_GETFL);     // Set lister to be non blocking
        if (flags == -1)
        {
            error = errno;
            status = LinuxError::LinuxErrorToNTStatus(error);
            KTraceFailedAsyncRequest(status, this, sfd, error);
            // Fatal error - bring down whole process in debug
            KAssert(FALSE);
            return;
        }
        flags |= O_NONBLOCK;
        error = fcntl(sfd, F_SETFL, flags);
        if (error == -1)
        {
            error = errno;
            status = LinuxError::LinuxErrorToNTStatus(error);
            KTraceFailedAsyncRequest(status, this, sfd, error);
            // Fatal error - bring down whole process in debug
            KAssert(FALSE);
            return;
        }
        
        error = listen(sfd, 1024);
        if (error == -1)
        {
            error = errno;
            status = LinuxError::LinuxErrorToNTStatus(error);
            KTraceFailedAsyncRequest(status, this, sfd, error);
            // Fatal error - bring down whole process in debug
            KAssert(FALSE);
            return;
        }

        //
        // Create an epoll to drive wakeups
        //
        struct epoll_event evnt = {0};

        epollFileDescriptor = epoll_create1(EPOLL_CLOEXEC);
        if (epollFileDescriptor == -1)
        {
            error = errno;
            status = LinuxError::LinuxErrorToNTStatus(error);
            KTraceFailedAsyncRequest(status, this, sfd, error);
            // Fatal error - bring down whole process in debug
            KAssert(FALSE);
            return;
        }

        evnt.data.fd = sfd;
        evnt.events = EPOLLIN | EPOLLOUT | EPOLLHUP | EPOLLERR;
        error = epoll_ctl(epollFileDescriptor, EPOLL_CTL_ADD, sfd, &evnt);
        if (error == -1)
        {
            error = errno;
            status = LinuxError::LinuxErrorToNTStatus(error);
            KTraceFailedAsyncRequest(status, this, sfd, error);
            // Fatal error - bring down whole process in debug
            KAssert(FALSE);
            return;
        }

        evnt.data.fd = _CancelThreadEventFd;
        evnt.events = EPOLLIN | EPOLLONESHOT;
        error = epoll_ctl(epollFileDescriptor, EPOLL_CTL_ADD, _CancelThreadEventFd, &evnt);
        if (error == -1)
        {
            error = errno;
            status = LinuxError::LinuxErrorToNTStatus(error);
            KTraceFailedAsyncRequest(status, this, sfd, error);
            // Fatal error - bring down whole process in debug
            KAssert(FALSE);
            return;
        }
        
        status = STATUS_SUCCESS;
    }

    //
    // Main loop waiting for incoming connections
    //
    while(TRUE)
    {
        count = epoll_wait(epollFileDescriptor, evnts, 1, -1);
        if (count == -1)
        {
            error = errno;
            if (error != EINTR)
            {
                status = LinuxError::LinuxErrorToNTStatus(error);
                KTraceFailedAsyncRequest(status, this, sfd, error);
                KAssert(FALSE);
            }

            //
            // Signal or other error, try again
            //
            continue;               
        }
        
        if (evnts[0].data.fd == _CancelThreadEventFd)
        {
            //
            // Thread should exit
            //
            break;
        } else if (evnts[0].data.fd == sfd) {
            //
            // Incoming connection, let's start it up
            //
            int incomingSfd;
            struct sockaddr_un clientAddress;
            socklen_t len;

            len = sizeof(struct sockaddr_un);
            incomingSfd = accept4(sfd, (struct sockaddr *)&clientAddress, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
            if (incomingSfd == -1) 
            {
                error = errno;
                if (errno != EAGAIN)
                {
                    status = LinuxError::LinuxErrorToNTStatus(error);
                    KTraceFailedAsyncRequest(status, this, sfd, error);
                    KAssert(FALSE);
                }
                continue;
            }

            ProcessIncomingConnection(incomingSfd);
            
        } else {
            //
            // Whose FD woke us ?
            //
            KTraceFailedAsyncRequest(status, this, _SocketAddress, evnts[0].data.fd);
            KInvariant(FALSE);
        }       
    }

#endif
  
    //
    // Indicate that the worker thread is completed. No work should be
    // done after this.
    //
    _WorkerThreadExit.SetEvent();
}

VOID KAWIpcServerPipeEndpoint::ServerWorker(
    __in PVOID Context
)
{
    KAWIpcServerPipeEndpoint* thisPtr = (KAWIpcServerPipeEndpoint*)Context;
    thisPtr->ServerWorkerRoutine();
}

ktl::Awaitable<VOID> KAWIpcServerPipeEndpoint::ShutdownServerWorker()
{
    NTSTATUS status;
    
#if !defined(PLATFORM_UNIX)
    SetEvent(_CancelThreadEvent);   
#else
    int error;
    error = eventfd_write(_CancelThreadEventFd, 1);
    if (error != 0)
    {
        status = LinuxError::LinuxErrorToNTStatus(error);
        KTraceFailedAsyncRequest(status, this, 0, error);

        // Bring down whole process on error
        KInvariant(FALSE);
    }
    
#endif

    status = co_await _WorkerThreadExitWait->StartWaitUntilSetAsync(nullptr);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, _SocketAddress, 0);
    }
    _WorkerThread = nullptr;
}

ktl::Awaitable<NTSTATUS> KAWIpcServerPipeEndpoint::OpenAsync(
    __in KAWIpcServer& Server,
    __in ULONGLONG SocketAddress,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt KAsyncGlobalContext* const GlobalContextOverride)
{
    KDbgCheckpointInformational(PtrToActivityId(this), "OpenAsync");

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
    _SocketAddress = SocketAddress;

    co_return co_await *awaiter;
}

VOID KAWIpcServerPipeEndpoint::OnServiceOpen()
{
    KDbgCheckpointInformational(PtrToActivityId(this), "OnServiceOpen");
    SetDeferredCloseBehavior();

    OpenTask();
}

Task KAWIpcServerPipeEndpoint::OpenTask()
{
    NTSTATUS status = STATUS_SUCCESS;

    //
    // For safety, keep a ref on this for the lifetime of the whole
    // routine.
    //
    KAWIpcServerPipeEndpoint::SPtr thisPtr = this;
    
    _WorkerThread = nullptr;
    
    //
    // Start up the worker thread that will process incoming
    // connections, reads, writes, etc
    //
    status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(GetThisAllocator(), GetThisAllocationTag(), _WorkerStartAcs);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, _SocketAddress, 0);
        goto done;
    }
    
    status = _WorkerThreadExit.CreateWaitContext(GetThisAllocationTag(), GetThisAllocator(), _WorkerThreadExitWait);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, _SocketAddress, 0);
        goto done;
    }

    status = KThread::Create(KAWIpcServerPipeEndpoint::ServerWorker,
                             this,
                             _WorkerThread,
                             GetThisAllocator(),
                             GetThisAllocationTag());
    
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, _SocketAddress, 0);
        goto done;
    }

    status = co_await _WorkerStartAcs->GetAwaitable();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, _SocketAddress, 0);
        goto done;
    }

    // TODO: Replace with KFinally when there is one that works with
    //       coroutines.
done:
    if (! NT_SUCCESS(status))
    {
        if (_WorkerThread)
        {
            co_await ShutdownServerWorker();
        }
        _Server = nullptr;
    }

   KDbgCheckpointWDataInformational(0,
                        "KAWIpcServerPipeEndpoint::OpenTask", status,
                        _SocketAddress,
                        0,
                        0,
                        0);
    
    CompleteOpen(status);   
}

ktl::Awaitable<NTSTATUS> KAWIpcServerPipeEndpoint::CloseAsync()
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

VOID KAWIpcServerPipeEndpoint::OnServiceClose()
{
    CloseTask();
}

Task KAWIpcServerPipeEndpoint::CloseTask()
{
    //
    // For safety, keep a ref on this for the lifetime of the whole
    // routine.
    //
    KAWIpcServerPipeEndpoint::SPtr thisPtr = this;
    
    //
    // Tell the worker thread to exit and wait for it
    //
    co_await ShutdownServerWorker();

    //
    // Break backlink to KAWIpcServer
    //
    _Server = nullptr;

   KDbgCheckpointWDataInformational(0,
                        "KAWIpcServerPipeEndpoint::CloseTask", STATUS_SUCCESS,
                        _SocketAddress,
                        0,
                        0,
                        0);
        
    CompleteClose(STATUS_SUCCESS);
}


KAWIpcServerPipeEndpoint::KAWIpcServerPipeEndpoint()
{
}


KAWIpcServerPipeEndpoint::~KAWIpcServerPipeEndpoint()
{
}

KAWIpcServerPipeEndpoint::AsyncWaitForIncomingConnection::AsyncWaitForIncomingConnection()
{
}

KAWIpcServerPipeEndpoint::AsyncWaitForIncomingConnection::~AsyncWaitForIncomingConnection()
{
}

NTSTATUS KAWIpcServerPipeEndpoint::CreateAsyncWaitForIncomingConnection(
    __out AsyncWaitForIncomingConnection::SPtr& Context)
{
    KAWIpcServerPipeEndpoint::AsyncWaitForIncomingConnection::SPtr context;
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) KAWIpcServerPipeEndpoint::AsyncWaitForIncomingConnection();
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

ktl::Awaitable<NTSTATUS> KAWIpcServerPipeEndpoint::WaitForIncomingConnectionAsync(
    __in SOCKETHANDLEFD ListeningSocketHandleFd,
    __out SOCKETHANDLEFD& SocketHandleFd,
    __in_opt KAsyncContextBase* const ParentAsyncContext
    )
{
    KCoService$ApiEntry(TRUE);
    
    NTSTATUS status;
    KAWIpcServerPipeEndpoint::AsyncWaitForIncomingConnection::SPtr context;

    status = CreateAsyncWaitForIncomingConnection(context);
    if (! NT_SUCCESS(status))
    {
        co_return status ;
    }   
    
    context->_ListeningSocketHandleFd = ListeningSocketHandleFd;
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

VOID KAWIpcServerPipeEndpoint::AsyncWaitForIncomingConnection::Execute(
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    
    //
    // For safety, keep a ref on this for the lifetime of the whole
    // routine.
    //

    KAWIpcServerPipeEndpoint::AsyncWaitForIncomingConnection::SPtr thisPtr = this;
    
#if !defined(PLATFORM_UNIX)
    BOOL b;
    DWORD error;
    
    b = ConnectNamedPipe(_ListeningSocketHandleFd.Get(), nullptr);
    if (! b)
    {
        error = GetLastError();
        if (error != ERROR_PIPE_CONNECTED)
        {
            status = STATUS_UNSUCCESSFUL;
            KTraceFailedAsyncRequest(status, this, 0, error);
        }
    }
    
    *_SocketHandleFd = _ListeningSocketHandleFd;    
#else
    // On Linux this is done on the server thread
#endif
    
    Complete(status);
}


VOID KAWIpcServerPipeEndpoint::AsyncWaitForIncomingConnection::OnReuse(
    )
{
}

VOID KAWIpcServerPipeEndpoint::AsyncWaitForIncomingConnection::OnCancel(
    )
{
}
