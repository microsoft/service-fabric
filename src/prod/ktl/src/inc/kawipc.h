/*++

    (c) 2016 by Microsoft Corp. All Rights Reserved.

    kawipc.h

    Description:
      Kernel Template Library (KTL): Cross process IPC mechanism

      These classes implement a high performance cross process IPC
      mechanism that can be used to implement a primitive RPC mechanism
      between two processes or between many client processes and a
      single server process.

      The mechanism has the following classes:

      KAWIpcSharedMemory - this class represents a shared memory
                           section and provides lifecycle services for
                           it along with offset to pointer mappings.

      KAWIpcEventKicker
      KAWIpcEventWaiter - these classes represent a cross process
                          eventing mechanism that can be used by a
                          message producer to awaken a message
                          consumer.

      KAWIpcPipe*       - these classes implement a functionality that
                          is like Windows named pipes. It is used as a
                          way for clients to discover and connect to
                          the server and then request resources from
                          the server such as shared memory,
                          EventWaiters, etc.

      KAWIpcKIoBufferElement
      KAWIpcPageAlignedHeap - these classes implement a paged aligned
                              heap on top of a KAWIpcSharedMemory
                              object. Heap allocations are supplied as
                              KIoBuffers where each KIoBufferElement is
                              a KAWIpcKIoBufferElement that overlays
                              memory in the KAWIpcSharedMemory. Note
                              that only one process may access the
                              KAWIpcPageAlignedHeap - it is not safe to
                              use cross process.

      KAWIpcMessage - Header for messages passed via IPC mechanism.

      
      KAWIpcLIFOList - LIFO list that is accessible by multiple
                       processes at the same time. It is used as a
                       global free of KAWIpcMessage. This class
                       maintains lists of different fixed sized
                       messages.

      KAWIpcMessageAllocator - Asynchronous allocator from the global
                               free list of KAWIpcMessage.


      KAWIpcRing
      KAWIpcRingConsumer
      KAWIpcRingProducer - These classes implement a queue for passing
                           KAWIpcMessages from the producer to the
                           consumer. The producer and consumer may be
                           in different processes. The implementation
                           is by a fixed sized ring buffer.

    History:
      alanwar          25-APR-2017         Initial version.

--*/

#pragma once

#if defined(PLATFORM_UNIX)
#include <palerr.h>
#endif

#include <KTpl.h>

#if defined(K_UseResumable)

using namespace ktl;
using namespace ktl::io;
using namespace ktl::kasync;
using namespace ktl::kservice;

//*********************************************************************
// Primitives/Platform encapsualtions. There is no IPC logic in these
// classes; IPC is built on top of them. These classes could be moved
// into a "PAL" layer at some point as they abstract system
// functionality that varies from Windows to Linux.
//
//    * Shared memory
//    * Thread wakeup
//    * Pipe/Socket
//*********************************************************************

//
// Platform independent type for identifying a shared memory section
//
class SHAREDMEMORYID : public KStrongType<SHAREDMEMORYID, KGuid>
{
    K_STRONG_TYPE(SHAREDMEMORYID, KGuid);

public:
    SHAREDMEMORYID() {};
    ~SHAREDMEMORYID() {};    
};


//
// Platform independent type for identifying an eventing object
//
#if !defined(PLATFORM_UNIX)
class EVENTHANDLEFD : public KStrongType<EVENTHANDLEFD, HANDLE>
{
    K_STRONG_TYPE(EVENTHANDLEFD, HANDLE);

public:
    EVENTHANDLEFD() {};
    ~EVENTHANDLEFD() {};
};

//
// Platform independent type for identifying a pipe socket
//
class SOCKETHANDLEFD : public KStrongType<SOCKETHANDLEFD, HANDLE>
{
    K_STRONG_TYPE(SOCKETHANDLEFD, HANDLE);

public:
    SOCKETHANDLEFD() {};
    ~SOCKETHANDLEFD() {};
};

#else
class EVENTHANDLEFD : public KStrongType<EVENTHANDLEFD, int>
{
    K_STRONG_TYPE(EVENTHANDLEFD, int);

public:
    EVENTHANDLEFD() {};
    ~EVENTHANDLEFD() {};
};

class SOCKETHANDLEFD : public KStrongType<SOCKETHANDLEFD, int>
{
    K_STRONG_TYPE(SOCKETHANDLEFD, int);

public:
    SOCKETHANDLEFD() {};
    ~SOCKETHANDLEFD() {};
};
#endif

//
// Platform encapsulation for functionality to execute a work item on a
// thread that is outside of the KTL thread pools.
//
template<class T> class SystemThreadWorker
{
    public:
        
    NTSTATUS StartThread()
    {
#if !defined(PLATFORM_UNIX)
        NTSTATUS status = STATUS_SUCCESS;
        ULONG error;
        BOOL b;
        b = QueueUserWorkItem(T::ThreadProcA, (T*)this, WT_EXECUTEDEFAULT);
        
        if (! b)
        {
            error = GetLastError();
            status = KAWIpcPipe::MapErrorToNTStatus(error);
            KTraceFailedAsyncRequest(status, nullptr, (ULONGLONG)this, (ULONGLONG)error);
        }
        return(status);
#else
        NTSTATUS status = STATUS_SUCCESS;
        int error;
        pthread_t thread;
        pthread_attr_t attr;
        int stack_size = 128 * 1024;
        void *sp;

        error = pthread_attr_init(&attr);
        if (error != 0)
        {
            status = LinuxError::LinuxErrorToNTStatus(error);           
            KTraceFailedAsyncRequest(status, nullptr, (ULONGLONG)this, (ULONGLONG)error);
            return(status);
        }
		KFinally([&]() {
			pthread_attr_destroy(&attr);
		});

        error = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        if (error != 0)
        {
            status = LinuxError::LinuxErrorToNTStatus(error);           
            KTraceFailedAsyncRequest(status, nullptr, (ULONGLONG)this, (ULONGLONG)error);
            return(status);
        }

	    error = pthread_attr_setstacksize(&attr, stack_size);
        if (error != 0)
        {
            status = LinuxError::LinuxErrorToNTStatus(error);           
            KTraceFailedAsyncRequest(status, nullptr, (ULONGLONG)this, (ULONGLONG)error);
            return(status);
        }

        error = pthread_create(&thread, &attr, T::ThreadProcA, (T*)this);
        if (error != 0)
        {
            status = LinuxError::LinuxErrorToNTStatus(error);           
            KTraceFailedAsyncRequest(status, nullptr, (ULONGLONG)this, (ULONGLONG)error);
            return(status);
        }

        return(status);
#endif
    }
    
    protected:
#if !defined(PLATFORM_UNIX)
    static DWORD WINAPI ThreadProcA(_In_ LPVOID lpParameter)
    {
        ((T*)lpParameter)->Execute();
        return 0;
    }
#else
    static void* ThreadProcA(void* context)
    {
        ((T*)context)->Execute();
        return(nullptr);
    }
#endif

    protected:
        
    NTSTATUS OnStartHook(
        )
    {
        NTSTATUS status;
                
        status = StartThread();
        return(status);
    }    
};

