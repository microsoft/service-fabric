// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#ifdef UNIFY
#define UPASSTHROUGH 1
#endif

#define VERBOSE 1

#include "KtlLogShimKernel.h"

//
// CoalesceRecords
//
const ULONG OverlayStream::CoalesceRecords::_CoalescingLinkageOffset = FIELD_OFFSET(OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext,
                                                                                    _CoalescingListEntry);

const ULONG OverlayStream::CoalesceRecords::_FlushingRecordsLinkageOffset = FIELD_OFFSET(OverlayStream::CoalesceRecords::AsyncFlushingRecordsContext,
                                                                                    _FlushingListEntry);

//
// AsyncReadContext
//
VOID
OverlayStream::CoalesceRecords::AsyncReadContextCoalesce::OnCompleted(
)
{
    if (_RequestRefAcquired)
    {
        _OverlayStream->ReleaseRequestRef();
        _RequestRefAcquired = FALSE;
    }
}

VOID
OverlayStream::CoalesceRecords::AsyncReadContextCoalesce::FSMContinue(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &OverlayStream::CoalesceRecords::AsyncReadContextCoalesce::OperationCompletion);

    switch (_State)
    {
        case Initial:
        {
            ULONGLONG version;

            Status = _CoalesceRecords->FindDataInFlushingRecords(_RecordAsn, _ReadType, version, *_MetaDataBuffer, *_IoBuffer);
            if (NT_SUCCESS(Status))
            {
                if (_ActualRecordAsn != NULL)
                {
                    *_ActualRecordAsn = _RecordAsn;
                }

                if (_Version != NULL)
                {
                    *_Version = version;
                }

            } else {
                if (Status == STATUS_NOT_FOUND)
                {
                    KDbgCheckpointWDataInformational(_OverlayStream->GetActivityId(), 
                        "OverlayStream::CoalesceRecords::AsyncReadContextCoalesce::FSMContinue", Status, _State, (ULONGLONG)this, _RecordAsn.Get(), _ReadType);
                } else {
                    KTraceFailedAsyncRequest(Status, this, _RecordAsn.Get(), _ReadType);
                }
            }

            Complete(Status);
            break;
        }

        default:
        {
            KInvariant(FALSE);
        }
    }    
}

VOID
OverlayStream::CoalesceRecords::AsyncReadContextCoalesce::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, _RecordAsn.Get());
}

VOID
OverlayStream::CoalesceRecords::AsyncReadContextCoalesce::OperationCompletion(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase& Async
    )
{
    NTSTATUS status = Async.Status();

    FSMContinue(status); 
}

VOID
OverlayStream::CoalesceRecords::AsyncReadContextCoalesce::OnStart(
    )
{
    KInvariant(_State == Initial);

    _RequestRefAcquired = FALSE;
    
    NTSTATUS status = _OverlayStream->TryAcquireRequestRef();
    if (! NT_SUCCESS(status))
    {
        Complete(status);
        return;
    }
    _RequestRefAcquired = TRUE;

    //
    // Do not go directly to the FSMContinue() but instead wait on this
    // event that always set. The reason for this is to force the async
    // into the OverlayStream apartment. In this way it will never run
    // in parallel with an Append or Flush. By doing this we know it is
    // safe to look at the currently coalescing record without a lock.
    //
    KAsyncContextBase::CompletionCallback completion(this, &OverlayStream::CoalesceRecords::AsyncReadContextCoalesce::OperationCompletion);
    _WaitContext->StartWaitUntilSet(_OverlayStream.RawPtr(),
                                    completion);
}

VOID
OverlayStream::CoalesceRecords::AsyncReadContextCoalesce::StartRead(
    __in RvdLogAsn,
    __out_opt ULONGLONG* const ,
    __out KBuffer::SPtr& ,
    __out KIoBuffer::SPtr& ,
    __in_opt KAsyncContextBase* const ,
    __in_opt KAsyncContextBase::CompletionCallback )
{
    KInvariant(FALSE);
}

VOID
OverlayStream::CoalesceRecords::AsyncReadContextCoalesce::StartRead(
    __in RvdLogAsn ,
    __in RvdLogStream::AsyncReadContext::ReadType ,
    __out_opt RvdLogAsn* const ,
    __out_opt ULONGLONG* const ,
    __out KBuffer::SPtr& ,
    __out KIoBuffer::SPtr& ,
    __in_opt KAsyncContextBase* const ,
    __in_opt KAsyncContextBase::CompletionCallback )
{
    KInvariant(FALSE);
}

VOID
OverlayStream::CoalesceRecords::AsyncReadContextCoalesce::StartRead(
    __in RvdLogAsn RecordAsn,
    __in RvdLogStream::AsyncReadContext::ReadType Type,
    __out_opt RvdLogAsn* const ActualRecordAsn,
    __out_opt ULONGLONG* const Version,
    __out KBuffer::SPtr& MetaDataBuffer,
    __out KIoBuffer::SPtr& IoBuffer,
    // ParentAsync is always OverlayStream object to ensure proper ordering
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _State = Initial;
    
    if (ActualRecordAsn != NULL)
    {
        *ActualRecordAsn = RvdLogAsn::Null();
    }

    if (Version != NULL)
    {
        *Version = 0;
    }

    MetaDataBuffer = nullptr;
    IoBuffer = nullptr;
    
    _ReadType = Type;
    _RecordAsn = RecordAsn;
    _ActualRecordAsn = ActualRecordAsn;
    _Version = Version;
    _MetaDataBuffer = &MetaDataBuffer;
    _IoBuffer = &IoBuffer;  
    
    Start(_OverlayStream.RawPtr(), CallbackPtr);
}

OverlayStream::CoalesceRecords::AsyncReadContextCoalesce::~AsyncReadContextCoalesce()
{
}

VOID
OverlayStream::CoalesceRecords::AsyncReadContextCoalesce::OnReuse(
    )
{
    _WaitContext->Reuse();
}

OverlayStream::CoalesceRecords::AsyncReadContextCoalesce::AsyncReadContextCoalesce(
    __in OverlayStream& OS,
    __in CoalesceRecords& CR
    )
{
    NTSTATUS status;

    status = CR._AlwaysSetEvent.CreateWaitContext(GetThisAllocationTag(), GetThisAllocator(), _WaitContext);
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }
    
    _OverlayStream = &OS;
    _CoalesceRecords = &CR;
    
}

NTSTATUS
OverlayStream::CoalesceRecords::CreateAsyncReadContext(
    __out AsyncReadContextCoalesce::SPtr& Context
    )
{
    NTSTATUS status;
    OverlayStream::CoalesceRecords::AsyncReadContextCoalesce::SPtr context;

    Context = nullptr;

    status = _OverlayStream->TryAcquireRequestRef();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }
    KFinally([&] { _OverlayStream->ReleaseRequestRef(); });
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncReadContextCoalesce(*_OverlayStream, *this);
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( GetActivityId(), status, this, GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    Context = context.RawPtr();
    
    return(STATUS_SUCCESS); 
}


//
// AsyncAppendOrFlushContext
//
VOID
OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext::CompleteRequest(
    __in NTSTATUS Status
    )
{
    Complete(Status);
}

VOID
OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext::OnCompleted(
)
{
    _DestagingWriteContext = nullptr;
    
    _RecordMetadataBuffer = nullptr;
    _RecordIoBuffer = nullptr;
    _LLSource.Clear();

    if (_CoalesceIoBuffer)
    {
        _ThrottledAllocator->FreeKIoBuffer(GetActivityId(), _CoalesceIoBuffer->QuerySize());
    }
    _CoalesceIoBuffer = nullptr;

    if (_RequestRefAcquired)
    {
        _OverlayStream->ReleaseRequestRef();
        _RequestRefAcquired = FALSE;
    }
}

