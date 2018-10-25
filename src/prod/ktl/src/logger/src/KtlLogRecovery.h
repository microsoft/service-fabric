/*++

    (c) 2011 by Microsoft Corp. All Rights Reserved.

    RvdLog and RvdLogStream Recovery defintions

    History:
      PengLi/richhas        4-Sep-2011         Initial version.

--*/


#pragma once
#include "InternalKtlLogger.h"

//** Common base class for both Log file-level and stream-level derivation classes
//
class RvdLogRecoveryBase : public KAsyncContextBase
{
    // BUG, richhas, xxxxxx, Add Raw surface scan structure verify.
    K_FORCE_SHARED_WITH_INHERITANCE(RvdLogRecoveryBase);

protected:
    // Derivation support interface

    // Initialize common state
    //
    // Returns: STATUS_SUCCESS if ok; else error state is traced thru failure notification interface
    NTSTATUS
    Init();

    VOID
    DoComplete(__in NTSTATUS Status);

    // Derivation interface
    virtual NTSTATUS
    ValidateUserRecord(
        RvdLogAsn UserRecordAsn,
        ULONGLONG UserRecordAsnVersion,
        void* UserMetadata,
        ULONG UserMetadataSize,
        RvdLogStreamType& LogStreamType,
        KIoBuffer::SPtr& UserIoBuffer) = 0;

    // Forward define a common log record reader utility class
    class RecordReader;

    //** Recovery/Validate failure notification interface
    virtual VOID
    MemoryAllocationFailed(ULONG LineNumber)
    {
        UNREFERENCED_PARAMETER(LineNumber);
    }

    virtual VOID
    AllocateTransferContextFailed(NTSTATUS Status, KBlockDevice::SPtr& BlockDevice, ULONG LineNumber)
    {
        UNREFERENCED_PARAMETER(Status);
        UNREFERENCED_PARAMETER(BlockDevice);
        UNREFERENCED_PARAMETER(LineNumber);
    }

    virtual VOID
    PhysicalStartReadFailed(
        NTSTATUS Status, 
        KBlockDevice::SPtr& BlockDevice, 
        ULONGLONG FileOffset, 
        ULONG ReadSize, 
        ULONG LineNumber)
    {
        UNREFERENCED_PARAMETER(LineNumber);
        UNREFERENCED_PARAMETER(ReadSize);
        UNREFERENCED_PARAMETER(FileOffset);
        UNREFERENCED_PARAMETER(BlockDevice);
        UNREFERENCED_PARAMETER(Status);
    }

    virtual VOID
    PhysicalReadFailed(
        NTSTATUS Status, 
        KBlockDevice::SPtr& BlockDevice, 
        ULONGLONG FileOffset, 
        ULONG ReadSize, 
        ULONG LineNumber)
    {
        UNREFERENCED_PARAMETER(LineNumber);
        UNREFERENCED_PARAMETER(ReadSize);
        UNREFERENCED_PARAMETER(FileOffset);
        UNREFERENCED_PARAMETER(BlockDevice);
        UNREFERENCED_PARAMETER(Status);
    }

    virtual VOID
    LogRecordReaderCreationFailed(NTSTATUS Status, ULONG Line)
    {
        UNREFERENCED_PARAMETER(Status);
        UNREFERENCED_PARAMETER(Line);
    }

    virtual VOID
    RecordReaderFailed(NTSTATUS Status, RvdLogLsn ReadAt, ULONG LineNumber)
    {
        UNREFERENCED_PARAMETER(Status);
        UNREFERENCED_PARAMETER(ReadAt);
        UNREFERENCED_PARAMETER(LineNumber);
    }

    virtual VOID
    RecordReaderFailed(NTSTATUS Status, ULONGLONG ReadAtOffset, ULONG LineNumber)
    {
        UNREFERENCED_PARAMETER(Status);
        UNREFERENCED_PARAMETER(ReadAtOffset);
        UNREFERENCED_PARAMETER(LineNumber);
    }

    virtual VOID
    UserRecordHeaderFault(RvdLogUserStreamRecordHeader& UserStreamRecordHeader, ULONG LineNumber)
    {
        UNREFERENCED_PARAMETER(UserStreamRecordHeader);
        UNREFERENCED_PARAMETER(LineNumber);
    }

protected:
    KBlockDevice::SPtr                  _BlockDevice;
        KAsyncContextBase::SPtr         _BlockDeviceNonContiguousReadOp;
        KAsyncContextBase::SPtr         _BlockDeviceReadOp0;
        KAsyncContextBase::SPtr         _BlockDeviceReadOp1;
    RvdLogId                            _LogId;
    KIoBufferView::SPtr                 _IoBufferView;
};

