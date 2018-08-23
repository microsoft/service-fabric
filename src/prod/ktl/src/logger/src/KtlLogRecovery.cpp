/*++

    (c) 2011 by Microsoft Corp. All Rights Reserved.

    RvdLogRecovery.cpp

    Description:
      RvdLog recovery implementation.

    History:
      PengLi/richhas        4-Sep-2011         Initial version.

--*/

#include "KtlLogRecovery.h"

#include "ktrace.h"

//*** RvdLogRecoveryBase implementation
RvdLogRecoveryBase::RvdLogRecoveryBase()
{
}

RvdLogRecoveryBase::~RvdLogRecoveryBase()
{
}

NTSTATUS
RvdLogRecoveryBase::Init()
{
    NTSTATUS status = _BlockDevice->AllocateNonContiguousReadContext(_BlockDeviceNonContiguousReadOp, KTL_TAG_LOGGER);
    if (!NT_SUCCESS(status))
    {
        AllocateTransferContextFailed(status, _BlockDevice, __LINE__);
        return status;
    }

    status = _BlockDevice->AllocateReadContext(_BlockDeviceReadOp0, KTL_TAG_LOGGER);
    if (!NT_SUCCESS(status))
    {
        AllocateTransferContextFailed(status, _BlockDevice, __LINE__);
        return status;
    }

    status = _BlockDevice->AllocateReadContext(_BlockDeviceReadOp1, KTL_TAG_LOGGER);
    if (!NT_SUCCESS(status))
    {
        AllocateTransferContextFailed(status, _BlockDevice, __LINE__);
        return status;
    }

    status = KIoBufferView::Create(_IoBufferView, GetThisAllocator(), KTL_TAG_LOGGER);
    if (!NT_SUCCESS(status))
    {
        MemoryAllocationFailed(__LINE__);
        return status;
    }

    return STATUS_SUCCESS;
}

VOID
RvdLogRecoveryBase::DoComplete(__in NTSTATUS Status)
{
    Complete(Status);
}



//*** RvdLogRecovery implementation
RvdLogRecovery::RvdLogRecovery()
    :   _StreamInfos(GetThisAllocator(), 0),
        _AllocatedRanges(GetThisAllocator())
{
    _CreationDirectory[0] = 0;
}

RvdLogRecovery::~RvdLogRecovery()
{
}

VOID
RvdLogRecovery::StartRecovery(
    __in KBlockDevice::SPtr BlockDevice,
    __in RvdLogId& LogId,
    __in DWORD CreationFlags,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _BlockDevice = Ktl::Move(BlockDevice);
    _LogId = LogId;
    _CreationFlags = CreationFlags;
    Start(ParentAsyncContext, CallbackPtr);
}

NTSTATUS
RvdLogRecovery::SnapLogState(__out LogState::SPtr& Result)
{
    LogState::SPtr result = nullptr;
    Result.Reset();

    NTSTATUS    status = LogState::Create(KTL_TAG_LOGGER, GetThisAllocator(), result);
    if (NT_SUCCESS(status))
    {
        KMemCpySafe(
            &result->_LogSignature[0], 
            LogState::LogSignatureSize,
            &_LogSignature[0], 
            RvdLogRecordHeader::LogSignatureSize);

        result->_LogFileSize = _LogFileSize;
        result->_LogFileLsnSpace = _LogFileLsnSpace;
        result->_MaxCheckPointLogRecordSize = _MaxCheckPointLogRecordSize;
        result->_LogFileFreeSpace = _LogFileFreeSpace;
        result->_LowestLsn = _LowestLsn;
        result->_NextLsnToWrite = _NextLsnToWrite;
        result->_HighestCompletedLsn = _HighestCompletedLsn;
        result->_NumberOfStreams = _NumberOfStreams;
        result->_Config = _Config;
        KMemCpySafe(
            &result->_CreationDirectory[0], 
            sizeof(result->_CreationDirectory), 
            &_CreationDirectory[0], 
            RvdLogMasterBlock::CreationDirectoryMaxSize);

        KMemCpySafe(
            &result->_LogType[0], 
            sizeof(result->_LogType), 
            &_LogType[0], 
            RvdLogManager::AsyncCreateLog::MaxLogTypeLength*sizeof(WCHAR));

        status = result->_StreamDescs.Reserve(_Config.GetMaxNumberOfStreams());
        if (! NT_SUCCESS(status))
        {
            return status;
        }
        
        KInvariant(result->_StreamDescs.SetCount(_Config.GetMaxNumberOfStreams()));

        if (NT_SUCCESS(status))
        {
            for (ULONG ix = 0; ix < _NumberOfStreams; ix++)
            {
                LogState::StreamDescriptor& dDesc = result->_StreamDescs[ix];

                dDesc._Info = _StreamInfos[ix];
                dDesc._TruncationPoint = RvdLogAsn::Null();
                dDesc._AsnIndex = nullptr;
                dDesc._LsnIndex = nullptr;

                if (dDesc._Info.LogStreamId == RvdDiskLogConstants::CheckpointStreamId())
                {
                    result->_HighestCheckpointLsn = dDesc._Info.HighestLsn;
                }
            }
        }
    }

    Result = result;
    return status;
}

VOID
RvdLogRecovery::OnStart()
{
    NTSTATUS status = Init();
    if (!NT_SUCCESS(status))
    {
        DoComplete(status);
        return;
    }

    // Capture/compute log file size and usable lsn space
    _BlockDevice->QueryDeviceAttributes(_DevAttrs);

    _LogFileSize = _DevAttrs.DeviceSize;
    _LogFileLsnSpace = _LogFileSize - (2 * RvdDiskLogConstants::MasterBlockSize);

    //*** Kickstart the structure validation/recovery/scan process
    status = _BlockDevice->Read(
        0,
        RvdDiskLogConstants::MasterBlockSize,
        TRUE,                   // contiguous buffer
        _IoBuffer0,
        KAsyncContextBase::CompletionCallback(this, &RvdLogRecovery::FirstMasterBlockReadComplete),
        nullptr,
        _BlockDeviceReadOp0);

    if (!K_ASYNC_SUCCESS(status))
    {
        PhysicalStartReadFailed(status, _BlockDevice, 0, RvdDiskLogConstants::MasterBlockSize, __LINE__);
        DoComplete(status);
        return;
    }
}

VOID
RvdLogRecovery::FirstMasterBlockReadComplete(__in_opt KAsyncContextBase* const Parent, __in KAsyncContextBase& CompletedContext)
{
    UNREFERENCED_PARAMETER(Parent);
    
    NTSTATUS status = CompletedContext.Status();
    if (!NT_SUCCESS(status))
    {
        PhysicalReadFailed(status, _BlockDevice, 0, RvdDiskLogConstants::MasterBlockSize, __LINE__);
        DoComplete(status);
        return;
    }

    RvdLogMasterBlock* masterBlk = (RvdLogMasterBlock*)_IoBuffer0->First()->GetBuffer();
    status = masterBlk->Validate(_LogId, _LogFileSize, 0);
    if (!NT_SUCCESS(status))
    {
        _FirstMasterBlockIsValid = FALSE;
        FirstMasterBlockValidationFailed(status, _BlockDevice, *masterBlk);
    }
    else
    {
        _FirstMasterBlockIsValid = TRUE;
        ReportFirstMasterBlockCorrect(*masterBlk);

        // Save signature off to be used in further validation
        KMemCpySafe(&_LogSignature[0], sizeof(_LogSignature), &masterBlk->LogSignature[0], RvdLogRecordHeader::LogSignatureSize);
        _Config.Set(masterBlk->GeometryConfig);

        if (! _Config.IsValidConfig(_LogFileSize))
        {
            KTraceFailedAsyncRequest(K_STATUS_LOG_STRUCTURE_FAULT, this, _LogFileSize, 0);          
        }
        
        KMemCpySafe(
            &_CreationDirectory[0], 
            sizeof(_CreationDirectory), 
            &masterBlk->CreationDirectory[0], 
            RvdLogMasterBlock::CreationDirectoryMaxSize);

        KMemCpySafe(&_LogType[0], sizeof(_LogType), &masterBlk->LogType[0], RvdLogManager::AsyncCreateLog::MaxLogTypeLength*sizeof(WCHAR));
    }

    // Read and validate trailing master block
    status = _BlockDevice->Read(
        _LogFileSize - RvdDiskLogConstants::MasterBlockSize,
        RvdDiskLogConstants::MasterBlockSize,
        TRUE,                   // contiguous buffer
        _IoBuffer0,
        KAsyncContextBase::CompletionCallback(this, &RvdLogRecovery::SecondMasterBlockReadComplete),
        nullptr,
        _BlockDeviceReadOp0);

    if (!K_ASYNC_SUCCESS(status))
    {
        PhysicalStartReadFailed(
            status,
            _BlockDevice,
            _LogFileSize - RvdDiskLogConstants::MasterBlockSize,
            RvdDiskLogConstants::MasterBlockSize,
            __LINE__);

        DoComplete(status);
        return;
    }
}

VOID
RvdLogRecovery::SecondMasterBlockReadComplete(__in_opt KAsyncContextBase* const Parent, __in KAsyncContextBase& CompletedContext)
{
    UNREFERENCED_PARAMETER(Parent);

    NTSTATUS status = CompletedContext.Status();
    if (!NT_SUCCESS(status))
    {
        PhysicalReadFailed(
            status,
            _BlockDevice,
            _LogFileSize - RvdDiskLogConstants::MasterBlockSize,
            RvdDiskLogConstants::MasterBlockSize,
            __LINE__);

        DoComplete(status);
        return;
    }

    RvdLogMasterBlock* masterBlk = (RvdLogMasterBlock*)_IoBuffer0->First()->GetBuffer();
    if (_FirstMasterBlockIsValid)
    {
        // NOTE: validates aginst the first master block signature
        status = masterBlk->Validate(
            _LogId,
            _LogFileSize,
            _LogFileSize - RvdDiskLogConstants::MasterBlockSize,
            &_LogSignature);
    }
    else
    {
        status = masterBlk->Validate(
            _LogId,
            _LogFileSize,
            _LogFileSize - RvdDiskLogConstants::MasterBlockSize);
    }

    // BUG, richhas, xxxxx, consider re-writing a bad Master Block iif one is good and the other is not.
    if (!NT_SUCCESS(status))
    {
        SecondMasterBlockValidationFailed(status, _BlockDevice, *masterBlk);
        if (!_FirstMasterBlockIsValid)
        {
            // Neither master block was valid
            DoComplete(status);
            return;
        }

        // Assume the first master block is correct
    }
    else
    {
        ReportSecondMasterBlockCorrect(*masterBlk);

        if (!_FirstMasterBlockIsValid)
        {
            // The first master block was corrupt but the second master block is ok - assume it's correct
            KMemCpySafe(
                &_LogSignature[0],
                sizeof(_LogSignature),
                &masterBlk->LogSignature[0],
                RvdLogRecordHeader::LogSignatureSize);

            _Config.Set(masterBlk->GeometryConfig);
            KMemCpySafe(
                &_CreationDirectory[0],
                sizeof(_CreationDirectory),
                &masterBlk->CreationDirectory[0],
                RvdLogMasterBlock::CreationDirectoryMaxSize);
        }
    }

    if (! _Config.IsValidConfig(_LogFileSize))
    {
        KTraceFailedAsyncRequest(K_STATUS_LOG_STRUCTURE_FAULT, this, _LogFileSize, 0);          
        DoComplete(K_STATUS_LOG_STRUCTURE_FAULT);
        return;
    }
    
    if ((_LogFileLsnSpace % _Config.GetMaxQueuedWriteDepth()) != 0)
    {
        AllocatedFileLengthWrong(_LogFileLsnSpace, _Config.GetMaxQueuedWriteDepth());
        DoComplete(K_STATUS_LOG_STRUCTURE_FAULT);
        return;
    }

    if (_DevAttrs.DeviceSize < _Config.GetMinFileSize())
    {
        AllocatedFileTooSmall(_DevAttrs.DeviceSize, _Config.GetMinFileSize());
        DoComplete(K_STATUS_LOG_STRUCTURE_FAULT);
        return;
    }

    masterBlk = nullptr;

    ULONGLONG logFileNumberOfChuncks = _LogFileLsnSpace / _Config.GetMaxQueuedWriteDepth();

    _IsSparseFile = ((_CreationFlags & RvdLogManager::AsyncCreateLog::FlagSparseFile) == RvdLogManager::AsyncCreateLog::FlagSparseFile) ? TRUE : FALSE;
    if (_IsSparseFile)
    {
        //
        // Configure KBlockFile to use sparse files
        //
        status = _BlockDevice->SetSparseFile(TRUE);
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, (ULONGLONG)_BlockDevice.RawPtr(), 0);
            DoComplete(status);
            return;         
        }

        //
        // If this is a sparse file then we need to do extra processing to determine the correct chunks
        // that contain the actual data. There are a number of states which the sparse log could be in
        //
        // Case 1: Recover a log with data only in the front
        //            [XXXXXXX        ]
        // Case 2: Recover a log with data only in the middle
        //            [    XXXXXX     ]
        // Case 3: Recover a log with data only in the end
        //            [         XXXXXX]
        // Case 4: Recover a log with data that wraps
        //            [XXXX     XXXXXX]
        // Case 5: Recover a log completely full
        //            [XXXXXXXXXXXXXXX]
        //
        // The steps are as follows:
        //
        // 1. Read the first chunck and the last chunck. If either are not full of all zero then it is
        //    case 1, 3, 4 or 5. These cases recover successfully when starting with LowChunk = first chunk (0)
        //    and MaxChunk = last chunk
        //    
        // 2. If the first and last chunks are all zero then it is case 2 and the code must determine the first and
        //    last chunks that contain valid data. This is done by querying the allocations for the region of the file
        //    that contains log data (ie, excludes master blocks) and then scanning the lowest allocations up until a region
        //    of non-zero is hit. This is the low chunk. Next the highest allocations are scanned down until a region of 
        //    non-zero is hit. This is the high chunk.
        //
        // 3. Proceed with the normal recovery using the newly discovered low and high chunks.
        //

        //
        // Read the last chunk of log space
        //
        ULONGLONG lastChunckFileOffset = ToFileOffsetFromChunk(logFileNumberOfChuncks-1);

        _BlockDeviceReadOp0->Reuse();
        status = _BlockDevice->Read(
            lastChunckFileOffset,
            _Config.GetMaxQueuedWriteDepth(),
            TRUE,                   // contiguous buffer
            _IoBuffer0,
            KAsyncContextBase::CompletionCallback(this, &RvdLogRecovery::LastChunckReadComplete),
            nullptr,
            _BlockDeviceReadOp0);

        if (!K_ASYNC_SUCCESS(status))
        {
            PhysicalStartReadFailed(
                status,
                _BlockDevice,
                lastChunckFileOffset,
                _Config.GetMaxQueuedWriteDepth(),
                __LINE__);

            DoComplete(status);
            return;
        }
        // continued at LastChunckReadComplete()
    }
    else 
    {
        DetermineChuncksComplete(0, logFileNumberOfChuncks);
    }
}

BOOLEAN RvdLogRecovery::HasValidRecordHeader(
    __in ULONGLONG ChunckNumber,
    __in KIoBuffer& Chunck
    )
{
    ULONG                   currentChunkOffset = 0;
    RvdLogRecordHeader*     currentChunkRecord = (RvdLogRecordHeader*)Chunck.First()->GetBuffer();
    ULONG                   chunkSizeRead = Chunck.QuerySize();
    ULONGLONG               currentChunkFileOffset = 0;

    while (currentChunkOffset < chunkSizeRead)
    {
        currentChunkFileOffset = ToFileOffsetFromChunk(ChunckNumber) + currentChunkOffset;

        if (RecordHeaderAppearsValid(currentChunkRecord, currentChunkFileOffset))
        {
            return(TRUE);
        }
        currentChunkOffset += RvdDiskLogConstants::BlockSize;
        currentChunkRecord = (RvdLogRecordHeader*)(((UCHAR*)currentChunkRecord) + RvdDiskLogConstants::BlockSize);
    }

    return(FALSE);
}

