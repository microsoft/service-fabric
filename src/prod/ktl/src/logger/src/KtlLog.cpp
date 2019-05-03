/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    RvdLog.cpp

    Description:
      RvdLog implementation.

    History:
      PengLi/richhas        22-Oct-2010         Initial version.

--*/

#include "InternalKtlLogger.h"

#include "ktrace.h"

#pragma prefast(push)
#pragma prefast(disable: 28167, "DevDiv:422165: Prefast not recognizing c++ dtor being called")

//** RvdLog base class ctor and dtor.
//
RvdLog::RvdLog()
{
}

RvdLog::~RvdLog()
{
}

//** RvdLogManagerImp::RvdOnDiskLog Implementation
RvdLogManagerImp::RvdOnDiskLog::RvdOnDiskLog(
    __in RvdLogManagerImp* const LogManager, 
    __in KGuid& DiskId,
    __in RvdLogId& LogId,
    __in KWString& FullyQualifiedLogName)
    :   _LogManager(LogManager),
        _DiskId(DiskId),
        _LogId(LogId),
        _IsOpen(FALSE),
        _Streams(FIELD_OFFSET(LogStreamDescriptor, Links)),
        _IsLogFailed(FALSE),
        _CheckPointStream(nullptr),
        _LastUsedLogStreamInfoIx(0),
        _CountOfOutstandingWrites(0),
        _IncomingStreamWriteOpQueueRefCount(FALSE),
        _CompletingStreamWriteLsnOrderedGateDispatched(FALSE),
        _DebugDisableAutoCheckpointing(FALSE),
        _DebugDisableTruncateCheckpointing(FALSE),
        _DebugDisableAssertOnLogFull(TRUE),
        _FullyQualifiedLogName(FullyQualifiedLogName),
        _DebugDisableHighestCompletedLsnUpdates(FALSE),
        _LogStreamInfoArray(GetThisAllocator(), 0),
        _ShutdownEvent(nullptr)
{
    NTSTATUS status = _LogStreamInfoArray.Status();

    // Start the incoming stream write queue - orders in records in LSN space
    if (NT_SUCCESS(status))
    {
        _IncomingStreamWriteDequeueCallback = KAsyncContextBase::CompletionCallback(this, &RvdOnDiskLog::IncomingStreamWriteDequeueCallback);
        status = AsyncWriteStreamQueue::Create(
            KTL_TAG_LOGGER,
            GetThisAllocator(),
            FIELD_OFFSET(RvdLogStreamImp::AsyncWriteStream, _WriteQueueLinks),
            _IncomingStreamWriteOpQueue);

        if (NT_SUCCESS(status))
        {
            status = _IncomingStreamWriteOpQueue->Activate(nullptr, nullptr);
            if (NT_SUCCESS(status))
            {
                // Start the pump on _IncomingStreamWriteOpQueue. This pump will receive
                // incomplete stream write ops as queued entries - cause them to be processed
                // in order thru IncomingStreamWriteDequeueCallback().

                status = _IncomingStreamWriteOpQueue->CreateDequeueOperation(_IncomingStreamWriteDequeueOp);
                if (NT_SUCCESS(status))
                {
                    _IncomingStreamWriteDequeueOp->StartDequeue(nullptr, _IncomingStreamWriteDequeueCallback);
                }
            }
        }
    }

    // Start the WriteCompletionGate pump - completes stream writes in LSN order
    if (NT_SUCCESS(status))
    {
        _CompletingStreamWriteLsnOrderedGateCallback = KAsyncContextBase::CompletionCallback(
            this, 
            &RvdOnDiskLog::CompletingStreamWriteLsnOrderedGateCallback);

        status = AsyncWriteStreamGate::Create(
            KTL_TAG_LOGGER,
            GetThisAllocator(),
            FIELD_OFFSET(RvdLogStreamImp::AsyncWriteStream, _WriteQueueLinks),
            _CompletingStreamWriteLsnOrderedGate);


        if (NT_SUCCESS(status))
        {
            // NOTE: In the case of log mount (recovery) ActivateGate::SetNextOrderedItemValue() is called 
            //       with next highest LSN to be written.
            status = _CompletingStreamWriteLsnOrderedGate->ActivateGate(RvdLogLsn::Min(), nullptr, nullptr);
            if (NT_SUCCESS(status))
            {
                // Start the pump on _CompletingStreamWriteLsnOrderedGate. This pump will receive
                // completed stream write ops in LSN sequence - they will be processed thru
                // CompletingStreamWriteLsnOrderedGateCallback().
                KSharedPtr<AsyncWriteStreamGate::OrderedDequeueOperation> completingStreamWriteOps;
                status = _CompletingStreamWriteLsnOrderedGate->CreateOrderedDequeueOperation(completingStreamWriteOps);
                if (NT_SUCCESS(status))
                {
                    completingStreamWriteOps->StartDequeue(nullptr, _CompletingStreamWriteLsnOrderedGateCallback);
                }
            }
        }
    }

    // Allocate empty KBuffer and KIoBuffer to be used by WriteStream logic
    if (NT_SUCCESS(status))
    {
        status = KBuffer::Create(0, _EmptyBuffer, GetThisAllocator(), KTL_TAG_LOGGER);
        if (NT_SUCCESS(status))
        {
            status = KIoBuffer::CreateEmpty(_EmptyIoBuffer, GetThisAllocator(), KTL_TAG_LOGGER);
        }
    }

    if (!NT_SUCCESS(status))
    {
        if (_IncomingStreamWriteOpQueue != nullptr)
        {
            _IncomingStreamWriteOpQueue->Deactivate();
            _IncomingStreamWriteOpQueue.Reset();
        }

        if (_CompletingStreamWriteLsnOrderedGate != nullptr)
        {
            _CompletingStreamWriteLsnOrderedGate->DeactivateGate();
            _CompletingStreamWriteLsnOrderedGate.Reset();
        }

        RvdLog::SetConstructorStatus(status);
        return;
    }
}