VOID
OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext::AppendFSMContinue(
    __in NTSTATUS Status
)
{
    OverlayStream::CoalesceRecords::AsyncFlushingRecordsContext::SPtr flushingRecordsContext = _CoalesceRecords->GetFlushingRecordsContext();
            
    KAsyncContextBase::CompletionCallback completion(this, &OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext::OperationCompletion);

    //
    // FSM is entered only when it is this async's turn to run. All
    // asyncs (Append and Flush) run within the same apartment and so
    // only one can be active at one time. Furthermore, asyncs are not
    // dispatched to their FSM until all asyncs that were received
    // previously have finished their work. In the case of AppendAsync
    // this is completing the copying of data into the coalescing
    // record; this may or may not cause the async to be completed. In
    // the case where the async isn't completed (due to the fact that
    // the coalescing record has not yet been flushed) the async will
    // be placed on the _CoalescingWrite list. When that coalescing
    // record is flushed then the async is completed. The functionality
    // for only allowing one async to run to finish at a time is
    // implemented by use of an AsyncEvent and keeping the waiting
    // asyncs on the _WaitingCoalescingWrite list. See OnStart() method
    // for this implementation.
    //

#ifdef UDRIVER
    KDbgCheckpointWData(GetActivityId(), "AppendFSMContinue", Status,
        (ULONGLONG)_AppendState,
        (ULONGLONG)this,
        (ULONGLONG)_LLSource.GetStreamBlockHeader() ? _LLSource.GetStreamBlockHeader()->StreamOffset : 0,
        (ULONGLONG)_LLSource.GetStreamBlockHeader() ? _LLSource.GetStreamBlockHeader()->HighestOperationId : 0);
#endif
    
    KInvariant((_AppendState == AppendState::AppendWritingDirectly) || (_CoalesceRecords->IsCurrentlyExecuting(*this)));

    if (! NT_SUCCESS(Status))
    {
        KTraceFailedAsyncRequest(Status, this, _AppendState, 0);
        // Fall through and let the individual states handle errors
    }

    switch (_AppendState)
    {
        case AppendState::AppendInitial:
        {
            //
            // Move the reservation from AppendContext into the CoalesceRecord. Reservation will be relieved when the
            // CoalesceRecord containing this data is actually flushed.
            //          
            _CoalesceRecords->AddReserveToUse(_ReserveToUse);

            //
            // Prepare the source record for copy
            //
            ULONG coreLoggerOffset = _DedicatedLogContainer->QueryUserRecordSystemMetadataOverhead();
            _LLSource.Initialize(coreLoggerOffset, coreLoggerOffset, *_RecordMetadataBuffer, _RecordIoBuffer.RawPtr());
            Status = _LLSource.InitializeExisting(FALSE);

            if (! NT_SUCCESS(Status))
            {
                //
                // This async has an error, complete it and kick off next one
                //
                KTraceFailedAsyncRequest(Status, this, _AppendState, 0);

                //
                // Reserve has been given to the _CoalesceRecords which will use it up on the next flush
                //
                _ReserveToUse = 0;

                //
                // Start the next operation and fail this one
                //
                _CoalesceRecords->RemoveWriteFromWaitingList(*this);
                _CoalesceRecords->StartNextWrite();
                Complete(Status);
                return;
            }
            _LLSource.InitializeSourceForCopy();

#ifdef UDRIVER
            KDbgCheckpointWData(GetActivityId(), "OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext::FSMContinue AcceptRecord", Status,
                (ULONGLONG)(((ULONG)_ForceFlush * 0x100000000) + _LLSource.GetDataSize()),
                (ULONGLONG)this,
                (ULONGLONG)_LLSource.GetStreamBlockHeader()->StreamOffset,
                (ULONGLONG)_LLSource.GetStreamBlockHeader()->HighestOperationId);

            KDbgCheckpointWData(GetActivityId(), "OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext::FSMContinue CoalesceState", Status,
                (ULONGLONG)_CoalesceRecords->GetDestinationDataSize(),
                (ULONGLONG)this,
                (ULONGLONG)_CoalesceRecords->GetDestinationStreamBlockHeader() ? _CoalesceRecords->GetDestinationStreamBlockHeader()->StreamOffset : 0,
                (ULONGLONG)_CoalesceRecords->GetDestinationStreamBlockHeader() ? _CoalesceRecords->GetDestinationStreamBlockHeader()->HighestOperationId : 0);
#endif
            
            _KIoBufferAllocationOffset = 0;

            if (_ForceFlush &&
                ((! flushingRecordsContext) ||
                 (flushingRecordsContext->IsCoalesceWritesEmpty())))
            {
                //
                // If the write is one where it should be force flushed and there is nothing being coalesced
                // then it is easiest to skip to just writing this record directly. Take back the reserve so it can 
                // be used on the direct write and free up any memory allocated from the pool
                //
                _CoalesceRecords->SubtractReserveToUse(_ReserveToUse);

                _AppendState = AppendState::AppendPerformDirectWrite;
                goto PerformDirectWrite;
            }

            //
            // Reserve has been given to the coalesce record
            //
            _ReserveToUse = 0;

            _AppendState = AppendState::AppendCopyRecordData;
            // Fall through
        }

        case AppendState::AppendCopyRecordData:
        {
 CopyRecordData:
            if (! _CoalesceRecords->_MetadataBufferX) 
            {
                //
                // Initialize new CoaleceRecords buffers if needed. This allocates the metadata buffer
                // but not any Io buffers.
                //
                KInvariant(! flushingRecordsContext);

                Status = _CoalesceRecords->Initialize();

                if (! NT_SUCCESS(Status))
                {
                    //
                    // If we hit an error then fail the async and start the next one
                    //
                    KTraceFailedAsyncRequest(Status, this, _AppendState, 0);
                    _CoalesceRecords->RemoveWriteFromWaitingList(*this);
                    _CoalesceRecords->StartNextWrite();
                    Complete(Status);
                    return;
                }

                flushingRecordsContext = _CoalesceRecords->GetFlushingRecordsContext();
            }

            //
            // Update destination and verify source headers
            //
            KLogicalLogInformation::StreamBlockHeader* sourceStreamBlockHeader = _LLSource.GetStreamBlockHeader();
            KLogicalLogInformation::StreamBlockHeader* destinationStreamBlockHeader = _CoalesceRecords->GetDestinationStreamBlockHeader();

#ifdef UDRIVER
            KDbgCheckpointWData(GetActivityId(),
                                "OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext::FSMContinue CoalesceState2", Status,
                                (ULONGLONG)_LLSource.GetDataConsumed(),
                                (ULONGLONG)this,
                                (ULONGLONG)destinationStreamBlockHeader ? destinationStreamBlockHeader->StreamOffset : 0,
                                (ULONGLONG)destinationStreamBlockHeader ? destinationStreamBlockHeader->HighestOperationId : 0);
#endif
            if (destinationStreamBlockHeader->StreamOffset == 0)
            {
                //
                // For first record setup the stream offset in the coalesced record header
                //
                _CoalesceRecords->UpdateDestinationHeaderInformation(sourceStreamBlockHeader->StreamOffset + _LLSource.GetDataConsumed(),
                                                                     sourceStreamBlockHeader->HighestOperationId,
                                                                     sourceStreamBlockHeader->HeadTruncationPoint);
            }
            else
            {
                //
                // For subsequent records, verify that the stream offsets are contiguous and that the 
                // operation id is increasing sequentially
                //
                ULONGLONG nextStreamOffset = destinationStreamBlockHeader->StreamOffset +
                                             _CoalesceRecords->GetDestinationDataSize();

                if (nextStreamOffset != sourceStreamBlockHeader->StreamOffset)
                {
                    //
                    // If we hit an error then fail the flushing and current asyncs and start the next one
                    //
                    KTraceFailedAsyncRequest(Status, this, nextStreamOffset, _LLSource.GetStreamBlockHeader()->StreamOffset);
                    flushingRecordsContext->CompleteFlushingAsyncs(Status);
                    flushingRecordsContext = nullptr;
                    _CoalesceRecords->ResetFlushingRecordsContext();                    
                    _CoalesceRecords->ClearDestination();

                    _CoalesceRecords->RemoveWriteFromWaitingList(*this);
                    _CoalesceRecords->StartNextWrite();
                    Complete(Status);
                    return;
                }

                if ((destinationStreamBlockHeader->HighestOperationId + 1) != sourceStreamBlockHeader->HighestOperationId)
                {
                    //
                    // If we hit an error then fail the flushing and current asyncs and start the next one
                    //
                    KTraceFailedAsyncRequest(Status, this, destinationStreamBlockHeader->HighestOperationId + 1,
                                             sourceStreamBlockHeader->HighestOperationId);
                    flushingRecordsContext->CompleteFlushingAsyncs(Status);
                    flushingRecordsContext = nullptr;
                    _CoalesceRecords->ResetFlushingRecordsContext();                    
                    _CoalesceRecords->ClearDestination();

                    _CoalesceRecords->RemoveWriteFromWaitingList(*this);
                    _CoalesceRecords->StartNextWrite();
                    Complete(Status);
                    return;
                }
            }

#ifdef UDRIVER
            KDbgCheckpointWData(GetActivityId(),
                                "OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext::FSMContinue CoalesceState3", Status,
                                (ULONGLONG)_CoalesceRecords->GetDestinationDataSize(),
                                (ULONGLONG)this,
                                (ULONGLONG)destinationStreamBlockHeader ? destinationStreamBlockHeader->StreamOffset : 0,
                                (ULONGLONG)destinationStreamBlockHeader ? destinationStreamBlockHeader->HighestOperationId : 0);

            KDbgCheckpointWData(GetActivityId(),
                                "OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext::FSMContinue UpdateCoalesceRecord", Status,
                                (ULONGLONG)_CoalesceRecords->GetDestinationDataSize(),
                                (ULONGLONG)this,
                                (ULONGLONG)destinationStreamBlockHeader->StreamOffset,
                                (ULONGLONG)destinationStreamBlockHeader->HighestOperationId);
#endif

            //
            // Assign part of _CoalesceIoBuffer to _CoalesceRecords so that we can build the coalesce buffers
            //
            ULONG sizeToAssign;
            ULONG ioBufferSpaceLeftInDestination = _CoalesceRecords->GetDestinationFullRecordSpaceLeft();

            if (_CoalesceIoBuffer)
            {
                if (_LLSource.GetDataLeftInSource() > _CoalesceRecords->GetSpaceLeftInDestination())
                {
                    //
                    // There is not enough room in the destination for
                    // the whole record so move ownership of the buffer
                    // from the AppendAsync into the CoalesceBuffer
                    //
                    if (_LLSource.GetDataLeftInSource() <= ioBufferSpaceLeftInDestination)
                    {
                        //
                        // Source record would completely fit into destination buffer so assign the whole _CoalesceIoBuffer
                        // to the _CoalesceBuffer
                        //
                        sizeToAssign = _CoalesceIoBuffer->QuerySize() - _KIoBufferAllocationOffset;
                        if (sizeToAssign > ioBufferSpaceLeftInDestination)
                        {
                            sizeToAssign = ioBufferSpaceLeftInDestination;
                        }
                        KInvariant((sizeToAssign + _CoalesceRecords->GetSpaceLeftInDestination()) >= _LLSource.GetDataLeftInSource());
                    } 
                    else 
                    {
                        //
                        // Source record will not completely fit into destination and so we need to break up the
                        // allocation into part for this coalesce record and then part for next coalesce record
                        //
                        sizeToAssign = ioBufferSpaceLeftInDestination;
                    }

#ifdef UDRIVER
                    KDbgCheckpointWData(GetActivityId(),
                        "ExtendIoBuffer", Status,
                        (ULONGLONG)_CoalesceRecords->GetIoBufferOffsetInDestination(),
                        (ULONGLONG)this,
                        (ULONGLONG)_KIoBufferAllocationOffset,
                        (ULONGLONG)sizeToAssign);
#endif

                    Status = _CoalesceRecords->ExtendDestinationIoBuffer(*_CoalesceIoBuffer,
                                                                         _KIoBufferAllocationOffset,
                                                                         sizeToAssign);
                    if (! NT_SUCCESS(Status))
                    {
                        KTraceFailedAsyncRequest(Status, this, _KIoBufferAllocationOffset, sizeToAssign);
                        flushingRecordsContext->CompleteFlushingAsyncs(Status);
                        flushingRecordsContext = nullptr;
                        _CoalesceRecords->ResetFlushingRecordsContext();                    
                        _CoalesceRecords->ClearDestination();

                        _CoalesceRecords->RemoveWriteFromWaitingList(*this);
                        _CoalesceRecords->StartNextWrite();
                        Complete(Status);
                        return;
                    }

                    _KIoBufferAllocationOffset += sizeToAssign;
                } else {
                    //
                    // Since the entire record can be accomodated in
                    // the CoalesceBuffer then there is no need to use
                    // the buffer passed.
                    //
                    _ThrottledAllocator->FreeKIoBuffer(GetActivityId(), _CoalesceIoBuffer->QuerySize());
                    _CoalesceIoBuffer = nullptr;
                }
            }
            //
            // Copy from source to destination. Note that the
            // destination buffer may be full if the incoming buffer
            // exactly fits to the 4K boundary even if the buffer is
            // not at the maximum record size. This is fine as we still
            // write a record with no wasted space.
            //
            BOOLEAN isDestinationFull = _CoalesceRecords->DestinationCopyFrom(_LLSource);

            //
            // The coalescing buffer is full or the incoming buffer is empty or both
            //
            BOOLEAN isRecordEmpty = (_LLSource.GetDataLeftInSource() == 0);

            //
            // We may want to force an early flush if we are under
            // pressure (shared log usage or memory) and we won't waste
            // too many bytes on disk.
            //
            // TODO: make _MaxBytesPaddingWhenUnderPressure a config
            // value
            BOOLEAN flushEarly = (_OverlayStream->IsUnderPressureToFlush() &&
                                  (_CoalesceRecords->GetSpaceLeftInDestination() < _MaxBytesPaddingWhenUnderPressure));

            BOOLEAN isThisFlushing = isDestinationFull ||
                                     _ForceFlush ||
                                     flushEarly;

            if (isRecordEmpty)
            {
                //
                // The incoming record has been completely consumed, update the coalesced record to use the 
                // highest operation id and latest head truncation point
                //
                destinationStreamBlockHeader->HighestOperationId = sourceStreamBlockHeader->HighestOperationId;
                destinationStreamBlockHeader->HeadTruncationPoint = sourceStreamBlockHeader->HeadTruncationPoint;
                if (_LLSource.GetRecordMarkerPlusOne() != 0)
                {
                    ULONG coalescedRecordMarkerPlusOne =  
                        (ULONG)((_LLSource.GetStreamOffset() + _LLSource.GetDataSize()) -
                                _CoalesceRecords->GetDestinationStreamOffset()) + 1;
                    _CoalesceRecords->SetRecordMarkerPlusOne(coalescedRecordMarkerPlusOne);

                }
            }

            if (isThisFlushing)
            {
                //
                // Coalescing record is full or record written with
                // force flush flag set so initiate flush
                //
                _AppendState = AppendState::AppendFlushInProgress;

                Status = _CoalesceRecords->PrepareRecordForWrite();
                if (! NT_SUCCESS(Status))
                {
                    //
                    // There was an error getting the coalesce buffer ready, fail this and all async flushing
                    //
                    KTraceFailedAsyncRequest(Status, this, _AppendState, 0);
                    flushingRecordsContext->CompleteFlushingAsyncs(Status);
                    flushingRecordsContext = nullptr;
                    _CoalesceRecords->ResetFlushingRecordsContext();                    
                    _CoalesceRecords->ClearDestination();

                    _CoalesceRecords->RemoveWriteFromWaitingList(*this);
                    _CoalesceRecords->StartNextWrite();

                    Complete(Status);
                    return;
                }

                //
                // Compute the amount of reservation that will be needed for the data that has not been moved into the
                // coalesce record. 
                //
                ULONGLONG reserveToSave;

                reserveToSave = KLoggerUtilities::RoundUpTo4K(_LLSource.GetDataLeftInSource());
                KInvariant(reserveToSave <= _CoalesceRecords->GetReserveToUse());

                _ReserveBeingUsed = _CoalesceRecords->GetReserveToUse() - reserveToSave;

                if (isRecordEmpty)
                {
                    //
                    // If we are done with this record then add to the list of records
                    //
                    _LLSource.Clear();
                    _CoalesceRecords->RemoveWriteFromWaitingList(*this);
                    _DestagingWriteContext->SetOrResetTruncateOnCompletion(FALSE);                  
                    flushingRecordsContext->AppendCoalescingWrite(this);
                }

                //
                // Flush the records being coalesced. This runs in an independent apartment and provides
                // no completion notification back to the AppendAsync.
                // Take a copy of _CoalesceRecords since after flush
                // below the this pointer may no longer be valid as
                // this will be completed asynchronous to the routine.
                //
                CoalesceRecords::SPtr coalesceRecords = _CoalesceRecords;

                coalesceRecords->SubtractReserveToUse(_ReserveBeingUsed);

                KDbgCheckpointWDataInformational(GetActivityId(),
                                    "OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext Flush", Status,
                                    (ULONGLONG)(((_ForceFlush ? 1 : 0) + (flushEarly ? 2 : 0)) * 0x100000000) +
                                                destinationStreamBlockHeader->DataSize,
                                    (ULONGLONG)this,
                                    (ULONGLONG)destinationStreamBlockHeader->StreamOffset,
                                    (ULONGLONG)destinationStreamBlockHeader->HighestOperationId);

                coalesceRecords->InsertCurrentFlushingRecordsContext();
                flushingRecordsContext->StartFlushRecords(_ReserveBeingUsed,
                                                          NULL,
                                                          NULL);

                //
                // NOTE: if isRecordEmpty then this pointer is not valid
                //

                flushingRecordsContext = nullptr;

                coalesceRecords->ClearDestination();

                //
                // Clear the record being flushed from the _CoalesceRecord so a new one can be started while
                // the write is in progress
                //
                coalesceRecords->Clear();

                if (isRecordEmpty)
                {
                    //
                    // If we are done with this record then we can go ahead and start next
                    //
                    coalesceRecords->StartNextWrite();


                    // !!!!! Did not free _RecordIoBuffer !!!!!
                    
                    return;
                }
                else
                {
                    //
                    // Otherwise there is more data to be harvested from the record so start copying it into the next buffer. 
                    //
                    goto CopyRecordData;
                }
            }
            else 
            {
                KInvariant(isRecordEmpty);

                //
                // This async has finished its work and will be waiting for it to be flushed. Move from the waiting list
                // to the Coalescing list and then start the next async. Finally restart the periodic flush timer.
                //
                _AppendState = AppendState::AppendWaitForFlush;
                _RecordMetadataBuffer = nullptr;
                _RecordIoBuffer = nullptr;

                _LLSource.Clear();
                _CoalesceRecords->RemoveWriteFromWaitingList(*this);
                _DestagingWriteContext->SetOrResetTruncateOnCompletion(FALSE);                  
                flushingRecordsContext->AppendCoalescingWrite(this);

                _CoalesceRecords->StartNextWrite();
                _CoalesceRecords->RestartPeriodicTimer();
            }

            break;
        }

        case AppendState::AppendFlushInProgress:
        {
            //
            // Should never get here
            //
            KInvariant(FALSE);
            break;
        }

        case AppendState::AppendPerformDirectWrite:
        {
            //
            // In this case the write had the ForceFlush flag set and there was nothing to be coalesced. So just
            // need to pass through the write
            //
PerformDirectWrite:
            _AppendState = AppendState::AppendWritingDirectly;

            //
            // Since the record is being written directly we do not need the buffer
            //
            if (_CoalesceIoBuffer)
            {
                _ThrottledAllocator->FreeKIoBuffer(GetActivityId(), _CoalesceIoBuffer->QuerySize());
                _CoalesceIoBuffer = nullptr;
            }

            // !!!!! Does not free _RecordIoBuffer !!!!!
            
            //
            // Include this on the Independent Write list in case we
            // get a read while this is outstanding
            //            
            _CoalesceRecords->RemoveWriteFromWaitingList(*this);
            _CoalesceRecords->AddWriteToIndependentList(*this);
            
            _DedicatedWrite->StartReservedWrite(
                _ReserveToUse,
                _LLSource.GetStreamOffset(),
                _LLSource.GetVersion(),
                _RecordMetadataBuffer,
                _RecordIoBuffer,
                this,
                completion);

            _CoalesceRecords->StartNextWrite();

            break;
        }

        case AppendState::AppendWritingDirectly:
        {
            _CoalesceRecords->RemoveWriteFromIndependentList(*this);
            Complete(Status);
            return;
        }

        default:
        {
            KInvariant(FALSE);
        }
    }
}