VOID
RvdLogRecovery::LastChunckReadComplete(__in_opt KAsyncContextBase* const Parent, __in KAsyncContextBase& CompletedContext)
{
    UNREFERENCED_PARAMETER(Parent);

    ULONGLONG lastChunckNumber = _LogFileLsnSpace / _Config.GetMaxQueuedWriteDepth();

    NTSTATUS status = CompletedContext.Status();
    if (!NT_SUCCESS(status))
    {
        PhysicalReadFailed(
            status,
            _BlockDevice,
            ToFileOffsetFromChunk(lastChunckNumber),
            _Config.GetMaxQueuedWriteDepth(),
            __LINE__);

        DoComplete(status);
        return;
    }

    BOOLEAN hasValidRecordHeader;
    hasValidRecordHeader = HasValidRecordHeader(lastChunckNumber, *_IoBuffer0);

    if (! hasValidRecordHeader)
    {
        //
        // Read the first chunk of log space
        //
        ULONGLONG firstChunckFileOffset = ToFileOffsetFromChunk(0);

        _BlockDeviceReadOp0->Reuse();
        status = _BlockDevice->Read(
            firstChunckFileOffset,
            _Config.GetMaxQueuedWriteDepth(),
            TRUE,                   // contiguous buffer
            _IoBuffer0,
            KAsyncContextBase::CompletionCallback(this, &RvdLogRecovery::FirstChunckReadComplete),
            nullptr,
            _BlockDeviceReadOp0);

        if (!K_ASYNC_SUCCESS(status))
        {
            PhysicalStartReadFailed(
                status,
                _BlockDevice,
                firstChunckFileOffset,
                _Config.GetMaxQueuedWriteDepth(),
                __LINE__);

            DoComplete(status);
            return;
        }
        // continued at FirstChunckReadComplete()
    }
    else
    {
        //
        // Since this is not all zeros then we can move on with recovery
        //
        DetermineChuncksComplete(0, _LogFileLsnSpace / _Config.GetMaxQueuedWriteDepth());
    }
}

VOID
RvdLogRecovery::FirstChunckReadComplete(__in_opt KAsyncContextBase* const Parent, __in KAsyncContextBase& CompletedContext)
{
    UNREFERENCED_PARAMETER(Parent);

    NTSTATUS status = CompletedContext.Status();
    if (!NT_SUCCESS(status))
    {
        PhysicalReadFailed(
            status,
            _BlockDevice,
            ToFileOffsetFromChunk(0),
            _Config.GetMaxQueuedWriteDepth(),
            __LINE__);

        DoComplete(status);
        return;
    }

    BOOLEAN hasValidRecordHeader;
    hasValidRecordHeader = HasValidRecordHeader(0, *_IoBuffer0);

    if (! hasValidRecordHeader)
    {
        //
        // Both first and last chunks are zero, we need to find the range where data
        // is allocated.
        //
        ULONGLONG firstChunckOffset = ToFileOffsetFromChunk(0);

        status = _BlockDevice->QueryAllocations(firstChunckOffset,
                                        _LogFileLsnSpace,
                                        _AllocatedRanges,
                                        KAsyncContextBase::CompletionCallback(this, &RvdLogRecovery::GetAllocatedRangesComplete),
                                        nullptr,
                                        nullptr);
        if (!K_ASYNC_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, firstChunckOffset, _LogFileLsnSpace);

            DoComplete(status);
            return;
        }

        // continued at GetAllocatedRangesComplete
    }
    else
    {
        //
        // Since this is not all zeros then we can move on with recovery
        //
        DetermineChuncksComplete(0, _LogFileLsnSpace / _Config.GetMaxQueuedWriteDepth());
    }
}

NTSTATUS
RvdLogRecovery::StartNextChunckRead(
)
{
    NTSTATUS status;
    ULONGLONG readOffset;

    readOffset = (_AllocatedRanges[_AllocatedRangeIndex].Offset + (ULONGLONG)_ChunckIndexInRange * (ULONGLONG)_Config.GetMaxQueuedWriteDepth());

    if (_DetermineChunckState == DetermineLowChunck)
    {
        //
        // Move forward in the allocations
        //
        ULONGLONG topOffset = _AllocatedRanges[_AllocatedRangeIndex].Offset + _AllocatedRanges[_AllocatedRangeIndex].Length;

        if (readOffset >= topOffset)
        {
            //
            // Move to the next allocation
            //
            _AllocatedRangeIndex++;
            _ChunckIndexInRange = 0;
            if (_AllocatedRangeIndex >= _AllocatedRanges.Count())
            {
                //
                // All allocation processed
                //
                return(STATUS_NO_MORE_ENTRIES);
            }

            topOffset = _AllocatedRanges[_AllocatedRangeIndex].Offset + _AllocatedRanges[_AllocatedRangeIndex].Length;
            readOffset = _AllocatedRanges[_AllocatedRangeIndex].Offset + _ChunckIndexInRange * _Config.GetMaxQueuedWriteDepth();
            KInvariant(readOffset < topOffset);
        }
        else
        {
            //
            // Move to the next chunck in the allocation
            //
            _ChunckIndexInRange++;
        }
    }
    else 
    {
        //
        // Move backward in the allocations
        //
        KInvariant(_DetermineChunckState == DetermineHighChunck);

        if (_AllocatedRangeIndex == -1)
        {
            //
            // All allocation processed
            //
            return(STATUS_NO_MORE_ENTRIES);
        }

        ULONGLONG bottomOffset;
        bottomOffset = _AllocatedRanges[_AllocatedRangeIndex].Offset;

        if (readOffset == bottomOffset)
        {
            KInvariant(_ChunckIndexInRange == 0);

            //
            // Move to the previous allocation for the next read
            //
            _AllocatedRangeIndex--;
            if (_AllocatedRangeIndex != -1)
            {
                ULONG chuncksInAllocation = (ULONG)(_AllocatedRanges[_AllocatedRangeIndex].Length / _Config.GetMaxQueuedWriteDepth());
                KInvariant(chuncksInAllocation >= 1);
                _ChunckIndexInRange = chuncksInAllocation - 1;
            }
        }
        else 
        {
            //
            // Move to the previous chunck
            //
            KInvariant(_ChunckIndexInRange > 0);
            _ChunckIndexInRange--;
        }
    }

    _ReadLength = _Config.GetMaxQueuedWriteDepth();
    _CurrentChunckIndex = (ULONG)(readOffset / _Config.GetMaxQueuedWriteDepth());

    _BlockDeviceReadOp0->Reuse();
    status = _BlockDevice->Read(
        readOffset + RvdDiskLogConstants::BlockSize,
        _ReadLength,
        TRUE,                   // contiguous buffer
        _IoBuffer0,
        KAsyncContextBase::CompletionCallback(this, &RvdLogRecovery::ChunckReadComplete),
        nullptr,
        _BlockDeviceReadOp0);

    if (!K_ASYNC_SUCCESS(status))
    {
        KInvariant(status != STATUS_NO_MORE_ENTRIES);

        PhysicalStartReadFailed(
            status,
            _BlockDevice,
            _AllocatedRanges[_AllocatedRangeIndex].Offset,
            _Config.GetMaxQueuedWriteDepth(),
            __LINE__);

        DoComplete(status);
        return(status);
    }

    return(status);
}

VOID
RvdLogRecovery::GetAllocatedRangesComplete(__in_opt KAsyncContextBase* const Parent, __in KAsyncContextBase& CompletedContext)
{
    UNREFERENCED_PARAMETER(Parent);

    NTSTATUS status = CompletedContext.Status();
    if (!NT_SUCCESS(status))
    {
        // TODO: Define a new failure report routine
        PhysicalReadFailed(
            status,
            _BlockDevice,
            _LogFileLsnSpace,
            _Config.GetMaxQueuedWriteDepth(),
            __LINE__);

        DoComplete(status);
        return;
    }

    if (_AllocatedRanges.Count() == 0)
    {
        //
        // If there are no allocations at all, this is a special case and we let the normal
        // recovery proceed.
        //
        DetermineChuncksComplete(0, _LogFileLsnSpace / _Config.GetMaxQueuedWriteDepth());
        return;
    }

    //
    // Normalize each allocation so that it is the offset from within the log portion of the 
    // file. It will be fixed up when read actually happens. This is to make it easier for reading
    // to start on a chunck boundary and has the proper length
    //
    for (ULONG i = 0; i < _AllocatedRanges.Count(); i++)
    {
        ULONGLONG baseOffset = _AllocatedRanges[i].Offset - RvdDiskLogConstants::BlockSize;
        ULONGLONG baseLength = _AllocatedRanges[i].Length;
        ULONGLONG baseChunckOffset;

        baseChunckOffset = (baseOffset / _Config.GetMaxQueuedWriteDepth()) * _Config.GetMaxQueuedWriteDepth();

        _AllocatedRanges[i].Offset = baseChunckOffset;
        baseLength += (baseOffset - baseChunckOffset);
        baseLength = ((baseLength + _Config.GetMaxQueuedWriteDepth() - 1) / _Config.GetMaxQueuedWriteDepth()) * _Config.GetMaxQueuedWriteDepth();

        _AllocatedRanges[i].Length = baseLength;
    }

    //
    // First step is to determine the low chunk
    //
    _DetermineChunckState = DetermineLowChunck;
    _AllocatedRangeIndex = 0;
    _ChunckIndexInRange = 0;
    _LowChunkIndex = (ULONG)-1;

    status = StartNextChunckRead();
    if (status == STATUS_NO_MORE_ENTRIES)
    {
        //
        // There are no more allocations - hand off to normal recovery
        //
        DetermineChuncksComplete(0, _LogFileLsnSpace / _Config.GetMaxQueuedWriteDepth());
        return;
    }

    if (!K_ASYNC_SUCCESS(status))
    {
        //
        // Read failed, just leave quietly as the async has been completed
        //
        return;
    }

    // continued at ChunckReadComplete()
}

VOID
RvdLogRecovery::ChunckReadComplete(__in_opt KAsyncContextBase* const Parent, __in KAsyncContextBase& CompletedContext)
{
    UNREFERENCED_PARAMETER(Parent);

    NTSTATUS status = CompletedContext.Status();
    if (!NT_SUCCESS(status))
    {
        PhysicalReadFailed(
            status,
            _BlockDevice,
            _CurrentChunckIndex,
            _Config.GetMaxQueuedWriteDepth(),
            __LINE__);

        DoComplete(status);
        return;
    }

    BOOLEAN hasValidRecordHeader = HasValidRecordHeader(_CurrentChunckIndex, *_IoBuffer0);

    //KDbgCheckpointWData(0, "Recovery Read ok", STATUS_SUCCESS, isAllZero ? 0 : 1, _ReadLength, _AllocatedRangeIndex, _ChunckIndexInRange);

    if (hasValidRecordHeader)
    {
        if (_DetermineChunckState == DetermineLowChunck)
        {
            //
            // We found the low chunk with non-zeros - so now we know our low chunck
            //
            _LowChunkIndex = _CurrentChunckIndex;

            //
            // Now start looking at the high chunk from the end of the file to the beginning
            //
            _DetermineChunckState = DetermineHighChunck;
            _HighChunkIndex = (ULONG)-1;

            _AllocatedRangeIndex = _AllocatedRanges.Count() - 1;

            ULONG chuncksInAllocation = (ULONG)(_AllocatedRanges[_AllocatedRangeIndex].Length / _Config.GetMaxQueuedWriteDepth());
            KInvariant(chuncksInAllocation >= 1);
            _ChunckIndexInRange = chuncksInAllocation - 1;

        }
        else
        {
            KInvariant(_DetermineChunckState == DetermineHighChunck);

            //
            // We found the high chunk with non-zeros - so now we know our high chunck
            //
            _HighChunkIndex = _CurrentChunckIndex;

            KDbgCheckpointWData(0, "Recovery HighAndLow", STATUS_SUCCESS, (ULONGLONG)this, _LowChunkIndex, _HighChunkIndex, 0);
            DetermineChuncksComplete(_LowChunkIndex, _HighChunkIndex+1);
            return;
        }
    }

    //
    // Move forward to the next chunk
    //
    status = StartNextChunckRead();
    if (status == STATUS_NO_MORE_ENTRIES)
    {
        //
        // There are no more allocations - hand off to normal recovery
        //
        DetermineChuncksComplete(0, _LogFileLsnSpace / _Config.GetMaxQueuedWriteDepth());
        return;
    }

    if (!K_ASYNC_SUCCESS(status))
    {
        //
        // Read failed, just leave quietly as the async has been completed
        //
        return;
    }

    // continued at ChunckReadComplete()

}

VOID
RvdLogRecovery::DetermineChuncksComplete(
    __in ULONGLONG LowSearchChunckNumber,
    __in ULONGLONG LogFileNumberOfChuncks
    )
{
    NTSTATUS status;

    _LogFileNumberOfChuncks = LogFileNumberOfChuncks;
    _MaxCheckPointLogRecordSize = RvdLogPhysicalCheckpointRecord::ComputeMaxSizeOnDisk(_Config);
    _LogFileFreeSpace = _LogFileLsnSpace;

    // Allocate steam info array
    _StreamInfos.Reserve(_Config.GetMaxNumberOfStreams());
    if (!NT_SUCCESS(_StreamInfos.Status()))
    {
        MemoryAllocationFailed(__LINE__);
        DoComplete(_StreamInfos.Status());
        return;
    }
    KInvariant(_StreamInfos.SetCount(_Config.GetMaxNumberOfStreams()));

    ReportFileSizeLayout(
        *_BlockDevice,
        _LogFileSize,
        _Config.GetMaxQueuedWriteDepth(),
        _LogFileFreeSpace,
        _MaxCheckPointLogRecordSize);

    status = RecordReader::Create(
        _BlockDevice,
        _Config,
        _LogFileLsnSpace,
        _LogId,
        _LogSignature,
        _RecordReader,
        KTL_TAG_LOGGER,
        GetThisAllocator());

    if (!NT_SUCCESS(status))
    {
        LogRecordReaderCreationFailed(status, __LINE__);
        DoComplete(status);
        return;
    }

    _ValidationProcessStatus = STATUS_SUCCESS;
    _LowSearchChunkNumber = LowSearchChunckNumber;
    _SearchSpaceStep = 1;
    _MaxNumberOfSearchChunks = _LogFileNumberOfChuncks;
    _HighestLsn = RvdLogLsn::Null();
    _HighestLsnChunkNumber = 0;

    //** Doing high speed recovery / validation
    // Start a binary search of the entire _LogFileLsnSpace for the highest LSN (record) recorded.
    // The _LogFileLsnSpace is broken into _LogFileNumberOfChuncks chunks that are each
    // RvdDiskLogConstants::MaxQueuedWriteDepth bytes in length. It is this set of chunks over
    // which the binary search is performed.
    ReadLowChunck();
}

