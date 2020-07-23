// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include <ktl.h>
#include <KTrace.h>
#include "KtlLogDestagerImp.h"

#pragma region KtlLogDestagerImp::AsyncWriteContextImp implementation

const ULONG KtlLogDestagerImp::AsyncWriteContextImp::QueueLinkageOffset = FIELD_OFFSET(KtlLogDestagerImp::AsyncWriteContextImp, _QueueLinkage);
const ULONG KtlLogDestagerImp::AsyncWriteContextKey::AsnTableLinkageOffset = FIELD_OFFSET(KtlLogDestagerImp::AsyncWriteContextKey, _AsnTableLinkage);

NTSTATUS
KtlLogDestagerImp::AsyncWriteContextImp::Create(
    __out AsyncWriteContextImp::SPtr& Result,
    __in KtlLogDestagerImp& Destager,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )
{
    Result = _new(AllocationTag, Allocator) AsyncWriteContextImp(Destager, AllocationTag);
    if (!Result)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS status = Result->Status();
    if (!NT_SUCCESS(status))
    {
        Result.Reset();
    }

    return status;
}

KtlLogDestagerImp::AsyncWriteContextImp::AsyncWriteContextImp(
    __in KtlLogDestagerImp& Destager,
    __in ULONG AllocationTag
    ) : 
    _Destager(&Destager),
    _FlushTriggerEvent(TRUE, FALSE) // Manual reset event
{
    NTSTATUS status = STATUS_SUCCESS;

    status = _FlushTriggerEvent.CreateWaitContext(AllocationTag, GetThisAllocator(), _WaitForFlushTriggerContext);
    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    status = _Destager->_DestinationLogStream->CreateAsyncWriteContext(_AsyncLogStreamWriteContext);
    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    Zero();
}

KtlLogDestagerImp::AsyncWriteContextImp::~AsyncWriteContextImp()
{
}

VOID KtlLogDestagerImp::AsyncWriteContextImp::Zero()
{
    _EnqueueTime = 0;
    _EnqueueStatus = STATUS_PENDING;

    _FlushTriggerEvent.ResetEvent();

    _RecordAsn = 0;
    _Version = 0;
    _MetaDataBuffer.Reset();
    _IoBuffer.Reset();
    _ResourceCounter = nullptr;

    _MemoryUsage = 0;
}

VOID
KtlLogDestagerImp::AsyncWriteContextImp::StartWrite(
    __in RvdLogAsn RecordAsn,
    __in ULONGLONG Version,
    __in const KBuffer::SPtr& MetaDataBuffer,
    __in const KIoBuffer::SPtr& IoBuffer,
    __out_opt ResourceCounter* Counter,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _RecordAsn = RecordAsn;
    _Version = Version;
    _MetaDataBuffer = MetaDataBuffer;
    _IoBuffer = IoBuffer;
    _ResourceCounter = Counter;

    _MemoryUsage = (const_cast<KBuffer::SPtr&>(MetaDataBuffer).RawPtr() != nullptr) ? MetaDataBuffer->QuerySize() : 0;
    _MemoryUsage += (const_cast<KIoBuffer::SPtr&>(IoBuffer).RawPtr() != nullptr) ? IoBuffer->QuerySize() : 0;

    _EnqueueTime = 0;

    //
    // Add this async into the queue after all data fields are initialized.
    // The invariance is that any write context in the queue must contain valid data.
    //
    // The reason for not enqueueing in OnStart() is subtle. 
    // Suppose a user calls StartWrite(), then after StartWrite() returns, the caller calls StartClose().
    // It would expect the write to succeed. If the request is enqueued in OnStart(), it may be rejected 
    // with STATUS_FILE_CLOSED because the close async OnStart() has raced ahead of write async OnStart().
    // Strictly speaking, that behavior is still correct, but is counter-intuitive.
    //

    _EnqueueStatus = _Destager->AddWrite(*this);
    KInvariant(_EnqueueStatus != STATUS_PENDING);
    if (!NT_SUCCESS(_EnqueueStatus))
    {
        KTraceFailedAsyncRequest(_EnqueueStatus, this, _RecordAsn.Get(), _Version);
    }

    Start(ParentAsyncContext, CallbackPtr);
}