//** Log file recovery class
//
// This class performs the verification and recovery on a physical log file. This process, if successful,
// results in capturing the state of the physical log contents (free space, lowest and highest LSNs...), 
// including the extend of each resident stream up to the latest physical checkpoint written into the log.
// This recovered state provides enough information to re-open a log file and to support the stream-level
// recovery and verification logic provided the RvdLogStreamRecovery class.
//
// The process of recovery is started via the StartRecovery() method and once that async operation completes
// successfully, the SnapLogState() method can be used to retrieve the recovered physical log file state; 
// support the stream-level of revovery, verification.
//
class RvdLogRecovery : public RvdLogRecoveryBase
{
    // BUG, richhas, xxxxxx, Add Raw surface scan structure verify.
    K_FORCE_SHARED_WITH_INHERITANCE(RvdLogRecovery);

public:
    static const ULONG _MaxNonContiguousReadBuffers = 64;
    static const ULONG _MinNonContiguousReadBufferLength = 256 * 1024;
    
    
    VOID
    //* Start the physical log recovery.
    //
    // Parameters:
    //      BlockDevice         -   Device I/O abstraction thru which the log file contents will be read.
    //      LogId               -   Unique ID of the log that is to be recovered.
    //      
    StartRecovery(
        __in KBlockDevice::SPtr BlockDevice,
        __in RvdLogId& LogId,
        __in DWORD CreationFlags,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

    NTSTATUS
    //* Return the recovered LogState from the latest StartRecovery() operation.
    //
    //  Returns:    !NT_SUCCESS     - LogState creation failure - see LogState::Create()
    //              Result          - The recovered state of the physical log file.
    //
    SnapLogState(__out LogState::SPtr& Result);

protected:
    // Derivation interface

    //** Recovery/Validate failure notification interface
    virtual VOID
    AllocatedFileTooSmall(ULONGLONG FileSize, ULONGLONG MinFileSizeAllowed)
    {
        UNREFERENCED_PARAMETER(FileSize);
        UNREFERENCED_PARAMETER(MinFileSizeAllowed);
    }

    virtual VOID
    AllocatedFileLengthWrong(ULONGLONG FileSize, ULONGLONG ChunkSize)
    {
        UNREFERENCED_PARAMETER(FileSize);
        UNREFERENCED_PARAMETER(ChunkSize);
    }

    virtual VOID
    FirstMasterBlockValidationFailed(NTSTATUS Status, KBlockDevice::SPtr& BlockDevice, RvdLogMasterBlock& MasterBlk)
    {
        UNREFERENCED_PARAMETER(Status);
        UNREFERENCED_PARAMETER(BlockDevice);
        UNREFERENCED_PARAMETER(MasterBlk);
    }

    virtual VOID
    SecondMasterBlockValidationFailed(NTSTATUS Status, KBlockDevice::SPtr& BlockDevice, RvdLogMasterBlock& MasterBlk)
    {
        UNREFERENCED_PARAMETER(Status);
        UNREFERENCED_PARAMETER(BlockDevice);
        UNREFERENCED_PARAMETER(MasterBlk);
    }

    virtual VOID
    SplitReadFailed(NTSTATUS Status)
    {
        UNREFERENCED_PARAMETER(Status);
    }

    virtual VOID
    KIoBufferElementCreationFailed(NTSTATUS Status, ULONG LineNumber)
    {
        UNREFERENCED_PARAMETER(Status);
        UNREFERENCED_PARAMETER(LineNumber);
    }

    virtual VOID
    MultipartReadFailed(NTSTATUS Status, ULONG LineNumber)
    {
        UNREFERENCED_PARAMETER(Status);
        UNREFERENCED_PARAMETER(LineNumber);
    }

    virtual VOID
    LsnOrderingFailed(
        ULONGLONG RecordFileOffset, 
        RvdLogLsn RecordLsn, 
        RvdLogLsn RecordNextLsn, 
        RvdLogLsn ExpectedNextLsn, 
        ULONG LineNumber)
    {
        UNREFERENCED_PARAMETER(LineNumber);
        UNREFERENCED_PARAMETER(ExpectedNextLsn);
        UNREFERENCED_PARAMETER(RecordNextLsn);
        UNREFERENCED_PARAMETER(RecordLsn);
        UNREFERENCED_PARAMETER(RecordFileOffset);
    }

    virtual VOID
    HighestCompletedLsnNotCoherentWithHighestLsn(RvdLogLsn HighestCompletedLsn, RvdLogLsn HighestLsn)
    {
        UNREFERENCED_PARAMETER(HighestLsn);
        UNREFERENCED_PARAMETER(HighestCompletedLsn);
    }

    virtual VOID
    PhysicalCheckpointRecordIoBufferSizeNotZero(RvdLogLsn ReadAt, ULONG LineNumber)
    {
        UNREFERENCED_PARAMETER(LineNumber);
        UNREFERENCED_PARAMETER(ReadAt);
    }

    virtual VOID
    ValidateUserRecordFailed(RvdLogRecordHeader& RecordHeader, ULONG LineNumber)
    {
        UNREFERENCED_PARAMETER(LineNumber);
        UNREFERENCED_PARAMETER(RecordHeader);
    }

    virtual VOID
    HighestCheckpointLsnNotFound()
    {
    }

    virtual VOID
    PhysicalCheckpointRecordFault(RvdLogPhysicalCheckpointRecord& Record, ULONG LineNumber)
    {
        UNREFERENCED_PARAMETER(LineNumber);
        UNREFERENCED_PARAMETER(Record);
    }

    virtual VOID
    InconsistentStreamInfoFound(RvdLogStreamInformation& CurrentInfo, RvdLogPhysicalCheckpointRecord& CpRecord)
    {
        UNREFERENCED_PARAMETER(CpRecord);
        UNREFERENCED_PARAMETER(CurrentInfo);
    }

    virtual VOID
    CheckpointStreamInfoMissingFromCheckpointRecord(RvdLogPhysicalCheckpointRecord& CpRecord)
    {
        UNREFERENCED_PARAMETER(CpRecord);
    }

    virtual VOID
    LowestLogLsnNotEstablishedInCheckpointRecord(RvdLogPhysicalCheckpointRecord& CpRecord)
    {
        UNREFERENCED_PARAMETER(CpRecord);
    }

    virtual VOID
    StreamMissingInValidRecord(RvdLogRecordHeader& RecordHeader)
    {
        UNREFERENCED_PARAMETER(RecordHeader);
    }

    virtual VOID
    InvalidStreamTypeInValidRecord(RvdLogRecordHeader& RecordHeader)
    {
        UNREFERENCED_PARAMETER(RecordHeader);
    }

    virtual VOID
    InvalidStreamLimitsInValidRecord(RvdLogRecordHeader& RecordHeader, RvdLogStreamInformation& StreamInfo)
    {
        UNREFERENCED_PARAMETER(StreamInfo);
        UNREFERENCED_PARAMETER(RecordHeader);
    }


    //** Recovery/Validate progress notification interface
    virtual VOID
    ReportFileSizeLayout(
        KBlockDevice& BlockDevice,
        ULONGLONG FileSize,
        ULONG ChunkSize,
        ULONGLONG MaxFreeSpace,
        ULONGLONG MaxCheckPointLogRecordSize)
    {
        UNREFERENCED_PARAMETER(MaxCheckPointLogRecordSize);
        UNREFERENCED_PARAMETER(MaxFreeSpace);
        UNREFERENCED_PARAMETER(ChunkSize);
        UNREFERENCED_PARAMETER(FileSize);
        UNREFERENCED_PARAMETER(BlockDevice);
    }

    virtual VOID
    ReportFirstMasterBlockCorrect(RvdLogMasterBlock& MasterBlk)
    {
        UNREFERENCED_PARAMETER(MasterBlk);
    }

    virtual VOID
    ReportSecondMasterBlockCorrect(RvdLogMasterBlock& MasterBlk)
    {
        UNREFERENCED_PARAMETER(MasterBlk);
    }

    virtual VOID
    ReportSearchLowChunck(
        BOOLEAN IsContinuedSplitRead,
        ULONGLONG LowSearchChunkNumber,
        ULONG CurrentChunkOffset,
        ULONG ChunkSizeRead,
        BOOLEAN FoundValidRecord,
        BOOLEAN FoundHole)
    {
        UNREFERENCED_PARAMETER(FoundHole);
        UNREFERENCED_PARAMETER(FoundValidRecord);
        UNREFERENCED_PARAMETER(ChunkSizeRead);
        UNREFERENCED_PARAMETER(CurrentChunkOffset);
        UNREFERENCED_PARAMETER(LowSearchChunkNumber);
        UNREFERENCED_PARAMETER(IsContinuedSplitRead);
    }

    virtual VOID
    ReportPhysicallyCorrectRecordHeader(ULONGLONG CurrentFileOffset, RvdLogRecordHeader& RecordHeader, ULONG LineNumber)
    {
        UNREFERENCED_PARAMETER(LineNumber);
        UNREFERENCED_PARAMETER(RecordHeader);
        UNREFERENCED_PARAMETER(CurrentFileOffset);
    }

    virtual VOID
    ReportPhysicallyCorrectAndPlacedRecordHeader(
        ULONGLONG CurrentFileOffset,
        RvdLogRecordHeader& RecordHeader,
        ULONG LineNumber,
        RvdLogLsn HighestCheckpointLsn)
    {
        UNREFERENCED_PARAMETER(HighestCheckpointLsn);
        UNREFERENCED_PARAMETER(LineNumber);
        UNREFERENCED_PARAMETER(RecordHeader);
        UNREFERENCED_PARAMETER(CurrentFileOffset);
    }

    virtual VOID
    ReportOldRecordHeader(ULONGLONG CurrentFileOffset, RvdLogRecordHeader& RecordHeader, ULONG LineNumber)
    {
        UNREFERENCED_PARAMETER(LineNumber);
        UNREFERENCED_PARAMETER(RecordHeader);
        UNREFERENCED_PARAMETER(CurrentFileOffset);
    }

    virtual VOID
    ReportHoleFoundInChunk(ULONGLONG CurrentFileOffset, ULONGLONG LowSearchChunkNumber, ULONG LineNumber)
    {
        UNREFERENCED_PARAMETER(LineNumber);
        UNREFERENCED_PARAMETER(LowSearchChunkNumber);
        UNREFERENCED_PARAMETER(CurrentFileOffset);
    }

    virtual VOID
    ReportEmptyLogFound(ULONG LineNumber)
    {
        UNREFERENCED_PARAMETER(LineNumber);
    }

    virtual VOID
    ReportHighestLsnRecordHeaderChunkFound(
        ULONGLONG Chunk,
        ULONGLONG FileOffset,
        RvdLogLsn HighestLsn,
        ULONGLONG HighLsnRecordSize,
        LONGLONG SizeOfSpacePreceedingHighLsn)
    {
        UNREFERENCED_PARAMETER(SizeOfSpacePreceedingHighLsn);
        UNREFERENCED_PARAMETER(HighLsnRecordSize);
        UNREFERENCED_PARAMETER(HighestLsn);
        UNREFERENCED_PARAMETER(FileOffset);
        UNREFERENCED_PARAMETER(Chunk);
    }

    virtual VOID
    ReportSearchingForFinalHighLsn(ULONGLONG FileOffset, ULONG RegionSize, BOOLEAN IsWrappedSpace)
    {
        UNREFERENCED_PARAMETER(IsWrappedSpace);
        UNREFERENCED_PARAMETER(RegionSize);
        UNREFERENCED_PARAMETER(FileOffset);
    }

    virtual VOID
    ReportHoleFoundSearchingForFinalHighLsn(ULONGLONG CurrentFileOffset, ULONG LineNumber)
    {
        UNREFERENCED_PARAMETER(LineNumber);
        UNREFERENCED_PARAMETER(CurrentFileOffset);
    }

    virtual VOID
    ReportLogHeadRegion(
        RvdLogLsn NextLsnToWrite,
        RvdLogLsn HighestLsn,
        RvdLogLsn HighestCompletedLsn,
        RvdLogLsn HighestCheckpointLsn)
    {
        UNREFERENCED_PARAMETER(HighestCheckpointLsn);
        UNREFERENCED_PARAMETER(HighestCompletedLsn);
        UNREFERENCED_PARAMETER(HighestLsn);
        UNREFERENCED_PARAMETER(NextLsnToWrite);
    }

    virtual VOID
    ReportLowestHeadLsnCrcError(RvdLogLsn LowestHeadLsn)
    {
        UNREFERENCED_PARAMETER(LowestHeadLsn);
    }

    virtual VOID
    ReportPhysicalCheckpointRecordFound(RvdLogLsn ReadAt, ULONG LineNumber)
    {
        UNREFERENCED_PARAMETER(LineNumber);
        UNREFERENCED_PARAMETER(ReadAt);
    }

    virtual VOID
    ReportHighestLsnsEstablished(RvdLogLsn NextLsnToWrite, RvdLogLsn HighestCompletedLsn, RvdLogLsn HighestCheckpointLsn)
    {
        UNREFERENCED_PARAMETER(HighestCheckpointLsn);
        UNREFERENCED_PARAMETER(HighestCompletedLsn);
        UNREFERENCED_PARAMETER(NextLsnToWrite);
    }

    virtual VOID
    ReportHighestPhysicalCheckpoint(RvdLogPhysicalCheckpointRecord& Record)
    {
        UNREFERENCED_PARAMETER(Record);
    }

    virtual VOID
    ReportRecoveredStreamInfoInCheckpointRecord(ULONG StreamIx, RvdLogStreamInformation& CurrentInfo)
    {
        UNREFERENCED_PARAMETER(CurrentInfo);
        UNREFERENCED_PARAMETER(StreamIx);
    }

    virtual VOID
    ReportPhysicalCheckpointStreamLimits(RvdLogLsn LowestLsn, RvdLogLsn HighestLsn, RvdLogLsn NextLsn)
    {
        UNREFERENCED_PARAMETER(NextLsn);
        UNREFERENCED_PARAMETER(HighestLsn);
        UNREFERENCED_PARAMETER(LowestLsn);
    }

    virtual VOID
    ReportLowestLogLsnFound(RvdLogLsn LowestLsn)
    {
        UNREFERENCED_PARAMETER(LowestLsn);
    }

    virtual VOID
    ReportAllStreamLimitsRecovered(ULONG NumberOfStreams, RvdLogStreamInformation& StreamInfoArray)
    {
        UNREFERENCED_PARAMETER(StreamInfoArray);
        UNREFERENCED_PARAMETER(NumberOfStreams);
    }

    virtual VOID
    ReportPhysicalLogRecovered(
        ULONGLONG Freespace,
        RvdLogLsn LowestLsn,
        RvdLogLsn NextLsnToWrite,
        RvdLogLsn HighestCompletedLsn,
        RvdLogLsn HighestCheckpointLsn)
    {
        UNREFERENCED_PARAMETER(HighestCheckpointLsn);
        UNREFERENCED_PARAMETER(HighestCompletedLsn);
        UNREFERENCED_PARAMETER(NextLsnToWrite);
        UNREFERENCED_PARAMETER(LowestLsn);
        UNREFERENCED_PARAMETER(Freespace);
    }

    virtual VOID
    ReportRecordHeaderApprearsValid(RvdLogRecordHeader& RecordHeader, ULONGLONG CurrentFileOffset)
    {
        UNREFERENCED_PARAMETER(CurrentFileOffset);
        UNREFERENCED_PARAMETER(RecordHeader);
    }

    virtual VOID
    ReportRecordHeaderIsInvalid(RvdLogRecordHeader& RecordHeader, ULONGLONG CurrentFileOffset)
    {
        UNREFERENCED_PARAMETER(CurrentFileOffset);
        UNREFERENCED_PARAMETER(RecordHeader);
    }

private:
    VOID
    OnStart();

    VOID
    FirstMasterBlockReadComplete(__in_opt KAsyncContextBase* const Parent, __in KAsyncContextBase& CompletedContext);

    VOID
    SecondMasterBlockReadComplete(__in_opt KAsyncContextBase* const Parent, __in KAsyncContextBase& CompletedContext);

    BOOLEAN HasValidRecordHeader(
        __in ULONGLONG ChunckNumber,
        __in KIoBuffer& Chunck);
    
    VOID
    FirstChunckReadComplete(__in_opt KAsyncContextBase* const Parent, __in KAsyncContextBase& CompletedContext);

    VOID
    LastChunckReadComplete(__in_opt KAsyncContextBase* const Parent, __in KAsyncContextBase& CompletedContext);

    VOID
    GetAllocatedRangesComplete(__in_opt KAsyncContextBase* const Parent, __in KAsyncContextBase& CompletedContext);

    VOID
    ChunckReadComplete(__in_opt KAsyncContextBase* const Parent, __in KAsyncContextBase& CompletedContext);

    NTSTATUS
    StartNextChunckRead();

    VOID
    DetermineChuncksComplete(
        __in ULONGLONG LowSearchChunckNumber,
        __in ULONGLONG LogFileNumberOfChuncks);

    VOID
    ReadLowChunck();

    VOID
    ReadLowChunckComplete(__in_opt KAsyncContextBase* const Parent, __in KAsyncContextBase& CompletedContext);

    VOID
    SplitRecordReadComplete(__in_opt KAsyncContextBase* const Parent, __in KAsyncContextBase& CompletedContext);

    VOID
    SearchLowChunckRead(__in BOOLEAN IsContinuedSplitRead);

    NTSTATUS GetCurrentChunkRecordAndMetadata(
        __in KIoBuffer& ChunkIoBuffer,
        __in KIoBufferElement& ChunkIoBufferElementHint,
        __in ULONG ChunkIoBufferPositionHint,
        __in ULONG ChunkOffset,
        __inout RvdLogRecordHeader*& RecordHeaderCopy,
        __out KBuffer::SPtr& ChunkRecordHeaderAndMetadataBuffer,
        __out ULONG& ChunkRecordHeaderAndMetadataBufferSize
        );

    RvdLogRecordHeader* GetCurrentChunkRecordHeader(
        __in KIoBuffer& ChunkIoBuffer,
        __in ULONG ChunkOffset,
        __in ULONG SkipOffset,
        __inout KIoBufferElement::SPtr& ChunkIoBufferElementHint,
        __inout ULONG& ChunkIoBufferPositionHint,
        __inout ULONG& ChunkIoBufferSize,
        __inout ULONG& InElementOffset,
        __inout PUCHAR& InElementPointer
        );
        
    VOID
    SearchFinalHighLsnReadComplete(__in_opt KAsyncContextBase* const Parent, __in KAsyncContextBase& CompletedContext);

    VOID
    ValidateLogHeadRecords();

    VOID
    StartLowestHeadRecordRead();

    VOID
    LowestHeadRecordReadComplete(__in_opt KAsyncContextBase* const Parent, __in KAsyncContextBase& CompletedContext);

    VOID
    ReadLastCheckpoint();

    VOID
    ReadLastCheckpointComplete(__in_opt KAsyncContextBase* const Parent, __in KAsyncContextBase& CompletedContext);

    VOID
    ReadNextCompletedLsn();

    VOID
    ReadNextCompletedLsnCallback(__in_opt KAsyncContextBase* const Parent, __in KAsyncContextBase& CompletedContext);

    VOID
    AllStreamLimitsRecovered();

    //** Support functions
    VOID
    StartMultipartRead(__in RvdLogLsn LsnToRead, __in ULONG SizeToRead, __in KAsyncContextBase::CompletionCallback Callback);

    BOOLEAN
    RecordHeaderAppearsValid(
        __in RvdLogRecordHeader* const RecordHeader,
        __in ULONGLONG const CurrentFileOffset,
        __in_opt RvdLogLsn* const LsnToValidate = nullptr);

    VOID
    CompleteWithEmptyLogIndication();

    ULONGLONG __inline
    ToFileOffsetFromChunk(ULONGLONG ChunkNumber)
    {
        return (ChunkNumber * _Config.GetMaxQueuedWriteDepth()) + RvdDiskLogConstants::BlockSize;
    }

private:
    KBlockDevice::DeviceAttributes      _DevAttrs;
    BOOLEAN                             _FirstMasterBlockIsValid;
    ULONG                               _LogSignature[RvdDiskLogConstants::SignatureULongs];
    KSharedPtr<RecordReader>            _RecordReader;
    NTSTATUS volatile                   _ValidationProcessStatus;
    KIoBuffer::SPtr                     _IoBuffer0;
    KIoBuffer::SPtr                     _IoBuffer1;
    KIoBufferElement::SPtr              _SplitReadBuffer;
    LONG volatile                       _ReadOpCount;
    ULONGLONG                           _FileOffset;
    ULONGLONG                           _FileOffset1;
    ULONG                               _MultipartReadSize;
    BOOLEAN                             _DidMultipartRead;
    RvdLogLsn                           _MultipartReadLsn;
	BOOLEAN                             _IsSparseFile;

    // State used to scan log head [_HighestCompletedLsn, _HighestLsn]
    RvdLogLsn                           _LowestHeadLsn;
    RvdLogLsn                           _HighestHeadLsn;

    // State used to save and continue SearchLowChunckRead() after a split read completes
    ULONG                               _SavedCurrentChunkOffset;
    BOOLEAN                             _SavedFoundValidRecord;
    BOOLEAN                             _SavedFoundHole;
    ULONGLONG                           _LogFileNumberOfChuncks;
    ULONGLONG                           _LowSearchChunkNumber;
    ULONGLONG                           _MaxNumberOfSearchChunks;
    ULONGLONG                           _SearchSpaceStep;
    ULONGLONG                           _HighestLsnChunkNumber;

    // Recovered Log File State
    ULONGLONG                           _LogFileSize;
    ULONGLONG                           _LogFileLsnSpace;           // total available LSN-space within _LogFileSize
    ULONGLONG                           _MaxCheckPointLogRecordSize;
    ULONGLONG                           _LogFileFreeSpace;
    RvdLogLsn                           _LowestLsn;
    RvdLogLsn                           _HighestLsn;
    RvdLogLsn                           _NextLsnToWrite;
    RvdLogLsn                           _HighestCompletedLsn;
    RvdLogLsn                           _HighestCheckpointLsn;
    ULONG                               _NumberOfStreams;
    RvdLogConfig                        _Config;
    KArray<RvdLogStreamInformation>     _StreamInfos;
    WCHAR                               _CreationDirectory[RvdDiskLogConstants::MaxCreationPathSize];
    WCHAR                               _LogType[RvdLogManager::AsyncCreateLog::MaxLogTypeLength];

    //
    // Sparse file recovery
    //
    DWORD                               _CreationFlags;
    KArray<KBlockFile::AllocatedRange>  _AllocatedRanges;
    ULONG                               _AllocatedRangeIndex;
    ULONG                               _ChunckIndexInRange;
    ULONG                               _CurrentChunckIndex;

    ULONG                               _LowChunkIndex;
    ULONG                               _HighChunkIndex;

    ULONG                               _ReadLength;
    enum { DetermineLowChunck, DetermineHighChunck }   _DetermineChunckState;

};


//** Log file stream recovery class
//
// This class performs the verification and recovery on a stream in a log file. This process, if successful,
// results in capturing the state of the log stream contents (e.g. all records, truncation point, last cp...). 
// This recovered state provides enough information to re-open a log stream.
//
// The process of recovery is started via the StartRecovery() method and once that async operation completes
// successfully, the UpdateSnappedLogState() method can be used to retrieve the recovered log stream state. 
//
class RvdLogStreamRecovery : public RvdLogRecoveryBase
{
    K_FORCE_SHARED_WITH_INHERITANCE(RvdLogStreamRecovery);

public:
    VOID
    //* Start the physical log recovery.
    //
    // Parameters:
    //      BlockDevice         -   Device I/O abstraction thru which the log file contents will be read.
    //      Config              -   The RvdLogConfig recovered from the containing log file.
    //      LogId               -   Unique ID of the log that is to be recovered.
    //      LogFileLsnSpace     -   Available LSN space in physical log.
    //      LogLowestLsn        -   Lowest LSN known to exits in the log file - no record can exist below this.
    //      LogNextLsnToWrite   -   Highest LSN+1 of the log file - no records can exist at or above this.
    //      LogSignature        -   Unique log file signature.
    //      ForStream           -   Initial stream extent information - typically recovered via RvdLogRecovery or
    //                              a previously opened stream in a running system.
    //      
    StartRecovery(
        __in KBlockDevice::SPtr BlockDevice,
        __in RvdLogConfig& Config,
        __in RvdLogId& LogId,
        __in ULONGLONG LogFileLsnSpace,
        __in RvdLogLsn LogLowestLsn,
        __in RvdLogLsn LogNextLsnToWrite,
        __in_ecount(NUMBER_SIGNATURE_ULONGS) ULONG LogSignature[RvdDiskLogConstants::SignatureULongs],
        __in RvdLogStreamInformation& ForStream,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

    VOID
    //* Return recovered stream state
    //
    GetRecoveredStreamInformation(
        __out RvdLogStreamInformation&  StreamInfo,
        __out RvdAsnIndex*& AsnIndex,
        __out RvdLogLsnEntryTracker*& LsnIndex,
        __out RvdLogAsn& TruncationPoint,
        __out RvdLogLsn& HighestCheckPointLsn);

protected:
    // Derivation interface
    //** Recovery/Validate failure notification interface
    virtual VOID
    InvalidMultiSegmentStreamCheckpointRecord(RvdLogUserStreamRecordHeader& RecordHeader, ULONG LineNumber)
    {
        UNREFERENCED_PARAMETER(LineNumber);
        UNREFERENCED_PARAMETER(RecordHeader);
    }

    virtual VOID
    InvalidStreamIdInValidRecord(RvdLogUserStreamRecordHeader& RecordHeader)
    {
        UNREFERENCED_PARAMETER(RecordHeader);
    }

    virtual VOID
    AddLowerLsnRecordIndexEntryFailed(NTSTATUS Status, RvdLogUserStreamRecordHeader& RecordHeader, ULONG LineNumber)
    {
        UNREFERENCED_PARAMETER(LineNumber);
        UNREFERENCED_PARAMETER(RecordHeader);
        UNREFERENCED_PARAMETER(Status);
    }

    virtual VOID
    AddAsnRecordIndexEntryFailed(NTSTATUS Status, RvdLogAsn RecordAsn, ULONGLONG RecordVersion, ULONG LineNumber)
    {
        UNREFERENCED_PARAMETER(LineNumber);
        UNREFERENCED_PARAMETER(RecordVersion);
        UNREFERENCED_PARAMETER(RecordAsn);
        UNREFERENCED_PARAMETER(Status);
    }

    virtual VOID
    LsnIndexQueryRecordFailed(NTSTATUS Status)
    {
        UNREFERENCED_PARAMETER(Status);
    }

    virtual VOID
    LsnIndexRecordMissing(RvdLogLsn Lsn)
    {
        UNREFERENCED_PARAMETER(Lsn);
    }

    virtual VOID
    ReportLogStreamRecovered(
        RvdLogStreamId& StreamId, 
        RvdLogStreamType& StreamType, 
        ULONG LsnCount, 
        RvdLogLsn LowestLsn, 
        RvdLogLsn HighestLsn, 
        RvdLogLsn NextLsn, 
        RvdLogLsn HighestStreamCheckPoint, 
        ULONG AsnCount, 
        RvdLogAsn LowestAsn, 
        RvdLogAsn HighestAsn, 
        RvdLogAsn TruncationAsn, 
        RvdLogLsn TruncationLsn)
    {
        UNREFERENCED_PARAMETER(TruncationLsn);
        UNREFERENCED_PARAMETER(TruncationAsn);
        UNREFERENCED_PARAMETER(HighestAsn);
        UNREFERENCED_PARAMETER(LowestAsn);
        UNREFERENCED_PARAMETER(AsnCount);
        UNREFERENCED_PARAMETER(HighestStreamCheckPoint);
        UNREFERENCED_PARAMETER(NextLsn);
        UNREFERENCED_PARAMETER(HighestLsn);
        UNREFERENCED_PARAMETER(LowestLsn);
        UNREFERENCED_PARAMETER(LsnCount);
        UNREFERENCED_PARAMETER(StreamType);
        UNREFERENCED_PARAMETER(StreamId);
    }

private:
    VOID
    OnStart();

    VOID
    ContinueRecoveryOfStream();
    
    VOID
    ReadStreamRecordComplete(KAsyncContextBase* const Parent, KAsyncContextBase& CompletedContext);

    VOID
    StreamRecoveryDone();

    NTSTATUS
    AddOrUpdateRvdAsnIndexEntry(
        RvdLogAsn Asn,
        ULONGLONG AsnVersion,
        ULONG IoBufferSize,
        RvdLogLsn Lsn);

protected:
    RvdLogStreamId const& GetStreamId()
    {
        return _StreamInfo.LogStreamId;
    }

private:
    RvdLogConfig*                       _Config;
    ULONGLONG                           _LogFileLsnSpace;
    RvdLogLsn                           _LogLowestLsn;
    RvdLogLsn                           _LogNextLsnToWrite;
    RvdLogStreamInformation             _StreamInfo;
    RvdLogLsn                           _StreamHighestCheckPointLsn;
    ULONG                               _LogSignature[RvdDiskLogConstants::SignatureULongs];
    KSharedPtr<RecordReader>            _RecordReader;
    RvdAsnIndex*                        _AsnIndex;
    RvdLogLsnEntryTracker*              _LsnIndex;
    RvdLogAsn                           _TruncationPoint;
    RvdLogLsn                           _NextStreamLsn;
    KIoBuffer::SPtr                     _IoBuffer0;
    ULONG                               _NumberOfCheckpointSegments;    // MAXULONG means 1st cp not read yet
    ULONG                               _NextExpectedCheckpointSegment;
};

//** Support types for log file recovery

//** Log File Record Reader class
//
//  This class reads the entire record and fully validates the RVD Log record header and metadata.
//  A successful read returns a KIoBuffer with a single element in it containing the entire record.
//  This is accomplished by via memory copies and thus may not perform well enough for all use cases.
//
class RvdLogRecoveryBase::RecordReader : public KAsyncContextBase
{
    K_FORCE_SHARED(RecordReader);

public:
    static NTSTATUS
    Create(
        __in KBlockDevice::SPtr FromBlockDevice,
        __in RvdLogConfig& Config,
        __in ULONGLONG const LogSpace,
        __in RvdLogId& LogId,
        __in ULONG (&LogSignature)[RvdDiskLogConstants::SignatureULongs],
        __out RecordReader::SPtr& Result,
        __in ULONG const AllocationTag,
        __in KAllocator& Allocator);