VOID
RvdLogRecovery::ReadLowChunck()
//
// Entry:
//      _LowSearchChunkNumber - Chunk in file to read
//
// Completion:
//      _IoBuffer - contains the data read from the chunk @_LowSearchChunkNumber
{
    // Read @ _LowSearchChunkNumber a buffer of RvdDiskLogConstants::MaxQueuedWriteDepth (one chunck)
    ULONGLONG   fileOffset = ToFileOffsetFromChunk(_LowSearchChunkNumber);

    ULONG bufferLength = _Config.GetMaxQueuedWriteDepth() / _MaxNonContiguousReadBuffers;
    if (bufferLength < _MinNonContiguousReadBufferLength)
    {
        bufferLength = _MinNonContiguousReadBufferLength;
    }

    KDbgCheckpointWData(0, "ReadNonContiguous", STATUS_SUCCESS, 
        fileOffset, _Config.GetMaxQueuedWriteDepth(), bufferLength, 0); 
    
    NTSTATUS status = _BlockDevice->ReadNonContiguous(
        fileOffset,
        _Config.GetMaxQueuedWriteDepth(),
        bufferLength,
        _IoBuffer0,
        KAsyncContextBase::CompletionCallback(this, &RvdLogRecovery::ReadLowChunckComplete),
        nullptr,
        _BlockDeviceNonContiguousReadOp,
        nullptr);                              // CONSIDER: Prealloc

    if (!K_ASYNC_SUCCESS(status))
    {
        PhysicalStartReadFailed(
            status,
            _BlockDevice,
            fileOffset,
            _Config.GetMaxQueuedWriteDepth(),
            __LINE__);

        DoComplete(status);
        return;
    }

    // Continued @ ReadLowChunckComplete()
}


VOID
RvdLogRecovery::ReadLowChunckComplete(__in_opt KAsyncContextBase* const Parent, __in KAsyncContextBase& CompletedContext)
// Continued from ReadLowChunck()
{
    UNREFERENCED_PARAMETER(Parent);
    
    KDbgCheckpointWData(0, "ReadNonContiguous Complete", STATUS_SUCCESS, 
        ToFileOffsetFromChunk(_LowSearchChunkNumber), _Config.GetMaxQueuedWriteDepth(), 0, 0);
    
    NTSTATUS status = CompletedContext.Status();
    if (!NT_SUCCESS(status))
    {
        PhysicalReadFailed(
            status,
            _BlockDevice,
            ToFileOffsetFromChunk(_LowSearchChunkNumber),
            _Config.GetMaxQueuedWriteDepth(),
            __LINE__);

        DoComplete(status);
        return;
    }

    KInvariant(_IoBuffer0->QuerySize() == _Config.GetMaxQueuedWriteDepth());
    SearchLowChunckRead(FALSE);
}

VOID
RvdLogRecovery::SplitRecordReadComplete(__in_opt KAsyncContextBase* const Parent, __in KAsyncContextBase& CompletedContext)
// Continued from SearchLowChunckRead() when the last record in the current chunk is split across
// that chunk boundary
{
    UNREFERENCED_PARAMETER(Parent);
    
    NTSTATUS status = CompletedContext.Status();

    InterlockedCompareExchange(&_ValidationProcessStatus, status, STATUS_SUCCESS);
    KAssert(_ReadOpCount > 0);
    if (InterlockedDecrement(&_ReadOpCount) != 0)
    {
        return;
    }

    if (!NT_SUCCESS(_ValidationProcessStatus))
    {
        SplitReadFailed(_ValidationProcessStatus);
        DoComplete(_ValidationProcessStatus);
        return;
    }

    KInvariant(_IoBuffer0->QueryNumberOfIoBufferElements() == 1);
    union
    {
        RvdLogRecordHeader*     currentRecord;
        UCHAR*                  rawBufferPtr;
    };

    currentRecord = (RvdLogRecordHeader*)_SplitReadBuffer->GetBuffer();

    BOOLEAN                 readWasWrapped = (_IoBuffer1 != nullptr);
    ULONG                   additionalSizeRead = readWasWrapped ? (_IoBuffer0->QuerySize() + _IoBuffer1->QuerySize())
                                                                : _IoBuffer0->QuerySize();
    KAssert(additionalSizeRead < currentRecord->ThisHeaderSize);
    ULONG                   ioBuffer0Offset = currentRecord->ThisHeaderSize - additionalSizeRead;

    KMemCpySafe(
        rawBufferPtr + ioBuffer0Offset, 
        _SplitReadBuffer->QuerySize() - ioBuffer0Offset, 
        _IoBuffer0->First()->GetBuffer(), 
        _IoBuffer0->QuerySize());

    if (readWasWrapped)
    {
        KInvariant(_IoBuffer1->QueryNumberOfIoBufferElements() == 1);
        KMemCpySafe(
            rawBufferPtr + ioBuffer0Offset + _IoBuffer0->QuerySize(),
            _SplitReadBuffer->QuerySize() - (ioBuffer0Offset + _IoBuffer0->QuerySize()),
            _IoBuffer1->First()->GetBuffer(),
            _IoBuffer1->QuerySize());

        _IoBuffer1.Reset();
    }

    _IoBuffer0->Clear();
    _IoBuffer0->AddIoBufferElement(*_SplitReadBuffer.RawPtr());
    _SplitReadBuffer.Reset();

    SearchLowChunckRead(TRUE);
}

// TODO: Get rid of the extra KDbgCheckpointWData

NTSTATUS RvdLogRecovery::GetCurrentChunkRecordAndMetadata(
    __in KIoBuffer& ChunkIoBuffer,
    __in KIoBufferElement& ChunkIoBufferElementHint,
    __in ULONG ChunkIoBufferPositionHint,
    __in ULONG ChunkOffset,
    __inout RvdLogRecordHeader*& RecordHeaderCopy,
    __out KBuffer::SPtr& ChunkRecordHeaderAndMetadataBuffer,
    __out ULONG& ChunkRecordHeaderAndMetadataBufferSize
    )
{
    //
    // Since the ChunkIoBuffer is not always contiguous, this routine
    // will copy the record header plus the following metadata if not
    // contiguous or if contiguous, return a pointer into the KIoBuffer
    //
    NTSTATUS status;
    BOOLEAN b;
    ULONG sizeNeeded;
    ULONG sizeLeftInIoBuffer;
    RvdLogRecordHeader* recordHeader;
    KIoBufferElement::SPtr chunkIoBufferElementHint = &ChunkIoBufferElementHint;

    recordHeader = RecordHeaderCopy;
    ChunkRecordHeaderAndMetadataBuffer = nullptr;
        
    sizeNeeded = recordHeader->ThisHeaderSize;
    sizeLeftInIoBuffer = ChunkIoBuffer.QuerySize() - ChunkOffset;
    if (sizeNeeded > sizeLeftInIoBuffer)
    {
        //
        // If at the end of the IoBuffer then return only what is available
        //
        sizeNeeded = sizeLeftInIoBuffer;
    }

    ChunkRecordHeaderAndMetadataBufferSize = sizeNeeded;
    RecordHeaderCopy = (RvdLogRecordHeader*)ChunkIoBuffer.GetContiguousPointer(chunkIoBufferElementHint,
                                                                               ChunkIoBufferPositionHint,
                                                                               ChunkOffset, sizeNeeded);
    if (RecordHeaderCopy == nullptr)
    {           
        status = KBuffer::Create(sizeNeeded, ChunkRecordHeaderAndMetadataBuffer, GetThisAllocator());
        if (! NT_SUCCESS(status))
        {
            return(status);
        }
        RecordHeaderCopy = (RvdLogRecordHeader*)ChunkRecordHeaderAndMetadataBuffer->GetBuffer();
        b = KIoBufferStream::CopyFrom(ChunkIoBuffer, ChunkOffset, sizeNeeded, RecordHeaderCopy);
        KInvariant(b);
    }
    
    return(STATUS_SUCCESS);
}

RvdLogRecordHeader* RvdLogRecovery::GetCurrentChunkRecordHeader(
    __in KIoBuffer& ChunkIoBuffer,
    __in ULONG ChunkOffset,
    __in ULONG SkipOffset,
    __inout KIoBufferElement::SPtr& ChunkIoBufferElementHint,
    __inout ULONG& ChunkIoBufferPositionHint,
    __inout ULONG& ChunkIoBufferSize,
    __inout ULONG& InElementOffset,
    __inout PUCHAR& InElementPointer
    )
{
    if ((InElementOffset + SkipOffset) >= ChunkIoBufferSize)
    {       
        InElementPointer = (PUCHAR)ChunkIoBuffer.GetContiguousPointer(ChunkIoBufferElementHint,
                                                                      ChunkIoBufferPositionHint,
                                                                      ChunkOffset, sizeof(RvdLogRecordHeader));
        InElementOffset = (ULONG)((ULONG_PTR)InElementPointer - (ULONG_PTR)ChunkIoBufferElementHint->GetBuffer());
        ChunkIoBufferSize = ChunkIoBufferElementHint->QuerySize();
    } else {
        //
        // The next record is in the current buffer
        //
        InElementOffset += SkipOffset;
        InElementPointer = InElementPointer + SkipOffset;
    }
        
    return((RvdLogRecordHeader*)InElementPointer);
    
}

