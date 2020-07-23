// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "servicewrapper.h"
#include "../inc/KtlLogCounters.h"

// CONSIDER: Figure out a way to make OverlayStream and OverlayContainer
// proper derivations from RvdLog counterparts

// CONSIDER: Change the GlobalTable template to be based on a
// KNodeHashTable and have need no allocations when adding and removing
// from the tables. Also place the hashing function in the table and
// not in the objects that are placed in the table

//
// Generic template of a class that supports a global table of objects
// using a scalar key.
//
template <class T, class Key>
class GlobalTable : public KObject<GlobalTable<T, Key>>
{
    public:
        GlobalTable<T, Key>(
            __in KAllocator& Allocator,
            __in ULONG AllocationTag
        );

        ~GlobalTable<T, Key>();

        NTSTATUS AddObject(
            __in KSharedPtr<T>& Object,
            __in Key& ObjectKey
        );

        NTSTATUS AddObjectNoLock(
            __in KSharedPtr<T>& Object,
            __in const Key& ObjectKey
        );

        NTSTATUS AddOrUpdateObject(
            __in KSharedPtr<T>& Object,
            __in Key& ObjectKey
        );

        NTSTATUS AddOrUpdateObjectNoLock(
            __in KSharedPtr<T>& Object,
            __in Key& ObjectKey
        );

        NTSTATUS RemoveObject(
            __out KSharedPtr<T>& Object,
            __in Key& ObjectKey
        );

        NTSTATUS RemoveObjectNoLock(
            __out KSharedPtr<T>& Object,
            __in const Key& ObjectKey
        );
        NTSTATUS LookupObject(
            __out KSharedPtr<T>& Object,
            __in Key& ObjectKey
        );
        NTSTATUS LookupObjectNoLock(
            __out KSharedPtr<T>& Object,
            __in const Key& ObjectKey
        );

        NTSTATUS GetFirstObject(
            __out KSharedPtr<T>& Object,
            __out Key& ObjectKey,
            __out PVOID& Index
        );

        NTSTATUS GetFirstObjectNoLock(
            __out KSharedPtr<T>& Object,
            __out Key& ObjectKey,
            __out PVOID& Index
        );

        NTSTATUS GetNextObject(
            __inout PVOID& Index,
            __out KSharedPtr<T>& Object
        );

        NTSTATUS GetNextObjectNoLock(
            __inout PVOID& Index,
            __out KSharedPtr<T>& Object
        );

        VOID Clear(
        );

        KSpinLock& GetLock()
        {
            return(_Lock);
        }

        class Entry : public KObject<Entry>
        {
            friend class GlobalTable<T, Key>;

            public:
                Entry();
                ~Entry();

                static LONG
                Comparator(Entry& Left, Entry& Right)
                {
                    // -1 if Left < Right
                    // +1 if Left > Right
                    //  0 if Left == Right
                    return(T::CompareRoutine(Left._ObjectKey, Right._ObjectKey));
                };

                static ULONG
                GetLinksOffset() { return FIELD_OFFSET(Entry, _TableEntry); };

            private:
                KTableEntry _TableEntry;
                Key _ObjectKey;
                KSharedPtr<T> _Object;
        };

        friend Entry;

    protected:
        KAllocator& _Allocator;
        ULONG _AllocationTag;

    private:
        KNodeTable<Entry> _Table;
        KSpinLock _Lock;
        Entry _LookupKey;
};


template <class T, class Key>
GlobalTable<T,Key>::Entry::Entry()
{
}

template <class T, class Key>
GlobalTable<T,Key>::Entry::~Entry()
{
}


template <class T, class Key> GlobalTable<T,Key>::GlobalTable(
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    ) :
    _Table(Entry::GetLinksOffset(),
           typename KNodeTable<Entry>::CompareFunction(&Entry::Comparator)),
    _Allocator(Allocator),
    _AllocationTag(AllocationTag)
{
}

template <class T, class Key> GlobalTable<T,Key>::~GlobalTable<T, Key>()
{
    KInvariant(_Table.Count() == 0);
}

template <class T, class Key>
NTSTATUS GlobalTable<T,Key>::AddObjectNoLock(
    __in KSharedPtr<T>& Object,
    __in const Key& ObjectKey
)
{
    NTSTATUS status;

    Entry* entry = _new(_AllocationTag, _Allocator) Entry();

    if (! entry)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, STATUS_INSUFFICIENT_RESOURCES, 0, 0, 0);
        return(status);
    }

    status = entry->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    entry->_Object = Object;
    entry->_ObjectKey = ObjectKey;

    if (! _Table.Insert(*entry))
    {
        //
        // Entry already exists - this shouldn't happen
        //
        KInvariant(FALSE);
    }

    return(STATUS_SUCCESS);
}

template <class T, class Key>
NTSTATUS GlobalTable<T,Key>::AddObject(
    __in KSharedPtr<T>& Object,
    __in Key& ObjectKey
)
{
    NTSTATUS status;

    K_LOCK_BLOCK(_Lock)
    {
        status = AddObjectNoLock(Object,
                                 ObjectKey);
    }

    return(status);
}

template <class T, class Key>
NTSTATUS GlobalTable<T,Key>::AddOrUpdateObjectNoLock(
    __in KSharedPtr<T>& Object,
    __in Key& ObjectKey
)
{
    NTSTATUS status;
    Entry* entry;

    _LookupKey._ObjectKey = ObjectKey;
    entry = _Table.Lookup(_LookupKey);
    if (entry != NULL)
    {
        //
        // Update existing object
        //
        entry->_Object = Object;
        return(STATUS_SUCCESS);
    } else {
        //
        // Add a new object
        //
        entry = _new(_AllocationTag, _Allocator) Entry();

        if (! entry)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            return(status);
        }

        status = entry->Status();
        if (! NT_SUCCESS(status))
        {
            return(status);
        }

        entry->_Object = Object;
        entry->_ObjectKey = ObjectKey;

        if (! _Table.Insert(*entry))
        {
            //
            // Entry already exists - this shouldn't happen
            //
            KInvariant(FALSE);
        }
    }

    return(STATUS_SUCCESS);
}

template <class T, class Key>
NTSTATUS GlobalTable<T,Key>::AddOrUpdateObject(
    __in KSharedPtr<T>& Object,
    __in Key& ObjectKey
)
{
    NTSTATUS status = STATUS_SUCCESS;

    K_LOCK_BLOCK(_Lock)
    {
        status = AddOrUpdateObjectNoLock(Object,
                                         ObjectKey);
    }

    return(status);
}


template <class T, class Key>
NTSTATUS GlobalTable<T,Key>::RemoveObjectNoLock(
    __out KSharedPtr<T>& Object,
    __in const Key& ObjectKey
)
{
    Entry* entry = NULL;

    _LookupKey._ObjectKey = ObjectKey;

    entry = _Table.Lookup(_LookupKey);

    if (entry == NULL)
    {
        return(STATUS_NOT_FOUND);
    }

    _Table.Remove(*entry);


    Object = entry->_Object;

    //
    // Release the reference taken when the entry was added to the
    // table.
    //
    _delete(entry);

    return(STATUS_SUCCESS);
}

template <class T, class Key>
NTSTATUS GlobalTable<T,Key>::RemoveObject(
    __out KSharedPtr<T>& Object,
    __in Key& ObjectKey
)
{
    NTSTATUS status = STATUS_SUCCESS;

    K_LOCK_BLOCK(_Lock)
    {
        status = RemoveObjectNoLock(Object, ObjectKey);
    }

    return(status);
}

template <class T, class Key>
NTSTATUS GlobalTable<T,Key>::LookupObjectNoLock(
    __out KSharedPtr<T>& Object,
    __in const Key& ObjectKey
)
{
    Entry* entry;

    _LookupKey._ObjectKey = ObjectKey;

    entry = _Table.Lookup(_LookupKey);

    if (entry == NULL)
    {
        return(STATUS_NOT_FOUND);
    }

    Object = entry->_Object;

    return(STATUS_SUCCESS);
}

template <class T, class Key>
NTSTATUS GlobalTable<T,Key>::LookupObject(
    __out KSharedPtr<T>& Object,
    __in Key& ObjectKey
)
{
    NTSTATUS status = STATUS_SUCCESS;

    K_LOCK_BLOCK(_Lock)
    {
        status = LookupObjectNoLock(Object,
                                    ObjectKey);
    }

    return(status);
}

template <class T, class Key>
NTSTATUS GlobalTable<T,Key>::GetFirstObjectNoLock(
    __out KSharedPtr<T>& Object,
    __out Key& ObjectKey,
    __out PVOID& Index
)
{
    Entry* entry;

    entry = _Table.First();

    if (entry == NULL)
    {
        return(STATUS_NOT_FOUND);
    }

    Object = entry->_Object;
    ObjectKey = entry->_ObjectKey;
    Index = (PVOID)entry;

    return(STATUS_SUCCESS);
}

template <class T, class Key>
NTSTATUS GlobalTable<T,Key>::GetFirstObject(
    __out KSharedPtr<T>& Object,
    __out Key& ObjectKey,
    __out PVOID& Index
)
{
    NTSTATUS status = STATUS_SUCCESS;

    K_LOCK_BLOCK(_Lock)
    {
        status = GetFirstObjectNoLock(Object, ObjectKey, Index);
    }

    return(status);
}

template <class T, class Key>
NTSTATUS GlobalTable<T,Key>::GetNextObjectNoLock(
    __inout PVOID& Index,
    __out KSharedPtr<T>& Object
)
{
    KInvariant(Index != NULL);

    Entry* entry = (Entry*)Index;

    entry = _Table.Next(*entry);

    if (entry == NULL)
    {
        return(STATUS_NOT_FOUND);
    }

    Object = entry->_Object;
    Index = (PVOID)entry;

    return(STATUS_SUCCESS);
}

template <class T, class Key>
NTSTATUS GlobalTable<T,Key>::GetNextObject(
    __inout PVOID& Index,
    __out KSharedPtr<T>& Object
)
{
    NTSTATUS status = STATUS_SUCCESS;

    K_LOCK_BLOCK(_Lock)
    {
        status = GetNextObjectNoLock(Index, Object);
    }

    return(status);
}

template <class T, class Key>
VOID GlobalTable<T,Key>::Clear(
)
{
    Entry* entry = NULL;

    //
    // Walk the table and remove each entry as we need to remove the
    // reference to each of them so they are freed. Entries cannot be
    // cleared while holding the spinlock as the destructor may invoke
    // a Zw api that should be called at passive level
    //
    do
    {
        K_LOCK_BLOCK(_Lock)
        {
            entry = _Table.First();
            if (entry != NULL)
            {
                _Table.Remove(*entry);
            }
        }

        if (entry != NULL)
        {
            entry->_Object = nullptr;
            _delete(entry);
        }
    } while (entry != NULL);
}

class KtlLogStreamKernel;
class OverlayLog;
class OverlayManager;
class LogStreamOpenGateContext;

class ThrottledKIoBufferAllocator : public KObject<ThrottledKIoBufferAllocator>, public KShared<ThrottledKIoBufferAllocator>
{
    K_FORCE_SHARED(ThrottledKIoBufferAllocator);

    public:
    ThrottledKIoBufferAllocator(
        __in KtlLogManager::MemoryThrottleLimits& MemoryThrottleLimits
       );

    static NTSTATUS
    CreateThrottledKIoBufferAllocator(
        __in KtlLogManager::MemoryThrottleLimits& MemoryThrottleLimits,
        __in KAllocator& Allocator,
        __in ULONG AllocationTag,
        __out ThrottledKIoBufferAllocator::SPtr& Context);


    public:
    class AsyncAllocateKIoBufferContext : public KAsyncContextBase
    {
        K_FORCE_SHARED(AsyncAllocateKIoBufferContext);

        friend ThrottledKIoBufferAllocator;

        public:
            AsyncAllocateKIoBufferContext(
                __in ThrottledKIoBufferAllocator& OM
            );

            VOID CompleteRequest(
                __in NTSTATUS Status
            )
            {
                Complete(Status);
            }

            //
            // Accessors
            //
            static const ULONG _AllocationExtentSize = 64 * 1024;
            static inline ULONG GetAllocationExtentSize()
            {
                return(_AllocationExtentSize);
            }

            inline LONGLONG GetSize()
            {
                return((LONGLONG)_Size);
            }

            inline ULONG GetSizeUlong()
            {
                return(_Size);
            }

            inline ULONGLONG GetTickTimeout()
            {
                return(_TickTimeout);
            }

            inline ULONGLONG GetTimeout()
            {
                return(_Timeout);
            }

            inline void SetIoBufferPtr(
                __in_opt KIoBuffer* IoBufferPtr
            )
            {
                *_IoBufferPtr = IoBufferPtr;
            }

            //
            // Size must be a multiple of 4K
            //
            VOID
            StartAllocateKIoBuffer(
                __in ULONG Size,
                __in ULONGLONG Timeout,
                __out KIoBuffer::SPtr& IoBuffer,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

            VOID DoComplete(
                __in NTSTATUS Status
            );

        protected:
            VOID
            OnStart(
                ) override ;

            VOID
            OnReuse(
                ) override ;

            VOID
            OnCancel(
                ) override ;

        private:
            enum { Initial,
                   WaitForFreeMemory, AllocateMemory,
                   Completed, CompletedWithError } _State;

            VOID FSMContinue(
                __in NTSTATUS Status
                );

            VOID OperationCompletion(
                __in_opt KAsyncContextBase* const ParentAsync,
                __in KAsyncContextBase& Async
                );

        private:
            //
            // General members
            //
            ThrottledKIoBufferAllocator::SPtr _OM;
            KListEntry _AllocsWaitingListEntry;

            //
            // Parameters to api
            //
            LONG _Size;
            KIoBuffer::SPtr* _IoBufferPtr;
            ULONGLONG _Timeout;

            //
            // Members needed for functionality
            //
            ULONGLONG _TickTimeout;
    };

    NTSTATUS
    CreateAsyncAllocateKIoBufferContext(
        __out AsyncAllocateKIoBufferContext::SPtr& Context);

    VOID FreeKIoBuffer(
        __in KActivityId ActivityId,
        __in LONG Size
    );

    VOID SetLimit(
        __in LONGLONG Min,
        __in LONGLONG Max,
        __in LONG PerStream
    );

    LONG AddToLimit(
    );

    VOID RemoveFromLimit(
        LONG Limit
    );

    inline LONGLONG GetFreeAllocation()
    {
        return( _TotalAllocationLimit - _CurrentAllocations );
    }

    inline BOOLEAN IsUnderMemoryPressure()
    {
        return( _WaitingAllocsList.Count() != 0 );
    }

    inline LONGLONG GetCurrentAllocations()
    {
        return(_CurrentAllocations);
    }

    inline LONGLONG GetTotalAllocationLimit()
    {
        return(_TotalAllocationLimit);
    }
    
    inline LONGLONG* GetCurrentAllocationsPointer()
    {
        return(&_CurrentAllocations);
    }

    inline LONGLONG* GetTotalAllocationLimitPointer()
    {
        return(&_TotalAllocationLimit);
    }

    VOID SetAllocationTimeoutInMs(ULONG AllocationTimeoutInMs)
    {
        _AllocationTimeoutInMs = AllocationTimeoutInMs;
    }
    
    VOID Shutdown();        

    // If there is an entry on the list then the timer will fire so
    // that the entry has a chance to be satisfied
    static const ULONG _PingTimerInMs = 1000;

    private:
        NTSTATUS AllocateIfPossible(
            __in BOOLEAN RetryAllocation,
            __in AsyncAllocateKIoBufferContext& WaitingAlloc
        );

        VOID ProcessFreedMemory();

        VOID ProcessTimedOutRequests();

        VOID AddToList(
            __in BOOLEAN InsertHead,
            __in AsyncAllocateKIoBufferContext& WaitingAlloc
        );

        ThrottledKIoBufferAllocator::AsyncAllocateKIoBufferContext::SPtr RemoveTopOfList();

        VOID TimerCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );

        BOOLEAN RestartTimerIfNeeded();

        ULONG GetAllocationTimeoutInMs()
        {
            return(_AllocationTimeoutInMs);
        }
        
    private:
        static const ULONG _ThrottleLinkageOffset;
        ULONG _AllocationTimeoutInMs;
        LONGLONG _TotalAllocationLimit;
        LONGLONG _MaxAllocationLimit;
        LONG _PerStreamAllocation;

        KSpinLock _Lock;
        KTimer::SPtr _Timer;
        BOOLEAN _TimerRunning;
        BOOLEAN _ShuttingDown;
        KNodeList<AsyncAllocateKIoBufferContext> _WaitingAllocsList;
        LONGLONG _CurrentAllocations;
};

class OverlayStreamBase : public WrappedServiceBase
{
#include "PhysLogStream.h"
};

class OverlayStream;

class OverlayGlobalContext : public KAsyncGlobalContext
{
    K_FORCE_SHARED(OverlayGlobalContext);

public:
    OverlayGlobalContext(
        __in KActivityId ActivityId
        );

    static NTSTATUS Create(
        __in OverlayStream& OS,
        __in KAllocator& Allocator,
        __in ULONG AllocationTag,
        __out OverlayGlobalContext::SPtr& GlobalContext);
};

class OverlayStream : public OverlayStreamBase
{
    K_FORCE_SHARED(OverlayStream);

    public:
        OverlayStream(
            __in OverlayLog& OverlayLog,
            __in RvdLogManager& BaseLogManager,
            __in RvdLog& BaseSharedLog,
            __in const KtlLogStreamId& StreamId,
            __in const KGuid& DiskId,
            __in_opt KString const * const Path,
            __in ULONG MaxRecordSize,
            __in LONGLONG StreamSize,
            __in ULONG StreamMetadataSize,
            __in ThrottledKIoBufferAllocator &ThrottledAllocator
            );

    //
    // Methods needed to satisfy WrappedServiceBase
    //
        static NTSTATUS CreateUnderlyingService(
            __out WrappedServiceBase::SPtr& Context,
            __in ServiceWrapper& FreeService,
            __in ServiceWrapper::Operation& OperationOpen,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag
        );

        NTSTATUS StartServiceOpen(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in_opt OpenCompletionCallback OpenCallbackPtr,
            __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr
        ) override ;

        NTSTATUS StartServiceClose(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in_opt CloseCompletionCallback CloseCallbackPtr
        ) override ;

    public:
        // Each request performed against the stream must call
        // TryAcquireRequestRef() successfully from the Start..()
        // before actually starting the request. It may return an error
        // in the case that the container is closing.
        //
        // ReleaseRequestRef() should be called when the operation has
        // completed and is about to be completed back up to the caller.
        //
        NTSTATUS TryAcquireRequestRef();
        VOID ReleaseRequestRef();

        //
        // This overlay container tracks the KTL shim objects using it
        //
        VOID
        AddKtlLogStream(
            __in KtlLogStreamKernel& StreamUser
        );

        VOID
        RemoveKtlLogStream(
            __in KtlLogStreamKernel& StreamUser
        );

        inline BOOLEAN IsClosed()
        {
            return(_ObjectState == Closed);
        }
        
        inline KtlLogStreamId GetStreamId()
        {
            return(_StreamId);
        }

        inline KSharedPtr<OverlayLog> GetOverlayLog()
        {
            return(_OverlayLog);
        }

        inline RvdLog::SPtr GetBaseSharedLog()
        {
            return(_BaseSharedLog);
        }
        
        inline ULONG GetMaxRecordSize()
        {
            return(_MaxRecordSize);
        }

        inline ULONG QueryMaxMetadataLength()
        {
            return(_StreamMetadataSize);
        }

        inline RvdLogStream::SPtr GetSharedLogStream()
        {
            return(_SharedLogStream);
        }

        inline RvdLogStream::SPtr GetDedicatedLogStream()
        {
            return(_DedicatedLogStream);
        }

        inline void SetSharedLogStream(RvdLogStream& Stream)
        {
            _SharedLogStream = &Stream;
        }

        inline void SetDedicatedLogStream(RvdLogStream& Stream)
        {
            _DedicatedLogStream = &Stream;
            if (_CoalesceRecords)
            {
                _CoalesceRecords->SetDedicatedLogStream(Stream);
            }
        }

        inline ULONG GetThrottledAllocatorBytes()
        {
            return(_ThrottledAllocatorBytes);
        }

        inline BOOLEAN IsNextRecordFirstWrite()
        {
            return(_IsNextRecordFirstWrite);
        }

        inline VOID ResetIsNextRecordFirstWrite()
        {
            KInvariant(_IsNextRecordFirstWrite);
            _IsNextRecordFirstWrite = FALSE;
        }

        inline VOID SetThrottledAllocatorBytes(
            ULONG ThrottledAllocatorBytes
        )
        {
            _ThrottledAllocatorBytes = ThrottledAllocatorBytes;
        }

        inline VOID SetDisableCoalescing(__in BOOLEAN Flag)
        {
            _DisableCoalescing = Flag;
        }
        inline BOOLEAN IsDisableCoalescing()
        {
            return(_DisableCoalescing);
        }

        inline KGuid GetDiskId()
        {
            return(_DiskId);
        }

        inline KString const * GetPath()
        {
            return(_Path.RawPtr());
        }

        inline KtlLogContainerId GetDedicatedContainerId()
        {
            return(_DedicatedContainerId);
        }

#if DBG
        inline ULONG GetSharedTimerDelay()
        {
            return(_SharedTimerDelay);
        }

        inline VOID SetSharedTimerDelay(__in ULONG Delay)
        {
            _SharedTimerDelay = Delay;
        }