VOID KtlLogDestagerImp::AsyncWriteContextImp::OnStart()
{
    // If the enqueue operation has failed, fail this write request
    if (!NT_SUCCESS(_EnqueueStatus))
    {
        Complete(_EnqueueStatus);
        return;
    }

    // Wait for flush trigger
    _WaitForFlushTriggerContext->StartWaitUntilSet(
        this,   // ParentAsync
        KAsyncContextBase::CompletionCallback(this, &KtlLogDestagerImp::AsyncWriteContextImp::WaitForFlushTriggerCallback)
        );
}

//
// This method starts the process of flushing this log record to the destaged log stream.
// Completion of the log stream write will complete this async. 
// There is no separate completion callback for flush.
//
VOID KtlLogDestagerImp::AsyncWriteContextImp::StartFlush()
{
    _FlushTriggerEvent.SetEvent();
}

VOID
KtlLogDestagerImp::AsyncWriteContextImp::WaitForFlushTriggerCallback(
    __in KAsyncContextBase* const Parent, 
    __in KAsyncContextBase& CompletingSubOp
    )
{
    UNREFERENCED_PARAMETER(Parent);
    KAssert(this->IsInApartment());

    NTSTATUS status = CompletingSubOp.Status();
    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, _RecordAsn.Get(), _Version);
        Complete(status);
        return;
    }

    // Launch the log stream write
    KAssert(_AsyncLogStreamWriteContext.RawPtr() != nullptr);

    _AsyncLogStreamWriteContext->StartWrite(
        _RecordAsn, 
        _Version, 
        _MetaDataBuffer, 
        _IoBuffer, 
        this,       // ParentAsync
        KAsyncContextBase::CompletionCallback(this, &KtlLogDestagerImp::AsyncWriteContextImp::LogStreamWriteCallback)
        );
}

VOID
KtlLogDestagerImp::AsyncWriteContextImp::LogStreamWriteCallback(
    __in KAsyncContextBase* const Parent, 
    __in KAsyncContextBase& CompletingSubOp
    )
{
    UNREFERENCED_PARAMETER(Parent);
    KAssert(this->IsInApartment());

    NTSTATUS status = CompletingSubOp.Status();
    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, _RecordAsn.Get(), _Version);
    }

    Complete(status);
    return;
}

VOID KtlLogDestagerImp::AsyncWriteContextImp::OnCancel()
{
    if (_WaitForFlushTriggerContext)
    {
        _WaitForFlushTriggerContext->Cancel();
    }

    if (_AsyncLogStreamWriteContext)
    {
        _AsyncLogStreamWriteContext->Cancel();
    }
}

VOID KtlLogDestagerImp::AsyncWriteContextImp::OnCompleted()
{
    //
    // Remove this async from the internal queue.
    // Do this before releasing the buffers. 
    // The invariance is that any write context in the queue must contain valid data.
    //
    // This operation also removes the ref count on this async taken by the queue.
    // It breaks the ref count circle between this async and the destager.
    //
    if (NT_SUCCESS(_EnqueueStatus))
    {
        _Destager->RemoveWrite(*this);
        _EnqueueStatus = STATUS_PENDING;
    }

    // Fill in the current resource counter
    if (_ResourceCounter)
    {
        _Destager->QueryResourceCounter(*_ResourceCounter);
    }

    // Release all internal resources
    Zero();
}

VOID KtlLogDestagerImp::AsyncWriteContextImp::OnReuse()
{
    if (_WaitForFlushTriggerContext)
    {
        _WaitForFlushTriggerContext->Reuse();
    }

    if (_AsyncLogStreamWriteContext)
    {
        _AsyncLogStreamWriteContext->Reuse();
    }
}

//
// This method reads content of this log record.
//
VOID 
KtlLogDestagerImp::AsyncWriteContextImp::ReadCachedRecord(
    __out_opt ULONGLONG* const Version,
    __out KBuffer::SPtr& MetaDataBuffer,
    __out KIoBuffer::SPtr& IoBuffer
    )
{
    if (Version)
    {
        *Version = _Version;
    }

    MetaDataBuffer = _MetaDataBuffer;
    IoBuffer = _IoBuffer;
}

//
// Properties of this write request.
//

ULONGLONG
KtlLogDestagerImp::AsyncWriteContextImp::GetVersion()
{
    return _Version;
}

ULONG 
KtlLogDestagerImp::AsyncWriteContextImp::GetMemoryUsage()
{
    return _MemoryUsage;
}

LONGLONG 
KtlLogDestagerImp::AsyncWriteContextImp::GetEnqueueTime()
{
    return _EnqueueTime;
}

VOID 
KtlLogDestagerImp::AsyncWriteContextImp::SetEnqueueTime(
    __in LONGLONG CurrentTime
    )
{
    _EnqueueTime = CurrentTime;
}