VOID
RvdLogRecovery::SearchLowChunckRead(__in BOOLEAN IsContinuedSplitRead)
// Continued from ReadLowChunck or SplitRecordReadComplete()
{
    //** Scan current chunk just read for its highest Lsn record
    RvdLogRecordHeader*     currentChunkRecord;
    KBuffer::SPtr           currentRecordHeaderBuffer;
    ULONG                   currentRecordHeaderBufferSize;
   
    BOOLEAN                 foundValidRecord = FALSE;
    BOOLEAN                 foundHole = FALSE;
    ULONG                   chunkSizeRead = _IoBuffer0->QuerySize();
    NTSTATUS                status;
    ULONGLONG               currentChunkFileOffset = 0;

    ULONG                   splitReadOffset = 0;
    ULONG                   currentChunkOffset = 0;
    KIoBufferElement::SPtr  currentChunkIoBufferElementHint = _IoBuffer0->First();
    ULONG                   currentChunkIoBufferPositionHint = 0;
    ULONG                   chunkIoBufferSize = currentChunkIoBufferElementHint->QuerySize();
    ULONG                   inElementOffset = 0;
    PUCHAR                  inElementPointer = (PUCHAR)currentChunkIoBufferElementHint->GetBuffer();
    currentChunkRecord = GetCurrentChunkRecordHeader(*_IoBuffer0, currentChunkOffset, 0,
                                                     currentChunkIoBufferElementHint,
                                                     currentChunkIoBufferPositionHint,
                                                     chunkIoBufferSize,
                                                     inElementOffset,
                                                     inElementPointer
                                                     );

    if (IsContinuedSplitRead)
    {
        // Restore state from split read started below - see #1
        currentChunkOffset = _SavedCurrentChunkOffset;
        foundValidRecord = _SavedFoundValidRecord;
        foundHole = _SavedFoundHole;
        KAssert(currentChunkRecord->ThisHeaderSize == chunkSizeRead);

        // Treat as if chunkSizeRead describes the chunk that included the
        // start of the last record plus the split section of that record that
        // fell into the following chunk. Note the offset of the beginning of the 
        // split read so GetCurrentChunkRecordHeaderAndMetadata can index into the IoBuffer
        // correctly.
        //
        chunkSizeRead += currentChunkOffset;
        splitReadOffset = currentChunkOffset;

    }

    ReportSearchLowChunck(
        IsContinuedSplitRead,
        _LowSearchChunkNumber,
        currentChunkOffset,
        chunkSizeRead,
        foundValidRecord,
        foundHole);

    while (currentChunkOffset < chunkSizeRead)
    {
        currentChunkFileOffset = ToFileOffsetFromChunk(_LowSearchChunkNumber) + currentChunkOffset;

        if (RecordHeaderAppearsValid(currentChunkRecord, currentChunkFileOffset))
        {
            //
            // If the record header is correct then promote our
            // currentChunkRecord pointer to point at the header and
            // the metadata that follows
            //
            status = GetCurrentChunkRecordAndMetadata(*_IoBuffer0,
                                                      *currentChunkIoBufferElementHint, currentChunkIoBufferPositionHint,
                                                      currentChunkOffset - splitReadOffset,
                                                      currentChunkRecord,
                                                      currentRecordHeaderBuffer, currentRecordHeaderBufferSize);
            if (! NT_SUCCESS(status))
            {
                //
                // Probably an out of memory error
                //
                KTraceFailedAsyncRequest(status, this, currentChunkOffset, chunkSizeRead);
                DoComplete(status);
                return;
            }
            
            // May have the valid start of a record
            if (!IsContinuedSplitRead &&
                ((currentChunkOffset + currentChunkRecord->ThisHeaderSize) > _Config.GetMaxQueuedWriteDepth()))
            {
                // Must do a Split Read...
                // We have the last record header (plus metadata) in the chunk split across the end of the current
                // chunk and the next (which could be wrapped to the start of file).

                // Save state to be restored @ #1 above when continued after ScanSplitRecordComplete()
                _SavedCurrentChunkOffset = currentChunkOffset;
                _SavedFoundValidRecord = foundValidRecord;
                _SavedFoundHole = foundHole;

                // At this point _IoBuffer0 contains the start of the final record header in the current
                // chunk that is split across that chunk boundary. The remaining portion of that header and
                // metadata must be read in but that remaining portion itself may be wrapped back to
                // the start of file. The approach is to accumulate that entire header (and metadata) in
                // _SplitReadBuffer and then re-enter this logic - SearchLowChunckRead() - again.

                void* splitReadBuffer;
                status = KIoBufferElement::CreateNew(
                    currentChunkRecord->ThisHeaderSize,
                    _SplitReadBuffer,
                    splitReadBuffer,
                    GetThisAllocator(),
                    KTL_TAG_LOGGER);

                if (!NT_SUCCESS(status))
                {
                    KIoBufferElementCreationFailed(status, __LINE__);
                    DoComplete(status);
                    return;
                }

                ULONG headerPortionSizeRead = chunkSizeRead - currentChunkOffset;
                KAssert(headerPortionSizeRead < currentChunkRecord->ThisHeaderSize);

                // Save what's been read already
                KMemCpySafe(splitReadBuffer, _SplitReadBuffer->QuerySize(), currentChunkRecord, headerPortionSizeRead);

                // Start the next read(s) to pull in remaining record header and metadata - compute an
                // an I/O plan and start the read(s)
                StartMultipartRead(
                    currentChunkRecord->Lsn + headerPortionSizeRead,
                    currentChunkRecord->ThisHeaderSize - headerPortionSizeRead,
                    KAsyncContextBase::CompletionCallback(this, &RvdLogRecovery::SplitRecordReadComplete));

                // Continue @ SplitRecordReadComplete() - then this function with IsContinuedSplitRead->TRUE
                return;
            }

            // Finish validation of the header
            ULONG sizeToCheckSum = sizeof(RvdLogRecordHeader) + currentChunkRecord->MetaDataSize;

            if (sizeToCheckSum <= currentRecordHeaderBufferSize)
            {
                ULONGLONG savedChksum = currentChunkRecord->ThisBlockChecksum;
                currentChunkRecord->ThisBlockChecksum = 0;
                ULONGLONG partialChksum = KChecksum::Crc64(
                    ((UCHAR*)currentChunkRecord) + sizeof(currentChunkRecord->LsnChksumBlock),
                    sizeToCheckSum - sizeof(currentChunkRecord->LsnChksumBlock));

                currentChunkRecord->ThisBlockChecksum = savedChksum;

                if (KChecksum::Crc64(
                        currentChunkRecord,
                        sizeof(currentChunkRecord->LsnChksumBlock),
                        partialChksum) == savedChksum)
                {
                    // We have a physically correct record (header). Must be higher in LSN space than observed to
                    // be interesting as we are searching for the highest LSN record in the log. _NextLsnToWrite
                    // is used to track the next min LSN value OR the next exact value when scanning a valid
                    // region.
                    ULONG   recordSize = currentChunkRecord->IoBufferSize + currentChunkRecord->ThisHeaderSize;

                    ReportPhysicallyCorrectRecordHeader(currentChunkFileOffset, *currentChunkRecord, __LINE__);

                    // There are three interesting conditions: 1) BOF, 2) 1st valid record in current chunk >= _NextLsnToWrite, or
                    // 3) record exactly at _NextLsnToWrite in a valid region in the current chunk
                    if  (_NextLsnToWrite.IsNull()                                           ||
                        (!foundValidRecord && currentChunkRecord->Lsn >= _NextLsnToWrite)   ||
                        (foundValidRecord && currentChunkRecord->Lsn == _NextLsnToWrite))
                    {
                        // We have found a record which is exactly beyond the current observed max LSN (or the first in the file).
                        // Capture limits and "high-water marks".
                        foundValidRecord = TRUE;
                        _HighestLsnChunkNumber = _LowSearchChunkNumber;

                        KAssert(_HighestLsn < currentChunkRecord->Lsn);
                        _HighestLsn = currentChunkRecord->Lsn;
                        _NextLsnToWrite = currentChunkRecord->Lsn + recordSize;
                        _HighestCompletedLsn = currentChunkRecord->HighestCompletedLsn;

                        if (currentChunkRecord->LogStreamId == RvdDiskLogConstants::CheckpointStreamId())
                        {
                            KAssert(currentChunkRecord->Lsn >= _HighestCheckpointLsn);
                            _HighestCheckpointLsn = currentChunkRecord->Lsn;
                        }
                        else
                        {
                            KAssert(currentChunkRecord->LastCheckPointLsn >= _HighestCheckpointLsn);
                            _HighestCheckpointLsn = currentChunkRecord->LastCheckPointLsn;
                        }

                        ReportPhysicallyCorrectAndPlacedRecordHeader(
                            currentChunkFileOffset,
                            *currentChunkRecord,
                            __LINE__,
                            _HighestCheckpointLsn);
                    }
                    else if ((currentChunkRecord->Lsn != _NextLsnToWrite) && foundValidRecord)
                    {
                        // we have a case where records in this chunk contain a valid highest LSN
                        // record followed by a valid record of a lower LSN. This indicates that
                        // the log has potentially wrapped. Treat the lower LSN record as a hole.
                        foundHole = TRUE;
                        ReportOldRecordHeader(currentChunkFileOffset, *currentChunkRecord, __LINE__);
                        break;
                    }

                    // Scanning forward within a set of valid record(s) in the current chunk
                    currentChunkOffset += recordSize;                
                    currentChunkRecord = GetCurrentChunkRecordHeader(*_IoBuffer0, currentChunkOffset, recordSize,
                                                         currentChunkIoBufferElementHint,
                                                         currentChunkIoBufferPositionHint,
                                                         chunkIoBufferSize,
                                                         inElementOffset,
                                                         inElementPointer);

                    continue;
                }
            }
        }
        else
        {
            // If we reach this point and IsContinuedSplitRead is true, it means that we are continuing
            // with the last record of the chunk - and as such the RecordHeaderAppearsValid() must
            // have returned true initially (and should have this second time thru) but for some reason
            // it did not.
            KInvariant(!IsContinuedSplitRead);
        }

        // State @ currentChunkRecord (current buffer offset) is not a valid record
        if (foundValidRecord)
        {
            // had found at least one previous valid record (set) - we have detected a hole
            // after that valid record set.
            foundHole = TRUE;
            ReportHoleFoundInChunk(currentChunkFileOffset, _LowSearchChunkNumber, __LINE__);

            if (_IsSparseFile)
            {
                //
                // Verify that the hole is not actually a corrupted record, at
                // least on a sparse file. Unused regions of sparse files are
                // always zero and so a non-zero section indicates a corrupted
                // record
                //
                PULONGLONG p = (PULONGLONG)currentChunkRecord;
                ULONGLONG zero = 0;
                ULONG count = RvdDiskLogConstants::BlockSize / sizeof(ULONGLONG);
                for (ULONG i = 0; i < count; i++)
                {
                    if (p[i] != zero)
                    {
                        //
                        // This is a corrupted record
                        //
                        KTraceFailedAsyncRequest(K_STATUS_LOG_STRUCTURE_FAULT, this, currentChunkFileOffset, i);                            
                        DoComplete(K_STATUS_LOG_STRUCTURE_FAULT);
                        return;
                    }
                }
            }
            
            break;
        }

        // Continue the scan of a non-valid region in the current chunk
        if (!IsContinuedSplitRead)
        {
            currentChunkOffset += RvdDiskLogConstants::BlockSize;
            currentChunkRecord = GetCurrentChunkRecordHeader(*_IoBuffer0, currentChunkOffset, RvdDiskLogConstants::BlockSize,
                                                     currentChunkIoBufferElementHint,
                                                     currentChunkIoBufferPositionHint,
                                                     chunkIoBufferSize,
                                                     inElementOffset,
                                                     inElementPointer);

        }
        else
        {
            // This occurs when we are continued with a split read at the end of a chunk; cause exit from the
            // loop
            break;
        }
    }

    // At this point we must decide how/if to continue the binary search of the chunks in the file
    if (!foundValidRecord)
    {
        if (_LowSearchChunkNumber == 0)
        {
            // There are no valid records at the start of file (within the first chunk) - the log could be
            // empty or there could be a valid high LSN record at the end of the file (wrapped or not).
            //
            // Trigger searching the last chunk in the file
            _LowSearchChunkNumber = _MaxNumberOfSearchChunks - 1;
            _HighestLsnChunkNumber = _LowSearchChunkNumber;

            ReadLowChunck();
            return;
        }

        // Found an completly "empty" chunk @ _LowSearchChunkNumber - we jumped too far ahead in the b-search. A valid
        // end (head) of the log must be in a chunk lower than _LowSearchChunkNumber (just read and searched)
        // but => _HighestLsnChunkNumber.

        // Half the search space btw these two points.
        KAssert(_HighestLsnChunkNumber <= _LowSearchChunkNumber);
        ULONGLONG remainingSearchSpace = _LowSearchChunkNumber - _HighestLsnChunkNumber;
        if (remainingSearchSpace > 1)
        {
            // Search [_HighestLsnChunkNumber, _LowSearchChunkNumber)
            _MaxNumberOfSearchChunks = _LowSearchChunkNumber;

            _SearchSpaceStep = remainingSearchSpace / 2;
            _LowSearchChunkNumber = _HighestLsnChunkNumber + _SearchSpaceStep;
            KAssert(_LowSearchChunkNumber < _MaxNumberOfSearchChunks);

            ReadLowChunck();
            return;
        }

        //** We have isolated the highest LSN chuck @ _HighestLsnChunkNumber.
        // Falls thru to continuation logic below.
    }
    else
    {
        // Valid record(s) found...
        if (!foundHole)
        {
            // The current chunk just searched @_LowSearchChunkNumber ended in a set of valid
            // records.
            if (_LowSearchChunkNumber != (_MaxNumberOfSearchChunks - 1)) 
            {
                // There are more chunks in the interval (_LowSearchChunkNumber, _MaxNumberOfSearchChunks-1]
                // Search next chunk - in binary search fashion - halve the remaining space
                _SearchSpaceStep = (_MaxNumberOfSearchChunks - _LowSearchChunkNumber) / 2;
                KAssert((LONG)_SearchSpaceStep > 0);

                _LowSearchChunkNumber = _LowSearchChunkNumber + _SearchSpaceStep;
                KAssert(_LowSearchChunkNumber < _LogFileNumberOfChuncks);

                ReadLowChunck();
                return;
            }

            // The last chunk in the log is full (exactly) with proper records in increasing order.
            // Falls thru to continuation logic below.
        }

        // Found the last chunk in the log with a continuous sequence of proper records with a "hole" after it.
        // This appears to be the last chunk that can have the head of the log in it.
        //
        // Falls thru to continuation logic below.
    }

    //*** Binary search for the highest LSN is complete

    // At this point we have found the highest LSN record ever written to the log in the correct location OR
    // we have an empty log (_HighestLsn.IsNull()).

    if (_HighestLsn.IsNull())
    {
        // We have detected an empty log
        ReportEmptyLogFound(__LINE__);
        CompleteWithEmptyLogIndication();
        return;
    }

    // The previous space (max of RvdDiskLogConstants::MaxQueuedWriteDepth) before this highest record in
    // LSN-space will now be checked to see if there is a hole. If there is a hole, then the highest LSN
    // record in the first valid region in that space will be the actual highest LSN that could have been
    // observed by a logger application and thus will become the highest LSN of the recovered log.

    KAssert(_HighestLsn < _NextLsnToWrite);
    ULONGLONG   highestLsnRecordSize = _NextLsnToWrite.Get() - _HighestLsn.Get();
    KAssert(highestLsnRecordSize <= _Config.GetMaxQueuedWriteDepth());

    LONGLONG    sizeOfSpacePreceedingHighLsn = _Config.GetMaxQueuedWriteDepth() - highestLsnRecordSize;
    RvdLogLsn   lowBaseLsnToSearch = (_HighestLsn.Get() < sizeOfSpacePreceedingHighLsn)
                                        ? RvdLogLsn::Min()
                                        : _HighestLsn.Get() - sizeOfSpacePreceedingHighLsn;

    KAssert(_HighestLsn >= lowBaseLsnToSearch);
    sizeOfSpacePreceedingHighLsn = _HighestLsn.Get() - lowBaseLsnToSearch.Get();
    KAssert((sizeOfSpacePreceedingHighLsn % RvdDiskLogConstants::BlockSize) == 0);

    ReportHighestLsnRecordHeaderChunkFound(
        _LowSearchChunkNumber,
        currentChunkFileOffset,
        _HighestLsn,
        highestLsnRecordSize,
        sizeOfSpacePreceedingHighLsn);

    if (sizeOfSpacePreceedingHighLsn == 0)
    {
        ValidateLogHeadRecords();
        return;
    }

    // Read in the rest of the relative chunk (RvdDiskLogConstants::MaxQueuedWriteDepth bytes) that
    // contains the entire record @_HighestLsn - as its last record.
    StartMultipartRead(
        lowBaseLsnToSearch,
        (ULONG)sizeOfSpacePreceedingHighLsn,
        KAsyncContextBase::CompletionCallback(this, &RvdLogRecovery::SearchFinalHighLsnReadComplete));
    // Continued @ SearchFinalHighLsnReadComplete()
}

VOID
RvdLogRecovery::SearchFinalHighLsnReadComplete(__in_opt KAsyncContextBase* const Parent, __in KAsyncContextBase& CompletedContext)
// Continued from SearchLowChunckRead() - via StartMultipartRead()
{
    UNREFERENCED_PARAMETER(Parent);
    
    NTSTATUS status = CompletedContext.Status();

    // NOTE: Multiple completions could be in flight for this logical read due to file wrap - see StartMultipartRead()
    InterlockedCompareExchange(&_ValidationProcessStatus, status, STATUS_SUCCESS);
    KAssert(_ReadOpCount > 0);
    if (InterlockedDecrement(&_ReadOpCount) != 0)
    {
        // More read ops in this logical read
        return;
    }

    // Logical read is complete
    if (!NT_SUCCESS(_ValidationProcessStatus))
    {
        MultipartReadFailed(_ValidationProcessStatus, __LINE__);
        DoComplete(_ValidationProcessStatus);
        return;
    }

    KInvariant(_IoBuffer0->QueryNumberOfIoBufferElements() == 1);
    ReportSearchingForFinalHighLsn(_FileOffset, _MultipartReadSize, _DidMultipartRead);

    // If multiple read ops used for the logical read - glue buffers back together
    if (_IoBuffer1 != nullptr)
    {
        KInvariant(_IoBuffer1->QueryNumberOfIoBufferElements() == 1);
        KAssert(_DidMultipartRead);
        KAssert(_MultipartReadSize == (_IoBuffer0->QuerySize() + _IoBuffer1->QuerySize()));

        // We did a split read - build a single memory continous buffer under _IoBuffer0 for the sake
        // of simplicity.
        KIoBufferElement::SPtr      b;
        UCHAR*                      bPtr;

        // Hint to prefast
        __analysis_assume(_MultipartReadSize == (_IoBuffer0->QuerySize() + _IoBuffer1->QuerySize()));

        NTSTATUS status1 = KIoBufferElement::CreateNew(
            _MultipartReadSize,
            b,
            (void*&)bPtr,
            GetThisAllocator(),
            KTL_TAG_LOGGER);

        if (!NT_SUCCESS(status1))
        {
            KIoBufferElementCreationFailed(status1, __LINE__);
            DoComplete(status1);
            return;
        }

        KMemCpySafe(bPtr, b->QuerySize(), _IoBuffer0->First()->GetBuffer(), _IoBuffer0->QuerySize());
        KMemCpySafe(
            bPtr + _IoBuffer0->QuerySize(), 
            b->QuerySize() - _IoBuffer0->QuerySize(), 
            _IoBuffer1->First()->GetBuffer(), 
            _IoBuffer1->QuerySize());

        _IoBuffer1.Reset();
        _IoBuffer0->Clear();
        _IoBuffer0->AddIoBufferElement(*b.RawPtr());
        KInvariant(_IoBuffer0->QueryNumberOfIoBufferElements() == 1);
    }
    else
    {
        KAssert(_MultipartReadSize == _IoBuffer0->QuerySize());
    }

    // The block of file state in _IoBuffer0 is the imd preceeding bytes before the record @_HighestLsn.
    // That block may have been have a hole in it due to parallel writes occuring with this region during
    // a crash - that region in question is bounded on the high end by _HighestLsn. The following scan
    // will detect the largest record that is less than _HighestLsn with a hole btw it and _HighestLsn - if
    // there is one. If there is not a hole, the current _HighestLsn is the actual head of the queue. If
    // there is a hole, the actual head record of the log will be highest record before the hole.
    //
    // The recorded highest completed LSN and latest cp record LSN are captured during this scan.
    //
    RvdLogLsn               nextExpectedLsn(_MultipartReadLsn);
    RvdLogRecordHeader*     currentRecord = (RvdLogRecordHeader*)_IoBuffer0->First()->GetBuffer();
    ULONGLONG               currentOffset = 0;
    BOOLEAN                 foundValidRecord = FALSE;
    BOOLEAN                 foundHole = FALSE;
    RvdLogLsn               nextLsnToWrite;
    RvdLogLsn               highestLsn;
    RvdLogLsn               highestCompletedLsn;
    RvdLogLsn               highestCheckpointLsn;
    ULONGLONG               currentFileOffset = 0;  // Assumes IoBuffer0->QuerySize() > 0

    while (currentOffset < _IoBuffer0->QuerySize())
    {
        currentFileOffset = RvdLogRecordHeader::ToFileAddress(nextExpectedLsn, _LogFileLsnSpace);

        if (RecordHeaderAppearsValid(currentRecord, currentFileOffset, &nextExpectedLsn))
        {
            // May have the valid start of a record

            // Finish validation of the header and metadata
            NTSTATUS status1 = currentRecord->ChecksumIsValid(_IoBuffer0->QuerySize() - static_cast<ULONG>(currentOffset));
                    
            if (NT_SUCCESS(status1))
            {
                // We have physically a correct record (header). Must be higher in LSN space than observed to
                // be interesting as we are searching for the head of the log
                ULONG   recordSize = currentRecord->IoBufferSize + currentRecord->ThisHeaderSize;

                ReportPhysicallyCorrectRecordHeader(currentFileOffset, *currentRecord, __LINE__);

                if (nextLsnToWrite.IsNull() || (currentRecord->Lsn == nextLsnToWrite))
                {
                    // We have found a record which is exactly beyond the current observed max LSN (or the first in the file).
                    // Capture limits and "high-water marks".
                    foundValidRecord = TRUE;

                    KAssert(highestLsn < currentRecord->Lsn);
                    highestLsn = currentRecord->Lsn;
                    nextLsnToWrite = currentRecord->Lsn + recordSize;
                    highestCompletedLsn = currentRecord->HighestCompletedLsn;

                    if (currentRecord->LogStreamId == RvdDiskLogConstants::CheckpointStreamId())
                    {
                        KAssert(currentRecord->Lsn >= highestCheckpointLsn);
                        highestCheckpointLsn = currentRecord->Lsn;
                    }
                    else
                    {
                        KAssert(currentRecord->LastCheckPointLsn >= highestCheckpointLsn);
                        highestCheckpointLsn = currentRecord->LastCheckPointLsn;
                    }

                    ReportPhysicallyCorrectAndPlacedRecordHeader(
                        currentFileOffset,
                        *currentRecord,
                        __LINE__,
                        highestCheckpointLsn);
                }
                else if ((currentRecord->Lsn != nextLsnToWrite) && foundValidRecord)
                {
                    // we have a case where records in this chunk contain a valid highest LSN
                    // record followed by a valid record of a lower LSN. This indicates that
                    // the log has potentially wrapped. Treat the lower LSN record as a hole.
                    foundHole = TRUE;

                    ReportOldRecordHeader(
                        currentFileOffset,
                        *currentRecord,
                        __LINE__);

                    break;
                }

                currentOffset += recordSize;
                nextExpectedLsn = nextExpectedLsn.Get() + recordSize;
                currentRecord = (RvdLogRecordHeader*)(((UCHAR*)currentRecord) + recordSize);
                continue;
            }
        }

        if (foundValidRecord)
        {
            foundHole = TRUE;
            ReportHoleFoundSearchingForFinalHighLsn(currentFileOffset, __LINE__);
            break;
        }

        currentOffset += RvdDiskLogConstants::BlockSize;
        nextExpectedLsn = nextExpectedLsn.Get() + RvdDiskLogConstants::BlockSize;
        currentRecord = (RvdLogRecordHeader*)(((UCHAR*)currentRecord) + RvdDiskLogConstants::BlockSize);
    }

    if (foundValidRecord)
    {
        if (!foundHole)
        {
            // We found a continous set of valid record preceeding the record @ _HighestLsn. The value
            // of nextLsnToWrite must equal _HighestLsn or we have a structure fault.
            if (nextLsnToWrite != _HighestLsn)
            {
                LsnOrderingFailed(currentFileOffset, highestLsn, nextLsnToWrite, _HighestLsn, __LINE__);
                DoComplete(K_STATUS_LOG_STRUCTURE_FAULT);
                return;
            }

            // NOTE: the record @ _HighestLsn is truly the highest that could have been observed by
            //       a logger application.
        }
        else
        {
            // A hole was found, the highest LSN before that hole is the highest that could be
            // of interest - iif that LSN is lower than _HighestLsn.
            if (highestLsn >= _HighestLsn)
            {
                LsnOrderingFailed(currentFileOffset, highestLsn, nextLsnToWrite, _HighestLsn, __LINE__);
                DoComplete(K_STATUS_LOG_STRUCTURE_FAULT);
                return;
            }

            //*** Use highestLsn and its high water marks as the _HighestLsn and such.
            _NextLsnToWrite = nextLsnToWrite;
            _HighestLsn = highestLsn;
            _HighestCompletedLsn = highestCompletedLsn;
            _HighestCheckpointLsn = highestCheckpointLsn;
        }
    }

    ReportLogHeadRegion(_NextLsnToWrite, _HighestLsn, _HighestCompletedLsn, _HighestCheckpointLsn);
    ValidateLogHeadRecords();
}