NTSTATUS
RvdLogManagerImp::RvdOnDiskLog::CompleteActivation(__in RvdLogConfig& Config)
{
    KInvariant(_LogStreamInfoArray.Count() == 0);       // Only allow calling CompleteActivation() once

    _Config.Set(Config);
    _MaxCheckPointLogRecordSize = RvdLogPhysicalCheckpointRecord::ComputeMaxSizeOnDisk(_Config);
    _FreeLogStreamInfoCount = _Config.GetMaxNumberOfStreams();

    NTSTATUS status = _LogStreamInfoArray.Reserve(_Config.GetMaxNumberOfStreams());
    if (NT_SUCCESS(status))
    {
        KInvariant(_LogStreamInfoArray.SetCount(_Config.GetMaxNumberOfStreams()));
        for (ULONG ix = 0; ix < _Config.GetMaxNumberOfStreams(); ix++)
        {
            _LogStreamInfoArray[ix].LogStreamId = RvdDiskLogConstants::FreeStreamInfoStreamId();
        }

        // Start the _StreamWriteQuotaGate - limits the depth of outstanding writes (in terms of bytes) at any moment
        if (NT_SUCCESS(status))
        {
            status = KQuotaGate::Create(KTL_TAG_LOGGER, GetThisAllocator(), _StreamWriteQuotaGate);
        }

        if (NT_SUCCESS(status))
        {
            status = _StreamWriteQuotaGate->Activate(_Config.GetMaxQueuedWriteDepth(), nullptr, nullptr);
            if (!NT_SUCCESS(status))
            {
                _StreamWriteQuotaGate = nullptr;
            }
        }
    }

    return status;
}


RvdLogManagerImp::RvdOnDiskLog::~RvdOnDiskLog()
{
    // Stop the pumps - shutsdown in an async manner - but the only reason
    // this instance should be dtor'ing is because no Stream (and indirectly
    // their ops) have a ref to this instance. Meaning that there should not
    // by any activity in _IncomingStreamWriteOpQueue or _CompletingStreamWriteLsnOrderedGate.
    if (_IncomingStreamWriteOpQueue != nullptr)
    {
        _IncomingStreamWriteOpQueue->Deactivate();
    }

    if (_CompletingStreamWriteLsnOrderedGate != nullptr)
    {
        _CompletingStreamWriteLsnOrderedGate->DeactivateGate();
    }

    if (_StreamWriteQuotaGate != nullptr)
    {
        _StreamWriteQuotaGate->Deactivate();
        _StreamWriteQuotaGate.Reset();
    }

    LogStreamDescriptor* topStream;
    while ((topStream = _Streams.RemoveHead()) != nullptr)
    {
        if (topStream == _CheckPointStream)
        {
            // Special case for the _CheckPointStream
            KAssert(topStream->State != LogStreamDescriptor::OpenState::Deleting);
            topStream->State = LogStreamDescriptor::OpenState::Closed;
            topStream->WRef.Reset();        // might as well clean up wref while we're here
            _CheckPointStream = nullptr;
        }

        KAssert(topStream->State == LogStreamDescriptor::OpenState::Closed);
        _delete(topStream);
    }

    _LogManager->DiskLogDestructed();

    if (_BlockDevice)
    {
        _BlockDevice->Close();
    }
    
    if (_ShutdownEvent != nullptr)
    {
        KDbgCheckpointWData((KActivityId)(_LogId.Get().Data1), "SetEventForRvdLog", STATUS_SUCCESS,
                            (ULONGLONG)0,
                            (ULONGLONG)this,
                            (ULONGLONG)0,
                            (ULONGLONG)0);                                
        _ShutdownEvent->SetEvent();
        _ShutdownEvent = nullptr;
    }
}

VOID 
RvdLogManagerImp::RvdOnDiskLog::QueryLogType(__out KWString& LogType)
{
    LogType = _LogType;
}
    
VOID 
RvdLogManagerImp::RvdOnDiskLog::QueryLogId(__out KGuid& DiskId, __out RvdLogId& LogId)
{
    DiskId = _DiskId;
    LogId = _LogId;
}
    
VOID 
RvdLogManagerImp::RvdOnDiskLog::QuerySpaceInformation(
    __out_opt ULONGLONG* const TotalSpace, 
    __out_opt ULONGLONG* const FreeSpace)
{
    if (TotalSpace != nullptr)
    {
        *TotalSpace = _LogFileLsnSpace;
    }
    if (FreeSpace != nullptr)
    {
        *FreeSpace = _LogFileFreeSpace;
    }
}

ULONGLONG
RvdLogManagerImp::RvdOnDiskLog::QueryReservedSpace()
{
    return(2 * QueryMaxRecordSize());
}


VOID
RvdLogManagerImp::RvdOnDiskLog::QueryLsnRangeInformation(
    __out LONGLONG& LowestLsn,
    __out LONGLONG& HighestLsn,
    __out RvdLogStreamId& LowestLsnStreamId
    )
{
    KGuid guidNull(0x00000000,0x0000,0x0000,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00);
    
    LowestLsn = RvdLogLsn::Max().Get();
    HighestLsn = _NextLsnToWrite.Get();
    LowestLsnStreamId = guidNull;   

    //
    // Search all of the active streams for the lowest lsn in the log
    // so that its stream id can be returned.
    //
    K_LOCK_BLOCK(_ThisLock)
    {
        LogStreamDescriptor* sDesc = _Streams.PeekHead();
        while (sDesc != nullptr)
        {                 
            if ((sDesc->State != LogStreamDescriptor::OpenState::Deleting) &&
                (! (sDesc->Info.IsEmptyStream())) &&    // not empty
                (! (sDesc->Info.LogStreamType == RvdDiskLogConstants::CheckpointStreamType())))
            {
                if (sDesc->Info.LowestLsn < LowestLsn)
                {
                    LowestLsn = sDesc->Info.LowestLsn.Get();
                    LowestLsnStreamId = sDesc->Info.LogStreamId;
                }

            }
            sDesc = _Streams.Successor(sDesc);
        }
    }
}

    
VOID 
RvdLogManagerImp::RvdOnDiskLog::QueryCacheSize(
    __out_opt LONGLONG* const ReadCacheSizeLimit, 
    __out_opt LONGLONG* const ReadCacheUsage)
{
    // BUG, richhas, xxxxx, FINISH
    if (ReadCacheSizeLimit != nullptr)
    {
        *ReadCacheSizeLimit = 0;
    }
    if (ReadCacheUsage != nullptr)
    {
        *ReadCacheUsage = 0;
    }
}
    
VOID 
RvdLogManagerImp::RvdOnDiskLog::SetCacheSize(__in LONGLONG CacheSizeLimit)
{
    UNREFERENCED_PARAMETER(CacheSizeLimit);
    
    // BUG, richhas, xxxxx, FINISH
}