VOID
OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext::FlushFSMContinue(
    __in NTSTATUS Status
)
{
    OverlayStream::CoalesceRecords::AsyncFlushingRecordsContext::SPtr flushingRecordsContext = _CoalesceRecords->GetFlushingRecordsContext();
    
    KAsyncContextBase::CompletionCallback completion(this, &OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext::OperationCompletion);

    //
    // FSM is entered only when it is this async's turn to run. All
    // asyncs (Append and Flush) run within the same apartment and so
    // only one can be active at one time. Furthermore, asyncs are not
    // dispatched to their FSM until all asyncs that were received
    // previously have finished their work. In the case of the
    // FlushAsync, this is when any coalesce record is fully flushed.
    // The functionality for only allowing one async to run to finish at a time is
    // implemented by use of an AsyncEvent and keeping the waiting
    // asyncs on the _WaitingCoalescingWrite list. See OnStart() method
    // for this implementation.
    //
    
    KInvariant(_CoalesceRecords->IsCurrentlyExecuting(*this));

    KDbgCheckpointWDataInformational(GetActivityId(),
                        "OverlayStream::CoalesceRecords::AsyncFlushContext::FSMContinue", Status,
                        (ULONGLONG)_FlushState,
                        (ULONGLONG)this,
                        (ULONGLONG)_CoalesceRecords->GetDestinationStreamBlockHeader() ? _CoalesceRecords->GetDestinationStreamBlockHeader()->StreamOffset : 0,
                        (ULONGLONG)_CoalesceRecords->GetDestinationStreamBlockHeader() ? _CoalesceRecords->GetDestinationStreamBlockHeader()->HighestOperationId : 0);
    
    if (! NT_SUCCESS(Status))
    {
        KTraceFailedAsyncRequest(Status, this, _FlushState, _CoalesceRecords->GetReserveToUse());
    }

    switch (_FlushState)
    {
        case FlushState::FlushInitial:
        {
            //
            // If closing then flush can only occur when all records
            // are processed and no more are waiting.
            //
            if ((! flushingRecordsContext) ||
                (flushingRecordsContext->IsCoalesceWritesEmpty()))
            {
                if (_IsClosing)
                {
                    KInvariant(_CoalesceRecords->GetWaitingCoalescingWritesCount() == 1);
                
                    if (_CoalesceRecords->GetReserveToUse() > 0)
                    {
                        //
                        // If there is any extra reservation held then it needs to be released
                        //
                        _FlushState = FlushState::FlushWaitForReservationRelease;

                        _CoalesceRecords->SetReserveToUse(0);
                        _DedicatedReservation->StartUpdateReservation(-1 * _CoalesceRecords->GetReserveToUse(),
                                                                     _OverlayStream.RawPtr(),
                                                                     completion);

                        return;
                    }
                }
                
                //
                // Nothing to flush and no reservation held
                //
                _CoalesceRecords->RemoveWriteFromWaitingList(*this);
                _CoalesceRecords->StartNextWrite();
               
                Complete(STATUS_SUCCESS);
                return;
            }

            //
            // Flush anything being coalesced so far and use up all reservation held
            //
            _FlushState = FlushState::FlushWaitForFlush;

            Status = _CoalesceRecords->PrepareRecordForWrite();
            if (! NT_SUCCESS(Status))
            {                
                KTraceFailedAsyncRequest(Status, this, _FlushState, 0);

                flushingRecordsContext->CompleteFlushingAsyncs(Status);
                flushingRecordsContext = nullptr;
                _CoalesceRecords->ResetFlushingRecordsContext();
                _CoalesceRecords->ClearDestination();
                
                _CoalesceRecords->RemoveWriteFromWaitingList(*this);
                _CoalesceRecords->StartNextWrite();
                
                Complete(Status);
                return;
            }

#ifdef UDRIVER
            KDbgCheckpointWData(GetActivityId(),
                                "OverlayStream::CoalesceRecords::AsyncFlushContext::FSMContinue PerformWrite", Status,
                                (ULONGLONG)_CoalesceRecords->GetReserveToUse(),
                                (ULONGLONG)this,
                                (ULONGLONG)_CoalesceRecords->GetDestinationStreamBlockHeader()->StreamOffset,
                                (ULONGLONG)_CoalesceRecords->GetDestinationStreamBlockHeader()->HighestOperationId);
#endif
            _CoalesceRecords->RemoveWriteFromWaitingList(*this);
            flushingRecordsContext->AppendCoalescingWrite(this);

            //
            // Flush the records being coalesced. This runs in an independent apartment and provides
            // no completion notification back to the AppendAsync. 
            //
            CoalesceRecords::SPtr coalesceRecords = _CoalesceRecords;

            coalesceRecords->SetReserveToUse(0);

            coalesceRecords->InsertCurrentFlushingRecordsContext();
            flushingRecordsContext->StartFlushRecords(coalesceRecords->GetReserveToUse(),
                                                      NULL,
                                                      NULL);
            //
            // NOTE: this ptr is no longer be valid as it was added to
            //       _CoalescingWrites list and may be completed
            //

            flushingRecordsContext = nullptr;
            coalesceRecords->ClearDestination();

            coalesceRecords->Clear();

            coalesceRecords->StartNextWrite();
            return;
        }

        case FlushState::FlushWaitForFlush:
        {
            //
            // Should never get here
            //
            KInvariant(FALSE);
        }

        case FlushState::FlushWaitForReservationRelease:
        {
            // CONSIDER: What to do if release reserve fails ?

            _CoalesceRecords->RemoveWriteFromWaitingList(*this);
            _CoalesceRecords->StartNextWrite();
            Complete(Status);
            return;
        }


        default:
        {
            KInvariant(FALSE);
        }
    }
}

