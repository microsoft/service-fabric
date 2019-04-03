// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "NetIoManager.h"
#include "UdpOverlap.h"
#include "UdpListener.h"

void DNS::CreateNetIoManager(
    __out INetIoManager::SPtr& spManager,
    __in KAllocator& allocator,
    __in const IDnsTracer::SPtr& spTracer,
    __in ULONG numberOfConcurrentQueries
)
{
    NetIoManager::SPtr sp;
    NetIoManager::Create(/*out*/sp, allocator, spTracer, numberOfConcurrentQueries);

    spManager = sp.RawPtr();
}

/*static*/
void NetIoManager::Create(
    __out NetIoManager::SPtr& spManager,
    __in KAllocator& allocator,
    __in const IDnsTracer::SPtr& spTracer,
    __in ULONG numberOfConcurrentQueries
)
{
    spManager = _new(TAG, allocator) NetIoManager(spTracer, numberOfConcurrentQueries);
    KInvariant(spManager != nullptr);
}

NetIoManager::NetIoManager(
    __in const IDnsTracer::SPtr& spTracer,
    __in ULONG numberOfConcurrentQueries
) : _efd(-1),
_spTracer(spTracer),
_htSockets(numberOfConcurrentQueries, K_DefaultHashFunction, GetThisAllocator())
{
    Tracer().Trace(DnsTraceLevel_Info, "Constructing DNS NetIoManager.");

#if !defined(PLATFORM_UNIX)
    WORD wVersionRequested = MAKEWORD(2, 2);
    WSADATA wsaData;

    int err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0)
    {
        Tracer().Trace(DnsTraceLevel_Error,
            "DNS NetIoManager, failed to initialize Winsock, error {0}.", static_cast<LONG>(err));

        KInvariant(false);
    }
#else
    _efd = epoll_create1(0);
    if (_efd == -1)
    {
        Tracer().Trace(DnsTraceLevel_Error, "DNS NetIoManager, failed to create epoll fd, error {0}", errno);
    }
#endif
}

NetIoManager::~NetIoManager()
{
    Tracer().Trace(DnsTraceLevel_Info, "Destructing DNS NetIoManager.");

#if !defined(PLATFORM_UNIX)
    int err = WSACleanup();
    if (err != 0)
    {
        int error = 0;
        error = WSAGetLastError();
        Tracer().Trace(DnsTraceLevel_Error,
            "DNS NetIoManager, failed to uninitialize Winsock, API return code, {0}, WSAGetLastError {1}.",
            static_cast<LONG>(err),
            static_cast<LONG>(error));
    }
#endif
}


//***************************************
// BEGIN KAsyncContext region
//***************************************

void NetIoManager::OnStart()
{
    Tracer().Trace(DnsTraceLevel_Info, "DNS NetIoManager started");

#if defined(PLATFORM_UNIX)
    NTSTATUS status = KThread::Create(NetIoManager::MainLoop, this, _spEpollThread, GetThisAllocator());
    KInvariant(status == STATUS_SUCCESS);
#endif
}

void NetIoManager::OnCancel()
{
    Tracer().Trace(DnsTraceLevel_Info, "DNS NetIoManager OnCancel called.");
    Complete(STATUS_CANCELLED);
}

void NetIoManager::OnCompleted()
{
    Tracer().Trace(DnsTraceLevel_Info, "DNS NetIoManager OnCompleted called.");

    K_LOCK_BLOCK(_lockSockets)
    {
        ULONG_PTR h;
        Queues queues;
        _htSockets.Reset();
        while (STATUS_SUCCESS == _htSockets.Next(/*out*/h, /*out*/queues))
        {
            {
                Queue& readQueue = *queues.ReadQueue;
                IUdpAsyncOp::SPtr spOverlapOp;
                while (readQueue.Deq(spOverlapOp))
                {
                    spOverlapOp->IOCP_Completion(WSA_OPERATION_ABORTED, 0);
                }
            }

            {
                Queue& writeQueue = *queues.WriteQueue;
                IUdpAsyncOp::SPtr spOverlapOp;
                while (writeQueue.Deq(spOverlapOp))
                {
                    spOverlapOp->IOCP_Completion(WSA_OPERATION_ABORTED, 0);
                }
            }
        }

        _htSockets.Clear();
    }

    Tracer().Trace(DnsTraceLevel_Info, "DNS NetIoManager Completed.");
}

//***************************************
// END KAsyncContext region
//***************************************

void NetIoManager::StartManager(
    __in_opt KAsyncContextBase* const parent
)
{
    Start(parent, nullptr);
}