ULONG
RvdLogManagerImp::RvdOnDiskLog::UnsafeAllocateStreamInfo()
//
// Returns: MAXULONG if no free items in _LogStreamInfoArray
{
    //  The array, _LogStreamInfoArray, is kept in a form so that it can be written directly to
    //  the log during a physical cp (see SnapSafeCopyOfStreamsTable()). To accomplish this we
    //  keep current a highest element used index (_LastUsedLogStreamInfoIx) such that the set
    //  _LogStreamInfoArray(0.._LastUsedLogStreamInfoIx) will contain all stream descriptors that
    //  are not marked as free (FreeStreamInfoStreamId). This set will contain a combination of
    //  entries that are being used, about to be used (InvalidLogStreamId), or free entries.
    //
    //  This routine attempts to allocate the lowest free entry and re-computes the current
    //  _LastUsedLogStreamInfoIx value. An allocated is marked with a stream id of 
    //  InvalidLogStreamId.
    //
    ULONG       result = MAXULONG;

    KAssert(_Config.GetMaxNumberOfStreams() == _LogStreamInfoArray.Count());
    _LastUsedLogStreamInfoIx = _LogStreamInfoArray.Count();

    for (ULONG ix = 0; ix < _LogStreamInfoArray.Count(); ix++)
    {
        if (_LogStreamInfoArray[ix].LogStreamId == RvdDiskLogConstants::FreeStreamInfoStreamId())
        {
            if (result == MAXULONG)
            {
                // Allocate 1st free slot
                KAssert(_FreeLogStreamInfoCount > 0);
                _FreeLogStreamInfoCount--;
                _LogStreamInfoArray[ix].LogStreamId = RvdDiskLogConstants::InvalidLogStreamId();
                result = ix;

                // Set of allocated streams contains at least [ix]
                _LastUsedLogStreamInfoIx = ix;
            }
        }
        else
        {
            // Not a free slot... Remember [ix] in the set of allocated streams
            _LastUsedLogStreamInfoIx = ix;
        }
    }

    return result;
}

VOID
RvdLogManagerImp::RvdOnDiskLog::UnsafeDeallocateStreamInfo(__inout RvdLogStreamInformation& EntryToFree)
{
    KAssert(_Config.GetMaxNumberOfStreams() == _LogStreamInfoArray.Count());
    KAssert(_FreeLogStreamInfoCount < _LogStreamInfoArray.Count());

    ULONGLONG volatile siIndexToFree = &EntryToFree - &_LogStreamInfoArray[0];
    KAssert(siIndexToFree < _LogStreamInfoArray.Count());
    KAssert(siIndexToFree <= _LastUsedLogStreamInfoIx);

    EntryToFree.LogStreamId = RvdDiskLogConstants::FreeStreamInfoStreamId();
    _FreeLogStreamInfoCount++;

    if ((_LastUsedLogStreamInfoIx == siIndexToFree) && 
        (_FreeLogStreamInfoCount != _LogStreamInfoArray.Count()))
    {
        // If freeing the highest used entry and not an empty table, recompute _LastUsedLogStreamInfoIx
#pragma warning(disable:4127)   // C4127: conditional expression is constant
        while (TRUE)
        {
            if (_LogStreamInfoArray[(ULONG)siIndexToFree].LogStreamId != RvdDiskLogConstants::FreeStreamInfoStreamId())
            {
                _LastUsedLogStreamInfoIx = (ULONG)siIndexToFree;
                return;
            }

            KAssert(siIndexToFree > 0);
            siIndexToFree--;
        }
    }
}
    
ULONG
RvdLogManagerImp::RvdOnDiskLog::SnapSafeCopyOfStreamsTable(__out RvdLogStreamInformation* DestLocation)
//
// Parameters:
//      DestLocation -> RvdLogStreamInformation[_LogStreamInfoArray.Count()]
//
// Returns: 
//      Number of entires stored @DestLocation
//
{
    KAssert(_Config.GetMaxNumberOfStreams() == _LogStreamInfoArray.Count());
    ULONG       result = 0;

    K_LOCK_BLOCK(_ThisLock)
    {
        result = _LogStreamInfoArray.Count() - _FreeLogStreamInfoCount;
        if (result > 0)
        {
            result = _LastUsedLogStreamInfoIx + 1;
            KMemCpySafe(
                &DestLocation[0], 
                _LogStreamInfoArray.Count() * sizeof(RvdLogStreamInformation), 
                &_LogStreamInfoArray[0], 
                result * sizeof(RvdLogStreamInformation));
        }
    }

    return result;
}

NTSTATUS
RvdLogManagerImp::RvdOnDiskLog::FindOrCreateStream(
    __in BOOLEAN const MustCreate,
    __in const RvdLogStreamId& LogStreamId, 
    __in const RvdLogStreamType& LogStreamType,
    __out KSharedPtr<RvdLogStreamImp>& ResultStream,
    __out LogStreamDescriptor*& ResultStreamInfo)
//
//  Common logic for maintaining _Streams for stream open, create, and delete Async Contexts
//
//  Parameters:
//      MustCreate      - TRUE if a Create is being done - there must not be existing interest in the
//                        subject stream.
//      LogStreamId     - Unique ID of the stream being accessed
//      LogStreamType   - The type of stream when MustCreate == TRUE
//      ResultStream    - On STATUS_SUCCESS this SPtr will contain a ref to the underlying RvdLogStreamImp
//                        for stream LogStreamId.
//      ResultStreamInfo- If STATUS_SUCCESS *ResultStreamInfo the LogStreamDescriptor instance related 
//                        to ResultStream
//
//  Returns: 
//      STATUS_SUCCESS:                 A ref counted RvdLogStreamImp* is returned via ResultStream
//      STATUS_INSUFFICIENT_RESOURCES:  RvdLogStreamImp could not be allocated - out of memory
//      STATUS_OBJECT_NAME_COLLISION:   Attempt to Create an existing log stream that is already open
//      STATUS_OBJECT_NAME_NOT_FOUND:   Attempt to Open/Delete a non-existing stream
//
{
    // BUG, richhas, xxxxx, finish adding RvdLogStreamType support

    ResultStream.Reset();
    ResultStreamInfo = nullptr;
    
    RvdLogStreamImp::SPtr   streamObject;
    NTSTATUS                status = STATUS_SUCCESS;
    LogStreamDescriptor*    streamPtr = nullptr;
    BOOLEAN                 streamDescWasCreated = FALSE;

    K_LOCK_BLOCK(_ThisLock)
    {
        // Try to find an existing log object.
        streamPtr = _Streams.PeekHead();
        while (streamPtr != nullptr)
        {
            if ((streamPtr->WRef) && !streamPtr->WRef->IsAlive() && (streamPtr !=_CheckPointStream))
            {
                // Clean up stranded wrefs as we go
                streamPtr->WRef.Reset();
                KAssert(streamPtr->State != LogStreamDescriptor::OpenState::Opened);
            }

            if (streamPtr->Info.LogStreamId == LogStreamId)
            {
                break;
            }

            streamPtr = _Streams.Successor(streamPtr);      // for the whole list
        }

        if (streamPtr != nullptr)
        {
            // Have matching Stream instance
            if (MustCreate)
            {
                // Trying to create an existing stream
                return STATUS_OBJECT_NAME_COLLISION;
            }

            if (streamPtr->State == LogStreamDescriptor::OpenState::Deleting)
            {
                // Once a stream is in this state, it will never leave that
                // state - indicate delete pending
                return STATUS_DELETE_PENDING;
            }

            if (streamPtr->WRef)
            {
                // Attempt to gain access to wref'd API object
                streamObject = streamPtr->WRef->TryGetTarget();
            }
        }
        else
        {
            if (!MustCreate)
            {
                // Attempt to open non-existing stream
                return STATUS_OBJECT_NAME_NOT_FOUND;
            }

            // New stream being created
            LogStreamDescriptor*     newStream;
            status = UnsafeAllocateLogStreamDescriptor(LogStreamId, LogStreamType, newStream);
            if (!NT_SUCCESS(status))
            {
                return status;
            }

            _Streams.AppendTail(newStream);
            streamPtr = newStream;
            streamDescWasCreated = TRUE;
        }

        if (!streamObject)
        {
            // Create new stream API object
            streamPtr->SerialNumber++;
            streamObject = _new(KTL_TAG_LOGGER, GetThisAllocator()) RvdLogStreamImp(*this, *streamPtr, streamPtr->SerialNumber);

            if (!streamObject)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
            else
            {
                status = streamObject->Status();
            }
            if (!NT_SUCCESS(status))
            {
                if (streamDescWasCreated)
                {
                    UnsafeDeallocateStreamInfo(streamPtr->Info);
                    _Streams.Remove(streamPtr);
                    _delete(streamPtr);
                    streamPtr = nullptr;
                }

                return status;
            }

            streamPtr->WRef = streamObject->GetWeakRef();
        }
    }

    ResultStream = Ktl::Move(streamObject);
    ResultStreamInfo = streamPtr;
    return STATUS_SUCCESS;
}