//
// This class owns or accesses a shared memory section and provides mapping between
// offsets and process specific pointers. All offsets in the shared memory section are
// based on the ServerBase, that is, the offset from the beginning of
// the shared memory section as mapped in the process designated as the server.
//
// Other processes are designated as clients however will only map a part of the shared memory that is
// mapped in the server's space and thus the offsets in that shared
// memory will not be relative to the client's memory base address. The
// client needs to know the ClientBaseOffset which is the offset from the
// ServerBase to the section shared with this client. In this way the
// client can accurately map from offset to pointer.
//
// For example, lets say we have an object at an offset of 0xB040 recorded in the
// shared memory and the server maps the shared memory section at base
// address 0x10000000 then the pointer to that object in
// the server process would be 0x1000B040.  If a specific client maps the part
// of shared memory section that starts at 0x8000 at a base address of
// 0x20000000 then the ClientBaseOffset would be 0x8000 and the pointer to
// the object in that client's process be (0x20000000 + 0xB040 - 0x8000).
//
// It is expected that all mapping between offsets to pointers are
// through this object so higher level classes may use pointers only.
//
//
class KAWIpcSharedMemory : public KObject<KAWIpcSharedMemory>, public KShared<KAWIpcSharedMemory>
{
    K_FORCE_SHARED(KAWIpcSharedMemory);

    private:
        KAWIpcSharedMemory(
            __in ULONG Size    
            );

        KAWIpcSharedMemory(
            __in SHAREDMEMORYID SharedMemoryId,
            __in ULONG StartingOffset,
            __in ULONG Size,
            __in ULONG ClientBaseOffset
            );
    
    public:
        //
        // Allocate a new shared memory section and Create a
        // KAWIpcSharedMemory object that and setup the ServerBase
        // and ClientBaseOffset to the location where it was mapped. In the
        // KAWIpc case, this is typically used by the server to
        // allocate shared memory area for a client or global shared
        // memory. When an object created by this call is destructed
        // then the underlying shared memory object is unmapped and
        // deleted.
        //
        // Parameters:
        //
        //     Size - must be on a page aligned boundary
        //
        static NTSTATUS Create(
            __in KAllocator& Allocator,
            __in ULONG AllocationTag,
            __in ULONG Size,
            __out KAWIpcSharedMemory::SPtr& Context
        );

        //
        // Create an object that maps over an already existing shared
        // memory section. This is typically done by the client. When
        // the object created by this api is destructed then the
        // underlying shared memory object is unmapped.
        //
        // Parameters:
        //
        //     StartingOffset - this is the offset within the shared
        //                      memory section that is provided for the
        //                      client's use.
        //     Size - must be on a page aligned boundary
        //     ClientBaseOffset - specifies the offset of the beginning of
        //                        the section in the server's memory map
        //                        space.
        //
        static NTSTATUS Create(
            __in KAllocator& Allocator,
            __in ULONG AllocationTag,
            __in SHAREDMEMORYID SharedMemoryId,
            __in ULONG StartingOffset,
            __in ULONG Size,
            __in ULONG ClientBaseOffset,
            __out KAWIpcSharedMemory::SPtr& Context
        );

#if defined(PLATFORM_UNIX)
        //
        // Remove any shared memory sections that might not have been
        // cleaned up previously due to a crash. This would typically
        // be done when the server restarts.
        //
        static ktl::Awaitable<NTSTATUS> CleanupFromPreviousCrashes(
            __in KAllocator& Allocator,
            __in ULONG AllocationTag
            );

        // 
        // This is for testing purposes only
        //
        __inline VOID SetOwnSection(__in BOOLEAN OwnSection) { _OwnSection = OwnSection; };
#endif
        
        __inline SHAREDMEMORYID GetId() { return(_Id); };
        __inline ULONG GetSize() { return(_Size); };
        __inline PVOID GetBaseAddress() { return(_BaseAddress); };

        //
        // Map the offset of an object in the shared memory section to
        // a pointer that is valid relative to the processing owning
        // this KAWIpcSharedMemory.
        //
        __inline PVOID OffsetToPtr(
            __in ULONG Offset
        )
        {
            PUCHAR p;

            p = (PUCHAR)_BaseAddress + (Offset - _ClientBaseOffset); 

            return((PVOID)p);
        }

        //
        // Map the pointer to an object in the shared memory section
        // relative to the processing owning this KAWIpcSharedMemory to
        // an offset relative to the server and thus suitable for
        // storing in the shared memory.
        //
        __inline ULONG PtrToOffset(
            __in PVOID Pointer
        )
        {
            ULONG o;
            KInvariant(((ULONG_PTR)Pointer >= (ULONG_PTR)_BaseAddress) &&
                       ((ULONG_PTR)Pointer < ((ULONG_PTR)_BaseAddress + _Size)));
            
            o = (ULONG)((ULONG_PTR)Pointer - (ULONG_PTR)_BaseAddress) + _ClientBaseOffset;
            return(o);
        }

        static WCHAR const * const _SharedMemoryNamePrefix;
        static CHAR const * const _SharedMemoryNamePrefixAnsi;
        
    private:
        static const ULONG _SharedMemoryNameLength = 128;
        NTSTATUS CreateSectionName(
            __in SHAREDMEMORYID SharedMemoryId,
            __out KDynString& Name
            );
        

    private:
        SHAREDMEMORYID _Id;
        ULONG _Size;
        BOOLEAN _OwnSection;

        // This is the offset from the begining of the ServerBase to
        // this section in the Server memory map.
        ULONG _ClientBaseOffset;
        
        // This is the base address that the shared memory is mapped
        // into this process.
        PVOID _BaseAddress;
#if !defined(PLATFORM_UNIX)
        HANDLE _SectionHandle;
#else
        int _SectionFd;
#endif
};

//
// This class encapsulates a mechanism to wakeup a peer.
// This would typically be used by a component that is awaiting
// some action, for example, an entry added
// to a list of operations. The async completes when the associated
// event handle or fd is signalled. On Linux it may use eventfd or KXM
// syncfile while on Windows it would use a named event. On destruction
// the EventHandle or EventFd is closed. Creation and deletion of the
// underlying eventfd or named event is not done by the constructor of
// this class but is done by the user of the class through the
// CreateEventWaiter and CleanupEventWaiter static methods.
//
class KAWIpcEventWaiter : public KAsyncContextBase, public SystemThreadWorker<KAWIpcEventWaiter>
{
    K_FORCE_SHARED(KAWIpcEventWaiter);
    friend SystemThreadWorker<KAWIpcEventWaiter>;

    public:
        static NTSTATUS Create(
            __in KAllocator& Allocator,
            __in ULONG AllocatorTag,
            __out KAWIpcEventWaiter::SPtr& Context
        );

        //
        // Platform independent method that creates an event used by
        // KAWIpcEventWaiter.
        //
        // Parameters:
        //
        //     Name must be a unique name for the event
        //
        static NTSTATUS CreateEventWaiter(
            __in_opt PWCHAR Name,
            __out EVENTHANDLEFD& EventHandleFd
            );

        //
        // Platform independent method that closes an event used by
        // KAWIpcEventWaiter.
        //
        static NTSTATUS CleanupEventWaiter(
            __in EVENTHANDLEFD EventHandleFd
            );

        //
        // KAsyncContext start method to await the event being
        // signalled.
        //
        // Parameters:
        //
        //     EventHandleFd is the event identifier returned by CreateEventWaiter
        //
        VOID StartWait(
            __in EVENTHANDLEFD EventHandleFd,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt CompletionCallback CallbackPtr
        );