#pragma endregion

#pragma region KtlLogDestagerImp::AsyncCloseContextImp implementation

NTSTATUS
KtlLogDestagerImp::AsyncCloseContextImp::Create(
    __out AsyncCloseContextImp::SPtr& Result,
    __in KtlLogDestagerImp& Destager,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )
{
    Result = _new(AllocationTag, Allocator) AsyncCloseContextImp(Destager, AllocationTag);
    if (!Result)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS status = Result->Status();
    if (!NT_SUCCESS(status))
    {
        Result.Reset();
    }

    return status;
}

KtlLogDestagerImp::AsyncCloseContextImp::AsyncCloseContextImp(
    __in KtlLogDestagerImp& Destager,
    __in ULONG AllocationTag
    ) : 
    _Destager(&Destager)
{
    NTSTATUS status = STATUS_SUCCESS;

    status = _Destager->CreateQueueEmptyWaitContext(AllocationTag, GetThisAllocator(), _WaitForQueueEmptyContext);
    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }
}

KtlLogDestagerImp::AsyncCloseContextImp::~AsyncCloseContextImp()
{

}

VOID
KtlLogDestagerImp::AsyncCloseContextImp::StartClose(
    __in BOOLEAN CancelIo,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    ) 
{
    _CancelIo = CancelIo;
    Start(ParentAsyncContext, CallbackPtr);
}

VOID KtlLogDestagerImp::AsyncCloseContextImp::OnStart()
{
    // Trigger rundown of the internal queue
    _Destager->StartClose(_CancelIo);

    // Wait until the queue becomes empty
    _WaitForQueueEmptyContext->StartWaitUntilSet(
        this,   // ParentAsync
        KAsyncContextBase::CompletionCallback(this, &KtlLogDestagerImp::AsyncCloseContextImp::WaitForQueueEmptyCallback)
        );
}

VOID KtlLogDestagerImp::AsyncCloseContextImp::OnCancel()
{
    if (_WaitForQueueEmptyContext)
    {
        _WaitForQueueEmptyContext->Cancel();
    }
}

VOID KtlLogDestagerImp::AsyncCloseContextImp::OnReuse()
{
    if (_WaitForQueueEmptyContext)
    {
        _WaitForQueueEmptyContext->Reuse();
    }
}

VOID
KtlLogDestagerImp::AsyncCloseContextImp::WaitForQueueEmptyCallback(
    __in KAsyncContextBase* const Parent, 
    __in KAsyncContextBase& CompletingSubOp
    )
{
    UNREFERENCED_PARAMETER(Parent);
    KAssert(this->IsInApartment());

    NTSTATUS status = CompletingSubOp.Status();

    if (NT_SUCCESS(status))
    {
        KAssert(_Destager->IsClosed());
    }
    else
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
    }

    Complete(status);
    return;
}

#pragma endregion

#pragma region KtlLogDestagerImp implementation

NTSTATUS
KtlLogDestagerImp::Create(
    __out KtlLogDestager::SPtr& Result,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag,
    __in RvdLogStream& DestinationLogStream
    )
{
    Result = _new(AllocationTag, Allocator) KtlLogDestagerImp(DestinationLogStream);
    if (!Result)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS status = Result->Status();
    if (!NT_SUCCESS(status))
    {
        Result.Reset();
    }

    return status;
}

KtlLogDestagerImp::KtlLogDestagerImp(
    __in RvdLogStream& DestinationLogStream
    ) :
    _DestinationLogStream(&DestinationLogStream),
    _PendingWriteQueue(AsyncWriteContextImp::QueueLinkageOffset),
    _WritesSortedByAsn(AsyncWriteContextImp::AsnTableLinkageOffset, &KtlLogDestagerImp::AsyncWriteContextKey::Compare),
    _QueueEmptyEvent(TRUE, FALSE),  // Manual reset event
    _IsClosed(FALSE)
{
}

KtlLogDestagerImp::~KtlLogDestagerImp()
{
#if DBG
    KAssert(_WritesSortedByAsn.Count() == 0);
    KAssert(_PendingWriteQueue.IsEmpty());

    ResourceCounter zeroCounter;
    KAssert(memcmp(&_ResourceCounter, &zeroCounter, sizeof(ResourceCounter)) == 0);
#endif
}

//
// This method allocates an AsyncWriteContext.
//