void NetIoManager::CloseAsync()
{
    Tracer().Trace(DnsTraceLevel_Info, "DNS NetIoManager CloseAsync");
    Cancel();
}

void NetIoManager::CreateUdpListener(
    __out IUdpListener::SPtr& spListener,
    __in bool fEnableSocketAddressReuse
)
{
    UdpListener::SPtr spInternal;
    UdpListener::Create(/*out*/spInternal, GetThisAllocator(), _spTracer, this, fEnableSocketAddressReuse);

    spListener = spInternal.RawPtr();
}

#if defined(PLATFORM_UNIX)
bool NetIoManager::RegisterSocket(
    __in SOCKET socket
)
{
    K_LOCK_BLOCK(_lockSockets)
    {
        struct epoll_event ev;
        ev.data.fd = socket;
        ev.events = EPOLLIN | EPOLLET;

        int s = epoll_ctl(_efd, EPOLL_CTL_ADD, socket, &ev);
        if (s == -1)
        {
            Tracer().Trace(DnsTraceLevel_Warning, "DNS NetIoManager, failed to RegisterSocket {0}, error {1}", socket, errno);
            return false;
        }

        ULONG_PTR h = (ULONG_PTR)socket;
        Queues queues;
        queues.ReadQueue = _new(TAG, GetThisAllocator()) Queue();
        queues.WriteQueue = _new(TAG, GetThisAllocator()) Queue();
        if (STATUS_SUCCESS != _htSockets.Put(h, queues))
        {
            Tracer().Trace(DnsTraceLevel_Warning, "Dns NetIoManage, failed to add socket {0} to map", socket);
            return false;
        }
    }

    return true;
}

void NetIoManager::UnregisterSocket(
    __in SOCKET socket
)
{
    K_LOCK_BLOCK(_lockSockets)
    {
        ULONG_PTR h = (ULONG_PTR)socket;
        Queues queues;
        if (STATUS_SUCCESS == _htSockets.Get(h, /*out*/queues))
        {
            {
                Queue& readQueue = *queues.ReadQueue;
                IUdpAsyncOp::SPtr spOverlapOp;
                while (readQueue.Deq(spOverlapOp))
                {
                    spOverlapOp->IOCP_Completion(WSA_OPERATION_ABORTED, 0);
                }
            }

            {
                Queue& writeQueue = *queues.WriteQueue;
                IUdpAsyncOp::SPtr spOverlapOp;
                while (writeQueue.Deq(spOverlapOp))
                {
                    spOverlapOp->IOCP_Completion(WSA_OPERATION_ABORTED, 0);
                }
            }
        }

        if (STATUS_SUCCESS != _htSockets.Remove(h))
        {
            Tracer().Trace(DnsTraceLevel_Warning, "Dns NetIoManage, failed to remove socket {0} from the map", socket);
        }

        epoll_ctl(_efd, EPOLL_CTL_DEL, socket, nullptr);
    }
}

int NetIoManager::ReadInternal(
    __in SOCKET socket,
    __in IUdpAsyncOp& overlapOp,
    __out ULONG& bytesRead
)
{
    bytesRead = 0;
    int result = NOERROR;

    KBuffer::SPtr spBuffer = overlapOp.GetBuffer();
    ISocketAddress::SPtr spAddress = overlapOp.GetAddress();

    int count = recvfrom(socket, spBuffer->GetBuffer(), spBuffer->QuerySize(), 0 /*flags*/,
        reinterpret_cast<SOCKADDR*>(spAddress->Address()), spAddress->SizePtr());

    if (count < 0)
    {
        result = errno;
    }
    else
    {
        bytesRead = count;
    }

    return result;
}

int NetIoManager::ReadAsync(
    __in SOCKET socket,
    __in IUdpAsyncOp& overlapOp,
    __out ULONG& bytesRead
)
{
    int result = ReadInternal(socket, overlapOp, bytesRead);
    if (result == NOERROR)
    {
        Tracer().Trace(DnsTraceLevel_Noise,
            "DNS NetIoManager ReadAsync, successfully read sync from socket {0} bytes read {1}",
            socket, bytesRead);

        overlapOp.IOCP_Completion(STATUS_SUCCESS, bytesRead);

        return result;
    }

    if ((result != EAGAIN) && (result != EWOULDBLOCK))
    {
        Tracer().Trace(DnsTraceLevel_Warning,
            "DNS NetIoManager ReadAsync, failed to read internal socket {0} error {1}",
            socket, result);

        return WSAEFAULT;
    }

    // Nothing to read, completion callback will occur
    K_LOCK_BLOCK(_lockSockets)
    {
        Queues queues;
        ULONG_PTR h = (ULONG_PTR)socket;
        if (STATUS_SUCCESS != _htSockets.Get(h, /*out*/queues))
        {
            Tracer().Trace(DnsTraceLevel_Warning, "DNS NetIoManager, failed to ReadAsync, socket not found in the map.");
            return WSAEFAULT;
        }

        if (!queues.ReadQueue->Enq(&overlapOp))
        {
            KInvariant(false);
        }
    }

    Tracer().Trace(DnsTraceLevel_Noise, "DNS NetIoManager ReadAsync, successfully scheduled socket {0} read op", socket);

    return WSA_IO_PENDING;
}

