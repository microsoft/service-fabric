/*++

Copyright (c) 2011  Microsoft Corporation

Module Name:

    kresourcelock.h

Abstract:

    This file defines a resource lock class.

Author:

    Norbert P. Kusters (norbertk) 18-Nov-2011

Environment:

    Kernel mode and User mode

Notes:

--*/

#pragma once

class KResourceLock : public KObject<KResourceLock>, public KShared<KResourceLock> {

    K_FORCE_SHARED(KResourceLock);

    public:

        class AcquireContext : public KAsyncContextBase {

            K_FORCE_SHARED(AcquireContext);

            public:

            private:

                VOID
                InitializeForAcquire(
                    __inout KResourceLock& ResourceLock,
                    __in BOOLEAN IsExclusiveAcquire
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

                BOOLEAN
                DoComplete(
                    __in NTSTATUS Status
                    );

                friend class KResourceLock;

                KListEntry ListEntry;

                KResourceLock::SPtr _ResourceLock;
                BOOLEAN _IsExclusiveAcquire;

        };

        static
        NTSTATUS
        Create(
            __out KResourceLock::SPtr& ResourceLock,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag = KTL_TAG_RESOURCE_LOCK
            );

        NTSTATUS
        AllocateAcquire(
            __out AcquireContext::SPtr& Async,
            __in ULONG AllocationTag = KTL_TAG_RESOURCE_LOCK
            );

        VOID
        AcquireShared(
            __in AcquireContext& Async,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync = NULL
            );

        BOOLEAN
        TryAcquireShared(
            );

        VOID
        AcquireExclusive(
            __in AcquireContext& Async,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync = NULL
            );

        VOID
        ReleaseShared(
            );

        VOID
        ReleaseExclusive(
            );

    private:

        KSpinLock _SpinLock;
        LONG _SharedRefCount;
        LONG _ExclusiveAcquireInProgress;
        AcquireContext* _ExclusiveWaiter;
        KNodeList<AcquireContext> _Waiters;

};