VOID
RvdLogRecovery::ValidateLogHeadRecords()
// Continued from SearchFinalHighLsnReadComplete() OR SearchLowChunckReadComplete()
{
    // At this point the record @ _HighestLsn represents the actual highest record sequence who's
    // record header and metadata were successfully written - based soley on headers in the log file.
    // NOTE: The IoBuffer region following the header for this and others records may or may not be valid.

    if (!_HighestCompletedLsn.IsNull() && (_HighestCompletedLsn > _HighestLsn))
    {
        // For some reason the highest completed LSN recorded in record headers is
        // not coherent with _HighestLsn.
        HighestCompletedLsnNotCoherentWithHighestLsn(_HighestCompletedLsn, _HighestLsn);
        DoComplete(K_STATUS_LOG_STRUCTURE_FAULT);
        return;
    }

    // Now try and validate user aspects of all the records in the interval
    // [_HighestCompletedLsn, _HighestLsn]. The goal is to try and push _HighestCompletedLsn
    // toward _HighestLsn as much as possible to recover as many records as possible after
    // _HighestCompletedLsn-1.
    KAssert(!_HighestLsn.IsNull());
    _LowestHeadLsn = (_HighestCompletedLsn.IsNull()) ? RvdLogLsn::Min() : _HighestCompletedLsn;
    _HighestHeadLsn = _HighestLsn;
    KAssert(_LowestHeadLsn <= _HighestHeadLsn);

    StartLowestHeadRecordRead();
}

VOID
RvdLogRecovery::StartLowestHeadRecordRead()
{
    // Read the first block of the record @_LowestHeadLsn
    _RecordReader->StartRead(
        _LowestHeadLsn,
        _IoBuffer0,
        nullptr,
        KAsyncContextBase::CompletionCallback(this, &RvdLogRecovery::LowestHeadRecordReadComplete));
    // Continued @ HeadRecordReadComplete()
}

VOID
RvdLogRecovery::LowestHeadRecordReadComplete(__in_opt KAsyncContextBase* const Parent, __in KAsyncContextBase& CompletedContext)
// Continued from StartHeadRecordRead()
{
    UNREFERENCED_PARAMETER(Parent);
    
    NTSTATUS status = CompletedContext.Status();
    CompletedContext.Reuse();

    if (status == STATUS_CRC_ERROR)
    {
        if ((_LowestHeadLsn == _HighestCompletedLsn)
            || (_HighestCompletedLsn.IsNull() && _LowestHeadLsn == RvdLogLsn::Min()))
        {
            // There are really no records in the file
            ReportEmptyLogFound(__LINE__);
            CompleteWithEmptyLogIndication();
            return;
        }
        else
        {
            ReportLowestHeadLsnCrcError(_LowestHeadLsn);
            ReadLastCheckpoint();
            return;
        }
    }

    if (!NT_SUCCESS(status))
    {
        RecordReaderFailed(status, _LowestHeadLsn, __LINE__);
        DoComplete(status);
        return;
    }

    // Finish validation of the record @_LowestHeadLsn
    KAssert(_IoBuffer0->QueryNumberOfIoBufferElements() == 1);

    RvdLogRecordHeader*     recordHeader = (RvdLogRecordHeader*)_IoBuffer0->First()->GetBuffer();
    ULONG                   recordSize = recordHeader->IoBufferSize + recordHeader->ThisHeaderSize;
    KAssert(_IoBuffer0->QuerySize() == recordSize);

    // Validate the user portions of the record @_LowestHeadLsn
    void*       metadataPtr = nullptr;
    ULONG       metatdataSize = 0;
    void*       ioBufferPtr = nullptr;
    RvdLogLsn   recordsCheckpointLsn;
    RvdLogAsn   userRecordsAsn;
    ULONGLONG   userRecordsAsnVersion = 0;

    if (recordHeader->LogStreamId == RvdDiskLogConstants::CheckpointStreamId())
    {
        // Checkpoint stream record
        if (recordHeader->IoBufferSize != 0)
        {
            PhysicalCheckpointRecordIoBufferSizeNotZero(_LowestHeadLsn, __LINE__);
            DoComplete(K_STATUS_LOG_STRUCTURE_FAULT);
            return;
        }

        recordsCheckpointLsn = _LowestHeadLsn;
        ReportPhysicalCheckpointRecordFound(recordsCheckpointLsn, __LINE__);
    }
    else
    {
        // User stream record
        recordsCheckpointLsn = recordHeader->LastCheckPointLsn;

        RvdLogUserStreamRecordHeader* userStreamRecordHeader = (RvdLogUserStreamRecordHeader*)recordHeader;
        switch (userStreamRecordHeader->RecordType)
        {
            case RvdLogUserStreamRecordHeader::Type::UserRecord:
            {
                RvdLogUserRecordHeader* userRecordHeader = (RvdLogUserRecordHeader*)userStreamRecordHeader;
                if (userRecordHeader->MetaDataSize < (sizeof(RvdLogUserRecordHeader) - sizeof(RvdLogRecordHeader)))
                {
                    UserRecordHeaderFault(*userStreamRecordHeader, __LINE__);
                    DoComplete(K_STATUS_LOG_STRUCTURE_FAULT);
                    return;
                }

                // Have valid user log record header (sans the Asn-space aspects)
                metadataPtr = &userRecordHeader->UserMetaData[0];
                metatdataSize = userRecordHeader->MetaDataSize
                                    - (sizeof(RvdLogUserRecordHeader) - sizeof(RvdLogRecordHeader));

                ioBufferPtr = ((UCHAR*)userRecordHeader) + userRecordHeader->ThisHeaderSize;
                userRecordsAsn = userRecordHeader->Asn;
                userRecordsAsnVersion = userRecordHeader->AsnVersion;
            }
            break;

            case RvdLogUserStreamRecordHeader::Type::CheckPointRecord:
            {
                if (recordHeader->IoBufferSize != 0)
                {
                    UserRecordHeaderFault(*userStreamRecordHeader, __LINE__);
                    DoComplete(K_STATUS_LOG_STRUCTURE_FAULT);
                    return;
                }
            }
            break;

            default:
            {
                UserRecordHeaderFault(*userStreamRecordHeader, __LINE__);
                DoComplete(K_STATUS_LOG_STRUCTURE_FAULT);
                return;
            }
            break;
        }
    }

    if ((ioBufferPtr != nullptr) || (metatdataSize > 0))
    {
        if (recordHeader->IoBufferSize > 0)
        {
            _IoBufferView->SetView(*_IoBuffer0, recordHeader->ThisHeaderSize, recordHeader->IoBufferSize);
            KAssert(_IoBufferView->First()->GetBuffer() == ioBufferPtr);
        }
        else
        {
            _IoBufferView->Clear();
        }

        KIoBuffer::SPtr     tPtr = _IoBufferView.RawPtr();

        status = ValidateUserRecord(
            userRecordsAsn,
            userRecordsAsnVersion,
            metadataPtr,
            metatdataSize,
            recordHeader->LogStreamType,
            tPtr);

        if (!NT_SUCCESS(status))
        {
            ValidateUserRecordFailed(*recordHeader, __LINE__);

            if (status == K_STATUS_LOG_STRUCTURE_FAULT)
            {
                // "Normal" record rejection
                ReadLastCheckpoint();
                return;
            }

            DoComplete(status);
            return;
        }
    }

    // Record @ _LowestHeadLsn is valid - record new _HighestCompletedLsn and related values;
    // then move to the next record (if any)
    KAssert(_HighestCompletedLsn <= _LowestHeadLsn);

    _HighestCompletedLsn = _LowestHeadLsn;
    _NextLsnToWrite = _HighestCompletedLsn + recordSize;
    _HighestCheckpointLsn = recordsCheckpointLsn;

    _LowestHeadLsn += recordSize;
    _IoBuffer0.Reset();

    if (_LowestHeadLsn <= _HighestHeadLsn)
    {
        // Loop reading up all records in the head range
        StartLowestHeadRecordRead();
        return;
    }

    ReadLastCheckpoint();
}

VOID
RvdLogRecovery::ReadLastCheckpoint()
{
    if (_HighestCheckpointLsn.IsNull())
    {
        HighestCheckpointLsnNotFound();
        DoComplete(K_STATUS_LOG_STRUCTURE_FAULT);
        return;
    }

    // At this point _HighestCompletedLsn, _NextLsnToWrite, and _HighestCheckpointLsn are accurate.
    // Next read in the last checkpoint record @_HighestCheckpointLsn to retrieve all stream state
    // as of that checkpoint.
    ReportHighestLsnsEstablished(_NextLsnToWrite, _HighestCompletedLsn, _HighestCheckpointLsn);

    _RecordReader->StartRead(
        _HighestCheckpointLsn,
        _IoBuffer0,
        nullptr,
        KAsyncContextBase::CompletionCallback(this, &RvdLogRecovery::ReadLastCheckpointComplete));
    // Continued @ ReadLastCheckpointComplete()
}

