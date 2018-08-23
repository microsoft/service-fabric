/*++

Copyright (c) 2010  Microsoft Corporation

Module Name:

    kreadcache.h

Abstract:

    This file defines and implements the read cache for the RVD stream store.

Author:

    Norbert P. Kusters (norbertk) 9-Dec-2010

Environment:

Notes:

Revision History:

--*/

class KReadCache : public KObject<KReadCache>, public KShared<KReadCache> {

    K_FORCE_SHARED(KReadCache);

    NOFAIL
    KReadCache(
        __in ULONGLONG TotalCacheSize,
        __in ULONGLONG TargetBurstCacheSize,
        __in ULONG CorrelatedReferenceTimeoutInSeconds,
        __in ULONG AllocationTag
        );

    class CacheItem;

    public:

        static const ULONG DefaultCorrelatedReferenceTimeoutInSeconds = 0;

        class RegisterForEvictionContext : public KAsyncContextBase {

            K_FORCE_SHARED(RegisterForEvictionContext);

            public:

                KListEntry RegisterForEvictionListEntry;

            private:

                VOID
                Initialize(
                    );

                VOID
                InitializeForRegister(
                    __inout KReadCache& ReadCache,
                    __in const GUID& FileId,
                    __in ULONGLONG FileOffset
                    );

                VOID
                OnStart(
                    );

                VOID
                OnCancel(
                    );

                VOID
                OnReuse(
                    );

                friend class KReadCache;
                friend class CacheItem;

                KReadCache* _ReadCache;
                GUID _FileId;
                ULONGLONG _FileOffset;
                CacheItem* _Item;

        };

        static
        NTSTATUS
        Create(
            __in ULONGLONG TotalCacheSize,
            __in ULONGLONG TargetBurstCacheSize,
            __out KReadCache::SPtr& ReadCache,
            __in ULONG CorrelatedReferenceTimeoutInSeconds,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag = KTL_TAG_READ_CACHE
            );

        VOID
        SetCacheSize(
            __in ULONGLONG TotalCacheSize,
            __in ULONGLONG TargetBurstCacheSize
            );

        VOID
        QueryCacheSize(
            __out ULONGLONG& TotalCacheSize,
            __out ULONGLONG& TargetBurstCacheSize
            );

        VOID
        QueryCacheUsage(
            __out ULONGLONG& TotalCacheUsage,
            __out ULONGLONG& BurstCacheUsage
            );

        NTSTATUS
        Add(
            __in const GUID& FileId,
            __in ULONGLONG FileOffset,
            __inout KIoBufferElement& IoBufferElement
            );

        BOOLEAN
        Query(
            __in const GUID& FileId,
            __in ULONGLONG FileOffset,
            __out ULONGLONG& ActualFileOffset,
            __out KIoBufferElement::SPtr& IoBufferElement
            );

        BOOLEAN
        QueryEqualOrNext(
            __in const GUID& FileId,
            __in ULONGLONG FileOffset,
            __out ULONGLONG& CacheItemFileOffset
            );

        VOID
        RemoveRange(
            __in const GUID& FileId,
            __in ULONGLONG FileOffset,
            __in ULONGLONG Length
            );

        NTSTATUS
        AllocateRegisterForEviction(
            __out RegisterForEvictionContext::SPtr& Async,
            __in ULONG AllocationTag = KTL_TAG_READ_CACHE
            );

        VOID
        RegisterForEviction(
            __inout RegisterForEvictionContext& Async,
            __in const GUID& FileId,
            __in ULONGLONG FileOffset,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync = NULL
            );

        VOID
        Touch(
            __in RegisterForEvictionContext& Async
            );

    private:

        static const int LruKValue = 3;

        class CacheItem {

            K_DENY_COPY(CacheItem);

            public:

                ~CacheItem(
                    );

                NOFAIL
                CacheItem(
                    __in KReadCache& ReadCache,
                    __in const GUID& FileId,
                    __in ULONGLONG FileOffset,
                    __in KIoBufferElement& IoBufferElement
                    );

                NOFAIL
                CacheItem(
                    __in const GUID& FileId,
                    __in ULONGLONG FileOffset,
                    __in ULONGLONG Length
                    );

                VOID
                UpdateHistory(
                    __in BOOLEAN BypassCorrelatedReferenceCheck = FALSE
                    );

                VOID
                Insert(
                    );

                VOID
                Remove(
                    );

                VOID
                Touch(
                    );

                static
                LONG
                RangeCompare(
                    __in CacheItem& First,
                    __in CacheItem& Second
                    );

                static
                LONG
                LruKCompare(
                    __in CacheItem& First,
                    __in CacheItem& Second
                    );

                KTableEntry RangeTableEntry;
                KListEntry LruListEntry;
                KTableEntry LruKTableEntry;

            private:

                VOID
                Initialize(
                    __in KReadCache& ReadCache,
                    __in const GUID& FileId,
                    __in ULONGLONG FileOffset,
                    __in KIoBufferElement& IoBufferElement
                    );

                VOID
                Initialize(
                    __in const GUID& FileId,
                    __in ULONGLONG FileOffset,
                    __in ULONGLONG Length
                    );

                friend KReadCache;

                KReadCache* _ReadCache;
                GUID _FileId;
                ULONGLONG _FileOffset;
                ULONGLONG _Length;
                KIoBufferElement::SPtr _IoBufferElement;
                ULONGLONG _LastTouchTickCount;
                ULONGLONG _TouchHistory[LruKValue];
                ULONG _NextTouchIndex;
                BOOLEAN _InLru;
                KNodeList<RegisterForEvictionContext> _EvictionNotificationList;

        };

        typedef KNodeList<CacheItem> CacheItemList;

        typedef KNodeTable<CacheItem> CacheItemTable;

        VOID
        Initialize(
            __in ULONGLONG TotalCacheSize,
            __in ULONGLONG TargetBurstCacheSize,
            __in ULONG CorrelatedReferenceTimeoutInSeconds,
            __in ULONG AllocationTag
            );

        VOID
        AdjustCacheUsage(
            __inout CacheItemList& DeleteList
            );

        VOID
        DeleteCacheItems(
            __inout CacheItemList& DeleteList
            );

        VOID
        RemoveRangeHelper(
            __in const GUID& FileId,
            __in ULONGLONG FileOffset,
            __in ULONGLONG Length,
            __inout CacheItemList& DeleteList
            );

        KSpinLock _SpinLock;

        ULONGLONG _TotalCacheSize;
        ULONGLONG _TargetBurstCacheSize;
        ULONG _CorrelatedReferenceTimeoutInTicks;
        ULONG _AllocationTag;

        ULONGLONG _CurrentCacheSize;
        ULONGLONG _CurrentBurstCacheSize;
        ULONGLONG _CurrentSequenceNumber;

        CacheItemTable::CompareFunction _RangeCompare;
        CacheItemTable::CompareFunction _LruKCompare;

        CacheItemTable _RangeSorted;
        CacheItemList _LruList;
        CacheItemTable _LruKSorted;

};
