/*++

Module Name:

    KAWIpcPipe.cpp

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
#include <unistd.h>
#include <poll.h>

#include <palio.h>
#endif

VOID KAWIpcPipe::CloseSocketHandleFd(
    __in SOCKETHANDLEFD SocketHandleFd
    )
{
#if !defined(PLATFORM_UNIX)
    CloseHandle(SocketHandleFd.Get());
#else
    shutdown(SocketHandleFd.Get(), SHUT_RDWR);
    close(SocketHandleFd.Get());
#endif
}

#if !defined(PLATFORM_UNIX)
NTSTATUS KAWIpcPipe::MapErrorToNTStatus(
    __in DWORD Error
    )
{
    NTSTATUS status;
    
    switch(Error)
    {
        case ERROR_FILE_NOT_FOUND:
        {
            status = STATUS_NO_SUCH_FILE;
            break;
        }

        case ERROR_INVALID_HANDLE:
        {
            status = STATUS_INVALID_HANDLE;
            break;
        }

        case ERROR_PIPE_LISTENING:
        {
            status = STATUS_PIPE_LISTENING;
            break;
        }

        case ERROR_PIPE_CONNECTED:
        {
            status = STATUS_PIPE_CONNECTED;
            break;
        }

        case ERROR_PIPE_NOT_CONNECTED:
        {
            status = STATUS_PIPE_DISCONNECTED;
            break;
        }

        case ERROR_BROKEN_PIPE:
        {
            status = STATUS_PIPE_BROKEN;
            break;
        }

        case ERROR_NO_DATA:
        {
            status = STATUS_PIPE_EMPTY;
            break;
        }
        
        case ERROR_BAD_PIPE:
        {
            status = STATUS_INVALID_PIPE_STATE;
            break;
        }
        
        case ERROR_PIPE_BUSY:
        {
            status = STATUS_PIPE_BUSY;
            break;
        }

        case ERROR_SEM_TIMEOUT:
        {
            status = STATUS_IO_TIMEOUT;
            break;
        }

        case ERROR_NOT_FOUND:
        {
            status = STATUS_NOT_FOUND;
            break;
        }
        
        default:
        {
            KAssert(FALSE);                 // Fail to catch unmapped errors
            status = STATUS_UNSUCCESSFUL;
            KTraceFailedAsyncRequest(status, nullptr, 0, Error);            
            break;
        }
    }
    return(status);
}

NTSTATUS KAWIpcPipe::GetSocketName(
    __in ULONGLONG SocketAddress,
    __out KDynString& Name
)
{
    NTSTATUS status = STATUS_SUCCESS;
    KStringView prefix(L"\\\\.\\pipe\\_KtlLogger_Microsoft_");
    BOOLEAN b;

    b = Name.Concat(prefix);
    if (! b)
    {
        status = STATUS_UNSUCCESSFUL;
        return(status);
    }
    
    b = Name.FromULONGLONG(SocketAddress, FALSE, TRUE);
    if (! b)
    {
        status = STATUS_UNSUCCESSFUL;
        return(status);
    }

    b = Name.SetNullTerminator();
    if (! b)
    {
        status = STATUS_UNSUCCESSFUL;
    }
    
    return(status);
}
#endif


const ULONG KAWIpcPipe::_OperationLinkageOffset = FIELD_OFFSET(KAWIpcPipeAsyncOperation,
                                                               _ListEntry);

NTSTATUS KAWIpcPipe::AddOperation(
    __in KAWIpcPipeAsyncOperation& Operation
    )
{
    //
    // Take ref count when adding to the list. The ref count will be
    // removed when the item is removed from the list.
    //
    Operation.AddRef();
    K_LOCK_BLOCK(_OperationsListLock)
    {
        _OperationsList.AppendTail(&Operation);
    }
    
    return(STATUS_SUCCESS);
}

NTSTATUS KAWIpcPipe::RemoveOperation(
    __in KAWIpcPipeAsyncOperation& Operation
    )
{
    KAWIpcPipeAsyncOperation* operation = nullptr;
    
    K_LOCK_BLOCK(_OperationsListLock)
    {
        operation = _OperationsList.Remove(&Operation);
    }
    
    if (operation == nullptr)
    {
        return(STATUS_NOT_FOUND);
    }

    //
    // Release ref count taken when added to list
    //
    Operation.Release();

    return(STATUS_SUCCESS);
}

NTSTATUS KAWIpcPipe::RundownAllOperations(
    )
{
    KAWIpcPipeAsyncOperation* operation = nullptr;

    KDbgCheckpointWDataInformational(0,
                        "RundownAllOperations", STATUS_SUCCESS, (ULONGLONG)this, (ULONGLONG)_SocketHandleFd.Get(), 0, 0);
    
    K_LOCK_BLOCK(_OperationsListLock)
    {
        operation = _OperationsList.PeekHead();
        while (operation != nullptr)
        {
            operation->Cancel();
            operation = _OperationsList.Successor(operation);
        }
    }
    return(STATUS_SUCCESS);
}

KAWIpcPipe::AsyncSendData::AsyncSendData()
{
    KInvariant(FALSE);
}

KAWIpcPipe::AsyncSendData::AsyncSendData(
    __in KAWIpcPipe& Pipe,
    __in SOCKETHANDLEFD SocketHandleFd                                       
    )
{
#if !defined(PLATFORM_UNIX)
    _IoCompleteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (_IoCompleteEvent == nullptr)
    {
        SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
        return;
    }
#endif
    _Pipe = &Pipe;
    _SocketHandleFd = SocketHandleFd;
    _Cancelled = FALSE;
}

KAWIpcPipe::AsyncSendData::~AsyncSendData()
{
#if !defined(PLATFORM_UNIX)
    CloseHandle(_IoCompleteEvent);
#endif
}

NTSTATUS KAWIpcPipe::CreateAsyncSendData(
    __out AsyncSendData::SPtr& Context)
{
    KAWIpcPipe::AsyncSendData::SPtr context;
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) KAWIpcPipe::AsyncSendData(*this, _SocketHandleFd);
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

ktl::Awaitable<NTSTATUS> KAWIpcPipe::SendDataAsync(
	__in BOOLEAN IsFdData,
    __in KBuffer& Data,
    __out ULONG& BytesSent,
    __in_opt KAsyncContextBase* const ParentAsyncContext
    )
{
    KCoService$ApiEntry(TRUE);
    
    NTSTATUS status;
    NTSTATUS status2;
    KAWIpcPipe::AsyncSendData::SPtr context;
    KBuffer::SPtr data = &Data;

    status = CreateAsyncSendData(context);
    if (! NT_SUCCESS(status))
    {
        co_return status ;
    }   

    context->_Data = &Data;
    context->_BytesSent = &BytesSent;
	context->_SendFd = IsFdData;

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

    status = AddOperation(*context);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        co_return(status);
    }
    
    status = co_await *awaiter;

    status2 = RemoveOperation(*context);
    KInvariant(NT_SUCCESS(status2));
    co_return status;       
}

VOID KAWIpcPipe::AsyncSendData::Execute(
    )
{
    //
    // For safety, keep a ref on this for the lifetime of the whole
    // routine.
    //
    KAWIpcPipe::AsyncSendData::SPtr thisPtr = this;
    
#if !defined(PLATFORM_UNIX)
    //
    // TODO: Do not use a dedicated thread but instead use async
    // completion ports.
    //
    NTSTATUS status = STATUS_SUCCESS;
    BOOL b;
    DWORD error;
    ULONG bytes;

    memset(&_Overlapped, 0, sizeof(_Overlapped));
    _Overlapped.hEvent = _IoCompleteEvent;
    
    b = WriteFile(_SocketHandleFd.Get(),
                 _Data->GetBuffer(),
                 _Data->QuerySize(),
                 &bytes,
                 &_Overlapped);

    if (! b)
    {
        error = GetLastError();
        if (error != ERROR_IO_PENDING)
        {
            status = KAWIpcPipe::MapErrorToNTStatus(error);
            KTraceFailedAsyncRequest(status, this, (ULONGLONG)_SocketHandleFd.Get(), error);
            Complete(status);
            return;
        }
    } else {
        status = STATUS_SUCCESS;
        *_BytesSent = bytes;
        Complete(status);
        return;
    }

    while (! _Cancelled)
    {
        b = GetOverlappedResultEx(_SocketHandleFd.Get(),
                                &_Overlapped,
                                &bytes,
                                1000,
                                TRUE);
        if (! b)
        {
            error = GetLastError();
            if ((error == WAIT_IO_COMPLETION) || (error == WAIT_TIMEOUT))
            {
                continue;
            }

            status = KAWIpcPipe::MapErrorToNTStatus(error);
            KTraceFailedAsyncRequest(status, this, (ULONGLONG)_SocketHandleFd.Get(), error);
            Complete(status);
            return;
        } else {
            *_BytesSent = bytes;
            Complete(STATUS_SUCCESS);
            return;
        }                               
    }

    KDbgCheckpointWDataInformational(0,
                        "CancelIo", status, (ULONGLONG)this, (ULONGLONG)_SocketHandleFd.Get(), (ULONGLONG)_Pipe.RawPtr(), 0);
    
    b = CancelIoEx(_SocketHandleFd.Get(),
                   &_Overlapped);
    if (! b)
    {
        error = GetLastError();
        status = KAWIpcPipe::MapErrorToNTStatus(error);
        KTraceFailedAsyncRequest(status, this, (ULONGLONG)_SocketHandleFd.Get(), error);
    }
    
    KTraceFailedAsyncRequest(STATUS_CANCELLED, this, (ULONGLONG)_SocketHandleFd.Get(), 0);
    Complete(STATUS_CANCELLED);
#else
    NTSTATUS status;
    ssize_t bytesSent;
    int error;
    ULONG retries = 0;

	void* cmsgBuf;
	struct msghdr msg = { 0 };
	struct cmsghdr *cmsg;
	
	if (_SendFd)
	{
		int *fdPtrSource;
		int *fdPtr;
		ULONG fdCount;

		fdPtrSource = (int*)_Data->GetBuffer();
		fdCount = _Data->QuerySize() / sizeof(int);
		
		int cmsgBufLen = CMSG_SPACE(fdCount * sizeof(int));		
		cmsgBuf = (void*) _new(GetThisAllocationTag(), GetThisAllocator()) UCHAR[cmsgBufLen];
		if (cmsgBuf == nullptr)
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
			KTraceFailedAsyncRequest(status, this, _SocketHandleFd.Get(), cmsgBufLen);    
			Complete(status);
			return;
		}

		msg.msg_control = (char*)cmsgBuf;
		msg.msg_controllen = cmsgBufLen;
		cmsg = CMSG_FIRSTHDR(&msg);
		cmsg->cmsg_level = SOL_SOCKET;
		cmsg->cmsg_type = SCM_RIGHTS;
		cmsg->cmsg_len = CMSG_LEN(sizeof(int) * fdCount);
		fdPtr = (int *) CMSG_DATA(cmsg);    /* Initialize the payload */
		for (ULONG i = 0; i < fdCount; i++)
		{
			fdPtr[i] = fdPtrSource[i];
		}
	}
	
    //
    // Wait for writing to be possible
    //
    while (! _Cancelled)
    {
        struct pollfd p;
        p.fd = _SocketHandleFd.Get();
        p.events = POLLOUT;
        p.revents = 0;

        //
        // TODO: Rather than use a timeout, use ppoll and have this get
        // notified of a cancel via a signal via pthread_kill. Note
        // that there is a race condition and so you would use
        // InterlockedExchange to swap cancel flag and PID.
        //
        // Or event better, use a single thread that waits on all
        // sockets and work with the async to send without the use of a
        // separate thread or blocking call.
        //
        error = poll(&p, 1, 1000);
        if (error == 1)
        {
			if (_SendFd)
			{
				bytesSent = sendmsg(_SocketHandleFd.Get(), &msg, MSG_EOR | MSG_DONTWAIT);		
			} else {
				bytesSent = send(_SocketHandleFd.Get(), _Data->GetBuffer(), _Data->QuerySize(), 0);
			}
            if (bytesSent == -1)
            {
                //
                // Error occured on send
                //
                error = errno;
                status = LinuxError::LinuxErrorToNTStatus(error);
                KTraceFailedAsyncRequest(status, this, _SocketHandleFd.Get(), error);
                break;
            } else {
                _Pipe->_SendCount++;
                *_BytesSent = (ULONG)bytesSent;
                status = STATUS_SUCCESS;
                break;
            }
        } else if (error == 0) {
            //
            // Timeout, try again
            //
            retries++;
            if (retries == 15)
            {
                KTraceFailedAsyncRequest(status, this, _SocketHandleFd.Get(), (ULONGLONG)_Pipe.RawPtr());
                retries = 0;
            }
            continue;
        } else {
            //
            // Error occured on poll
            //
            error = errno;
            status = LinuxError::LinuxErrorToNTStatus(error);
            KTraceFailedAsyncRequest(status, this, _SocketHandleFd.Get(), error);
            break;
        }
    }
        
    if (_Cancelled)
    {
        status = STATUS_CANCELLED;
        KTraceFailedAsyncRequest(status, this, _SocketHandleFd.Get(), 0);
    }

	if (_SendFd)
	{
		_delete(cmsgBuf);
	}
		
    Complete(status);
    return;
    