        inline ULONG GetDedicatedTimerDelay()
        {
            return(_DedicatedTimerDelay);
        }

        inline VOID SetDedicatedTimerDelay(__in ULONG Delay)
        {
            _DedicatedTimerDelay = Delay;
        }

#endif
        inline OverlayGlobalContext::SPtr GetGlobalContext()
        {
            return(_GlobalContext);
        }

        static inline BOOLEAN IsStreamEmpty(
            __in RvdLogAsn LowAsn,
            __in RvdLogAsn HighAsn,
            __in RvdLogAsn TruncationAsn
        )
        {
            return( (LowAsn == HighAsn) && (HighAsn <= TruncationAsn) );
        }

        inline BOOLEAN IsStreamForLogicalLog()
        {
            return(_IsStreamForLogicalLog);
        }

        VOID SetSharedQuota(
            __in LONGLONG Quota
            )
        {
            _SharedLogQuota = Quota;
        }

        ULONGLONG GetStreamSize(
        )
        {
            return(_StreamSize);
        }

        LONGLONG GetSharedQuota(
        )
        {
            return(_SharedLogQuota);
        }

        LONGLONG* GetSharedQuotaPointer(
        )
        {
            return(&_SharedLogQuota);
        }

        VOID SetLastSharedLogStatus(
            __in NTSTATUS Status
        )
        {
            _LastSharedLogStatus = Status;
        }

        BOOLEAN IsUnderPressureToFlush();

        LONGLONG GetDedicatedWriteBytesOutstanding()
        {
            return(_DedicatedWriteBytesOutstanding);
        }

        LONGLONG GetDedicatedWriteBytesOutstandingThreshold()
        {
            return(_DedicatedWriteBytesOutstandingThreshold);
        }

        VOID SetSharedWriteBytesOutstanding(
            __in LONGLONG NumberBytes
            )
        {
            _SharedWriteBytesOutstanding = NumberBytes;
        }
        
        volatile LONGLONG* GetSharedWriteBytesOutstandingPointer()
        {
            return(&_SharedWriteBytesOutstanding);
        }

        VOID AddDedicatedWriteBytesOutstanding(
            __in ULONG NumberBytes
            )
        {
            LONGLONG numberBytes = (LONGLONG)NumberBytes;
            InterlockedAdd64(&_DedicatedWriteBytesOutstanding, numberBytes);
        }

        VOID SubtractDedicatedWriteBytesOutstanding(
            __in ULONG NumberBytes
            )
        {
            LONGLONG numberBytes = (LONGLONG)NumberBytes;
            InterlockedAdd64(&_DedicatedWriteBytesOutstanding, -1*numberBytes);
        }

        volatile LONGLONG* GetDedicatedWriteBytesOutstandingPointer()
        {
            return(&_DedicatedWriteBytesOutstanding);
        }

        LONGLONG* GetDedicatedWriteBytesOutstandingThresholdPointer()
        {
            return(&_DedicatedWriteBytesOutstandingThreshold);
        }

        VOID SetDedicatedWriteBytesOutstandingThreshold(__in LONGLONG Number)
        {
            _DedicatedWriteBytesOutstandingThreshold = Number;
        }

        LONGLONG* GetSharedWriteLatencyTimeAveragePointer()
        {
            return(&_SharedWriteLatencyTimeAverage);
        }

        LONGLONG* GetDedicatedWriteLatencyTimeAveragePointer()
        {
            return(&_DedicatedWriteLatencyTimeAverage);
        }
        
        VOID UpdateDedicatedWriteLatencyTime(
            __in LONGLONG DedicatedWriteLatencyTime
        );

        VOID UpdateSharedWriteLatencyTime(
            __in LONGLONG SharedWriteLatencyTime
        );        
        
        //
        // This is the delay time to use when debugging the scenario
        // where the shared or dedicated log is delayed to force
        // completions at different times.
        //
        static const ULONG DebugDelayTimeInMs = 250;

        NTSTATUS ComputeLogPercentageUsed(__out ULONG& PercentageUsed);

        NTSTATUS ComputeLogSizeAndSpaceRemaining(
            __out ULONGLONG& LogSize,
            __out ULONGLONG& SpaceRemaining
        );

        inline void SetLogicalLogTailOffset(__in KtlLogAsn TailOffset)
        {
            _LogicalLogTailOffset = TailOffset;
        }

        inline KtlLogAsn GetLogicalLogTailOffset()
        {
            return(_LogicalLogTailOffset);
        }

        inline void SetIfLargerLogicalLogTailOffset(__in KtlLogAsn TailOffset)
        {
            _LogicalLogTailOffset.SetIfLarger(TailOffset);
        }

        inline ULONGLONG GetLogicalLogLastVersion()
        {
            return(_LogicalLogLastVersion);
        }

        inline VOID SetIfLargerLogicalLogLastVersion(__in ULONGLONG Version)
        {
            // CONSIDER: Doing this in a way that is interlocked
            if (Version > _LogicalLogLastVersion)
            {
                _LogicalLogLastVersion = Version;
            }
        }

        inline NTSTATUS GetFailureStatus()
        {
            return(_FailureStatus);
        }

        inline void SetFailureStatus(__in NTSTATUS Status)
        {
            _FailureStatus = Status;
        }

        inline ULONG GetSharedMaximumRecordSize()
        {
            return(_SharedMaximumRecordSize);
        }

        inline void SetWriteOnlyToDedicated(__in BOOLEAN Flag)
        {
            _WriteOnlyToDedicated = Flag;
        }

        inline BOOLEAN GetWriteOnlyToDedicated()
        {
            return(_WriteOnlyToDedicated);
        }

        inline KtlLogAsn GetSharedTruncationAsn()
        {
            return(_SharedTruncationAsn);
        }

        inline VOID SetSharedTruncationAsn(__in KtlLogAsn TruncationAsn)
        {
            _SharedTruncationAsn = TruncationAsn;
        }

        inline VOID SetIfLargerSharedTruncationAsn(__in KtlLogAsn TruncationAsn)
        {
            _SharedTruncationAsn.SetIfLarger(TruncationAsn);
        }

        inline ULONGLONG GetSharedTruncationVersion(void)
        {
            return(_SharedTruncationVersion);
        }

        inline VOID AddPerfCounterDedicatedBytesWritten(__in LONGLONG TotalNumberBytesToWrite)
        {
            InterlockedAdd64(&_DedicatedBytesWritten, TotalNumberBytesToWrite);
            InterlockedAdd64(_ContainerDedicatedBytesWritten, TotalNumberBytesToWrite);
        }
        
        inline volatile LONGLONG* GetPerfCounterDedicatedBytesWrittenPointer()
        {
            return(&_DedicatedBytesWritten);
        }

        inline VOID AddPerfCounterSharedBytesWritten(__in LONGLONG TotalNumberBytesToWrite)
        {
            InterlockedAdd64(&_SharedBytesWritten, TotalNumberBytesToWrite);
            InterlockedAdd64(_ContainerSharedBytesWritten, TotalNumberBytesToWrite);
        }

        inline volatile LONGLONG* GetPerfCounterSharedBytesWrittenPointer()
        {
            return(&_SharedBytesWritten);
        }
        
        inline VOID AddPerfCounterBytesWritten(__in LONGLONG TotalNumberBytesToWrite)
        {
            InterlockedAdd64(&_ApplicationBytesWritten, TotalNumberBytesToWrite);
        }

        inline volatile LONGLONG* GetPerfConterApplicationBytesWrittenPointer()
        {
            return(&_ApplicationBytesWritten);
        }
        
        inline VOID UpdateSharedTruncationVersion(__in ULONGLONG Version)
        {
            K_LOCK_BLOCK(_SharedTruncationValuesLock)
            {
                if (_SharedTruncationVersion < Version)
                {
                    _SharedTruncationVersion = Version;
                }
            }
        }

        NTSTATUS CreateTruncateCompletedWaiter(
            __out KAsyncEvent::WaitContext::SPtr& WaitContext
        );

        VOID ResetTruncateCompletedEvent();
        
        inline KLogicalLogInformation::StreamBlockHeader* GetRecoveredLogicalLogHeader()
        {
            return(&_RecoveredLogicalLogHeader);
        }

        inline ULONG GetPeriodicFlushTimeInSec()
        {
            return(_PeriodicFlushTimeInSec);
        }

        inline ULONG GetPeriodicTimerIntervalInSec()
        {
            return(_PeriodicTimerIntervalInSec);
        }

#if defined(UDRIVER) || defined(UPASSTHROUGH)
        inline void SetPeriodicFlushTimeInSec(__in ULONG Time)
        {
            _PeriodicFlushTimeInSec = Time;
        }

        inline void SetPeriodicTimerIntervalInSec(__in ULONG Interval)
        {
            _PeriodicTimerIntervalInSec = Interval;
        }

        inline void SetPeriodicTimerTestDelayInSec(__in ULONG Seconds)
        {
            _PeriodicTimerTestDelayInSec = Seconds;
        }

        inline ULONG GetPeriodicTimerTestDelayInSec()
        {
            return(_PeriodicTimerTestDelayInSec);
        }
#endif

    private:

        VOID DeleteTailTruncatedRecords(
            __in BOOLEAN ForSharedLog,
            __in RvdLogAsn RecordAsn
        );

        NTSTATUS DetermineStreamTailAsn(
            __out KtlLogAsn& StreamTailAsn
            );

        class AsyncCopySharedToDedicatedContext : public KAsyncContextBase
        {
            K_FORCE_SHARED(AsyncCopySharedToDedicatedContext);

            public:
            AsyncCopySharedToDedicatedContext(
                __in OverlayStream& OStream,
                __in RvdLogStream& SharedLogStream,
                __in RvdLogStream& DedicatedLogStream
                );

                VOID
                StartCopySharedToDedicated(
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

            protected:
                VOID
                OnStart(
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

                VOID
                OnCompleted(
                    ) override ;

                VOID
                OnCancel(
                    ) override ;

            private:
                enum { Initial, ReadRecord, WriteRecord } _State;

                VOID FSMContinue(
                    __in NTSTATUS Status
                    );

                VOID OperationCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

            private:
                //
                // General members
                //
                OverlayStream::SPtr _OverlayStream;
                RvdLogStream::SPtr  _SharedLogStream;
                RvdLogStream::SPtr  _DedicatedLogStream;
                RvdLogStream::AsyncReadContext::SPtr _SharedRead;
                RvdLogStream::AsyncWriteContext::SPtr _DedicatedWrite;

                //
                // Parameters to api
                //

                //
                // Members needed for functionality (reused)
                //
                RvdLogAsn _CopyAsn;
                ULONGLONG _CopyVersion;
                KBuffer::SPtr _CopyMetadataBuffer;
                KIoBuffer::SPtr _CopyIoBuffer;
        };

        NTSTATUS
        CreateAsyncCopySharedToDedicatedContext(__out AsyncCopySharedToDedicatedContext::SPtr& Context);

        class AsyncCopySharedToBackupContext : public KAsyncContextBase
        {
            K_FORCE_SHARED(AsyncCopySharedToBackupContext);

            public:
                AsyncCopySharedToBackupContext(
                    __in OverlayStream& OStream,
                    __in RvdLogManager& LogManager, 
                    __in RvdLogStream& SharedLogStream
                );

                VOID
                StartCopySharedToBackup(
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

            protected:
                VOID
                OnStart(
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

                VOID
                OnCompleted(
                    ) override ;

                VOID
                OnCancel(
                    ) override ;

            private:
                enum { Initial, CreateBackupLog, CreateBackupStream,
                      CopyRecords, ReadRecord, WriteRecord } _State;

                VOID FSMContinue(
                    __in NTSTATUS Status
                    );

                VOID OperationCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

            private:
                //
                // General members
                //
                OverlayStream::SPtr _OverlayStream;

                //
                // Parameters to api
                //
                RvdLogManager::SPtr _LogManager;
                RvdLogStream::SPtr  _SharedLogStream;

                //
                // Members needed for functionality (reused)
                //
                KString::SPtr _Path;
                RvdLogManager::AsyncOpenLog::SPtr _OpenLog;
                RvdLogManager::AsyncCreateLog::SPtr _CreateLog;
                RvdLog::AsyncOpenLogStreamContext::SPtr _OpenStream;
                RvdLog::AsyncCreateLogStreamContext::SPtr _CreateStream;
                
                RvdLog::SPtr _BackupLogContainer;
                RvdLogStream::SPtr  _BackupLogStream;
                RvdLogStream::AsyncReadContext::SPtr _SharedRead;
                RvdLogStream::AsyncWriteContext::SPtr _BackupWrite;
                
                RvdLogAsn _CopyAsn;
                ULONGLONG _CopyVersion;
                KBuffer::SPtr _CopyMetadataBuffer;
                KIoBuffer::SPtr _CopyIoBuffer;
                KWString _LogType;
        };

        NTSTATUS
        CreateAsyncCopySharedToBackupContext(
            __out AsyncCopySharedToBackupContext::SPtr& Context
            );
        
    public:
        //
        // Unit test support routines
        //
        typedef enum { Unassigned = 0,
                      OpenInitial = 1, AllocateResources = 2, WaitForGate = 3,
                      OpenSharedStream = 4, OpenDedicatedContainer = 5,
                      OpenDedicatedStream = 6, CopyFromSharedToDedicated = 7, FinishOpen = 8,
                      RecoverTailTruncationAsn = 9, RecoverTailTruncationAsnNoRecord = 10,
                      CloseDedicatedContainer = 11, WaitForDedicatedContainerClose = 12,
                      CopySharedLogDataToBackup = 13, CleanupStreamResources = 14, BeginCopyFromSharedToDedicated = 15,
                      VerifyDedicatedSharedContiguousness = 16,
               CloseInitial = 100, CloseDedicatedLog = 101
             } OpenCloseState;

        inline OpenCloseState GetOpenCloseState()
        {
            return(_State);
        }

        inline void SetOpenCloseState(__in OpenCloseState State)
        {
            _State = State;
        }

        typedef KDelegate<VOID(
            __in OverlayStream& OverlayStream,
            __in OpenCloseState OldState,
            __in OpenCloseState NewState
            )> OpenServiceFSMCallout;

        VOID SetOpenServiceFSMCallout(__in_opt OpenServiceFSMCallout Callout)
        {
            _OpenServiceFSMCallout = Callout;
        }

        OpenServiceFSMCallout _OpenServiceFSMCallout;

    protected:
        VOID OnServiceOpen() override ;
        VOID OnServiceClose() override ;
        VOID OnServiceReuse() override ;

    private:
        enum { Closed, Opened } _ObjectState;

        OpenCloseState _State;

        VOID AdvanceState(__in OpenCloseState NewState)
        {
            if (_OpenServiceFSMCallout)
            {
                _OpenServiceFSMCallout(*this, _State, NewState);
            }
            _State = NewState;
        }


        VOID
        DoCompleteOpen(
            __in NTSTATUS Status
            );

        VOID OpenServiceFSM(
            __in NTSTATUS Status
            );

        VOID CloseServiceFSM(
            __in NTSTATUS Status
            );

        VOID OpenServiceCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );

        VOID OpenServiceReadRecordCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );

        VOID CloseServiceCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );

        VOID CheckThresholdNotifications(
            );

    //
    // Stream metadata read/write
    //
    public:
        class AsyncWriteMetadataContextOverlay : public KAsyncContextBase
        {
            K_FORCE_SHARED(AsyncWriteMetadataContextOverlay);

            public:
                AsyncWriteMetadataContextOverlay(
                    __in OverlayStream& OStream
                    );