        //
        // Coroutine method to await the event being signalled.
        //
        // Parameters:
        //
        //     EventHandleFd is the event identifier returned by CreateEventWaiter
        //
        ktl::Awaitable<NTSTATUS> WaitAsync(
            __in EVENTHANDLEFD EventHandleFd,
            __in_opt KAsyncContextBase* const ParentAsyncContext
            );

    protected:
        VOID
        OnCancel(
            )  override ;
                
        VOID Execute() ;
                
        VOID
        OnStart(
            )
        {
            NTSTATUS status = SystemThreadWorker<KAWIpcEventWaiter>::OnStartHook();
            if (! NT_SUCCESS(status))
            {
                Complete(status);
            }
        }

        VOID
        OnReuse(
            )  override ;

    private:
        EVENTHANDLEFD _EventHandleFd;
        BOOLEAN _Cancel;
        static const ULONG _PollIntervalInMs = 2000;
#if !defined(PLATFORM_UNIX)
        ULONG _LastError;
#else
        int _LastError;
#endif
};

//
// This class encapsulates signalling an event to provide a wakeup
// from the client or server to another client or server. This would
// typically be used by a component that is producing something for the
// waiter to process and wants to wake up the waiter, for example, an entry added
// to a list of operations. On Linux it may use eventfd or KXM
// syncfile while on Windows it would use a named event. On destruction
// the EventHandle or EventFd is closed. The EventHandleFd used would
// be created by KAWIpcEventWaiter::CreateEventWaiter and closed with
// KAWIpcEventWaiter::CleanupEventWaiter.
//
class KAWIpcEventKicker : public KObject<KAWIpcEventKicker>, public KShared<KAWIpcEventKicker>
{
    K_FORCE_SHARED(KAWIpcEventKicker);

    KAWIpcEventKicker(
        __in EVENTHANDLEFD EventHandleFd
    );
    
    public:

        //
        // Create a KAWIpcEventKicker object that is associated with a
        // specific EventHandleFd.
        //
        // Parameters:
        //
        //     EventHandleFd is the identifier for the event to set
        //     when Kick() method is called.
        //
        static NTSTATUS Create(
            __in KAllocator& Allocator,
            __in ULONG AllocatorTag,
            __in EVENTHANDLEFD EventHandleFd,
            __out KAWIpcEventKicker::SPtr& Context
        );

        //
        // This will cause the event associated with this class to be
        // signalled and awake any listener waiting for it to be set.
        //
        NTSTATUS Kick();

        __inline EVENTHANDLEFD GetEventHandleFd() { return(_EventHandleFd); };

    private:
        EVENTHANDLEFD _EventHandleFd;
};

////////////////////////////////////////////////////////////////////////////////////////
// KAWIpc Pipe functionality
//
//     This provides a mechanism for communications between two
//     different processes via a named pipe (windows) or domain socket
//     (linux). Classes have been abstracted to an extent so that it is
//     easy to build components that layer on top of the pipe
//     effectively.
//
//     Typically your server would inherit from KAWIpcServer and implement a custom
//     IncomingConnectionHandler() Task which would perform the work of
//     creating a per connection data structure and potentially
//     negotiating with the incoming client. There is a base class for
//     the per connection data structure - KAWIpcServerClientConnection
//     and it is expected that your server would override that as well.
//
//     The base classes will provide your server code with a
//     KAWIpcServerPipe which is used to communicate with the client.
//     The base classes will also control the lifetime of the
//     KAWIpcServerPipe and the KAWIpcServerClientConnection (or derivative).
//
//     Depending upon the complexity of your client, you may override
//     KAWIpcClient and KAWIpcClientServerConnection classes and those
//     serve similar purposes as that of the server. Client would use
//     KAWIpcClientPipe to communicate with the server.

//
// This class encapsulates the server side endpoint to which clients
// initiate connections and start the IPC process. On Linux this would
// be a domain socket endpoint and on Windows a named pipe endpoint.
// A single instance of this class is created by KAWIpcServer and 
// when this KAWIpcServerPipeEndpoint is started and there is an incoming connection,
// this class will create a KAWIpcServerPipe for the server end of the
// pipe and a KAWIpcServerClientConnection service and start
// that.
//
// This is a traditional "listener" role.
//

class KAWIpcServer;
class KAWIpcServerClientConnection;
class KAWIpcServerPipe;

class KAWIpcServerPipeEndpoint : public KAsyncServiceBase
{
    K_FORCE_SHARED(KAWIpcServerPipeEndpoint);
    friend KAWIpcServer;

    public:
        //
        // Starts listening at the SocketAddress.
        //
        // Parameters:
        //
        //     Server is the KAWIpcServer to be associated with this
        //         endpoint.
        //
        //     SocketAddress is a unique number that represents the
        //         well-known socket endpoint to which clients connect.
        //
        ktl::Awaitable<NTSTATUS> OpenAsync(
            __in KAWIpcServer& Server,
            __in ULONGLONG SocketAddress,
            __in_opt KAsyncContextBase* const ParentAsync = nullptr,
            __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr
        );

        //
        // Shuts down the listener
        //
        ktl::Awaitable<NTSTATUS> CloseAsync();

    protected:          
        VOID OnServiceOpen() override ;
        VOID OnServiceClose() override ;

    private:
        ktl::Task OpenTask();
        ktl::Task CloseTask();

        //
        // TODO: Since there is not yet first class support for sockets in the
        // KXM on linux for socket completions through the completion port
        // model, we need to have a dedicated thread that services the
        // domain socket. Although this support is available on
        // windows, we do not use it but instead rely on mechanisms
        // common to linux and windows.
        //
        static VOID ServerWorker(
            __in PVOID Context
        );

        VOID ServerWorkerRoutine();

        ktl::Awaitable<VOID> ShutdownServerWorker();

        
        ktl::Task ProcessIncomingConnection(
            __in SOCKETHANDLEFD SocketHandleFd
            );
        
        NTSTATUS CreateServerPipe(
            __out KSharedPtr<KAWIpcServerPipe>& Context
        );

        ktl::Awaitable<NTSTATUS> WaitForIncomingConnectionAsync(
            __in SOCKETHANDLEFD ListeningSocketHandleFd,
            __out SOCKETHANDLEFD& SocketHandleFd,
            __in_opt KAsyncContextBase* const ParentAsyncContext
            );

        class AsyncWaitForIncomingConnection : public KAsyncContextBase, public SystemThreadWorker<AsyncWaitForIncomingConnection>
        {
            K_FORCE_SHARED(AsyncWaitForIncomingConnection);
            friend KAWIpcServerPipeEndpoint;
            friend SystemThreadWorker<AsyncWaitForIncomingConnection>;
            
            protected:
                VOID
                Execute() ;
                
                VOID
                OnStart(
                    )
                {
                    NTSTATUS status = SystemThreadWorker<AsyncWaitForIncomingConnection>::OnStartHook();
                    if (! NT_SUCCESS(status))
                    {
                        Complete(status);
                    }
                }

                VOID
                OnReuse(
                    )  override ;

                VOID
                OnCancel(
                    )  override ;

            private:
                SOCKETHANDLEFD _ListeningSocketHandleFd;
                SOCKETHANDLEFD* _SocketHandleFd;
        };
        
        NTSTATUS CreateAsyncWaitForIncomingConnection(
            __out AsyncWaitForIncomingConnection::SPtr& Context
            );
                        