VOID
OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext::OnCancel(
)
{
    KTraceCancelCalled(this, FALSE, FALSE, 0);
}

VOID
OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext::OperationCompletion(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase& Async
)
{
    NTSTATUS status = Async.Status();

    if (_AppendState == Unassigned)
    {
        FlushFSMContinue(status);
    }
    else
    {
        AppendFSMContinue(status);
    }
}

VOID
OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext::WaitToRunContext(
    )
{
    KAsyncContextBase::CompletionCallback completion(this,
                                                     &OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext::OperationCompletion);

    //
    // First thing is to add this async to the waiting list and then wait until it is time for it to run. The
    // requirement is that these Asyncs must run to completion in order, that is, if async A is running and
    // in the middle of flushing to disk, async B should not sneak in while disk write in progress. To enforce this,
    // each async will add itself to the _WaitingCoalescingWrites list and wait on the event
    // emedded in async. If the list was empty AddWriteToWaitingList will set the event right away. If the list
    // is not empty then there is another async running and so this async will wait for its turn.
    //
    // CONSIDER: We could use a more fancy data structure which would reorder
    //           writes that are out of order and allow out of order writes
    //
    _CoalesceRecords->AddWriteToWaitingList(*this);

    //
    // Only set event after adding to waiting list since we do not want
    // shared write to completed until the record is placed on the list
    // and thus can be read
    //
    if (_DestagingWriteContext)
    {
        _DestagingWriteContext->SetDedicatedCoalescedEvent();
    }
    
    _WaitContext->StartWaitUntilSet(_OverlayStream.RawPtr(),
                                    completion);
}

VOID
OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext::OnStart(
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    KInvariant((_AppendState == AppendState::AppendInitial) || (_FlushState == FlushState::FlushInitial));
    _RequestRefAcquired = FALSE;
    KFinally([&] {
        if (! NT_SUCCESS(status))
        {
            if (_DestagingWriteContext)
            {
                _DestagingWriteContext->SetDedicatedCoalescedEvent();
            }
            Complete(status);
        }
    });

    if (IsAppendContext())
    {
        //
        // Prepare the source record for copy
        //
        ULONG coreLoggerOffset = _DedicatedLogContainer->QueryUserRecordSystemMetadataOverhead();
        _LLSource.Initialize(coreLoggerOffset, coreLoggerOffset, *_RecordMetadataBuffer, _RecordIoBuffer.RawPtr());
        status = _LLSource.InitializeExisting(FALSE);

        if (! NT_SUCCESS(status))
        {
            //
            // This async has an error, complete it and kick off next one
            //
            KTraceFailedAsyncRequest(status, this, _AppendState, 0);
            return;
        }
        _LLSource.InitializeSourceForCopy();
    }   
    
    if ((_AppendState == AppendState::AppendInitial) || ((_FlushState == FlushState::FlushInitial) && (! _IsClosing)))
    {
        if (_CoalesceRecords->IsClosing())
        {
            //
            // Do not accept any new operations once the stream is being flushed for closing
            //
            status = STATUS_OBJECT_NO_LONGER_EXISTS;
            KTraceFailedAsyncRequest(status, this, 0, 0);
            return;
        }
        
        status = _OverlayStream->TryAcquireRequestRef();
        if (! NT_SUCCESS(status))
        {
            return;
        }
        _RequestRefAcquired = TRUE;     
    }
    
    WaitToRunContext();
}


VOID
OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext::StartAppend(
    __in KBuffer& RecordMetadataBuffer,
    __in KIoBuffer& RecordIoBuffer,
    __in ULONGLONG ReserveToUse,
    __in BOOLEAN ForceFlush,
    __in_opt KIoBuffer* CoalesceIoBuffer,
    __in AsyncDestagingWriteContextOverlay& DestagingWriteContext,
    // ParentAsync is always OverlayStream object to ensure proper ordering
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _AppendState = AppendState::AppendInitial;
    _FlushState = FlushState::FlushUnassigned;

    _RecordMetadataBuffer = &RecordMetadataBuffer;
    _RecordIoBuffer = &RecordIoBuffer;
    _ReserveToUse = ReserveToUse;
    _CoalesceIoBuffer = CoalesceIoBuffer;
    _DestagingWriteContext = &DestagingWriteContext;
    if (_OverlayStream->IsDisableCoalescing())
    {
        _ForceFlush = TRUE;
    }
    else 
    {
        _ForceFlush = ForceFlush;
    }

    Start(_OverlayStream.RawPtr(), CallbackPtr);
}

VOID
OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext::StartFlush(
    __in BOOLEAN IsClosing,
    // ParentAsync is always OverlayStream object to ensure proper ordering
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _FlushState = FlushState::FlushInitial;
    _AppendState = AppendState::AppendUnassigned;

    _IsClosing = IsClosing;
    Start(_OverlayStream.RawPtr(), CallbackPtr);
}


OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext::~AsyncAppendOrFlushContext()
{
}

VOID
OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext::OnReuse(
)
{
    _AppendState = AppendState::AppendUnassigned;
    _FlushState = FlushState::FlushUnassigned;
    
    _DedicatedWrite->Reuse();
    _DedicatedReservation->Reuse();
    _WaitContext->Reuse();

    _DestagingWriteContext = nullptr;
}

OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext::AsyncAppendOrFlushContext(
    __in OverlayStream& OS,
    __in CoalesceRecords& CR,   
    __in RvdLog& DedicatedLogContainer,
    __in RvdLogStream& DedicatedLogStream,
    __in ThrottledKIoBufferAllocator& ThrottledAllocator
    ) :
        _OverlayStream(&OS),
        _CoalesceRecords(&CR),
        _DedicatedLogContainer(&DedicatedLogContainer),
        _DedicatedLogStream(&DedicatedLogStream),
        _ThrottledAllocator(&ThrottledAllocator),
        _AsyncEvent(FALSE,    // IsManualReset
                    FALSE)    // Initial state

{
    NTSTATUS status;

    status = _DedicatedLogStream->CreateAsyncWriteContext(_DedicatedWrite);
    if (! NT_SUCCESS(status))
    {
        KTraceOutOfMemory(GetActivityId(), status, this, 0, 0);
        SetConstructorStatus(status);
        return;
    }
    
    status = _DedicatedLogStream->CreateUpdateReservationContext(_DedicatedReservation);
    if (! NT_SUCCESS(status))
    {
        KTraceOutOfMemory(GetActivityId(), status, this, 0, 0);
        SetConstructorStatus(status);
    }

    status = _AsyncEvent.CreateWaitContext(GetThisAllocationTag(), GetThisAllocator(), _WaitContext);
    if (! NT_SUCCESS(status))
    {
        KTraceOutOfMemory(GetActivityId(), status, this, 0, 0);
        SetConstructorStatus(status);
        return;
    }

    _AppendState = AppendState::AppendUnassigned;
    _FlushState = FlushState::FlushUnassigned;
   
    _ReserveToUse = 0;
    _IsClosing = FALSE;
}