                VOID
                StartWriteMetadata(
                    __in const KIoBuffer::SPtr& IoBuffer,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

            protected:
                VOID
                OnStart(
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

                VOID
                OnCompleted(
                    ) override ;

                VOID
                OnCancel(
                    ) override ;

            private:
                enum { Initial, OpenDedMBInfo, WriteMetadata, CloseDedMBInfo } _State;

                VOID FSMContinue(
                    __in NTSTATUS Status
                    );

                VOID OperationCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

            private:
                //
                // General members
                //
                OverlayStream::SPtr _OverlayStream;

                //
                // Parameters to api
                //
                KIoBuffer::SPtr _IoBuffer;

                //
                // Members needed for functionality (reused)
                //
                DedicatedLCMBInfoAccess::SPtr _DedMBInfo;
                DedicatedLCMBInfoAccess::AsyncWriteMetadataContext::SPtr _WriteMetadata;
                BOOLEAN _RequestRefAcquired;
                NTSTATUS _LastStatus;
        };

        NTSTATUS
        CreateAsyncWriteMetadataContext(__out AsyncWriteMetadataContextOverlay::SPtr& Context);


        class AsyncReadMetadataContextOverlay : public KAsyncContextBase
        {
            K_FORCE_SHARED(AsyncReadMetadataContextOverlay);

            public:
                AsyncReadMetadataContextOverlay(
                    __in OverlayStream& OStream
                    );

                VOID
                StartReadMetadata(
                    __out KIoBuffer::SPtr& IoBuffer,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

            protected:
                VOID
                OnStart(
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

                VOID
                OnCompleted(
                    ) override ;

                VOID
                OnCancel(
                    ) override ;

            private:
                enum { Initial, OpenDedMBInfo, ReadMetadata, CloseDedMBInfo } _State;

                VOID FSMContinue(
                    __in NTSTATUS Status
                    );

                VOID OperationCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

            private:
                 //
                // General members
                //
                OverlayStream::SPtr _OverlayStream;

                //
                // Parameters to api
                //
                KIoBuffer::SPtr* _IoBufferPtr;

                //
                // Members needed for functionality (reused)
                //
                KIoBuffer::SPtr _IoBuffer;
                DedicatedLCMBInfoAccess::SPtr _DedMBInfo;
                DedicatedLCMBInfoAccess::AsyncReadMetadataContext::SPtr _ReadMetadata;
                BOOLEAN _RequestRefAcquired;
                NTSTATUS _LastStatus;
        };

        NTSTATUS
        CreateAsyncReadMetadataContext(__out AsyncReadMetadataContextOverlay::SPtr& Context);

        class AsyncIoctlContextOverlay : public KAsyncContextBase
        {
            K_FORCE_SHARED(AsyncIoctlContextOverlay);

            public:
                AsyncIoctlContextOverlay(
                    __in OverlayStream& OStream,
                    __in RvdLog& DedicatedLogContainer
                    );

                VOID
                StartIoctl(
                    __in ULONG ControlCode,
                    __in_opt KBuffer* const InBuffer,
                    __out ULONG& Result,
                    __out KBuffer::SPtr& OutBuffer,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

            protected:
                VOID
                OnStart(
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

                VOID
                OnCompleted(
                    ) override ;

                VOID
                OnCancel(
                    ) override ;

            private:
                enum { Initial, Completed, CompletedWithError } _State;

                VOID FSMContinue(
                    __in NTSTATUS Status
                    );

                VOID OperationCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

            private:
                //
                // General members
                //
                OverlayStream::SPtr _OverlayStream;
                RvdLog::SPtr _DedicatedLogContainer;

                //
                // Parameters to api
                //
                ULONG _ControlCode;
                KBuffer::SPtr _InBuffer;
                ULONG* _Result;
                KBuffer::SPtr* _OutBuffer;

                //
                // Members needed for functionality (reused)
                //
                BOOLEAN _RequestRefAcquired;
        };

        NTSTATUS
        CreateAsyncIoctlContext(__out AsyncIoctlContextOverlay::SPtr& Context);



    //
    // Interface to the stream should match that of the core logger
    //
    public:
        LONGLONG
        QueryLogStreamUsage() override ;

        VOID
        QueryLogStreamType(__out RvdLogStreamType& LogStreamType) override ;

        VOID
        QueryLogStreamId(__out RvdLogStreamId& LogStreamId) override ;

        NTSTATUS
        QueryRecordRange(
            __out_opt RvdLogAsn* const LowestAsn,
            __out_opt RvdLogAsn* const HighestAsn,
            __out_opt RvdLogAsn* const LogTruncationAsn) override ;

        ULONGLONG QueryCurrentReservation() override ;

        NTSTATUS
        SetTruncationCompletionEvent(__in_opt KAsyncEvent* const EventToSignal)
        {
            UNREFERENCED_PARAMETER(EventToSignal);
            KInvariant(FALSE);
            return(STATUS_SUCCESS);
        }
        
        
        class AsyncDestagingWriteContextOverlay;

        class CoalesceRecords : public KObject<CoalesceRecords>, public KShared<CoalesceRecords>
        {
            K_FORCE_SHARED(CoalesceRecords);

            friend OverlayStream;

        public:
            class AsyncAppendOrFlushContext;

            CoalesceRecords(
                __in OverlayStream& OS,
                __in RvdLog& DedicatedLogContainer,
                __in RvdLogStream& DedicatedLogStream,
                __in ThrottledKIoBufferAllocator& ThrottledAllocator,
                __in KActivityId ActivityId,
                __in ULONG IoBufferSize,
                __in ULONG MetadataBufferSize,
                __in ULONG ReservedMetadataSize
                );

            //
            // Accessors
            //
#if defined(UDRIVER) || defined(UPASSTHROUGH)
            void DisablePeriodicFlush()
            {
                _PeriodicFlushInProgress = -1;
            }
#endif

            inline const KActivityId GetActivityId()
            {
                return(_ActivityId);
            }

            inline KBuffer::SPtr GetMetadataBufferX()
            {
                return(_MetadataBufferX);
            }

            inline KIoBuffer::SPtr GetIoBufferX()
            {
                return(_IoBufferX);
            }

            inline void SetPeriodicFlushContext(
                __in_opt OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext* PeriodicFlushContext
            )
            {
                _PeriodicFlushContext = PeriodicFlushContext;
            }

            __inline KTimer* GetPeriodicTimerContext()
            {
                return(_FlushTimer.RawPtr());
            }

            __inline OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext* GetPeriodicFlushContext()
            {
                return(_PeriodicFlushContext.RawPtr());
            }

            inline void SetCloseFlushContext(
                __in_opt OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext* CloseFlushContext
            )
            {
                _CloseFlushContext = CloseFlushContext;
            }

            inline OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext* GetCloseFlushContext()
            {
                return(_CloseFlushContext.RawPtr());
            }

            inline void SetDedicatedLogStream(RvdLogStream& Stream)
            {
                _DedicatedLogStream = &Stream;

                if (_PeriodicFlushContext)
                {
                    _PeriodicFlushContext->SetDedicatedLogStream(Stream);
                }

                if (_CloseFlushContext)
                {
                    _CloseFlushContext->SetDedicatedLogStream(Stream);
                }
            }        

        public:
            class AsyncFlushingRecordsContext : public KAsyncContextBase
            {
                K_FORCE_SHARED(AsyncFlushingRecordsContext);

                friend CoalesceRecords;

            public:
                AsyncFlushingRecordsContext(
                    __in OverlayStream& OS,
                    __in OverlayStream::CoalesceRecords& CR,
                    __in RvdLogStream& DedicatedLogStream
                );

                inline ULONGLONG GetStreamOffset()
                {
                    return(_StreamOffset);
                }

                inline ULONGLONG GetHighestOperationId()
                {
                    return(_HighestOperationId);
                }

                inline ULONG GetDataSize()
                {
                    return(_DataSize);
                }

                inline ULONGLONG GetIoBufferSize()
                {
                    return(_IoBuffer->QuerySize());
                }

                inline KBuffer::SPtr GetMetadataBuffer()
                {
                    return(_MetadataBuffer);
                }

                inline KIoBuffer::SPtr GetIoBuffer()
                {
                    return(_IoBuffer);
                }

                ULONG CoalescingWriteCount()
                {
                    return(_CoalescingWrites.Count());
                }

                BOOLEAN IsCoalesceWritesEmpty()
                {
                    return(_CoalescingWrites.IsEmpty());
                }

                VOID AppendCoalescingWrite(
                    __in OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext* Context
                )
                {
                    _CoalescingWrites.AppendTail(Context);
                }

                VOID CompleteFlushingAsyncs(
                    __in NTSTATUS Status
                );

                VOID StartFlushRecords(
                    __in ULONGLONG ReserveToUse,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);


            protected:
                VOID
                    OnStart(
                    ) override;

                VOID
                    OnReuse(
                    ) override;

                VOID
                    OnCompleted(
                    ) override;

                VOID
                    OnCancel(
                    ) override;

            private:
                enum { Initial, Flushing, CompletedWithError } _State;

            VOID FSMContinue(
                __in NTSTATUS Status
                );

            VOID OperationCompletion(
                __in_opt KAsyncContextBase* const ParentAsync,
                __in KAsyncContextBase& Async
                );

            private:
                //
                // General members
                //
                OverlayStream::SPtr _OverlayStream;
                CoalesceRecords::SPtr _CoalesceRecords;
                RvdLogStream::SPtr _DedicatedLogStream;

                //
                // Parameters for API
                //
                ULONGLONG _ReserveToUse;

                //
                // Members needed for functionality (reused)
                //
                KListEntry _FlushingListEntry;
                KNodeList<AsyncAppendOrFlushContext> _CoalescingWrites;
                ULONGLONG _StreamOffset;
                ULONGLONG _HighestOperationId;
                ULONG _DataSize;
                KBuffer::SPtr _MetadataBuffer;
                KIoBuffer::SPtr _IoBuffer;
                BOOLEAN _RequestRefAcquired;
                RvdLogStream::AsyncWriteContext::SPtr _DedicatedWrite;
                LONGLONG _FlushLatencyStart;
            };

            NTSTATUS CreateAsyncFlushingRecordsContext(
                __out AsyncFlushingRecordsContext::SPtr& Context
                );

        public:

            //
            // All Asyncs must run in the same apartment to ensure that
            // the CoalesceRecords data structures are accessed in a
            // safe manner. The apartment chosen is that of the
            // OverlayStream. Additionally the asyncs are ordered so
            // they run in a first in, first out way. This is important
            // since the dedicated writes need to be in order when
            // coalescing into a larger record. The higher layers
            // ensure that records are written in order.
            //
            // Asyncs that are awaiting their turn to run in order are
            // kept on the _WaitingCoalescingRecords list. Asyncs that
            // have already been coalesced and are awaiting flush are
            // kept on the _CoalescingRecords list.
            //
            class AsyncAppendOrFlushContext : public KAsyncContextBase
            {
                K_FORCE_SHARED(AsyncAppendOrFlushContext);

                friend CoalesceRecords;

            public:
                AsyncAppendOrFlushContext(
                    __in OverlayStream& OS,
                    __in CoalesceRecords& CR,
                    __in RvdLog& DedicatedLogContainer,
                    __in RvdLogStream& DedicatedLogStream,
                    __in ThrottledKIoBufferAllocator& ThrottledAllocator
                    );

                VOID
                CompleteRequest(
                    __in NTSTATUS Status
                    );

                VOID
                StartAppend(
                    __in KBuffer& RecordMetadataBuffer,
                    __in KIoBuffer& RecordIoBuffer,
                    __in ULONGLONG ReserveToUse,
                    __in BOOLEAN ForceFlush,
                    __in KIoBuffer* CoalesceIoBuffer,
                    __in AsyncDestagingWriteContextOverlay& DestagingWriteContext,
                    // ParentAsync is always OverlayStream object to ensure proper ordering
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

                VOID
                StartFlush(
                    __in BOOLEAN IsClosing,
                    // ParentAsync is always OverlayStream object to ensure proper ordering
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

                inline BOOLEAN IsFlushContext()
                {
                    return(_AppendState == AppendState::AppendUnassigned);
                }
                
                inline BOOLEAN IsAppendContext()
                {
                    return(_AppendState == AppendState::AppendInitial);
                }

                inline VOID SetAsyncEvent()
                {
                    _AsyncEvent.SetEvent();
                }

                inline void SetDedicatedLogStream(RvdLogStream& Stream)
                {
                    _DedicatedLogStream = &Stream;
                }

                inline ULONGLONG GetStreamOffset()
                {
                    return(_LLSource.GetStreamOffset());
                }

                inline ULONGLONG GetVersion()
                {
                    return(_LLSource.GetVersion());
                }

                inline ULONG GetDataSize()
                {
                    return(_LLSource.GetDataSize());
                }

                inline KBuffer::SPtr GetMetadataBuffer()
                {
                    return(_RecordMetadataBuffer);
                }

                inline KIoBuffer::SPtr GetIoBuffer()
                {
                    return(_RecordIoBuffer);
                }

                //
                // Expose this as public so that tests can intercept
                // the async via KAsyncInterceptor
                //
                VOID
                    OnStart(
                    ) override;


            protected:
                VOID
                    OnReuse(
                    ) override;

                VOID
                    OnCompleted(
                    ) override;

                VOID
                    OnCancel(
                    ) override;

            private:
                typedef enum { AppendUnassigned, AppendInitial, AppendPerformDirectWrite, AppendWritingDirectly, AppendCopyRecordData,
                               AppendWaitForFlush, AppendFlushInProgress, AppendCompleted, AppendCompletedWithError } AppendState;
                AppendState _AppendState;

                typedef enum {
                               FlushUnassigned, FlushInitial, FlushWaitForFlush, FlushWaitForReservationRelease,
                               FlushCompleted, FlushCompletedWithError } FlushState;
                FlushState _FlushState;

                VOID AppendFSMContinue(
                    __in NTSTATUS Status
                    );

                VOID FlushFSMContinue(
                    __in NTSTATUS Status
                    );

                VOID OperationCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

                VOID WaitToRunContext(
                    );

                inline KActivityId GetActivityId()
                {
                    return(_OverlayStream->GetActivityId());
                }

            private:
                //
                // General members
                //
                CoalesceRecords::SPtr _CoalesceRecords;
                RvdLogStream::SPtr _DedicatedLogStream;
                ThrottledKIoBufferAllocator::SPtr _ThrottledAllocator;
                OverlayStream::SPtr _OverlayStream;
                RvdLog::SPtr _DedicatedLogContainer;

                //
                // Parameters to api
                //

                // Append
                KBuffer::SPtr _RecordMetadataBuffer;
                KIoBuffer::SPtr _RecordIoBuffer;
                ULONGLONG _ReserveToUse;
                BOOLEAN _ForceFlush;
                KIoBuffer::SPtr _CoalesceIoBuffer;
                KSharedPtr<AsyncDestagingWriteContextOverlay> _DestagingWriteContext;

                // TODO: Move this into a config or make it a percentage
                static const ULONG _MaxBytesPaddingWhenUnderPressure = 32;

                // Flush
                BOOLEAN _IsClosing;

                //
                // Members needed for functionality (reused)
                //
                LLRecordObject _LLSource;
                ULONGLONG _ReserveBeingUsed;
                ULONG _KIoBufferAllocationOffset;

                RvdLogStream::AsyncWriteContext::SPtr _DedicatedWrite;
                RvdLogStream::AsyncReservationContext::SPtr _DedicatedReservation;

                KListEntry _CoalescingListEntry;
                KAsyncEvent _AsyncEvent;
                KAsyncEvent::WaitContext::SPtr _WaitContext;

                BOOLEAN _RequestRefAcquired;
            };

            NTSTATUS
            CreateAsyncAppendOrFlushContext(__out AsyncAppendOrFlushContext::SPtr& Context);

            class AsyncReadContextCoalesce : public AsyncReadContext
            {
                K_FORCE_SHARED(AsyncReadContextCoalesce);

                friend CoalesceRecords;

                public:
                    AsyncReadContextCoalesce(
                        __in OverlayStream& OS,
                        __in CoalesceRecords& CR
                        );

                    VOID
                    StartRead(
                        __in RvdLogAsn RecordAsn,
                        __out_opt ULONGLONG* const Version,
                        __out KBuffer::SPtr& MetaDataBuffer,
                        __out KIoBuffer::SPtr& IoBuffer,
                        __in_opt KAsyncContextBase* const ParentAsyncContext,
                        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;

                    VOID
                    StartRead(
                        __in RvdLogAsn RecordAsn,
                        __in RvdLogStream::AsyncReadContext::ReadType Type,
                        __out_opt RvdLogAsn* const ActualRecordAsn,
                        __out_opt ULONGLONG* const Version,
                        __out KBuffer::SPtr& MetaDataBuffer,
                        __out KIoBuffer::SPtr& IoBuffer,
                        __in_opt KAsyncContextBase* const ParentAsyncContext,
                        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;

                    VOID
                    StartRead(
                        __in RvdLogAsn RecordAsn,
                        __in RvdLogStream::AsyncReadContext::ReadType Type,
                        __out_opt RvdLogAsn* const ActualRecordAsn,
                        __out_opt ULONGLONG* const Version,
                        __out KBuffer::SPtr& MetaDataBuffer,
                        __out KIoBuffer::SPtr& IoBuffer,
                        // ParentAsync is always OverlayStream object to ensure proper ordering
                        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

                protected:
                    VOID
                    OnStart(
                        ) override ;

                    VOID
                    OnReuse(
                        ) override ;

                    VOID
                    OnCompleted(
                        ) override ;

                    VOID
                    OnCancel(
                        ) override ;

                private:

                    enum { Initial, PerformRead } _State;

                    VOID FSMContinue(
                        __in NTSTATUS Status
                        );

                    VOID OperationCompletion(
                        __in_opt KAsyncContextBase* const ParentAsync,
                        __in KAsyncContextBase& Async
                        );

                private:
                    //
                    // General members
                    //
                    OverlayStream::SPtr _OverlayStream;
                    CoalesceRecords::SPtr _CoalesceRecords;
                    KAsyncEvent::WaitContext::SPtr _WaitContext;

                    //
                    // Parameters to api
                    //
                    RvdLogAsn _RecordAsn;
                    RvdLogStream::AsyncReadContext::ReadType _ReadType;
                    RvdLogAsn* _ActualRecordAsn;
                    ULONGLONG* _Version;
                    KBuffer::SPtr* _MetaDataBuffer;
                    KIoBuffer::SPtr* _IoBuffer;

                    //
                    // Members needed for functionality (reused)
                    //
                    BOOLEAN _RequestRefAcquired;
            };

            NTSTATUS
            CreateAsyncReadContext(__out AsyncReadContextCoalesce::SPtr& Context);


            //
            class AsyncFlushAllRecordsForClose : public KAsyncContextBase
            {
                K_FORCE_SHARED(AsyncFlushAllRecordsForClose);

                friend CoalesceRecords;
                
                public:
                    AsyncFlushAllRecordsForClose(
                        __in OverlayStream& OS,
                        __in CoalesceRecords& CR
                        );                                            

                protected:
                    VOID
                    OnStart(
                        ) override ;

                    VOID
                    OnReuse(
                        ) override ;

                    VOID
                    OnCompleted(
                        ) override ;

                    VOID
                    OnCancel(
                        ) override ;

                private:

                    enum { Initial, PerformFinalFlush, WaitForFlushingRecords } _State;

                    VOID FSMContinue(
                        __in NTSTATUS Status
                        );

                    VOID OperationCompletion(
                        __in_opt KAsyncContextBase* const ParentAsync,
                        __in KAsyncContextBase& Async
                        );

                private:
                    //
                    // General members
                    //
                    OverlayStream::SPtr _OverlayStream;
                    CoalesceRecords::SPtr _CoalesceRecords;
                    KAsyncEvent::WaitContext::SPtr _FlushingRecordsWait;

                    //
                    // Parameters to api
                    //

                    //
                    // Members needed for functionality (reused)
                    //
                    BOOLEAN _RequestRefAcquired;
            };

            NTSTATUS
            CreateAsyncFlushAllRecordsForClose(__out AsyncFlushAllRecordsForClose::SPtr& Context);          

            VOID FlushAllRecordsForClose(
                // ParentAsync is always OverlayStream object to ensure proper ordering
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);
            
#if defined(UDRIVER) || defined(UPASSTHROUGH)
            VOID ForcePeriodicFlush(
                // ParentAsync is always OverlayStream object to ensure proper ordering
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);
#endif
            VOID RestartPeriodicTimer();

            ULONG ComputeIoBufferNeeded(
                __in ULONG RecordDataSize
            );

            inline BOOLEAN IsCurrentlyExecuting(__in AsyncAppendOrFlushContext& Async)
            {
                return(_CurrentlyExecuting == &Async);
            }

            inline VOID SetReserveToUse(ULONGLONG ReserveToUse)
            {
                _ReserveToUse = ReserveToUse;;
            }

            inline VOID AddReserveToUse(ULONGLONG ReserveToUse)
            {
                _ReserveToUse += ReserveToUse;;
            }

            inline VOID SubtractReserveToUse(ULONGLONG ReserveToUse)
            {
                _ReserveToUse -= ReserveToUse;;
            }

            inline ULONG GetDestinationDataSize()
            {
                return(_LLDestination.GetDataSize());
            }

            inline KLogicalLogInformation::StreamBlockHeader* GetDestinationStreamBlockHeader()
            {
                return(_LLDestination.GetStreamBlockHeader());
            }

            inline VOID UpdateDestinationHeaderInformation(
                __in ULONGLONG StreamOffset,
                __in ULONGLONG Version,
                __in ULONGLONG HeadTruncationPoint
                )
            {
                _LLDestination.UpdateHeaderInformation(StreamOffset, Version, HeadTruncationPoint);
            }

            inline VOID ClearDestination()
            {
                _LLDestination.Clear();
            }

            inline NTSTATUS ExtendDestinationIoBuffer(
                __in KIoBuffer& IoBuffer,
                __in ULONG IoBufferOffset,
                __in ULONG SizeToExtend
                )
            {
                NTSTATUS status;

                status = _LLDestination.ExtendIoBuffer(IoBuffer,
                                                       IoBufferOffset,
                                                       SizeToExtend);
                return(status);
            }

            inline BOOLEAN DestinationCopyFrom(
                __inout LLRecordObject& Source
                )
            {
                BOOLEAN b;

                b = _LLDestination.CopyFrom(Source);

                return(b);
            }

            inline ULONG GetDestinationFullRecordSpaceLeft()
            {
                return(_MaxCoreLoggerRecordSize - _IoBufferX->QuerySize());
            }

            inline VOID SetRecordMarkerPlusOne(
                __in ULONG Marker
                )
            {
                _LLDestination.SetRecordMarkerPlusOne(Marker);
            }

            inline ULONGLONG GetDestinationStreamOffset()
            {
                return(_LLDestination.GetStreamOffset());
            }

            inline ULONG GetSpaceLeftInDestination()
            {
                return(_LLDestination.GetSpaceLeftInDestination());
            }

            inline ULONG GetIoBufferOffsetInDestination()
            {
                return(_LLDestination.GetIoBufferOffsetInDestination());
            }

            inline AsyncFlushingRecordsContext::SPtr GetFlushingRecordsContext()
            {
                return(_FlushingRecordsContext);
            }

            inline VOID ResetFlushingRecordsContext()
            {
                _FlushingRecordsContext = nullptr;
            }

            inline ULONGLONG GetReserveToUse()
            {
                return(_ReserveToUse);
            }

            inline ULONG GetWaitingCoalescingWritesCount()
            {
                return(_WaitingCoalescingWrites.Count());
            }

            inline BOOLEAN IsClosing()
            {
                return(_IsClosing);
            }

            inline VOID SetIsClosing()
            {
                _IsClosing = TRUE;
            }

            inline VOID CancelFlushTimer()
            {
                _FlushTimer->Cancel();
            }

            inline VOID StartFinalFlush(
                // ParentAsync is always OverlayStream object to ensure proper ordering
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
            {
                _CloseFlushContext->StartFlush(_IsClosing, CallbackPtr);                
            }

            inline BOOLEAN IsFlushingRecordsEmpty()
            {
                return(_FlushingRecords.IsEmpty());
            }

            inline ULONG FlushingRecordsCount()
            {
                return(_FlushingRecords.Count());
            }

            inline NTSTATUS CreateFlushingRecordsWaitContext(
                __out KAsyncEvent::WaitContext::SPtr& Wait
                )
            {
                NTSTATUS status;
                
                status = _FlushingRecordsFlushed.CreateWaitContext(GetThisAllocationTag(),
                                                                   GetThisAllocator(),
                                                                   Wait);
                return(status);
            }
            
            void InsertCurrentFlushingRecordsContext();

            void RemoveFlushingRecordsContext(
                __in NTSTATUS Status,
                __in AsyncFlushingRecordsContext& FlushingRecordsContext
                        );

            void RemoveAllFailedFlushingRecordsContext();

            NTSTATUS FindDataInFlushingRecords(
                __inout RvdLogAsn& RecordAsn,
                __in RvdLogStream::AsyncReadContext::ReadType Type,
                __out ULONGLONG& Version,
                __out KBuffer::SPtr& MetaDataBuffer,
                __out KIoBuffer::SPtr& IoBuffer
            );
            
            VOID StartPeriodicFlush();

        private:
            //
            // Internal routines
            //
            NTSTATUS Initialize(
                );

            NTSTATUS PrepareRecordForWrite();

            VOID Clear();

            VOID AddWriteToWaitingList(
                __in OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext& AppendContext
            );

            VOID RemoveWriteFromWaitingList(
                __in OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext& AppendContext
                );

            VOID AddWriteToIndependentList(
                __in OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext& AppendContext
            );

            VOID RemoveWriteFromIndependentList(
                __in OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext& AppendContext
                );

            VOID StartNextWrite(
                );

            VOID CompleteFlushingAsyncs(
                __in NTSTATUS Status
                );

            VOID PeriodicTimerCompletion(
                __in_opt KAsyncContextBase* const ParentAsync,
                __in KAsyncContextBase& Async
                );

            VOID PeriodicFlushCompletion(
                __in_opt KAsyncContextBase* const ParentAsync,
                __in KAsyncContextBase& Async
                );


        private:
            OverlayStream::SPtr _OverlayStream;
            RvdLog::SPtr _DedicatedLogContainer;
            RvdLogStream::SPtr  _DedicatedLogStream;
            ThrottledKIoBufferAllocator::SPtr _ThrottledAllocator;
            KActivityId _ActivityId;

            ULONG _PeriodicFlushCounter;
            BOOLEAN _IsFlushTimerActive;
            BOOLEAN _RestartPeriodicFlushTimer;

            KTimer::SPtr _FlushTimer;
            volatile LONG _PeriodicFlushInProgress;
            volatile LONG _PeriodicFlushOnCompletion;
            OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext::SPtr _PeriodicFlushContext;
            OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext::SPtr _CloseFlushContext;

            KSpinLock _ListLock;
            OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext* _CurrentlyExecuting;
            OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext* _LastExecuting;
            static const ULONG _CoalescingLinkageOffset;
            KNodeList<AsyncAppendOrFlushContext> _WaitingCoalescingWrites;
            BOOLEAN _IsClosing;

            AsyncFlushingRecordsContext::SPtr _FlushingRecordsContext;
            AsyncFlushAllRecordsForClose::SPtr _FlushAllRecordsForClose;

            ULONG _MaxCoreLoggerRecordSize;

            KAsyncEvent _AlwaysSetEvent;
            KSpinLock _FRListLock;
            static const ULONG _FlushingRecordsLinkageOffset;
            ULONG _FailedFlushingRecordCount;
            KNodeList<AsyncFlushingRecordsContext> _FlushingRecords;
            KAsyncEvent _FlushingRecordsFlushed;
            KNodeList<AsyncAppendOrFlushContext> _IndependentWrites;

            //
            // Currently coalescing record
            //
            LLRecordObject _LLDestination;
            KBuffer::SPtr _MetadataBufferX;
            KIoBuffer::SPtr _IoBufferX;
            ULONG _ReservedMetadataSize;
            ULONG _MetadataBufferSize;
            ULONG _OffsetToDataLocation;
            ULONGLONG _ReserveToUse;
        };

        NTSTATUS CreateCoalesceRecords(
            __out CoalesceRecords::SPtr& Context,
            __in ULONG IoBufferSize,
            __in ULONG MetadataBufferSize,
            __in ULONG ReservedMetadataSize
            );

    public:
        inline CoalesceRecords::SPtr GetCoalesceRecords() { return _CoalesceRecords; };


        class AsyncWriteContextOverlay;
    public:
        //
        // AsyncDestagingWriteContextOverlay is the class that provides
        // the destaging implementation for streams
        //
        class AsyncDestagingWriteContextOverlay : public KAsyncContextBase
        {
            K_FORCE_SHARED(AsyncDestagingWriteContextOverlay);

            friend OverlayStream;
            friend OverlayLog;

            public:
                static NTSTATUS Create(
                    __out AsyncDestagingWriteContextOverlay::SPtr& Context,
                    __in KAllocator& Allocator,
                    __in ULONG AllocationTag,
                    __in KActivityId ActivityId
                    );

                AsyncDestagingWriteContextOverlay(
                    __in OverlayStream& OStream,
                    __in ThrottledKIoBufferAllocator& ThrottledAllocator,
                    __in RvdLogStream& SharedLogStream,
                    __in RvdLog& DedicatedLogContainer,
                    __in RvdLogStream& DedicatedLogStream
                    );

                static LONG
                Comparator(AsyncDestagingWriteContextOverlay& Left, AsyncDestagingWriteContextOverlay& Right) {
                    // -1 if Left < Right
                    // +1 if Left > Right
                    //  0 if Left == Right

                    //
                    // First order key is the ASN and second order key is version
                    //
                    if (Left._RecordAsn < Right._RecordAsn)
                    {
                        return(-1);
                    } else if (Left._RecordAsn > Right._RecordAsn) {
                        return(+1);
                    } else if (Left._Version < Right._Version) {
                        return(-1);
                    } else if (Left._Version > Right._Version) {
                        return(+1);
                    }

                    return(0);
                };

                //
                // Accessors
                //
                inline const KActivityId GetActivityId()
                {
                    return(_OverlayStream->GetActivityId());
                }

                VOID SetOrResetTruncateOnCompletion(__in BOOLEAN DoTruncate)
                {
                    _TruncateOnDedicatedCompletion = DoTruncate;
                }
                                
                static inline ULONG
                GetDedicatedLinksOffset() { return FIELD_OFFSET(AsyncDestagingWriteContextOverlay, _DedicatedTableEntry); };

                static inline ULONG
                GetSharedLinksOffset() { return FIELD_OFFSET(AsyncDestagingWriteContextOverlay, _SharedTableEntry); };

                inline RvdLogAsn GetRecordAsn()
                {
                    return(_RecordAsn);
                }

                inline void SetRecordAsn(__in RvdLogAsn RecordAsn)
                {
                    _RecordAsn = RecordAsn;
                }

                void SetDedicatedCoalescedEvent()
                {
                    _DedicatedCoalescedEvent.SetEvent();
                }

                VOID
                StartReservedWrite(
                    __in ULONGLONG ReserveToUse,
                    __in RvdLogAsn RecordAsn,
                    __in ULONGLONG Version,
                    __in const KBuffer::SPtr& MetaDataBuffer,
                    __in const KIoBuffer::SPtr& IoBuffer,
                    __in BOOLEAN ForceFlush,
                    __in AsyncWriteContextOverlay& ParentAsyncContext
                );

            protected:
                VOID
                OnStart(
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

                VOID
                OnCompleted(
                    ) override ;

                VOID
                OnCancel(
                    ) override ;

            private:
                VOID OnStartContinue();

                VOID OnStartAllocateBuffer();

                VOID DedicatedWriteCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

                VOID DedicatedFlushCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

                VOID SharedWriteCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

                VOID SharedWriteCompletion2(
                    __in NTSTATUS Status
                    );

                VOID SharedWriteCompletionAfterEventSet(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

                VOID ProcessWriteCompletion(
                    __in BOOLEAN SharedWriteCompleted,
                    __in NTSTATUS Status
                    );

                VOID
                ReservationCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

                VOID
                AllocateKIoBufferCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

                VOID
                ReservationCompletionWorker(
                    __in NTSTATUS Status
                    );

                VOID
                DedicatedTimerWaitCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

                VOID
                SharedTimerWaitCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

                VOID
                SharedLogFullTimerWaitCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );
                
                BOOLEAN
                ShouldSendToSharedStream(
                    );

                ULONGLONG
                NumberBytesForWrite()
                {
                    return( (_SharedIoBuffer ? _SharedIoBuffer->QuerySize() : 0) +
                            (_MetaDataBuffer ? _MetaDataBuffer->QuerySize() : 0) );
                }

                ULONG GetDataSize()
                {
                    return(_DataSize);
                }

            private:
                //
                // General members
                //
                OverlayStream::SPtr _OverlayStream;
                ThrottledKIoBufferAllocator::SPtr _ThrottledAllocator;
                CoalesceRecords::SPtr _CoalesceRecords;
                RvdLogStream::SPtr _SharedLogStream;
                RvdLog::SPtr _DedicatedLogContainer;
                RvdLogStream::SPtr _DedicatedLogStream;
                KSpinLock _ThisLock;
                KInstrumentedOperation _ContainerOperation;

                //
                // Parameters to api
                //
                ULONGLONG _ReserveToUse;
                RvdLogAsn _RecordAsn;
                ULONGLONG _Version;
                KBuffer::SPtr _MetaDataBuffer;
                KIoBuffer::SPtr _SharedIoBuffer;
                KIoBuffer::SPtr _DedicatedIoBuffer;
                BOOLEAN _ForceFlush;

                //
                // Members needed for functionality (reused)
                //
                RvdLogStream::AsyncWriteContext::SPtr _DedicatedWrite;
                RvdLogStream::AsyncWriteContext::SPtr _SharedWrite;
                LONG _CompletedToCaller;
                LONG _AsyncsOutstanding;
                NTSTATUS _DedicatedCompletionStatus;
                KSharedPtr<AsyncWriteContextOverlay> _CallerAsync;
                ULONGLONG _SharedWriteLatencyTimeStart;
                KTimer::SPtr _SharedLogFullTimer;
                static const ULONG _SharedLogFullRetryTimerInMs = 3 * 1000;
                ULONG _SharedLogFullRetries;
                
                KTableEntry _DedicatedTableEntry;
                KTableEntry _SharedTableEntry;

                KtlLogAsn _LogicalLogTailOffset;
                BOOLEAN _RequestRefAcquired;
                RvdLogStream::AsyncReservationContext::SPtr _DedicatedReservation;
                BOOLEAN _IsTruncateTailWrite;
                BOOLEAN _IsMarkedEndOfRecord;
                ULONGLONG _TotalNumberBytesToWrite;
                ULONG _IoBufferSize;

                KListEntry _ThrottleListEntry;
                ThrottledKIoBufferAllocator::AsyncAllocateKIoBufferContext::SPtr _AllocateKIoBuffer;
                KIoBuffer::SPtr _CoalesceIoBuffer;

#if DBG
                KTimer::SPtr _SharedTimer;
                KTimer::SPtr _DedicatedTimer;
#endif
                BOOLEAN _SendToShared;
                OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext::SPtr _AppendFlushContext;

                ULONG _DataSize;

                RvdLogAsn _TruncationPendingAsn;

                NTSTATUS _SharedCompletionStatus;
                KAsyncEvent _DedicatedCoalescedEvent;
                KAsyncEvent::WaitContext::SPtr _DedicatedCoalescedWaitContext;
                BOOLEAN _TruncateOnDedicatedCompletion;
        };

        NTSTATUS
        CreateAsyncDestagingWriteContext(__out AsyncDestagingWriteContextOverlay::SPtr& Context);

        NTSTATUS
        CreateAsyncDestagingWriteContextObject(__out AsyncDestagingWriteContextOverlay::SPtr& Context);

        VOID UnthrottleWritesIfPossible(
            __in BOOLEAN ReleaseOne
            );

        BOOLEAN ThrottleWriteIfNeeded(
            __in OverlayStream::AsyncDestagingWriteContextOverlay& DestagingWriteContext
            );

        NTSTATUS
        AddDestagingWrite(
            __in KNodeTable<AsyncDestagingWriteContextOverlay>* WriteTable,
            __in AsyncDestagingWriteContextOverlay& DestagingWrite
        );

        NTSTATUS
        RemoveDestagingWrite(
            __in KNodeTable<AsyncDestagingWriteContextOverlay>* WriteTable,
            __in AsyncDestagingWriteContextOverlay& DestagingWrite
        );

        RvdLogAsn
        SetTruncationPending(
            __in KNodeTable<AsyncDestagingWriteContextOverlay>& WriteTable,
            __in RvdLogAsn TruncationPendingAsn
        );

        RvdLogAsn
        GetLowestDestagingWriteAsn(
            __in KNodeTable<AsyncDestagingWriteContextOverlay>& WriteTable
        );

        BOOLEAN
        IsAsnInDestagingList(
            __in KNodeTable<AsyncDestagingWriteContextOverlay>& WriteTable,
            __in RvdLogAsn Asn
            );


        VOID FreeKIoBuffer(
            __in KActivityId ActivityId,
            __in ULONG Size);

        inline KNodeTable<AsyncDestagingWriteContextOverlay>* GetOutstandingDedicatedWriteTable()
        {
            return(&_OutstandingDedicatedWriteTable);
        }

        inline KNodeTable<AsyncDestagingWriteContextOverlay>* GetOutstandingSharedWriteTable()
        {
            return(&_OutstandingSharedWriteTable);
        }

    public:     
        class AsyncWriteContextOverlay : public AsyncWriteContext
        {
            K_FORCE_SHARED(AsyncWriteContextOverlay);

            public:
                AsyncWriteContextOverlay(
                    __in OverlayStream& OStream
                    );

                VOID
                StartWrite(
                    __in BOOLEAN LowPriorityIO,
                    __in RvdLogAsn RecordAsn,
                    __in ULONGLONG Version,
                    __in const KBuffer::SPtr& MetaDataBuffer,
                    __in const KIoBuffer::SPtr& IoBuffer,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override;

                VOID
                StartWrite(
                    __in RvdLogAsn RecordAsn,
                    __in ULONGLONG Version,
                    __in const KBuffer::SPtr& MetaDataBuffer,
                    __in const KIoBuffer::SPtr& IoBuffer,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override;

                VOID
                StartReservedWrite(
                    __in ULONGLONG ReserveToUse,
                    __in RvdLogAsn RecordAsn,
                    __in ULONGLONG Version,
                    __in const KBuffer::SPtr& MetaDataBuffer,
                    __in const KIoBuffer::SPtr& IoBuffer,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override;

                VOID
                StartReservedWrite(
                    __in ULONGLONG ReserveToUse,
                    __in RvdLogAsn RecordAsn,
                    __in ULONGLONG Version,
                    __in const KBuffer::SPtr& MetaDataBuffer,
                    __in const KIoBuffer::SPtr& IoBuffer,
                    __out ULONGLONG& LogSize,
                    __out ULONGLONG& LogSpaceRemaining,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override;

                VOID
                StartReservedWrite(
                    __in BOOLEAN LowPriorityIO,
                    __in ULONGLONG ReserveToUse,
                    __in RvdLogAsn RecordAsn,
                    __in ULONGLONG Version,
                    __in const KBuffer::SPtr& MetaDataBuffer,
                    __in const KIoBuffer::SPtr& IoBuffer,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override;

                VOID
                StartReservedWrite(
                    __in ULONGLONG ReserveToUse,
                    __in RvdLogAsn RecordAsn,
                    __in ULONGLONG Version,
                    __in const KBuffer::SPtr& MetaDataBuffer,
                    __in const KIoBuffer::SPtr& IoBuffer,
                    __in BOOLEAN ForceFlush,
                    __out_opt BOOLEAN * const SendToShared,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

                VOID
                CompleteRequest(
                    __in NTSTATUS Status
                );

                inline void SetSendToShared(
                    __in BOOLEAN SendToShared
                )
                {
                    if (_SendToShared)
                    {
                        *_SendToShared = SendToShared;
                    }
                }

            protected:
                VOID
                OnStart(
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

                VOID
                OnCompleted(
                    ) override ;

                VOID
                OnCancel(
                    ) override ;

            private:
                enum { Initial, WriteToDestaging, Completed, CompletedWithError } _State;

                VOID FSMContinue(
                    __in NTSTATUS Status
                    );

                VOID OperationCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

            private:
                //
                // General members
                //
                OverlayStream::SPtr _OverlayStream;

                //
                // Parameters to api
                //
                ULONGLONG _ReserveToUse;
                RvdLogAsn _RecordAsn;
                ULONGLONG _Version;
                KBuffer::SPtr _MetaDataBuffer;
                KIoBuffer::SPtr _IoBuffer;
                BOOLEAN* _SendToShared;
                BOOLEAN _ForceFlush;
                ULONGLONG* _LogSize;
                ULONGLONG* _LogSpaceRemaining;

                //
                // Members needed for functionality (reused)
                //
                OverlayStream::AsyncDestagingWriteContextOverlay::SPtr _DestagingWrite;
                BOOLEAN _RequestRefAcquired;
        };

        NTSTATUS
        CreateAsyncWriteContext(__out AsyncWriteContext::SPtr& Context) override ;

        class AsyncReadContextOverlay : public AsyncReadContext
        {
            K_FORCE_SHARED(AsyncReadContextOverlay);

            public:
                AsyncReadContextOverlay(
                    __in OverlayStream& OStream,
                    __in_opt OverlayStream::CoalesceRecords* CoalesceRecords,
                    __in RvdLogStream& SharedLogStream,
                    __in RvdLog& DedicatedLogContainer,
                    __in RvdLogStream& DedicatedLogStream
                    );

                VOID
                StartRead(
                    __in RvdLogAsn RecordAsn,
                    __out_opt ULONGLONG* const Version,
                    __out KBuffer::SPtr& MetaDataBuffer,
                    __out KIoBuffer::SPtr& IoBuffer,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;

                VOID
                StartRead(
                    __in RvdLogAsn RecordAsn,
                    __in RvdLogStream::AsyncReadContext::ReadType Type,
                    __out_opt RvdLogAsn* const ActualRecordAsn,
                    __out_opt ULONGLONG* const Version,
                    __out KBuffer::SPtr& MetaDataBuffer,
                    __out KIoBuffer::SPtr& IoBuffer,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;

                VOID
                StartRead(
                    __in RvdLogAsn RecordAsn,
                    __in RvdLogStream::AsyncReadContext::ReadType Type,
                    __out_opt RvdLogAsn* const ActualRecordAsn,
                    __out_opt ULONGLONG* const Version,
                    __out KBuffer::SPtr& MetaDataBuffer,
                    __out KIoBuffer::SPtr& IoBuffer,
                    __out_opt BOOLEAN* const ReadFromDedicated,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

            protected:
                VOID
                OnStart(
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

                VOID
                OnCompleted(
                    ) override ;

                VOID
                OnCancel(
                    ) override ;

            private:
                NTSTATUS
                DetermineRecordLocation(
                    __out BOOLEAN& ReadFromDedicated,
                    __out RvdLogAsn& AsnToRead,
                    __out RvdLogStream::AsyncReadContext::ReadType& ReadType
                    );

                NTSTATUS TrimLogicalLogRecord(
                    __out ULONGLONG& RecordStreamOffset,
                    __out ULONG& RecordFullDataSize,
                    __out ULONG& RecordTrimmedDataSize
                    );

                enum { Initial, ReadFromShared, ReadFromDedicated,
                       InitialCoalesce, ReadFromCoalesceBuffersExactContaining, ReadFromCoalesceDedicatedExactContaining,
                       VerifyRecord } _State;

                VOID FSMContinue(
                    __in NTSTATUS Status
                    );

                VOID OperationCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

            private:
                //
                // General members
                //
                OverlayStream::SPtr _OverlayStream;
                OverlayStream::CoalesceRecords::SPtr _CoalesceRecords;
                RvdLogStream::SPtr  _SharedLogStream;
                RvdLog::SPtr _DedicatedLogContainer;
                RvdLogStream::SPtr  _DedicatedLogStream;

                //
                // Parameters to api
                //
                RvdLogAsn _RecordAsn;
                RvdLogStream::AsyncReadContext::ReadType _ReadType;
                RvdLogAsn* _ActualRecordAsnPtr;
                ULONGLONG* _VersionPtr;
                RvdLogAsn _ActualRecordAsnLocal;
                ULONGLONG _VersionLocal;
                KBuffer::SPtr* _MetaDataBuffer;
                KIoBuffer::SPtr* _IoBuffer;
                BOOLEAN* _ReadFromDedicated;

                //
                // Members needed for functionality (reused)
                //
                RvdLogAsn _AsnToRead;
                RvdLogStream::AsyncReadContext::SPtr _DedicatedRead;
                RvdLogStream::AsyncReadContext::SPtr _SharedRead;
                ULONGLONG _LogicalLogTailOffset;
                BOOLEAN _RequestRefAcquired;

                CoalesceRecords::AsyncReadContextCoalesce::SPtr _CoalesceRead;

                //
                // Number of times a read will be retried in case a
                // race condition causes a read error
                //
                const static ULONG _TryAgainRetries = 3;
                ULONG _TryAgainCounter;
        };

        NTSTATUS
        CreateAsyncReadContext(__out AsyncReadContext::SPtr& Context) override ;


        class AsyncMultiRecordReadContextOverlay : public KAsyncContextBase
        {
            K_FORCE_SHARED(AsyncMultiRecordReadContextOverlay);

        public:
            AsyncMultiRecordReadContextOverlay(
                __in OverlayStream& OStream,
                __in RvdLog& DedicatedLogContainer
                );

            VOID
                StartRead(
                __in RvdLogAsn RecordAsn,
                __inout KIoBuffer& MetaDataBuffer,
                __inout KIoBuffer& IoBuffer,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

        protected:
            VOID
                OnStart(
                ) override;

            VOID
                OnReuse(
                ) override;

            VOID
                OnCompleted(
                ) override;

            VOID
                OnCancel(
                ) override;

        private:
            enum { Initial, ReadFirstRecord, ReadSubsequentRecords, Completed } _State;

            VOID FSMContinue(
                __in NTSTATUS Status
                );

            VOID OperationCompletion(
                __in_opt KAsyncContextBase* const ParentAsync,
                __in KAsyncContextBase& Async
                );

        NTSTATUS
        VerifyRecordAndGatherStreamBlock(
            __in KIoBuffer& IoBuffer,
            __in KBuffer& MetadataBuffer,
            __out KLogicalLogInformation::MetadataBlockHeader*& RecordMetadataHeader,
            __out KLogicalLogInformation::StreamBlockHeader*& RecordStreamBlockHeader,
            __out ULONG& DataSizeInRecord,
            __out ULONG& DataOffsetInRecordMetadata
            );

        private:
            //
            // General members
            //
            static const ULONG _MinimumFirstReadCompleteSize = ((128 * 1024) - 0x110);   // TODO: Pick a number
            OverlayStream::SPtr _OverlayStream;
            RvdLog::SPtr _DedicatedLogContainer;

            //
            // Parameters to api
            //
            RvdLogAsn _RecordAsn;
            KIoBuffer::SPtr _MetadataBuffer;
            KIoBuffer::SPtr _IoBuffer;

            //
            // Members needed for functionality (reused)
            //
            KLogicalLogInformation::StreamBlockHeader* _StreamBlockHeader;
            PUCHAR _MetadataBufferPtr;
            ULONG _MetadataBufferSizeLeft;
            ULONG _OffsetToDataLocation;
            PUCHAR _IoBufferPtr;
            ULONG _IoBufferSizeLeft;
            ULONG _TotalSize;

            OverlayStreamBase::AsyncReadContext::SPtr _UnderlyingRead;
            KBuffer::SPtr _RecordMetadataBuffer;
            KIoBuffer::SPtr _RecordIoBuffer;
            RvdLogAsn _ActualRecordAsn;
            ULONGLONG _ActualRecordVersion;
            RvdLogAsn _NextRecordAsn;

            BOOLEAN _RequestRefAcquired;
        };

        NTSTATUS
        CreateAsyncMultiRecordReadContextOverlay(__out AsyncMultiRecordReadContextOverlay::SPtr& Context);

        NTSTATUS
        QueryRecordAndVerifyCompleted(
            __in BOOLEAN ForSharedLog,
            __inout RvdLogAsn& RecordAsn,
            __in RvdLogStream::AsyncReadContext::ReadType Type,
            __out_opt ULONGLONG* const Version,
            __out_opt RvdLogStream::RecordDisposition* const Disposition,
            __out_opt ULONG* const IoBufferSize,
            __out_opt ULONGLONG* const DebugInfo1);

        NTSTATUS
        QueryRecord(
            __in RvdLogAsn RecordAsn,
            __out_opt ULONGLONG* const Version,
            __out_opt RvdLogStream::RecordDisposition* const Disposition,
            __out_opt ULONG* const IoBufferSize = nullptr,
            __out_opt ULONGLONG* const DebugInfo1 = nullptr) override;

        NTSTATUS
        QueryRecord(
            __inout RvdLogAsn& RecordAsn,
            __in RvdLogStream::AsyncReadContext::ReadType Type,
            __out_opt ULONGLONG* const Version,
            __out_opt RvdLogStream::RecordDisposition* const Disposition,
            __out_opt ULONG* const IoBufferSize = nullptr,
            __out_opt ULONGLONG* const DebugInfo1 = nullptr) override;

        NTSTATUS
        QueryRecords(
            __in RvdLogAsn LowestAsn,
            __in RvdLogAsn HighestAsn,
            __in KArray<RvdLogStream::RecordMetadata>& ResultsArray) override ;

        NTSTATUS
        DeleteRecord(
            __in RvdLogAsn RecordAsn,
            __in ULONGLONG Version) override;

        VOID
        TruncateSharedStreamIfPossible(
            __in RvdLogAsn DesiredTruncationPoint,
            __in ULONGLONG Version
            );

        VOID
        Truncate(__in RvdLogAsn TruncationPoint, __in RvdLogAsn PreferredTruncationPoint) override;

        VOID
        TruncateBelowVersion(__in RvdLogAsn TruncationPoint, __in ULONGLONG Version) override;

        class AsyncReservationContextOverlay : public AsyncReservationContext
        {
            K_FORCE_SHARED(AsyncReservationContextOverlay);

            public:
                AsyncReservationContextOverlay(
                    __in OverlayStream& OStream
                    );

                VOID
                StartUpdateReservation(
                    __in LONGLONG Delta,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;

            protected:
                VOID
                OnStart(
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

                VOID
                OnCompleted(
                    ) override ;

                VOID
                OnCancel(
                    ) override ;

            private:
                enum { Initial, ReserveFromDedicated, Completed, CompletedWithError } _State;

                VOID FSMContinue(
                    __in NTSTATUS Status
                    );

                VOID OperationCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

            private:
                //
                // General members
                //
                OverlayStream::SPtr _OverlayStream;

                //
                // Parameters to api
                //
                LONGLONG _Delta;

                //
                // Members needed for functionality (reused)
                //
                RvdLogStream::AsyncReservationContext::SPtr _DedicatedReservation;
                BOOLEAN _RequestRefAcquired;
        };

        NTSTATUS
        CreateUpdateReservationContext(__out AsyncReservationContext::SPtr& Context) override ;

    private:
        //
        // Configuration
        //
        OverlayGlobalContext::SPtr _GlobalContext;

        RvdLogManager::SPtr _BaseLogManager;
        RvdLog::SPtr _BaseSharedLog;

        KGuid _DiskId;
        KString::CSPtr _Path;
        ULONG _MaxRecordSize;
        LONGLONG _StreamSize;
        ULONG _StreamMetadataSize;

        ULONG _PeriodicFlushTimeInSec;
        ULONG _PeriodicTimerIntervalInSec;

#if defined(UDRIVER) || defined(UPASSTHROUGH)
        ULONG _PeriodicTimerTestDelayInSec;
#endif

        BOOLEAN _IsStreamForLogicalLog;
        KtlLogAsn _LogicalLogTailOffset;
        ULONGLONG _LogicalLogLastVersion;

        RvdLogStreamType _LogStreamType;
        KtlLogStreamId _StreamId;
        KtlLogContainerId _DedicatedContainerId;

        KSharedPtr<OverlayLog> _OverlayLog;

        ThrottledKIoBufferAllocator::SPtr _ThrottledAllocator;

        //
        // This is a list of KtlLogStreamKernel objects exposed for
        // this stream
        //
        KNodeListShared<KtlLogStreamKernel> _StreamUsers;

        KSpinLock _StreamUsersLock;

        //
        // Pointers to underlying core logger objects
        //
        RvdLogStream::SPtr _SharedLogStream;

        RvdLog::SPtr _DedicatedLogContainer;
        RvdLogStream::SPtr _DedicatedLogStream;

        KAsyncEvent _DedicatedLogShutdownEvent;
        KAsyncEvent::WaitContext::SPtr _DedicatedLogShutdownWait;

        //
        // Performance counter
        //
        static const ULONG _LatencyOperationSampleCount;
        volatile LONGLONG _SharedWriteLatencyTime;
        volatile LONG _SharedWriteLatencyTimeIntervals;
        LONGLONG _SharedWriteLatencyTimeAverage;
        volatile LONGLONG _DedicatedWriteLatencyTime;
        volatile LONG _DedicatedWriteLatencyTimeIntervals;
        LONGLONG _DedicatedWriteLatencyTimeAverage;
                
        LONGLONG* _ContainerDedicatedBytesWritten;
        LONGLONG* _ContainerSharedBytesWritten;
        volatile LONGLONG _ApplicationBytesWritten;
        volatile LONGLONG _SharedBytesWritten;
        volatile LONGLONG _DedicatedBytesWritten;       
#if !defined(PLATFORM_UNIX)
        KPerfCounterSetInstance::SPtr _PerfCounterSetInstance;
#endif
        
        //
        // Write processor values
        //
        BOOLEAN _IsNextRecordFirstWrite;
#if DBG
        ULONG _SharedTimerDelay;
        ULONG _DedicatedTimerDelay;
#endif
        ULONG _SharedMaximumRecordSize;
        KtlLogAsn _SharedTruncationAsn;
        ULONGLONG _SharedTruncationVersion;

        NTSTATUS _FailureStatus;
        BOOLEAN _WriteOnlyToDedicated;
        BOOLEAN _DisableCoalescing;

        NTSTATUS _LastSharedLogStatus;

        CoalesceRecords::SPtr _CoalesceRecords;

        KSpinLock _SharedTruncationValuesLock;

        KSpinLock _Lock;
        KNodeTable<AsyncDestagingWriteContextOverlay> _OutstandingDedicatedWriteTable;
        KNodeTable<AsyncDestagingWriteContextOverlay> _OutstandingSharedWriteTable;
        AsyncDestagingWriteContextOverlay::SPtr _WriteTableLookupKey;


        // Throttling
        LONG _StreamAllocation;
        LONGLONG _SharedLogQuota;
        volatile LONGLONG _SharedWriteBytesOutstanding;

        volatile LONGLONG _DedicatedWriteBytesOutstanding;
        LONGLONG _DedicatedWriteBytesOutstandingThreshold;   // TODO: Make configurable
        
        
        KNodeList<AsyncDestagingWriteContextOverlay> _ThrottledWritesList;
        KSpinLock _ThrottleWriteLock;
        ULONG _ThrottledAllocatorBytes;

        KAsyncEvent _SharedTruncationEvent;
        
        //
        // Open work
        //
        RvdLogManager::AsyncOpenLog::SPtr _OpenDedicatedLog;
        RvdLog::AsyncOpenLogStreamContext::SPtr _OpenStream;
        RvdLog::AsyncDeleteLogStreamContext::SPtr _DeleteStream;
        AsyncCopySharedToDedicatedContext::SPtr _CopySharedToDedicated;
        AsyncCopySharedToBackupContext::SPtr _CopySharedToBackup;
        NTSTATUS _FinalStatus;

        RvdLogStream::AsyncReadContext::SPtr _TailReadContext;
        KBuffer::SPtr _TailMetadataBuffer;
        KIoBuffer::SPtr _TailIoBuffer;
        
        RvdLogStream::AsyncReadContext::SPtr _LastReadContext;
        KBuffer::SPtr _LastMetadataBuffer;
        KIoBuffer::SPtr _LastIoBuffer;
        ULONGLONG _LastVersion;
        RvdLogAsn _LastDedicatedAsn;
        RvdLogAsn _FirstSharedAsn;
        
        KLogicalLogInformation::StreamBlockHeader _RecoveredLogicalLogHeader;
        BOOLEAN _GateAcquired;
        KSharedPtr<LogStreamOpenGateContext> _OpenGateContext;
        
    public:
        static const ULONG _ThrottleLinkageOffset;      
};

class OverlayLog;
class OverlayLogFreeService;

class OverlayStreamFreeService : public ServiceWrapper
{
    K_FORCE_SHARED(OverlayStreamFreeService);

    public:
        OverlayStreamFreeService(
            __in KtlLogStreamId& StreamId,
            __in KGuid DiskId,
            __in_opt KString const * const Path,
            __in ULONG MaxRecordSize,
            __in LONGLONG StreamSize,
            __in ULONG StreamMetadataSize
            );

        BOOLEAN IsOpened()
        {
            return(GetObjectState() == ServiceWrapper::Opened);
        }

        static LONG CompareRoutine(
            __in KtlLogStreamId Left,
            __in KtlLogStreamId Right
            )
        {
            GUID leftGuid = Left.Get();
            GUID rightGuid = Right.Get();
            ULONG64 leftHi = *((PULONG64)&leftGuid);
            ULONG64 leftLo = *((PULONG64)((PUCHAR)&leftGuid + 8));
            ULONG64 rightHi = *((PULONG64)&rightGuid);
            ULONG64 rightLo = *((PULONG64)((PUCHAR)&rightGuid + 8));

            // -1 if Left < Right
            // +1 if Left > Right
            //  0 if Left == Right
            if (leftHi < rightHi)
            {
                return(-1);
            } else if (leftHi > rightHi) {
                return(+1);
            } else {
                if (leftLo < rightLo)
                {
                    return(-1);
                } else if (leftLo > rightLo) {
                    return(+1);
                }
            }
            return(0);
        }

        //
        // These manage the ref count for the number of times users are
        // referencing this while on the global list of streams.
        //
        VOID
        IncrementOnListCount(
            );

        BOOLEAN
        DecrementOnListCount(
            );


        //
        // Accessors
        //
        inline KtlLogStreamId GetStreamId()
        {
            return(_StreamId);
        }

        inline KString::CSPtr GetPath()
        {
            return(_Path);
        }

        inline KGuid GetDiskId()
        {
            return(_DiskId);
        }

        inline OverlayStream::SPtr GetOverlayStream()
        {
            OverlayStream::SPtr os = Ktl::Move(down_cast<OverlayStream>(GetUnderlyingService()));
            return(os);
        }

        inline VOID SetLCMBEntryIndex(ULONG Index)
        {
            KInvariant(_LCMBEntryIndex == (ULONG)-1);
            KInvariant(Index != (ULONG)-1);
            _LCMBEntryIndex = Index;
        }

        inline ULONG GetLCMBEntryIndex()
        {
            return(_LCMBEntryIndex);
        }

        inline VOID SetStreamConfiguration(
            __in ULONG MaxRecordSize,
            __in LONGLONG StreamSize,
            __in ULONG StreamMetadataSize
        )
        {
            _MaxRecordSize = MaxRecordSize;
            _StreamSize = StreamSize;
            _StreamMetadataSize = StreamMetadataSize;
        }

        inline ULONG GetMaxRecordSize()
        {
            KInvariant(_MaxRecordSize != (ULONG)-1);

            return(_MaxRecordSize);
        }

        inline LONGLONG GetStreamSize()
        {
            KInvariant(_StreamSize != (ULONG)-1);

            return(_StreamSize);
        }

        inline ULONG GetStreamMetadataSize()
        {
            KInvariant(_StreamMetadataSize != (ULONG)-1);

            return(_StreamMetadataSize);
        }

        inline VOID SetSharedQuota(
            __in LONGLONG Quota
            )
        {
            _SharedLogQuota = Quota;
        }

        inline LONGLONG GetSharedQuota(
            )
        {
            return(_SharedLogQuota);
        }

    private:
    //
    // Create, Delete stream asyncs
    //
        //
        // This async is used to initialize a new container and
        // metadata info file
        //
        class AsyncInitializeStreamContext : public KAsyncContextBase
        {
            K_FORCE_SHARED(AsyncInitializeStreamContext);

            friend OverlayStreamFreeService;

            public:
                AsyncInitializeStreamContext(
                    __in OverlayStreamFreeService& OS
                );

                KActivityId GetActivityId()
                {
                    return((KActivityId)_StreamId.Get().Data1);
                }

            private:
                VOID
                StartInitializeStream(
                    __in RvdLogManager& LogManager,
                    __in RvdLog& SharedLog,
                    __in SharedLCMBInfoAccess& LCMBInfo,
                    __in KGuid& DiskId,
                    __in_opt KString const * const Path,
                    __in const RvdLogStreamType LogStreamType,
                    __in KtlLogStreamId& StreamId,
                    __in ULONG MaxRecordSize,
                    __in LONGLONG StreamSize,
                    __in ULONG StreamMetadataSize,
                    __in_opt KString const * const Alias,
                    __in DWORD Flags,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
                );

            protected:
                VOID
                OnStart(
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

                VOID
                OnCompleted(
                    ) override ;

                VOID
                OnCancel(
                    ) override ;

            private:

                ULONGLONG DedicatedContainerSizeFudgeFactor(
                    __in ULONGLONG BaseSize
                    )
                {
                    return(BaseSize / 4);    // TODO: we need to make this more intelligent
                }

                enum { Initial,
                       AllocateResources, UpdateSharedMetadata, CreateSharedStream,
                       CreateDedicatedContainer, CreatedDedicatedMetadata, CreateDedicatedStream, MarkStreamCreated,
                       WaitForDedicatedContainerClose,
                       Completed, CompletedWithError } _State;

                VOID FSMContinue(
                    __in NTSTATUS Status
                    );

                VOID OperationCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

            private:
                //
                // General members
                //
                OverlayStreamFreeService::SPtr _OS;

                //
                // Parameters to api
                //
                RvdLogManager::SPtr _LogManager;
                RvdLog::SPtr _SharedLog;
                SharedLCMBInfoAccess::SPtr _LCMBInfo;
                KGuid _DiskId;
                KtlLogStreamId _StreamId;
                KtlLogContainerId _ContainerId;
                ULONG _MaxRecordSize;
                LONGLONG _StreamSize;
                ULONG _StreamMetadataSize;
                KString::CSPtr _Alias;
                KString::CSPtr _Path;
                RvdLogStreamType _LogStreamType;
                DWORD _Flags;

                //
                // Members needed for functionality
                //
                RvdLogManager::AsyncCreateLog::SPtr _CreateLogContext;
                RvdLog::AsyncCreateLogStreamContext::SPtr _CreateStreamContext;
                SharedLCMBInfoAccess::AsyncUpdateEntryContext::SPtr _UpdateEntryContext;
                SharedLCMBInfoAccess::AsyncAddEntryContext::SPtr _AddEntryContext;
                DedicatedLCMBInfoAccess::SPtr _DedMBInfo;

                RvdLogStream::SPtr _SharedLogStream;
                RvdLog::SPtr _DedicatedLogContainer;
                RvdLogStream::SPtr _DedicatedLogStream;

                ULONG _LCMBEntryIndex;

                KAsyncEvent _DedicatedLogShutdownEvent;
                KAsyncEvent::WaitContext::SPtr _DedicatedLogShutdownWait;
        };

        NTSTATUS
        CreateAsyncInitializeStreamContext(
            __out AsyncInitializeStreamContext::SPtr& Context
            );

    public:

        //
        // This async is used to delete a Stream and
        // metadata info file
        //
        class AsyncDeleteStreamContext : public KAsyncContextBase
        {
            K_FORCE_SHARED(AsyncDeleteStreamContext);

            friend OverlayStreamFreeService;

            public:
                AsyncDeleteStreamContext(
                    __in OverlayStreamFreeService& OS
                );

                VOID
                StartDeleteStream(
                    __in RvdLogManager& LogManager,
                    __in_opt RvdLog* const SharedLog,
                    __in_opt SharedLCMBInfoAccess* const LCMBInfo,
                    __in KGuid& DiskId,
                    __in_opt KString const * const Path,
                    __in KtlLogStreamId& StreamId,
                    __in ULONG LCMBEntryIndex,
                    __in ULONG StreamMetadataSize,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
                );

            protected:
                VOID
                OnStart(
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

                VOID
                OnCompleted(
                    ) override ;

                VOID
                OnCancel(
                    ) override ;

            private:
                enum { Initial,
                       AllocateResources, UpdateSharedMetadata1, DeleteDedMBInfo, DeleteDedicatedContainer,
                       DeleteSharedStream, UpdateSharedMetadata2,
                       Completed, CompletedWithError } _State;

                VOID FSMContinue(
                    __in NTSTATUS Status
                    );

                VOID OperationCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

            private:
                //
                // General members
                //

                //
                // Parameters to api
                //
                RvdLogManager::SPtr _LogManager;
                RvdLog::SPtr _SharedLog;
                SharedLCMBInfoAccess::SPtr _LCMBInfo;
                KGuid _DiskId;
                KString::CSPtr _Path;
                KtlLogStreamId _StreamId;
                KtlLogContainerId _ContainerId;
                ULONG _LCMBEntryIndex;
                ULONG _StreamMetadataSize;

                //
                // Members needed for functionality
                //
                RvdLogManager::AsyncDeleteLog::SPtr _DeleteLogContext;
                RvdLog::AsyncDeleteLogStreamContext::SPtr _DeleteStreamContext;
                DedicatedLCMBInfoAccess::SPtr _DedMBInfo;
                SharedLCMBInfoAccess::AsyncUpdateEntryContext::SPtr _UpdateEntryContext;
        };

        static NTSTATUS
        CreateAsyncDeleteStreamContext(
            __out AsyncDeleteStreamContext::SPtr& Context,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag
            );

    private:
    //
    // Free Service mangement definitions
    //
        VOID
        OpenToCloseTransitionCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );

        NTSTATUS
        OpenToCloseTransitionWorker(
            __in NTSTATUS Status,
            __in ServiceWrapper::Operation& Op,
            __in KAsyncServiceBase::CloseCompletionCallback CloseCompletion
            ) override;

        VOID
        OperationCreateCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );

        VOID
        OnOperationCreate(
            __in ServiceWrapper::Operation& Op
            );

        VOID
        OperationDeleteCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );

        VOID
        OnOperationDelete(
            __in ServiceWrapper::Operation& Op
            );

    public:
        NTSTATUS CreateSyncWaitContext(
            __out KAsyncEvent::WaitContext::SPtr& WaitContext
            );

        VOID ResetSyncEvent(
            );

        class OperationCreate : public ServiceWrapper::Operation
        {
            K_FORCE_SHARED(OperationCreate);

            friend OverlayStreamFreeService;

            public:
                OperationCreate(
                    __in OverlayStreamFreeService& OS,
                    __in RvdLogManager& BaseLogManager,
                    __in RvdLog& BaseSharedLog,
                    __in KGuid& DiskId,
                    __in_opt KString const * const Path,
                    __in_opt KString const * const Alias,
                    __in DWORD Flags,
                    __in const RvdLogStreamType LogStreamType,
                    __in SharedLCMBInfoAccess& LCMBInfo
                );

            private:
                AsyncInitializeStreamContext::SPtr _InitializeStreamContext;
                RvdLogManager::SPtr _BaseLogManager;
                RvdLog::SPtr _BaseSharedLog;
                SharedLCMBInfoAccess::SPtr _LCMBInfo;
                KtlLogContainerId _ContainerId;
                KString::CSPtr _Alias;
                DWORD _Flags;
                KGuid _DiskId;
                KString::CSPtr _Path;
                RvdLogStreamType _LogStreamType;
        };

        class OperationOpen : public ServiceWrapper::Operation
        {
            K_FORCE_SHARED(OperationOpen);

            friend OverlayStreamFreeService;

            public:
                OperationOpen(
                    __in OverlayStreamFreeService& OS,
                    __in OverlayLog& OverlayLog,
                    __in RvdLogManager& BaseLogManager,
                    __in RvdLog& BaseSharedLog,
                    __in SharedLCMBInfoAccess& LCMBInfo
                );

                KSharedPtr<OverlayLog> GetOverlayLog()
                {
                    return(_OverlayLog);
                }

                RvdLogManager::SPtr GetBaseLogManager()
                {
                    return(_BaseLogManager);
                }

                RvdLog::SPtr GetBaseSharedLog()
                {
                    return(_BaseSharedLog);
                }

                SharedLCMBInfoAccess::SPtr GetLCMBInfo()
                {
                    return(_LCMBInfo);
                }

            private:
                KSharedPtr<OverlayLog> _OverlayLog;
                RvdLogManager::SPtr _BaseLogManager;
                RvdLog::SPtr _BaseSharedLog;
                SharedLCMBInfoAccess::SPtr _LCMBInfo;
                KtlLogContainerId _ContainerId;
        };

        class OperationDelete : public ServiceWrapper::Operation
        {
            K_FORCE_SHARED(OperationDelete);

            friend OverlayStreamFreeService;

            public:
                OperationDelete(
                    __in OverlayStreamFreeService& OS,
                    __in RvdLogManager& BaseLogManager,
                    __in RvdLog& BaseSharedLog,
                    __in SharedLCMBInfoAccess& LCMBInfo
                );

            private:
                RvdLogManager::SPtr _BaseLogManager;
                RvdLog::SPtr _BaseSharedLog;
                SharedLCMBInfoAccess::SPtr _LCMBInfo;
                KtlLogContainerId _ContainerId;
        };

    private:
        //
        // Configuration
        //
        KtlLogStreamId _StreamId;
        KGuid _DiskId;
        KString::CSPtr _Path;
        ULONG _MaxRecordSize;
        LONGLONG _StreamSize;
        ULONG _StreamMetadataSize;
        DWORD _Flags;
        LONGLONG _SharedLogQuota;

        //
        // Index into shared log container metadata block
        //
        ULONG _LCMBEntryIndex;

        //
        // Count of number of times referenced on global streams list
        //
        LONG _OnListCount;

        KWString _DedicatedLogContainerType;

        //
        // This is the maximum number of streams that can be created
        // within a dedicated log.
        //
        static const ULONG _DedicatedLogContainerStreamCount = 4;

        //
        // Event that limits only one thread from performing Create, Open or Delete at a time
        //
        KAsyncEvent _SingleAccessEvent;

        //
        // Close worker
        //
        ServiceWrapper::Operation* _CloseOp;
        KAsyncServiceBase::CloseCompletionCallback _CloseCompletion;
};


class GlobalStreamsTable : public GlobalTable<OverlayStreamFreeService, KtlLogStreamId>
{
    public:
        GlobalStreamsTable(
            __in KAllocator& Allocator,
            __in ULONG AllocationTag
        );

        ~GlobalStreamsTable();


        NTSTATUS
        CreateOrGetOverlayStream(
            __in BOOLEAN ReadOnly,
            __in KtlLogStreamId& StreamId,
            __in KGuid& DiskId,
            __in_opt KString const * const Path,
            __in ULONG MaxRecordSize,
            __in LONGLONG StreamSize,
            __in ULONG StreamMetadataSize,
            __out OverlayStreamFreeService::SPtr& OS,
            __out BOOLEAN& StreamCreated
            );

        NTSTATUS
        FindOverlayStream(
            __in const KtlLogStreamId& StreamId,
            __out OverlayStreamFreeService::SPtr& OS
            );

        BOOLEAN
        ReleaseOverlayStream(
            __in OverlayStreamFreeService& OS
            );

        NTSTATUS
        GetFirstStream(
            __out OverlayStreamFreeService::SPtr& OS
            );

};

//
// Support for log stream aliases
//
class AliasToStreamEntry : public KShared<AliasToStreamEntry>, public KObject<AliasToStreamEntry>
{
    K_FORCE_SHARED(AliasToStreamEntry);

    public:
        AliasToStreamEntry(
            __in KtlLogStreamId& LogStreamId
        );

    public:
        //
        // CompareRoutine for ContainerAliasTable
        //
        static LONG CompareRoutine(
            __in KString::CSPtr Left,
            __in KString::CSPtr Right
            )
        {
            // -1 if Left < Right
            // +1 if Left > Right
            //  0 if Left == Right
            return (Left->Compare(*Right));
        }

        KtlLogStreamId GetStreamId()
        {
            return(_StreamId);
        }

    private:
        KtlLogStreamId _StreamId;
};

class ContainerAliasToStreamTable : public GlobalTable<AliasToStreamEntry, KString::CSPtr>
{
    public:
        ContainerAliasToStreamTable(
            __in KAllocator& Allocator,
            __in ULONG AllocationTag
        );

        ~ContainerAliasToStreamTable();

    private:
        VOID Clear();

};

class StreamToAliasEntry : public KShared<StreamToAliasEntry>, public KObject<StreamToAliasEntry>
{
    K_FORCE_SHARED(StreamToAliasEntry);

    public:
        StreamToAliasEntry(
            __in KString const & Alias
        );

    public:
        //
        // CompareRoutine for ContainerAliasTable
        //
        static LONG CompareRoutine(
            __in KtlLogStreamId Left,
            __in KtlLogStreamId Right
            )
        {
            // -1 if Left < Right
            // +1 if Left > Right
            //  0 if Left == Right
            int result = memcmp(&Left, &Right, sizeof(KtlLogStreamId));

            if (result < 0)
            {
                return(-1);
            }

            if (result > 0)
            {
                return(+1);
            }

            return(result);
        }

        KString::CSPtr GetAlias()
        {
            return(_Alias);
        }

    private:
        KString::CSPtr _Alias;
};

class ContainerStreamToAliasTable : public GlobalTable<StreamToAliasEntry, KtlLogStreamId>
{
    public:
        ContainerStreamToAliasTable(
            __in KAllocator& Allocator,
            __in ULONG AllocationTag
        );

        ~ContainerStreamToAliasTable();

    private:
        VOID Clear();

};


//
// OverlayLog is the object that provides a forwarding interface layer
// above the core logger RvdLog interface. It is also a service;
// opening the service accesses the RvdLog and metadata while closing
// the service closes access to them.
//
class GlobalContainersTable;
class KtlLogContainerKernel;

class OverlayLogBase : public WrappedServiceBase
{
#include "PhysLogContainer.h"
};

class OverlayLog : public OverlayLogBase
{
    K_FORCE_SHARED(OverlayLog);

    public:
        OverlayLog(
            __in OverlayManager& OM,
            __in RvdLogManager& LogManager,
            __in const KGuid& DiskId,
            __in_opt KString const * const Path,
            __in const KtlLogContainerId& ContainerId,
            __in ThrottledKIoBufferAllocator& ThrottledAllocator
        );

        OverlayLog(
            __in RvdLogManager& LogManager,
            __in KGuid& DiskId,
            __in_opt KString const * const Path,
            __in KtlLogContainerId& ContainerId,
            __in ThrottledKIoBufferAllocator& ThrottledAllocator
        );

        NTSTATUS
        CommonConstructor();
        
    //
    // Methods needed to satisfy WrappedServiceBase
    //
        static NTSTATUS CreateUnderlyingService(
            __out WrappedServiceBase::SPtr& Context,
            __in ServiceWrapper& FreeService,
            __in ServiceWrapper::Operation& OperationOpen,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag
        );

        NTSTATUS StartServiceOpen(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in_opt OpenCompletionCallback OpenCallbackPtr,
            __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr
        ) override ;

        NTSTATUS StartServiceClose(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in_opt CloseCompletionCallback CloseCallbackPtr
        ) override ;

    //
    // Useful accessors
    //
        ThrottledKIoBufferAllocator* GetThrottledAllocator()
        {
            return(_ThrottledAllocator.RawPtr());
        }

        KSharedPtr<OverlayManager> GetOverlayManager()
        {
            return(_OverlayManager);
        }

        KGuid GetDiskId()
        {
            return(_DiskId);
        }

        KtlLogContainerId GetContainerId()
        {
            return(_ContainerId);
        }

        NTSTATUS GetStreamQuota(
            __in KtlLogStreamId StreamId,
            __out ULONGLONG& Quota
            );

        RvdLog::SPtr GetBaseLogContainer()
        {
            return(_BaseLogContainer);
        }

        KInstrumentedComponent::SPtr GetInstrumentedComponent()
        {
            return(_LogInstrumentedComponent);
        }
        
    //
    // Service management definitions
    //
    public:
        //
        // Each request performed against the container must call
        // TryAcquireRequestRef() successfully from the Start..()
        // before actually starting the request. It may return an error
        // in the case that the container is closing.
        //
        // ReleaseRequestRef() should be called when the operation has
        // completed and is about to be completed back up to the caller.
        //
        NTSTATUS TryAcquireRequestRef();
        VOID ReleaseRequestRef();

        //
        // This overlay container tracks the KTL shim objects using it
        //
        VOID
        AddKtlLogContainer(
            __in KtlLogContainerKernel& ContainerUser
        );

        VOID
        RemoveKtlLogContainer(
            __in KtlLogContainerKernel& ContainerUser
        );

        VOID
        AccountForStreamOpen(
            __in OverlayStream& Osfs
            );

        VOID
        AccountForStreamClosed(
            __in OverlayStream& Osfs
            );

        VOID
        RecomputeStreamQuotas(
            __in BOOLEAN IncludeClosedStreams,
            __in ULONGLONG TotalDedicatedSpace
            );

        BOOLEAN IsAccelerateInActiveMode()
        {
            return(_IsAccelerateInActiveMode);
        }
        
        BOOLEAN
        IsUnderThrottleLimit(
            __out ULONGLONG& TotalSpace,
            __out ULONGLONG& FreeSpace
        );

        BOOLEAN
        ShouldThrottleSharedLog(
            __in OverlayStream::AsyncDestagingWriteContextOverlay& DestagingWriteContext,
            __out ULONGLONG& TotalSpace,
            __out ULONGLONG& FreeSpace,
            __out LONGLONG& LowestLsn,
            __out LONGLONG& HighestLsn,
            __out RvdLogStreamId& LowestLsnStreamId
        );

        VOID
        ShouldUnthrottleSharedLog(
            __in BOOLEAN ReleaseOne
        );

        VOID
        AccelerateFlushesIfNeeded(
            );

        VOID
        AccelerateFlushTimerCompletion(
            __in_opt KAsyncContextBase* const,
            __in KAsyncContextBase&
        );

        VOID
        TruncateCompletedCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
        );      
        
        ULONG
        WaitingThrottledCount()
        {
            return(_ThrottledWritesList.Count());
        }               
        
        LONGLONG* GetContainerDedicatedBytesWrittenPointer()
        {
            return(&_PerfCounterData.DedicatedBytesWritten);
        }
        
        LONGLONG* GetContainerSharedBytesWrittenPointer()
        {
            return(&_PerfCounterData.SharedBytesWritten);
        }
        
    private:

        VOID
        StreamRecoveryCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );

        VOID
        StreamFailedRecoveryCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );

        VOID
        StreamFailedRecoveryQueueDeactivated(
            __in ServiceWrapper& Service
            );

        NTSTATUS
        LCEntryCallback(
            __in SharedLCMBInfoAccess::LCEntryEnumAsync& Async
            );

    protected:
        VOID OnServiceOpen() override ;
        VOID OnServiceClose() override ;
        VOID OnServiceReuse() override ;

    private:
        enum { Unassigned,

               OpenInitial = 0x100, OpenBaseContainer, OpenLCMBInfo,
               CloseInitial = 0x200, CleanupStreams, FlushStream, ClosedStream, CloseLCMBInfo, CloseBaseLog

              } _State;

        VOID OpenServiceFSM(
            __in NTSTATUS Status
            );

        VOID CloseServiceFSM(
            __in NTSTATUS Status
            );

        VOID OpenServiceCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );

        VOID CloseServiceCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );

        VOID
        StreamQueueDeactivated(
            __in ServiceWrapper& Service
            );

        VOID
        StreamQueueDeactivatedOnCleanup(
            __in ServiceWrapper& Service
            );

    private:
        RvdLogManager::SPtr _BaseLogManager;
        KGuid _DiskId;
        KString::CSPtr _Path;
        KtlLogContainerId _ContainerId;

        ULONGLONG _TotalDedicatedSpace;

        RvdLogManager::AsyncOpenLog::SPtr _OpenLogContext;
        SharedLCMBInfoAccess::SPtr _LCMBInfo;

        enum { Closed, Opened } _ObjectState;

        RvdLog::SPtr _BaseLogContainer;
#if DBG
        RvdLog* _BaseLogContainerRaw;
#endif
        KAsyncEvent _BaseLogShutdownEvent;
        KAsyncEvent::WaitContext::SPtr _BaseLogShutdownWait;

        KSharedPtr<OverlayManager> _OverlayManager;

        NTSTATUS _FinalStatus;
        OverlayStreamFreeService::SPtr _LastOverlayStreamFS;
        ServiceWrapper::Operation::SPtr _CloseFSOperation;

        //
        // This is a list of KtlLogContainerKernel objects exposed for
        // this container
        //
        KNodeList<KtlLogContainerKernel> _ContainerUsers;

        ThrottledKIoBufferAllocator::SPtr _ThrottledAllocator;

        ULONGLONG _ThrottleThreshold;
        
#if !defined(PLATFORM_UNIX)
        //
        // Performance counter
        //
        KPerfCounterSetInstance::SPtr _PerfCounterSetInstance;
#endif
        LogContainerPerfCounterData _PerfCounterData;
        KInstrumentedComponent::SPtr _LogInstrumentedComponent;
        
    public:
        // Make this public for the convenience of the OverlayLogTest
        GlobalStreamsTable _StreamsTable;

    private:
        KSpinLock _AliasTablesLock;
        ContainerAliasToStreamTable _AliasToStreamTable;
        ContainerStreamToAliasTable _StreamToAliasTable;

        //
        // Shared log throttling
        //
        KNodeList<OverlayStream::AsyncDestagingWriteContextOverlay> _ThrottledWritesList;
        KSpinLock _ThrottleWriteLock;

        ULONG _AccelerateFlushActiveTimerInMs;
        ULONG _AccelerateFlushPassiveTimerInMs;
        ULONG _AccelerateFlushActivePercent;
        ULONG _AccelerateFlushPassivePercent;
        
        ULONG _AccelerateFlushTimerInMs;
        ULONGLONG _AcceleratedFlushActiveThreshold;
        ULONGLONG _AcceleratedFlushPassiveThreshold;

        BOOLEAN _IsAccelerateInActiveMode;
        KTimer::SPtr _AccelerateFlushTimer;
        KAsyncEvent::WaitContext::SPtr _AcceleratedFlushInProgress;
        ULONGLONG _AcceleratedFlushInProgressTimestamp;
        KtlLogStreamId _AcceleratedFlushInProgressStreamId;
        volatile LONG _ActiveFlushCount;
                
        
    //
    // Interface to the container should match that of the core logger
    //
    public:
        VOID
        QueryLogType(__out KWString& LogType) override ;

        VOID
        QueryLogId(__out KGuid& DiskId, __out RvdLogId& LogId) override ;

        VOID
        QuerySpaceInformation(
            __out_opt ULONGLONG* const TotalSpace,
            __out_opt ULONGLONG* const FreeSpace = nullptr) override ;

        ULONGLONG QueryReservedSpace() override ;

        ULONGLONG QueryCurrentReservation() override ;

        ULONG QueryMaxAllowedStreams() override ;

        ULONG QueryMaxRecordSize()  override ;

        ULONG QueryMaxUserRecordSize() override ;

        ULONG QueryUserRecordSystemMetadataOverhead() override ;

        VOID
        QueryLsnRangeInformation(
            __out LONGLONG& LowestLsn,
            __out LONGLONG& HighestLsn,
            __out RvdLogStreamId& LowestLsnStreamId
            ) override ;
        
        VOID
        QueryCacheSize(__out_opt LONGLONG* const ReadCacheSizeLimit, __out_opt LONGLONG* const ReadCacheUsage) override ;

        VOID
        SetCacheSize(__in LONGLONG CacheSizeLimit) override ;

        class AsyncCreateLogStreamContextOverlay : public AsyncCreateLogStreamContext
        {
            K_FORCE_SHARED(AsyncCreateLogStreamContextOverlay);

            public:
                AsyncCreateLogStreamContextOverlay(
                    __in OverlayLog& OContainer
                    );

                VOID
                StartCreateLogStream(
                    __in const RvdLogStreamId& LogStreamId,
                    __in const RvdLogStreamType& LogStreamType,
                    __out RvdLogStream::SPtr& LogStream,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;

                VOID
                StartCreateLogStream(
                    __in const RvdLogStreamId& LogStreamId,
                    __in const RvdLogStreamType& LogStreamType,
                    __in KGuid& DiskId,
                    __in_opt KString const * const Path,
                    __in ULONG MaxRecordSize,
                    __in LONGLONG StreamSize,
                    __in ULONG StreamMetadataSize,
                    __in_opt KString const * const Alias,
                    __in DWORD Flags,
                    __out OverlayStream::SPtr& LogStream,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
                    );

            protected:
                VOID
                OnStart(
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

                VOID
                OnCancel(
                    ) override ;

            private:
                enum { Initial, SyncStreamAccess, CreateOverlayStream, OpenOverlayStream, DeactivateQueue,
                       Completed, CompletedWithError } _State;

                VOID FSMContinue(
                    __in NTSTATUS Status
                    );

                VOID OperationCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

                VOID QueueDeactivated(
                    __in ServiceWrapper& Service
                    );

                VOID DoComplete(
                    __in NTSTATUS Status
                );
            private:
                //
                // General members
                //
                OverlayLog::SPtr _OverlayLog;

                //
                // Parameters to api
                //
                RvdLogStreamId _LogStreamId;
                RvdLogStreamType _LogStreamType;
                KGuid _DiskId;
                ULONG _MaxRecordSize;
                LONGLONG _StreamSize;
                ULONG _StreamMetadataSize;
                KString::CSPtr _Alias;
                KString::CSPtr _Path;
                DWORD _Flags;

                OverlayStream::SPtr* _ResultLogStreamPtr;

                //
                // Members needed for functionality (reused)
                //
                OverlayStreamFreeService::OperationCreate::SPtr _FSCreateOperation;
                OverlayStreamFreeService::OperationOpen::SPtr _FSOpenOperation;
                OverlayStreamFreeService::SPtr _OverlayStreamFS;
                BOOLEAN _OSRefCountTaken;
                BOOLEAN _QueueActivated;
                NTSTATUS _FinalStatus;
                BOOLEAN _OwnSyncEvent;
                KAsyncEvent::WaitContext::SPtr _SingleAccessWaitContext;
        };

        NTSTATUS
        CreateAsyncCreateLogStreamContext(__out AsyncCreateLogStreamContext::SPtr& Context) override ;


        class AsyncDeleteLogStreamContextOverlay : public AsyncDeleteLogStreamContext
        {
            K_FORCE_SHARED(AsyncDeleteLogStreamContextOverlay);

            public:
                AsyncDeleteLogStreamContextOverlay(
                    __in OverlayLog& OContainer
                    );

                VOID
                StartDeleteLogStream(
                    __in const RvdLogStreamId& LogStreamId,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;
            protected:
                VOID
                OnStart(
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

                VOID
                OnCancel(
                    ) override ;

            private:
                enum { Initial, SyncStreamAccess, DeleteOnDisk, ShutdownQueue, Completed, CompletedWithError } _State;

                VOID QueueDeactivated(
                    __in ServiceWrapper& Service
                );

                VOID FSMContinue(
                    __in NTSTATUS Status
                    );

                VOID OperationCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

                VOID DoComplete(
                    __in NTSTATUS Status
                );
            private:
                //
                // General members
                //
                OverlayLog::SPtr _OverlayLog;

                //
                // Parameters to api
                //
                RvdLogStreamId _LogStreamId;

                //
                // Members needed for functionality (reused)
                //
                OverlayStreamFreeService::OperationDelete::SPtr _FSDeleteOperation;
                OverlayStreamFreeService::SPtr _OverlayStreamFS;
                BOOLEAN _OSRefCountTaken;
                KAsyncEvent::WaitContext::SPtr _SingleAccessWaitContext;
                BOOLEAN _OwnSyncEvent;
        };

        NTSTATUS
        CreateAsyncDeleteLogStreamContext(__out AsyncDeleteLogStreamContext::SPtr& Context) override ;


        class AsyncOpenLogStreamContextOverlay : public AsyncOpenLogStreamContext
        {
            K_FORCE_SHARED(AsyncOpenLogStreamContextOverlay);

            public:
                AsyncOpenLogStreamContextOverlay(
                    __in OverlayLog& OContainer
                    );

                VOID
                StartOpenLogStream(
                    __in const RvdLogStreamId& LogStreamId,
                    __out OverlayStream::SPtr& LogStream,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

                VOID
                StartOpenLogStream(
                    __in const RvdLogStreamId& LogStreamId,
                    __out RvdLogStream::SPtr& LogStream,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;


            protected:
                VOID
                OnStart(
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

                VOID
                OnCancel(
                    ) override ;

            private:
                enum { Initial, SyncStreamAccess, OpenUp, Completed, CompletedWithError } _State;

                VOID FSMContinue(
                    __in NTSTATUS Status
                    );

                VOID OperationCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

                VOID DoComplete(
                    __in NTSTATUS Status
                );
            private:
                //
                // General members
                //
                OverlayLog::SPtr _OverlayLog;

                //
                // Parameters to api
                //
                RvdLogStreamId _LogStreamId;
                OverlayStream::SPtr* _ResultLogStreamPtr;

                //
                // Members needed for functionality (reused)
                //
                OverlayStreamFreeService::OperationOpen::SPtr _FSOpenOperation;
                OverlayStreamFreeService::SPtr _OverlayStreamFS;
                BOOLEAN _OSRefCountTaken;
                KAsyncEvent::WaitContext::SPtr _SingleAccessWaitContext;
                BOOLEAN _OwnSyncEvent;
        };

        NTSTATUS
        CreateAsyncOpenLogStreamContext(__out AsyncOpenLogStreamContext::SPtr& Context) override ;

        class AsyncEnumerateStreamsContextOverlay : public KAsyncContextBase
        {
            K_FORCE_SHARED(AsyncEnumerateStreamsContextOverlay);

            friend OverlayLog;

            AsyncEnumerateStreamsContextOverlay(
                __in OverlayLog& OContainer
                );

            public:
                VOID
                StartEnumerateStreams(
                    __out KArray<KtlLogStreamId>& LogStreamIds,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

            protected:
                VOID
                OnStart(
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

                VOID
                OnCancel(
                    ) override ;

            private:
                enum { Initial, Completed, CompletedWithError } _State;

                VOID FSMContinue(
                    __in NTSTATUS Status
                    );

                VOID OperationCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

                VOID DoComplete(
                    __in NTSTATUS Status
                );
            private:
                //
                // General members
                //
                OverlayLog::SPtr _OverlayLog;

                //
                // Parameters to api
                //
                KArray<KtlLogStreamId>* _LogStreamIds;

                //
                // Members needed for functionality (reused)
                //
        };

        NTSTATUS
        CreateAsyncEnumerateStreamsContext(__out AsyncEnumerateStreamsContextOverlay::SPtr& Context);

        //** Test support API
        BOOLEAN
        IsStreamIdValid(__in RvdLogStreamId StreamId) override ;

        BOOLEAN
        GetStreamState(
            __in RvdLogStreamId StreamId,
            __out_opt BOOLEAN * const IsOpen = nullptr,
            __out_opt BOOLEAN * const IsClosed = nullptr,
            __out_opt BOOLEAN * const IsDeleted = nullptr) override ;

        BOOLEAN
        IsLogFlushed() override ;

        ULONG
        GetNumberOfStreams() override ;

        ULONG
        GetStreams(__in ULONG MaxResults, __in RvdLogStreamId* Streams) override ;

        NTSTATUS
        SetShutdownEvent(__in_opt KAsyncEvent* const EventToSignal) override ;


        class AsyncCloseLogStreamContextOverlay : public KAsyncContextBase
        {
            K_FORCE_SHARED(AsyncCloseLogStreamContextOverlay);

            public:
                AsyncCloseLogStreamContextOverlay(
                    __in OverlayLog& OContainer
                );

                VOID
                StartCloseLogStream(
                    __in const OverlayStream::SPtr& OverlayStream,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

            protected:
                VOID
                OnStart(
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

                VOID
                OnCancel(
                    ) override ;

            private:
                enum { Initial,
                       CloseStream,
                       Completed, CompletedWithError } _State;

                VOID FSMContinue(
                    __in NTSTATUS Status
                    );

                VOID OperationCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

                VOID DoComplete(
                    __in NTSTATUS Status
                );

            private:
                //
                // General members
                //
                OverlayLog::SPtr _OverlayLog;

                //
                // Parameters to api
                //
                OverlayStream::SPtr _OverlayStream;

                //
                // Members needed for functionality (reused)
                //
                ServiceWrapper::Operation::SPtr _FSOperation;
                OverlayStreamFreeService::SPtr _OverlayStreamFS;
                BOOLEAN _OSRefCountTaken;
        };

        NTSTATUS
        CreateAsyncCloseLogStreamContext(__out AsyncCloseLogStreamContextOverlay::SPtr& Context);


        //
        // Support for aliases
        //
        class AliasOperationContext : public KAsyncContextBase
        {
            K_FORCE_SHARED(AliasOperationContext);

            public:
                AliasOperationContext(
                    __in OverlayLog& OC
                );

                VOID StartUpdateAlias(
                    __in KString const & String,
                    __in KtlLogStreamId LogStreamId,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
                    );

                VOID StartRemoveAlias(
                    __in KString const & String,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
                    );

            protected:
                VOID
                OnStart(
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

                VOID
                OnCompleted(
                    ) override ;

                VOID
                OnCancel(
                    ) override ;

            private:
                enum { Unassigned = 0,
                       InitialAdd,
                       InitialRemove,
                       UpdateAliasEntry,
                       Completed, CompletedWithError } _State;

                VOID FSMContinue(
                    __in NTSTATUS Status
                    );

                VOID OperationCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

            private:
                //
                // General members
                //
                OverlayLog::SPtr _OverlayLog;
                SharedLCMBInfoAccess::AsyncUpdateEntryContext::SPtr _UpdateLCMBEntry;

                //
                // Parameters to api
                //
                KtlLogStreamId _StreamId;
                KString::CSPtr _Alias;

                //
                // Members needed for functionality
                //
        };

        NTSTATUS
        CreateAsyncAliasOperationContext(
            __out AliasOperationContext::SPtr& Context
            );

        NTSTATUS LookupAlias(
            __in KString const & String,
            __out KtlLogStreamId& LogStreamId
            );

        NTSTATUS AddOrUpdateAlias(
            __in KString const & Alias,
            __in KtlLogStreamId& LogStreamId
            );

        NTSTATUS
        RemoveAliasByAlias(
            __in KString const & Alias,
            __out KtlLogStreamId& StreamId
            );

        NTSTATUS
        RemoveAliasByAliasNoLock(
            __in KString const & Alias,
            __out KtlLogStreamId& StreamId
            );

        NTSTATUS
        RemoveAliasByStreamId(
            __in KtlLogStreamId& StreamId
            );
};


class OverlayLogFreeService : public ServiceWrapper
{
    K_FORCE_SHARED(OverlayLogFreeService);

    public:
        OverlayLogFreeService(
            __in OverlayManager& OM,
            __in RvdLogManager& BaseLogManager,
            __in KGuid DiskId,
            __in KtlLogContainerId ContainerId
        );

    public:
        static LONG CompareRoutine(
            __in KtlLogContainerId Left,
            __in KtlLogContainerId Right
            )
        {
            GUID leftGuid = Left.Get();
            GUID rightGuid = Right.Get();
            ULONG64 leftHi = *((PULONG64)&leftGuid);
            ULONG64 leftLo = *((PULONG64)((PUCHAR)&leftGuid + 8));
            ULONG64 rightHi = *((PULONG64)&rightGuid);
            ULONG64 rightLo = *((PULONG64)((PUCHAR)&rightGuid + 8));

            // -1 if Left < Right
            // +1 if Left > Right
            //  0 if Left == Right
            if (leftHi < rightHi)
            {
                return(-1);
            } else if (leftHi > rightHi) {
                return(+1);
            } else {
                if (leftLo < rightLo)
                {
                    return(-1);
                } else if (leftLo > rightLo) {
                    return(+1);
                }
            }
            return(0);
        }

        //
        // These manage the ref count for the number of times users are
        // referencing this while on the global list of containers.
        //
        VOID
        IncrementOnListCount(
            );

        BOOLEAN
        DecrementOnListCount(
            );

        class OperationOpen : public ServiceWrapper::Operation
        {
            K_FORCE_SHARED(OperationOpen);

            friend OverlayLogFreeService;

            public:
                OperationOpen(
                    __in OverlayLogFreeService& OC,
                    __in KGuid& DiskId,
                    __in_opt KString const * const Path
                );

                KString::CSPtr GetPath()
                {
                    return(_Path);
                }

                KGuid GetDiskId()
                {
                    return(_DiskId);
                }

            private:
                KGuid _DiskId;
                KString::CSPtr _Path;
        };


        //
        // Accessors
        //
        KGuid GetDiskId()
        {
            return(_DiskId);
        }

        KtlLogContainerId GetContainerId()
        {
            return(_ContainerId);
        }

        RvdLogManager::SPtr GetBaseLogManager()
        {
            return(_BaseLogManager);
        }

        OverlayManager& GetOverlayManager()
        {
            return(_OverlayManager);
        }

    private:
        //
        // This async is used to initialize a new container and
        // metadata info file
        //
        class AsyncInitializeContainerContext : public KAsyncContextBase
        {
            K_FORCE_SHARED(AsyncInitializeContainerContext);

            friend OverlayLogFreeService;

            public:
                AsyncInitializeContainerContext(
                    __in OverlayLogFreeService& OC
                );

            private:

                VOID
                StartInitializeContainer(
                    __in RvdLogManager& BaseLogManager,
                    __in KGuid DiskId,
                    __in_opt KString const * const Path,
                    __in KtlLogContainerId ContainerId,
                    __in ULONG MaxRecordSize,
                    __in ULONG MaxNumberStreams,
                    __in KWString& LogType,
                    __in LONGLONG LogSize,
                    __in DWORD Flags,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
                );

            protected:
                VOID
                OnStart(
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

                VOID
                OnCompleted(
                    ) override ;

                VOID
                OnCancel(
                    ) override ;

            private:
                enum { Initial,
                       DeleteTempContainer, DeleteTempMBInfo,
                       CreateBaseContainer, CreateLCMBInfo, WaitForContainerClose,
                       MarkContainerReady, MarkContainerReadyMetadata,
                       Completed, CompletedWithError } _State;

                VOID FSMContinue(
                    __in NTSTATUS Status
                    );

                VOID OperationCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

            private:
                //
                // General members
                //
                OverlayLogFreeService::SPtr _OC;

                //
                // Parameters to api
                //
                RvdLogManager::SPtr _BaseLogManager;
                KGuid _DiskId;
                KString::CSPtr _Path;
                KString::SPtr _PathTemp;
                KtlLogContainerId _ContainerId;
                ULONG _MaxNumberStreams;
                ULONG _MaxRecordSize;
                KWString* _LogType;
                LONGLONG _LogSize;
                DWORD _Flags;

                //
                // Members needed for functionality
                //
                RvdLogManager::AsyncCreateLog::SPtr _CreateLogContext;
                SharedLCMBInfoAccess::SPtr _LCMBInfo;
                RvdLog::SPtr _BaseLogContainer;
                KAsyncEvent _BaseLogShutdownEvent;
                KAsyncEvent::WaitContext::SPtr _BaseLogShutdownWait;
        };

        NTSTATUS
        CreateAsyncInitializeContainerContext(
            __out AsyncInitializeContainerContext::SPtr& Context
            );

        //
        // This async is used to delete a container and
        // metadata info file
        //
        class AsyncDeleteContainerContext : public KAsyncContextBase
        {
            K_FORCE_SHARED(AsyncDeleteContainerContext);

            friend OverlayLogFreeService;

            public:
                AsyncDeleteContainerContext(
                    __in OverlayLogFreeService& OC
                );

            private:

                VOID
                StartDeleteContainer(
                    __in RvdLogManager& BaseLogManager,
                    __in KGuid DiskId,
                    __in_opt KString const * const Path,
                    __in KtlLogContainerId ContainerId,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
                );

            protected:
                VOID
                OnStart(
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

                VOID
                OnCancel(
                    ) override ;

            private:
                enum { Initial,
                       OpenBaseContainer, DeleteAllStreams, CloseLCMBInfo, CloseBaseContainer,
                       DeleteLCMBInfo, DeleteBaseContainer, WaitForAllDeleteStreams,
                       Completed, CompletedWithError } _State;

                VOID DoComplete(
                    __in NTSTATUS Status
                    );

                VOID FSMContinue(
                    __in NTSTATUS Status
                    );

                VOID DeleteStreamCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

                VOID OperationCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

                NTSTATUS
                LCEntryCallback(
                    __in SharedLCMBInfoAccess::LCEntryEnumAsync& Async
                    );

            private:
                //
                // General members
                //
                OverlayLogFreeService::SPtr _OC;

                //
                // Parameters to api
                //
                RvdLogManager::SPtr _BaseLogManager;
                KGuid _DiskId;
                KtlLogContainerId _ContainerId;
                KString::CSPtr _Path;

                //
                // Members needed for functionality
                //
                RvdLogManager::AsyncDeleteLog::SPtr _DeleteLogContext;
                RvdLogManager::AsyncOpenLog::SPtr _OpenBaseLog;
                RvdLog::SPtr _BaseSharedLog;
#if DBG
                RvdLog* _BaseSharedLogRaw;
#endif
                SharedLCMBInfoAccess::SPtr _LCMBInfo;
                KAsyncEvent _BaseSharedLogShutdownEvent;
                KAsyncEvent::WaitContext::SPtr _BaseSharedLogShutdownWait;

                KAsyncEvent _DeleteStreamCompletionEvent;
                KAsyncEvent::WaitContext::SPtr _DeleteStreamCompletionWait;
                LONG _DeleteStreamAsyncCount;
        };

        NTSTATUS
        CreateAsyncDeleteContainerContext(
            __out AsyncDeleteContainerContext::SPtr& Context
            );


    //
    // Free Service mangement definitions
    //
        VOID
        OperationCreateCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );

        VOID
        OnOperationCreate(
            __in ServiceWrapper::Operation& Op
            );

        VOID
        OperationDeleteCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );

        VOID
        OnOperationDelete(
            __in ServiceWrapper::Operation& Op
            );

    public:

        class OperationCreate : public ServiceWrapper::Operation
        {
            K_FORCE_SHARED(OperationCreate);

            friend OverlayLogFreeService;

            public:
                OperationCreate(__in OverlayLogFreeService& OC,
                                __in KGuid& DiskId,
                                __in_opt KString const * const Path,
                                __in ULONG MaxRecordSize,
                                __in ULONG MaxNumberStreams,
                                __in KWString& LogType,
                                __in LONGLONG LogSize,
                                __in DWORD Flags
                                );

            private:
                KGuid _DiskId;
                KString::CSPtr _Path;
                ULONG _MaxRecordSize;
                ULONG _MaxNumberStreams;
                KWString* _LogType;
                LONGLONG _LogSize;
                DWORD _Flags;
        };

        class OperationDelete : public ServiceWrapper::Operation
        {
            K_FORCE_SHARED(OperationDelete);

            friend OverlayLogFreeService;

            public:
                OperationDelete(__in OverlayLogFreeService& OC,
                                __in KGuid DiskId,
                                __in_opt KString const * const Path
                   );

            private:
                KGuid _DiskId;
                KString::CSPtr _Path;
        };

    private:
        //
        // General members
        //
        OverlayManager& _OverlayManager;
        RvdLogManager::SPtr _BaseLogManager;
        KGuid _DiskId;
        KtlLogContainerId _ContainerId;

        //
        // Members used while running
        //
        ULONG _OnListCount;
};

class GlobalContainersTable : public GlobalTable<OverlayLogFreeService, KtlLogContainerId>
{
    public:
        GlobalContainersTable(
            __in OverlayManager& OM,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag
            );

        ~GlobalContainersTable();

        NTSTATUS
        CreateOrGetOverlayContainer(
            __in const KGuid& DiskId,
            __in const KtlLogContainerId& ContainerId,
            __out OverlayLogFreeService::SPtr& OC,
            __out BOOLEAN& ContainerCreated
            );

        BOOLEAN
        ReleaseOverlayContainer(
            __in OverlayLogFreeService& OC
            );

        NTSTATUS
        GetFirstContainer(
            __out OverlayLogFreeService::SPtr& OC
            );

        VOID
        SetBaseLogManager(
            __in RvdLogManager& BaseLogManager
            )
        {
            _BaseLogManager = &BaseLogManager;
        }

    private:
        RvdLogManager::SPtr _BaseLogManager;
        OverlayManager& _OverlayManager;
};



class OverlayManagerBase : public KAsyncServiceBase
{
#include "PhysLogManager.h"
};



class LogStreamOpenGateContext : public KAsyncContextBase
{
    K_FORCE_SHARED(LogStreamOpenGateContext);

    public:
        LogStreamOpenGateContext(
            __in OverlayManager& OM
        );

        VOID
        StartWaitForGate(
            __in KGuid& DiskId,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

        VOID
        ReleaseGate(
            __in OverlayStream& OStream
        );

    private:
        enum { Initial, WaitForQuanta, Completed, CompletedWithError } _State;

        VOID FSMContinue(
            __in NTSTATUS Status
            );

        VOID OperationCompleted(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );

    protected:
        VOID
        OnStart(
            ) override ;

        VOID
        OnReuse(
            ) override ;

        VOID
        OnCompleted(
            ) override ;

        VOID
        OnCancel(
            ) override ;

    private:
        //
        // General members
        //
        KSharedPtr<OverlayManager> _OverlayManager;

        //
        // Parameters to api
        //
        KGuid _DiskId;

        //
        // Members needed for functionality
        //
        KQuotaGate::AcquireContext::SPtr _QuotaAcquireContext;
        KQuotaGate::SPtr _Gate;
};

class OverlayManager : public OverlayManagerBase
{
    K_FORCE_SHARED(OverlayManager);

    public:
        OverlayManager::OverlayManager(
            __in KtlLogManager::MemoryThrottleLimits& MemoryThrottleLimits,
            __in KtlLogManager::SharedLogContainerSettings& SharedLogContainerSettings,
            __in KtlLogManager::AcceleratedFlushLimits& AcceleratedFlushLimits
            );

    public:
        static NTSTATUS Create(
           __out OverlayManager::SPtr& Result,
           __in KtlLogManager::MemoryThrottleLimits& MemoryThrottleLimits,
           __in KtlLogManager::SharedLogContainerSettings& SharedLogContainerSettings,
           __in KtlLogManager::AcceleratedFlushLimits& AcceleratedFlushLimits,
           __in KAllocator& Allocator,
           __in ULONG AllocationTag
        );

    //
    // Methods needed to satisfy WrappedServiceBase
    //
        static NTSTATUS CreateUnderlyingService(
            __out WrappedServiceBase::SPtr& Context,
            __in ServiceWrapper& FreeService,
            __in ServiceWrapper::Operation& OperationOpen,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag
        );

        NTSTATUS StartServiceOpen(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in_opt OpenCompletionCallback OpenCallbackPtr,
            __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr
        );

        NTSTATUS StartServiceClose(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in_opt CloseCompletionCallback CloseCallbackPtr
        );

    //
    // Quota gate routines for throttling log stream opens
    //
        class GateTableEntry
        {
            friend OverlayManager;

            private:
                KListEntry Link;
                KGuid DiskId;
                KQuotaGate::SPtr Gate;
        };

#if !defined(PLATFORM_UNIX)
        KVolumeNamespace::VolumeInformationArray _VolumeList;
#endif

        KSpinLock _GateTableSpinLock;
        KNodeList<GateTableEntry> _GateList;  // TODO: move these
        ULONG _GateCount;

        NTSTATUS CreateOrGetLogStreamGate(
            __in KGuid& DiskId,
            __out KQuotaGate::SPtr& Gate
        );

        NTSTATUS FindLogStreamGate(
            __in KGuid& DiskId,
            __out KQuotaGate::SPtr& Gate
        );

        ULONG DeactivateAllLogStreamGates();

        NTSTATUS CreateLogStreamOpenGateContext(
            __out LogStreamOpenGateContext::SPtr& Context
        );

        NTSTATUS MapPathToDiskId(
            __in KString const & Path,
            __out KGuid& DiskId
            );


#if !defined(PLATFORM_UNIX)
    //
    // Perf counter definitions
    //
    private:
        NTSTATUS ConfigurePerfCounters(
            );

    public:
        NTSTATUS CreateNewLogStreamPerfCounterSet(
            __in KtlLogStreamId StreamId,
            __in OverlayStream& LogStream,
            __out KPerfCounterSetInstance::SPtr& PerfCounterSetInstance
            );

        NTSTATUS CreateNewLogContainerPerfCounterSet(
            __in KtlLogContainerId ContainerId,
            __in LogContainerPerfCounterData* Data,
            __out KPerfCounterSetInstance::SPtr& PerfCounterSetInstance
            );

        NTSTATUS 
        CreateNewLogManagerPerfCounterSet(
            __out KPerfCounterSetInstance::SPtr& PerfCounterSetInstance
            );
        
        KPerfCounterManager::SPtr _PerfCounterMgr;
        KPerfCounterSet::SPtr _PerfCounterSetLogStream;                
        KPerfCounterSet::SPtr _PerfCounterSetLogContainer;                
        KPerfCounterSet::SPtr _PerfCounterSetLogManager;
        KPerfCounterSetInstance::SPtr _PerfCounterSetLogManagerInstance;
#endif
        
    //
    // Service mangement definitions
    //
    public:
        //
        // Each request performed against the container must call
        // TryAcquireRequestRef() successfully from the Start..()
        // before actually starting the request. It may return an error
        // in the case that the container is closing.
        //
        // ReleaseRequestRef() should be called when the operation has
        // completed and is about to be completed back up to the caller.
        //
        NTSTATUS TryAcquireRequestRef();
        VOID ReleaseRequestRef();

    protected:
        VOID OnServiceOpen() override ;
        VOID OnServiceClose() override ;
        VOID OnServiceReuse() override ;

    private:
        enum { Closed, Opened } _ObjectState;

        enum { Created, OpenAttempted, CloseInitial, CloseContainers, WaitForQuotaGateDeactivation, DeactivateBaseLogManager, CloseCleanup,
               Completed, CompletedWithError } _State;

        VOID DoCompleteClose(
            __in NTSTATUS Status
            );

        VOID CloseServiceFSM(
            __in NTSTATUS Status
            );

        VOID CloseServiceCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );

        VOID QueryVolumeListCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );

        VOID QueueDeactivated(
            __in ServiceWrapper& Service
        );

    //
    // RvdLogManager Methods
    //
    public:
        NTSTATUS
        Activate(
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;

        VOID
        Deactivate() override;

        class AsyncEnumerateLogsOverlay : public AsyncEnumerateLogs
        {
            K_FORCE_SHARED(AsyncEnumerateLogsOverlay);

            public:
                AsyncEnumerateLogsOverlay(
                    __in OverlayManager& OM
                );

                VOID
                StartEnumerateLogs(
                    __in const KGuid& DiskId,
                    __out KArray<RvdLogId>& LogIdArray,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;

            protected:
                VOID
                OnStart(
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

                VOID
                OnCancel(
                    ) override ;

            private:
                VOID OperationCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

            private:
                //
                // General members
                //
                OverlayManager::SPtr _OM;

                //
                // Parameters to api
                //
                KGuid _DiskId;
                KArray<RvdLogId>* _LogIdArray;


                //
                // Members needed for functionality
                //
                RvdLogManager::AsyncEnumerateLogs::SPtr _BaseEnumerateLogs;
            };

        NTSTATUS
        CreateAsyncEnumerateLogsContext(__out AsyncEnumerateLogs::SPtr& Context) override;


        class AsyncCreateLogOverlay : public AsyncCreateLog
        {
            K_FORCE_SHARED(AsyncCreateLogOverlay);

            public:
                AsyncCreateLogOverlay(
                    __in OverlayManager& OM
                );

                VOID
                StartCreateLog(
                    __in KGuid const& DiskId,
                    __in_opt KString const * const Path,
                    __in RvdLogId const& LogId,
                    __in KWString& LogType,
                    __in LONGLONG LogSize,
                    __in ULONG MaxAllowedStreams,
                    __in ULONG MaxRecordSize,
                    __in DWORD Flags,
                    __out OverlayLog::SPtr& Log,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

                VOID
                StartCreateLog(
                    __in KGuid const& DiskId,
                    __in RvdLogId const& LogId,
                    __in KWString& LogType,
                    __in LONGLONG LogSize,
                    __out RvdLog::SPtr& Log,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override
                {
                    UNREFERENCED_PARAMETER(DiskId);
                    UNREFERENCED_PARAMETER(LogId);
                    UNREFERENCED_PARAMETER(LogType);
                    UNREFERENCED_PARAMETER(LogSize);
                    UNREFERENCED_PARAMETER(Log);
                    UNREFERENCED_PARAMETER(ParentAsyncContext);
                    UNREFERENCED_PARAMETER(CallbackPtr);

                    //
                    // Callers should use one that returns
                    // OverlayLog
                    //
                    KInvariant(FALSE);
                }

                VOID
                StartCreateLog(
                    __in KGuid const& DiskId,
                    __in RvdLogId const& LogId,
                    __in KWString& LogType,
                    __in LONGLONG LogSize,
                    __in ULONG MaxAllowedStreams,
                    __in ULONG MaxRecordSize,
                    __out RvdLog::SPtr& Log,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override
                {
                    UNREFERENCED_PARAMETER(DiskId);
                    UNREFERENCED_PARAMETER(LogId);
                    UNREFERENCED_PARAMETER(LogType);
                    UNREFERENCED_PARAMETER(LogSize);
                    UNREFERENCED_PARAMETER(MaxAllowedStreams);
                    UNREFERENCED_PARAMETER(MaxRecordSize);
                    UNREFERENCED_PARAMETER(Log);
                    UNREFERENCED_PARAMETER(ParentAsyncContext);
                    UNREFERENCED_PARAMETER(CallbackPtr);

                    //
                    // Callers should use one that returns
                    // OverlayLog
                    //
                    KInvariant(FALSE);
                }

                VOID
                StartCreateLog(
                    __in KStringView const& FullyQualifiedLogFilePath,
                    __in RvdLogId const& LogId,
                    __in KWString& LogType,
                    __in LONGLONG LogSize,
                    __in ULONG MaxAllowedStreams,
                    __in ULONG MaxRecordSize,
                    __out RvdLog::SPtr& Log,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override
                {
                    UNREFERENCED_PARAMETER(FullyQualifiedLogFilePath);
                    UNREFERENCED_PARAMETER(LogId);
                    UNREFERENCED_PARAMETER(LogType);
                    UNREFERENCED_PARAMETER(LogSize);
                    UNREFERENCED_PARAMETER(MaxAllowedStreams);
                    UNREFERENCED_PARAMETER(MaxRecordSize);
                    UNREFERENCED_PARAMETER(Log);
                    UNREFERENCED_PARAMETER(ParentAsyncContext);
                    UNREFERENCED_PARAMETER(CallbackPtr);

                    //
                    // Callers should use one that returns
                    // OverlayLog
                    //
                    KInvariant(FALSE);
                }

                VOID
                StartCreateLog(
                    __in KGuid const& DiskId,
                    __in RvdLogId const& LogId,
                    __in KWString& LogType,
                    __in LONGLONG LogSize,
                    __in ULONG MaxAllowedStreams,
                    __in ULONG MaxRecordSize,
                    __in DWORD Flags,
                    __out RvdLog::SPtr& Log,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override
                {
                    UNREFERENCED_PARAMETER(DiskId);
                    UNREFERENCED_PARAMETER(LogId);
                    UNREFERENCED_PARAMETER(LogType);
                    UNREFERENCED_PARAMETER(LogSize);
                    UNREFERENCED_PARAMETER(MaxAllowedStreams);
                    UNREFERENCED_PARAMETER(MaxRecordSize);
                    UNREFERENCED_PARAMETER(Flags);
                    UNREFERENCED_PARAMETER(Log);
                    UNREFERENCED_PARAMETER(ParentAsyncContext);
                    UNREFERENCED_PARAMETER(CallbackPtr);

                    //
                    // Callers should use one that returns
                    // OverlayLog
                    //
                    KInvariant(FALSE);
                }

                VOID
                StartCreateLog(
                    __in KStringView const& FullyQualifiedLogFilePath,
                    __in RvdLogId const& LogId,
                    __in KWString& LogType,
                    __in LONGLONG LogSize,
                    __in ULONG MaxAllowedStreams,
                    __in ULONG MaxRecordSize,
                    __in DWORD Flags,
                    __out RvdLog::SPtr& Log,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override
                {
                    UNREFERENCED_PARAMETER(FullyQualifiedLogFilePath);
                    UNREFERENCED_PARAMETER(LogId);
                    UNREFERENCED_PARAMETER(LogType);
                    UNREFERENCED_PARAMETER(LogSize);
                    UNREFERENCED_PARAMETER(MaxAllowedStreams);
                    UNREFERENCED_PARAMETER(MaxRecordSize);
                    UNREFERENCED_PARAMETER(Flags);
                    UNREFERENCED_PARAMETER(Log);
                    UNREFERENCED_PARAMETER(ParentAsyncContext);
                    UNREFERENCED_PARAMETER(CallbackPtr);

                    //
                    // Callers should use one that returns
                    // OverlayLog
                    //
                    KInvariant(FALSE);
                }


            protected:
                VOID
                OnStart(
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

                VOID
                OnCancel(
                    ) override ;

            private:
                enum { Initial,
                       InitializeContainer, ReopenContainer, Cleanup,
                       Completed, CompletedWithError } _State;

                VOID FSMContinue(
                    __in NTSTATUS Status
                    );

                VOID OperationCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

                VOID DoComplete(
                    __in NTSTATUS Status
                );

                VOID
                QueueDeactivated(
                    __in ServiceWrapper& Service
                    );

            private:
                //
                // General members
                //
                OverlayManager::SPtr _OM;
                static const ULONG _DefaultMaxNumberStreams = 1024; // Should match RvdLogOnDiskConfig::_DefaultMaxNumberOfStreams

                //
                // Parameters to api
                //
                KGuid _DiskId;
                KString::CSPtr _Path;
                RvdLogId _LogId;
                KWString* _LogType;
                LONGLONG _LogSize;
                ULONG _MaxNumberStreams;
                ULONG _MaxRecordSize;
                DWORD _Flags;
                OverlayLog::SPtr* _LogPtr;

                //
                // Members needed for functionality
                //
                OverlayLogFreeService::SPtr _OC;
                OverlayLogFreeService::OperationCreate::SPtr _OCOperation;
                BOOLEAN _OCReferenced;
                NTSTATUS _FinalStatus;
        };

        NTSTATUS
        CreateAsyncCreateLogContext(__out AsyncCreateLog::SPtr& Context) override ;


        class AsyncOpenLogOverlay : public AsyncOpenLog
        {
            K_FORCE_SHARED(AsyncOpenLogOverlay);

            public:
                AsyncOpenLogOverlay(
                    __in OverlayManager& OM
                );

                VOID
                StartOpenLog(
                    __in const KGuid& DiskId,
                    __in_opt KString const * const Path,
                    __in const RvdLogId& LogId,
                    __out OverlayLog::SPtr& Log,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

                VOID
                StartOpenLog(
                    __in const KGuid& DiskId,
                    __in const RvdLogId& LogId,
                    __out RvdLog::SPtr& Log,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override
                {
                    UNREFERENCED_PARAMETER(DiskId);
                    UNREFERENCED_PARAMETER(LogId);
                    UNREFERENCED_PARAMETER(Log);
                    UNREFERENCED_PARAMETER(ParentAsyncContext);
                    UNREFERENCED_PARAMETER(CallbackPtr);

                    //
                    // Callers should use one that returns
                    // OverlayContainer
                    //
                    KInvariant(FALSE);
                }

                VOID
                StartOpenLog(
                    __in const KStringView& FullyQualifiedLogFilePath,
                    __out RvdLog::SPtr& Log,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override
                {
                    UNREFERENCED_PARAMETER(FullyQualifiedLogFilePath);
                    UNREFERENCED_PARAMETER(Log);
                    UNREFERENCED_PARAMETER(ParentAsyncContext);
                    UNREFERENCED_PARAMETER(CallbackPtr);

                    //
                    // Callers should use one that returns
                    // OverlayContainer
                    //
                    KInvariant(FALSE);
                }

            protected:
                VOID
                OnStart(
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

                VOID
                OnCancel(
                    ) override ;

            private:
                enum { Initial,
                       OpenLog, Cleanup,
                       Completed, CompletedWithError } _State;

                VOID FSMContinue(
                    __in NTSTATUS Status
                    );

                VOID OperationCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

                VOID DoComplete(
                    __in NTSTATUS Status
                );

                VOID
                QueueDeactivated(
                    __in ServiceWrapper& Service
                    );

            private:
                //
                // General members
                //
                OverlayManager::SPtr _OM;

                //
                // Parameters to api
                //
                KGuid _DiskId;
                KString::CSPtr _Path;
                RvdLogId _LogId;
                OverlayLog::SPtr* _LogPtr;

                //
                // Members needed for functionality
                //
                OverlayLogFreeService::SPtr _OC;
                ServiceWrapper::Operation::SPtr _OCOperation;
                BOOLEAN _OCReferenced;
                NTSTATUS _FinalStatus;
        };

        NTSTATUS
        CreateAsyncOpenLogContext(__out AsyncOpenLog::SPtr& Context) override ;


        class AsyncQueryLogIdOverlay : public AsyncQueryLogId
        {
            K_FORCE_SHARED(AsyncQueryLogIdOverlay);

            public:
                AsyncQueryLogIdOverlay(
                    __in OverlayManager& OM
                );

            VOID
            StartQueryLogId(
                __in const KStringView& FullyQualifiedLogFilePath,
                __out RvdLogId& LogId,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;
                
            protected:
                VOID
                OnStart(
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

                VOID
                OnCancel(
                    ) override ;

            private:
                enum { Initial,
                       QueryLogId,
                       Completed, CompletedWithError } _State;

                VOID FSMContinue(
                    __in NTSTATUS Status
                    );

                VOID OperationCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

                VOID DoComplete(
                    __in NTSTATUS Status
                );

            private:
                //
                // General members
                //
                OverlayManager::SPtr _OM;

                //
                // Parameters to api
                //
                RvdLogId* _LogId;
                const KStringView* _Path;

                //
                // Members needed for functionality
                //
        };

        NTSTATUS
        CreateAsyncQueryLogIdContext(__out AsyncQueryLogId::SPtr& Context) override ;

        class AsyncCloseLogOverlay : public KAsyncContextBase
        {
            K_FORCE_SHARED(AsyncCloseLogOverlay);

            public:
                AsyncCloseLogOverlay(
                    __in OverlayManager& OM
                );

                VOID
                StartCloseLog(
                    __in const OverlayLog::SPtr& OverlayLog,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

            protected:
                VOID
                OnStart(
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

                VOID
                OnCancel(
                    ) override ;

            private:
                enum { Initial,
                       CloseLog, Cleanup,
                       Completed, CompletedWithError } _State;

                VOID FSMContinue(
                    __in NTSTATUS Status
                    );

                VOID OperationCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

                VOID DoComplete(
                    __in NTSTATUS Status
                );

                VOID
                QueueDeactivated(
                    __in ServiceWrapper& Service
                    );

            private:
                //
                // General members
                //
                OverlayManager::SPtr _OM;

                //
                // Parameters to api
                //
                OverlayLog::SPtr _Log;

                //
                // Members needed for functionality
                //
                OverlayLogFreeService::SPtr _OC;
                ServiceWrapper::Operation::SPtr _OCOperation;
                BOOLEAN _OCReferenced;
                NTSTATUS _FinalStatus;
        };

        NTSTATUS
        CreateAsyncCloseLogContext(__out AsyncCloseLogOverlay::SPtr& Context);


        class AsyncDeleteLogOverlay : public AsyncDeleteLog
        {
            K_FORCE_SHARED(AsyncDeleteLogOverlay);

            public:
                AsyncDeleteLogOverlay(
                    __in OverlayManager& OM
                );

                VOID
                StartDeleteLog(
                    __in const KGuid& DiskId,
                    __in const RvdLogId& LogId,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override
                {
                    UNREFERENCED_PARAMETER(DiskId);
                    UNREFERENCED_PARAMETER(LogId);
                    UNREFERENCED_PARAMETER(ParentAsyncContext);
                    UNREFERENCED_PARAMETER(CallbackPtr);

                    KInvariant(FALSE);
                }

                VOID
                StartDeleteLog(
                    __in const KStringView& FullyQualifiedLogFilePath,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override
                {
                    UNREFERENCED_PARAMETER(FullyQualifiedLogFilePath);
                    UNREFERENCED_PARAMETER(ParentAsyncContext);
                    UNREFERENCED_PARAMETER(CallbackPtr);

                    KInvariant(FALSE);
                }

                VOID
                StartDeleteLog(
                    __in const KGuid& DiskId,
                    __in_opt KString const * const Path,
                    __in const RvdLogId& LogId,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);


            protected:
                VOID
                OnStart(
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

                VOID
                OnCancel(
                    ) override ;

            private:
                enum { Initial,
                       DeleteLog, Cleanup,
                       Completed, CompletedWithError } _State;

                VOID FSMContinue(
                    __in NTSTATUS Status
                    );

                VOID OperationCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

                VOID DoComplete(
                    __in NTSTATUS Status
                );

                VOID
                QueueDeactivated(
                    __in ServiceWrapper& Service
                    );

            private:
                //
                // General members
                //
                OverlayManager::SPtr _OM;

                //
                // Parameters to api
                //
                KGuid _DiskId;
                KString::CSPtr _Path;
                RvdLogId _LogId;

                //
                // Members needed for functionality
                //
                OverlayLogFreeService::SPtr _OC;
                ServiceWrapper::Operation::SPtr _OCOperation;
                BOOLEAN _OCReferenced;
                NTSTATUS _FinalStatus;
        };

        NTSTATUS
        CreateAsyncDeleteLogContext(__out AsyncDeleteLog::SPtr& Context) override ;

        class AsyncConfigureContextOverlay : public KAsyncContextBase
        {
            K_FORCE_SHARED(AsyncConfigureContextOverlay);

            public:
                AsyncConfigureContextOverlay(
                    __in OverlayManager& OM
                    );

                VOID
                StartConfigure(
                    __in ULONG ControlCode,
                    __in_opt KBuffer* const InBuffer,
                    __out ULONG& Result,
                    __out KBuffer::SPtr& OutBuffer,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

            protected:
                VOID
                OnStart(
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

                VOID
                OnCancel(
                    ) override ;

            private:
                enum { Initial, Completed } _State;

                VOID
                DoComplete(
                    __in NTSTATUS Status
                );

                VOID FSMContinue(
                    __in NTSTATUS Status
                    );

                VOID OperationCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

            private:
                //
                // General members
                //
                OverlayManager::SPtr _OM;

                //
                // Parameters to api
                //
                ULONG _Code;
                KBuffer::SPtr _InBuffer;
                ULONG* _Result;
                KBuffer::SPtr* _OutBuffer;

                //
                // Members needed for functionality (reused)
                //
        };

        NTSTATUS
        CreateAsyncConfigureContext(__out AsyncConfigureContextOverlay::SPtr& Context);


        NTSTATUS
        RegisterVerificationCallback(
            __in const RvdLogStreamType& LogStreamType,
            __in RvdLogManager::VerificationCallback Callback) override ;

        RvdLogManager::VerificationCallback
        UnRegisterVerificationCallback(__in const RvdLogStreamType& LogStreamType) override ;

        RvdLogManager::VerificationCallback
        QueryVerificationCallback(__in const RvdLogStreamType& LogStreamType) override ;

        VOID FreeKIoBuffer(
            __in KActivityId ActivityId,
            __in ULONG Size
        );

        NTSTATUS
        CreateAsyncAllocateKIoBufferContext(
            __out ThrottledKIoBufferAllocator::AsyncAllocateKIoBufferContext::SPtr& Context);

        ThrottledKIoBufferAllocator* GetThrottledAllocator()
        {
            return(_ThrottledAllocator.RawPtr());
        }

        LONGLONG GetPinnedMemoryLimit()
        {
            return(_MemoryThrottleLimits.PinnedMemoryLimit);
        }

        VOID GetPeriodicTimerSettings(
            __out ULONG& PeriodicFlushTimeInSec,
            __out ULONG& PeriodicTimerIntervalInSec
        )
        {
            PeriodicFlushTimeInSec = _MemoryThrottleLimits.PeriodicFlushTimeInSec;
            PeriodicTimerIntervalInSec = _MemoryThrottleLimits.PeriodicTimerIntervalInSec;
        }

        ULONG GetAllocationTimeoutInMs()
        {
            return(_MemoryThrottleLimits.AllocationTimeoutInMs);
        }

        KtlLogManager::MemoryThrottleLimits& GetMemoryThrottleLimits()
        {
            return(_MemoryThrottleLimits);
        }

        LONGLONG GetMaximumDestagingWriteOutstanding()
        {
            return(_MemoryThrottleLimits.MaximumDestagingWriteOutstanding);
        }

        ULONG GetSharedLogThrottleLimit()
        {
            return(_MemoryThrottleLimits.SharedLogThrottleLimit);
        }
        
        NTSTATUS MapSharedLogDefaultSettings(
            __inout KGuid& DiskId,
            __inout KString::CSPtr& Path,
            __inout KtlLogContainerId& LogId,
            __inout LONGLONG& LogSize,
            __inout ULONG& MaxAllowedStreams,
            __inout ULONG& MaxRecordSize,
            __inout DWORD& Flags
        );

#if DBG
        inline VOID IncrementPinMemoryFailureCount()
        {
            InterlockedIncrement(&_PinMemoryFailureCount);
        }

        inline LONG GetPinMemoryFailureCount()
        {
            return(_PinMemoryFailureCount);
        }
#endif
    public:
        inline LONGLONG IncrementPinnedIoBufferUsage(
            __in ULONG IoBufferElementSize
            )
        {
            LONGLONG pinnedIoBufferUsage = InterlockedAdd64(&_PinnedIoBufferUsage, (LONG)IoBufferElementSize);
            KInvariant(_PinnedIoBufferUsage >= 0);
            return(pinnedIoBufferUsage);
        }

        inline LONGLONG DecrementPinnedIoBufferUsage(
            __in ULONG IoBufferElementSize
            )
        {
            LONGLONG negativeSize = -1 * (LONG)IoBufferElementSize;
            LONGLONG pinnedIoBufferUsage = InterlockedAdd64(&_PinnedIoBufferUsage, negativeSize);
            KInvariant(_PinnedIoBufferUsage >= 0);
            return(pinnedIoBufferUsage);
        }

        inline LONGLONG GetPinnedIoBufferLimit()
        {
            return(_MemoryThrottleLimits.PinnedMemoryLimit);
        }

        inline LONGLONG GetPinnedIoBufferUsage()
        {
            return(_PinnedIoBufferUsage);
        }

        inline BOOLEAN IsPinnedIoBufferUsageOverLimit()
        {
            if (GetPinnedIoBufferLimit() == KtlLogManager::MemoryThrottleLimits::_NoLimit)
            {
                return(FALSE);
            }
            
            return(_PinnedIoBufferUsage >= GetPinnedIoBufferLimit());
        }

        inline KtlLogManager::AcceleratedFlushLimits* GetAcceleratedFlushLimits()
        {
            return(&_AcceleratedFlushLimits);
        }

#if defined(UDRIVER) || defined(UPASSTHROUGH)
        LONG IncrementUsageCount()
        {
            LONG count;

            count = InterlockedIncrement(&_UsageCount);
            return(count);
        }

        LONG DecrementUsageCount()
        {
            LONG count;

            count = InterlockedDecrement(&_UsageCount);
            return(count);
        }
#endif

#if defined(UPASSTHROUGH)
        NTSTATUS CreateOpenWaitContext(
            __out KAsyncEvent::WaitContext::SPtr & WaitContext
            );
#endif      
                
        RvdLogManager::SPtr GetBaseLogManager()
        {
            return(_BaseLogManager);
        }
        
    private:
        GlobalContainersTable _ContainersTable;
        RvdLogManager::SPtr _BaseLogManager;
        LONG _ActiveCount;
                
        OverlayLogFreeService::SPtr _LastOverlayLogFS;
        ServiceWrapper::Operation::SPtr _CloseOperation;
        NTSTATUS _FinalStatus;

        ThrottledKIoBufferAllocator::SPtr _ThrottledAllocator;
        KtlLogManager::MemoryThrottleLimits _MemoryThrottleLimits;
        KtlLogManager::SharedLogContainerSettings _SharedLogContainerSettings;
        KtlLogManager::AcceleratedFlushLimits _AcceleratedFlushLimits;
        
        volatile LONGLONG _PinnedIoBufferUsage;
#if DBG
        volatile LONG _PinMemoryFailureCount;
#endif

#if defined(UDRIVER) || defined(UPASSTHROUGH)
        volatile LONG _UsageCount;
#endif

#if defined(UPASSTHROUGH)
        KAsyncEvent _OpenWaitEvent;
#endif
        
};