    private:
        KSharedPtr<KAWIpcServer> _Server;
        ULONGLONG _SocketAddress;
        ktl::AwaitableCompletionSource<NTSTATUS>::SPtr _WorkerStartAcs;
        KAsyncEvent _WorkerThreadExit;
        KAsyncEvent::WaitContext::SPtr _WorkerThreadExitWait;
        KThread::SPtr _WorkerThread;

#if !defined(PLATFORM_UNIX)
        HANDLE _CancelThreadEvent;
        static const ULONG _ListenBacklog = 1024;
#else
        int _CancelThreadEventFd;
#endif
};

//
// This is the internal common base class for operations that are performed on a
// KAWIpcPipe derivation.
//
class KAWIpcPipeAsyncOperation : public KAsyncContextBase
{
    K_FORCE_SHARED_WITH_INHERITANCE(KAWIpcPipeAsyncOperation);

    protected:
        VOID OnCancel() override ;
    
    public:
        KListEntry _ListEntry;      
};

//
// This class exposes the operations that can be performed on
// KAWIpcServerPipe and KAWIpcClientPipe such as send, receive,
// rundown, etc.
//
class KAWIpcPipe : public KAsyncServiceBase
{
    K_FORCE_SHARED_WITH_INHERITANCE(KAWIpcPipe);
    public:
        class AsyncSendData;
        class AsyncReceiveData;
        friend AsyncSendData;
        friend AsyncReceiveData;

        //
        // This causes all operations that are outstanding on the pipe
        // to be cancelled.
        //
        NTSTATUS RundownAllOperations();

        //
        // This is a platform independent function that closes a socket
        // or pipe handle.
        //
        static VOID CloseSocketHandleFd(__in SOCKETHANDLEFD SocketHandleFd);
#if !defined(PLATFORM_UNIX)
        static NTSTATUS MapErrorToNTStatus(
            __in DWORD Error
            );

        static const ULONG _SocketNameLength = 128;
        
        static NTSTATUS GetSocketName(
            __in ULONGLONG SocketAddress,
            __out KDynString& Name
        );
#endif
        VOID SetSocketHandleFd(__in SOCKETHANDLEFD SocketHandleFd) { _SocketHandleFd = SocketHandleFd; };
        SOCKETHANDLEFD GetSocketHandleFd() { return(_SocketHandleFd); };

        //
        // Send data on the pipe to the peer.
        //
        // Parameters:
        //
		//     For Linux, it is posible to send a file descriptor over
		//     the pipe. If IsFdData is TRUE then the contents of Data
		//     is an array of file descriptors.
		//
        //     Data contains the data to be sent to the peer.
        //
        //     BytesSent returns the number of bytes sent to the peer.
        //
        ktl::Awaitable<NTSTATUS> SendDataAsync(
            __in BOOLEAN IsFdData,
            __in KBuffer& Data,
            __out ULONG& BytesSent,
            __in_opt KAsyncContextBase* const ParentAsyncContext = nullptr
            );
		
        class AsyncSendData : public KAWIpcPipeAsyncOperation, public SystemThreadWorker<AsyncSendData>
        {
            K_FORCE_SHARED(AsyncSendData);
            friend KAWIpcPipe;
            friend SystemThreadWorker<AsyncSendData>;

            AsyncSendData(__in KAWIpcPipe& Pipe, __in SOCKETHANDLEFD SocketHandleFd);
            
            protected:
                VOID
                Execute() ;

                VOID
                OnStart()
                {
                    NTSTATUS status = SystemThreadWorker<AsyncSendData>::OnStartHook();
                    if (! NT_SUCCESS(status))
                    {
                        Complete(status);
                    }
                }
                
                VOID
                OnReuse(
                    )  override ;

                VOID
                OnCancel(
                    )  override ;

            private:
                SOCKETHANDLEFD _SocketHandleFd;
                KAWIpcPipe::SPtr _Pipe;
                KBuffer::SPtr _Data;
                ULONG* _BytesSent;
                BOOLEAN _Cancelled;
				BOOLEAN _SendFd;
#if !defined(PLATFORM_UNIX)
                HANDLE _IoCompleteEvent;
                OVERLAPPED _Overlapped;
#endif
        };
        
        NTSTATUS CreateAsyncSendData(
            __out AsyncSendData::SPtr& Context
            );

        
        //
        // Receive data from the pipe to the peer.
        //
        // Parameters:
        //
		//     For Linux, it is posible to send a file descriptor over
		//     the pipe. If IsFdData is TRUE then the contents of Data
		//     is an array of file descriptors.
		//
        //     Data returns the data received from the peer. Note that
        //         Data must be allocated as the received data is
        //         copied into the contents of the KBuffer. 
        //
        //     BytesReceived returns the number of bytes received from the peer.
        //
        ktl::Awaitable<NTSTATUS> ReceiveDataAsync(
            __in BOOLEAN IsFdData,
            __inout KBuffer& Data,
            __out ULONG& BytesReceived,
            __in_opt KAsyncContextBase* const ParentAsyncContext = nullptr
            );      
		
        class AsyncReceiveData : public KAWIpcPipeAsyncOperation, public SystemThreadWorker<AsyncReceiveData>
        {
            K_FORCE_SHARED(AsyncReceiveData);
            friend KAWIpcPipe;
            friend SystemThreadWorker<AsyncReceiveData>;

            AsyncReceiveData(__in KAWIpcPipe& Pipe, __in SOCKETHANDLEFD SocketHandleFd);
            
            public:
                VOID StartReceiveData(
                    __inout KBuffer& Data,
                    __out ULONG& BytesReceived,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt CompletionCallback CallbackPtr
                );

            protected:
                VOID
                Execute();
                
                VOID
                OnStart()
                {
                    NTSTATUS status = SystemThreadWorker<AsyncReceiveData>::OnStartHook();
                    if (! NT_SUCCESS(status))
                    {
                        Complete(status);
                    }
                }
                
                VOID
                OnReuse(
                    )  override ;

                VOID
                OnCancel(
                    )  override ;

            private:
                SOCKETHANDLEFD _SocketHandleFd;
                KAWIpcPipe::SPtr _Pipe;
                KBuffer::SPtr _Data;
                ULONG* _BytesReceived;
                BOOLEAN _Cancelled;
				BOOLEAN _ReceiveFd;
#if !defined(PLATFORM_UNIX)         
                HANDLE _IoCompleteEvent;
                OVERLAPPED _Overlapped;
#endif
        };
        
        NTSTATUS CreateAsyncReceiveData(
            __out AsyncReceiveData::SPtr& Context);

    private:
        NTSTATUS AddOperation(
            __in KAWIpcPipeAsyncOperation& Operation
            );

        NTSTATUS RemoveOperation(
            __in KAWIpcPipeAsyncOperation& Operation
            );

    public:
        ULONG _SendCount;
        ULONG _ReceiveCount;
        
    protected:
        SOCKETHANDLEFD _SocketHandleFd;
    
    private:
        KNodeList<KAWIpcPipeAsyncOperation> _OperationsList;
        KSpinLock _OperationsListLock;
        static const ULONG _OperationLinkageOffset;
};

class KAWIpcServer;

//
// This class encapsulates the server side of a bidirectional "pipe" or "socket" between
// client and server and is used for negotiating the connection between
// client and server. On Linux it uses a domain socket while on windows
// it uses a named pipe. At this level the data passed is opaque; there
// is a higher level negotiation process in KAWIpcServerClientConnection. There is one of these
// instances per incoming client connection. An instance of this class is
// created for each incoming client connection and is created and
// started by KAWIpcServerPipeEndpoint and supplied to the user via the
// KAWIpcServer::IncomingClientConnection() method.
//
class KAWIpcServerPipe : public KAWIpcPipe
{
    K_FORCE_SHARED_WITH_INHERITANCE(KAWIpcServerPipe);
    friend KAWIpcServerPipeEndpoint;
    