NTSTATUS
RvdLogManagerImp::RvdOnDiskLog::UnsafeAllocateLogStreamDescriptor(
    __in const RvdLogStreamId& LogStreamId, 
    __in const RvdLogStreamType& LogStreamType, 
    __out LogStreamDescriptor*& Result)
{
    NTSTATUS        status = STATUS_SUCCESS;

    Result = nullptr;
    ULONG allocatedStreamInfo = UnsafeAllocateStreamInfo();
    if (allocatedStreamInfo == MAXULONG)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    LogStreamDescriptor*     newStream = _new(KTL_TAG_LOGGER, GetThisAllocator()) 
                                            LogStreamDescriptor(
                                                GetThisAllocator(), 
                                                _LogStreamInfoArray[allocatedStreamInfo]);
    if (newStream == nullptr)
    {
        UnsafeDeallocateStreamInfo(_LogStreamInfoArray[allocatedStreamInfo]);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (LogStreamId != RvdDiskLogConstants::CheckpointStreamId())
    {
        // Pre-allocate a write op for this stream's checkpoint records
        status = RvdLogStreamImp::AsyncWriteStream::CreateCheckpointRecordAsyncWriteContext(
            GetThisAllocator(),
            *this,
            *newStream,
            newStream->CheckPointRecordWriteOp);

        if (NT_SUCCESS(status))
        {
            status = RvdLogStreamImp::AsyncWriteStream::CreateTruncationAsyncWriteContext(
                GetThisAllocator(),
                *this,
                *newStream,
                newStream->StreamTruncateWriteOp);
        }

        if (NT_SUCCESS(status))
        {
            status = KInstrumentedComponent::Create(newStream->_InstrumentedComponent,
                                                  GetThisAllocator(),
                                                  GetThisAllocationTag());
            if (NT_SUCCESS(status))
            {
                //
                // Configure InstrumentedComponent
                //
                KStringView prefix(LogContainerText);
                status = newStream->_InstrumentedComponent->SetComponentName(prefix,
                                                                             _LogId.Get(),
                                                                             LogStreamId.Get());
                newStream->_InstrumentedComponent->SetReportFrequency(KInstrumentedComponent::DefaultReportFrequency);
            }
        }       
    }

    if (!NT_SUCCESS(status))
    {
        UnsafeDeallocateStreamInfo(_LogStreamInfoArray[allocatedStreamInfo]);
        _delete(newStream);
        return status;
    }

    // Init on disk stream info
    newStream->Info.LogStreamId = LogStreamId;
    newStream->Info.LogStreamType = LogStreamType;
    newStream->Info.HighestLsn = RvdLogLsn::Null();
    newStream->Info.LowestLsn = RvdLogLsn::Null();
    newStream->Info.NextLsn = RvdLogLsn::Null();
    Result = newStream;

    return STATUS_SUCCESS;
}

NTSTATUS
RvdLogManagerImp::RvdOnDiskLog::PopulateLogState(__in LogState::SPtr& State)
{
    K_LOCK_BLOCK(_ThisLock)
    {
        KAssert(_Streams.IsEmpty());
        KAssert(!_IsOpen);
        KAssert(State->_NumberOfStreams != 0);

        NTSTATUS status = CompleteActivation(State->_Config);
        if (!NT_SUCCESS(status))
        {
            return status;
        }
        
        _LogFileMinFreeSpace = _Config.GetMinFreeSpace();
        KMemCpySafe(&_LogSignature[0], sizeof(_LogSignature), &State->_LogSignature[0], RvdLogRecordHeader::LogSignatureSize);
        _LogFileSize = State->_LogFileSize;
        _LogFileLsnSpace = State->_LogFileLsnSpace;
        _MaxCheckPointLogRecordSize = State->_MaxCheckPointLogRecordSize;
        _LogFileFreeSpace = State->_LogFileFreeSpace;
        _LogFileReservedSpace = 0;
        _LowestLsn = State->_LowestLsn;
        _NextLsnToWrite = State->_NextLsnToWrite;
        _HighestCompletedLsn = State->_HighestCompletedLsn;
        _CompletingStreamWriteLsnOrderedGate->SetNextOrderedItemValue(_NextLsnToWrite);
        KMemCpySafe(&_LogType, sizeof(_LogType), &State->_LogType, RvdLogManager::AsyncCreateLog::MaxLogTypeLength*sizeof(WCHAR));

        for (ULONG ix = 0; ix < State->_NumberOfStreams; ix++)
        {
            // copy info
            LogStreamDescriptor*     newStream;
            status = UnsafeAllocateLogStreamDescriptor(
                State->_StreamDescs[ix]._Info.LogStreamId, 
                State->_StreamDescs[ix]._Info.LogStreamType, 
                newStream);

            if (!NT_SUCCESS(status))
            {
                return status;
            }

            newStream->Info.HighestLsn = State->_StreamDescs[ix]._Info.HighestLsn;
            newStream->Info.LowestLsn = State->_StreamDescs[ix]._Info.LowestLsn;
            newStream->Info.NextLsn = State->_StreamDescs[ix]._Info.NextLsn;

            if (newStream->Info.LogStreamId == RvdDiskLogConstants::CheckpointStreamId())
            {
                _CheckPointStream = newStream;
            }

            _Streams.AppendTail(newStream);
        }
    }

    KInvariant(_CheckPointStream != nullptr);
    return STATUS_SUCCESS;
}

VOID
RvdLogManagerImp::RvdOnDiskLog::ComputeUnusedFileRanges(
    __out ULONGLONG& FromRange1,
    __out ULONGLONG& ToRange1,
    __out ULONGLONG& FromRange2,
    __out ULONGLONG& ToRange2
)
{
    ULONGLONG i = 0;
    ULONGLONG lowestLsn;
    ULONGLONG startOffset;
    RvdLogLsn endPlusProtected;
    ULONGLONG endOffset;
    
    //
    // _LowestLsn may be short of a block boundary so in this case we bump it 
    // up to be on the next block boundary.
    //
    ULONGLONG blockSize = RvdDiskLogConstants::BlockSize;
    lowestLsn = (_LowestLsn.Get() + (blockSize-1)) & ~(blockSize-1);

    //
    // Used space is between lowestLsn and _NextLsnToWrite
    //

    //
    // Since recovery will search the region of chaos at the end of the
    // log, we never want to trim more than MaxQueuedWriteDepth or else
    // the recovery code may not work. In the case where less than
    // MaxQueuedWriteDepth has been written then we want to do no
    // trimming at all.
    //
    ULONGLONG minimumKeepSize = _Config.GetMaxQueuedWriteDepth();
    ULONGLONG keepSize = _NextLsnToWrite.Get() - lowestLsn;
    ULONGLONG nextLsnToWrite = _NextLsnToWrite.Get();

    if (nextLsnToWrite <= minimumKeepSize)
    {
        KDbgCheckpointWDataInformational(_LogId.Get().Data1, "Trim lowkeep", STATUS_SUCCESS, _LowestLsn.Get(), _NextLsnToWrite.Get(), minimumKeepSize, lowestLsn);
        FromRange1 = 0;
        ToRange1 = 0;

        FromRange2 = 0;
        ToRange2 = 0;
        i = 1;
        return;
    }
    
    if (keepSize < minimumKeepSize)
    {
        lowestLsn = nextLsnToWrite - minimumKeepSize;
        KDbgCheckpointWDataInformational(_LogId.Get().Data1, "Trim minkeep", STATUS_SUCCESS, _LowestLsn.Get(), _NextLsnToWrite.Get(), minimumKeepSize, lowestLsn);
    }
    

    //
    // Convert to file addresses
    //
    startOffset = (lowestLsn == 0) ? ToFileAddress(lowestLsn) : ToFileAddress(lowestLsn - 1) + 1;
    endPlusProtected = _NextLsnToWrite;
    endOffset = ToFileAddress(endPlusProtected.Get() - 1) + 1;
    

    
    if (startOffset == endOffset)
    {
        //
        // The whole log is full. No trimming
        //
        FromRange1 = 0;
        ToRange1 = 0;

        FromRange2 = 0;
        ToRange2 = 0;
        i = 1;
    }
    else if (startOffset < endOffset)
    {
        ULONGLONG fileAddress0 = ToFileAddress(0);
        if (startOffset == fileAddress0)
        {
            //
            // Free space is in one segment at the end of the log
            //
            FromRange1 = endOffset;
            ToRange1 = ToFileAddress(_LogFileLsnSpace - 1) + 1;

            FromRange2 = 0;
            ToRange2 = 0;
            i = 4;
        }
        else 
        {
            //
            // Free space is in 2 chunks
            //
            FromRange1 = fileAddress0;
            ToRange1 = startOffset;

            FromRange2 = endOffset;
            ToRange2 = ToFileAddress(_LogFileLsnSpace - 1) + 1;
            i = 2;
        }
    }
    else if (startOffset > endOffset)
    {
        //
        // Free space is in a single chunk
        //
        FromRange1 = endOffset;
        ToRange1 = startOffset;

        FromRange2 = 0;
        ToRange2 = 0;
        i = 3;
    }

    KDbgCheckpointWDataInformational(_LogId.Get().Data1, "Trim locatio", STATUS_SUCCESS, _LowestLsn.Get(), _NextLsnToWrite.Get(), lowestLsn, _LogFileLsnSpace);
    KDbgCheckpointWDataInformational(_LogId.Get().Data1, "Trim data   ", STATUS_SUCCESS, startOffset, endOffset, i, 0);
    KDbgCheckpointWDataInformational(_LogId.Get().Data1, "Trim offsets", STATUS_SUCCESS, FromRange1, ToRange1, FromRange2, ToRange2);
}



RvdLogLsn
RvdLogManagerImp::RvdOnDiskLog::ComputeLowestUsedLsn()
//
// Return the LSN for the lowest used location in the log's LSN-space.
// All streams that are not being deleted an not emtpy are considered. 
{
    RvdLogLsn   minLsnUsed = RvdLogLsn::Max();

    K_LOCK_BLOCK(_ThisLock)
    {
        LogStreamDescriptor* sDesc = _Streams.PeekHead();
        while (sDesc != nullptr)
        {
            if ((sDesc->State != LogStreamDescriptor::OpenState::Deleting) &&
                (! (sDesc->Info.IsEmptyStream())) &&    // not empty
                (! (sDesc->Info.LogStreamType == RvdDiskLogConstants::CheckpointStreamType())) &&
                (sDesc->Info.LowestLsn < minLsnUsed))
            {
                // For each non-deleting and non-empty stream, capture lsn interesting lsn
                minLsnUsed = sDesc->Info.LowestLsn;
            }
            sDesc = _Streams.Successor(sDesc);
        }
    }

    // Return either the min lsn still used across all existing streams OR all streams are
    // empty and thus the log as a whole is empty
    return (minLsnUsed == RvdLogLsn::Max()) ? _NextLsnToWrite : minLsnUsed;
}

NTSTATUS
RvdLogManagerImp::RvdOnDiskLog::StartDeleteStream(
    __in RvdLogStreamId& LogStreamId, 
    __out KSharedPtr<RvdLogStreamImp>& ResultStreamSPtr,
    __out LogStreamDescriptor*& ResultStreamDesc)
//
// Returns:
//      STATUS_DELETE_PENDING           - Delete is already pending
//      STATUS_OBJECT_NAME_NOT_FOUND    - Attempting to start deletion on a non existing stream
//      STATUS_SUCCESS                  - Stream id is valid:
//          ResultStreamSPtr == null        - State has been set to Deleting - deletion can 
//                                            occur now. ResultStream is safe because State
//                                            has been set to Deleting.
//          ResultStreamSPtr != null        - ResultStream-> LogStreamDescriptor which is protected because the
//                                            ResultStreamSPtr is keeping a ref on the underlying
//                                            RvdLogStreamImp - deletion can continue after the
//                                            OpenCloseDeleteLock is acquired and State proven
//                                            to not be Deleting at that point.
//          ResultStream                    - Points to the LogStreamDescriptor entry in _Streams list for LogStreamId
{
again:
    ResultStreamSPtr.Reset();
    ResultStreamDesc = nullptr;

    K_LOCK_BLOCK(_ThisLock)
    {
        LogStreamDescriptor* streamPtr = _Streams.PeekHead();

        while (streamPtr != nullptr)
        {
            if (streamPtr->Info.LogStreamId == LogStreamId)
            {
                if (streamPtr->State == LogStreamDescriptor::OpenState::Deleting)
                {
                    return STATUS_DELETE_PENDING;
                }
                
                if (streamPtr->WRef)
                {
                    ResultStreamSPtr = streamPtr->WRef->TryGetTarget();
                }

                if (!ResultStreamSPtr)
                {
                    // There's no RvdLogStreamImp and therefore there can't be any open/close ops
                    // in flight as they both must go thru FindOrCreateStream() which will either
                    // have created an underlying RvdLogStreamImp or returned an error. The other
                    // impication is that OpenCloseDeleteLock is not held - State can just be set 
                    // to Deleting - blocking all other open/create activity on this Stream id 
                    // until the current delete op completes and *streamPtr is removed at the end 
                    // of that process.
                    if (streamPtr->State != LogStreamDescriptor::OpenState::Closed)
                    {
                        //
                        // This handles a rare race condition where
                        // close is racing with delete. The close is in
                        // process within the destructor for
                        // RvdLogStreamImp but waiting on the
                        // _ThisLock which is held by this routine.
                        // Delete cannot proceed until the
                        // streamPtr->State is Closed. Let go of the
                        // lock and then retry. It is expected that
                        // releasing the lock will allow the destructor
                        // to run and streamPtr->State to become Closed.
                        goto again;
                    }
                    
                    streamPtr->State = LogStreamDescriptor::OpenState::Deleting;
                }

                ResultStreamDesc = streamPtr;       // ResultStream is safe
                return STATUS_SUCCESS;
            }

            streamPtr = _Streams.Successor(streamPtr);
        }
    }

    return STATUS_OBJECT_NAME_NOT_FOUND;
}

VOID
RvdLogManagerImp::RvdOnDiskLog::FinishDeleteStream(__in LogStreamDescriptor* const LogStreamDescriptor)
//
// Called by AsyncDeleteLogStream as its last act after having done all of the actual
// stream deletion work. 
{
    K_LOCK_BLOCK(_ThisLock)
    {
        KAssert(LogStreamDescriptor->State == LogStreamDescriptor::OpenState::Deleting);
        UnsafeDeallocateStreamInfo(LogStreamDescriptor->Info);
        _Streams.Remove(LogStreamDescriptor);
        _delete(LogStreamDescriptor);
    }
}

VOID
RvdLogManagerImp::RvdOnDiskLog::StreamClosed(
    __in LogStreamDescriptor& LogStreamDescriptor, 
    __in ULONG StreamImpInstanceId)
//
// Called by Stream Imp dtor as its last act - only called if there's not a delayed delete 
// of *LogStreamInfo occuring. The underyling Stream object may or may not exist at this point,
// but may have been re-opened (racing) while dtor'ing is occuring on a non-deleted stream. 
{
    K_LOCK_BLOCK(_ThisLock)
    {
        if (LogStreamDescriptor.SerialNumber == StreamImpInstanceId)
        {
            // Special case: The internal checkpoint stream is never closed once it is opened by the
            //               Log level Open(Mount)/Create logic
            if (LogStreamDescriptor.Info.LogStreamId != RvdDiskLogConstants::CheckpointStreamId())
            {
                KInvariant(LogStreamDescriptor.State != LogStreamDescriptor::OpenState::Deleting);

                // Put back the state as if ctor'd
                RvdLogAsn               defaultAsn;
                RvdLogLsn               defaultLsn;

                LogStreamDescriptor.MaxAsnWritten = defaultAsn;
                LogStreamDescriptor.TruncationPoint = defaultAsn;
                LogStreamDescriptor.HighestLsn = defaultLsn;
                LogStreamDescriptor.LastCheckPointLsn = defaultLsn;
                LogStreamDescriptor.LsnTruncationPointForStream = defaultLsn;

                LogStreamDescriptor.AsnIndex.Clear();
                LogStreamDescriptor.LsnIndex.Clear();

                LogStreamDescriptor.WRef.Reset();        // might as well clean up wref while we're here
                
                // Stream object for this stream id still exists and is logically
                // ref'ing the calling dtor'ing streamimp obj
                LogStreamDescriptor.State = LogStreamDescriptor::OpenState::Closed;

            }
        }
    }
}

NTSTATUS
RvdLogManagerImp::RvdOnDiskLog::EnqueueStreamWriteOp(__in RvdLogStreamImp::AsyncWriteStream& WriteOpToEnqueue)
{
    return _IncomingStreamWriteOpQueue->Enqueue(WriteOpToEnqueue);
    // Continued @ IncomingStreamWriteDequeueCallback()
}

VOID
RvdLogManagerImp::RvdOnDiskLog::IncomingStreamWriteDequeueCallback(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& CompletingSubOp)
//
// Continued from EnqueueStreamWriteOp()
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    if (NT_SUCCESS(CompletingSubOp.Status()))
    {
        AsyncWriteStreamQueue::DequeueOperation* dequeuedOp;
        dequeuedOp = static_cast<AsyncWriteStreamQueue::DequeueOperation*>(&CompletingSubOp);
        KAssert(dequeuedOp == _IncomingStreamWriteDequeueOp.RawPtr());

        // Allow the dequeued write op to continued 
        KAssert(!_IncomingStreamWriteOpQueueRefCount);
        _IncomingStreamWriteOpQueueRefCount = TRUE;
        NTSTATUS status = dequeuedOp->GetDequeuedItem().ContinueRecordWrite();

        if (!NT_SUCCESS(status))
        {
            // Fatal strcuture or write failure - Disable all further LOG file writes
            _IsLogFailed = TRUE;
            KDbgCheckpoint(0, "Log File Failed");
        }

        LONG        refCount = InterlockedDecrement(&_IncomingStreamWriteOpQueueRefCount);
        KInvariant(refCount >= FALSE);
        if (refCount == FALSE)
        {
            // Allow rx of next stream write op
            dequeuedOp->Reuse();
            dequeuedOp->StartDequeue(nullptr, _IncomingStreamWriteDequeueCallback);
        }
    }
    else
    {
        KDbgCheckpoint(1, "Pump being stopped");
    }
}