NTSTATUS
KtlLogDestagerImp::CreateAsyncWriteContext(
    __out AsyncWriteContext::SPtr& Result,
    __in ULONG AllocationTag
    ) 
{
    Result.Reset();

    AsyncWriteContextImp::SPtr context;
    NTSTATUS status = AsyncWriteContextImp::Create(context, *this, GetThisAllocator(), AllocationTag);
    if (!NT_SUCCESS(status))
    {
        return status;
    }
    else
    {
        Result.Attach(context.Detach());
        return STATUS_SUCCESS;
    }
}

//
// This method allocates an AsyncCloseContext.
//

NTSTATUS
KtlLogDestagerImp::CreateAsyncCloseContext(
    __out AsyncCloseContext::SPtr& Result,
    __in ULONG AllocationTag
    ) 
{
    Result.Reset();

    AsyncCloseContextImp::SPtr context;
    NTSTATUS status = AsyncCloseContextImp::Create(context, *this, GetThisAllocator(), AllocationTag);
    if (!NT_SUCCESS(status))
    {
        return status;
    }
    else
    {
        Result.Attach(context.Detach());
        return STATUS_SUCCESS;
    }
}

//
// Read methods.
// These methods synchronously read a log record from the internal queue.
//

NTSTATUS
KtlLogDestagerImp::Read(
    __in RvdLogAsn RecordAsn,
    __out_opt ULONGLONG* const Version,
    __out KBuffer::SPtr& MetaDataBuffer,
    __out KIoBuffer::SPtr& IoBuffer
    ) 
{
    NTSTATUS status = STATUS_SUCCESS;
    AsyncWriteContextKey key(RecordAsn);

    K_LOCK_BLOCK(_PendingdWriteLock)
    {
        AsyncWriteContextImp* writeContext = _WritesSortedByAsn.Lookup(key);
        if (writeContext)
        {
            writeContext->ReadCachedRecord(Version, MetaDataBuffer, IoBuffer);
        }
        else
        {
            status = STATUS_NOT_FOUND;
        }
    }

    return status;
}

NTSTATUS
KtlLogDestagerImp::ReadPrevious(
    __in RvdLogAsn NextRecordAsn,
    __out_opt RvdLogAsn* RecordAsn,
    __out_opt ULONGLONG* const Version,
    __out KBuffer::SPtr& MetaDataBuffer,
    __out KIoBuffer::SPtr& IoBuffer
    ) 
{
    NTSTATUS status = STATUS_SUCCESS;
    AsyncWriteContextKey key(NextRecordAsn);

    K_LOCK_BLOCK(_PendingdWriteLock)
    {
        AsyncWriteContextImp* writeContext = _WritesSortedByAsn.Lookup(key);
        if (!writeContext)
        {
            status = STATUS_NO_MATCH;
            break;
        }

        writeContext = _WritesSortedByAsn.Previous(*writeContext);
        if (writeContext)
        {
            if (RecordAsn)
            {
                *RecordAsn = writeContext->GetAsn();
            }

            writeContext->ReadCachedRecord(Version, MetaDataBuffer, IoBuffer);
        }
        else
        {
            status = STATUS_NOT_FOUND;
        }
    }

    return status;
}

NTSTATUS
KtlLogDestagerImp::ReadNext(
    __in RvdLogAsn PreviousRecordAsn,
    __out_opt RvdLogAsn* RecordAsn,
    __out_opt ULONGLONG* const Version,
    __out KBuffer::SPtr& MetaDataBuffer,
    __out KIoBuffer::SPtr& IoBuffer
    ) 
{
    NTSTATUS status = STATUS_SUCCESS;
    AsyncWriteContextKey key(PreviousRecordAsn);

    K_LOCK_BLOCK(_PendingdWriteLock)
    {
        AsyncWriteContextImp* writeContext = _WritesSortedByAsn.Lookup(key);
        if (!writeContext)
        {
            status = STATUS_NO_MATCH;
            break;
        }

        writeContext = _WritesSortedByAsn.Next(*writeContext);
        if (writeContext)
        {
            if (RecordAsn)
            {
                *RecordAsn = writeContext->GetAsn();
            }

            writeContext->ReadCachedRecord(Version, MetaDataBuffer, IoBuffer);
        }
        else
        {
            status = STATUS_NOT_FOUND;
        }
    }

    return status;
}