VOID
RvdLogRecovery::ReadLastCheckpointComplete(__in_opt KAsyncContextBase* const Parent, __in KAsyncContextBase& CompletedContext)
// Continued from ReadLastCheckpoint()
{
    UNREFERENCED_PARAMETER(Parent);
    
    NTSTATUS status = CompletedContext.Status();
    CompletedContext.Reuse();

    if (!NT_SUCCESS(status))
    {
        RecordReaderFailed(status, _HighestCheckpointLsn, __LINE__);
        DoComplete(status);
        return;
    }

    KInvariant(_IoBuffer0->QueryNumberOfIoBufferElements() == 1);
    RvdLogPhysicalCheckpointRecord* cpRecord = (RvdLogPhysicalCheckpointRecord*)_IoBuffer0->First()->GetBuffer();
    if ((cpRecord->IoBufferSize != 0) ||
        (cpRecord->NumberOfLogStreams > _Config.GetMaxNumberOfStreams()) ||
        (cpRecord->NumberOfLogStreams < 1))
    {
        PhysicalCheckpointRecordFault(*cpRecord, __LINE__);
        DoComplete(K_STATUS_LOG_STRUCTURE_FAULT);
        return;
    }

    ReportPhysicalCheckpointRecordFound(_HighestCheckpointLsn, __LINE__);
    ReportHighestPhysicalCheckpoint(*cpRecord);

    // Save all RvdLogStreamInformation's for all streams existing at the time of the latest CP write. Also
    // collect overall lowest Lsns across all streams to recover the overall lowest Lsn for the log file.
    _NumberOfStreams = 0;
    RvdLogStreamInformation*    cpStream = nullptr;
    RvdLogLsn                   lowestLsn(RvdLogLsn::Max());

    for (ULONG ix = 0; ix < cpRecord->NumberOfLogStreams; ix++)
    {
        if ((cpRecord->LogStreamArray[ix].LogStreamId != RvdDiskLogConstants::InvalidLogStreamId()) &&
            (cpRecord->LogStreamArray[ix].LogStreamId != RvdDiskLogConstants::FreeStreamInfoStreamId()) &&
            (cpRecord->LogStreamArray[ix].LogStreamType != RvdDiskLogConstants::DeletingStreamType()))
        {
            _StreamInfos[_NumberOfStreams] = cpRecord->LogStreamArray[ix];
            RvdLogStreamInformation& currentInfo = _StreamInfos[_NumberOfStreams];

            if (currentInfo.LogStreamId == RvdDiskLogConstants::CheckpointStreamId())
            {
                KAssert(cpStream == nullptr);
                cpStream = &currentInfo;
            }

            if ((currentInfo.LowestLsn > currentInfo.HighestLsn)  ||
                (currentInfo.HighestLsn > currentInfo.NextLsn)    ||
                (currentInfo.NextLsn > cpRecord->Lsn))
            {
                InconsistentStreamInfoFound(currentInfo, *cpRecord);
                DoComplete(K_STATUS_LOG_STRUCTURE_FAULT);
                return;
            }

            if (!currentInfo.IsEmptyStream())
            {
                if (currentInfo.LowestLsn < lowestLsn)
                {
                    lowestLsn = currentInfo.LowestLsn;
                }
            }

            ReportRecoveredStreamInfoInCheckpointRecord(_NumberOfStreams, currentInfo);
            _NumberOfStreams++;
        }
    }

    if (cpStream == nullptr)
    {
        CheckpointStreamInfoMissingFromCheckpointRecord(*cpRecord);
        DoComplete(K_STATUS_LOG_STRUCTURE_FAULT);
        return;
    }

    if (lowestLsn.IsNull())
    {
        LowestLogLsnNotEstablishedInCheckpointRecord(*cpRecord);
        DoComplete(K_STATUS_LOG_STRUCTURE_FAULT);
        return;
    }

    if (lowestLsn == RvdLogLsn::Max())
    {
        // Empty log - per cpStream info
        lowestLsn = RvdLogLsn::Min();
    }

    // Recover the in-memory CP stream info.
    if (cpRecord->Lsn == RvdLogLsn::Min())
    {
        // recovering 1st CP record written to the file - fix up the CP stream state to what it was 
        // right after it was written
        if ((cpStream->LowestLsn != RvdLogLsn::Null()) ||
            (cpStream->HighestLsn != RvdLogLsn::Null()) ||
            (cpStream->NextLsn != RvdLogLsn::Null()))
        {
            InconsistentStreamInfoFound(*cpStream, *cpRecord);
            DoComplete(K_STATUS_LOG_STRUCTURE_FAULT);
            return;
        }

        cpStream->LowestLsn = RvdLogLsn::Min();
    }

    cpStream->HighestLsn = cpRecord->Lsn;
    cpStream->NextLsn = cpRecord->Lsn + cpRecord->ThisHeaderSize + cpRecord->IoBufferSize;

    ReportPhysicalCheckpointStreamLimits(cpStream->LowestLsn, cpStream->HighestLsn, cpStream->NextLsn);

    // Capture lowest and nextToWrite (cpStream->NextLsn)
    _LowestLsn = lowestLsn;

    BOOLEAN     lastRecordIsCheckpoint = (_HighestCompletedLsn == cpStream->HighestLsn);
    if (lastRecordIsCheckpoint)
    {
        // Special case: the last record in the log is the highest CP record
        KAssert(_NextLsnToWrite == cpStream->NextLsn);
        KAssert(_HighestCompletedLsn == cpRecord->Lsn);
    }
    else
    {
        KAssert(cpStream->NextLsn <= _HighestCompletedLsn);
        _NextLsnToWrite = cpStream->NextLsn;
    }

    ReportLowestLogLsnFound(_LowestLsn);

    if (lastRecordIsCheckpoint)
    {
        AllStreamLimitsRecovered();
        return;
    }

    // Next all records in the interval [_NextLsnToWrite, _HighestCompletedLsn] will be read,
    // validated, and their related stream info limits updated. NOTE: in this case _NextLsnToWrite
    // is the record imd following the CP record.
    //
    //  From a correctness POV this _HighestCompletedLsn represents the highest LSN 
    //  that could have been observed as completed by a log application. Thus all 
    //  records written after such a highest LSN record that have have the same 
    //  _HighestCompletedLsn value recorded in their headers were written in parallel 
    //  to each other. This region of potential chaos is limited in size due to the 
    //  log-level quota throttle in the log stream write logic. This means that records 
    //  at or below (in lsn-space) the record at _HighestCompletedLsn can be assumed 
    //  to be fully laid down on the disk - sans physical, undetected, write failures 
    //  and post write media faults.
    //
    ReadNextCompletedLsn();
}

VOID
RvdLogRecovery::ReadNextCompletedLsn()
// Continued from ReadLastCheckpoint()
{
    KAssert(_NextLsnToWrite <= _HighestCompletedLsn);

    // BUG, richhas, xxxxx, consider if the entire record (with IoBuffer) should be read here. At
    //                      some point we will be reading this thru a read-ahead cache with the
    //                      limit of _NextLsnToWrite. 
    //  
    _RecordReader->StartRead(
        _NextLsnToWrite,
        _IoBuffer0,
        nullptr,
        KAsyncContextBase::CompletionCallback(this, &RvdLogRecovery::ReadNextCompletedLsnCallback));
    // Continued @ ReadNextCompletedLsnCallback()
}

VOID
RvdLogRecovery::ReadNextCompletedLsnCallback(__in_opt KAsyncContextBase* const Parent, __in KAsyncContextBase& CompletedContext)
// Continued from ReadNextCompletedLsn()
{
    UNREFERENCED_PARAMETER(Parent);
    
    NTSTATUS status = CompletedContext.Status();
    CompletedContext.Reuse();

    if (!NT_SUCCESS(status))
    {
        RecordReaderFailed(status, _NextLsnToWrite, __LINE__);
        DoComplete(status);
        return;
    }

    KAssert(_IoBuffer0->QueryNumberOfIoBufferElements() == 1);
    RvdLogRecordHeader* recordHeader = (RvdLogRecordHeader*)_IoBuffer0->First()->GetBuffer();

    // Validate and process current record's stream extent info
    RvdLogStreamInformation* streamInfo = nullptr;
    for (ULONG ix = 0; ix < _NumberOfStreams; ix++)
    {
        if (recordHeader->LogStreamId == _StreamInfos[ix].LogStreamId)
        {
            KAssert(streamInfo == nullptr);
            streamInfo = &_StreamInfos[ix];
            break;
        }
    }

    if (streamInfo == nullptr)
    {
        // Found a record for a stream not in the last CP record - should not happen as
        // Create and Delete stream operations create a CP record.
        StreamMissingInValidRecord(*recordHeader);
        DoComplete(K_STATUS_LOG_STRUCTURE_FAULT);
        return;
    }

    if (recordHeader->LogStreamType != streamInfo->LogStreamType)
    {
        InvalidStreamTypeInValidRecord(*recordHeader);
        DoComplete(K_STATUS_LOG_STRUCTURE_FAULT);
        return;
    }

    // Special case of recordHeader->PrevLsnInLogStream.IsNull(): This implies there was a truncate not represented by a CP
    if ((!streamInfo->IsEmptyStream()) && (!recordHeader->PrevLsnInLogStream.IsNull()))
    {
        if ((recordHeader->Lsn < streamInfo->NextLsn)                       ||
            (recordHeader->PrevLsnInLogStream != streamInfo->HighestLsn))
        {
            // The record described is out of sync with the stream's recovered limits
            InvalidStreamLimitsInValidRecord(*recordHeader, *streamInfo);
            DoComplete(K_STATUS_LOG_STRUCTURE_FAULT);
            return;
        }
    }
    else
    {
        // Empty stream - first record in that stream to be recovered after the last CP or CP-less truncate
        // written to log file
        streamInfo->LowestLsn = recordHeader->Lsn;
    }

    ULONG recordSize;
    streamInfo->HighestLsn = recordHeader->Lsn;
    streamInfo->NextLsn = recordHeader->Lsn
                            + (recordSize = (recordHeader->ThisHeaderSize + recordHeader->IoBufferSize));

    _NextLsnToWrite += recordSize;
    if (_NextLsnToWrite <= _HighestCompletedLsn)
    {
        // Read and process next record in interval [last CP record, _HighestCompletedLsn]
        ReadNextCompletedLsn();
        return;
    }

    AllStreamLimitsRecovered();
}

VOID
RvdLogRecovery::AllStreamLimitsRecovered()
//  Continued from ReadNextCompletedLsnCallback() or ReadLastCheckpointComplete()
{
    ReportAllStreamLimitsRecovered(_NumberOfStreams, *(&_StreamInfos[0]));

    KInvariant(_LowestLsn < _NextLsnToWrite);
    KInvariant(_LowestLsn <= _HighestCompletedLsn);
    KInvariant(_HighestCompletedLsn < _NextLsnToWrite);
    KInvariant(_HighestCheckpointLsn < _NextLsnToWrite);
    KInvariant(_HighestCheckpointLsn <= _HighestCompletedLsn);

    ULONGLONG spaceUsed = _NextLsnToWrite.Get() - _LowestLsn.Get();
    KInvariant(spaceUsed <= _LogFileLsnSpace);

    _LogFileFreeSpace = _LogFileLsnSpace - spaceUsed;

    //*** All physical log properties have been recovered. Only a physical scan of the
    //    LSN space in the interval [_LowestLsn, _NextLsnToWrite -1] will reveal additional
    //    faults.
    ReportPhysicalLogRecovered(_LogFileFreeSpace, _LowestLsn, _NextLsnToWrite, _HighestCompletedLsn, _HighestCheckpointLsn);
    DoComplete(STATUS_SUCCESS);
}


//** Common support functions
VOID
RvdLogRecovery::StartMultipartRead(__in RvdLogLsn LsnToRead, __in ULONG SizeToRead, __in KAsyncContextBase::CompletionCallback Callback)
//
// Start a read that completes up to two reads @Callback. There will be two reads iif the Lsn space requested
// wraps back to the start of file.
//
// On exit:
//      _IoBuffer0 - Read is posted @LsnToRead
//      _IoBuffer1 - Read is posted iif the requested Lsn space wraps - else == nullptr
//      _FileOffset - file offset for the first read
//      _FileOffset1 - file offset for the second read
//      _MultipartReadSize = SizeToRead
//      _MultipartReadLsn = LsnToRead
//      _DidMultipartRead - TRUE if multipart read was needed
//      _ReadOpCount - is set to the number of reads started (1 or 2)
//
//      NOTE: DoComplete() will be called if an error is detected (_ValidationProcessStatus will have the offending status)
//
{
    ULONG       firstSegSize;
    ULONGLONG   firstOffset = RvdLogRecordHeader::ToFileAddress(
        LsnToRead,
        _LogFileLsnSpace,
        SizeToRead,
        firstSegSize);

    BOOLEAN     lastLsnSpaceChunkWrapsLog = (SizeToRead != firstSegSize);
    _ReadOpCount = lastLsnSpaceChunkWrapsLog ? 2 : 1;
    _FileOffset = firstOffset;
    _FileOffset1 = 0;
    _MultipartReadSize = SizeToRead;
    _DidMultipartRead = lastLsnSpaceChunkWrapsLog;
    _MultipartReadLsn = LsnToRead;

    _IoBuffer0.Reset();
    _IoBuffer1.Reset();

    _ValidationProcessStatus = STATUS_SUCCESS;
    NTSTATUS status = _BlockDevice->Read(
        firstOffset,
        firstSegSize,
        TRUE,                   // contiguous buffer
        _IoBuffer0,
        Callback,
        nullptr,
        _BlockDeviceReadOp0);

    if (!K_ASYNC_SUCCESS(status))
    {
        DoComplete(status);
        return;
    }

    if (lastLsnSpaceChunkWrapsLog)
    {
        ULONG       secondSegSize;
        ULONGLONG   secondOffset = RvdLogRecordHeader::ToFileAddress(
            LsnToRead + firstSegSize,
            _LogFileLsnSpace,
            (SizeToRead - firstSegSize),
            secondSegSize);
        _FileOffset1 = secondOffset;

        status = _BlockDevice->Read(
            secondOffset,
            secondSegSize,
            TRUE,                   // contiguous buffer
            _IoBuffer1,
            Callback,
            nullptr,
            _BlockDeviceReadOp1);

        if (!K_ASYNC_SUCCESS(status))
        {
            InterlockedCompareExchange(&_ValidationProcessStatus, status, STATUS_SUCCESS);
            if (InterlockedDecrement(&_ReadOpCount) == 0)
            {
                DoComplete(status);
                return;
            }
            return;
        }
    }
}

BOOLEAN
RvdLogRecovery::RecordHeaderAppearsValid(
    __in RvdLogRecordHeader* const RecordHeader,
    __in ULONGLONG const CurrentFileOffset,
    __in_opt RvdLogLsn* const LsnToValidate)
{
    if (RecordHeader->IsBasicallyValid(_Config, CurrentFileOffset, _LogId, _LogSignature, _LogFileLsnSpace, LsnToValidate))
    {
        ReportRecordHeaderApprearsValid(*RecordHeader, CurrentFileOffset);
        return TRUE;
    }

    ReportRecordHeaderIsInvalid(*RecordHeader, CurrentFileOffset);
    return FALSE;
}

VOID
RvdLogRecovery::CompleteWithEmptyLogIndication()
{
    RvdLogStreamInformation& cpStreamInfo = _StreamInfos[0];
    cpStreamInfo.LogStreamId = RvdDiskLogConstants::CheckpointStreamId();
    cpStreamInfo.LogStreamType = RvdDiskLogConstants::CheckpointStreamType();
    cpStreamInfo.HighestLsn = RvdLogLsn::Null();
    cpStreamInfo.LowestLsn = RvdLogLsn::Null();
    cpStreamInfo.NextLsn = RvdLogLsn::Null();

    _LogFileFreeSpace = _LogFileLsnSpace;
    _LowestLsn = RvdLogLsn::Min();
    _HighestLsn = RvdLogLsn::Null();
    _NextLsnToWrite = RvdLogLsn::Min();
    _HighestCompletedLsn  = RvdLogLsn::Null();
    _HighestCheckpointLsn = RvdLogLsn::Null();
    _NumberOfStreams = 1;

    DoComplete(STATUS_SUCCESS);
}


//*** Support class implementations for Log recovery and verification

//** Log File Recovery/Verification Record Reader implementation
NTSTATUS
RvdLogRecoveryBase::RecordReader::Create(
    __in KBlockDevice::SPtr FromBlockDevice,
    __in RvdLogConfig& Config,
    __in ULONGLONG const LogSpace,
    __in RvdLogId& LogId,
    __in ULONG (&LogSignature)[RvdDiskLogConstants::SignatureULongs],
    __out RecordReader::SPtr& Result,
    __in ULONG const AllocationTag,
    __in KAllocator& Allocator)
{
    Result = _new(AllocationTag, Allocator) RecordReader(FromBlockDevice, Config, LogSpace, LogId, LogSignature);
    if (Result == nullptr)
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

RvdLogRecoveryBase::RecordReader::RecordReader(
    __in KBlockDevice::SPtr FromBlockDevice,
    __in RvdLogConfig&      Config,
    __in ULONGLONG const    LogSpace,
    __in RvdLogId&          LogId,
    __in ULONG(&            LogSignature)[RvdDiskLogConstants::SignatureULongs])
    :   _BlockDevice(Ktl::Move(FromBlockDevice)),
        _LogSpace(LogSpace),
        _LogId(LogId),
        _LogSignature(LogSignature),
        _Config(Config)
{
    NTSTATUS status = _BlockDevice->AllocateReadContext(_ReadOp0);
    if (NT_SUCCESS(status))
    {
        status = _BlockDevice->AllocateReadContext(_ReadOp1);
    }

    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
    }
}

RvdLogRecoveryBase::RecordReader::~RecordReader()
{
}