NTSTATUS
OverlayStream::CoalesceRecords::CreateAsyncAppendOrFlushContext(
    __out AsyncAppendOrFlushContext::SPtr& Context
)
{
    NTSTATUS status;
    OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext::SPtr context;

    Context = nullptr;
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncAppendOrFlushContext(*_OverlayStream,
                                                                                         *this,
                                                                                         *_DedicatedLogContainer,
                                                                                         *_DedicatedLogStream,
                                                                                         *_ThrottledAllocator);
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(_OverlayStream->GetActivityId(), status, 0, GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    Context = Ktl::Move(context);

    return(STATUS_SUCCESS);
}


ULONG OverlayStream::CoalesceRecords::ComputeIoBufferNeeded(
    __in ULONG RecordDataSize
)
{
    ULONG ioBufferNeeded = 0;

    //
    // Round up to the extent size
    //
    ioBufferNeeded = ((RecordDataSize) +
                        (ThrottledKIoBufferAllocator::AsyncAllocateKIoBufferContext::_AllocationExtentSize-1)) &
                        (~(ThrottledKIoBufferAllocator::AsyncAllocateKIoBufferContext::_AllocationExtentSize-1));
    
    return(ioBufferNeeded);
}

#if defined(UDRIVER) || defined(UPASSTHROUGH)
VOID
OverlayStream::CoalesceRecords::ForcePeriodicFlush(
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
)
{
    //
    // Make sure periodic flush is manually disabled
    //
    KInvariant(_PeriodicFlushInProgress == -1);

    _PeriodicFlushContext->Reuse();
    _PeriodicFlushContext->StartFlush(FALSE, CallbackPtr);
}
#endif

VOID
OverlayStream::CoalesceRecords::StartPeriodicFlush()
{
    NTSTATUS status;

    //
    // Take service activity to represent the outstanding periodic
    // flush. It is released when the flush completes.
    //
    status = _OverlayStream->TryAcquireRequestRef();
    if (! NT_SUCCESS(status))
    {
        //
        // If this fails then we are closing
        //
        KTraceFailedAsyncRequest(status, _OverlayStream.RawPtr(), (ULONGLONG)this, 0);
        return;
    }   
    
    KAsyncContextBase::CompletionCallback periodicFlushCompletion(this,
                                                                  &OverlayStream::CoalesceRecords::PeriodicFlushCompletion);
    
    ULONG flushInProgress;

    flushInProgress = InterlockedCompareExchange(&_PeriodicFlushInProgress, 1, 0);

    if (flushInProgress == 0)
    {
        //
        // Only one periodic flush should be in progress at one time
        //
        _PeriodicFlushContext->Reuse();         
        _PeriodicFlushContext->StartFlush(FALSE, periodicFlushCompletion);
    } else {
        //
        // Since there is currently a periodic flush in progress,
        // we cannot kick off another one. So set the flag to
        // indicate that when the current periodic flush completes,
        // that it needs to turn around and perform another flush
        //
        KDbgCheckpointWDataInformational(_OverlayStream->GetActivityId(),
            "Delayed Periodic Flush", STATUS_SUCCESS,
            (ULONGLONG)flushInProgress,
            (ULONGLONG)this,
            (ULONGLONG)0,
            (ULONGLONG)0);

        InterlockedIncrement(&_PeriodicFlushOnCompletion);

        //
        // Release activity taken in StartPeriodicFlush()
        //
        _OverlayStream->ReleaseRequestRef();    
    }
}

VOID
OverlayStream::CoalesceRecords::PeriodicFlushCompletion(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase& Async
    )
{       
    //
    // On exit, release ref take in StartPeriodicFlush()
    //
    KFinally([&] { _OverlayStream->ReleaseRequestRef();  });
    NTSTATUS status = Async.Status();
    
    if (! NT_SUCCESS(status))
    {
        //
        // If this fails then keep going
        //
        KTraceFailedAsyncRequest(status, _OverlayStream.RawPtr(), (ULONGLONG)this, _PeriodicFlushOnCompletion);
    }
    
    KInvariant(_PeriodicFlushInProgress == 1);
    _PeriodicFlushInProgress = 0;

    LONG flushOnCompletion;
    flushOnCompletion = InterlockedExchange(&_PeriodicFlushOnCompletion, 0);
    if (flushOnCompletion != 0)     
    {
        //
        // There was a new periodic flush needed while the just
        // completed periodic flush was in progress. Kick off a new
        // periodic flush.      
        //
        KDbgCheckpointWDataInformational(_OverlayStream->GetActivityId(),
                            "OverlayStream::CoalesceRecords::PeriodicFlushCompletion FlushOnCompletion", STATUS_SUCCESS,
                            (ULONGLONG)flushOnCompletion,
                            (ULONGLONG)this,
                            (ULONGLONG)0,
                            (ULONGLONG)0);
        StartPeriodicFlush();
    }          
}

VOID 
OverlayStream::CoalesceRecords::PeriodicTimerCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
)
{
    NTSTATUS status = Async.Status();
    OverlayStream::SPtr overlayStream = (OverlayStream*)ParentAsync;

    //
    // See if the OverlayStream is closing or not. If closing or closed
    // then there is nothing to do here
    //
    NTSTATUS acquireStatus = overlayStream->TryAcquireRequestRef();
    if (! NT_SUCCESS(acquireStatus))
    {
        KDbgCheckpointWDataInformational(overlayStream->GetActivityId(),
                            "OverlayStream::CoalesceRecords::PeriodicTimerCompletion timer die", acquireStatus,
                            (ULONGLONG)overlayStream.RawPtr(),
                            (ULONGLONG)this,
                            (ULONGLONG)0,
                            (ULONGLONG)0);
        return;
    }
    KFinally([&] { overlayStream->ReleaseRequestRef(); });
            
    //
    // NOTE: Timer completion should only run in the OverlayStream
    //       apartment.
    KAsyncContextBase::CompletionCallback periodicTimerCompletion(this,
                                            &OverlayStream::CoalesceRecords::PeriodicTimerCompletion);
    
    _IsFlushTimerActive = FALSE;
    
    if (_IsClosing)
    {
        //
        // If stream is closing then don't bother with periodic flush
        //
        KDbgCheckpointWData(_OverlayStream->GetActivityId(),
                            "PeriodicTimer Closing", status,
                            (ULONGLONG)_PeriodicFlushCounter,
                            (ULONGLONG)_OverlayStream->GetPeriodicFlushTimeInSec(),
                            (ULONGLONG)0,
                            (ULONGLONG)0);
        return;
    }

    if (status == STATUS_CANCELLED)
    {
        //
        // If the timer was cancelled it was likely because the
        // periodic timer needed to be reset.
        //
        if (_RestartPeriodicFlushTimer)
        {
            //
            // Restart timer if need be
            //
            _IsFlushTimerActive = TRUE;
            _RestartPeriodicFlushTimer = FALSE;
            _PeriodicFlushCounter = 0;
#ifdef UDRIVER
            KDbgCheckpointWData(_OverlayStream->GetActivityId(),
                                "restart timer from cancel", STATUS_SUCCESS,
                                (ULONGLONG)_PeriodicFlushCounter,
                                (ULONGLONG)_OverlayStream->GetPeriodicFlushTimeInSec(),
                                (ULONGLONG)0,
                                (ULONGLONG)0);
#endif
            _FlushTimer->Reuse();
            _FlushTimer->StartTimer(_OverlayStream->GetPeriodicTimerIntervalInSec() * 1000,
                                    _OverlayStream.RawPtr(), periodicTimerCompletion);
        } else {
            KTraceCancelCalled(NULL, FALSE, FALSE, (ULONGLONG)this);
        }
        return;
    }

#if defined(UDRIVER) || defined(UPASSTHROUGH)
    if (_OverlayStream->GetPeriodicTimerTestDelayInSec() != 0) 
    {
        //
        // Test helper KtlPhysicalLogTest::OverlayLogTests::PeriodicTimerCloseRaceTest
        //
        KNt::Sleep(_OverlayStream->GetPeriodicTimerTestDelayInSec() * 1000);
    }
#endif
    
    //
    // Our periodic flush timer has fired, if it is time to flush or
    // the system is under memory pressure then force the flush
    //
    _PeriodicFlushCounter += _OverlayStream->GetPeriodicTimerIntervalInSec();

    BOOLEAN isUnderPressure = _OverlayStream->IsUnderPressureToFlush();

#ifdef UDRIVER
    KDbgCheckpointWData(_OverlayStream->GetActivityId(),
                        "flushornot", STATUS_SUCCESS,
                        (ULONGLONG)_PeriodicFlushCounter,
                        (ULONGLONG)_OverlayStream->GetPeriodicFlushTimeInSec(),
                        (ULONGLONG)isUnderPressure,
                        (ULONGLONG)0);
#endif
    
    if ((isUnderPressure) || (_PeriodicFlushCounter >= _OverlayStream->GetPeriodicFlushTimeInSec()))
    {
        KDbgCheckpointWDataInformational(_OverlayStream->GetActivityId(),
                            "OverlayStream::CoalesceRecords::PeriodicTimerCompletion Flush", status,
                            (ULONGLONG)isUnderPressure,
                            (ULONGLONG)this,
                            (ULONGLONG)_PeriodicFlushCounter,
                            (ULONGLONG)_OverlayStream->GetPeriodicFlushTimeInSec());
        StartPeriodicFlush();
    } else {
#ifdef UDRIVER
        KDbgCheckpointWData(_OverlayStream->GetActivityId(),
                            "restart timer since not time", STATUS_SUCCESS,
                            (ULONGLONG)_PeriodicFlushCounter,
                            (ULONGLONG)_OverlayStream->GetPeriodicFlushTimeInSec(),
                            (ULONGLONG)0,
                            (ULONGLONG)0);
#endif
        _IsFlushTimerActive = TRUE;
        _RestartPeriodicFlushTimer = FALSE;
        _FlushTimer->Reuse();
        _FlushTimer->StartTimer(_OverlayStream->GetPeriodicTimerIntervalInSec() * 1000,
                                _OverlayStream.RawPtr(), periodicTimerCompletion);
    }
}

VOID 
OverlayStream::CoalesceRecords::RestartPeriodicTimer(
)
{
    //
    // This routine assumes that it is called from within the
    // CoalescingRecords apartment and thus does not need to worry
    // about taking any locks
    //
    KAsyncContextBase::CompletionCallback periodicTimerCompletion(this,
                                                                  &OverlayStream::CoalesceRecords::PeriodicTimerCompletion);
    if (!_IsClosing)
    {
        _PeriodicFlushCounter = 0;
        if (!_IsFlushTimerActive)
        {
            //
            // If the timer is not running then start it up
            //
            _IsFlushTimerActive = TRUE;
            _RestartPeriodicFlushTimer = FALSE;
            
#ifdef UDRIVER
            KDbgCheckpointWData(_OverlayStream->GetActivityId(),
                                             "restart timer", STATUS_SUCCESS,
                                             (ULONGLONG)_PeriodicFlushCounter,
                                             (ULONGLONG)_OverlayStream->GetPeriodicFlushTimeInSec(),
                                             (ULONGLONG)0,
                                             (ULONGLONG)0);
#endif
            _FlushTimer->Reuse();
            _FlushTimer->StartTimer(_OverlayStream->GetPeriodicTimerIntervalInSec() * 1000,
                                    _OverlayStream.RawPtr(), periodicTimerCompletion);
        } else {
            //
            // Try to reset the timer
            //
#ifdef UDRIVER
            KDbgCheckpointWData(_OverlayStream->GetActivityId(),
                                "reset timer", STATUS_SUCCESS,
                                (ULONGLONG)this,
                                (ULONGLONG)_OverlayStream->GetPeriodicFlushTimeInSec(),
                                (ULONGLONG)_PeriodicFlushCounter,
                                (ULONGLONG)0);
#endif
            if (! _FlushTimer->ResetTimer(_OverlayStream->GetPeriodicTimerIntervalInSec() * 1000))
            {
                //
                // if for some reason the timer is still active but the
                // timer can't be reset then try to cancel and restart
                //
                KDbgCheckpointWDataInformational(_OverlayStream->GetActivityId(),
                                                 "cant restart timer", STATUS_SUCCESS,
                                                 (ULONGLONG)_PeriodicFlushCounter,
                                                 (ULONGLONG)_OverlayStream->GetPeriodicFlushTimeInSec(),
                                                 (ULONGLONG)0,
                                                 (ULONGLONG)0);
                _RestartPeriodicFlushTimer = TRUE;
                _FlushTimer->Cancel();
            }
        }
    }
}

//
// AsyncFlushAllRecordsForClose
//
VOID
OverlayStream::CoalesceRecords::AsyncFlushAllRecordsForClose::OnCompleted(
)
{
    if (_RequestRefAcquired)
    {
        _OverlayStream->ReleaseRequestRef();
        _RequestRefAcquired = FALSE;
    }
}

VOID
OverlayStream::CoalesceRecords::AsyncFlushAllRecordsForClose::FSMContinue(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &OverlayStream::CoalesceRecords::AsyncFlushAllRecordsForClose::OperationCompletion);

    KDbgCheckpointWDataInformational(_OverlayStream->GetActivityId(), 
        "OverlayStream::CoalesceRecords::AsyncFlushAllRecordsForClose::FSMContinue", Status, _State, (ULONGLONG)this, 0, 0);

    switch (_State)
    {
        case Initial:
        {
            //
            // Set the flag that the stream is being closed and so no more
            // records will be accepted for coalescing. This flush should be
            // the very last append or flush async
            //
            KInvariant(! _CoalesceRecords->IsClosing());
            _CoalesceRecords->SetIsClosing();

            _CoalesceRecords->CancelFlushTimer();

            // CONSIDER:  Wait for timer to actually finish

            _State = PerformFinalFlush;
            _CoalesceRecords->StartFinalFlush(completion);
            
            break;
        }

        case PerformFinalFlush:
        {
            //
            // At this point the final flush has been sent to the
            // dedicated log but the corresponding write may not have been finished yet. We
            // need to wait here for all of the writes outstanding to
            // the dedicated log be flushed to disk before proceeding with the close
            //
            KDbgCheckpointWDataInformational(_OverlayStream->GetActivityId(), 
                "OverlayStream::CoalesceRecords::AsyncFlushAllRecordsForClose::FSMContinue",
                                             Status, _State, (ULONGLONG)this, _CoalesceRecords->FlushingRecordsCount(), 0);

            _CoalesceRecords->RemoveAllFailedFlushingRecordsContext();
            if (_CoalesceRecords->IsFlushingRecordsEmpty())
            {
                Complete(STATUS_SUCCESS);
                return;
            }

            //
            // Wait for all writes to complete
            //
            _State = WaitForFlushingRecords;
            _FlushingRecordsWait->StartWaitUntilSet(_OverlayStream.RawPtr(),
                                                    completion);
            break;
        }

        case WaitForFlushingRecords:
        {
            Complete(STATUS_SUCCESS);
            break;
        }

        default:
        {
            KInvariant(FALSE);
        }
    }    
}