    VOID
    StartRead(
        __in RvdLogLsn& AtLsn,
        __out KIoBuffer::SPtr& IoBuffer,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

    VOID
    StartPhysicalRead(
        __in ULONGLONG AtOffset,
        __out KIoBuffer::SPtr& IoBuffer,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

private:
    RecordReader(
        __in KBlockDevice::SPtr FromBlockDevice,
        __in RvdLogConfig&      Config,
        __in ULONGLONG const    LogSpace,
        __in RvdLogId&          LogId,
        __in ULONG(&            LogSignature)[RvdDiskLogConstants::SignatureULongs]);

    VOID
    DoComplete(NTSTATUS Status);

    VOID
    OnStart();

    VOID
    HeaderReadComplete(__in_opt KAsyncContextBase* const Parent, __in KAsyncContextBase& CompletedContext);

    VOID
    RecordReadComplete(__in_opt KAsyncContextBase* const Parent, __in KAsyncContextBase& CompletedContext);

    VOID
    FinishRecordRead();

private:
    RvdLogConfig&           _Config;
    KBlockDevice::SPtr      _BlockDevice;
    ULONGLONG const         _LogSpace;
    RvdLogId&               _LogId;
    ULONG(&                 _LogSignature)[RvdDiskLogConstants::SignatureULongs];
    KAsyncContextBase::SPtr _ReadOp0;
    KAsyncContextBase::SPtr _ReadOp1;

    BOOLEAN                 _IsDoingLsnRead;
    ULONGLONG               _FileOffet;
    RvdLogLsn               _Lsn;
    KIoBuffer::SPtr*        _ResultingIoBuffer;

    ULONG                   _NumberOfReadsLeft;
    NTSTATUS                _StatusOfOtherOp;
    KIoBuffer::SPtr         _IoBuffer0;
    KIoBuffer::SPtr         _IoBuffer1;
    KIoBuffer::SPtr         _IoBuffer2;
    ULONG                   _TotalSizeToRead;
};