int NetIoManager::WriteInternal(
    __in SOCKET socket,
    __in KBuffer& buffer,
    __in ULONG count,
    __in ISocketAddress& address,
    __out ULONG& bytesWritten
)
{
    int error = NOERROR;
    bytesWritten = sendto(socket, buffer.GetBuffer(), count, 0/*flags*/,
        reinterpret_cast<SOCKADDR*>(address.Address()), address.Size());

    if (bytesWritten < 0)
    {
        bytesWritten = 0;
        error = errno;
    }
    else
    {
        error = NOERROR;
    }

    return error;
}

int NetIoManager::WriteAsync(
    __in SOCKET socket,
    __in KBuffer& buffer,
    __in ULONG count,
    __in ISocketAddress& address,
    __in IUdpAsyncOp& overlapOp,
    __out ULONG& bytesWritten
)
{
    bytesWritten = 0;

    int result = WriteInternal(socket, buffer, count, address, /*out*/bytesWritten);
    if (result == NOERROR)
    {
        Tracer().Trace(DnsTraceLevel_Noise,
            "DNS NetIoManager WriteAsync, successfully wrote sync to socket {0} bytes written {1}",
            socket, bytesWritten);

        overlapOp.IOCP_Completion(STATUS_SUCCESS, bytesWritten);

        return result;
    }

    if ((result != EAGAIN) && (result != EWOULDBLOCK))
    {
        Tracer().Trace(DnsTraceLevel_Warning, "DNS NetIoManager, failed to WriteAsync, error {0}", result);
        return WSAEFAULT;
    }

    K_LOCK_BLOCK(_lockSockets)
    {
        Queues queues;
        ULONG_PTR h = (ULONG_PTR)socket;
        if (STATUS_SUCCESS != _htSockets.Get(h, /*out*/queues))
        {
            Tracer().Trace(DnsTraceLevel_Warning, "DNS NetIoManager, failed to WriteAsync, socket not found in the map.");
            return WSAEFAULT;
        }

        if (!queues.WriteQueue->Enq(&overlapOp))
        {
            KInvariant(false);
        }

        struct epoll_event ev;
        ev.data.fd = socket;
        ev.events = EPOLLIN | EPOLLET | EPOLLOUT;

        int s = epoll_ctl(_efd, EPOLL_CTL_MOD, socket, &ev);
        if (s == -1)
        {
            Tracer().Trace(DnsTraceLevel_Warning, "DNS NetIoManager, failed to modify socket {0}, error {1}", socket, errno);
            return WSAEFAULT;
        }

        Tracer().Trace(DnsTraceLevel_Noise, "DNS NetIoManager WriteAsync, successfully scheduled socket {0} write op", socket);
    }

    return WSA_IO_PENDING;
}