#endif  
}

VOID KAWIpcPipe::AsyncSendData::OnReuse(
    )
{
    _Cancelled = FALSE;
}

VOID KAWIpcPipe::AsyncSendData::OnCancel(
    )
{
    KTraceCancelCalled(this, TRUE, FALSE, 0);
    _Cancelled = TRUE;
}


KAWIpcPipe::AsyncReceiveData::AsyncReceiveData()
{
    KInvariant(FALSE);
}

KAWIpcPipe::AsyncReceiveData::AsyncReceiveData(
    __in KAWIpcPipe& Pipe,                                             
    __in SOCKETHANDLEFD SocketHandleFd                                       
    )
{
#if !defined(PLATFORM_UNIX)
    _IoCompleteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (_IoCompleteEvent == nullptr)
    {
        SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
        return;
    }
#endif
    _Pipe = &Pipe;
    _SocketHandleFd = SocketHandleFd;
    _Cancelled = FALSE;
}

KAWIpcPipe::AsyncReceiveData::~AsyncReceiveData()
{
#if !defined(PLATFORM_UNIX)
    CloseHandle(_IoCompleteEvent);
#endif
}

NTSTATUS KAWIpcPipe::CreateAsyncReceiveData(
    __out AsyncReceiveData::SPtr& Context)
{
    KAWIpcPipe::AsyncReceiveData::SPtr context;
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) KAWIpcPipe::AsyncReceiveData(*this, _SocketHandleFd);
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