    public:
        ktl::Awaitable<NTSTATUS> OpenAsync(
            __in KAWIpcServer& Server,
            __in SOCKETHANDLEFD SocketHandleFd,
            __in_opt KAsyncContextBase* const ParentAsync = nullptr,
            __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr
        );

        ktl::Awaitable<NTSTATUS> CloseAsync();

        KSharedPtr<KAWIpcServer> GetServer() { return(_Server); };
        
    protected:

        VOID OnServiceOpen() override ;
        VOID OnServiceClose() override ;
        VOID OnDeferredClosing() override ;

    private:

        //
        // Perform async work as a part of Open.
        // 
        ktl::Task OpenTask();
            
        //
        // Perform async work as a part of Close.
        //
        ktl::Task CloseTask();
        
    private:
        KSharedPtr<KAWIpcServer> _Server;
};

//
// This class encapsulates the client side of a bidirectional "pipe" or "socket" between
// client and server and is used for negotiating the connection between
// client and server. On Linux it uses a domain socket while on windows
// it uses a named pipe. At this level the data passed is opaque; there
// is a higher level negotiation process. This is created by the
// KAWIpcClient when it initiates a connection to the server and
// returned to the user in the KAWIpcClientServerConnetion after the
// KAWIpcClientServerConnection has successfully connected to the
// server.
//
class KAWIpcClient;
class KAWIpcClientPipe : public KAWIpcPipe
{
    K_FORCE_SHARED_WITH_INHERITANCE(KAWIpcClientPipe);
    friend KAWIpcClient;

    KAWIpcClientPipe(__in KAWIpcClient& Client);
    
    public:
        ktl::Awaitable<NTSTATUS> OpenAsync(
            __in ULONGLONG SocketAddress,
            __in_opt KAsyncContextBase* const ParentAsync = nullptr,
            __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr
        );

        ktl::Awaitable<NTSTATUS> CloseAsync();

        ktl::Awaitable<NTSTATUS> ConnectToServerAsync(
            __in ULONGLONG SocketAddress,
            __out SOCKETHANDLEFD& SocketHandleFd,
            __in_opt KAsyncContextBase* const ParentAsyncContext
            );

        SOCKETHANDLEFD GetSocketHandleFd() { return(_SocketHandleFd); };
        
        class AsyncConnectToServer : public KAsyncContextBase, public SystemThreadWorker<AsyncConnectToServer>
        {
            K_FORCE_SHARED(AsyncConnectToServer);
            friend KAWIpcClientPipe;
            friend SystemThreadWorker<AsyncConnectToServer>;

            public:
                AsyncConnectToServer(__in KAWIpcClientPipe& Pipe);
                
            protected:
                VOID
                Execute() ;
                
                VOID
                OnStart(
                    )
                {
                    NTSTATUS status = SystemThreadWorker<AsyncConnectToServer>::OnStartHook();
                    if (! NT_SUCCESS(status))
                    {
                        Complete(status);
                    }
                }

                VOID
                OnReuse(
                    )  override ;

                VOID
                OnCancel(
                    )  override ;

            private:
                ULONGLONG _SocketAddress;
                SOCKETHANDLEFD* _SocketHandleFd;
                KAWIpcClientPipe::SPtr _Pipe;
                BOOLEAN _Cancelled;
        };
        
        NTSTATUS CreateAsyncConnectToServer(
            __out AsyncConnectToServer::SPtr& Context
            );
        
        
    protected:
        VOID OnServiceOpen() override ;
        VOID OnServiceClose() override ;
        VOID OnDeferredClosing() override ;

    private:

        //
        // Perform async work as a part of Open.
        // 
        ktl::Task OpenTask();
            
        //
        // Perform async work as a part of Close.
        //
        ktl::Task CloseTask();
        
    private:
        ULONGLONG _SocketAddress;
        KSharedPtr<KAWIpcClient> _Client;
};


//*********************************************************************
// Server specific
//*********************************************************************
//
// This class represents a client connection incoming to the server. It
// is created by the KAWIpcServerPipeEndpoint when it accepts a new
// incoming client.
//
// This class has the main responsibility for managing the connection
// with the client and includes the following activities:
//
// * Negotiating with the client and creating the client specific
//   resources needed.
// * Dead client process detection and client resource rundown
//
// This class may be overriden by an application and if so then the
// base classes must be called.
//
class KAWIpcServerClientConnection : public KAsyncServiceBase
{
    K_FORCE_SHARED_WITH_INHERITANCE(KAWIpcServerClientConnection);
    friend KAWIpcServer;

    public:
        virtual ktl::Awaitable<NTSTATUS> OpenAsync(
            __in KAWIpcServerPipe& ServerPipe,
            __in_opt KAsyncContextBase* const ParentAsync = nullptr,
            __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr
        );

        virtual ktl::Awaitable<NTSTATUS> CloseAsync();

        VOID SetServerPipe(__in KAWIpcServerPipe& ServerPipe)
        {
            _ServerPipe = &ServerPipe;
        };

        KAWIpcServerPipe::SPtr GetServerPipe(){ return(_ServerPipe); };
        
    protected:          
        VOID OnServiceOpen() override ;
        VOID OnServiceClose() override ;

    private:
        //
        // Perform async work as a part of Open.
        // 
        ktl::Task OpenTask();

        //
        // Perform async work as a part of Close.
        //
        ktl::Task CloseTask();      
        
    private:
        KListEntry _ListEntry;
        KAWIpcServerPipe::SPtr _ServerPipe;

};

//
// This class represents a pipe server that listens for incoming pipe
// connections and services them. It may be overridden by the
// application.
//
class KAWIpcServer : public KAsyncServiceBase
{
    K_FORCE_SHARED_WITH_INHERITANCE(KAWIpcServer);
    friend KAWIpcServerPipeEndpoint;
    
    public:
        static NTSTATUS Create(
            __in KAllocator& Allocator,
            __in ULONG AllocatorTag,
            __out KAWIpcServer::SPtr& Context
            );

        //
        // Start up the server and listener
        //
        // Parameters:
        //
        //     SocketAddress is a unique number that represents the
        //         well-known socket endpoint to which clients connect.
        virtual ktl::Awaitable<NTSTATUS> OpenAsync(
            __in ULONGLONG SocketAddress,
            __in_opt KAsyncContextBase* const ParentAsync = nullptr,
            __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr
        );

        virtual ktl::Awaitable<NTSTATUS> CloseAsync();

        //
        // Shutdown a specific incoming client connection
        //
        // Parameters:
        //
        //     Connection is the specific client connection
        //  
        ktl::Awaitable<NTSTATUS> ShutdownServerClientConnection(
            __in KAWIpcServerClientConnection& Connection,
            __in_opt KAsyncContextBase* const ParentAsync = nullptr,
            __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr
            );

        //
        // Create a new KAWIpcServerClientConnection
        //
        NTSTATUS CreateServerClientConnection(
            __out KSharedPtr<KAWIpcServerClientConnection>& Context
        );
        
    protected:
        VOID OnServiceOpen() override ;
        VOID OnServiceClose() override ;

