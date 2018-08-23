/*++

    (c) 2011 by Microsoft Corp. All Rights Reserved.

    KtlSystem.h

    Description:
        Types and definition published by the KtlSystemBase class

    History:
        richhas            14-July-2011         Initial version.

--*/

#pragma once

#if DBG
#ifndef TRACK_ALLOCATIONS
#define TRACK_ALLOCATIONS
#endif
#endif

#include <ktl.h>

//* KtlSysBaseNonPageAlignedAllocator
template <POOL_TYPE PoolType>
class KtlSysBaseNonPageAlignedAllocator : public KtlCoreNonPageAlignedAllocator<PoolType>
{
public:
    KtlSysBaseNonPageAlignedAllocator(KtlSystemBase* const system);
    virtual ~KtlSysBaseNonPageAlignedAllocator();
};

template <POOL_TYPE PoolType>
KtlSysBaseNonPageAlignedAllocator<PoolType>::KtlSysBaseNonPageAlignedAllocator(KtlSystemBase* const System)
#if KTL_USER_MODE
    :   KtlCoreNonPageAlignedAllocator<PoolType>(System->GetHeapHandle(), System)
#else
    :   KtlCoreNonPageAlignedAllocator<PoolType>(System)
#endif
{
}

template <POOL_TYPE PoolType>
KtlSysBaseNonPageAlignedAllocator<PoolType>::~KtlSysBaseNonPageAlignedAllocator()
{
    if (this->GetKtlSystem().GetStrictAllocationChecks())
    {
#if KTL_USER_MODE
#if defined(TRACK_ALLOCATIONS)
        KInvariant(this->_AllocatedEntries.Count() == 0);
#else
        KInvariant(this->GetAllocsRemaining() == 0);
#endif
#else
        KInvariant(this->_AllocatedEntries.Count() == 0);
#endif
    }
}

//* KtlSysBasePageAlignedAllocator
class KtlSysBasePageAlignedAllocator : public KtlCorePageAlignedAllocator
{
public:
    KtlSysBasePageAlignedAllocator(KtlSystemBase* const System);
    virtual ~KtlSysBasePageAlignedAllocator();
};

//* KtlSystemBase
class KtlSystemBase : public KtlSystem
{
public:
    // BUG, richhas, xxxxx, allow the various heaps to be limited in size; default to unlimited
    // BUG, richhas, xxxxx, allow the various thread pool threads to be limited; default to thread pool limits
    FAILABLE KtlSystemBase(__in const GUID& InstanceId, ULONG ThreadPoolThreadLimit = 0);

    ~KtlSystemBase();

    VOID
    Activate(__in ULONG MaxActiveDurationInMsecs) override; // keeps thread until error or safe to start using the environment

    VOID
    Deactivate(__in ULONG MaxDeactiveDurationInMsecs) override;

    GUID
    GetInstanceId() override;

    KAllocator&
    NonPagedAllocator() override;

    KAllocator&
    PagedAllocator() override;

    KAllocator&
    PageAlignedRawAllocator() override;

    KThreadPool&
    DefaultThreadPool() override;

    KThreadPool&
    DefaultSystemThreadPool() override;

    NTSTATUS
    RegisterGlobalObject(
        __in_z const WCHAR* ObjectName,
        __inout KSharedBase& Object) override;

    VOID
    UnregisterGlobalObject(__in_z const WCHAR* ObjectName) override;

    KSharedBase::SPtr
    GetGlobalObjectSharedBase(__in_z const WCHAR* ObjectName) override;

    void
    SetStrictAllocationChecks(BOOLEAN AllocationChecksOn) override;

    BOOLEAN
    GetStrictAllocationChecks() override;

    #if KTL_USER_MODE
    HANDLE
    GetHeapHandle()
    {
        return(_HeapHandle);
    }
    #endif

    #if KTL_USER_MODE
        // Controls what thread pool is used by KAsyncContextBase derivations for their internal
        // threading needs. By default the System thread pool is used in user-mode while in
        // kernel-mode KTL's dedicated thread pool is the default. This behavior may be overriden
        // via this method for the async with a current KtlSystem instance and then on an async by
        // async basis via the KAsyncContextBase::SetDefaultSystemThreadPoolUsage().
        void
        SetDefaultSystemThreadPoolUsage(BOOLEAN OnOrOff) override;

        BOOLEAN
        GetDefaultSystemThreadPoolUsage() override;

        #if KTL_USER_MODE
            void SetEnableAllocationCounters(bool ToEnabled) override;
        #endif
    #endif

