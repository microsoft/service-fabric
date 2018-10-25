/*++

Copyright (c) 2010  Microsoft Corporation

Module Name:

    kthreadpool.h

Abstract:

    This file defines the interface for a KTL thread pool.

    The KTL thread pool returned by 'Create' supports at most one thread per processor and is designed to be used only for
    non-blocking operations.  For this thread pool, queuing checks to see if the current thread is a thread
    pool thread and re-queues to the same thread.  In this way queuing keeps this same thread without blowing through the stack.
    Queuing when the current thread is not a thread pool thread queues to a central queue instead.  Queuing to the central queue
    will be picked up by the next available thread.  Every thread pool thread checks first its own private queue followed by
    the central queue.

    The purpose of a thread pool created with 'CreateSystem' is for work items that will actually block execution, such as
    'CreateFile'.

    The KTL thread pool returned by 'CreateSystem', for kernel mode, is just a wrapper around the default 'DelayedWorkQueue'
    system thread pool.  So, as to not overwhelm that central thread pool, this class will limit the number of outstanding
    work items that it queues to the 'DelayedWorkQueue'.

    The KTL thread pool returned by 'CreateSystem', for user mode, is just a wrapper around a newly created TP_POOL thread
    pool.  So as to not create too many threads, this class will limit the number of outstanding work items that it queues to
    the TP_POOL.

Author:

    Norbert P. Kusters (norbertk) 27-Sep-2010

Environment:

    Kernel mode and User mode

Notes:

    Because the use of these threads is inherent in the async model, this class itself relies on being created and destroyed
    without the use of the async model.  That is, directly with the use of a thread that you can wait on.  Therefore some
    care must be taken that the 'Create' method must be called on a thread that can wait and that the last reference to
    the object be decremented from a thread that can wait.

--*/

class KThreadPool : public KObject<KThreadPool>, public KShared<KThreadPool> 
{
    K_FORCE_SHARED_WITH_INHERITANCE(KThreadPool);

public:
    // ThreadPoool mix-in class to provide dispatch to its derivation via Execute()
    class WorkItem 
    {
        K_DENY_COPY(WorkItem);

    public:
        WorkItem();
        virtual ~WorkItem();
        virtual VOID Execute() = 0;
        static void* Dispatch(WorkItem& WI);

    private:
        friend class KThreadPool;
        friend class KThreadPoolStandard;
        friend class KSystemThreadPoolStandard;

        void*       _EnqueuerCallerAddress;
        WorkItem*   _Link;
    };

	//** Helper class to pass a parameter of type TParam to a supplied thread pool work item function of type 
	//   WorkItemProcessor.
	//
	template<typename TParam>
	class ParameterizedWorkItem : public WorkItem
	{
	public:
		using WorkItemProcessor = void (*)(TParam Param);

		ParameterizedWorkItem(TParam Param, WorkItemProcessor Processor) : _Param(Param), _Processor(Processor) {}
		~ParameterizedWorkItem() {}

	private:
		// Disallow default behaviors
		ParameterizedWorkItem() = delete;
		ParameterizedWorkItem(ParameterizedWorkItem&) = delete;
		ParameterizedWorkItem(ParameterizedWorkItem&&) = delete;
		ParameterizedWorkItem& operator=(ParameterizedWorkItem&) = delete;
		ParameterizedWorkItem& operator=(ParameterizedWorkItem&&) = delete;

		void Execute() override { _Processor(_Param); }

	private:
		TParam				_Param;
		WorkItemProcessor	_Processor;
	};

public:
    // ThreadPool factory: KTL dedicated - Create a fixed thread per processor - thread pool
    static
    NTSTATUS
    Create(
        __out KThreadPool::SPtr& ThreadPool,
        __in KAllocator& Allocator,
        __in ULONG AllocationTag = KTL_TAG_THREAD_POOL,
        __in ULONG MaximumNumberOfThreads = 0
        );

    // ThreadPool factory: Wrapper for System ThreadPool - does support partial per-thread queuing in UM
    static
    NTSTATUS
    CreateSystem(
        __out KThreadPool::SPtr& ThreadPool,
        __in KAllocator& Allocator,
        __in ULONG AllocationTag = KTL_TAG_THREAD_POOL,
        __in ULONG MaxActive = 16
        );

    // Schedule a WorkItem derivation for executionion via WorkItem::Execute()
    //
    //  If UsePerThreadQueue is set true all attempts will be made to optimize the queuing of
    //  of WorkItem on the private work queue for the current thread. 
    //
    //  NOTE: UseCurrentThreadQueue is ignored for the KTL-Dedicated ThreadPool as it always 
    //        queues onto a per-thread queue
    //
    virtual
    VOID
    QueueWorkItem(__inout WorkItem& WorkItem, __in BOOLEAN UseCurrentThreadQueue = FALSE) = 0;

    // Drain and disable the current ThreadPool - automatically done during dtor
    virtual
    VOID
    Wait() = 0;

    #if KTL_USER_MODE
        //* IOCompPort Binding API
        typedef
        VOID
        (*IoCompletionCallback)(
            __in_opt VOID* Context,
            __inout_xcount("varies") OVERLAPPED* Overlapped,
            __in DWORD Win32ErrorCode,
            __in ULONG BytesTransferred
            );

        virtual
        NTSTATUS
        RegisterIoCompletionCallback(
            __in HANDLE Handle,
            __in IoCompletionCallback CompletionCallback,
            __in_opt VOID* Context,
            __out VOID** RegistrationContext
            );

        virtual
        VOID
        UnregisterIoCompletionCallback(__inout VOID* RegistrationContext);
    #endif

    // Detect if current thread belongs is a KTL-dedicated thread pool (non-system thread pool) thread.
    virtual
    BOOLEAN
    IsCurrentThreadOwned()              { return FALSE; }

    virtual
    BOOLEAN
    IsCurrentThreadOwnedSupported()     { return FALSE; }


    #if KTL_USER_MODE
        // Attempt to detach the current thread from its KThreadPool responsibilities
        //
        //  Returns: TRUE  - Thread is/was detached
        //           FALSE - Detachment is not supported on this thread or type of thread pool
        virtual
        BOOLEAN
        DetachCurrentThread()               { return FALSE; }
    #endif

    virtual
    BOOLEAN
    SetThreadPriority(__in KThread::Priority Priority) = 0;

    virtual
    BOOLEAN
    SetWorkItemExecutionTimeBound(__in ULONG BoundInMilliSeconds) = 0;

    virtual
    LONGLONG
    GetWorkItemExecutionTooLongCount() = 0;

    virtual
    ULONG
    GetThreadCount() = 0;

protected:
    // Common ThreadPool supoort 
    class WorkQueue 
    {
    public:
        WorkQueue()                 { _Front = nullptr; _Back = nullptr; }
        ~WorkQueue()                {}
        WorkItem*   PeekFront()     { return _Front; }
        void        Reset()         { _Front = nullptr; _Back = nullptr; }

        VOID        Insert(__inout WorkItem& WorkItem);
        WorkItem*   Remove();

    private:
        WorkItem* _Front;
        WorkItem* _Back;
    };

    static
    WorkItem* GetLink(__in WorkItem& WI);

    static
    VOID SetLink(__inout WorkItem& WI, __in_opt WorkItem* Link);
};