        //
        // OnIncomingConnection() is called whenever a client connects
        // to the server endpoint. Derived classes may perform
        // negotiation and initialization.
        //
        virtual ktl::Awaitable<NTSTATUS> OnIncomingConnection(
            __in KAWIpcServerPipe& Pipe,
            __out KAWIpcServerClientConnection::SPtr& PerClientConnection
            );
                
    private:
        ktl::Task OpenTask();
        ktl::Task CloseTask();
                
        NTSTATUS CreateServerPipeEndpoint(
            __out KAWIpcServerPipeEndpoint::SPtr& Context
        );

        VOID AddServerClientConnection(
            __in KAWIpcServerClientConnection& Connection
            );
        
        NTSTATUS RemoveServerClientConnection(
            __in KAWIpcServerClientConnection& Connection
            );

        ktl::Awaitable<VOID> RundownAllServerClientConnections();

    private:
        ULONGLONG _SocketAddress;
        KAWIpcServerPipeEndpoint::SPtr _PipeEndpoint;

        static const ULONG _ConnectionLinkageOffset;
        KNodeList<KAWIpcServerClientConnection> _ConnectionsList;
        KSpinLock _ConnectionsListLock;
};


//*********************************************************************
// Client Pipe specific
//*********************************************************************

//
// This class represents a client connection incoming to the server on
// the client side. This class may be derived from so that the user of
// the class can keep their own data structures along side it.
//
class KAWIpcClientServerConnection : public KAsyncServiceBase
{
    K_FORCE_SHARED_WITH_INHERITANCE(KAWIpcClientServerConnection);
    friend KAWIpcClient;

    public:
        virtual ktl::Awaitable<NTSTATUS> OpenAsync(
            __in KAWIpcClientPipe& ClientPipe,
            __in_opt KAsyncContextBase* const ParentAsync = nullptr,
            __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr
        );

        virtual ktl::Awaitable<NTSTATUS> CloseAsync();

        KAWIpcClientPipe::SPtr GetClientPipe(){ return(_ClientPipe.RawPtr()); };
        
    protected:          
        VOID OnServiceOpen() override ;
        VOID OnServiceClose() override ;

    private:
        //
        // Perform async work as a part of Open.
        // 
        ktl::Task OpenTask();

        //
        // Perform async work as a part of Close.
        //
        ktl::Task CloseTask();      
        
    private:
        KListEntry _ListEntry;
        KAWIpcClientPipe::SPtr _ClientPipe;
};


class KAWIpcClient : public KAsyncServiceBase
{
    K_FORCE_SHARED_WITH_INHERITANCE(KAWIpcClient);

    public:
        static NTSTATUS Create(
            __in KAllocator& Allocator,
            __in ULONG AllocatorTag,
            __out KAWIpcClient::SPtr& Context
            );

        
        virtual ktl::Awaitable<NTSTATUS> OpenAsync(
            __in_opt KAsyncContextBase* const ParentAsync = nullptr,
            __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr
        );

        virtual ktl::Awaitable<NTSTATUS> CloseAsync();

        NTSTATUS CreateClientServerConnection(
            __out KSharedPtr<KAWIpcClientServerConnection>& Context
        );
                
        ktl::Awaitable<NTSTATUS> ConnectPipe(
            __in ULONGLONG SocketAddress,
            __in KAWIpcClientServerConnection& Connection,
            __in_opt KAsyncContextBase* const ParentAsync = nullptr
            );                                           
                                             
        ktl::Awaitable<NTSTATUS> DisconnectPipe(
            __in KAWIpcClientServerConnection& Connection,
            __in_opt KAsyncContextBase* const ParentAsync = nullptr
            );                                           
                                             
    protected:          
        VOID OnServiceOpen() override ;
        VOID OnServiceClose() override ;

    private:
        ktl::Task OpenTask();
        ktl::Task CloseTask();

        NTSTATUS CreateClientPipe(
            __out KAWIpcClientPipe::SPtr& Context
        );

        ktl::Awaitable<NTSTATUS> DisconnectPipeInternal(
            __in KAWIpcClientServerConnection& Connection,
            __in BOOLEAN RemoveFromList,            
            __in_opt KAsyncContextBase* const ParentAsync = nullptr
            );
        
        VOID AddClientServerConnection(
            __in KAWIpcClientServerConnection& Connection
            );
        
        NTSTATUS RemoveClientServerConnection(
            __in KAWIpcClientServerConnection& Connection
            );

        ktl::Awaitable<VOID> RundownAllClientServerConnections(
            __in_opt KAsyncContextBase* const ParentAsync = nullptr
            );
        
        
    private:
        static const ULONG _ConnectionLinkageOffset;
        KNodeList<KAWIpcClientServerConnection> _ConnectionsList;
        KSpinLock _ConnectionsListLock;

};


//*********************************************************************
// Heap and list managements
//*********************************************************************

//
// This class is a page aligned heap that is implemented over a
// KAWIpcSharedMemory object. It provides an async allocator function
// that provides KIoBuffer objects for use in passing buffers. This is
// used only in the client.
//
class KAWIpcKIoBufferElement;
class KAWIpcPageAlignedHeap : public KAsyncServiceBase
{
    friend KAWIpcKIoBufferElement;
    
    K_FORCE_SHARED(KAWIpcPageAlignedHeap);

    KAWIpcPageAlignedHeap(
        __in KAWIpcSharedMemory& SharedMemory
        );
    
    public:        
        //
        // Note that it is never guaranteed that the KIoBuffer will be
        // allocated as a single contiguous KIoBufferElement.
        //
        static NTSTATUS Create(
            __in KAllocator& Allocator,
            __in ULONG AllocatorTag,
            __in KAWIpcSharedMemory& SharedMemory,
            __out KAWIpcPageAlignedHeap::SPtr& Context
        );

        ktl::Awaitable<NTSTATUS> OpenAsync(
            __in_opt KAsyncContextBase* const ParentAsync = nullptr,
            __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr
        );

        ktl::Awaitable<NTSTATUS> CloseAsync();

        //
        // Size must be 4K aligned
        //
        ktl::Awaitable<NTSTATUS> AllocateAsync(
            __in ULONG Size,
            __out KIoBuffer::SPtr& IoBuffer
        );

        ktl::Awaitable<NTSTATUS> AllocateAsync(
            __in ULONG Size,
            __out KIoBufferElement::SPtr& IoBuffer
        );

        //
        // Size must be on a 4K boundary
        //
        NTSTATUS Allocate(
            __in ULONG Size,
            __out KIoBuffer::SPtr& IoBuffer,
            __out ULONG& RemainingQuanta
        );

        NTSTATUS Allocate(
            __in ULONG Size,
            __out KIoBufferElement::SPtr& IoBufferElement,
            __out ULONG& RemainingQuanta
        );
        
        inline KAWIpcSharedMemory::SPtr GetSharedMemory()
        {
            return(_SharedMemory);
        }
        
        //
        // Note that this must be called before the
        // KAWIpcPageAlignedHeap service is started
        //
        ULONG ReduceMaximumAllocationPossible(__in ULONG Reduction) { KInvariant(! IsOpen()); _MaximumAllocationPossible -= Reduction; };
        ULONG GetMaximumAllocationPossible() { return(_MaximumAllocationPossible); };
        ULONG GetFreeAllocation();
        LONG GetNumberAllocations() { return(_NumberAllocations); };
        