NTSTATUS
KtlLogDestagerImp::ReadContaining(
    __in RvdLogAsn SpecifiedRecordAsn,
    __out_opt RvdLogAsn* RecordAsn,
    __out_opt ULONGLONG* const Version,
    __out KBuffer::SPtr& MetaDataBuffer,
    __out KIoBuffer::SPtr& IoBuffer
    ) 
{
    NTSTATUS status = STATUS_SUCCESS;
    AsyncWriteContextKey key(SpecifiedRecordAsn);

    K_LOCK_BLOCK(_PendingdWriteLock)
    {
        AsyncWriteContextImp* writeContext = _WritesSortedByAsn.LookupEqualOrPrevious(key);
        if (writeContext)
        {
            if (RecordAsn)
            {
                *RecordAsn = writeContext->GetAsn();
            }

            writeContext->ReadCachedRecord(Version, MetaDataBuffer, IoBuffer);
        }
        else
        {
            status = STATUS_NOT_FOUND;
        }
    }

    return status;
}

//
// This method drops all log records whose Asn's are less than or equal to PurgePoint from the destaging queue.
// All outstanding writes for the purged log records are also cancelled.
//

VOID 
KtlLogDestagerImp::PurgeQueuedWrites(
    __in RvdLogAsn PurgePoint
    )
{
    K_LOCK_BLOCK(_PendingdWriteLock)
    {
        AsyncWriteContextImp* writeContext = _WritesSortedByAsn.First();
        while (writeContext)
        {
            if (writeContext->GetAsn() > PurgePoint)
            {
                break;
            }

            writeContext->Cancel();
            writeContext = _WritesSortedByAsn.Next(*writeContext);
        }
    }
}

//
// This method returns the current resource counter.
//

VOID
KtlLogDestagerImp::QueryResourceCounter(
    __out ResourceCounter& Counter
    )
{
    K_LOCK_BLOCK(_PendingdWriteLock)
    {
        Counter = _ResourceCounter;
    }
}

//
// The following methods are mechanisms to flush queued log records to the dedicated log stream.
// It is up to the external caller to decide when to call these methods and how much to flush.
// Each queued log record write will eventually complete as its AsyncWriteContext completes.
// There is no separate notification mechanism for flush completion.
//

VOID 
KtlLogDestagerImp::FlushByTime(
    __in LONG MaxQueueingTimeInMs
    )
{
    LONGLONG flushTime = KNt::GetTickCount64() - MaxQueueingTimeInMs;

    K_LOCK_BLOCK(_PendingdWriteLock)
    {
        // Walk the queue from the oldest to the newest. Flush all writes that are older than flushTime.
        AsyncWriteContextImp* writeContext = _PendingWriteQueue.PeekHead();
        while (writeContext)
        {
            if (writeContext->GetEnqueueTime() > flushTime)
            {
                break;
            }
                
            writeContext->StartFlush();
            writeContext = _PendingWriteQueue.Successor(writeContext);
        }
    }
}

VOID 
KtlLogDestagerImp::FlushByMemory(
    __in LONGLONG TargetLocalMemoryUsage
    )
{
    K_LOCK_BLOCK(_PendingdWriteLock)
    {
        const LONGLONG memoryToRelease = _ResourceCounter.LocalMemoryUsage - TargetLocalMemoryUsage;
        LONGLONG memoryReleased = 0;

        AsyncWriteContextImp* writeContext = _WritesSortedByAsn.First();
        while (writeContext)
        {
            if (memoryReleased >= memoryToRelease)
            {
                break;
            }

            writeContext->StartFlush();
            memoryReleased += writeContext->GetMemoryUsage();

            writeContext = _WritesSortedByAsn.Next(*writeContext);
        }
    }
}

VOID 
KtlLogDestagerImp::FlushByAsn(
    __in RvdLogAsn FlushToRecordAsn
    ) 
{
    K_LOCK_BLOCK(_PendingdWriteLock)
    {
        AsyncWriteContextImp* writeContext = _WritesSortedByAsn.First();
        while (writeContext)
        {
            if (writeContext->GetAsn() > FlushToRecordAsn)
            {
                break;
            }

            writeContext->StartFlush();
            writeContext = _WritesSortedByAsn.Next(*writeContext);
        }
    }
}