VOID
RvdLogManagerImp::RvdOnDiskLog::SuspendIncomingStreamWriteDequeuing()
{
    InterlockedIncrement(&_IncomingStreamWriteOpQueueRefCount);
}

VOID
RvdLogManagerImp::RvdOnDiskLog::ResumeIncomingStreamWriteDequeuing()
{
    LONG        refCount = InterlockedDecrement(&_IncomingStreamWriteOpQueueRefCount);
    KInvariant(refCount >= FALSE);
    if (refCount == FALSE)
    {
        // Allow rx of next stream write op
        _IncomingStreamWriteDequeueOp->Reuse();
        _IncomingStreamWriteDequeueOp->StartDequeue(nullptr, _IncomingStreamWriteDequeueCallback);
    }
}

NTSTATUS
RvdLogManagerImp::RvdOnDiskLog::EnqueueCompletedStreamWriteOp(__in RvdLogStreamImp::AsyncWriteStream& WriteOpToEnqueue)
{
    return _CompletingStreamWriteLsnOrderedGate->EnqueueOrdered(WriteOpToEnqueue);
    // Continued @ CompletingStreamWriteLsnOrderedGateCallback()
}

VOID
RvdLogManagerImp::RvdOnDiskLog::CompletingStreamWriteLsnOrderedGateCallback(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& CompletingSubOp)
//
// Continued from EnqueueCompletedStreamWriteOp()
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    if (NT_SUCCESS(CompletingSubOp.Status()))
    {
        AsyncWriteStreamGate::OrderedDequeueOperation* dequeuedOp;
        dequeuedOp = static_cast<AsyncWriteStreamGate::OrderedDequeueOperation*>(&CompletingSubOp);

        // Allow the dequeued write op to continued 
        KAssert(!_CompletingStreamWriteLsnOrderedGateDispatched);
        _CompletingStreamWriteLsnOrderedGateDispatched = TRUE;
        NTSTATUS status = dequeuedOp->GetDequeuedItem().ContinueRecordWriteCompletion();
        _CompletingStreamWriteLsnOrderedGateDispatched = FALSE;
        if (!NT_SUCCESS(status))
        {
            // Fatal strcuture or write failure - Disable all further LOG file writes
            _IsLogFailed = TRUE;
            KDbgCheckpoint(0, "Log File Failed");
        }

        // Allow rx of next stream write op
        dequeuedOp->Reuse();
        dequeuedOp->StartDequeue(nullptr, _CompletingStreamWriteLsnOrderedGateCallback);
    }
    else
    {
        KDbgCheckpoint(1, "Pump being stopped");
    }
}