    static KNodeList<KtlSystemBase>&
    GetKtlSystemBases();

    // Returns nullptr if not found. NOTE: If non-nullptr returned, it is up to the application to keep that pointer valid
    static KtlSystemBase* TryFindKtlSystem(__in GUID& ForId);

    static VOID
    DebugOutputKtlSystemBases();

    static volatile LONG CountOfSystemInstances;

protected:
    //* Derivation override API

    //  Allow the derivation aspects to activate to a safe point - useful work can be done by
    //  the caller of KtlSystem::Activate().
    //
    //  Parameters:
    //      MaxActiveDurationInMsecs - Max duration the derivation is allowed to activate to
    //                                 a useful state.
    //                                 A value of KtlSystem::InfiniteTime indicates no timeout
    //                                 should be applied.
    //
    //  Return:
    //      Status() is set to the return value.
    //          NOTE: The public API will be non-functional if Status() != STATUS_SUCCESS
    //
    //  Default behavior: Returns STATUS_SUCCESS
    //
    virtual NTSTATUS
    StartSystemInstance(__in ULONG MaxActiveDurationInMsecs);

    //  Inform the derivation that it should trigger a complete shutdown of its resources.
    //
    //  Description:
    //      It is assumed that the derivation will not keep this thread but only triggers
    //      a (shutdown). This should result in ALL allocated memory being released. This
    //      implies that all thread pool activity will also stop given there is not application
    //      (derivation) state. The KtlSystemBase implementation does the following during the
    //      KtlSystem::Deactivate() call:
    //          1) Computes the time when it should give up wait for the derivation to free all
    //             of its memory allocations.
    //          2) It informs the derivation via this method to start total shutdown.
    //          3) It waits for all application heaps to go empty (if not already) - or timeout.
    //          4) It waits for all thread pool activity to cease - or timeout.
    //          5) It triggers eventual dtor of the instance's thread pools.
    //          6) It waits for for the thread pools to dtor - or timeout.
    //          7) It releases it's internal heaps.
    //
    //      If a timeout is detected, TimeoutDuringDeactivation() is called allowing the derivation
    //      to control how deactivation timeouts are handled. If TimeoutDuringDeactivation() returns,
    //      the instance's Status() is set to STATUS_UNSUCCESSFUL - disabling it's public API supporting
    //      memory allocations.
    //
    //  Return:
    //      Any non-successful status will cause Status() to be set to that value.
    //
    //  Default Behavior:
    //      Returns STATUS_SUCCESS.
    //
    virtual NTSTATUS
    StartSystemInstanceShutdown();

    //  Allow the derivation to handle a timeout situation during the deactivation process.
    //
    //  Description:
    //      If a timeout is detected during deactivation, TimeoutDuringDeactivation() is called allowing
    //      the derivation to control how deactivation timeouts are handled. If TimeoutDuringDeactivation()
    //      returns, the instance's Status() is set to STATUS_UNSUCCESSFUL - disabling it's public API
    //      supporting memory allocations.
    //
    //  Default Behavior:   FAIL-FASTS
    //
    virtual
    VOID
    TimeoutDuringDeactivation();

    virtual
    VOID
    DumpSystemInstanceDebugInfo();

private:
	friend KtlSystem;

    KtlSystemBase();

    BOOLEAN                             // Returns FALSE on timeout
    WaitUntil(KEvent& Event, ULONGLONG UntilTimeOfDayInMsecs);

    VOID
    DumpDebugInfo();

    VOID
    DumpDebugInstanceId(UCHAR* Result);     // Result must point to UCHAR[SizeOfAsciiGUIDStr];

    static KNodeList<KtlSystemBase>*
    UnsafeGetKtlSystemBases();

private:
    static KGlobalSpinLockStorage     gs_GlobalLock;
    static ULONG const                SizeOfAsciiGUIDStr = 32 + 7;

private:

    struct ObjectEntryKey {

        static
        LONG
        Compare(
            __in ObjectEntryKey& First,
            __in ObjectEntryKey& Second
            );

        KTableEntry TableEntry;
        const WCHAR* String;

    };

    class ObjectEntry : public ObjectEntryKey {
    public:
        KWString ObjectName;
        KSharedBase::SPtr Object;

        ObjectEntry(KAllocator& Alloc) : ObjectName(Alloc) {}
    };