VOID
RvdLogRecoveryBase::RecordReader::StartRead(
    __in RvdLogLsn& AtLsn,
    __out KIoBuffer::SPtr& IoBuffer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _Lsn = AtLsn;
    _ResultingIoBuffer = &IoBuffer;
    IoBuffer.Reset();
    _IsDoingLsnRead = TRUE;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
RvdLogRecoveryBase::RecordReader::StartPhysicalRead(
    __in ULONGLONG AtOffset,
    __out KIoBuffer::SPtr& IoBuffer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _FileOffet = AtOffset;
    _ResultingIoBuffer = &IoBuffer;
    IoBuffer.Reset();
    _IsDoingLsnRead = FALSE;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
RvdLogRecoveryBase::RecordReader::DoComplete(NTSTATUS Status)
{
    _IoBuffer0.Reset();
    _IoBuffer1.Reset();
    _IoBuffer2.Reset();
    Complete(Status);
}

VOID
RvdLogRecoveryBase::RecordReader::OnStart()
{
    // Read first block of record's header
    ULONGLONG           offset = _IsDoingLsnRead ? RvdLogRecordHeader::ToFileAddress(_Lsn, _LogSpace)
                                                    : _FileOffet;
    NTSTATUS status = _BlockDevice->Read(
        offset,
        RvdDiskLogConstants::BlockSize,
        TRUE,                   // contiguous buffer
        _IoBuffer0,
        KAsyncContextBase::CompletionCallback(this, &RecordReader::HeaderReadComplete),
        this,
        _ReadOp0);

    if (!NT_SUCCESS(status))
    {
        DoComplete(status);
        return;
    }
}

VOID
RvdLogRecoveryBase::RecordReader::HeaderReadComplete(__in_opt KAsyncContextBase* const Parent, __in KAsyncContextBase& CompletedContext)
{
    NTSTATUS status = CompletedContext.Status();
    CompletedContext.Reuse();
    if (!NT_SUCCESS(status))
    {
        DoComplete(status);
        return;
    }

    RvdLogRecordHeader* header = (RvdLogRecordHeader*)_IoBuffer0->First()->GetBuffer();
    ULONGLONG           offset = _IsDoingLsnRead ? RvdLogRecordHeader::ToFileAddress(_Lsn, _LogSpace)
                                                 : _FileOffet;

    // TODO: Need to dump out why the record isn't valid

    if (!header->IsBasicallyValid(
        _Config,
        offset,
        _LogId,
        _LogSignature,
        _LogSpace,
        (_IsDoingLsnRead ? &_Lsn : nullptr)))
    {
        DoComplete(K_STATUS_LOG_STRUCTURE_FAULT);
        return;
    }

    if (!_IsDoingLsnRead)
    {
        if (RvdLogRecordHeader::ToFileAddress(header->Lsn, _LogSpace) != _FileOffet)
        {
            // Record location and it's LSN disagree
            DoComplete(K_STATUS_LOG_STRUCTURE_FAULT);
            return;
        }

        _Lsn = header->Lsn;
    }

    ULONG remainingSizeToRead = (_TotalSizeToRead = header->ThisHeaderSize + header->IoBufferSize)
                                - RvdDiskLogConstants::BlockSize;

    if (remainingSizeToRead == 0)
    {
        // Subject record was contained within first block
        FinishRecordRead();
        return;
    }

    ULONG       firstSegSize = 0;
    ULONGLONG   firstSegOffset = RvdLogRecordHeader::ToFileAddress(
                                    _Lsn + RvdDiskLogConstants::BlockSize,
                                    _LogSpace,
                                    remainingSizeToRead,
                                    firstSegSize);
    ULONG       secondSegSize = 0;
    ULONGLONG   secondSegOffset = 0;
    _NumberOfReadsLeft = 1;

    if (firstSegSize != remainingSizeToRead)
    {
        // Wrapping @ EOF
        KAssert(remainingSizeToRead > firstSegSize);
        secondSegOffset = RvdLogRecordHeader::ToFileAddress(
            _Lsn + RvdDiskLogConstants::BlockSize + firstSegSize,
            _LogSpace,
            remainingSizeToRead - firstSegSize,
            secondSegSize);

        _NumberOfReadsLeft++;
    }
    KAssert((secondSegSize + firstSegSize) == remainingSizeToRead);

    _StatusOfOtherOp = STATUS_SUCCESS;
    status = _BlockDevice->Read(
        firstSegOffset,
        firstSegSize,
        TRUE,                   // contiguous buffer
        _IoBuffer1,
        KAsyncContextBase::CompletionCallback(this, &RecordReader::RecordReadComplete),
        this,
        _ReadOp0);

    if (!K_ASYNC_SUCCESS(status))
    {
        DoComplete(status);
        return;
    }

    // NOTE: the manipulation of _StatusOfOtherOp and _NumberOfReadsLeft below (and in RecordReadComplete() assume only
    //       one thread at a time will execute with in the current RecordReader instance. The following KInvariant()s assert this.
    //
    KInvariant(this == Parent);
    KInvariant(IsInApartment());
    if (_NumberOfReadsLeft == 2)
    {
        status = _BlockDevice->Read(
            secondSegOffset,
            secondSegSize,
            TRUE,                   // contiguous buffer
            _IoBuffer2,
            KAsyncContextBase::CompletionCallback(this, &RecordReader::RecordReadComplete),
            this,
            _ReadOp1);

        if (!K_ASYNC_SUCCESS(status))
        {
            _StatusOfOtherOp = status;
            _NumberOfReadsLeft--;
            // The first read above was started ok and will complete thru RecordReadComplete().
            // It will detect _StatusOfSecondOp being set to a failure value and complete.
            return;
        }
    }

    // Continued @ RecordReadComplete()
}

VOID
RvdLogRecoveryBase::RecordReader::RecordReadComplete(__in_opt KAsyncContextBase* const Parent, __in KAsyncContextBase& CompletedContext)
// Continued from HeaderReadComplete()
{
    // NOTE: the manipulation of _StatusOfOtherOp and _NumberOfReadsLeft below (and in HeaderReadComplete() assume only
    //       one thread at a time will execute with in the current RecordReader instance. The following KInvariant()s assert this.
    //
    KInvariant(this == Parent);
    KInvariant(IsInApartment());

    NTSTATUS status = CompletedContext.Status();
    if (!NT_SUCCESS(status) && (_StatusOfOtherOp == STATUS_SUCCESS))
    {
        _StatusOfOtherOp = status;
        return;
    }

    KAssert(_NumberOfReadsLeft > 0);
    _NumberOfReadsLeft--;
    if (_NumberOfReadsLeft > 0)
    {
        return;     // All reads are not done
    }

    if (!NT_SUCCESS(_StatusOfOtherOp))
    {
        DoComplete(_StatusOfOtherOp);
        return;
    }

    FinishRecordRead();
}

VOID
RvdLogRecoveryBase::RecordReader::FinishRecordRead()
{
    NTSTATUS status;
    
    // At this point we have finished all reads and have up to three buffers to merge together
    if (_IoBuffer1 != nullptr)
    {
        // There at least two buffers to be merged
        KIoBufferElement::SPtr  be;
        void*                   bPtr;
        status = KIoBufferElement::CreateNew(
            _TotalSizeToRead,
            be,
            bPtr,
            GetThisAllocator(),
            KTL_TAG_LOGGER);

        if (!NT_SUCCESS(status))
        {
            DoComplete(status);
            return;
        }

        KInvariant(_IoBuffer1->QueryNumberOfIoBufferElements() == 1);
        KAssert((_IoBuffer0->QuerySize() + _IoBuffer1->QuerySize()) <= _TotalSizeToRead);

        KMemCpySafe(bPtr, be->QuerySize(), _IoBuffer0->First()->GetBuffer(), _IoBuffer0->QuerySize());
        KMemCpySafe(
            ((UCHAR*)bPtr) + _IoBuffer0->QuerySize(), 
            be->QuerySize() - _IoBuffer0->QuerySize(), 
            _IoBuffer1->First()->GetBuffer(), 
            _IoBuffer1->QuerySize());

        if (_IoBuffer2 != nullptr)
        {
            // There are three buffers to be merged
            KInvariant(_IoBuffer2->QueryNumberOfIoBufferElements() == 1);
            KAssert((_IoBuffer0->QuerySize() + _IoBuffer1->QuerySize() + _IoBuffer2->QuerySize()) <= _TotalSizeToRead);
            KMemCpySafe(
                ((UCHAR*)bPtr) + _IoBuffer0->QuerySize() + _IoBuffer1->QuerySize(),
                be->QuerySize() - (_IoBuffer0->QuerySize() + _IoBuffer1->QuerySize()),
                _IoBuffer2->First()->GetBuffer(), 
                _IoBuffer2->QuerySize());
        }

        // Validate the header and metadata of the record
        RvdLogRecordHeader* recordHeader = (RvdLogRecordHeader*)bPtr;

        status = recordHeader->ChecksumIsValid(_TotalSizeToRead);
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, 0, _TotalSizeToRead);          
            DoComplete(status);
            return;
        }

        // Steal one of the KIoBuffers generated by a read for the resulting KIoBuffer
        _IoBuffer0->Clear();
        _IoBuffer0->AddIoBufferElement(*be.RawPtr());
    }
    else
    {
        KAssert(_IoBuffer0->QuerySize() == RvdDiskLogConstants::BlockSize);

        // Validate the header and metadata of the record
        RvdLogRecordHeader* recordHeader = (RvdLogRecordHeader*)_IoBuffer0->First()->GetBuffer();
        
        status = recordHeader->ChecksumIsValid(_IoBuffer0->First()->QuerySize());
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, 0, _IoBuffer0->First()->QuerySize());          
            DoComplete(status);
            return;
        }
    }

    (*_ResultingIoBuffer) = Ktl::Move(_IoBuffer0);

    DoComplete(STATUS_SUCCESS);
}

//** Surface scan logic
// BUG, richhas, xxxxx, to be completed as low level log browser utilty support class is implemented