VOID
OverlayStream::CoalesceRecords::AsyncFlushAllRecordsForClose::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, 0);
}

VOID
OverlayStream::CoalesceRecords::AsyncFlushAllRecordsForClose::OperationCompletion(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase& Async
    )
{
    NTSTATUS status = Async.Status();

    FSMContinue(status); 
}

VOID
OverlayStream::CoalesceRecords::AsyncFlushAllRecordsForClose::OnStart(
    )
{
    _State = Initial;

    _RequestRefAcquired = FALSE;
    
    NTSTATUS status = _OverlayStream->TryAcquireRequestRef();
    if (! NT_SUCCESS(status))
    {
        Complete(status);
        return;
    }
    _RequestRefAcquired = TRUE;

    FSMContinue(STATUS_SUCCESS);
}

OverlayStream::CoalesceRecords::AsyncFlushAllRecordsForClose::~AsyncFlushAllRecordsForClose()
{
}

VOID
OverlayStream::CoalesceRecords::AsyncFlushAllRecordsForClose::OnReuse(
    )
{
    KInvariant(FALSE);
}

OverlayStream::CoalesceRecords::AsyncFlushAllRecordsForClose::AsyncFlushAllRecordsForClose(
    __in OverlayStream& OS,
    __in CoalesceRecords& CR
    )
{
    NTSTATUS status;

    _OverlayStream = &OS;
    _CoalesceRecords = &CR;

    status = _CoalesceRecords->CreateFlushingRecordsWaitContext(_FlushingRecordsWait);
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }
}

NTSTATUS
OverlayStream::CoalesceRecords::CreateAsyncFlushAllRecordsForClose(
    __out AsyncFlushAllRecordsForClose::SPtr& Context
    )
{
    NTSTATUS status;
    OverlayStream::CoalesceRecords::AsyncFlushAllRecordsForClose::SPtr context;

    Context = nullptr;
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncFlushAllRecordsForClose(*_OverlayStream, *this);
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( GetActivityId(), status, this, GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    Context = context.RawPtr();
    
    return(STATUS_SUCCESS); 
}

VOID 
OverlayStream::CoalesceRecords::FlushAllRecordsForClose(
    // ParentAsync is always OverlayStream object to ensure proper ordering
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    KDbgCheckpointWDataInformational(_OverlayStream->GetActivityId(),
                        "OverlayStream::CoalesceRecords::FlushAllRecordsForClose", STATUS_SUCCESS,
                        (ULONGLONG)_IsClosing,
                        (ULONGLONG)this,
                        (ULONGLONG)_WaitingCoalescingWrites.Count(),
                        (ULONGLONG)_FlushingRecordsContext ? _FlushingRecordsContext->CoalescingWriteCount() : 0);

    _FlushAllRecordsForClose->Start(_OverlayStream.RawPtr(), CallbackPtr);
    _FlushAllRecordsForClose = nullptr;
}

VOID OverlayStream::CoalesceRecords::AddWriteToWaitingList(
    __in OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext& AppendContext
    )
{
    OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext::SPtr appendContext = &AppendContext;
    BOOLEAN startContextNow;

    K_LOCK_BLOCK(_ListLock)
    {
        //
        // If the list is curently empty then we should proceed with
        // processing
        //
        startContextNow = _WaitingCoalescingWrites.IsEmpty();

        //
        // Add this context to the end of the list. Take a ref to account
        // for being on the list; it is removed when taken off the list
        //
        appendContext->AddRef();
        _WaitingCoalescingWrites.AppendTail(&AppendContext);

        if (startContextNow)
        {
            //
            // Wakeup the current async
            //
            _LastExecuting = _CurrentlyExecuting;
            _CurrentlyExecuting = appendContext.RawPtr();
            appendContext->SetAsyncEvent();
        }
    }
}

VOID OverlayStream::CoalesceRecords::RemoveWriteFromWaitingList(
    __in OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext& AppendContext
    )
{
    OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext::SPtr appendContext = &AppendContext;

    K_LOCK_BLOCK(_ListLock)
    {
        OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext* p;
        p = _WaitingCoalescingWrites.Remove(&AppendContext);
        KInvariant(p != NULL);
    }

    //
    // Take away refcount added when placed on list
    //
    appendContext->Release();
}


VOID OverlayStream::CoalesceRecords::AddWriteToIndependentList(
    __in OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext& AppendContext
    )
{
    K_LOCK_BLOCK(_ListLock)
    {
        //
        // Add this context to the end of the list. Take a ref to account
        // for being on the list; it is removed when taken off the list
        //
        AppendContext.AddRef();
        _IndependentWrites.AppendTail(&AppendContext);
    }
}

VOID OverlayStream::CoalesceRecords::RemoveWriteFromIndependentList(
    __in OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext& AppendContext
    )
{
    K_LOCK_BLOCK(_ListLock)
    {
        OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext* p;
        p = _IndependentWrites.Remove(&AppendContext);
        KInvariant(p != NULL);
    }

    //
    // Take away refcount added when placed on list
    //
    AppendContext.Release();
}


VOID OverlayStream::CoalesceRecords::StartNextWrite(
    )
{
    OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext::SPtr appendContext;
    
    K_LOCK_BLOCK(_ListLock)
    {
        _LastExecuting = _CurrentlyExecuting;
        appendContext = _WaitingCoalescingWrites.PeekHead();
        if (appendContext)
        {
            _CurrentlyExecuting = appendContext.RawPtr();
            //
            // Wake up the next write async on the stack
            //
            appendContext->SetAsyncEvent();
        }
        else
        {
            _CurrentlyExecuting = (OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext*) - 1;
        }
    }
}


NTSTATUS OverlayStream::CoalesceRecords::PrepareRecordForWrite(
    )
{
    //
    // This routine assumes that it is called from within the
    // CoalescingRecords apartment and thus does not need to worry
    // about taking any locks
    //

    //
    // See if the IoBuffer needs to be trimmed - I don't think this is ever the case
    //
    _LLDestination.TrimIoBufferToSizeNeeded();
    _IoBufferX = _LLDestination.GetIoBuffer();

    _LLDestination.UpdateValidationInformation();

    return(STATUS_SUCCESS);
}

NTSTATUS OverlayStream::CoalesceRecords::Initialize(
)
{
    NTSTATUS status;
    KBuffer::SPtr metadataBuffer;
    KIoBuffer::SPtr ioBuffer;
    CoalesceRecords::AsyncFlushingRecordsContext::SPtr flushingRecordsContext;

    //
    // This routine assumes that it is called from within the
    // CoalescingRecords apartment and thus does not need to worry
    // about taking any locks
    //
    KInvariant(_MetadataBufferX == nullptr);
    KInvariant(_IoBufferX == nullptr);

    status = CreateAsyncFlushingRecordsContext(flushingRecordsContext);
    if (! NT_SUCCESS(status))
    {
        KTraceOutOfMemory(_OverlayStream->GetActivityId(), status, this, 0, 0);
        return(status);
    }

    ULONG coreLoggerOffset = _DedicatedLogContainer->QueryUserRecordSystemMetadataOverhead();

    HRESULT hr;
    ULONG result;
    hr = ULongSub(_MetadataBufferSize, coreLoggerOffset, &result);
    KInvariant(SUCCEEDED(hr));
    status = KBuffer::Create(result, metadataBuffer, GetThisAllocator(), GetThisAllocationTag());
    if (! NT_SUCCESS(status))
    {
        KTraceOutOfMemory(_OverlayStream->GetActivityId(), status, this, _MetadataBufferSize, 0);
        return(status);
    }

    status = KIoBuffer::CreateEmpty(ioBuffer, GetThisAllocator(), GetThisAllocationTag());
    if (! NT_SUCCESS(status))
    {
        KTraceOutOfMemory(_OverlayStream->GetActivityId(), status, this, _MetadataBufferSize, 0);
        return(status);
    }

    _MetadataBufferX = metadataBuffer;
    _IoBufferX = ioBuffer;
    _FlushingRecordsContext = flushingRecordsContext;

    _LLDestination.Initialize(coreLoggerOffset, 0, *_MetadataBufferX, _IoBufferX.RawPtr());
    _LLDestination.InitializeHeadersForNewRecord(_OverlayStream->GetStreamId(), 0, 0, 0, 0);
    _LLDestination.InitializeDestinationForCopy();

    return(STATUS_SUCCESS);
}

VOID 
OverlayStream::CoalesceRecords::Clear()
{
    //
    // This routine assumes that it is called from within the
    // CoalescingRecords apartment and thus does not need to worry
    // about taking any locks
    //
    _MetadataBufferX = nullptr;
    _IoBufferX = nullptr;

    _OffsetToDataLocation = 0;
}

OverlayStream::CoalesceRecords::~CoalesceRecords()
{
    KInvariant(_FlushingRecords.IsEmpty());
    KInvariant(_IndependentWrites.IsEmpty());
}

OverlayStream::CoalesceRecords::CoalesceRecords(
    __in OverlayStream& OS,
    __in RvdLog& DedicatedLogContainer,
    __in RvdLogStream& DedicatedLogStream,
    __in ThrottledKIoBufferAllocator& ThrottledAllocator,
    __in KActivityId ActivityId,
    __in ULONG IoBufferSize,
    __in ULONG MetadataBufferSize,
    __in ULONG ReservedMetadataSize
    ) :
    _WaitingCoalescingWrites(_CoalescingLinkageOffset),
    _IndependentWrites(_CoalescingLinkageOffset),
    _FlushingRecords(_FlushingRecordsLinkageOffset),
    _FlushingRecordsFlushed(TRUE, FALSE),
    _AlwaysSetEvent(TRUE, TRUE)
{
    NTSTATUS status;

    _OverlayStream = &OS;
    _ActivityId = ActivityId;
    _DedicatedLogContainer = &DedicatedLogContainer;
    _DedicatedLogStream = &DedicatedLogStream;
    _ThrottledAllocator = &ThrottledAllocator;
    _MetadataBufferSize = MetadataBufferSize;
    _ReservedMetadataSize = ReservedMetadataSize;
    _ReserveToUse = 0;
    _IsClosing = FALSE;

    _CurrentlyExecuting = NULL;

    _FlushingRecordsContext = nullptr;
    _FailedFlushingRecordCount = 0;

    _RestartPeriodicFlushTimer = FALSE;
    _IsFlushTimerActive = FALSE;
    _PeriodicFlushCounter = 0;

    //
    // Max record size also includes the metadata size so subtract it
    // here. Also the core logger wants to reserve 1/4 of the record space for
    // checkpoint records so remove that as well.
    //
    _MaxCoreLoggerRecordSize = 3 * (IoBufferSize / 4);
    _MaxCoreLoggerRecordSize -= (2 * KLogicalLogInformation::FixedMetadataSize);
    
    Clear();

    status = CreateAsyncAppendOrFlushContext(_CloseFlushContext);
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    status = CreateAsyncAppendOrFlushContext(_PeriodicFlushContext);
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    status = CreateAsyncFlushAllRecordsForClose(_FlushAllRecordsForClose);
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }
    
    _PeriodicFlushOnCompletion = 0;
    _PeriodicFlushInProgress = 0;
    status = KTimer::Create(_FlushTimer, GetThisAllocator(), GetThisAllocationTag());
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }
}