NTSTATUS 
KtlLogDestagerImp::AddWrite(
    __in AsyncWriteContextImp& WriteContext
    )
{
    K_LOCK_BLOCK(_PendingdWriteLock)
    {
        // Once the rundown has started, reject any new write request
        if (_IsClosed)
        {
            return STATUS_FILE_CLOSED;
        }

        // Look for another record of the same Asn in the cache
        AsyncWriteContextKey key(WriteContext.GetAsn());
        AsyncWriteContextImp* existingRecord = _WritesSortedByAsn.Lookup(key);
        if (existingRecord)
        {
            if (existingRecord->GetVersion() < WriteContext.GetVersion())
            {
                //
                // The existing request is stale. Remove it from the index and cancel it.
                // The stale write request will stay in the queue until it completes,
                // but it won't be eligible for any read because it has been superceded.
                //
                _WritesSortedByAsn.Remove(*existingRecord);
                existingRecord->Cancel();
            }
            else 
            {
                // The new write request is stale. Fail it immediately.
                return STATUS_OBJECT_NAME_COLLISION;
            }
        }

        //
        // Add the write async to the queue tail.
        // All write async's in the queue are sorted by their enqueue times.
        //
        WriteContext.SetEnqueueTime(KNt::GetTickCount64());
        _PendingWriteQueue.AppendTail(&WriteContext);

        // Index the write async by its Asn
        _WritesSortedByAsn.Insert(WriteContext);

        // Adjust resource counters
        _ResourceCounter.LocalMemoryUsage += WriteContext.GetMemoryUsage();
        _ResourceCounter.PendingWriteCount++;
    }

    return STATUS_SUCCESS;
}

VOID 
KtlLogDestagerImp::RemoveWrite(
    __in AsyncWriteContextImp& WriteContext
    )
{
    BOOLEAN setQueueEmptyEvent = FALSE;

    K_LOCK_BLOCK(_PendingdWriteLock)
    {
        //
        // It is possible that this write context has been superceded by another higher versioned write request.
        // Therefore it is no longer in the Asn-index table. Make allowance for that.
        //
        if (WriteContext.IsInAsnTable())
        {
            _WritesSortedByAsn.Remove(WriteContext);
        }

        // Remove the request from the queue. 
        AsyncWriteContextImp::SPtr context = _PendingWriteQueue.Remove(&WriteContext);
        KAssert(context.RawPtr() != nullptr);

        // Adjust resource counters
        KAssert(_ResourceCounter.LocalMemoryUsage >= WriteContext.GetMemoryUsage());
        _ResourceCounter.LocalMemoryUsage -= WriteContext.GetMemoryUsage();

        KAssert(_ResourceCounter.PendingWriteCount > 0);
        _ResourceCounter.PendingWriteCount--;

        if (_IsClosed && _PendingWriteQueue.IsEmpty() && !_QueueEmptyEvent.IsSignaled())
        {
            setQueueEmptyEvent = TRUE;
        }
    }

    if (setQueueEmptyEvent)
    {
        // Drop reference to the dedicated log stream
        _DestinationLogStream.Reset();

        // Signal pending close event
        _QueueEmptyEvent.SetEvent();
    }
}

VOID 
KtlLogDestagerImp::StartClose(
    __in BOOLEAN CancelIo
    )
{
    K_LOCK_BLOCK(_PendingdWriteLock)
    {
        if (_IsClosed == TRUE)
        {
            return;
        }

        if (CancelIo)
        {
            AsyncWriteContextImp* writeContext = _PendingWriteQueue.PeekHead();
            while (writeContext)
            {
                writeContext->Cancel();
                writeContext = _PendingWriteQueue.Successor(writeContext);
            }
        }
        else
        {
            AsyncWriteContextImp* writeContext = _PendingWriteQueue.PeekHead();
            while (writeContext)
            {
                writeContext->StartFlush();
                writeContext = _PendingWriteQueue.Successor(writeContext);
            }
        }

        _IsClosed = TRUE;
    }
}

NTSTATUS
KtlLogDestagerImp::CreateQueueEmptyWaitContext(
    __in ULONG AllocationTag,
    __in KAllocator& Allocator,
    __out KAsyncEvent::WaitContext::SPtr& Context
    )
{
    return _QueueEmptyEvent.CreateWaitContext(AllocationTag, Allocator, Context);
}

BOOLEAN
KtlLogDestagerImp::IsClosed()
{
    K_LOCK_BLOCK(_PendingdWriteLock)
    {
        if (!_IsClosed)
        {
            return FALSE;
        }

        if (!_PendingWriteQueue.IsEmpty() || _WritesSortedByAsn.Count())
        {
            return FALSE;
        }

        if (_ResourceCounter.LocalMemoryUsage != 0 || _ResourceCounter.PendingWriteCount != 0)
        {
            return FALSE;
        }
    }

    return TRUE;
}

#pragma endregion