ktl::Awaitable<NTSTATUS> KAWIpcPipe::ReceiveDataAsync(
	__in BOOLEAN IsFdData,
    __inout KBuffer& Data,
    __out ULONG& BytesReceived,
    __in_opt KAsyncContextBase* const ParentAsyncContext
    )
{
    KCoService$ApiEntry(TRUE);
    
    NTSTATUS status;
    NTSTATUS status2;
    KAWIpcPipe::AsyncReceiveData::SPtr context;
    KBuffer::SPtr data = &Data;

    status = CreateAsyncReceiveData(context);
    if (! NT_SUCCESS(status))
    {
        co_return status ;
    }   

    context->_Data = &Data;
    context->_BytesReceived = &BytesReceived;
	context->_ReceiveFd = IsFdData;

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

    status = AddOperation(*context);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        co_return status;
    }    
    
    status = co_await *awaiter;

    status2 = RemoveOperation(*context);
    KInvariant(NT_SUCCESS(status2));
    
    co_return status;       
}

VOID KAWIpcPipe::AsyncReceiveData::Execute(
    )
{
    //
    // For safety, keep a ref on this for the lifetime of the whole
    // routine.
    //
    KAWIpcPipe::AsyncReceiveData::SPtr thisPtr = this;
    
#if !defined(PLATFORM_UNIX)
    //
    // TODO: Do not use a dedicated thread but instead use async
    // completion ports.
    //
    NTSTATUS status = STATUS_SUCCESS;
    BOOL b;
    DWORD error;
    ULONG bytes;

    while (! _Cancelled)
    {
        memset(&_Overlapped, 0, sizeof(_Overlapped));
        _Overlapped.hEvent = _IoCompleteEvent;

        b = ReadFile(_SocketHandleFd.Get(),
                     _Data->GetBuffer(),
                     _Data->QuerySize(),
                     &bytes,
                     &_Overlapped);

        if (! b)
        {
            error = GetLastError();
            if (error == ERROR_MORE_DATA) {
                if (bytes == 0)
                {
                    KTraceFailedAsyncRequest(status, this, (ULONGLONG)_SocketHandleFd.Get(), bytes);
                    KNt::Sleep(50);
                    continue;
                }
                *_BytesReceived = bytes;
                Complete(STATUS_SUCCESS);
                return;             
            } else if (error != ERROR_IO_PENDING) {
                status = KAWIpcPipe::MapErrorToNTStatus(error);
                KTraceFailedAsyncRequest(status, this, (ULONGLONG)_SocketHandleFd.Get(), error);
                Complete(status);
                return;
            }

        } else {
            *_BytesReceived = bytes;
            status = STATUS_SUCCESS;
            Complete(status);
            return;
        }

        while (! _Cancelled)
        {
            b = GetOverlappedResultEx(_SocketHandleFd.Get(),
                                    &_Overlapped,
                                    &bytes,
                                    1000,
                                    TRUE);
            if (! b)
            {
                error = GetLastError();
                if ((error == WAIT_IO_COMPLETION) || (error == WAIT_TIMEOUT))
                {
                    continue;
                }

                if (error == ERROR_MORE_DATA)
                {
                    *_BytesReceived = bytes;
                    Complete(STATUS_SUCCESS);
                    return;             
                }

                status = KAWIpcPipe::MapErrorToNTStatus(error);
                KTraceFailedAsyncRequest(status, this, (ULONGLONG)_SocketHandleFd.Get(), error);
                Complete(status);
                return;
            } else {
                *_BytesReceived = bytes;
                Complete(STATUS_SUCCESS);
                return;
            }                               
        }
    }

    KDbgCheckpointWDataInformational(0,
                        "CancelIo", status, (ULONGLONG)this, (ULONGLONG)_SocketHandleFd.Get(), (ULONGLONG)_Pipe.RawPtr(), 0);
    
    b = CancelIoEx(_SocketHandleFd.Get(),
                   &_Overlapped);
    if (! b)
    {
        error = GetLastError();
        status = KAWIpcPipe::MapErrorToNTStatus(error);
        KTraceFailedAsyncRequest(status, this, (ULONGLONG)_SocketHandleFd.Get(), error);
    }
    
    KTraceFailedAsyncRequest(STATUS_CANCELLED, this, (ULONGLONG)_SocketHandleFd.Get(), 0);
    Complete(STATUS_CANCELLED);
#else
    NTSTATUS status;
    int error;
    ssize_t bytesReceived;

    //
    // Wait for writing to be possible
    //
    ULONG retries = 0;

	void* cmsgBuf;
	struct msghdr msgh = { 0 };
	struct cmsghdr *cmhp;
	struct iovec iov;
	int *fdPtrDest;
	ULONG fdCount;
	int* data;
	
	if (_ReceiveFd)
	{
		int cmsgBufLen;
		struct cmsghdr* cmsg;

		fdPtrDest = (int*)_Data->GetBuffer();
		fdCount = _Data->QuerySize() / sizeof(int);
		
		cmsgBufLen = CMSG_SPACE(fdCount * sizeof(int));		
		cmsgBuf = (void*) _new(GetThisAllocationTag(), GetThisAllocator()) UCHAR[cmsgBufLen];
		if (cmsgBuf == nullptr)
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
			KTraceFailedAsyncRequest(status, this, _SocketHandleFd.Get(), cmsgBufLen);    
			Complete(status);
			return;
		}

		data = (int*) _new(GetThisAllocationTag(), GetThisAllocator()) int[fdCount];
		if (data == nullptr)
		{
			_delete(cmsgBuf);
			status = STATUS_INSUFFICIENT_RESOURCES;
			KTraceFailedAsyncRequest(status, this, _SocketHandleFd.Get(), cmsgBufLen);    
			Complete(status);
			return;
		}
				
		cmsg = (struct cmsghdr*)cmsgBuf;

		cmsg->cmsg_len = CMSG_LEN(fdCount * sizeof(int));
		cmsg->cmsg_level = SOL_SOCKET;
		cmsg->cmsg_type = SCM_RIGHTS;

		/* Set 'msgh' fields to describe 'control_un' */

		msgh.msg_control = (char*)cmsgBuf;
		msgh.msg_controllen = cmsgBufLen;

		/* Set fields of 'msgh' to point to buffer used to receive (real)
		   data read by recvmsg() */

		msgh.msg_iov = &iov;
		msgh.msg_iovlen = 1;
		iov.iov_base = data;
		iov.iov_len = fdCount * sizeof(int);

		msgh.msg_name = nullptr;               /* We don't need address of peer */
		msgh.msg_namelen = 0;
	}
	
    while (! _Cancelled)
    {
        struct pollfd p;
        p.fd = _SocketHandleFd.Get();
        p.events = POLLIN;
        p.revents = 0;

        //
        // TODO: Rather than use a timeout, use ppoll and have this get
        // notified of a cancel via a signal via pthread_kill. Note
        // that there is a race condition and so you would use
        // InterlockedExchange to swap cancel flag and PID
        //
        // Or event better, use a single thread that waits on all
        // sockets and work with the async to send without the use of a
        // separate thread or blocking call.
        //
        error = poll(&p, 1, 1000);
        if (error == 1)
        {
			if (_ReceiveFd)
			{
				bytesReceived = recvmsg(_SocketHandleFd.Get(), &msgh, 0);
				if (bytesReceived != -1)
				{
					cmhp = CMSG_FIRSTHDR(&msgh);
					KInvariant(cmhp != nullptr);
					KInvariant(cmhp->cmsg_len == CMSG_LEN(fdCount * sizeof(int)));
					KInvariant(cmhp->cmsg_level == SOL_SOCKET);
					KInvariant(cmhp->cmsg_type == SCM_RIGHTS);

					int *fdPtr = (int *)CMSG_DATA(cmhp);
					for (ULONG i = 0; i < fdCount; i++)
					{
						fdPtrDest[i] = fdPtr[i];
					}					
				}
			} else {
				bytesReceived = recv(_SocketHandleFd.Get(), _Data->GetBuffer(), _Data->QuerySize(), 0);
			}
            if (bytesReceived == -1)
            {
                //
                // Error occured on receive
                //
                error = errno;
                status = LinuxError::LinuxErrorToNTStatus(error);
                KTraceFailedAsyncRequest(status, this, _SocketHandleFd.Get(), error);
                break;
            } else if ((bytesReceived == 0) && (! _ReceiveFd)) {
                if ((p.revents & (POLLERR | POLLHUP | POLLRDHUP)) != 0)
                {
                    status = STATUS_PIPE_BROKEN;
                    KTraceFailedAsyncRequest(status, this, _SocketHandleFd.Get(), error);
                    break;
                } else {
                    //
                    // Not sure why we woke up
                    //
					
                    KAssert(FALSE);
                    KTraceFailedAsyncRequest(status, this, (ULONGLONG)_SocketHandleFd.Get(), 0);
                    KNt::Sleep(50);
                    continue;
                }
            } else {
                _Pipe->_ReceiveCount++;
                *_BytesReceived = (ULONG)bytesReceived;
                status = STATUS_SUCCESS;
                break;
            }
        } else if (error == 0) {
            //
            // Timeout, try again
            //
            retries++;
            if (retries == 15)
            {
                KTraceFailedAsyncRequest(status, this, (ULONGLONG)_SocketHandleFd.Get(), (ULONGLONG)_Pipe.RawPtr());
                retries = 0;
            }
            continue;
        } else {
            //
            // Error occured on poll
            //
            error = errno;
            status = LinuxError::LinuxErrorToNTStatus(error);
            KTraceFailedAsyncRequest(status, this, _SocketHandleFd.Get(), error);
            break;
        }
    }

    if (_Cancelled)
    {
        status = STATUS_CANCELLED;
        KTraceFailedAsyncRequest(status, this, _SocketHandleFd.Get(), 0);
    }

	if (_ReceiveFd)
	{
		_delete(cmsgBuf);
		_delete(data);
	}
	
	
    Complete(status);
    return;
    
#endif  
}

VOID KAWIpcPipe::AsyncReceiveData::OnReuse(
    )
{
    _Cancelled = FALSE;
}

VOID KAWIpcPipe::AsyncReceiveData::OnCancel(
    )
{
    KTraceCancelCalled(this, TRUE, FALSE, 0);
    _Cancelled = TRUE;
}

KAWIpcPipe::KAWIpcPipe() :
   _OperationsList(_OperationLinkageOffset)
{
    _SocketHandleFd = (SOCKETHANDLEFD)0;
    _SendCount = 0;
    _ReceiveCount = 0;
}

KAWIpcPipe::~KAWIpcPipe()
{
    KInvariant(_SocketHandleFd == (SOCKETHANDLEFD)0);
}

KAWIpcPipeAsyncOperation::KAWIpcPipeAsyncOperation()
{
}

KAWIpcPipeAsyncOperation::~KAWIpcPipeAsyncOperation()
{
}

VOID KAWIpcPipeAsyncOperation::OnCancel(
    )
{
    KTraceCancelCalled(this, TRUE, FALSE, 0);
}
