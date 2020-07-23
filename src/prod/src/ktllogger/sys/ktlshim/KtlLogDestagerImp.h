// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "KtlLogDestager.h"

class KtlLogDestagerImp : public KtlLogDestager
{
    K_FORCE_SHARED(KtlLogDestagerImp);

public:

    static NTSTATUS
    Create(
        __out KtlLogDestager::SPtr& Result,
        __in KAllocator& Allocator,
        __in ULONG AllocationTag,
        __in RvdLogStream& DestinationLogStream
        );

    //
    // This method allocates an AsyncWriteContext.
    //

    virtual NTSTATUS
    CreateAsyncWriteContext(
        __out AsyncWriteContext::SPtr& Result,
        __in ULONG AllocationTag
        ) override;

    //
    // This method allocates an AsyncCloseContext.
    //

    virtual NTSTATUS
    CreateAsyncCloseContext(
        __out AsyncCloseContext::SPtr& Result,
        __in ULONG AllocationTag
        ) override;

    //
    // Read methods.
    // These methods synchronously read a log record from the internal queue.
    //

    virtual NTSTATUS
    Read(
        __in RvdLogAsn RecordAsn,
        __out_opt ULONGLONG* const Version,
        __out KBuffer::SPtr& MetaDataBuffer,
        __out KIoBuffer::SPtr& IoBuffer
        ) override;

    virtual NTSTATUS
    ReadPrevious(
        __in RvdLogAsn NextRecordAsn,
        __out_opt RvdLogAsn* RecordAsn,
        __out_opt ULONGLONG* const Version,
        __out KBuffer::SPtr& MetaDataBuffer,
        __out KIoBuffer::SPtr& IoBuffer
        ) override;

    virtual NTSTATUS
    ReadNext(
        __in RvdLogAsn PreviousRecordAsn,
        __out_opt RvdLogAsn* RecordAsn,
        __out_opt ULONGLONG* const Version,
        __out KBuffer::SPtr& MetaDataBuffer,
        __out KIoBuffer::SPtr& IoBuffer
        ) override;

    virtual NTSTATUS
    ReadContaining(
        __in RvdLogAsn SpecifiedRecordAsn,
        __out_opt RvdLogAsn* RecordAsn,
        __out_opt ULONGLONG* const Version,
        __out KBuffer::SPtr& MetaDataBuffer,
        __out KIoBuffer::SPtr& IoBuffer
        ) override;

    //
    // This method drops all log records whose Asn's are less than or equal to PurgePoint from the destaging queue.
    // All outstanding writes for the purged log records are also cancelled.
    //

    virtual VOID 
    PurgeQueuedWrites(
        __in RvdLogAsn PurgePoint
        ) override;

    //
    // This method returns the current resource counter.
    //

    virtual VOID
    QueryResourceCounter(
        __out ResourceCounter& Counter
        ) override;

    //
    // The following methods are mechanisms to flush queued log records to the dedicated log stream.
    // It is up to the external caller to decide when to call these methods and how much to flush.
    // Each queued log record write will eventually complete as its AsyncWriteContext completes.
    // There is no separate notification mechanism for flush completion.
    //
    virtual VOID FlushByTime(__in LONG MaxQueueingTimeInMs) override;

    virtual VOID FlushByMemory(__in LONGLONG TargetLocalMemoryUsage) override;

    virtual VOID FlushByAsn(__in RvdLogAsn FlushToRecordAsn) override;

private:

    class AsyncWriteContextKey;
    class AsyncWriteContextImp;
    class AsyncCloseContextImp;

    KtlLogDestagerImp::KtlLogDestagerImp(
        __in RvdLogStream& DestinationLogStream
        );

    NTSTATUS 
    AddWrite(
        __in AsyncWriteContextImp& WriteContext
        );

    VOID 
    RemoveWrite(
        __in AsyncWriteContextImp& WriteContext
        );

    VOID 
    StartClose(
        __in BOOLEAN CancelIo
        );

    NTSTATUS
    CreateQueueEmptyWaitContext(
        __in ULONG AllocationTag,
        __in KAllocator& Allocator,
        __out KAsyncEvent::WaitContext::SPtr& Context
        );

    BOOLEAN IsClosed();

    //
    // The destaging log stream.
    //
    RvdLogStream::SPtr _DestinationLogStream;

    //
    // A queue of all pending writes. New writes are always inserted to the end of the queue.
    // Therefore, all writes are ordered by their enqueue times.
    //
    KNodeListShared<AsyncWriteContextImp> _PendingWriteQueue;

    //
    // An index tree built over pending writes in _PendingWriteQueue.
    // Items are ordered by their Asn's.
    //
    KNodeTable<AsyncWriteContextImp, AsyncWriteContextKey> _WritesSortedByAsn;

    //
    // This event is signalled when the destager is closed and the queue becomes empty.
    //
    KAsyncEvent _QueueEmptyEvent;

    //
    // This flag indicates if this destager has been instructed to close.
    //
    BOOLEAN _IsClosed;

    //
    // The current resource counters.
    //
    ResourceCounter _ResourceCounter;

    //
    // This lock protected _PendingWriteQueue, _WritesSortedByAsn, _IsClosed and _ResourceCounter.
    //
    KSpinLock _PendingdWriteLock;
};

//
// Search key for indexing write requests by Asn
//
class KtlLogDestagerImp::AsyncWriteContextKey
{
public:

    AsyncWriteContextKey()
    {
        _RecordAsn = 0;
    }

    AsyncWriteContextKey(__in RvdLogAsn RecordAsn)
    {
        _RecordAsn = RecordAsn;
    }

    RvdLogAsn GetAsn()
    {
        return _RecordAsn;
    }

    static LONG
    Compare(
        __in AsyncWriteContextKey& First, 
        __in AsyncWriteContextKey& Second
        )
    {
        if (First.GetAsn() < Second.GetAsn())
        {
            return -1;
        }
        else if (First.GetAsn() > Second.GetAsn())
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }

    BOOLEAN IsInAsnTable()
    {
        return _AsnTableLinkage.IsInserted();
    }

    static const ULONG AsnTableLinkageOffset;

protected:

    RvdLogAsn _RecordAsn;
    KTableEntry _AsnTableLinkage;
};

//
// Implementation of AsyncWriteContext.
// AsyncWriteContextImp represents a pending write request. 
// Once started, it stays in the internal queue until it completes.
// It may also be in the Asn index table if it has not been superceded by a higher version.
//

class KtlLogDestagerImp::AsyncWriteContextImp : public AsyncWriteContext, public AsyncWriteContextKey
{
    K_FORCE_SHARED(AsyncWriteContextImp);

public:

    static NTSTATUS
    Create(
        __out AsyncWriteContextImp::SPtr& Result,
        __in KtlLogDestagerImp& Destager,
        __in KAllocator& Allocator,
        __in ULONG AllocationTag = KTL_TAG_LOGGER
        );

    virtual VOID
    StartWrite(
        __in RvdLogAsn RecordAsn,
        __in ULONGLONG Version,
        __in const KBuffer::SPtr& MetaDataBuffer,
        __in const KIoBuffer::SPtr& IoBuffer,
        __out_opt ResourceCounter* Counter,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
        ) override;

    //
    // This method reads content of this log record.
    //
    VOID 
    ReadCachedRecord(
        __out_opt ULONGLONG* const Version,
        __out KBuffer::SPtr& MetaDataBuffer,
        __out KIoBuffer::SPtr& IoBuffer
        );

    //
    // This method starts the process of flushing this log record to the destaged log stream.
    // Completion of the log stream write will complete this async. 
    // There is no separate completion callback for flush.
    //
    VOID StartFlush();

    //
    // Properties of this write request.
    //

    ULONGLONG GetVersion();
    ULONG GetMemoryUsage();

    LONGLONG GetEnqueueTime();
    VOID SetEnqueueTime(__in LONGLONG CurrentTime);

    static const ULONG QueueLinkageOffset;

private:

    AsyncWriteContextImp(
        __in KtlLogDestagerImp& Destager,
        __in ULONG AllocationTag
        );

    VOID Zero();

    VOID OnStart() override;
    VOID OnCancel() override;
    VOID OnCompleted() override;
    VOID OnReuse() override;

    VOID
    WaitForFlushTriggerCallback(
        __in KAsyncContextBase* const Parent, 
        __in KAsyncContextBase& CompletingSubOp
        );

    VOID
    LogStreamWriteCallback(
        __in KAsyncContextBase* const Parent, 
        __in KAsyncContextBase& CompletingSubOp
        );

    // Various linkages
    KListEntry _QueueLinkage;

    // SPtr to the destager object.
    KtlLogDestagerImp::SPtr _Destager;

    // This event is signalled when StartFlush() is called on this async.
    KAsyncEvent _FlushTriggerEvent;
    
    // The time when this async is put on the internal queue.
    LONGLONG _EnqueueTime;

    // The status of inserting this request into the internal queue.
    NTSTATUS _EnqueueStatus;

    // Async context to wait on _FlushTriggerEvent.
    KAsyncEvent::WaitContext::SPtr _WaitForFlushTriggerContext;

    // Async context to perform actual log stream write operation.
    RvdLogStream::AsyncWriteContext::SPtr _AsyncLogStreamWriteContext;

    // StartWrite() parameters
    ULONGLONG _Version;
    KBuffer::SPtr _MetaDataBuffer;
    KIoBuffer::SPtr _IoBuffer;
    ResourceCounter* _ResourceCounter;

    // Total memory usage of this log write request
    ULONG _MemoryUsage;
};

class KtlLogDestagerImp::AsyncCloseContextImp : public AsyncCloseContext
{
    K_FORCE_SHARED(AsyncCloseContextImp);

public:

    static NTSTATUS
    Create(
        __out AsyncCloseContextImp::SPtr& Result,
        __in KtlLogDestagerImp& Destager,
        __in KAllocator& Allocator,
        __in ULONG AllocationTag = KTL_TAG_LOGGER
        );

    virtual VOID
    StartClose(
        __in BOOLEAN CancelIo,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
        ) override;

private:

    AsyncCloseContextImp(
        __in KtlLogDestagerImp& Destager,
        __in ULONG AllocationTag
        );

    VOID
    WaitForQueueEmptyCallback(
        __in KAsyncContextBase* const Parent, 
        __in KAsyncContextBase& CompletingSubOp
        );

    VOID OnStart() override;
    VOID OnCancel() override;
    VOID OnReuse() override;

    KtlLogDestagerImp::SPtr _Destager;

    KAsyncEvent::WaitContext::SPtr _WaitForQueueEmptyContext;
    BOOLEAN _CancelIo;
};