HRESULT
RvdLogManagerImp::RvdOnDiskLog::SetShutdownEvent(__in_opt KAsyncEvent* const EventToSignal)
{
    K_LOCK_BLOCK(_ThisLock)
    {
        if ((_ShutdownEvent != nullptr) && (EventToSignal != nullptr))
        {
            return STATUS_OBJECT_NAME_COLLISION;
        }

        _ShutdownEvent = EventToSignal;
    }

    return STATUS_SUCCESS;
}


//** Test support methods
BOOLEAN
RvdLogManagerImp::RvdOnDiskLog::IsStreamIdValid(__in RvdLogStreamId StreamId)
{
    K_LOCK_BLOCK(_ThisLock)
    {
        LogStreamDescriptor* streamPtr = _Streams.PeekHead();

        while (streamPtr != nullptr)
        {
            if (streamPtr->Info.LogStreamId == StreamId)
            {
                return TRUE;
            }

            streamPtr = _Streams.Successor(streamPtr);
        }
    }

    return FALSE;
}

BOOLEAN
RvdLogManagerImp::RvdOnDiskLog::GetStreamState(
    __in RvdLogStreamId StreamId, 
    __out_opt BOOLEAN* const IsOpen, 
    __out_opt BOOLEAN* const IsClosed, 
    __out_opt BOOLEAN* const IsDeleted)
{
    if (IsOpen != nullptr)
    {
        *IsOpen = FALSE;
    }
    if (IsClosed != nullptr)
    {
        *IsClosed = FALSE;
    }
    if (IsDeleted != nullptr)
    {
        *IsDeleted = FALSE;
    }

    K_LOCK_BLOCK(_ThisLock)
    {
        LogStreamDescriptor* streamPtr = _Streams.PeekHead();

        while (streamPtr != nullptr)
        {
            if (streamPtr->Info.LogStreamId == StreamId)
            {
                switch (streamPtr->State)
                {
                    case LogStreamDescriptor::OpenState::Closed:
                        if (IsClosed != nullptr)
                        {
                            *IsClosed = TRUE;
                        }
                        break;

                    case LogStreamDescriptor::OpenState::Opened:
                        if (IsOpen != nullptr)
                        {
                            *IsOpen = TRUE;
                        }
                        break;

                    case LogStreamDescriptor::OpenState::Deleting:
                        if (IsDeleted != nullptr)
                        {
                            *IsDeleted = TRUE;
                        }
                        break;
                }
                return (TRUE);
            }

            streamPtr = _Streams.Successor(streamPtr);
        }
    }

    return FALSE;
}

