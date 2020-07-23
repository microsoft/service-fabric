/*++

Copyright (c) Microsoft Corporation

Module Name:

    RvdLoggerTestHelpers.cpp

Abstract:

    This file contains test helper functions

--*/
#pragma once
#include "RvdLogStreamAsyncIOTests.h"

#if (CONSOLE_TEST) || (DISPLAY_ON_CONSOLE)
#undef KDbgPrintf
#define KDbgPrintf(...)     printf(__VA_ARGS__)
#endif

//** LogStreamIoContext class implementation
LogStreamIoContext::LogStreamIoContext(
    __in RvdLogAsn const& RecordAsn,
    __in ULONGLONG const& Version)
{
    _RecordAsn = RecordAsn;
    _Version = Version;
    _IoBuffer = nullptr;
    _Metadata = nullptr;
    _IoBufferSize = 0;
    _MetadataSize = 0;

    SetConstructorStatus(STATUS_SUCCESS);
}

LogStreamIoContext::~LogStreamIoContext()
{
}

LogStreamIoContext::SPtr
LogStreamIoContext::Create(
    __in RvdLogAsn const& RecordAsn,
    __in ULONGLONG const& Version)
{
    return SPtr(_new(KTL_TAG_TEST,
                KtlSystem::GlobalNonPagedAllocator())
                LogStreamIoContext(RecordAsn, Version));
}

VOID
LogStreamIoContext::Reset()
{
    _RecordAsn = 0;
    _Version = 0;
    _IoBuffer = nullptr;
    _Metadata = nullptr;
    _IoBufferSize = 0;
    _MetadataSize = 0;
}

VOID
LogStreamIoContext::SetMetadataSize(__in const ULONG MetadataSize)
{
    _MetadataSize = MetadataSize;
}

VOID
LogStreamIoContext::SetIoBufferSize(__in const ULONG IoBufferSize)
{
    _IoBufferSize = IoBufferSize;
}

NTSTATUS
LogStreamIoContext::AllocateBuffers()
{
    NTSTATUS        status = STATUS_SUCCESS;
    KBuffer::SPtr   metadataBuffer;
    KIoBuffer::SPtr ioBuffer;

    status = KBuffer::Create(_MetadataSize, metadataBuffer, KtlSystem::GlobalNonPagedAllocator(), KTL_TAG_TEST);
    if(!NT_SUCCESS(status))
    {
        KDbgCheckpointWStatus((ULONG)-1, "KBuffer::Create() failed", status);
        return status;
    }

    void* ioBufferPtr = nullptr;
    if (_IoBufferSize > 0)
    {
        status = KIoBuffer::CreateSimple(_IoBufferSize, ioBuffer, ioBufferPtr, KtlSystem::GlobalNonPagedAllocator(), KTL_TAG_TEST);
    }
    else
    {
        status = KIoBuffer::CreateEmpty(ioBuffer, KtlSystem::GlobalNonPagedAllocator(), KTL_TAG_TEST);
    }
    if(!NT_SUCCESS(status))
    {
        KDbgCheckpointWStatus((ULONG)-1, "KIoBuffer::Create() failed", status);
        return status;
    }
    KInvariant(ioBuffer->QueryNumberOfIoBufferElements() <= 1);

    // Now fill the buffers with a repeated ASN, Version, and Type value
    ULONGLONG   fillValue = _RecordAsn.Get() | (_Version << 16);

    if (_MetadataSize > 0)
    {
        RepStoreULongLong((UCHAR*)(metadataBuffer->GetBuffer()), fillValue, _MetadataSize);
    }

    if (_IoBufferSize > 0)
    {
        fillValue |= (ULONGLONG)1 << 63;
        RepStoreULongLong((UCHAR*)ioBufferPtr, fillValue, _IoBufferSize);
    }

    _IoBuffer = Ktl::Move(ioBuffer);
    _Metadata = Ktl::Move(metadataBuffer);

    return status;
}

