/*++

    (c) 2011 by Microsoft Corp. All Rights Reserved.

    KtlSystemBase.cpp

    Description:
      Kernel Tempate Library (KTL) system instance base class

      Implements the common aspects of the KtlSystem abstraction and provides
      a base class so that application specific derivation are easy to build.

    History:
      richhas          14-July-2011         Initial version.

--*/

#include <ktl.h>
#include <ktrace.h>

//** KtlSysBasePageAlignedAllocator
KtlSysBasePageAlignedAllocator::KtlSysBasePageAlignedAllocator(KtlSystemBase* const System)
    :   KtlCorePageAlignedAllocator(System)
{
}

KtlSysBasePageAlignedAllocator::~KtlSysBasePageAlignedAllocator()
{
    if (GetKtlSystem().GetStrictAllocationChecks())
    {
        KInvariant(_AllocationCount == 0);
    }
}

//** KtlSystemBase Implementation
volatile LONG KtlSystemBase::CountOfSystemInstances = 0;

KtlSystemBase::KtlSystemBase(const GUID& InstanceId, ULONG ThreadPoolThreadLimit) 
    :   _InstanceId(InstanceId),
        _ObjectCompare(&ObjectEntryKey::Compare),
        _ObjectTable(FIELD_OFFSET(ObjectEntryKey, TableEntry), _ObjectCompare)
{
    InterlockedIncrement(&CountOfSystemInstances);

    _IsActive = FALSE;
    _AllocationChecksOn = TRUE;
    #if KTL_USER_MODE
        _IsSystemThreadPoolDefault = TRUE;
    #else
        _IsSystemThreadPoolDefault = FALSE;
    #endif

    #if KTL_USER_MODE
        _HeapHandle = nullptr;
        //
        // For user mode, create our own heap dedicated to this KtlSystem
        //
        _HeapHandle = HeapCreate(0,          // No Options
                                 0,          // start with 1 page
                                 0);         // No maximum size

    if (_HeapHandle == nullptr)
    {
        SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
        return;
    }
    #endif

    // Create the allocators
    _NonPagedThreadPoolAllocator = nullptr;
    _NonPagedAllocator = nullptr;
    _PagedAllocator = nullptr;
    _RawPageAlignedAllocator = nullptr;

    // This "first" allocation must be done from an external heap - not using any of the "new"
    // plumbing.
    _SystemHeap = (NonPagedKAllocator*)ExAllocatePoolWithTag(
#if KTL_USER_MODE
        _HeapHandle,
#endif
        NonPagedPool,
        sizeof(NonPagedKAllocator),
        KTL_TAG_BASE);

    if (_SystemHeap == nullptr)
    {
        SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
        return;
    }
    _SystemHeap->KtlSysBaseNonPageAlignedAllocator<NonPagedPool>::KtlSysBaseNonPageAlignedAllocator(this);    // CTOR

    // Now we can use _SystemHeap for the internal KtlSystemBase state - using the normal
    // "new" plumbing
    _NonPagedThreadPoolAllocator = _new(KTL_TAG_BASE, *_SystemHeap) NonPagedKAllocator(this);
    if (_NonPagedThreadPoolAllocator == nullptr)
    {
        SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    _NonPagedAllocator = _new(KTL_TAG_BASE, *_SystemHeap) NonPagedKAllocator(this);
    if (_NonPagedAllocator == nullptr)
    {
        SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    _PagedAllocator = _new(KTL_TAG_BASE, *_SystemHeap) PagedKAllocator(this);
    if (_PagedAllocator == nullptr)
    {
        SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    _RawPageAlignedAllocator = _new(KTL_TAG_BASE, *_SystemHeap) KtlSysBasePageAlignedAllocator(this);
    if (_RawPageAlignedAllocator == nullptr)
    {
        SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    // Allocate the thread pools for the system
    KThreadPool::Create(_ThreadPool, *_NonPagedThreadPoolAllocator, KTL_TAG_BASE, ThreadPoolThreadLimit);
    if (!_ThreadPool)
    {
        SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    KThreadPool::CreateSystem(_SystemThreadPool, *_NonPagedThreadPoolAllocator, KTL_TAG_BASE);
    if (!_SystemThreadPool)
    {
        SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    // Keep global list of KtlSyetmBase instances for debug support
    K_LOCK_BLOCK(gs_GlobalLock.Lock())
    {
        KNodeList<KtlSystemBase>&   systemsList = *UnsafeGetKtlSystemBases();
        systemsList.AppendTail(this);
    }
}

KtlSystemBase::~KtlSystemBase()
{
    KFatal(!_IsActive);

    // ThreadPools should destruct if they were ctor'd
    if (_SystemThreadPool)
    {
        KFatal(_SystemThreadPool.Reset());
    }
    if (_ThreadPool)
    {
        KFatal(_ThreadPool.Reset());
    }

    // dtor the allocators
    if (_RawPageAlignedAllocator != nullptr)
    {
        _delete(_RawPageAlignedAllocator);
        _RawPageAlignedAllocator = nullptr;
    }
    if (_PagedAllocator != nullptr)
    {
        _delete(_PagedAllocator);
        _PagedAllocator = nullptr;
    }
    if (_NonPagedAllocator != nullptr)
    {
        _delete(_NonPagedAllocator);
        _NonPagedAllocator = nullptr;
    }
    if (_NonPagedThreadPoolAllocator != nullptr)
    {
        _delete(_NonPagedThreadPoolAllocator);
        _NonPagedThreadPoolAllocator = nullptr;
    }

    // drop this instance from global list of KtlSystemBase instances
    K_LOCK_BLOCK(gs_GlobalLock.Lock())
    {
        KNodeList<KtlSystemBase>&   systemsList = *UnsafeGetKtlSystemBases();
        systemsList.Remove(this);
    }

    // Do last
    if (_SystemHeap != nullptr)
    {
        _SystemHeap->~KtlSysBaseNonPageAlignedAllocator();
#if KTL_USER_MODE
        ExFreePool(_HeapHandle, _SystemHeap);
#else
        ExFreePool(_SystemHeap);
#endif
        _SystemHeap = nullptr;
    }

#if KTL_USER_MODE
    if (_HeapHandle != nullptr)
    {
        HeapDestroy(_HeapHandle);
        _HeapHandle = nullptr;
    }
#endif

    InterlockedDecrement(&CountOfSystemInstances);
}

#if KTL_USER_MODE
    void KtlSystemBase::SetEnableAllocationCounters(bool ToEnabled)
    {
        _PagedAllocator->SetEnableAllocationCounters(ToEnabled);
        _NonPagedAllocator->SetEnableAllocationCounters(ToEnabled);
    }
#endif


VOID
KtlSystemBase::Activate(__in ULONG MaxActiveDurationInMsecs)
{
    K_LOCK_BLOCK(_ThisLock)
    {
        KFatal(!_IsActive);
        _IsActive = TRUE;
    }

    // Inform the derivation that it is time to start it's internal services; it only returns success when
    // it has reached a safe point where the started system is functional.
    NTSTATUS        status = StartSystemInstance(MaxActiveDurationInMsecs);
    SetStatus(status);
}

VOID
KtlSystemBase::Deactivate(__in ULONG MaxDeactiveDurationInMsecs)
{
    #if KTL_USER_MODE
        KInvariant(!IsCurrentThreadOwned());        // NOTE: Deactivate() can't be called on a KTL thread - will deadlock/fail
    #endif

    K_LOCK_BLOCK(_ThisLock)
    {
        KFatal(_IsActive);
        _IsActive = FALSE;
    }

    // Tell the derivation to start its shutdown process
    NTSTATUS        status = StartSystemInstanceShutdown();
    if (!NT_SUCCESS(status))
    {
        SetStatus(status);
        return;
    }

    //
    // Clean up the object table.
    //

    while (_ObjectTable.Count()) {
        ObjectEntry* p = _ObjectTable.First();
        _ObjectTable.Remove(*p);
        _delete(p);
    }

    if (_AllocationChecksOn)
    {
        // Wait for our allocators(heaps) to drain - or MaxDeactiveDurationInMsecs
        ULONGLONG       maxActiveTime = MAXULONGLONG;       // default to no-timeout

        if (MaxDeactiveDurationInMsecs != KtlSystem::InfiniteTime)
        {
            maxActiveTime = KNt::GetTickCount64() + MaxDeactiveDurationInMsecs;
        }

        KEvent          heapEmptyEvent(FALSE, FALSE);
        KFatal(NT_SUCCESS(heapEmptyEvent.Status()));

        _RawPageAlignedAllocator->SignalWhenEmpty(heapEmptyEvent);
        if (!WaitUntil(heapEmptyEvent, maxActiveTime))
        {
            // Timeout
            _RawPageAlignedAllocator->ClearHeapEmptyEvent();
            TimeoutDuringDeactivation();
            SetStatus(STATUS_UNSUCCESSFUL);
            return;
        }

        _PagedAllocator->SignalWhenEmpty(heapEmptyEvent);
        if (!WaitUntil(heapEmptyEvent, maxActiveTime))
        {
            // Timeout
            _PagedAllocator->ClearHeapEmptyEvent();
            TimeoutDuringDeactivation();
            SetStatus(STATUS_UNSUCCESSFUL);
            return;
        }

        _NonPagedAllocator->SignalWhenEmpty(heapEmptyEvent);
        if (!WaitUntil(heapEmptyEvent, maxActiveTime))
        {
            // Timeout
            _NonPagedAllocator->ClearHeapEmptyEvent();
            TimeoutDuringDeactivation();
            SetStatus(STATUS_UNSUCCESSFUL);
            return;
        }

        // At this point there are no application items in any of the heaps - release the thread pools.
        // The thread pools' shutdown will be detected because the _NonPagedThreadPoolAllocator
        // allocator will go empty.
        _SystemThreadPool.Reset();
        _ThreadPool.Reset();

        _NonPagedThreadPoolAllocator->SignalWhenEmpty(heapEmptyEvent);
        if (!WaitUntil(heapEmptyEvent, maxActiveTime))
        {
            // Timeout
            _NonPagedThreadPoolAllocator->ClearHeapEmptyEvent();
            TimeoutDuringDeactivation();
            SetStatus(STATUS_UNSUCCESSFUL);
            return;
        }

        // Next delete all allocators. This should allow the _SystemHeap to drain.
        _delete(_RawPageAlignedAllocator);
        _RawPageAlignedAllocator = nullptr;

        _delete(_PagedAllocator);
        _PagedAllocator = nullptr;

        _delete(_NonPagedAllocator);
        _NonPagedAllocator = nullptr;

        _delete(_NonPagedThreadPoolAllocator);
        _NonPagedThreadPoolAllocator = nullptr;

        // The system heap is hidden and thus should be empty
        _SystemHeap->SignalWhenEmpty(heapEmptyEvent);
        KFatal(WaitUntil(heapEmptyEvent, maxActiveTime));

        // NOTE: _SystemHeap will be dtor'd and deallocated in this instance's dtor
    }
}

//  Wait for an event to be set or a time to be reached - return FALSE if timedout
BOOLEAN
KtlSystemBase::WaitUntil(KEvent& Event, ULONGLONG UntilTimeOfDayInMsecs)
{
    if (UntilTimeOfDayInMsecs != MAXULONGLONG)
    {
        // Make sure not too late
        ULONGLONG       now = KNt::GetTickCount64();
        if (now >= UntilTimeOfDayInMsecs)
        {
            // Timeout
            return FALSE;
        }

        // Calc remaining duration in msecs
        ULONGLONG       unsignedDuration = UntilTimeOfDayInMsecs - now;
        KFatal(unsignedDuration <= MAXLONGLONG);

        // Wait for event signal OR timeout
        return Event.WaitUntilSet((ULONG)unsignedDuration);
    }

    Event.WaitUntilSet();
    return TRUE;
}

GUID
KtlSystemBase::GetInstanceId()
{
    return _InstanceId;
}

KAllocator&
KtlSystemBase::NonPagedAllocator()
{
    KFatal(Status() == STATUS_SUCCESS);
    return *_NonPagedAllocator;
}

KAllocator&
KtlSystemBase::PagedAllocator()
{
    KFatal(Status() == STATUS_SUCCESS);
    return *_PagedAllocator;
}

KAllocator&
KtlSystemBase::PageAlignedRawAllocator()
{
    KFatal(Status() == STATUS_SUCCESS);
    return *_RawPageAlignedAllocator;
}

KThreadPool&
KtlSystemBase::DefaultThreadPool()
{
    KFatal(Status() == STATUS_SUCCESS);
    return *_ThreadPool.RawPtr();
}

KThreadPool&
KtlSystemBase::DefaultSystemThreadPool()
{
    KFatal(Status() == STATUS_SUCCESS);
    return *_SystemThreadPool.RawPtr();
}

NTSTATUS
KtlSystemBase::RegisterGlobalObject(
    __in_z const WCHAR* ObjectName,
    __inout KSharedBase& Object
    )
{
    ObjectEntry* objectEntry;
    NTSTATUS status;
    BOOLEAN b = FALSE;

    objectEntry = _new(KTL_TAG_BASE, NonPagedAllocator()) ObjectEntry(NonPagedAllocator());

    if (!objectEntry)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    objectEntry->ObjectName = ObjectName;

    status = objectEntry->ObjectName.Status();

    if (!NT_SUCCESS(status))
    {
        _delete(objectEntry);
        return status;
    }

    objectEntry->Object = &Object;
    objectEntry->String = objectEntry->ObjectName;

    K_LOCK_BLOCK(_ThisLock)
    {
        b = _ObjectTable.Insert(*objectEntry);
    }

    if (!b)
    {
        _delete(objectEntry);
        return STATUS_OBJECT_NAME_COLLISION;
    }

    return STATUS_SUCCESS;
}

VOID
KtlSystemBase::UnregisterGlobalObject(
    __in_z const WCHAR* ObjectName
    )
{
    ObjectEntry* p = NULL;
    ObjectEntryKey key;

    key.String = ObjectName;

    K_LOCK_BLOCK(_ThisLock)
    {
        p = _ObjectTable.Lookup(key);

        if (p)
        {
            _ObjectTable.Remove(*p);
        }
    }

    if (p)
    {
        _delete(p);
    }
}

KSharedBase::SPtr
KtlSystemBase::GetGlobalObjectSharedBase(
    __in_z const WCHAR* ObjectName
    )
{
    ObjectEntry* p = NULL;
    ObjectEntryKey key;
    KSharedBase::SPtr object;

    key.String = ObjectName;

    K_LOCK_BLOCK(_ThisLock)
    {
        p = _ObjectTable.Lookup(key);

        if (!p)
        {
            return NULL;
        }

        object = p->Object;
    }

    return object;
}

void
KtlSystemBase::SetStrictAllocationChecks(BOOLEAN AllocationChecksOn)
{
    _AllocationChecksOn = AllocationChecksOn;
}

BOOLEAN
KtlSystemBase::GetStrictAllocationChecks()
{
    return _AllocationChecksOn;
}

#if KTL_USER_MODE
void
KtlSystemBase::SetDefaultSystemThreadPoolUsage(BOOLEAN OnOrOff)
{
    _IsSystemThreadPoolDefault = OnOrOff;
}

BOOLEAN
KtlSystemBase::GetDefaultSystemThreadPoolUsage()
{
    return _IsSystemThreadPoolDefault;
}
#endif

// Debug support
VOID
KtlSystemBase::DumpDebugInfo()
{
    UCHAR           asciiIdStr[SizeOfAsciiGUIDStr];
    DumpDebugInstanceId(&asciiIdStr[0]);
    struct Boolean
    {
        static UCHAR
        ToUCHAR(BOOLEAN Value)
        {
            return Value ? 'T' : 'F';
        }
    };

    KDbgPrintf("KtlSystemBase::DumpDebugInfo: Instance@: 0x%016I64X: Id: %s; Status(): 0x%08X; "
               "_IsActive: %c; _AllocationChecksOn: %c\n",
        this,
        &asciiIdStr[0],
        Status(),
        Boolean::ToUCHAR(_IsActive),
        Boolean::ToUCHAR(_AllocationChecksOn));

    struct AllocatorDesc
    {
        const char*       _Description;
        KtlCoreAllocator* _Allocator;

        AllocatorDesc(__in const char* Description, __in KtlCoreAllocator* Allocator)
            :   _Description(Description),
                _Allocator(Allocator)
        {
        }
    };

    AllocatorDesc           allocatorDescs[] =
    {
        AllocatorDesc("_SystemHeap", _SystemHeap),
        AllocatorDesc("_NonPagedThreadPoolAllocator", _NonPagedThreadPoolAllocator),
        AllocatorDesc("_NonPagedAllocator", _NonPagedAllocator),
        AllocatorDesc("_PagedAllocator", _PagedAllocator),
        AllocatorDesc("_RawPageAlignedAllocator", _RawPageAlignedAllocator),
        AllocatorDesc(nullptr, nullptr)
    };

    AllocatorDesc*          ad = &allocatorDescs[0];
    while (ad->_Allocator != nullptr)
    {
        KDbgPrintf("\t%s@:  0x%016I64X: Allocations: %I64u; _IsLocked: %c\n",
            ad->_Description,
            ad->_Allocator,
            ad->_Allocator->GetAllocsRemaining(),
            Boolean::ToUCHAR(ad->_Allocator->GetIsLocked()));

        ad->_Allocator->DebugDump();
        ad++;
    }

    DumpSystemInstanceDebugInfo();
}

VOID
KtlSystemBase::DumpDebugInstanceId(UCHAR* Result)
{
    struct HexPair
    {
        static VOID
        ToAscii(UCHAR Value, UCHAR* OutputPtr)
        {
            static UCHAR        printableChars[] = "0123456789ABCDEF";

            OutputPtr[0] = printableChars[((Value & 0xF0) >> 4)];
            OutputPtr[1] = printableChars[(Value & 0x0F)];
        }
    };

    Result[0] = '{';

    HexPair::ToAscii((UCHAR)(_InstanceId.Data1 >> 24), &Result[1+0]);
    HexPair::ToAscii((UCHAR)(_InstanceId.Data1 >> 16), &Result[1+2]);
    HexPair::ToAscii((UCHAR)(_InstanceId.Data1 >> 8), &Result[1+4]);
    HexPair::ToAscii((UCHAR)(_InstanceId.Data1 >> 0), &Result[1+6]);
    Result[1+8] = '-';

    HexPair::ToAscii((UCHAR)(_InstanceId.Data2 >> 8), &Result[10+0]);
    HexPair::ToAscii((UCHAR)(_InstanceId.Data2 >> 0), &Result[10+2]);
    Result[10+4] = '-';

    HexPair::ToAscii((UCHAR)(_InstanceId.Data3 >> 8), &Result[15+0]);
    HexPair::ToAscii((UCHAR)(_InstanceId.Data3 >> 0), &Result[15+2]);
    Result[15+4] = '-';

    HexPair::ToAscii(_InstanceId.Data4[0], &Result[20+0]);
    HexPair::ToAscii(_InstanceId.Data4[1], &Result[20+2]);
    Result[20+4] = '-';

    HexPair::ToAscii(_InstanceId.Data4[2], &Result[25+0]);
    HexPair::ToAscii(_InstanceId.Data4[3], &Result[25+2]);
    HexPair::ToAscii(_InstanceId.Data4[4], &Result[25+4]);
    HexPair::ToAscii(_InstanceId.Data4[5], &Result[25+6]);
    HexPair::ToAscii(_InstanceId.Data4[6], &Result[25+8]);
    HexPair::ToAscii(_InstanceId.Data4[7], &Result[25+10]);

    Result[25+12] = '}';
    Result[25+13] = 0;
}

// Default derivation implementation
NTSTATUS
KtlSystemBase::StartSystemInstance(__in ULONG MaxActiveDurationInMsecs)
{
    UNREFERENCED_PARAMETER(MaxActiveDurationInMsecs);
    return STATUS_SUCCESS;
}

NTSTATUS
KtlSystemBase::StartSystemInstanceShutdown()
{
    return STATUS_SUCCESS;
}

VOID
KtlSystemBase::DumpSystemInstanceDebugInfo()
{
}

VOID
KtlSystemBase::TimeoutDuringDeactivation()
{
    DebugOutputKtlSystemBases();
    KFatal(FALSE);
}




//* Global KtlSystemBase implementation

KGlobalSpinLockStorage    KtlSystemBase::gs_GlobalLock;


UCHAR KtlSystemListStorage[sizeof(KNodeList<KtlSystemBase>)];

KNodeList<KtlSystemBase>*
KtlSystemBase::UnsafeGetKtlSystemBases()
{
    KAssert(gs_GlobalLock.Lock().IsOwned());

    static BOOLEAN volatile     isInitialized = FALSE;

    KNodeList<KtlSystemBase>*   result = (KNodeList<KtlSystemBase>*)&KtlSystemListStorage[0];

    if (!isInitialized)
    {
        // Ctor in-place the list header first time thru
        result-> KNodeList<KtlSystemBase>::KNodeList(FIELD_OFFSET(KtlSystemBase, _KtlSystemsListEntry));
        isInitialized = TRUE;
    }

    return result;
}

KNodeList<KtlSystemBase>&
KtlSystemBase::GetKtlSystemBases()
{
    KNodeList<KtlSystemBase>*   result = nullptr;

    K_LOCK_BLOCK(gs_GlobalLock.Lock())
    {
        result = UnsafeGetKtlSystemBases();
    }

    return *result;
}

KtlSystemBase* KtlSystemBase::TryFindKtlSystem(__in GUID& ForId)
{
    K_LOCK_BLOCK(gs_GlobalLock.Lock())
    {
        KNodeList<KtlSystemBase>&   systemsList = *UnsafeGetKtlSystemBases();
        KtlSystemBase*              nextSystem = systemsList.PeekHead();

        while (nextSystem != nullptr)
        {
            if (nextSystem->GetInstanceId() == ForId)
            {
                return nextSystem;
            }
            nextSystem = systemsList.Successor(nextSystem);
        }
    }

    return nullptr;
}

VOID
KtlSystemBase::DebugOutputKtlSystemBases()
{
    K_LOCK_BLOCK(gs_GlobalLock.Lock())
    {
        KNodeList<KtlSystemBase>&   systemsList = *UnsafeGetKtlSystemBases();
        KtlSystemBase*              nextSystem = systemsList.PeekHead();

        KDbgPrintf("KtlSystemBase::DebugOutputKtlSystemBases(): %u Systems...\n", systemsList.Count());

        while (nextSystem != nullptr)
        {
            nextSystem->DumpDebugInfo();
            nextSystem = systemsList.Successor(nextSystem);
        }
    }
}

LONG
KtlSystemBase::ObjectEntryKey::Compare(
    __in ObjectEntryKey& First,
    __in ObjectEntryKey& Second
    )
{
    return wcscmp(First.String, Second.String);
}

//* KtlSystem imp
KtlSystem::KtlSystem()
{
}

KtlSystem::~KtlSystem()
{
}

KAllocator&
KtlSystem::GlobalNonPagedAllocator()
{
    return GetDefaultKtlSystem().NonPagedAllocator();
}

KAllocator&
KtlSystem::GlobalPagedAllocator()
{
    return GetDefaultKtlSystem().PagedAllocator();
}

#if KTL_USER_MODE
BOOLEAN
KtlSystem::IsCurrentThreadOwned()
{
	K_LOCK_BLOCK(KtlSystemBase::gs_GlobalLock.Lock())
	{
		KNodeList<KtlSystemBase>&   systemsList = *KtlSystemBase::UnsafeGetKtlSystemBases();
		KtlSystemBase*              nextSystem = systemsList.PeekHead();

		while (nextSystem != nullptr)
		{
			if (nextSystem->_IsActive)
			{
				KThreadPool::SPtr	tp = &nextSystem->DefaultThreadPool();

				if (tp)
				{
					KInvariant(tp->IsCurrentThreadOwnedSupported());
					if (tp->IsCurrentThreadOwned())
					{
						return TRUE;
					}
				}

				tp = &nextSystem->DefaultSystemThreadPool();
				if (tp)
				{
					KInvariant(tp->IsCurrentThreadOwnedSupported());
					if (tp->IsCurrentThreadOwned())
					{
						return TRUE;
					}
				}
			}
			nextSystem = systemsList.Successor(nextSystem);
		}
	}

	return FALSE;
}
#endif

//
// KtlSystemChildCleanup methods
//

KtlSystemChildCleanup::KtlSystemChildCleanup(
    __in KtlSystem& ParentKtlSystem
    ) : _ParentKtlSystem(ParentKtlSystem), _ChildKtlSystem(nullptr)
{
}

KtlSystemChildCleanup::~KtlSystemChildCleanup()
{
}

VOID
KtlSystemChildCleanup::RunCleanup(
    __in KtlSystemChild& ChildKtlSystem,
    __in ULONG MaxDeactiveDurationInMsecs
    )
{
    //
    // Each KtlSystemChildCleanup should only be attached to one child KTL system.
    // Otherwise we run the risk of leaking the child KTL system.
    //
    PVOID p = InterlockedExchangePointer((volatile PVOID*)(&_ChildKtlSystem), &ChildKtlSystem);
    KAssert(!p);
#if !DBG
    UNREFERENCED_PARAMETER(p);
#endif

    _ChildDeactivateTimeInMsecs = MaxDeactiveDurationInMsecs;

    //
    // Queue this object as a work item to the parent KtlSystem's thread pool.
    //
    _ParentKtlSystem.DefaultSystemThreadPool().QueueWorkItem(*this);
}

VOID KtlSystemChildCleanup::Execute()
{
    KAssert(_ChildKtlSystem != nullptr);

    //
    // Shut down the child KTL system. This call blocks until all resources inside the child KTL system are drained.
    //
    _ChildKtlSystem->Deactivate(_ChildDeactivateTimeInMsecs);

    //
    // Notify the derived class that the child KTL system has been deactivated and is going to be deleted.
    //
    OnChildKtlSystemDeactivated();

    //
    // The memory for the _ChildKtlSystem object is allocated in the parent KTL system.
    // Delete it now.
    //
    _delete(_ChildKtlSystem);
    _ChildKtlSystem = nullptr;

    //
    // Delete this clean up object. Its responsibility is to clean up _ChildKtlSystem.
    // _ChildKtlSystem is gone now.
    //
    _delete(this);
}

const KTagType KtlSystemChild::ThisTypeTag = "SysChild";