        static const ULONG FourKB = 0x1000;             // Handy to know how much 4KB
        static const ULONG FourKBInBits = 1;
        static const ULONG SixtyFourKB = 16 * FourKB;   // Handy to know how much 64KB
        static const ULONG SixtyFourKBInBits = 16;
        
    protected:
        VOID OnServiceOpen() override ;
        VOID OnDeferredClosing() override ;
        VOID OnServiceClose() override ;

    private:
        Task ShutdownQuotaGate();
        
        VOID Free(
            __in ULONG Size,
            __in PVOID AllocationPtr
          );

        ktl::Task OpenTask();
        ktl::Task CloseTask();

    private:
        KAWIpcSharedMemory::SPtr _SharedMemory;
        ULONG _MaximumAllocationPossible;
        KCoQuotaGate::SPtr _QuotaGate;
        volatile LONG _NumberAllocations;

        KSpinLock _Lock;
        KAllocationBitmap::SPtr _Bitmap;

        ULONG _NumberFreeElements;
        KNodeList<KAWIpcKIoBufferElement> _FreeIoBufferElements;
};

//
// This class is a derivation of KIoBufferElement that represents a page
// aligned allocation within a KAWIpcPageAlignedHeap. It is for both
// the client side and the server side although with slightly different
// semantics.
//
class KAWIpcKIoBufferElement : public KIoBufferElement
{
    K_FORCE_SHARED(KAWIpcKIoBufferElement);
    friend KAWIpcPageAlignedHeap;

    public:
        static NTSTATUS Create(
            __in KAWIpcPageAlignedHeap& Heap,
            __in PVOID Buffer,
            __in ULONG Size,
            __out KIoBufferElement::SPtr& Context
        );

        ULONG GetSharedMemoryOffset();

    protected:
        //
        // This is called when the reference count on this object
        // reaches zero. It allows for divorcing the freeing of the
        // underlying memory from processing that occurs when refcount
        // reaches zero.
        //
        VOID OnDelete();

    private:
        KAWIpcKIoBufferElement(
            __in KAWIpcPageAlignedHeap& Heap,
            __in PVOID Buffer,
            __in ULONG Size
            );

        
    private:
        KAWIpcPageAlignedHeap::SPtr _Heap;
        LIST_ENTRY _FreeListEntry;
        BOOLEAN _ReleaseActivity;
};


//*********************************************************************
// IPC Specific
//*********************************************************************

//
// This is the header for each message being passed and has the control
// structures needed for being maintained on the LIFO (free) list and
// also on the consumer/producer (CP) ring. Each message includes an OwnerId 
// which identifies which user of the IPC mechanism "owns" the message.
// The "owner" of the message is typically the one that places it on
// the ring. The OwnerId is
// useful when an owner crashes as that allows for cleanup of the
// OwnerId resources. Note that all OwnerId have the highest bit set so
// that they can be distinguished from offsets as the OwnerId is also
// used as a type of spinlock.
//
struct KAWIpcMessage 
{
    ULONG Next;
    ULONG Blink;
    LONG OwnerId;
    ULONG SizeIncludingHeader;
    UCHAR Data[1];
};

//
// This represents a single linked list that uses LIFO operation. One
// use is for the request free list and is available to the server and
// all clients. The server will call CreateAndInitialize and the shared
// memory will be formatted into a set of free lists depending upon the
// size of the requests.  The client will call Create which will create
// an object that will connect with the list already created on top of
// the shared memory.
//
class KAWIpcLIFOList : public KObject<KAWIpcLIFOList>, public KShared<KAWIpcLIFOList>
{
    K_FORCE_SHARED(KAWIpcLIFOList);

    KAWIpcLIFOList(
        __in KAWIpcSharedMemory& SharedMemory,
        __in BOOLEAN Initialize
    );
    
    //
    // TODO: This can be enhanced with per client caches, processor
    //       affinitized lists and lookasides
    //
    public:
        //
        // This will create a new list on top of the shared memory
        // section and break up the shared memory into smaller requests
        //
        static NTSTATUS CreateAndInitialize(
            __in KAWIpcSharedMemory& SharedMemory,
            __in KAllocator& Allocator,
            __in ULONG AllocatorTag,
            __out KAWIpcLIFOList::SPtr& Context
        );

        //
        // This will create an object that will access an already
        // created list on the shared memory
        //
        static NTSTATUS Create(
            __in KAWIpcSharedMemory& SharedMemory,
            __in KAllocator& Allocator,
            __in ULONG AllocatorTag,
            __out KAWIpcLIFOList::SPtr& Context
        );
        
        KAWIpcMessage* Remove(__in ULONG Size);
        VOID Add(__in KAWIpcMessage& Message);

        ULONG GetCount64() { return(_Count64); };
        ULONG GetCount1024() { return(_Count1024); };
        ULONG GetCount4096() { return(_Count4096); };
        ULONG GetCount64k() { return(_Count64K); };
        
    private:
        static const ULONG OneKb = 1024;
        static const ULONG FourKb = 4 * 1024;
        static const ULONG SixtyFourKb = 64 * 1024;
        static const ULONG OneMb = 1024 * 1024;
        static const ULONG SixtyFour = 64;
        
        //
        // TODO: This can be enhanced to allow chaining of multiple
        //       shared memory sections as a way to shrink and grow the list
        //
        // This is the list header that is maintained in front of the
        // shared memory section.
        //
        typedef union
        {
            volatile ULONG Head;
            LONG OwnerId;
        } SingleListHead;
        
        // TODO: Figure out the right sizes
        typedef struct 
        {
            volatile SingleListHead Head64;
            ULONG Offset64;
            volatile SingleListHead Head1024;
            ULONG Offset1024;
            volatile SingleListHead Head4096;
            ULONG Offset4096;
            volatile SingleListHead Head64K;
            ULONG Offset64K;
            volatile LONG OnListCount;
        } ListHeads;
        
    private:
        BOOLEAN _IsCreator;
        KAWIpcSharedMemory::SPtr _SharedMemory;
        volatile ULONG* _ListHead64;
        volatile ULONG* _ListHead1024;       
        volatile ULONG* _ListHead4096;       
        volatile ULONG* _ListHead64K;       
        volatile LONG* _OnListCountPtr;
        ULONG _Count64;
        ULONG _Count1024;
        ULONG _Count4096;
        ULONG _Count64K;
};

class KAWIpcMessageAllocator : public KAsyncServiceBase
{
    K_FORCE_SHARED(KAWIpcMessageAllocator);

    public:
        static NTSTATUS Create(
            __in KAllocator& Allocator,
            __in ULONG AllocatorTag,
            __out KAWIpcMessageAllocator::SPtr& Context
        );

        ktl::Awaitable<NTSTATUS> OpenAsync(
            __in KAWIpcLIFOList& Lifo,
            __in_opt KAsyncContextBase* const ParentAsync = nullptr,
            __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr
        );

        ktl::Awaitable<NTSTATUS> CloseAsync();
        
        ktl::Awaitable<NTSTATUS> AllocateAsync(
            __in ULONG Size,
            __out KAWIpcMessage*& Message
            );

        VOID Free(
            __in KAWIpcMessage* Message
            );
        
    protected:          
        VOID OnServiceOpen() override ;
        VOID OnDeferredClosing() override ;
        VOID OnServiceClose() override ;

    private:
        ktl::Task OpenTask();
        ktl::Task CloseTask();