NTSTATUS
OverlayStream::CreateCoalesceRecords(
    __out CoalesceRecords::SPtr& Context,
    __in ULONG IoBufferSize,
    __in ULONG MetadataBufferSize,
    __in ULONG ReservedMetadataSize
    )
{
    NTSTATUS status;
    OverlayStream::CoalesceRecords::SPtr context;

    Context = nullptr;
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) CoalesceRecords(*this,
                                                                               *_DedicatedLogContainer,
                                                                               *_DedicatedLogStream,
                                                                               *_ThrottledAllocator,
                                                                               GetActivityId(),
                                                                               IoBufferSize, 
                                                                               MetadataBufferSize, 
                                                                               ReservedMetadataSize);
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(GetActivityId(), status, _DedicatedLogStream.RawPtr(), GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    Context = Ktl::Move(context);

    return(STATUS_SUCCESS);
}

VOID
OverlayStream::CoalesceRecords::AsyncFlushingRecordsContext::OnCompleted(
    )
{
    KInvariant(_CoalescingWrites.IsEmpty());

    _CoalesceRecords->RemoveFlushingRecordsContext(Status(), *this);
    
    _IoBuffer = nullptr;
    _MetadataBuffer = nullptr;

    if (_RequestRefAcquired)
    {
        _OverlayStream->ReleaseRequestRef();
        _RequestRefAcquired = FALSE;
    }    
}

VOID
OverlayStream::CoalesceRecords::AsyncFlushingRecordsContext::FSMContinue(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &OverlayStream::CoalesceRecords::AsyncFlushingRecordsContext::OperationCompletion);
        
    KDbgCheckpointWDataInformational(_OverlayStream->GetActivityId(),
                        "OverlayStream::CoalesceRecords::AsyncFlushingRecordsContext::FSMContinue", Status,
                        (ULONGLONG)_State,
                        (ULONGLONG)this,
                        (ULONGLONG)_StreamOffset,
                        (ULONGLONG)((_CoalescingWrites.Count() * 0x100000000) + GetDataSize()));

    switch (_State)
    {
        case Initial:
        {
            //
            // Write record to disk
            //
            _State = Flushing;

            _DedicatedWrite->Reuse();

            _FlushLatencyStart = KNt::GetPerformanceTime();
            
            _DedicatedWrite->StartReservedWrite(
                _ReserveToUse,
                _StreamOffset,
                _HighestOperationId,
                _MetadataBuffer,
                _IoBuffer,
                NULL,
                completion);
            break;
        }
        
        case Flushing:
        {
            //
            // Complete with whatever status we got
            //
            if (! NT_SUCCESS(Status))
            {
                _CoalesceRecords->AddReserveToUse(_ReserveToUse);
            } else {
#if !defined(PLATFORM_UNIX)
                LONGLONG flushLatencyTime = KNt::GetPerformanceTime() - _FlushLatencyStart;
                _OverlayStream->UpdateDedicatedWriteLatencyTime(flushLatencyTime);
#endif
            }
            
            CompleteFlushingAsyncs(Status);
            Complete(Status);
            return;
        }
        
        default:
        {
            KInvariant(FALSE);
        }
    }    
}

VOID
OverlayStream::CoalesceRecords::AsyncFlushingRecordsContext::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, 0);
}

VOID
OverlayStream::CoalesceRecords::AsyncFlushingRecordsContext::OperationCompletion(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase& Async
    )
{
    NTSTATUS status = Async.Status();

    FSMContinue(status); 
}

VOID
OverlayStream::CoalesceRecords::AsyncFlushingRecordsContext::OnStart(
    )
{
    KInvariant(_State == Initial);
    
    _RequestRefAcquired = FALSE;

    NTSTATUS status = _OverlayStream->TryAcquireRequestRef();
    if (! NT_SUCCESS(status))
    {
        CompleteFlushingAsyncs(status);
        Complete(status);
        return;
    }

    _RequestRefAcquired = TRUE;

    FSMContinue(STATUS_SUCCESS);   
}

VOID
OverlayStream::CoalesceRecords::AsyncFlushingRecordsContext::StartFlushRecords(
    __in ULONGLONG ReserveToUse,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _State = Initial;

    _ReserveToUse = ReserveToUse;
        
    Start(ParentAsyncContext, CallbackPtr);
}

VOID
OverlayStream::CoalesceRecords::AsyncFlushingRecordsContext::OnReuse(
    )
{
    _DedicatedWrite->Reuse();
}

OverlayStream::CoalesceRecords::AsyncFlushingRecordsContext::AsyncFlushingRecordsContext(
    __in OverlayStream& OS,
    __in OverlayStream::CoalesceRecords& CR,
    __in RvdLogStream& DedicatedLogStream
    ) :
    _OverlayStream(&OS),
    _CoalesceRecords(&CR),
    _DedicatedLogStream(&DedicatedLogStream),
    _CoalescingWrites(OverlayStream::CoalesceRecords::_CoalescingLinkageOffset)
{
    NTSTATUS status;
    
    status = _DedicatedLogStream->CreateAsyncWriteContext(_DedicatedWrite);
    if (! NT_SUCCESS(status))
    {
        KTraceOutOfMemory(OS.GetActivityId(), status, this, 0, 0);
        SetConstructorStatus(status);
        return;
    }
}

OverlayStream::CoalesceRecords::AsyncFlushingRecordsContext::~AsyncFlushingRecordsContext()
{
}

VOID OverlayStream::CoalesceRecords::AsyncFlushingRecordsContext::CompleteFlushingAsyncs(
    __in NTSTATUS Status
    )
{
    OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext::SPtr appendContext;

    //
    // Find the last append async in the list, this is the one that
    // should truncate
    //
    appendContext = _CoalescingWrites.PeekTail();
    while ((appendContext != nullptr) && (appendContext->IsFlushContext()))
    {
        appendContext = _CoalescingWrites.Predecessor(appendContext.RawPtr());
    }
    
    if (appendContext)
    {
        appendContext->_DestagingWriteContext->SetOrResetTruncateOnCompletion(TRUE);
    }
    
    while (!_CoalescingWrites.IsEmpty())
    {
        appendContext = _CoalescingWrites.RemoveHead();
        appendContext->CompleteRequest(Status);
    }
}

