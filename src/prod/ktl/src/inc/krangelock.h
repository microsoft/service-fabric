/*++

Copyright (c) 2010  Microsoft Corporation

Module Name:

    krangelock.h

Abstract:

    This file defines a range lock object.

Author:

    Norbert P. Kusters (norbertk) 26-Dec-2010

Environment:

Notes:

Revision History:

--*/

class KRangeLock : public KObject<KRangeLock>, public KShared<KRangeLock> {

    K_FORCE_SHARED(KRangeLock);

    FAILABLE
    KRangeLock(
        __in ULONG AllocationTag
        );

    public:

        class AcquireContext : public KAsyncContextBase {

            K_FORCE_SHARED(AcquireContext);

            public:

                KTableEntry TableEntry;
                KListEntry ListEntry;

            private:

                VOID
                Initialize(
                    );

                VOID
                InitializeForAcquire(
                    __inout KRangeLock& RangeLock,
                    __in_opt const GUID* Id,
                    __in ULONGLONG Offset,
                    __in ULONGLONG Length
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

                static
                LONG
                Compare(
                    __in AcquireContext& First,
                    __in AcquireContext& Second
                    );

                VOID
                AcquireRange(
                    );

                VOID
                ReleaseRange(
                    );

                friend class KRangeLock;

                KRangeLock* _RangeLock;
                GUID _Id;
                ULONGLONG _Offset;
                ULONGLONG _LengthDesired;
                ULONGLONG _LengthAcquired;
                KNodeList<AcquireContext> _Waiters;
                AcquireContext::SPtr _InWaitList;

        };

        static
        NTSTATUS
        Create(
            __out KRangeLock::SPtr& RangeLock,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag = KTL_TAG_RANGE_LOCK
            );

        NTSTATUS
        AllocateAcquire(
            __out AcquireContext::SPtr& Async,
            __in ULONG AllocationTag = KTL_TAG_RANGE_LOCK
            );

        static
        NTSTATUS
        AllocateAcquireStatic(
            __out AcquireContext::SPtr& Async,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag = KTL_TAG_RANGE_LOCK
            );

        VOID
        AcquireRange(
            __inout AcquireContext& Async,
            __in ULONGLONG Offset,
            __in ULONGLONG Length,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync = NULL,
            __in_opt const GUID* Id = NULL
            );

        VOID
        ReleaseRange(
            __inout AcquireContext& Async
            );

        BOOLEAN
        AreLocksOutstanding(
            );

    private:

        typedef KNodeTable<AcquireContext> LocksTable;

        NTSTATUS
        Initialize(
            __in ULONG AllocationTag
            );

        KSpinLock _SpinLock;
        LocksTable::CompareFunction _Compare;
        LocksTable _LocksTable;
        AcquireContext::SPtr _SearchKey;

};