    typedef KtlSysBaseNonPageAlignedAllocator<NonPagedPool>  NonPagedKAllocator;
    typedef KtlSysBaseNonPageAlignedAllocator<PagedPool>     PagedKAllocator;
    typedef KNodeTable<ObjectEntry, ObjectEntryKey>          ObjectTable;

    KSpinLock                       _ThisLock;
    KListEntry                      _KtlSystemsListEntry;
    BOOLEAN volatile                _IsActive;
    const GUID                      _InstanceId;
    BOOLEAN                         _IsSystemThreadPoolDefault;

    // Heap used only for the first order contents of this instance
    NonPagedKAllocator*             _SystemHeap;
    BOOLEAN                         _AllocationChecksOn;

    NonPagedKAllocator*             _NonPagedThreadPoolAllocator;
    NonPagedKAllocator*             _NonPagedAllocator;
    PagedKAllocator*                _PagedAllocator;
    KtlSysBasePageAlignedAllocator* _RawPageAlignedAllocator;
    KThreadPool::SPtr               _ThreadPool;
    KThreadPool::SPtr               _SystemThreadPool;

    #if KTL_USER_MODE
    HANDLE _HeapHandle;
    #endif

    //
    // General object table.
    //

    ObjectTable::CompareFunction    _ObjectCompare;
    ObjectTable                     _ObjectTable;

};

//
// A KtlSystemChildCleanup object is used to deactivate and clean up all resources related to a child KTL system.
// It is allocated from the memory pool of a parent KTL system and runs in the parent KTL system's thread pool.
//
// KtlSystemChildCleanup::RunCleanup() can be used to construct a KtlSystemChild::ShutdownCallback delegate.
//

class KtlSystemChild;

class KtlSystemChildCleanup : public KThreadPool::WorkItem
{
    K_DENY_COPY(KtlSystemChildCleanup);

public:

    KtlSystemChildCleanup(
        __in KtlSystem& ParentKtlSystem
        );

    VOID
    RunCleanup(
        __in KtlSystemChild& ChildKtlSystem,
        __in ULONG MaxDeactiveDurationInMsecs
        );

protected:

    friend VOID _delete<KtlSystemChildCleanup>(KtlSystemChildCleanup* HeapObj);
    friend class KThreadPool;

    //
    // Disallow stack construction.
    // The cleanup object's lifetime is much longer than a function call.
    //
    KtlSystemChildCleanup();
    virtual ~KtlSystemChildCleanup();

    //
    // KThreadPool::WorkItem method
    //
    VOID Execute();

    //
    // Notify the derived class that the child KTL system has been deactivated and is going to be deleted.
    //
    virtual void OnChildKtlSystemDeactivated() {}

    KtlSystem&          _ParentKtlSystem;
    KtlSystemChild*     _ChildKtlSystem;
    ULONG               _ChildDeactivateTimeInMsecs;
};

//
// This class represents a 'child' KTL system allocated by a 'parent' KTL system.
//
// A 'parent' KTL system can spawn many 'child' KTL systems. For each child, the parent
// allocates a KtlSystemChild object and a KtlSystemChild::ShutdownCallback delegate object from its own memory pool.
//
// When a child KTL system is no longer needed, any code inside or outside of the child KTL system
// can safely call KtlSystemChild::Shutdown() to free all resources related to the child KTL system.
//
// The lifetime of the parent KTL system is assumed to be longer than any of its child KTL system.
//
// Because the shutdown delegate object and child KTL system object are allocated from the parent's memory pool,
// the parent's built-in leak detection, waiting for resource drain during deactivation etc. all apply.
//

class KtlSystemChild : public KtlSystemBase
{
public:

    typedef KDelegate<VOID(
        __in KtlSystemChild& ChildKtlSystem,
        __in ULONG MaxDeactiveDurationInMsecs
        )> ShutdownCallback;

    FAILABLE
    KtlSystemChild(
        __in GUID& InstanceId,
        __in ShutdownCallback Callback
        ) : KtlSystemBase(InstanceId), _ShutdownCallback(Callback)
    {
        SetTypeTag(ThisTypeTag);
    }

    virtual ~KtlSystemChild() {}

    VOID
    Shutdown(
        __in ULONG MaxDeactiveDurationInMsecs = 5000
        )
    {
        //
        // The delegate is assumed to be allocated and run outside of this child KTL system.
        // Its lifetime is longer than this child KTL system.
        //
        _ShutdownCallback(*this, MaxDeactiveDurationInMsecs);
    }

    static const KTagType ThisTypeTag;

protected:
    ShutdownCallback _ShutdownCallback;
};