    private:
        KAWIpcLIFOList::SPtr _Lifo;
        BOOLEAN _IsCancelled;
};

//
// This represents a logical queue of messages in shared memory. 
//
class KAWIpcRing : public KObject<KAWIpcRing>, public KShared<KAWIpcRing>
{
    K_FORCE_SHARED(KAWIpcRing);

    KAWIpcRing(
        __in LONG OwnerId,
        __in KAWIpcSharedMemory& RingSharedMemory,
        __in KAWIpcSharedMemory& MessageSharedMemory,
        __in BOOLEAN Initialize
        );
    
    public:
        static NTSTATUS CreateAndInitialize(
            __in LONG OwnerId,
            __in KAWIpcSharedMemory& RingSharedMemory,
            __in KAWIpcSharedMemory& MessageSharedMemory,
            __in KAllocator& Allocator,
            __in ULONG AllocatorTag,
            __out KAWIpcRing::SPtr& Context
            );

        static NTSTATUS Create(
            __in LONG OwnerId,
            __in KAWIpcSharedMemory& RingSharedMemory,
            __in KAWIpcSharedMemory& MessageSharedMemory,
            __in KAllocator& Allocator,
            __in ULONG AllocatorTag,
            __out KAWIpcRing::SPtr& Context
            );

        NTSTATUS Enqueue(
            __in KAWIpcMessage* Message,
            __out BOOLEAN& WasEmpty
            );

        NTSTATUS Dequeue(
            __out KAWIpcMessage*& Message
            );

        BOOLEAN IsEmpty() { return(IsEmpty(_Ring->PC)); };
            
        VOID BreakOwner(
            __in LONG OwnerId
        );

        NTSTATUS ClearByOwnerId(
            __in LONG OwnerId
        );      
        
        NTSTATUS LockInternal(
            __in LONG OwnerId
            );

        inline NTSTATUS Lock() { NTSTATUS status = LockInternal(_OwnerId); return(status); };
        NTSTATUS LockForClose();
        inline BOOLEAN IsLockedForClose() { return(_Ring->LockedForClose != 0); };

        inline VOID Unlock(
            )
        {
            LONG result;

            result = InterlockedCompareExchange(&_Ring->Lock, 0, _OwnerId);
            KInvariant(result == _OwnerId);
        };

        ULONG GetNumberEntries() { return(_Ring->NumberEntries); };

        VOID SetWaitingForEvent() {_Ring->WaitingForEvent = 1; };
    private:
        //
        // RingHead structure lives at the front of the shared memory.
        // It is composed of the RingHead with the OwnerId and producer
        // and consumer offsets followed by the ring itself. The ring
        // itself is composed of a circular buffer of offsets. Those
        // offset are relative to the KAWIpcSharedMemory for the
        // associated KAWIpcLIFOList.
        //
        // Ring Empty is when P = C + 1
        // Ring Full is when P == C
        //
        typedef union 
        {
            volatile ULONGLONG PAndC;
            struct
            {
                volatile ULONG P;
                volatile ULONG C;    
            } a;
        } PANDC;
        
        typedef struct
        {
            PANDC PC;
            volatile LONG Lock;
            volatile LONG WaitingForEvent;
            volatile LONG LockedForClose;
            ULONG NumberEntries;
            ULONG Data[1];        // 1018 in first block, 1024 in others
        } RingHead;

        
    private:
        
        inline BOOLEAN IsEmpty(
            __in PANDC& PC
            )
        {
            return((PC.a.P == (PC.a.C + 1)) ||
                ((PC.a.P == 0) && (PC.a.C == _NumberEntries)));
        };

        inline ULONG Advance(
            __in ULONG OldOffset
            )
        {
            if (OldOffset == _NumberEntries)
            {
                return(0);
            }
            return(OldOffset+1);
        };

    private:
        BOOLEAN _IsCreator;
        KAWIpcSharedMemory::SPtr _RingSharedMemory;
        KAWIpcSharedMemory::SPtr _MessageSharedMemory;
        RingHead* _Ring;
        LONG _OwnerId;
        ULONG _NumberEntries;
        static const LONG _ClosedOwnerId = -1;
};


//
// An instance of this class is associated with a set of IPC message
// queues and is used by the consumer of those queues. The service is
// started at the beginning of the lifecyle of the component and will
// create an async for each queue and each async will wait for the
// its queue to have a message and then dispatch the message
// to a delegate. The asyncs do not ever complete until cancelled which
// is done when the KAWIpcQueueConsumer serviice is closed. Typically
// there will be a single async and queue for each core.
//
class KAWIpcRingConsumer : public KAsyncServiceBase
{
    K_FORCE_SHARED(KAWIpcRingConsumer);
    
    public:
        static NTSTATUS Create(
            __in KAllocator& Allocator,
            __in ULONG AllocatorTag,
            __out KAWIpcRingConsumer::SPtr& Context
        );

        ktl::Awaitable<NTSTATUS> OpenAsync(
            __in KAWIpcRing& Ring,
            __in EVENTHANDLEFD EventHandleFd,
            __in_opt KAsyncContextBase* const ParentAsync = nullptr,
            __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr
        );

        ktl::Awaitable<NTSTATUS> CloseAsync();
        
        ktl::Awaitable<NTSTATUS> ConsumeAsync(
            __out KAWIpcMessage*& Message
            );

        NTSTATUS LockForClose() { return(_Ring->LockForClose()); };
        BOOLEAN IsLockedForClose() { return(_Ring->IsLockedForClose()); };
        BOOLEAN IsEmpty() { return(_Ring->IsEmpty()); };
        
    protected:          
        VOID OnServiceOpen() override ;
        VOID OnDeferredClosing() override ;
        VOID OnServiceClose() override ;

    private:
        ktl::Task OpenTask();
        ktl::Task CloseTask();

    private:
        KAWIpcRing::SPtr _Ring;
        EVENTHANDLEFD _EventHandleFd;
        KAWIpcEventWaiter::SPtr _EventWaiter;
};

//
// This class is used to send generated message via the appropriate
// queue
//
class KAWIpcRingProducer : public KAsyncServiceBase
{
    K_FORCE_SHARED_WITH_INHERITANCE(KAWIpcRingProducer);
    
    public:
        static NTSTATUS Create(
            __in KAllocator& Allocator,
            __in ULONG AllocatorTag,
            __out KAWIpcRingProducer::SPtr& Context
        );
        
        ktl::Awaitable<NTSTATUS> OpenAsync(
            __in KAWIpcRing& Ring,
            __in EVENTHANDLEFD EventHandleFd,
            __in_opt KAsyncContextBase* const ParentAsync = nullptr,
            __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr
        );

        ktl::Awaitable<NTSTATUS> CloseAsync();
        
        ktl::Awaitable<NTSTATUS> ProduceAsync(
            __in KAWIpcMessage* Message
        );

    protected:          
        VOID OnServiceOpen() override ;
        VOID OnDeferredClosing() override ;
        VOID OnServiceClose() override ;

    private:
        ktl::Task OpenTask();
        ktl::Task CloseTask();

    private:
        KAWIpcRing::SPtr _Ring;
        EVENTHANDLEFD _EventHandleFd;
        KAWIpcEventKicker::SPtr _EventKicker;
        KTimer::SPtr _ThrottleTimer;
        BOOLEAN _IsCancelled;
};

#define PtrToActivityId(x) ((KActivityId)((ULONGLONG)(x)))


#endif // defined(K_UseResumable)