NTSTATUS OverlayStream::CoalesceRecords::CreateAsyncFlushingRecordsContext(
    __out AsyncFlushingRecordsContext::SPtr& Context
    )
{
    NTSTATUS status;
    AsyncFlushingRecordsContext::SPtr context;

    Context = nullptr;
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncFlushingRecordsContext(*_OverlayStream, *this, *_DedicatedLogStream);
    if (!context)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(_OverlayStream->GetActivityId(), status, 0, GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    Context = Ktl::Move(context);

    return(STATUS_SUCCESS);
}

void OverlayStream::CoalesceRecords::InsertCurrentFlushingRecordsContext()
{
    AsyncFlushingRecordsContext::SPtr flushingRecordsContext = GetFlushingRecordsContext();

    KLogicalLogInformation::StreamBlockHeader* streamBlockHeader = GetDestinationStreamBlockHeader();
    flushingRecordsContext->_StreamOffset = streamBlockHeader->StreamOffset;
    flushingRecordsContext->_DataSize = streamBlockHeader->DataSize;
    flushingRecordsContext->_HighestOperationId = streamBlockHeader->HighestOperationId;
    flushingRecordsContext->_MetadataBuffer = _MetadataBufferX;
    flushingRecordsContext->_IoBuffer = _IoBufferX;    

    flushingRecordsContext->AddRef();

    K_LOCK_BLOCK(_FRListLock)
    {
        _FlushingRecords.AppendTail(flushingRecordsContext.RawPtr());
    }
    
    ResetFlushingRecordsContext();              
}

void OverlayStream::CoalesceRecords::RemoveFlushingRecordsContext(
    __in NTSTATUS ,
    __in AsyncFlushingRecordsContext& FlushingRecordsContext
    )
{
    OverlayStream::CoalesceRecords::AsyncFlushingRecordsContext* p;
    K_LOCK_BLOCK(_FRListLock)
    {
        if (NT_SUCCESS(FlushingRecordsContext.Status()))
        {
            p =_FlushingRecords.Remove(&FlushingRecordsContext);
            KInvariant(p != NULL);
            FlushingRecordsContext.Release();
        } else {
            _FailedFlushingRecordCount++;
        }
    }

    //
    // When closing and flushing records list is empty then fire
    // an event that FlushAllRecordsForClose waits
    //
    if (_IsClosing)
    {
        _FlushingRecordsFlushed.SetEvent();
    }
}

void OverlayStream::CoalesceRecords::RemoveAllFailedFlushingRecordsContext(
    )
{
    AsyncFlushingRecordsContext* flushingRecordsContext;
    AsyncFlushingRecordsContext* nextFlushingRecordsContext;
    
    K_LOCK_BLOCK(_FRListLock)
    {
        if (_FailedFlushingRecordCount > 0)
        {
            flushingRecordsContext = _FlushingRecords.PeekHead();
            while ((flushingRecordsContext != NULL) && (_FailedFlushingRecordCount > 0))
            {
                nextFlushingRecordsContext = _FlushingRecords.Successor(flushingRecordsContext);
                if (! NT_SUCCESS(flushingRecordsContext->Status()))
                {
                    _FlushingRecords.Remove(flushingRecordsContext);
                    flushingRecordsContext->Release();
                    _FailedFlushingRecordCount--;
                }
                flushingRecordsContext = nextFlushingRecordsContext;
            }
        }
    }
    
    KInvariant(_FailedFlushingRecordCount == 0);
}


NTSTATUS OverlayStream::CoalesceRecords::FindDataInFlushingRecords(
    __inout RvdLogAsn& RecordAsn,
    __in RvdLogStream::AsyncReadContext::ReadType Type,
    __out ULONGLONG& Version,
    __out KBuffer::SPtr& MetaDataBuffer,
    __out KIoBuffer::SPtr& IoBuffer
    )
{
    NTSTATUS status;
    OverlayStream::CoalesceRecords::AsyncFlushingRecordsContext::SPtr flushingRecordsContext;
    AsyncAppendOrFlushContext::SPtr independentWriteContext;
    AsyncAppendOrFlushContext::SPtr waitingWriteContext;
    const ULONG lastBlockSize = 0x1000;

    KInvariant((Type == RvdLogStream::AsyncReadContext::ReadExactRecord) ||
               (Type == RvdLogStream::AsyncReadContext::ReadContainingRecord));
    
    flushingRecordsContext = nullptr;
    status = STATUS_NOT_FOUND;

    //
    // First check the currently coalescing buffer and harvest the
    // record being built if it matches the one we want.
    //
    flushingRecordsContext = GetFlushingRecordsContext();
    if ((flushingRecordsContext) && (!flushingRecordsContext->IsCoalesceWritesEmpty()))
    {
        KLogicalLogInformation::StreamBlockHeader* streamBlockHeader = GetDestinationStreamBlockHeader();

        if ( ((Type == RvdLogStream::AsyncReadContext::ReadExactRecord) &&
              (RecordAsn == streamBlockHeader->StreamOffset)) ||
             ((Type == RvdLogStream::AsyncReadContext::ReadContainingRecord) &&
              ((RecordAsn >= streamBlockHeader->StreamOffset) &&
               (RecordAsn < (streamBlockHeader->StreamOffset + _LLDestination.GetDataSize())))) )
        {
            //
            // To return the currently coalescing records, we need to
            // copy the metadata and the last 4K of the IO buffer since
            // writes after this point will change the data. We are
            // able to use the IO buffer up to the last 4K since we
            // know that will not change.
            //
            PUCHAR oldMetadataBuffer = _LLDestination.GetMetadataBuffer();
            ULONG oldMetadataSize = _LLDestination.GetMetadataBufferSize();
            KBuffer::SPtr metadataBuffer;
            status = KBuffer::Create(oldMetadataSize, metadataBuffer, GetThisAllocator(), GetThisAllocationTag());
            if (! NT_SUCCESS(status))
            {
                KTraceOutOfMemory(_OverlayStream->GetActivityId(), status, this, oldMetadataSize, RecordAsn.Get());
                return(status);
            }

            KMemCpySafe(metadataBuffer->GetBuffer(), metadataBuffer->QuerySize(), 
                        oldMetadataBuffer, oldMetadataSize);


            KIoBuffer::SPtr ioBuffer;
            KIoBufferElement::SPtr lastBufferElement;
            PVOID lastBufferElementPtr;

            status = KIoBuffer::CreateEmpty(ioBuffer, GetThisAllocator(), GetThisAllocationTag());
            if (! NT_SUCCESS(status))
            {
                KTraceOutOfMemory(_OverlayStream->GetActivityId(), status, this, sizeof(KIoBuffer), RecordAsn.Get());
                return(status);
            }

            ULONG trimmedIoBufferSize = _LLDestination.GetTrimmedIoBufferSize();

            if (trimmedIoBufferSize > 0)
            {                   
                KIoBuffer::SPtr oldIoBuffer;
                oldIoBuffer = _LLDestination.GetIoBuffer();

                status = KIoBufferElement::CreateNew(lastBlockSize, lastBufferElement, lastBufferElementPtr, GetThisAllocator(), GetThisAllocationTag());
                if (! NT_SUCCESS(status))
                {
                    KTraceOutOfMemory(_OverlayStream->GetActivityId(), status, this, lastBlockSize, RecordAsn.Get());
                    return(status);
                }

                if (trimmedIoBufferSize > lastBlockSize)
                {
                    status = ioBuffer->AddIoBufferReference(*oldIoBuffer, 0, (trimmedIoBufferSize - lastBlockSize), GetThisAllocationTag());
                    if (! NT_SUCCESS(status))
                    {
                        KTraceFailedAsyncRequest(status, _OverlayStream.RawPtr(), RecordAsn.Get(), 0);
                        return(status);
                    }
                }

                BOOLEAN b;
                b = KIoBufferStream::CopyFrom(*oldIoBuffer, (trimmedIoBufferSize - lastBlockSize), lastBlockSize, lastBufferElementPtr);
                KInvariant(b);

                status = ioBuffer->AddIoBufferElementReference(*lastBufferElement, 0, lastBlockSize, GetThisAllocationTag());
                if (! NT_SUCCESS(status))
                {
                    KTraceFailedAsyncRequest(status, _OverlayStream.RawPtr(), RecordAsn.Get(), 0);
                    return(status);
                }

            }

            //
            // Seal the record
            //
            KLogicalLogInformation::MetadataBlockHeader* metadataHeader;
            ULONG dataSizeRead;
            ULONG ioDataOffset;

            status = KLogicalLogInformation::FindLogicalLogHeadersWithCoreLoggerOffset(metadataBuffer,
                _DedicatedLogContainer->QueryUserRecordSystemMetadataOverhead(),
                *ioBuffer,
                sizeof(KtlLogVerify::KtlMetadataHeader),
                metadataHeader,
                streamBlockHeader,
                dataSizeRead,
                ioDataOffset);

            KInvariant(NT_SUCCESS(status));


            LLRecordObject llCopy;

            llCopy.Initialize(_LLDestination.GetHeaderSkipOffset(), _LLDestination.GetHeaderSkipOffset(),
                              metadataBuffer->GetBuffer(), metadataBuffer->QuerySize(), ioBuffer.RawPtr());
            llCopy.InitializeExisting(FALSE);
            llCopy.SetCoreLoggerOverhead(0);  // This converts the LLRecord from a 'read' record to a 'write' record
            llCopy.SetDataSize(_LLDestination.GetDataSize());
            llCopy.UpdateValidationInformation();

            RecordAsn = streamBlockHeader->StreamOffset;
            Version = streamBlockHeader->HighestOperationId;
            MetaDataBuffer = Ktl::Move(metadataBuffer);
            IoBuffer = Ktl::Move(ioBuffer);
            return(STATUS_SUCCESS);             
        }
    }
    
    K_LOCK_BLOCK(_FRListLock)
    {
        //
        // See if the data is in one of the coalesced data buffers
        //
        flushingRecordsContext = _FlushingRecords.PeekHead();
        while (flushingRecordsContext)
        {
            if ( ((Type == RvdLogStream::AsyncReadContext::ReadExactRecord) &&
                  (RecordAsn == flushingRecordsContext->GetStreamOffset())) ||
                 ((Type == RvdLogStream::AsyncReadContext::ReadContainingRecord) &&
                  ((RecordAsn >= flushingRecordsContext->GetStreamOffset()) &&
                   (RecordAsn < (flushingRecordsContext->GetStreamOffset() + flushingRecordsContext->GetDataSize())))) )
            {
                //
                // Since we are pointing at a context that is already queued for write then we
                // know it will not change. So we take copies of the metadata and IoBuffers. 
                //
                RecordAsn = flushingRecordsContext->GetStreamOffset();
                Version = flushingRecordsContext->GetHighestOperationId();

#ifdef UPASSTHROUGH
                //
                // For inproc logger, make a copy of the IoBuffer since
                // the logical log will call Clear() on it. This is not
                // needed in the case of the KDRIVER or UDRIVER since a
                // copy is made when crossing the user/kernel boundary.
                //
                status = flushingRecordsContext->GetIoBuffer()->CreateAlias(IoBuffer);
                if (! NT_SUCCESS(status))
                {
                    return(status);
                }               
#else
                IoBuffer = flushingRecordsContext->GetIoBuffer();
#endif              
                MetaDataBuffer = flushingRecordsContext->GetMetadataBuffer();
                
                return(STATUS_SUCCESS);
            }
            flushingRecordsContext = _FlushingRecords.Successor(flushingRecordsContext.RawPtr());
        }
    }

    K_LOCK_BLOCK(_ListLock)
    {
        //
        // If not then see if it is in any of the records that were
        // independently written outside of the coalesce buffers
        //
        independentWriteContext = _IndependentWrites.PeekHead();
        while (independentWriteContext)
        {
            if ( ((Type == RvdLogStream::AsyncReadContext::ReadExactRecord) &&
                  (RecordAsn == independentWriteContext->GetStreamOffset())) ||
                 ((Type == RvdLogStream::AsyncReadContext::ReadContainingRecord) &&
                  ((RecordAsn >= independentWriteContext->GetStreamOffset()) &&
                   (RecordAsn < (independentWriteContext->GetStreamOffset() + independentWriteContext->GetDataSize())))) )
            {
                //
                // Since we are pointing at a context that is already queued for write then we
                // know it will not change. So we take copies of the metadata and IoBuffers. 
                //
                KDbgCheckpointWDataInformational(_OverlayStream->GetActivityId(),
                                    "CoalesceRecords::FindDataInFlushingRecords IndependentWrite", STATUS_SUCCESS,
                                    (ULONGLONG)RecordAsn.Get(),
                                    (ULONGLONG)this,
                                    (ULONGLONG)0,
                                    (ULONGLONG)0);

                RecordAsn = independentWriteContext->GetStreamOffset();
                Version = independentWriteContext->GetVersion();

#ifdef UPASSTHROUGH
                //
                // For inproc logger, make a copy of the IoBuffer since
                // the logical log will call Clear() on it. This is not
                // needed in the case of the KDRIVER or UDRIVER since a
                // copy is made when crossing the user/kernel boundary.
                //
                status = independentWriteContext->GetIoBuffer()->CreateAlias(IoBuffer);
                if (! NT_SUCCESS(status))
                {
                    return(status);
                }               
#else
                IoBuffer = independentWriteContext->GetIoBuffer();
#endif              
                MetaDataBuffer = independentWriteContext->GetMetadataBuffer();
                
                return(STATUS_SUCCESS);
            }
            
            independentWriteContext = _IndependentWrites.Successor(independentWriteContext.RawPtr());
        }

        //
        // Finally check the waiting requests list
        //
        waitingWriteContext = _WaitingCoalescingWrites.PeekHead();
        while (waitingWriteContext)
        {
            //
            // Only consider those contexts that are writes and not flushes
            //
            if (waitingWriteContext->IsAppendContext())
            {           
                if ( ((Type == RvdLogStream::AsyncReadContext::ReadExactRecord) &&
                      (RecordAsn == waitingWriteContext->GetStreamOffset())) ||
                     ((Type == RvdLogStream::AsyncReadContext::ReadContainingRecord) &&
                      ((RecordAsn >= waitingWriteContext->GetStreamOffset()) &&
                       (RecordAsn < (waitingWriteContext->GetStreamOffset() + waitingWriteContext->GetDataSize())))) )
                {
                    //
                    // Since we are pointing at a context that is already queued for write then we
                    // know it will not change. So we take copies of the metadata and IoBuffers. 
                    //
                    KDbgCheckpointWDataInformational(_OverlayStream->GetActivityId(),
                                        "CoalesceRecords::FindDataInFlushingRecords WaitingWrite", STATUS_SUCCESS,
                                        (ULONGLONG)RecordAsn.Get(),
                                        (ULONGLONG)this,
                                        (ULONGLONG)0,
                                        (ULONGLONG)0);

                    RecordAsn = waitingWriteContext->GetStreamOffset();
                    Version = waitingWriteContext->GetVersion();
#ifdef UPASSTHROUGH
                    //
                    // For inproc logger, make a copy of the IoBuffer since
                    // the logical log will call Clear() on it. This is not
                    // needed in the case of the KDRIVER or UDRIVER since a
                    // copy is made when crossing the user/kernel boundary.
                    //
                    status = waitingWriteContext->GetIoBuffer()->CreateAlias(IoBuffer);
                    if (! NT_SUCCESS(status))
                    {
                        return(status);
                    }               
#else
                    IoBuffer = waitingWriteContext->GetIoBuffer();
#endif              
                    
                    MetaDataBuffer = waitingWriteContext->GetMetadataBuffer();
                    return(STATUS_SUCCESS);
                }
            }
            
            waitingWriteContext = _WaitingCoalescingWrites.Successor(waitingWriteContext.RawPtr());
        }       
    }
    
    return(STATUS_NOT_FOUND);
}