BOOLEAN
LogStreamIoContext::ValidateRecordContents(
    RvdLogAsn Asn,
    ULONGLONG Version,
    void* Metadata,
    ULONG MetadataSize,
    void* IoBuffer,
    ULONG IoBufferSize)
{
    ULONGLONG   expectedFillValue = Asn.Get() | (Version << 16);
    if (!RepScanULongLong((UCHAR*)Metadata, expectedFillValue, MetadataSize))
    {
        KDbgCheckpoint((ULONG)-1, "Record Metadata does not match");
        return FALSE;
    }

    expectedFillValue |= (ULONGLONG)1 << 63;
    if (!RepScanULongLong((UCHAR*)IoBuffer, expectedFillValue, IoBufferSize))
    {
        KDbgCheckpoint((ULONG)-1, "Record IoBuffer does not match");
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
LogStreamIoContext::ValidateBuffers()
{
    if (_ReadVersion < _Version)
    {
        KDbgCheckpoint((ULONG)-1, "Record versions do not match");
        return FALSE;
    }

    else if (_ReadVersion == _Version)
    {
        if (_IoBuffer->QuerySize() != _IoBufferSize)
        {
            KDbgCheckpoint((ULONG)-1, "Record IoBuffer sizes do not match");
            return FALSE;
        }

        if (_Metadata->QuerySize() != _MetadataSize)
        {
            KDbgCheckpoint((ULONG)-1, "Record Metadata sizes do not match");
            return FALSE;
        }
    }
    else
    {
        KAssert(_ReadVersion > _Version);
        KDbgPrintf(
            "Warning: ASN %I64u has higher version (%I64u) than that (%I64u) written\n",
            _RecordAsn.Get(),
            _ReadVersion,
            _Version);
    }

    ULONGLONG   expectedFillValue = _RecordAsn.Get() | (_ReadVersion << 16);

    if (!RepScanULongLong((UCHAR*)(_Metadata->GetBuffer()), expectedFillValue, _Metadata->QuerySize()))
    {
        KDbgCheckpoint((ULONG)-1, "Record Metadata does not match");
        return FALSE;
    }

    ULONG               totalToDo = _IoBuffer->QuerySize();
    KIoBufferElement*   currentElement = totalToDo > 0 ? _IoBuffer->First() : nullptr;
    expectedFillValue |= (ULONGLONG)1 << 63;
    ULONGLONG blockOffset = 0;

    while (totalToDo > 0)
    {
        ULONG   currentEleSize = currentElement->QuerySize();
        KAssert(currentEleSize <= totalToDo);
        ULONG   todo = __min(totalToDo, currentEleSize);
        if (!RepScanULongLong((UCHAR*)(currentElement->GetBuffer()), expectedFillValue, todo, blockOffset))
        {
            KDbgCheckpoint((ULONG)-1, "Record IoBuffer does not match");
            return FALSE;
        }

        currentElement = _IoBuffer->Next(*currentElement);
        totalToDo -= todo;
        KInvariant((todo % 4096) == 0);
        blockOffset += (todo / 4096);
    }

    return TRUE;
}

BOOLEAN
LogStreamIoContext::CompareTo(
    __in LogStreamIoContext& Comparand) const
{
    if(this->_Version != Comparand._Version)
    {
        KDbgCheckpoint((ULONG)-1, "Record versions do not match");
        return FALSE;
    }

    if((KBuffer::SPtr)this->_Metadata && (KBuffer::SPtr)Comparand._Metadata != nullptr)
    {
        if(this->_Metadata->QuerySize() == Comparand._Metadata->QuerySize())
        {
            if(this->_Metadata->QuerySize() == RtlCompareMemory(this->_Metadata->GetBuffer(), Comparand._Metadata->GetBuffer(), this->_Metadata->QuerySize()))
            {
                // Great!
            }
            else
            {
                KDbgCheckpoint((ULONG)-1, "Record metadata does not match");
                return FALSE;
            }
        }
        else
        {
            KDbgCheckpoint((ULONG)-1, "Record metadata size does not match");
            return FALSE;
        }
    }

    if((KIoBuffer::SPtr)this->_IoBuffer && (KIoBuffer::SPtr)Comparand._IoBuffer != nullptr)
    {
        if(this->_IoBuffer->QueryNumberOfIoBufferElements() == Comparand._IoBuffer->QueryNumberOfIoBufferElements())
        {
            KIoBufferElement *elem1 = this->_IoBuffer->First(), *elem2 = Comparand._IoBuffer->First();
            for (;(elem1 != NULL) && (elem2 != NULL); elem1 = this->_IoBuffer->Next(*elem1), elem2 = Comparand._IoBuffer->Next(*elem2))
            {
                if(elem1->QuerySize() == elem2->QuerySize())
                {
                    if(elem1->QuerySize() == RtlCompareMemory(elem1->GetBuffer(), elem2->GetBuffer(), elem1->QuerySize()))
                    {
                        // Great!
                    }
                    else
                    {
                        KDbgCheckpoint((ULONG)-1, "Record data does not match");
                        return FALSE;
                    }
                }
                else
                {
                    KDbgCheckpoint((ULONG)-1, "Record data element sizes do not match");
                    return FALSE;
                }
            }
        }
        else
        {
            KDbgCheckpoint((ULONG)-1, "Record data buffers dont have equal number of elements");
            return FALSE;
        }
    }

    return TRUE;
}


//** AsyncLogStreamIoContext class implementation
AsyncLogStreamIoContext::AsyncLogStreamIoContext(
        __in RvdLogAsn const& RecordAsn,
        __in ULONGLONG const& Version,
        __in IoType const AsyncIoType) : LogStreamIoContext(RecordAsn, Version)
{
    _AsyncIoType = AsyncIoType;
    _DoReadAfterWriteVerify = FALSE;
}

AsyncLogStreamIoContext::~AsyncLogStreamIoContext()
{
}

AsyncLogStreamIoContext::SPtr
AsyncLogStreamIoContext::Create(
    __in RvdLogAsn const& RecordAsn,
    __in ULONGLONG const& Version,
    __in IoType const AsyncIoType)
{
    return SPtr(_new(KTL_TAG_TEST,
                KtlSystem::GlobalNonPagedAllocator())
                AsyncLogStreamIoContext(RecordAsn, Version, AsyncIoType));
}

VOID
AsyncLogStreamIoContext::Reset()
{
    LogStreamIoContext::Reset();
    _AsyncIoType = IoType::Undefined;
    _DoReadAfterWriteVerify = FALSE;
    _OpaqueContext = nullptr;
}


//** AsyncLogStreamIoEngine implementation
AsyncLogStreamIoEngine::AsyncLogStreamIoEngine()
    :   _TruncateHoldList(FIELD_OFFSET(AsyncLogStreamIoContext, _TruncateHoldListEntry))
{
    _IssueNextOperationCallback.Bind(this, &AsyncLogStreamIoEngine::IssueNextIo);
}

AsyncLogStreamIoEngine::~AsyncLogStreamIoEngine()
{
}

AsyncLogStreamIoEngine::SPtr
AsyncLogStreamIoEngine::Create()
{
    return SPtr(_new(KTL_TAG_TEST,
                KtlSystem::GlobalNonPagedAllocator())
                AsyncLogStreamIoEngine());
}

VOID
AsyncLogStreamIoEngine::StartIoEngine(
    __in RvdLogStream::SPtr const& LogStream,
    __in KAsyncQueue<AsyncLogStreamIoContext>::SPtr IoRequestQueue,
    __in ULONG MaxOutstandingIoRequests,
    __in RvdLogAsn LowestAsnToWrite,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _LogStream = LogStream;
    _IoRequestQueue = IoRequestQueue;
    _IoQueueDepth = MaxOutstandingIoRequests;
    _NextExpectedWrittenAsn = LowestAsnToWrite;
    KInvariant(_NextExpectedWrittenAsn.Get() <= MAXULONG);

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
AsyncLogStreamIoEngine::OnStart()
{
    NTSTATUS                                                                status = STATUS_SUCCESS;
    KArray<KAsyncQueue<AsyncLogStreamIoContext>::DequeueOperation::SPtr>    processors(KtlSystem::GlobalNonPagedAllocator());

    _ActivityCount = 0;
    _HighestWrittenAsn = RvdLogAsn::Null();
    _HighestRequestedTruncationPoint = RvdLogAsn::Null();
    _HighestTruncationPoint = RvdLogAsn::Null();

    ULONG bitmapSize = KBitmap::QueryRequiredBufferSize(_MaxAsn + 1);
    status = KBuffer::Create(bitmapSize, _AsnBitmapBuffer, GetThisAllocator(), KTL_TAG_TEST);
    if(!NT_SUCCESS(status))
    {
        KDbgCheckpointWStatus((ULONG)-1, "KBuffer::Create() Failed", status);
        Complete(status);
        return;
    }

    status = _CompletedAsnBitmap.Reuse(_AsnBitmapBuffer->GetBuffer(), _AsnBitmapBuffer->QuerySize(), _MaxAsn + 1);
    if(!NT_SUCCESS(status))
    {
        KDbgCheckpointWStatus((ULONG)-1, "_CompletedAsnBitmap.Reuse Failed", status);
        Complete(status);
        return;
    }

    _CompletedAsnBitmap.ClearAllBits();
    if (_NextExpectedWrittenAsn.Get() > 0)
    {
        _CompletedAsnBitmap.SetBits(0, (ULONG)(_NextExpectedWrittenAsn.Get()));
        KInvariant(_CompletedAsnBitmap.CheckBit((ULONG)(_NextExpectedWrittenAsn.Get())) == FALSE);
    }

    // Allocate a DequeueOperation instance for each possible outstanding Logger operation
    for (ULONG c = 0; c < _IoQueueDepth; c++)
    {
        KAsyncQueue<AsyncLogStreamIoContext>::DequeueOperation::SPtr op;
        status = _IoRequestQueue->CreateDequeueOperation(op);
        if(!NT_SUCCESS(status))
        {
            KDbgCheckpointWStatus((ULONG)-1, "CreateDequeueOperation() Failed", status);
            Complete(status);
            return;
        }

        status = processors.Append(op);
        if(!NT_SUCCESS(status))
        {
            KDbgCheckpointWStatus((ULONG)-1, "processors.Append() Failed", status);
            Complete(status);
            return;
        }
    }

    _ActivityCount = _IoRequestQueue->GetNumberOfQueuedItems();
    _CompletionStatus = STATUS_SUCCESS;

    // NOTE: Only CommonCompleteOperation should be used for completion beyond this point

    // Now schedule all dequeue processors
    for (ULONG ix = 0; ix < processors.Count(); ix++)
    {
        processors[ix]->StartDequeue(nullptr, _IssueNextOperationCallback);
    }
}

VOID
AsyncLogStreamIoEngine::OnReuse()
{
    _LogStream = nullptr;
    _IoRequestQueue = nullptr;
    _IoQueueDepth = 0;
}

VOID
AsyncLogStreamIoEngine::CommonCompleteOperation(NTSTATUS Status, AsyncLogStreamIoContext& Request)
{
    Request._DebugLastWriteOp.Reset();
    Request._DequeueOp->StartDequeue(nullptr, _IssueNextOperationCallback);
    Request._DequeueOp.Reset();

    InterlockedCompareExchange(&_CompletionStatus, Status, STATUS_SUCCESS);

    KAssert(_ActivityCount > 0);
    if (InterlockedDecrement(&_ActivityCount) == 0)
    {
        Complete(_CompletionStatus);
    }
}

VOID
AsyncLogStreamIoEngine::CommonCompleteWriteOperation(AsyncLogStreamIoContext& WriteReq)
{
    RvdLogAsn       truncateTo;
    NTSTATUS        status = STATUS_SUCCESS;

    _HighestWrittenAsn.SetIfLarger(WriteReq._RecordAsn);
    K_LOCK_BLOCK(_TruncationLock)
    {
        KInvariant(WriteReq._RecordAsn >= _NextExpectedWrittenAsn);
        ULONG   currentAsnValue = (ULONG)(WriteReq._RecordAsn.Get());

        _CompletedAsnBitmap.SetBits(currentAsnValue, 1);
        if (_NextExpectedWrittenAsn == WriteReq._RecordAsn)
        {
            BOOLEAN     runType;
            ULONG       numberOfSeqCompletedAsns = _CompletedAsnBitmap.QueryRunLength(currentAsnValue, runType);
            KInvariant(runType == TRUE);

            _NextExpectedWrittenAsn = _NextExpectedWrittenAsn.Get() + numberOfSeqCompletedAsns;
        }

        if ((_HighestRequestedTruncationPoint < _NextExpectedWrittenAsn) && (_HighestRequestedTruncationPoint > _HighestTruncationPoint))
        {
            // We have completed a write at or above the _HighestRequestedTruncationPoint - trigger a truncate
            truncateTo = _HighestRequestedTruncationPoint;
            _HighestTruncationPoint = _HighestRequestedTruncationPoint;
        }
    }

    if (!truncateTo.IsNull())
    {
        status = DoTruncate(truncateTo);
    }

    CommonCompleteOperation(status, WriteReq);
}

NTSTATUS
AsyncLogStreamIoEngine::DoTruncate(RvdLogAsn TruncationPoint)
{
    _LogStream->Truncate(TruncationPoint, RvdLogAsn::Min());

    RvdLogAsn   lowest = RvdLogAsn::Min();
    RvdLogAsn   highest = RvdLogAsn::Min();
    RvdLogAsn   lastTruncation = RvdLogAsn::Min();

    NTSTATUS status = _LogStream->QueryRecordRange(&lowest, &highest, &lastTruncation);
    if (!NT_SUCCESS(status))
    {
        KDbgCheckpointWStatus((ULONG)-1, "QueryRecordRange() failed", status);
        return status;
    }

    if (lastTruncation < TruncationPoint)
    {
        status = STATUS_UNSUCCESSFUL;
        KDbgPrintf(
            "Last truncation ASN '%I64u' does not match most recent truncate ASN '%I64u\n'",
            lastTruncation.Get(),
            TruncationPoint.Get());

        KDbgCheckpointWStatus((ULONG)-1, "Log stream IO failed", status);
        return status;
    }

    // Release any held ops do to truncates that were too far ahead
    K_LOCK_BLOCK(_TruncationLock)
    {
        AsyncLogStreamIoContext*    heldOp = nullptr;
        while ((heldOp = _TruncateHoldList.RemoveHead()) != nullptr)
        {
            CommonCompleteOperation(STATUS_SUCCESS, *heldOp);

            // See #1 in IssueNextIo
            heldOp->Release();
            heldOp = nullptr;
        }
    }

    return STATUS_SUCCESS;
}

// Called when there is an AsyncLogStreamIoContext to process OR the queue is being shutdown
VOID
AsyncLogStreamIoEngine::IssueNextIo(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletedContext)
{
    UNREFERENCED_PARAMETER(Parent);

    NTSTATUS status = CompletedContext.Status();
    if (!NT_SUCCESS(status))
    {
        return;
    }

    // Have an AsyncLogStreamIoContext to process
    KAsyncQueue<AsyncLogStreamIoContext>::DequeueOperation* dequeueOp;
    dequeueOp = static_cast<KAsyncQueue<AsyncLogStreamIoContext>::DequeueOperation*>(&CompletedContext);

    // Take the AsyncLogStreamIoContext ref ownership from dequeue op
    AsyncLogStreamIoContext::SPtr ioReq;
    ioReq.Attach(&dequeueOp->GetDequeuedItem());
    dequeueOp->Reuse();

    ioReq->_DequeueOp = dequeueOp;          // Associate the current ioReq with the current dequeue op so that
                                            // the next dequeue can be issued once the ioReq completes

    KInvariant(ioReq->_RecordAsn.Get() <= MAXULONG);

    KAsyncContextBase::CompletionCallback ioCallback;

    if(ioReq->_AsyncIoType == IoType::Write)
    {
        RvdLogStream::AsyncWriteContext::SPtr writeContext;
        status = _LogStream->CreateAsyncWriteContext(writeContext);
        if (!NT_SUCCESS(status))
        {
            KDbgCheckpointWStatus((ULONG)-1, "CreateAsyncWriteContext() failed", status);
            CommonCompleteOperation(status, *ioReq);
            return;
        }

        status = ioReq->AllocateBuffers();
        if (!NT_SUCCESS(status))
        {
            KDbgCheckpointWStatus((ULONG)-1, "AllocateBuffers() failed", status);
            CommonCompleteOperation(status, *ioReq);
            return;
        }

        ioReq->_DebugLastWriteOp = (RvdLogStreamImp::AsyncWriteStream*)(writeContext.RawPtr());

        AsyncLogStreamIoContext*    writeReq = ioReq.RawPtr();
        ioReq->_AsyncIoContext = up_cast<KAsyncContextBase>(writeContext);
#ifdef _WIN64
        ioCallback.Bind(ioReq.Detach(), &AsyncLogStreamIoEngine::StaticStreamWriteCompleted);
#endif
        writeContext->StartWrite(
            writeReq->_RecordAsn,
            writeReq->_Version,
            writeReq->GetMetadata(),
            writeReq->GetIoBuffer(),
            this,
            ioCallback);

        writeReq = nullptr;
        return;
    }

    // BUG, amith, xxxxx, decide if this sort of read behavior should be resurrected
    /*
    else if(IoReq->_AsyncIoType == IoType::Read)
    {
        RvdLogStream::AsyncReadContext::SPtr readContext;
        status = _LogStream->CreateAsyncReadContext(readContext);
        if (!NT_SUCCESS(status))
        {
           KDbgCheckpointWStatus((ULONG)-1, "CreateAsyncReadContext() failed", status);
           return status;
        }

        IoReq->_AsyncIoContext = up_cast<KAsyncContextBase>(readContext);
        ioCallback.Bind(IoReq, &AsyncLogStreamIoContext::OnIoCompletionCallback);
        readContext->StartRead(IoReq->_RecordAsn, (ULONGLONG*)&IoReq->_Version, IoReq->_Metadata, IoReq->_IoBuffer, this, ioCallback);
    }
    */

    else if (ioReq->_AsyncIoType == IoType::Truncate)
    {
        // Delay the trucation point (_RecordAsn) if needed to complete any writes below that point in ASN space
        RvdLogAsn   truncateTo;
        status = STATUS_SUCCESS;

        K_LOCK_BLOCK(_TruncationLock)
        {
            _HighestRequestedTruncationPoint.SetIfLarger(ioReq->_RecordAsn);
            if (_HighestRequestedTruncationPoint < _NextExpectedWrittenAsn)
            {
                if (_HighestRequestedTruncationPoint > _HighestTruncationPoint)
                {
                    truncateTo = _HighestRequestedTruncationPoint;
                    _HighestTruncationPoint = _HighestRequestedTruncationPoint;
                }
            }
            else
            {
                // Requested truncation point is above our active write ASN region - hold this ops
                // completion to throttle write activity
                ioReq->AddRef();                                    // #1 keep alive while on the hold list
                _TruncateHoldList.AppendTail(ioReq.RawPtr());
                return;
            }

            if ((_HighestRequestedTruncationPoint < _NextExpectedWrittenAsn) && (_HighestRequestedTruncationPoint > _HighestTruncationPoint))
            {
                truncateTo = _HighestRequestedTruncationPoint;
                _HighestTruncationPoint = _HighestRequestedTruncationPoint;
            }
        }

        if (!truncateTo.IsNull())
        {
            status = DoTruncate(truncateTo);
        }

        CommonCompleteOperation(status, *ioReq);
        return;
    }

    KInvariant(FALSE);
}

VOID
AsyncLogStreamIoEngine::StaticStreamWriteCompleted(
    __in void* Context,                             // ref counted AsyncLogStreamIoContext*
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletedContext)
//  Continued from IssueNextIo()
{
    AsyncLogStreamIoEngine*             thisPtr = (AsyncLogStreamIoEngine*)Parent;
    RvdLogStream::AsyncWriteContext*    writeOp = (RvdLogStream::AsyncWriteContext*)(&CompletedContext);

    AsyncLogStreamIoContext::SPtr       writeReq;
    writeReq.Attach((AsyncLogStreamIoContext*)Context);

    thisPtr->StreamWriteCompleted(Ktl::Move(writeReq), *writeOp);
}

VOID
AsyncLogStreamIoEngine::StreamWriteCompleted(
    AsyncLogStreamIoContext::SPtr WriteReq,
    RvdLogStream::AsyncWriteContext& CompletedWriteOp)
{
    NTSTATUS status = CompletedWriteOp.Status();
    if(!NT_SUCCESS(status))
    {
        KDbgPrintf(
            "StreamWrite error: 0x%08X for ASN : %I64u, version : %I64u\n",
            status,
            WriteReq->_RecordAsn.Get(),
            WriteReq->_Version);

        KDbgCheckpointWStatus((ULONG)-1, "Log stream IO failed", status);
        CommonCompleteOperation(status, *WriteReq);
        return;
    }

    if (((RvdLogStreamImp::AsyncWriteStream*)&CompletedWriteOp)->IoWrappedEOF())
    {
        KDbgPrintf(
            "Write of ASN %I64u (Version %I64u) wrapped EOF\n",
            WriteReq->_RecordAsn.Get(),
            WriteReq->_Version);
    }

    if (!WriteReq->_DoReadAfterWriteVerify)
    {
        CommonCompleteWriteOperation(*WriteReq);
        return;
    }

    // Verify the current write
    _ReadVerifyTries = 4;
    RvdLogStream::AsyncReadContext::SPtr readContext;
    status = _LogStream->CreateAsyncReadContext(readContext);
    if (!NT_SUCCESS(status))
    {
        KDbgCheckpointWStatus((ULONG)-1, "CreateAsyncReadContext() failed", status);
        CommonCompleteOperation(status, *WriteReq);
        return;
    }

    KAsyncContextBase::CompletionCallback   ioCallback;
    AsyncLogStreamIoContext*                writeReqPtr = WriteReq.RawPtr();

    WriteReq->_AsyncIoContext = up_cast<KAsyncContextBase>(readContext);
#ifdef _WIN64
    ioCallback.Bind(WriteReq.Detach(), &AsyncLogStreamIoEngine::StaticStreamReadAfterWriteCompleted);
#endif
    readContext->StartRead(
        writeReqPtr->_RecordAsn,
        &writeReqPtr->_ReadVersion,
        writeReqPtr->GetMetadataRef(),
        writeReqPtr->GetIoBufferRef(),
        this,
        ioCallback);

    writeReqPtr = nullptr;
    // Continued @ StaticStreamReadAfterWriteCompleted()
}

VOID
AsyncLogStreamIoEngine::StaticStreamReadAfterWriteCompleted(
    __in void* Context,
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletedContext)
//  Continued from StreamWriteCompleted()
{
    AsyncLogStreamIoEngine*             thisPtr = (AsyncLogStreamIoEngine*)Parent;
    RvdLogStream::AsyncReadContext*     readOp = (RvdLogStream::AsyncReadContext*)(&CompletedContext);

    AsyncLogStreamIoContext::SPtr       writeReq;
    writeReq.Attach((AsyncLogStreamIoContext*)Context);

    thisPtr->StreamReadAfterWriteCompleted(Ktl::Move(writeReq), *readOp);
}

VOID
AsyncLogStreamIoEngine::StaticStreamReReadDelayCompleted(
    __in void* WriteReqContext,
    __in_opt KAsyncContextBase* const IoEngineParent,
    __in KAsyncContextBase& CompletedContext)
//  Continued from StreamReadAfterWriteCompleted()
{
    UNREFERENCED_PARAMETER(CompletedContext);

    AsyncLogStreamIoEngine*             thisPtr = (AsyncLogStreamIoEngine*)IoEngineParent;
    AsyncLogStreamIoContext::SPtr       writeReq;
    writeReq.Attach((AsyncLogStreamIoContext*)WriteReqContext);

    thisPtr->StreamReReadDelayCompleted(Ktl::Move(writeReq));
}

VOID
AsyncLogStreamIoEngine::StreamReReadDelayCompleted(AsyncLogStreamIoContext::SPtr WriteReq)
//  Continued from StaticStreamReReadDelayCompleted()
{
    RvdLogStream::AsyncReadContext::SPtr readContext;
    NTSTATUS status = _LogStream->CreateAsyncReadContext(readContext);
    if (!NT_SUCCESS(status))
    {
        KDbgCheckpointWStatus((ULONG)-1, "CreateAsyncReadContext() failed", status);
        CommonCompleteOperation(status, *WriteReq);
        return;
    }

    KAsyncContextBase::CompletionCallback   ioCallback;
    AsyncLogStreamIoContext*                writeReqPtr = WriteReq.RawPtr();

    WriteReq->_AsyncIoContext = up_cast<KAsyncContextBase>(readContext);
#ifdef _WIN64
    ioCallback.Bind(WriteReq.Detach(), &AsyncLogStreamIoEngine::StaticStreamReadAfterWriteCompleted);
#endif
    readContext->StartRead(
        writeReqPtr->_RecordAsn,
        &writeReqPtr->_ReadVersion,
        writeReqPtr->GetMetadataRef(),
        writeReqPtr->GetIoBufferRef(),
        this,
        ioCallback);

    writeReqPtr = nullptr;
    // Continued @ StaticStreamReadAfterWriteCompleted()
}

extern LONGLONG g_BlockFileNumBytesReceived;
extern LONGLONG g_BlockFileNumBytesSent;

VOID
DebugDumpWriteOpState(AsyncLogStreamIoContext::SPtr WriteReq)
{
    KDbgPrintf(
        "LastWriteInfo: ASN : %I64u, version: %I64u; FileOffset0: 0x%010I64X; Size0: %u; FileOffset1: 0x%010I64X; Size1: %u\n",
        WriteReq->_RecordAsn.Get(),
        WriteReq->_Version,
        WriteReq->_DebugLastWriteOp->_DebugFileWriteOffset0,
        WriteReq->_DebugLastWriteOp->_DebugFileWriteLength0,
        WriteReq->_DebugLastWriteOp->_DebugFileWriteOffset1,
        WriteReq->_DebugLastWriteOp->_DebugFileWriteLength1);

    KDbgPrintf(
        "LastWriteInfo: KBlockFile Info: g_BlockFileNumBytesReceived: %I64u; g_BlockFileNumBytesSent: %I64u\n",
        g_BlockFileNumBytesReceived,
        g_BlockFileNumBytesSent);
}

VOID
AsyncLogStreamIoEngine::StreamReadAfterWriteCompleted(
    AsyncLogStreamIoContext::SPtr WriteReq,
    RvdLogStream::AsyncReadContext&  CompletedReadOp)
{
    _ReadVerifyTries--;
    NTSTATUS status = CompletedReadOp.Status();
    CompletedReadOp.Reuse();

    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("StreamRead error: 0x%08X for ASN: %I64u, version: %I64u", status, WriteReq->_RecordAsn.Get(), WriteReq->_Version);

        if ((_ReadVerifyTries > 0) && (status == STATUS_NOT_FOUND))
        {
            // This can occur because of a newer version write for the same ASN racing ahead of
            // our read-after-write-verify
            KDbgPrintf(" -- Record missing - will retry after a short delay\n");

            // Delay a bit (100msecs) and retry the read to allow for the newer version to finish
            // being written.
            KTimer::SPtr    timer;
            status = KTimer::Create(timer, GetThisAllocator(), KTL_TAG_TEST);
            if(!NT_SUCCESS(status))
            {
                KDbgPrintf("StreamReadAfterWriteCompleted: KTimer::Create() failed: 0x%08X\n", status);
                KDbgCheckpointWStatus((ULONG)-1, "KTimer::Create() failed", status);
                CommonCompleteOperation(status, *WriteReq);
                return;
            }

            KAsyncContextBase::CompletionCallback   timerCallback;
#ifdef _WIN64
            timerCallback.Bind(WriteReq.Detach(), &AsyncLogStreamIoEngine::StaticStreamReReadDelayCompleted);
#endif
            timer->StartTimer(100, this, timerCallback);

            // Continued @ StaticStreamReReadDelayCompleted()
            return;
        }

        KDbgPrintf("\n");
        DebugDumpWriteOpState(WriteReq);
        KDbgCheckpointWStatus((ULONG)-1, "Log stream Read failed", status);

        CommonCompleteOperation(status, *WriteReq);
        return;
    }

    if (!WriteReq->ValidateBuffers())
    {
        status = STATUS_UNSUCCESSFUL;
        KDbgPrintf("Record read does not match written record ASN : %I64u, version : %I64u\n", WriteReq->_RecordAsn.Get(), WriteReq->_Version);
        DebugDumpWriteOpState(WriteReq);
        KDbgCheckpointWStatus((ULONG)-1, "Log stream IO failed", status);

        CommonCompleteOperation(status, *WriteReq);
        return;
    }

    CommonCompleteWriteOperation(*WriteReq);
}


//** IoRequestGenerator class implementation
IoRequestGenerator::IoRequestGenerator()
    :   _LogWriteHistory(FIELD_OFFSET(AsyncLogStreamIoContext, _HistoryListEntry))
{
    // Set defaults
    _StartingAsn = 1;
    _StartingVersion = 1;
    _LogWriteReorderWindow = 16;
    _LogTailMaxLength = 160;
    _VersionChangeProbability = 1000;
    _VersionChangeOverlapMaxSize = 10;
    _LogReadProbability = 10;
    _MinUserMetadataSize = 0;
    _MinUserIoBufferSize = 0;
    _MaxUserMetadataSize = 0;
    _MaxUserIoBufferSize = 0;

    Initialize();
}

IoRequestGenerator::~IoRequestGenerator()
{
}

VOID
IoRequestGenerator::Initialize()
{
    _HighestWrittenAsn = _StartingAsn;
    _CurrentVersion = _StartingVersion;
    _LastTruncAsn = _StartingAsn - 1;

    _WriteAsnLo = _StartingAsn;
    _WriteAsnHi = _StartingAsn + _LogWriteReorderWindow - 1;

    _LogTrimLength = _LogTailMaxLength / 4;
    _LogWriteRepeat = nullptr;
}

RvdLogAsn
IoRequestGenerator::GetNextWriteAsn()
{
    // Adjust the range from which write asns are picked if _LogWriteHistory is full
    if (_LogWriteReorderWindow == _LogWriteHistory.Count())
    {
        RvdLogAsn writtenAsnLo = RvdLogAsn::Max();
        RvdLogAsn writtenAsnHi = RvdLogAsn::Min();

        for(AsyncLogStreamIoContext* writeReq = _LogWriteHistory.PeekHead();
            writeReq != nullptr;
            writeReq = _LogWriteHistory.Successor(writeReq))
        {
            if(writeReq->_RecordAsn < writtenAsnLo)
            {
                writtenAsnLo = writeReq->_RecordAsn;
            }
            if(writeReq->_RecordAsn > writtenAsnHi)
            {
                writtenAsnHi = writeReq->_RecordAsn;
            }
        }

        if((_WriteAsnLo == writtenAsnLo.Get()) && (_WriteAsnHi == writtenAsnHi.Get()))
        {
            _WriteAsnLo = _WriteAsnLo + _LogWriteReorderWindow;
            _WriteAsnHi = _WriteAsnHi + _LogWriteReorderWindow;
        }
    }

    // Pick a random write asn from the range and verify that its not already used
    ULONGLONG writeAsn;
    BOOLEAN writeAsnUsed;
    do
    {
        writeAsn = (ULONGLONG)RandomGenerator::Get((ULONG)_WriteAsnHi, (ULONG)_WriteAsnLo);
        writeAsnUsed = FALSE;
        for(AsyncLogStreamIoContext* writeReq = _LogWriteHistory.PeekHead();
            writeReq != nullptr;
            writeReq = _LogWriteHistory.Successor(writeReq))
        {
            if(writeReq->_RecordAsn == writeAsn)
            {
                writeAsnUsed = TRUE;
                break;
            }
        }
    }
    while(writeAsnUsed);

    return writeAsn;
}

NTSTATUS
IoRequestGenerator::GenerateTruncateRequest(
    __out AsyncLogStreamIoContext::SPtr& IoRequest,
    __in RvdLogAsn Asn)
{
    NTSTATUS status = STATUS_SUCCESS;
    IoRequest = AsyncLogStreamIoContext::Create(Asn, 0, IoType::Truncate);
    //BUG, amith, xxxxx, check NewRequest == nullptr;
    return status;
}

NTSTATUS
IoRequestGenerator::GenerateWriteRequest(
        __out AsyncLogStreamIoContext::SPtr& IoRequest,
        __in RvdLogAsn Asn,
        __in ULONGLONG Version,
        __in ULONG MetadataSize,
        __in ULONG IoBufferSize,
        __in BOOLEAN DoReadVerify)
{
    NTSTATUS status = STATUS_SUCCESS;
    IoRequest = AsyncLogStreamIoContext::Create(Asn, Version, IoType::Write);
    //BUG, amith, xxxxx, check NewRequest == nullptr;

    IoRequest->SetMetadataSize(MetadataSize);
    IoRequest->SetIoBufferSize(IoBufferSize);
    IoRequest->_DoReadAfterWriteVerify = DoReadVerify;
    return status;
}

NTSTATUS
IoRequestGenerator::GenerateNewRequest(
        __in ULONG MaxRecordSize,
        __out AsyncLogStreamIoContext::SPtr& NewRequest)
{
    NTSTATUS status = STATUS_SUCCESS;

    // Generate truncate request
    if(_HighestWrittenAsn - _LastTruncAsn > _LogTailMaxLength)
    {
        RvdLogAsn truncAsn = (RvdLogAsn)(_LastTruncAsn + _LogTrimLength);
        status = GenerateTruncateRequest(NewRequest, truncAsn);
        _LastTruncAsn = truncAsn.Get();
        return status;
    }

    // BUG, amith, xxxxx, decide if this approach can be revived
    /*
    // Generate read request
    if(RandomGenerator::Get(_LogReadProbability, 1) == _LogReadProbability)
    {
        if(_WriteAsnLo > _StartingAsn + _LogWriteReorderWindow - 1)
        {
            // Pick the read asn randomly from [readAsnHi, readAsnLo] as this is the most stable recent portion of the log stream
            ULONGLONG readAsnLo, readAsnHi;
            readAsnHi = _WriteAsnLo - 1;
            readAsnLo = _WriteAsnLo - _LogWriteReorderWindow;
            ULONGLONG readAsn = (ULONGLONG)RandomGenerator::Get((ULONG)readAsnHi, (ULONG)readAsnLo);
            NewRequest = AsyncLogStreamIoContext::Create((RvdLogAsn)readAsn, 0, IoType::Read);
            return status;
        }
    }
    */

    // Generate write request
    if(_LogWriteRepeat)
    {
        // Repeating history of write requests since we are in the middle of a version change
        NewRequest = AsyncLogStreamIoContext::Create(_LogWriteRepeat->_RecordAsn, _CurrentVersion, IoType::Write);
        NewRequest->SetMetadataSize(_LogWriteRepeat->GetMetadataSize());
        NewRequest->SetIoBufferSize(_LogWriteRepeat->GetIoBufferSize());

        // Periodically (within _LogReadProbability) enable read verification
        if(RandomGenerator::Get(_LogReadProbability, 1) == _LogReadProbability)
        {
            NewRequest->_DoReadAfterWriteVerify = TRUE;
        }

        _LogWriteRepeat = _LogWriteHistory.Successor(_LogWriteRepeat.RawPtr());
    }
    else
    {
        // Generate a new write request
        RvdLogAsn writeAsn = GetNextWriteAsn();

        ULONG               metadataSize;
        ULONG               ioBufferSize;

        do
        {
            metadataSize = RandomGenerator::Get(_MaxUserMetadataSize, _MinUserMetadataSize);
            ioBufferSize = RandomGenerator::Get(_MaxUserIoBufferSize, _MinUserIoBufferSize);

            ioBufferSize = RvdDiskLogConstants::RoundUpToBlockBoundary(ioBufferSize);
        }
        while ((metadataSize + ioBufferSize) > MaxRecordSize);

        NewRequest = AsyncLogStreamIoContext::Create(writeAsn, _CurrentVersion, IoType::Write);
        //BUG, amith, xxxxx, check NewRequest == nullptr;

        NewRequest->SetMetadataSize(metadataSize);
        NewRequest->SetIoBufferSize(ioBufferSize);

        if(writeAsn > _HighestWrittenAsn)
        {
            _HighestWrittenAsn = writeAsn.Get();
        }

        // Update internal write history
        _LogWriteHistory.AppendTail(NewRequest);
        if(_LogWriteHistory.Count() > _LogWriteReorderWindow)
        {
            _LogWriteHistory.RemoveHead();
        }

        // Periodically (within _LogReadProbability) enable read verification
        if(RandomGenerator::Get(_LogReadProbability, 1) == _LogReadProbability)
        {
            NewRequest->_DoReadAfterWriteVerify = TRUE;
        }

        // version change now?
        if (_VersionChangeProbability > 0)
        {
            if(RandomGenerator::Get(_VersionChangeProbability, 1) == _VersionChangeProbability)
            {
                // Yes, so determine how far back we go in history due to version change
                ULONG _VersionChangeOverlapSize = RandomGenerator::Get(_VersionChangeOverlapMaxSize, 1);
                if (_VersionChangeOverlapSize >= _LogWriteHistory.Count())
                {
                    _LogWriteRepeat = _LogWriteHistory.PeekHead();
                }
                else
                {
                    _LogWriteRepeat = _LogWriteHistory.PeekTail();

                    // Now, go sufficiently back in write history
                    while(_VersionChangeOverlapSize--)
                    {
                        _LogWriteRepeat = _LogWriteHistory.Predecessor(_LogWriteRepeat.RawPtr());
                    }
                }

                // Update version
                _CurrentVersion = _CurrentVersion + 1;
            }
        }
    }

    return status;
}


//** Various local helper types
enum LogStreamState
{
    Closed,
    Open,
    Deleted
};

NTSTATUS
CheckLogStreamState(
    __in RvdLog::SPtr const& Log,
    __in KGuid const& LogStreamId,
    __in LogStreamState ExpectedState
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    if(!Log->IsStreamIdValid(RvdLogStreamId(LogStreamId)))
    {
        KDbgCheckpointWStatus((ULONG)-1, "IsStreamIdValid() failed", status);
        return STATUS_UNSUCCESSFUL;
    }

    LogStreamState currentState = LogStreamState::Deleted;
    BOOLEAN isOpen = FALSE, isClosed = FALSE, isDeleted = FALSE;
    if(!Log->GetStreamState(RvdLogStreamId(LogStreamId), &isOpen, &isClosed, &isDeleted))
    {
        KDbgCheckpointWStatus((ULONG)-1, "GetStreamState() failed", status);
        return STATUS_UNSUCCESSFUL;
    }

    if(isOpen)
    {
        currentState = LogStreamState::Open;
    }
    else if (isClosed)
    {
        currentState = LogStreamState::Closed;
    }

    if(currentState != ExpectedState)
    {
        KDbgCheckpointWStatus((ULONG)-1, "Current stream state unexpected", status);
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
CreateLogManagerInstance(
    __in RvdLogManager::SPtr& LogManager)
{
    NTSTATUS status = STATUS_SUCCESS;

    status = RvdLogManager::Create(KTL_TAG_LOGGER, KtlSystem::GlobalNonPagedAllocator(), LogManager);
    if (!NT_SUCCESS(status))
    {
        KDbgCheckpointWStatus((ULONG)-1, "Failed to create log manager instance", status);
        return status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
ActivateLogManagerInstance(
    __in KEvent& ActivateEvent,
    __in RvdLogManager::SPtr& LogManager)
{
    NTSTATUS status = STATUS_SUCCESS;

    KAsyncContextBase::CompletionCallback activateDoneCallback;
#ifdef _WIN64
    activateDoneCallback.Bind((PVOID)(&ActivateEvent), &StaticTestCallback);
#else
    UNREFERENCED_PARAMETER(ActivateEvent);
#endif

    status = LogManager->Activate(nullptr, activateDoneCallback);
    if (!K_ASYNC_SUCCESS(status))
    {
        KDbgCheckpointWStatus((ULONG)-1, "Failed to activate log manager instance", status);
        return status;
    }

    return STATUS_SUCCESS;
}


//** Various KAsyncContext derivations for specific tests

//* AsyncCreateLogTest implementation
class AsyncCreateLogTest : public KAsyncContextBase
{
    K_FORCE_SHARED_WITH_INHERITANCE(AsyncCreateLogTest);

public:
    static AsyncCreateLogTest::SPtr
    Create();

    NTSTATUS
    DoTest(
        __in RvdLogManager::SPtr const& LogManager,
        __in KGuid const& DiskId,
        __in KGuid const& LogId,
        __in KWString& LogType,
        __in LONGLONG LogSize,
        __out RvdLog::SPtr* Log);

private:
    VOID
    TestCompletionCallback(
        __in KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletedContext);

    VOID
    OnStart();

    VOID
    OnReuse();

    VOID
    PrivateOnFinalize();

private:
    RvdLogManager::SPtr _LogManager;
    KGuid               _DiskId;
    KGuid               _LogId;
    KWString            _LogType;
    LONGLONG            _LogSize;
    RvdLog::SPtr        _LogObj;
    KEvent              _CompletionEvent;
};

AsyncCreateLogTest::AsyncCreateLogTest()
    :   _CompletionEvent(FALSE, FALSE),
        _LogType(GetThisAllocator())
{
}

AsyncCreateLogTest::~AsyncCreateLogTest()
{
}

AsyncCreateLogTest::SPtr
AsyncCreateLogTest::Create()
{
    return SPtr(_new(KTL_TAG_TEST,
        KtlSystem::GlobalNonPagedAllocator()) AsyncCreateLogTest());
}

NTSTATUS
AsyncCreateLogTest::DoTest(
    __in RvdLogManager::SPtr const& LogManager,
    __in KGuid const& DiskId,
    __in KGuid const& LogId,
    __in KWString& LogType,
    __in LONGLONG LogSize,
    __out RvdLog::SPtr* Log)
{
    _LogManager = LogManager;
    _DiskId = DiskId;
    _LogId = LogId;
    _LogType = LogType;
    _LogSize = LogSize;
    _LogObj = 0;

    Start(nullptr, nullptr);
    _CompletionEvent.WaitUntilSet();

    NTSTATUS    status = Status();
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    // Check if log exists on disk
    status = CheckIfLogExists(_LogId, _DiskId);
    if (!NT_SUCCESS(status))
    {
        KDbgCheckpointWStatus((ULONG)-1, "CheckIfLogExists() failed", status);
        return status;
    }

    if(Log != nullptr)
    {
        *Log = _LogObj;
    }

    return status;
}

VOID
AsyncCreateLogTest::OnStart()
{
    NTSTATUS status = STATUS_SUCCESS;

    RvdLogManager::AsyncCreateLog::SPtr createOp;
    status = _LogManager->CreateAsyncCreateLogContext(createOp);
    if (!NT_SUCCESS(status))
    {
        KDbgCheckpointWStatus((ULONG)-1, "CreateAsyncCreateLogContext() failed", status);
        Complete(status);
    }

    KAsyncContextBase::CompletionCallback callback;
    callback.Bind(this, &AsyncCreateLogTest::TestCompletionCallback);

    // Create a log file
    createOp->StartCreateLog(
        _DiskId,
        RvdLogId(_LogId),
        _LogType,
        _LogSize,
        _LogObj,
        this,
        callback);
}

VOID
AsyncCreateLogTest::TestCompletionCallback(
    __in KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletedContext)
{
    UNREFERENCED_PARAMETER(Parent);

    NTSTATUS status = CompletedContext.Status();

    if (!NT_SUCCESS(status))
    {
        KDbgCheckpointWStatus((ULONG)-1, "StartCreateLog() failed", status);
        Complete(status);
    }

    Complete(STATUS_SUCCESS);
}

VOID
AsyncCreateLogTest::PrivateOnFinalize()
{
    // NOTE: This is a restricted method - should not be overridden in normal use cases
    _CompletionEvent.SetEvent();
    Release();
}

VOID
AsyncCreateLogTest::OnReuse()
{
    KGuid zeroGuid(TRUE);
    _LogManager = 0;
    _DiskId = zeroGuid;
    _LogId = zeroGuid;
    _LogType = L"";
    _LogSize = 0;
    _LogObj = 0;
}


//* AsyncDeleteLogTest implementation
class AsyncDeleteLogTest : public KAsyncContextBase
{
    K_FORCE_SHARED_WITH_INHERITANCE(AsyncDeleteLogTest);

public:
    static AsyncDeleteLogTest::SPtr
    Create();

    NTSTATUS
    DoTest(
        __in RvdLogManager::SPtr const& LogManager,
        __in KGuid const& DiskId,
        __in KGuid const& LogId);

private:
    VOID
    OnStart();

    VOID
    OnReuse();

    VOID
    PrivateOnFinalize();

    VOID
    TestCompletionCallback(
        __in KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletedContext);

private:
    RvdLogManager::SPtr _LogManager;
    KGuid               _DiskId;
    KGuid               _LogId;
    KEvent              _CompletionEvent;
};

AsyncDeleteLogTest::AsyncDeleteLogTest()
    :   _CompletionEvent(FALSE, FALSE)
{
}

AsyncDeleteLogTest::~AsyncDeleteLogTest()
{
}

AsyncDeleteLogTest::SPtr
AsyncDeleteLogTest::Create()
{
    return SPtr(_new(KTL_TAG_TEST,
        KtlSystem::GlobalNonPagedAllocator()) AsyncDeleteLogTest());
}

NTSTATUS
AsyncDeleteLogTest::DoTest(
    __in RvdLogManager::SPtr const& LogManager,
    __in KGuid const& DiskId,
    __in KGuid const& LogId)
{
    _LogManager = LogManager;
    _DiskId = DiskId;
    _LogId = LogId;

    Start(nullptr, nullptr);

    _CompletionEvent.WaitUntilSet();

    NTSTATUS status = Status();
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    // Check if log exists on disk
    status = CheckIfLogExists(_LogId, _DiskId);
    if (NT_SUCCESS(status))
    {
        KDbgCheckpointWStatus((ULONG)-1, "CheckIfLogExists() failed", status);
        Complete(STATUS_UNSUCCESSFUL);
    }

    return STATUS_SUCCESS;
}

VOID
AsyncDeleteLogTest::OnStart()
{
    NTSTATUS status = STATUS_SUCCESS;

    RvdLogManager::AsyncDeleteLog::SPtr deleteOp;
    status = _LogManager->CreateAsyncDeleteLogContext(deleteOp);
    if (!NT_SUCCESS(status))
    {
        KDbgCheckpointWStatus((ULONG)-1, "CreateAsyncDeleteLogContext() failed", status);
        Complete( status);
    }

    KAsyncContextBase::CompletionCallback callback;
    callback.Bind(this, &AsyncDeleteLogTest::TestCompletionCallback);

    // Open a log file
    deleteOp->StartDeleteLog(
        _DiskId,
        RvdLogId(_LogId),
        this,
        callback);
}

VOID
AsyncDeleteLogTest::TestCompletionCallback(
    __in KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletedContext)
{
    UNREFERENCED_PARAMETER(Parent);

    NTSTATUS status = CompletedContext.Status();

    if (!NT_SUCCESS(status))
    {
        KDbgCheckpointWStatus((ULONG)-1, "StartDeleteLog() failed", status);
        Complete(status);
    }

    Complete(STATUS_SUCCESS);
}

VOID
AsyncDeleteLogTest::PrivateOnFinalize()
{
    // NOTE: This is a restricted method - should not be overridden in normal use cases
    _CompletionEvent.SetEvent();
    Release();
}

VOID
AsyncDeleteLogTest::OnReuse()
{
    KGuid zeroGuid(TRUE);
    _LogManager = 0;
    _DiskId = zeroGuid;
    _LogId = zeroGuid;
}


//* AsyncCreateLogStreamTest implementation
class AsyncCreateLogStreamTest : public KAsyncContextBase
{
    K_FORCE_SHARED_WITH_INHERITANCE(AsyncCreateLogStreamTest);

public:
    static AsyncCreateLogStreamTest::SPtr
    Create();

    virtual VOID
    StartTest(
        __in RvdLog::SPtr const& Log,
        __in KGuid const& LogStreamId,
        __in KGuid const& LogStreamType,
        __out RvdLogStream::SPtr* LogStream,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

    VOID
    TestCompletionCallback(
        __in KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletedContext);

protected:
    VOID
    OnStart();

    VOID
    OnReuse();

protected:
    RvdLog::SPtr _Log;
    KGuid _LogStreamId;
    KGuid _LogStreamType;
    RvdLogStream::SPtr* _LogStream;
    RvdLogStream::SPtr _LogStreamPriv;
};

AsyncCreateLogStreamTest::AsyncCreateLogStreamTest()
{
}

AsyncCreateLogStreamTest::~AsyncCreateLogStreamTest()
{
}

AsyncCreateLogStreamTest::SPtr
AsyncCreateLogStreamTest::Create()
{
    return SPtr(_new(KTL_TAG_TEST,
        KtlSystem::GlobalNonPagedAllocator()) AsyncCreateLogStreamTest());
}

VOID
AsyncCreateLogStreamTest::StartTest(
    __in RvdLog::SPtr const& Log,
    __in KGuid const& LogStreamId,
    __in KGuid const& LogStreamType,
    __out RvdLogStream::SPtr* LogStream,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)

{
    _Log = Log;
    _LogStreamId = LogStreamId;
    _LogStreamType = LogStreamType;
    _LogStream = LogStream;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
AsyncCreateLogStreamTest::OnStart()
{
    NTSTATUS status = STATUS_SUCCESS;

    RvdLog::AsyncCreateLogStreamContext::SPtr createStreamOp;
    KAsyncContextBase::CompletionCallback callback(this, &AsyncCreateLogStreamTest::TestCompletionCallback);

    status = _Log->CreateAsyncCreateLogStreamContext(createStreamOp);
    if (!NT_SUCCESS(status))
    {
        KDbgCheckpointWStatus((ULONG)-1, "CreateAsyncCreateLogStreamContext() failed", status);
        Complete(status);
    }

    createStreamOp->StartCreateLogStream(
        RvdLogStreamId(_LogStreamId),
        RvdLogStreamType(_LogStreamType),
        _LogStreamPriv,
        this,
        callback);
}

VOID
AsyncCreateLogStreamTest::TestCompletionCallback(
    __in KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletedContext)
{
    UNREFERENCED_PARAMETER(Parent);

    NTSTATUS status = CompletedContext.Status();

    if (!NT_SUCCESS(status))
    {
        KDbgCheckpointWStatus((ULONG)-1, "StartCreateLogStream() failed", status);
        Complete(status);
    }

    status = CheckLogStreamState(_Log, _LogStreamId, LogStreamState::Open);
    if (!NT_SUCCESS(status))
    {
        KDbgCheckpointWStatus((ULONG)-1, "CheckLogStreamState() failed", status);
        Complete(status);
    }

    if(_LogStream != nullptr)
    {
        *_LogStream = _LogStreamPriv;
    }

    Complete(STATUS_SUCCESS);
}

VOID
AsyncCreateLogStreamTest::OnReuse()
{
    KGuid zeroGuid(TRUE);
    _Log = 0;
    _LogStreamId = zeroGuid;
    _LogStreamType = zeroGuid;
    _LogStream = nullptr;
    _LogStreamPriv = 0;
}


//* AsyncDeleteLogStreamTest implementation
class AsyncDeleteLogStreamTest : public KAsyncContextBase
{
    K_FORCE_SHARED_WITH_INHERITANCE(AsyncDeleteLogStreamTest);

public:
    static AsyncDeleteLogStreamTest::SPtr
    Create();

    virtual VOID
    StartTest(
        __in RvdLog::SPtr const& Log,
        __in KGuid const& LogStreamId,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

    VOID
    TestCompletionCallback(
        __in KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletedContext);

protected:
    VOID
    OnStart();

    VOID
    OnReuse();

protected:
    RvdLog::SPtr _Log;
    KGuid _LogStreamId;
};


AsyncDeleteLogStreamTest::AsyncDeleteLogStreamTest()
{
}

AsyncDeleteLogStreamTest::~AsyncDeleteLogStreamTest()
{
}

AsyncDeleteLogStreamTest::SPtr
AsyncDeleteLogStreamTest::Create()
{
    return SPtr(_new(KTL_TAG_TEST,
        KtlSystem::GlobalNonPagedAllocator()) AsyncDeleteLogStreamTest());
}

VOID
AsyncDeleteLogStreamTest::StartTest(
    __in RvdLog::SPtr const& Log,
    __in KGuid const& LogStreamId,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)

{
    _Log = Log;
    _LogStreamId = LogStreamId;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
AsyncDeleteLogStreamTest::OnStart()
{
    NTSTATUS status = STATUS_SUCCESS;

    RvdLog::AsyncDeleteLogStreamContext::SPtr deleteStreamOp;
    KAsyncContextBase::CompletionCallback callback(this, &AsyncDeleteLogStreamTest::TestCompletionCallback);

    status = _Log->CreateAsyncDeleteLogStreamContext(deleteStreamOp);
    if (!NT_SUCCESS(status))
    {
        KDbgCheckpointWStatus((ULONG)-1, "CreateAsyncDeleteLogStreamContext() failed", status);
        Complete(status);
    }

    deleteStreamOp->StartDeleteLogStream(
        RvdLogStreamId(_LogStreamId),
        this,
        callback);

}

VOID
AsyncDeleteLogStreamTest::TestCompletionCallback(
    __in KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletedContext)
{
    UNREFERENCED_PARAMETER(Parent);

    NTSTATUS status = CompletedContext.Status();

    if (!NT_SUCCESS(status))
    {
        KDbgCheckpointWStatus((ULONG)-1, "StartDeleteLogStream() failed", status);
        Complete(status);
        return;
    }

    if(_Log->IsStreamIdValid(RvdLogStreamId(_LogStreamId)))
    {
        status = STATUS_UNSUCCESSFUL;
        KDbgCheckpointWStatus((ULONG)-1, "IsStreamIdValid() returned TRUE after a stream delete - failed", status);
        Complete(status);
        return;
    }

    Complete(STATUS_SUCCESS);
}

VOID
AsyncDeleteLogStreamTest::OnReuse()
{
    KGuid zeroGuid(TRUE);
    _Log = 0;
    _LogStreamId = zeroGuid;
}



//** Log stream I/O tests
VOID
DeactivatingQueueSink(
    KAsyncQueue<AsyncLogStreamIoContext>& DeactivatingQueue,
    AsyncLogStreamIoContext&              DroppedItem)
{
    UNREFERENCED_PARAMETER(DeactivatingQueue);

    DroppedItem.Release();      // See #1 enqueue() calls in SingleLogStreamAsyncIoTests
}

NTSTATUS
ReadNextLine(__in KTextFile::SPtr& File, __out KWString& Result)
{
    while (!File->EndOfFile())
    {
        NTSTATUS status = File->ReadLine(Result);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        if (Result.Length() < 2)
        {
            return STATUS_SUCCESS;
        }

        static_assert(sizeof(L"//") >= 4, "Size mismatch");
        if (RtlCompareMemory((WCHAR*)Result, L"//", 4) != 4)
        {
            return STATUS_SUCCESS;
        }
    }

    return STATUS_END_OF_FILE;
}

VOID
ParseScriptLine(__in KWString& Src, __in KArray<KWString>& Results)
{
    WCHAR*      srcPtr = (WCHAR*)Src;
    USHORT      srcLgt = Src.Length();
    KAssert((srcLgt & 0x01) == 0);
    Results.Clear();

    while (srcLgt > 0)
    {
        KWString    element(KtlSystem::GlobalNonPagedAllocator());
        WCHAR*      subStrPtr = srcPtr;

        while (srcLgt > 0)
        {
            if ((*srcPtr) == (WCHAR)':')
            {
                *srcPtr = 0;
                element = subStrPtr;
                *srcPtr = ':';
                srcLgt -= 2;
                srcPtr++;
                break;
            }

            srcPtr++;
            srcLgt -= 2;
            if (srcLgt == 0)
            {
                element = subStrPtr;
            }
        }

        if (element.Length() > 0)
        {
            Results.Append(element);
        }
    }
}

// BUG, richhas, xxxxx, Add script generation option
//** Main SingleLogStreamAsyncIoTests entry point
NTSTATUS
SingleLogStreamAsyncIoTests(
    __in WCHAR const LogDriveLetter,
    __in ULONG RepeatCount = 1,
    __in_opt KGuid *const LogId = nullptr,
    __in_opt LogState::SPtr *const ResultingLogState = nullptr,
    _In_opt_z_ WCHAR *const OptionalScriptPath = nullptr)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (OptionalScriptPath != nullptr)
    {
        // Special case - only one pass if a script is being passed
        RepeatCount = 1;
    }

    RvdLogManager::SPtr logManager;
    KEvent activateEvent(FALSE, FALSE);
    status = CreateLogManagerInstance(logManager);
    if (!NT_SUCCESS(status))
    {
        KDbgCheckpointWStatus((ULONG)-1, "CreateLogManagerInstance() failed", status);
        return status;
    }

    status = ActivateLogManagerInstance(activateEvent, logManager);
    if (!NT_SUCCESS(status))
    {
        KDbgCheckpointWStatus((ULONG)-1, "", status);
        return status;
    }

    KGuid diskId;
    status = GetLogDriveGuid(LogDriveLetter, diskId);
    if (!NT_SUCCESS(status))
    {
        KDbgCheckpointWStatus((ULONG)-1, "GetLogDriveGuid() failed", status);
        return status;
    }

    status = CleanAndPrepLog(LogDriveLetter);
    if (!NT_SUCCESS(status))
    {
        KDbgCheckpointWStatus((ULONG)-1, "CleanAndPrepLog() failed", status);
        return status;
    }

    KTextFile::SPtr             scriptFile;

    if (OptionalScriptPath != nullptr)
    {
        // A script path has been passed - try and open the file
        KWString kwsOptionalScriptPath(KtlSystem::GlobalNonPagedAllocator(), OptionalScriptPath);
        status = KTextFile::OpenSynchronous(
            kwsOptionalScriptPath,
            256,
            scriptFile,
            KTL_TAG_TEST,
            &KtlSystem::GlobalNonPagedAllocator());

        if (!NT_SUCCESS(status))
        {
            KDbgCheckpointWStatus((ULONG)-1, "KTextFile:OpenSynchronous failed", status);
            return(status);
        }
    }

    Synchronizer                syncObject;
    KGuid                       logId;
    KGuid                       logStreamId;
    KGuid                       logStreamTypeId;

    if (scriptFile == nullptr)
    {
        logStreamId.CreateNew();
        logStreamTypeId.CreateNew();

        if (LogId == nullptr)
        {
            logId.CreateNew();
        }
        else
        {
            logId = *LogId;
        }
    }
    else
    {
        //              LogId                               StreamId                            StreamType                        Count
        //I:{14575ca2-1f3f-42a8-9671-c2fca7c475a6}:{d9c6d447-b7cf-4585-808e-fcf276b417f3}:{763194bb-0974-4800-9a1f-15ffeb1d2490}:00001000
        //0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456
        //0         1         2         3         4         5         6         7         8         9         0         1         2
        //0         0         0         0         0         0         0         0         0         0         1         1         1
        KWString        line(KtlSystem::GlobalNonPagedAllocator());

        status = ReadNextLine(scriptFile, line);
        if (!NT_SUCCESS(status))
        {
            KDbgCheckpointWStatus((ULONG)-1, "ReadNextLine() failed", status);
            return status;
        }

        KArray<KWString>    elements(KtlSystem::GlobalNonPagedAllocator());
        ParseScriptLine(line, elements);
        if (elements.Count() != 5)
        {
            status = STATUS_UNSUCCESSFUL;
            KDbgCheckpointWStatus((ULONG)-1, "Script file error - I: line too short", status);
            return status;
        }

        if (*((WCHAR*)(elements[0])) != (WCHAR)'I')
        {
            status = STATUS_UNSUCCESSFUL;
            KDbgCheckpointWStatus((ULONG)-1, "Script file error - I: line must be first in file", status);
            return status;
        }

        GUID        guid;

        status = elements[1].ExtractGuidSuffix(guid);
        if (!NT_SUCCESS(status))
        {
            KDbgCheckpointWStatus((ULONG)-1, "Script file error - I: logid guid format error", status);
            return status;
        }
        logId = guid;

        status = elements[2].ExtractGuidSuffix(guid);
        if (!NT_SUCCESS(status))
        {
            KDbgCheckpointWStatus((ULONG)-1, "Script file error - I: streamid guid format error", status);
            return status;
        }
        logStreamId = guid;

        status = elements[3].ExtractGuidSuffix(guid);
        if (!NT_SUCCESS(status))
        {
            KDbgCheckpointWStatus((ULONG)-1, "Script file error - I: streamtype guid format error", status);
            return status;
        }
        logStreamTypeId = guid;

        ULONGLONG           countOfRecords = 0;
        if (!ToULonglong(elements[4], countOfRecords))
        {
            KDbgCheckpointWStatus((ULONG)-1, "Script file error - I: record count format error", status);
            return status;
        }
    }

    RvdLogConfig                defaultLogConfig;
        KInvariant(NT_SUCCESS(defaultLogConfig.Status()));

    LONGLONG                    logSize = defaultLogConfig.GetMinFileSize() * 4;
    KWString                    logType(KtlSystem::GlobalNonPagedAllocator(), L"RvdLog");
    RvdLog::SPtr                log;

    // Create Log
    {
        AsyncCreateLogTest::SPtr    createTest = AsyncCreateLogTest::Create();
        status = createTest->DoTest(logManager, diskId, logId, logType, logSize, &log);
        if (!NT_SUCCESS(status))
        {
            KDbgCheckpointWStatus((ULONG)-1, "CreateLogTest() failed", status);
            return status;
        }

        //((RvdLogManagerImp::RvdOnDiskLog*)(log.RawPtr()))->_DebugDisableAutoCheckpointing = TRUE;
        //((RvdLogManagerImp::RvdOnDiskLog*)(log.RawPtr()))->_DebugDisableTruncateCheckpointing = TRUE;
    }

    // First batch of log stream IO
    {
        AsyncCreateLogStreamTest::SPtr  createStreamTest = AsyncCreateLogStreamTest::Create();
        RvdLogStream::SPtr              logStream;

        createStreamTest->StartTest(log, logStreamId, logStreamTypeId, &logStream, nullptr, syncObject.AsyncCompletionCallback());
        syncObject.WaitForCompletion();
        if (!NT_SUCCESS(status))
        {
            KDbgCheckpointWStatus((ULONG)-1, "CreateLogStreamTest() failed", status);
            return status;
        }

        IoRequestGenerator*         ioGenerator = _new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator()) IoRequestGenerator();
        KFinally([&ioGenerator]()
        {
            _delete(ioGenerator);
            ioGenerator = nullptr;
        });

        // Set test constants
        //RandomGenerator::SetSeed();
        RandomGenerator::SetSeed(0);

        //const int maxIoQueueDepth = 4;
        const int maxIoQueueDepth = 256;

        // Configure IO request generator
        ioGenerator->_MaxUserIoBufferSize = 64 * 1024;
        ioGenerator->_MinUserIoBufferSize = 4 * 1024;

        ioGenerator->_MaxUserMetadataSize = 256;
        ioGenerator->_MinUserMetadataSize = 8;

        ioGenerator->_StartingAsn = 100;
        ioGenerator->_StartingVersion = 0;
        ioGenerator->_LogWriteReorderWindow = 16;
        //ioGenerator->_LogTailMaxLength = 500;
        ioGenerator->_LogTailMaxLength = 100;
        ioGenerator->_LogReadProbability = 5;

        // BUG, amith, xxxxx, _VersionChangeProbability not supported at this time because it fundmentally
        //                    conflicts with the truncation deferral approach - which only syncs to the
        //                    highest written ASN with no consideration of higher version of an ASN being
        //                    written after an initial write completion for such an ASN.
        ioGenerator->_VersionChangeProbability = 0;
        ioGenerator->Initialize();

        while (RepeatCount-- > 0)
        {
            AsyncLogStreamIoEngine::SPtr                            ioEngine = AsyncLogStreamIoEngine::Create();
            KAsyncQueue<AsyncLogStreamIoContext>::SPtr              ioRequestQueue;
            Synchronizer                                            ioRequestQueueSyncObject;

            status = KAsyncQueue<AsyncLogStreamIoContext>::Create(
                KTL_TAG_LOGGER,
                KtlSystem::GlobalNonPagedAllocator(),
                FIELD_OFFSET(AsyncLogStreamIoContext, _IoListEntry),
                ioRequestQueue);

            if (!NT_SUCCESS(status))
            {
                KDbgCheckpointWStatus((ULONG)-1, "KAsyncQueue<AsyncLogStreamIoContext>::Create() failed", status);
                return status;
            }

            status = ioRequestQueue->Activate(nullptr, ioRequestQueueSyncObject.AsyncCompletionCallback());
            if (!NT_SUCCESS(status))
            {
                KDbgCheckpointWStatus((ULONG)-1, "ioRequestQueue->Activate() failed", status);
                return status;
            }

            KFinally([&ioRequestQueue, &ioRequestQueueSyncObject]()
            {
                KAsyncQueue<AsyncLogStreamIoContext>::DroppedItemCallback sink(&DeactivatingQueueSink);
                ioRequestQueue->Deactivate(sink);
                NTSTATUS status = ioRequestQueueSyncObject.WaitForCompletion();
                if (!NT_SUCCESS(status))
                {
                    KDbgCheckpointWStatus((ULONG)-1, "ioRequestQueue->Deactivate() failed", status);
                }
                ioRequestQueue.Reset();
            });

            // Generate IO requests or load them from a script file
            ULONGLONG       totalBytesGenerated = 0;
            ULONG           verifyCount = 0;
            ULONG           truncateCount = 0;
            RvdLogAsn       LowestAsnToWrite = RvdLogAsn::Max();
            ULONGLONG       initialLogQuotaGateState = ((RvdLogManagerImp::RvdOnDiskLog*)(log.RawPtr()))->GetQuotaGateState();

            if (scriptFile == nullptr)
            {
                // No script - generate a work load
                ULONG           noOfIoRequests = 40000;
                KWString        logIdStr(KtlSystem::GlobalNonPagedAllocator(), logId);
                KWString        streamIdStr(KtlSystem::GlobalNonPagedAllocator(), logStreamId);
                KWString        streamTypeStr(KtlSystem::GlobalNonPagedAllocator(), logStreamTypeId);

#if DISPLAY_ON_CONSOLE
#else
                //              LogId                               StreamId                            StreamType                        Count
                //I:{14575ca2-1f3f-42a8-9671-c2fca7c475a6}:{d9c6d447-b7cf-4585-808e-fcf276b417f3}:{763194bb-0974-4800-9a1f-15ffeb1d2490}:00001000
                //0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456
                //0         1         2         3         4         5         6         7         8         9         0         1         2
                //0         0         0         0         0         0         0         0         0         0         1         1         1
#if defined(PLATFORM_UNIX)
                KDbgPrintf("I:%s:%s:%s:%08u\n", Utf16To8((WCHAR*)logIdStr).c_str(), Utf16To8((WCHAR*)streamIdStr).c_str(), Utf16To8((WCHAR*)streamTypeStr).c_str(), noOfIoRequests);
#else
                KDbgPrintf("I:%S:%S:%S:%08u\n", (WCHAR*)logIdStr, (WCHAR*)streamIdStr, (WCHAR*)streamTypeStr, noOfIoRequests);
#endif
#endif
                while (noOfIoRequests--)
                {
                    AsyncLogStreamIoContext::SPtr ioreq;
                    status = ioGenerator->GenerateNewRequest(defaultLogConfig.GetMaxRecordSize(), ioreq);
                    if(!NT_SUCCESS(status))
                    {
                        KDbgCheckpointWStatus((ULONG)-1, "SingleLogStreamAsyncIoTests::GenerateNewRequest() failed", status);
                        return status;
                    }

                    if (ioreq->_AsyncIoType == IoType::Write)
                    {
#if DISPLAY_ON_CONSOLE
#else
                        //T        ASN              VER       MDSIZE   IOBUFSIZE  R
                        //W:00000000000000000000:0000000000:0000000000:0000000000:V
                        //W:00000000000000000000:0000000000:0000000000:0000000000:N
                        //012345678901234567890123456789012345678901234567890123456
                        //0         1         2         3         4         5
                        if (ioreq->_DoReadAfterWriteVerify)
                        {
                            KDbgPrintf(
                                "W:%020I64u:%010I64u:%010u:%010u:V\n",
                                ioreq->_RecordAsn.Get(),
                                ioreq->_Version,
                                ioreq->GetMetadataSize(),
                                ioreq->GetIoBufferSize());
                        }
                        else
                        {
                            KDbgPrintf(
                                "W:%020I64u:%010I64u:%010u:%010u:N\n",
                                ioreq->_RecordAsn.Get(),
                                ioreq->_Version,
                                ioreq->GetMetadataSize(),
                                ioreq->GetIoBufferSize());
                        }
#endif
                        if (LowestAsnToWrite > ioreq->_RecordAsn)
                        {
                            LowestAsnToWrite = ioreq->_RecordAsn;
                        }

                        totalBytesGenerated += ioreq->GetIoBufferSize();
                        totalBytesGenerated += RvdDiskLogConstants::RoundUpToBlockBoundary(ioreq->GetMetadataSize());
                    }
                    else if (ioreq->_AsyncIoType == IoType::Truncate)
                    {
#if DISPLAY_ON_CONSOLE
#else
                        //         ASN
                        //T:00000000000000000000
                        //012345678901234567890123456789012345678901234567890123456
                        //0         1         2         3         4         5
                        KDbgPrintf("T:%020I64u\n", ioreq->_RecordAsn.Get());
#endif
                    }
                    else
                    {
                        KInvariant(FALSE);
                    }

                    status = ioRequestQueue->Enqueue(*(ioreq.Detach()));    // #1 The IoEngine must cause the release ref
                    if(!NT_SUCCESS(status))
                    {
                        KDbgCheckpointWStatus((ULONG)-1, "ioRequestQueue->Enqueue() failed", status);
                        return status;
                    }
                }
#if DISPLAY_ON_CONSOLE
#else
                KDbgPrintf("E:\n");
#endif
            }
            else
            {
                // Load ioRequestQueue from operations in the passed script file
                BOOLEAN                         eof = FALSE;
                AsyncLogStreamIoContext::SPtr   ioreq;

                while (!scriptFile->EndOfFile() && !eof)
                {
                    KWString        line(KtlSystem::GlobalNonPagedAllocator());

                    status = ReadNextLine(scriptFile, line);
                    if (!NT_SUCCESS(status))
                    {
                        KDbgCheckpointWStatus((ULONG)-1, "ReadNextLine() failed", status);
                        return status;
                    }

                    KArray<KWString>    elements(KtlSystem::GlobalNonPagedAllocator());
                    ParseScriptLine(line, elements);
                    if (elements.Count() == 0)
                    {
                        status = STATUS_UNSUCCESSFUL;
                        KDbgCheckpointWStatus((ULONG)-1, "Script file error - line too short", status);
                        return status;
                    }

                    switch (*(WCHAR*)(elements[0]))
                    {
                        case 'T':
                            {
                                if (elements.Count() != 2)
                                {
                                    status = STATUS_UNSUCCESSFUL;
                                    KDbgCheckpointWStatus((ULONG)-1, "Script file T: line error", status);
                                    return status;
                                }

                                ULONGLONG asn;
                                if (!ToULonglong(elements[1], asn))
                                {
                                    status = STATUS_UNSUCCESSFUL;
                                    KDbgCheckpointWStatus((ULONG)-1, "Script file T: line error - asn invalid", status);
                                    return status;
                                }

                                status = ioGenerator->GenerateTruncateRequest(ioreq, asn);
                                if(!NT_SUCCESS(status))
                                {
                                    KDbgCheckpointWStatus(
                                        (ULONG)-1,
                                        "SingleLogStreamAsyncIoTests::GenerateTruncateRequest() failed",
                                        status);

                                    return status;
                                }

                                truncateCount++;
                            }
                            break;

                        case 'W':
                            {
                                //T        ASN              VER       MDSIZE   IOBUFSIZE  R
                                if (elements.Count() != 6)
                                {
                                    status = STATUS_UNSUCCESSFUL;
                                    KDbgCheckpointWStatus((ULONG)-1, "Script file W: line error", status);
                                    return status;
                                }

                                ULONGLONG asn;
                                if (!ToULonglong(elements[1], asn))
                                {
                                    status = STATUS_UNSUCCESSFUL;
                                    KDbgCheckpointWStatus((ULONG)-1, "Script file W: line error - asn invalid", status);
                                    return status;
                                }

                                ULONG version;
                                if (!ToULong(elements[2], version))
                                {
                                    status = STATUS_UNSUCCESSFUL;
                                    KDbgCheckpointWStatus((ULONG)-1, "Script file W: line error - version invalid", status);
                                    return status;
                                }

                                ULONG metadataSize;
                                if (!ToULong(elements[3], metadataSize))
                                {
                                    status = STATUS_UNSUCCESSFUL;
                                    KDbgCheckpointWStatus((ULONG)-1, "Script file W: line error - metadata size invalid", status);
                                    return status;
                                }

                                ULONG ioBufferSize;
                                if (!ToULong(elements[4], ioBufferSize))
                                {
                                    status = STATUS_UNSUCCESSFUL;
                                    KDbgCheckpointWStatus((ULONG)-1, "Script file W: line error - ioBuffer size invalid", status);
                                    return status;
                                }

                                BOOLEAN doWriteVerify = (*((WCHAR*)(elements[5])) == 'V');
                                if (doWriteVerify)
                                {
                                    verifyCount++;
                                }

                                status = ioGenerator->GenerateWriteRequest(
                                    ioreq,
                                    asn,
                                    version,
                                    metadataSize,
                                    ioBufferSize,
                                    doWriteVerify);

                                if(!NT_SUCCESS(status))
                                {
                                    KDbgCheckpointWStatus(
                                        (ULONG)-1,
                                        "SingleLogStreamAsyncIoTests::GenerateWriteRequest() failed",
                                        status);

                                    return status;
                                }

                                if (LowestAsnToWrite > ioreq->_RecordAsn)
                                {
                                    LowestAsnToWrite = ioreq->_RecordAsn;
                                }

                                totalBytesGenerated += ioreq->GetIoBufferSize();
                                totalBytesGenerated += RvdDiskLogConstants::RoundUpToBlockBoundary(ioreq->GetMetadataSize());
                            }
                            break;

                        case 'E':
                            eof = TRUE;
                            break;
                    }

                    if (ioreq != nullptr)
                    {
                        status = ioRequestQueue->Enqueue(*(ioreq.Detach()));    // #1 The IoEngine must cause the release ref
                        if(!NT_SUCCESS(status))
                        {
                            KDbgCheckpointWStatus((ULONG)-1, "ioRequestQueue->Enqueue() failed", status);
                            return status;
                        }
                    }
                }
            }

            // Start IO engine
            ioEngine->Reuse();

            ULONGLONG startTime = KNt::GetTickCount64();
            ioEngine->StartIoEngine(
                logStream,
                ioRequestQueue,
                maxIoQueueDepth,
                LowestAsnToWrite,
                nullptr,
                syncObject.AsyncCompletionCallback());

            status = syncObject.WaitForCompletion();
            ULONGLONG stopTime = KNt::GetTickCount64();

            ULONGLONG       secs = __max(((stopTime - startTime) / 1000), 1);
            ULONGLONG       mbPerSec = (totalBytesGenerated / (1024 * 1024)) / secs;
            KDbgPrintf(
                "SingleLogStreamAsyncIoTests: Pass: %u took %I64u seconds; %I64u bytes written; %I64u mb/sec\n",
                RepeatCount + 1,
                secs,
                totalBytesGenerated,
                mbPerSec);

            if(!NT_SUCCESS(status))
            {
                KDbgCheckpointWStatus((ULONG)-1, "Log stream IO test using AsyncLogStreamIoEngine failed", status);
                return status;
            }

            while (!log->IsLogFlushed())
            {
                KNt::Sleep(0);
            }

            ULONGLONG       finalLogQuotaGateState = ((RvdLogManagerImp::RvdOnDiskLog*)(log.RawPtr()))->GetQuotaGateState();
            if (finalLogQuotaGateState != initialLogQuotaGateState)
            {
                KDbgPrintf(
                    "SingleLogStreamAsyncIoTests: QuotaGate State Mismatch: Initial %I64u; Final: %I64u\n",
                    initialLogQuotaGateState,
                    finalLogQuotaGateState);

                return STATUS_UNSUCCESSFUL;
            }
        }

        if (ResultingLogState == nullptr)
        {
            logStream.Reset();
            AsyncDeleteLogStreamTest::SPtr deleteStreamTest = AsyncDeleteLogStreamTest::Create();

            deleteStreamTest->StartTest(log, logStreamId, nullptr, syncObject.AsyncCompletionCallback());
            status = syncObject.WaitForCompletion();
            if (!NT_SUCCESS(status))
            {
                KDbgCheckpointWStatus((ULONG)-1, "BasicLogStreamTestScenario() failed", status);
                return status;
            }
        }
        else
        {
            RvdLogManagerImp::RvdOnDiskLog* logImpPtr = (RvdLogManagerImp::RvdOnDiskLog*)log.RawPtr();
            status = logImpPtr->UnsafeSnapLogState(*ResultingLogState, KTL_TAG_LOGGER, KtlSystem::GlobalNonPagedAllocator());
            logStream.Reset();

            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("BasicLogStreamTestScenario: UnsafeSnapLogState failed: %i\n", __LINE__);
                KDbgCheckpointWStatus((ULONG)-1, "UnsafeSnapLogState() failed", status);
                return status;
            }
        }
    }

    log = nullptr;

    // Delete the Log
    if (ResultingLogState == nullptr)
    {
        AsyncDeleteLogTest::SPtr    deleteTest = AsyncDeleteLogTest::Create();
        status = deleteTest->DoTest(logManager, diskId, logId);
        if (!NT_SUCCESS(status))
        {
            KDbgCheckpointWStatus((ULONG)-1, "DeleteLogTest() failed", status);
            return status;
        }
    }

    logManager->Deactivate();
    activateEvent.WaitUntilSet();

    if (ResultingLogState == nullptr)
    {
        status = CleanAndPrepLog(LogDriveLetter);
        if (!NT_SUCCESS(status))
        {
            KDbgCheckpointWStatus((ULONG)-1, "CleanAndPrepLog() failed", status);
            return status;
        }
    }

    return STATUS_SUCCESS;
}


//** Main Test Entry Point: LogStreamAsyncIoTests
NTSTATUS
LogStreamAsyncIoTests(__in int argc, __in_ecount(argc) WCHAR* args[])
{
    NTSTATUS status = STATUS_SUCCESS;

    KDbgPrintf("LogStreamAsyncIoTests: Starting\n");

    EventRegisterMicrosoft_Windows_KTL();
    KFinally([]()
    {
        EventUnregisterMicrosoft_Windows_KTL();
    });

    KtlSystem* underlyingSystem;
    status = KtlSystem::Initialize(FALSE,
                                  &underlyingSystem);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("LogStreamAsyncIoTests: KtlSystem::Initialize Failed: %i\n", __LINE__);
        return status;
    }

    underlyingSystem->SetStrictAllocationChecks(TRUE);

    if (argc < 1)
    {
        KDbgPrintf("LogStreamAsyncIoTests: Drive Letter Test Parameter Missing: %i\n", __LINE__);
        status = STATUS_INVALID_PARAMETER_2;
    }

#ifdef FEATURE_TEST
    ULONG   testIterations = 8;
#else
    ULONG   testIterations = 2;
#endif	
    if (argc > 1)
    {
        KWString    t(KtlSystem::GlobalNonPagedAllocator(), args[1]);
        if (!ToULong(t, testIterations))
        {
            KDbgPrintf("LogStreamAsyncIoTests: Iterations Test Parameter Missing: %i\n", __LINE__);
            status = STATUS_INVALID_PARAMETER_2;
        }
    }

    WCHAR*      scriptPath = nullptr;
    //WCHAR*      scriptPath = L"\\SystemRoot\\temp\\log.log";
    if (argc > 2)
    {
        scriptPath = args[2];
    }

    if (NT_SUCCESS(status))
    {
        KGuid           testLogId(TestLogIdGuid);

        for (ULONG i = 0; i < testIterations; i++)
        {
            LogState::SPtr       resultingLogState;

            if ((status = SingleLogStreamAsyncIoTests(*(args[0]), 1, &testLogId, &resultingLogState, scriptPath)) != STATUS_SUCCESS)
            {
                KDbgPrintf("LogStreamAsyncIoTests: SingleLogStreamAsyncIoTests FAILED with status : 0x%0x; %i\n", status, __LINE__);
            }
            else
            {
                LogState::SPtr   verifiedState;

                status = VerifyDiskLogFile(
                    KtlSystem::GlobalNonPagedAllocator(),
                    args[0],
                    RvdLogRecoveryReportDetail::Normal,
                    TRUE,           // Use user record verification method for this test
                    &verifiedState);

                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf("LogStreamAsyncIoTests: VerifyDiskLogFile() failed: 0x%08X\n", status);
                }

                else if (resultingLogState != nullptr)
                {
                    status = ReportLogStateDifferences(
                        *verifiedState,
                        *resultingLogState);

                    if (!NT_SUCCESS(status))
                    {
                        KDbgPrintf("LogStreamAsyncIoTests: ReportLogStateDifferences failed: %i\n", __LINE__);
                    }
                }

                verifiedState = nullptr;
            }

            resultingLogState = nullptr;
        }
    }

    KtlSystem::Shutdown();

    KDbgPrintf("LogStreamAsyncIoTests: Completed");
    return status;
}