BOOLEAN
RvdLogManagerImp::RvdOnDiskLog::IsLogFlushed()
{
    return _CountOfOutstandingWrites == 0;
}

 ULONG
RvdLogManagerImp::RvdOnDiskLog::GetNumberOfStreams()
 {
     ULONG          result = 0;

    K_LOCK_BLOCK(_ThisLock)
    {
        result = _Streams.Count();
    }

    return result;
 }

ULONG
RvdLogManagerImp::RvdOnDiskLog::GetStreams(__in ULONG MaxResults, __in RvdLogStreamId* Streams)
{
    ULONG               todo = 0;

    K_LOCK_BLOCK(_ThisLock)
    {
        todo = __min(MaxResults, _Streams.Count());
        LogStreamDescriptor*        nextDesc = _Streams.PeekHead();

        for (ULONG ix = 0; ix < todo; ix++)
        {
            *Streams = nextDesc->Info.LogStreamId;
            Streams++;
            nextDesc = _Streams.Successor(nextDesc);
        }
    }

    return todo;
}

VOID
RvdLogManagerImp::RvdOnDiskLog::GetPhysicalExtent(__out LONGLONG& Lowest, __out LONGLONG& NextLsn)
{
    Lowest = _LowestLsn.Get();
    NextLsn = _NextLsnToWrite.Get();
}