#if 0
    // State used to do a full sequential surface scan
    ULONGLONG                       _NextScanFileOffset;
    enum
    {
        ScanningValidRegion,
        ScanningInvalidRegion,
        ScanningStartOfFile
    }                               _SurfaceScanState;
    RvdLogLsn                       _FirstValidLsnScannedInRegion;
    ULONGLONG                       _FirstOffsetScannedInRegion;
    RvdLogLsn                       _LastValidLsnScanned;
    ULONG                           _LastValidLsnSizeScanned;
    ULONGLONG                       _LastValidLsnOffsetScanned;
    ULONG                           _CountOfValidRecordsScanned;
    struct InvalidScannedRegion
    {
        ULONGLONG                   StartingOffset;
        ULONGLONG                   Size;
    };

    struct ValidScannedRegion
    {
        RvdLogLsn                   StartingLsn;
        ULONGLONG                   Size;
        ULONG                       NumberOfRecords;
    };

    KArray<InvalidScannedRegion>    _InvalidScannedRegions;
    KArray<ValidScannedRegion>      _ValidScannedRegions;


    VOID
    LogValidator::StartSurfaceScan(
        __in KBlockDevice::SPtr BlockDevice,
        __in RvdLogId& LogId,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
    {
        _BlockDevice = Ktl::Move(BlockDevice);
        _LogId = LogId;
        _IsDoingSurfaceScan = TRUE;
        Start(ParentAsyncContext, CallbackPtr);
    }

    VOID
    LogValidator::OnStart()
    {
        if (_IsDoingSurfaceScan)
        {
            _NextScanFileOffset = RvdDiskLogConstants::BlockSize;
            _SurfaceScanState = ScanningStartOfFile;
            ContinueSurfaceScan();
            return;
        }
    }

    VOID
    LogValidator::RecordInvalidScannedRegion()
    {
        InvalidScannedRegion newRegion;
        newRegion.Size = _NextScanFileOffset - _FirstOffsetScannedInRegion;
        newRegion.StartingOffset = _FirstOffsetScannedInRegion;
        _InvalidScannedRegions.Append(newRegion);
    }

    VOID
    LogValidator::RecordValidScannedRegion()
    {
        ValidScannedRegion  newRegion;
        newRegion.StartingLsn = _FirstValidLsnScannedInRegion;
        newRegion.Size = _NextScanFileOffset - _FirstOffsetScannedInRegion;
        newRegion.NumberOfRecords = _CountOfValidRecordsScanned;
        _ValidScannedRegions.Append(newRegion);
    }

    VOID
    LogValidator::MergeValidScannedRegions()
    {
        BOOLEAN changesMade = TRUE;
        while (changesMade)
        {
            changesMade = FALSE;
            for (ULONG ix = 0; ix < _ValidScannedRegions.Count(); ix++)
            {
                if (_ValidScannedRegions[ix].Size > 0)
                {
                    for (ULONG ix1 = 0; ix1 < _ValidScannedRegions.Count(); ix1++)
                    {
                        if (_ValidScannedRegions[ix1].Size > 0)
                        {
                            RvdLogLsn nextLsn = _ValidScannedRegions[ix].StartingLsn + _ValidScannedRegions[ix].Size;
                            if (nextLsn == _ValidScannedRegions[ix1].StartingLsn)
                            {
                                // Found continous regions - glue them together; mark merged (ix1) item as to-be-deleted
                                _ValidScannedRegions[ix].Size += _ValidScannedRegions[ix1].Size;
                                _ValidScannedRegions[ix].NumberOfRecords += _ValidScannedRegions[ix1].NumberOfRecords;
                                _ValidScannedRegions[ix1].Size = 0;
                                _ValidScannedRegions[ix1].NumberOfRecords = 0;
                                changesMade = TRUE;
                            }
                        }
                    }
                }
            }
        }

        // delete any regions that have been merged into another region
        BOOLEAN deletionMade = TRUE;
        while (deletionMade)
        {
            deletionMade = FALSE;
            for (ULONG ix = 0; ix < _ValidScannedRegions.Count(); ix++)
            {
                if (_ValidScannedRegions[ix].Size == 0)
                {
                    KAssert(_ValidScannedRegions[ix].NumberOfRecords == 0);
                    _ValidScannedRegions.Remove(ix);
                    deletionMade = TRUE;
                    break;
                }
            }
        }
    }

    VOID
    LogValidator::TrimInvalidScannedRegions()
    {
        // Trim any invalid regions that have intersection with valid regions
        for (ULONG ix = 0; ix < _ValidScannedRegions.Count(); ix++)
        {
            // Compute the file mapping layout for the current valid record range
            ULONGLONG firstSegmentSize;
            ULONGLONG firstSegmentOffset = RvdLogRecordHeader::ToFileAddressEx(
                _ValidScannedRegions[ix].StartingLsn,
                _LogFileLsnSpace,
                _ValidScannedRegions[ix].Size,
                firstSegmentSize);

            ULONGLONG secondSegmentSize = 0;
            ULONGLONG secondSegmentOffset = 0;
            if (firstSegmentSize != _ValidScannedRegions[ix].Size)
            {
                // The valid region wraps the EOF
                KAssert(firstSegmentSize < _ValidScannedRegions[ix].Size);
                secondSegmentOffset = RvdLogRecordHeader::ToFileAddressEx(
                    _ValidScannedRegions[ix].StartingLsn + firstSegmentSize,
                    _LogFileLsnSpace,
                    _ValidScannedRegions[ix].Size - firstSegmentSize,
                    secondSegmentSize);
            }

            for (ULONG ix1 = 0; ix1 < _InvalidScannedRegions.Count(); ix1++)
            {
                AdjustInvalidScannedRegion(_InvalidScannedRegions[ix1], firstSegmentOffset, firstSegmentSize);
                if (secondSegmentSize > 0)
                {
                    AdjustInvalidScannedRegion(_InvalidScannedRegions[ix1], secondSegmentOffset, secondSegmentSize);
                }
            }

            // Scan and delete any zero lgt invalid regions
            BOOLEAN     deletionMade = TRUE;
            while (deletionMade)
            {
                deletionMade = FALSE;
                for (ULONG ix = 0; ix < _InvalidScannedRegions.Count(); ix++)
                {
                    if (_InvalidScannedRegions[ix].Size == 0)
                    {
                        _InvalidScannedRegions.Remove(ix);
                        deletionMade = TRUE;
                        break;
                    }
                }
            }
        }
    }

    VOID
    LogValidator::AdjustInvalidScannedRegion(
        InvalidScannedRegion& InvalidRegion,
        ULONGLONG ValidRegionStart,
        ULONGLONG ValidRegionSize)
    {
        ULONGLONG   lowOffset = InvalidRegion.StartingOffset;
        ULONGLONG   nextHigherOffset = lowOffset + InvalidRegion.Size;
        ULONGLONG   validRegionEndOffset = ValidRegionStart + ValidRegionSize - 1;

        if ((ValidRegionStart > lowOffset) && (ValidRegionStart < nextHigherOffset))
        {
            // valid segment overlaps the end of the invalid region - also must overlap at least to the end
            // of the invalid region
            ULONGLONG   offsetSizeToOverlap = ValidRegionStart - lowOffset;
            KAssert(InvalidRegion.Size >= offsetSizeToOverlap);
            KAssert((InvalidRegion.Size - offsetSizeToOverlap) <= ValidRegionSize);
            InvalidRegion.Size = offsetSizeToOverlap;
            nextHigherOffset = lowOffset + InvalidRegion.Size;
            KAssert(nextHigherOffset == ValidRegionStart);
        }
        else if ((validRegionEndOffset >= lowOffset) && (validRegionEndOffset < nextHigherOffset))
        {
            // valid segment overlaps the start of the invalid region - must overlap at least the start
            // of that invalid region
            ULONGLONG overlapSize = (validRegionEndOffset + 1) - lowOffset;
            KAssert(overlapSize <= InvalidRegion.Size);
            InvalidRegion.Size -= overlapSize;
            InvalidRegion.StartingOffset += overlapSize;
        }
    }

    VOID
    LogValidator::ContinueSurfaceScan()
    {
        // Attempt to read the record @_NextScanFileOffset
        if (_NextScanFileOffset >= _LogFileLsnSpace)
        {
            // @ EOL
            if (_SurfaceScanState == ScanningValidRegion)
            {
                // Was in a valid region - mark its end
                CHAR*      wrapMsg = ((_LastValidLsnOffsetScanned + _LastValidLsnSizeScanned - 1) >= _LogFileLsnSpace)
                                        ? "; WrappedEOF" : "";
                KDbgPrintf(
                    "ReportValidRegion: OffsetRange:[%I64u, %I64u]; LSN Range: [%I64u, %I64u]; Size: %I64u; Records: %u%s\n",
                    _FirstOffsetScannedInRegion,
                    _NextScanFileOffset - 1,
                    _FirstValidLsnScannedInRegion.Get(),
                    _LastValidLsnScanned.Get() + _LastValidLsnSizeScanned - 1,
                    _NextScanFileOffset - _FirstOffsetScannedInRegion,
                    _CountOfValidRecordsScanned,
                    wrapMsg);

                RecordValidScannedRegion();
            }
            else if (_SurfaceScanState == ScanningInvalidRegion)
            {
                // Was in an invalid region - mark its end
                KDbgPrintf(
                    "ReportInvalidRegion: OffsetRange:[%I64u, %I64u]; Size: %I64u\n",
                    _FirstOffsetScannedInRegion,
                    _NextScanFileOffset - 1,
                    _NextScanFileOffset - _FirstOffsetScannedInRegion);

                RecordInvalidScannedRegion();
            }

            MergeValidScannedRegions();
            TrimInvalidScannedRegions();

            for (ULONG ix = 0; ix < _InvalidScannedRegions.Count(); ix++)
            {
                KDbgPrintf(
                    "ReportFinalInvalidRegion: OffsetRange:[%I64u, %I64u]; Size: %I64u\n",
                    _InvalidScannedRegions[ix].StartingOffset,
                    _InvalidScannedRegions[ix].StartingOffset + _InvalidScannedRegions[ix].Size - 1,
                    _InvalidScannedRegions[ix].Size);
            }

            for (ULONG ix = 0; ix < _ValidScannedRegions.Count(); ix++)
            {
                ValidScannedRegion&     region = _ValidScannedRegions[ix];
                ULONGLONG               firstSegmentSize;
                ULONGLONG               firstSegmentOffset = RvdLogRecordHeader::ToFileAddressEx(
                    region.StartingLsn,
                    _LogFileLsnSpace,
                    region.Size,
                    firstSegmentSize);

                ULONGLONG               secondSegmentSize = 0;
                ULONGLONG               secondSegmentOffset = 0;

                if (firstSegmentSize != region.Size)
                {
                    // The valid region wraps the EOF
                    KAssert(firstSegmentSize < region.Size);
                    secondSegmentOffset = RvdLogRecordHeader::ToFileAddressEx(
                        region.StartingLsn + firstSegmentSize,
                        _LogFileLsnSpace,
                        region.Size - firstSegmentSize,
                        secondSegmentSize);

                    KDbgPrintf(
                        "ReportFinalValidRegion: OffsetRange:{[%I64u, %I64u], [%I64u, %I64u]}; LSN Range: [%I64u, %I64u]; Size: %I64u; Records: %u\n",
                        secondSegmentOffset,
                        secondSegmentOffset + secondSegmentSize - 1,
                        firstSegmentOffset,
                        firstSegmentOffset + firstSegmentSize - 1,
                        region.StartingLsn.Get(),
                        region.StartingLsn.Get() + region.Size - 1,
                        region.Size,
                        region.NumberOfRecords);
                }
                else
                {
                    KDbgPrintf(
                        "ReportFinalValidRegion: OffsetRange:[%I64u, %I64u]; LSN Range: [%I64u, %I64u]; Size: %I64u; Records: %u\n",
                        firstSegmentOffset,
                        firstSegmentOffset + firstSegmentSize - 1,
                        region.StartingLsn.Get(),
                        region.StartingLsn.Get() + region.Size - 1,
                        region.Size,
                        region.NumberOfRecords);
                }
            }

            ULONGLONG   fileOffSetOfHighestLsn = RvdLogRecordHeader::ToFileAddress(_HighestLsn, _LogFileLsnSpace);

            KDbgPrintf(
                "ReportFinalLogLSNs: HighestLsn: %I64u (Offset: %I64u; Chunk#: %I64u); NextLsnToWrite: %I64u; "
                "HighestCompletedLsn: %I64u; HighestCheckpointLsn: %I64u\n",
                _HighestLsn.Get(),
                fileOffSetOfHighestLsn,
                (fileOffSetOfHighestLsn - RvdDiskLogConstants::BlockSize) / RvdDiskLogConstants::MaxQueuedWriteDepth,
                _NextLsnToWrite.Get(),
                _HighestCompletedLsn.Get(),
                _HighestCheckpointLsn.Get());

            DoComplete(STATUS_SUCCESS);
            return;
        }

        _RecordReader->StartPhysicalRead(
            _NextScanFileOffset,
            _IoBuffer0,
            nullptr,
                KAsyncContextBase::CompletionCallback(this, &LogValidator::SurfaceScanReadComplete));
        // Continued @ SurfaceScanReadComplete()

    }

    VOID
    LogValidator::SurfaceScanReadComplete(KAsyncContextBase* const Parent, KAsyncContextBase& CompletedContext)
    // Continued from ContinueSurfaceScan()
    {
        NTSTATUS status = CompletedContext.Status();
        CompletedContext.Reuse();

        if (!NT_SUCCESS(status))
        {
            if ((status == STATUS_CRC_ERROR) || (status == K_STATUS_LOG_STRUCTURE_FAULT))
            {
                // Expected errors from an invalid record
                if (_SurfaceScanState != ScanningInvalidRegion)
                {
                    // Found a new invalid region start
                    if (_SurfaceScanState == ScanningValidRegion)
                    {
                        // Was in a valid region - mark its end
                        CHAR*      wrapMsg = ((_LastValidLsnOffsetScanned + _LastValidLsnSizeScanned - 1) >= _LogFileLsnSpace)
                                                ? "; WrappedEOF" : "";
                        KDbgPrintf(
                            "ReportValidRegion: OffsetRange:[%I64u, %I64u]; LSN Range: [%I64u, %I64u]; Size: %I64u; Records: %u%s\n",
                            _FirstOffsetScannedInRegion,
                            _NextScanFileOffset - 1,
                            _FirstValidLsnScannedInRegion.Get(),
                            _LastValidLsnScanned.Get() + _LastValidLsnSizeScanned - 1,
                            _NextScanFileOffset - _FirstOffsetScannedInRegion,
                            _CountOfValidRecordsScanned,
                            wrapMsg);

                        RecordValidScannedRegion();
                    }

                    // Mark start of invalid region
                    _SurfaceScanState = ScanningInvalidRegion;
                    _FirstOffsetScannedInRegion = _NextScanFileOffset;
                }
            }
            else
            {
                // Some error other than one expected
                RecordReaderFailed(status, _NextScanFileOffset, __LINE__);
                Complete(status);
                return;
            }

            // Scan next block for a valid record
            _NextScanFileOffset += RvdDiskLogConstants::BlockSize;
            ContinueSurfaceScan();
            return;
        }

        KAssert(_IoBuffer0->QueryNumberOfIoBufferElements() == 1);

        // At this point the header + metadata has been validated - determine what kind of record we have
        RvdLogRecordHeader*             recordHeader = (RvdLogRecordHeader*)(_IoBuffer0->First()->GetBuffer());
        ULONG                           recordSize = recordHeader->ThisHeaderSize + recordHeader->IoBufferSize;

        // Physically valid record header and metadata read and located correctly at _NextScanFileOffset
        if (_SurfaceScanState != ScanningValidRegion)
        {
            // Found a new valid region start
            if (_SurfaceScanState == ScanningInvalidRegion)
            {
                // Was in an invalid region - mark its end
                KDbgPrintf(
                    "ReportInvalidRegion: OffsetRange:[%I64u, %I64u]; Size: %I64u\n",
                    _FirstOffsetScannedInRegion,
                    _NextScanFileOffset - 1,
                    _NextScanFileOffset - _FirstOffsetScannedInRegion);

                RecordInvalidScannedRegion();
            }

            // Mark the start of a new valid region
            _SurfaceScanState = ScanningValidRegion;
            _FirstOffsetScannedInRegion = _NextScanFileOffset;
            _FirstValidLsnScannedInRegion = recordHeader->Lsn;
            _CountOfValidRecordsScanned = 0;
        }
        else if (recordHeader->Lsn.Get() != (_LastValidLsnScanned.Get() + _LastValidLsnSizeScanned))
        {
            // Break in LSN sequence within a valid region
            // Mark its end
            CHAR*      wrapMsg = ((_LastValidLsnOffsetScanned + _LastValidLsnSizeScanned - 1) >= _LogFileLsnSpace)
                                    ? "; WrappedEOF" : "";
            KDbgPrintf(
                "ReportValidRegion: OffsetRange:[%I64u, %I64u]; LSN Range: [%I64u, %I64u]; Size: %I64u; Records: %u%s\n",
                _FirstOffsetScannedInRegion,
                _NextScanFileOffset - 1,
                _FirstValidLsnScannedInRegion.Get(),
                _LastValidLsnScanned.Get() + _LastValidLsnSizeScanned - 1,
                _NextScanFileOffset - _FirstOffsetScannedInRegion,
                _CountOfValidRecordsScanned,
                wrapMsg);

            RecordValidScannedRegion();

            // Mark the start of a new valid region
            _FirstOffsetScannedInRegion = _NextScanFileOffset;
            _FirstValidLsnScannedInRegion = recordHeader->Lsn;
            _CountOfValidRecordsScanned = 0;
        }

        _LastValidLsnScanned = recordHeader->Lsn;
        _LastValidLsnOffsetScanned = _NextScanFileOffset;
        _LastValidLsnSizeScanned = recordSize;
        _CountOfValidRecordsScanned++;

        BOOLEAN                         isCpRecord = recordHeader->LogStreamId == RvdDiskLogConstants::CheckpointStreamId();
        RvdLogPhysicalCheckpointRecord* physicalCpRecord = isCpRecord ? (RvdLogPhysicalCheckpointRecord*)recordHeader : nullptr;
        RvdLogUserStreamRecordHeader*   userStreamRecord = isCpRecord ? nullptr :  (RvdLogUserStreamRecordHeader*)recordHeader;
        RvdLogUserRecordHeader*         userRecord = (!isCpRecord && (userStreamRecord->RecordType == RvdLogUserStreamRecordHeader::Type::UserRecord))
                                                        ?  (RvdLogUserRecordHeader*)recordHeader
                                                        :  nullptr;
        RvdLogUserStreamCheckPointRecordHeader* streamCpRecord;
        streamCpRecord = (!isCpRecord && (userStreamRecord->RecordType == RvdLogUserStreamRecordHeader::Type::CheckPointRecord))
                            ?  (RvdLogUserStreamCheckPointRecordHeader*)recordHeader
                            :  nullptr;


        if ((userStreamRecord != nullptr) && (userRecord == nullptr) && (streamCpRecord == nullptr))
        {
            // ADD: ErrorReport - bad record type.
            _NextScanFileOffset += RvdDiskLogConstants::BlockSize;
            ContinueSurfaceScan();
            return;
        }

        if (((physicalCpRecord != nullptr) || (streamCpRecord != nullptr)) && (recordHeader->IoBufferSize > 0))
        {
            // ADD: ErrorReport - unexpected IoBufferRecordSection
            _NextScanFileOffset += RvdDiskLogConstants::BlockSize;
            ContinueSurfaceScan();
            return;
        }

        if (userRecord != nullptr)
        {
            // Finish validating user record
            BOOLEAN isValid = ValidateUserRecord(
                userRecord->Asn,
                userRecord->AsnVersion,
                &userRecord->UserMetaData[0],
                userRecord->MetaDataSize - (sizeof(RvdLogUserRecordHeader) - sizeof(RvdLogRecordHeader)),
                ((UCHAR*)userRecord) + userRecord->ThisHeaderSize,
                recordHeader->IoBufferSize);

            if (!isValid)
            {
                // ADD??: ErrorReport - invalid user record
                KDbgPrintf(
                    "ReportInvalidUserRecord: Offset: %I64u; LSN: %I64u; IoBuffer region failed validation\n",
                    _NextScanFileOffset,
                    recordHeader->Lsn.Get());
            }

            // ADD: ReportValidUserRecord @
        }

        if (streamCpRecord != nullptr)
        {
            // ADD: ReportValidStreamCpRecord @ ... StreamId
            //KDbgPrintf("ReportValidStreamCpRecord @Offset: %I64u; LSN: %I64u\n", _NextScanFileOffset, streamCpRecord->Lsn.Get());
        }

        if (physicalCpRecord != nullptr)
        {
            // ADD: ReportValidCpRecord @
            //KDbgPrintf("ReportValidCpRecord @Offset: %I64u; LSN: %I64u\n", _NextScanFileOffset, physicalCpRecord->Lsn.Get());
        }

        // Common LSN space accounting:
        _HighestLsn.SetIfLarger(recordHeader->Lsn);
        _NextLsnToWrite.SetIfLarger(recordHeader->Lsn.Get() + recordSize);
        _HighestCompletedLsn.SetIfLarger(recordHeader->HighestCompletedLsn);
        _HighestCheckpointLsn.SetIfLarger(recordHeader->LastCheckPointLsn);
        //??RvdLogLsn                           _LowestLsn;                 // All physical space mapped below this may be reclaimed

        // Next record
        _NextScanFileOffset += recordSize;
        ContinueSurfaceScan();
    }
#endif