void NetIoManager::HandleEpollEvents()
{
    typedef struct epoll_event EPOLLEVENT;

    const ULONG MaxEvents = 256;
    ULONG bufferSize = sizeof(EPOLLEVENT) * MaxEvents;
    KBuffer::SPtr spBuffer;
    KBuffer::Create(bufferSize, spBuffer, GetThisAllocator());

    EPOLLEVENT* events = (EPOLLEVENT*)spBuffer->GetBuffer();

    while (Status() == STATUS_PENDING)
    {
        int n = epoll_wait(_efd, events, MaxEvents, /*timeout*/1000);
        if (n == -1)
        {
            Tracer().Trace(DnsTraceLevel_Warning, "DNS NetIoManager MainLoop, epoll wait failed, error {0}", errno);
            continue;
        }

        if (n > 0)
        {
            Tracer().Trace(DnsTraceLevel_Noise, "DNS NetIoManager MainLoop, epoll wait returned {0} sockets", n);
        }

        for (int i = 0; i < n; i++)
        {
            EPOLLEVENT& ev = events[i];

            Tracer().Trace(DnsTraceLevel_Noise, "DNS NetIoManager MainLoop,  event {0} of {1}, socket {2}, flags {3}",
                i, n, ev.data.fd, ev.events);

            K_LOCK_BLOCK(_lockSockets)
            {
                Queues queues;
                ULONG_PTR h = (ULONG_PTR)ev.data.fd;
                if (STATUS_SUCCESS != _htSockets.Get(h, /*out*/queues))
                {
                    Tracer().Trace(DnsTraceLevel_Warning,
                        "DNS NetIoManager MainLoop, failed to find socket {0} in the map",
                        ev.data.fd);
                    continue;
                }

                if (ev.events & EPOLLIN)
                {
                    bool fReadDone = false;
                    Queue& readQueue = *queues.ReadQueue;
                    while (!readQueue.IsEmpty() && !fReadDone)
                    {
                        IUdpAsyncOp::SPtr spOp = readQueue.Peek();
                        ULONG bytesRead;
                        int result = ReadInternal(ev.data.fd, *spOp, /*out*/bytesRead);
                        if ((result != EAGAIN) && (result != EWOULDBLOCK))
                        {
                            Tracer().Trace(DnsTraceLevel_Noise,
                                "DNS NetIoManager MainLoop EPOLLIN, successfully read data from socket {0}, bytesRead {1}, error {2}",
                                ev.data.fd, bytesRead, (LONG)result);

                            spOp->IOCP_Completion(result, bytesRead);
                            readQueue.Deq();
                        }
                        else
                        {
                            // Nothing to read, this is perfectly OK, don't dequeue the item
                            fReadDone = true;

                            Tracer().Trace(DnsTraceLevel_Noise,
                                "DNS NetIoManager MainLoop EPOLLIN, no more data to read for socket {0}, error {1}",
                                ev.data.fd, (LONG)result);
                        }
                    }

                    if (readQueue.IsEmpty())
                    {
                        Tracer().Trace(DnsTraceLevel_Noise,
                            "DNS NetIoManager MainLoop EPOLLIN, read queue is empty for socket {0}",
                            ev.data.fd);
                    }
                }

                if (ev.events & EPOLLOUT)
                {
                    // Send the data until the queue is empty or socket blocks.
                    bool fWriteDone = false;
                    Queue& writeQueue = *queues.WriteQueue;
                    while (!writeQueue.IsEmpty() && !fWriteDone)
                    {
                        IUdpAsyncOp::SPtr spOp = writeQueue.Peek();
                        ULONG bytesWritten = 0;

                        int result = WriteInternal(
                            ev.data.fd,
                            *spOp->GetBuffer(),
                            spOp->GetBufferDataLength(),
                            *spOp->GetAddress(),
                            /*out*/bytesWritten);

                        if ((result != EAGAIN) && (result != EWOULDBLOCK))
                        {
                            spOp->IOCP_Completion(result, bytesWritten);
                            writeQueue.Deq();
                        }
                        else
                        {
                            // Can't write, exit
                            fWriteDone = true;

                            Tracer().Trace(DnsTraceLevel_Noise,
                                "DNS NetIoManager MainLoop EPOLLOUT, can't write to socket {0}, error",
                                ev.data.fd, (LONG)result);
                        }
                    }

                    if (writeQueue.IsEmpty())
                    {
                        struct epoll_event writeEv;
                        writeEv.data.fd = ev.data.fd;
                        writeEv.events = EPOLLIN | EPOLLET;
                        int s = epoll_ctl(_efd, EPOLL_CTL_MOD, ev.data.fd, &writeEv);
                        if (s == -1)
                        {
                            Tracer().Trace(DnsTraceLevel_Warning,
                                "DNS NetIoManager MainLoop EPOLLOUT, failed to modify socket {0}, error {1}",
                                ev.data.fd, errno);
                        }
                        else
                        {
                            Tracer().Trace(DnsTraceLevel_Noise,
                                "DNS NetIoManager MainLoop EPOLLOUT, successfully wrote all data to socket {0}, removing EPOLLOUT",
                                ev.data.fd);
                        }
                    }
                }
            }
        }
    }

    Tracer().Trace(DnsTraceLevel_Info, "NetIoManager MainLoop finished.");
}

/*static*/
void NetIoManager::MainLoop(
    __inout_opt void* parameter
)
{
    NetIoManager* pThis = static_cast<NetIoManager*>(parameter);
    KInvariant(pThis != nullptr);

    pThis->HandleEpollEvents();
}
#endif