ULONGLONG
RvdLogManagerImp::RvdOnDiskLog::RemainingLsnSpaceToEOF()
{
    return _LogFileLsnSpace - (ToFileAddress(_NextLsnToWrite) - RvdDiskLogConstants::BlockSize);
}

NTSTATUS
RvdLogManagerImp::RvdOnDiskLog::UnsafeSnapLogState(
    __out LogState::SPtr& Result, 
    __in ULONG const AllocationTag, 
    __in KAllocator& Allocator)
{
    LogState::SPtr      result;
    Result.Reset();

    NTSTATUS    status = LogState::Create(AllocationTag, Allocator, result);
    if (!NT_SUCCESS(status))
    {
        return status;
    }
    else
    {
        ULONG           numberOfStreams = _Streams.Count();

        status = result->_StreamDescs.Reserve(numberOfStreams);
        if (!NT_SUCCESS(status))
        {
            return status;
        }
        KInvariant(result->_StreamDescs.SetCount(numberOfStreams));

        KMemCpySafe(
            &result->_LogSignature[0], 
            sizeof(result->_LogSignature), 
            &_LogSignature[0], 
            RvdLogRecordHeader::LogSignatureSize);

        result->_LogFileSize = _LogFileSize;
        result->_LogFileLsnSpace = _LogFileLsnSpace;
        result->_MaxCheckPointLogRecordSize = _MaxCheckPointLogRecordSize;
        result->_LogFileFreeSpace = _LogFileFreeSpace;
        result->_LowestLsn = _LowestLsn;
        result->_NextLsnToWrite = _NextLsnToWrite;
        result->_HighestCompletedLsn = _HighestCompletedLsn;
        result->_NumberOfStreams = numberOfStreams;

        LogStreamDescriptor*    currentSDesc = _Streams.PeekHead();
        
        for (ULONG ix = 0; ix < numberOfStreams; ix++)
        {
            KAssert(currentSDesc != nullptr);

            LogState::StreamDescriptor& dDesc = result->_StreamDescs[ix];
            LogStreamDescriptor&        sDesc = *currentSDesc;
            BOOLEAN                     isCheckpointStream = FALSE;

            dDesc._Info = sDesc.Info;
            dDesc._TruncationPoint = sDesc.TruncationPoint;
            if (sDesc.Info.LogStreamId == RvdDiskLogConstants::CheckpointStreamId())
            {
                result->_HighestCheckpointLsn = sDesc.Info.HighestLsn;
                isCheckpointStream = TRUE;
            }

            // Copy sDesc._AsnIndex contents -> dDesc._AsnIndex
            dDesc._AsnIndex = _new(AllocationTag, Allocator) RvdAsnIndex(Allocator);
            if (dDesc._AsnIndex == nullptr)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            status = dDesc._AsnIndex->Status();
            if (!NT_SUCCESS(status))
            {
                break;
            }

            if (!isCheckpointStream)
            {
                KAssert(sDesc.AsnIndex.Validate());
                for (RvdAsnIndexEntry::SPtr asnEntry = Ktl::Move(sDesc.AsnIndex.UnsafeFirst()); 
                     asnEntry != nullptr; 
                     asnEntry = Ktl::Move(sDesc.AsnIndex.UnsafeNext(asnEntry)))
                {
                    KInvariant(asnEntry->GetDisposition() == RvdLogStream::RecordDisposition::eDispositionPersisted);

                    RvdAsnIndexEntry::SPtr          resultEntry;
                    RvdAsnIndexEntry::SavedState    entryState;
                    BOOLEAN                         previousIndexEntryWasStored;
                    RvdAsnIndexEntry::SPtr          newEntry = _new(AllocationTag, Allocator) RvdAsnIndexEntry(
                        asnEntry->GetAsn(),
                        asnEntry->GetVersion(),
                        asnEntry->GetIoBufferSizeHint(),
                        asnEntry->GetDisposition(),
                        asnEntry->GetLsn());
                
                    if (newEntry == nullptr)
                    {
                        status = STATUS_INSUFFICIENT_RESOURCES;
                        break;
                    }

                    newEntry->SetLowestLsnOfHigherASNs(asnEntry->GetLowestLsnOfHigherASNs());

                    ULONGLONG dontCare;
                    status = dDesc._AsnIndex->AddOrUpdate(newEntry, resultEntry, entryState, previousIndexEntryWasStored, dontCare);
                    if (!NT_SUCCESS(status))
                    {
                        break;
                    }
                }

                if (!NT_SUCCESS(status))
                {
                    break;
                }
            }

            // Copy sDesc._LsnIndex contents -> dDesc._LsnIndex
            dDesc._LsnIndex = _new(AllocationTag, Allocator) RvdLogLsnEntryTracker(Allocator);
            if (dDesc._LsnIndex == nullptr)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            if (!isCheckpointStream)
            {
                ULONG       numberOfLsnEntries = sDesc.LsnIndex.QueryNumberOfRecords();
                for (ULONG ix1 = 0; ix1 < numberOfLsnEntries; ix1++)
                {
                    RvdLogLsn       lsn;
                    ULONG           headerAndMetadataSize;
                    ULONG           ioBufferSize;

                    status = sDesc.LsnIndex.QueryRecord(ix1, lsn, &headerAndMetadataSize, &ioBufferSize);
                    if (!NT_SUCCESS(status))
                    {
                        break;
                    }

                    status = dDesc._LsnIndex->AddHigherLsnRecord(lsn, headerAndMetadataSize, ioBufferSize);
                    if (!NT_SUCCESS(status))
                    {
                        break;
                    }
                }

                if (!NT_SUCCESS(status))
                {
                    break;
                }
            }

            currentSDesc = _Streams.Successor(currentSDesc);
        }

        KAssert(currentSDesc == nullptr);
    }

    if (!NT_SUCCESS(status))
    {
        result = nullptr;
    }

    Result = result;
    return status;
}
    
ULONGLONG
RvdLogManagerImp::RvdOnDiskLog::GetQuotaGateState()
{
    return _StreamWriteQuotaGate->GetFreeQuanta();
}

#pragma prefast(pop)
