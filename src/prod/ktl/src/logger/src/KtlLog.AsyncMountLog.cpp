/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    RvdLog.AsyncMountLog.cpp

    Description:
      RvdLogManagerImp::RvdOnDiskLog::AsyncMountLog implementation.

    History:
      PengLi        22-Oct-2010         Initial version.

--*/

#include "InternalKtlLogger.h"
#include "KtlLogRecovery.h"
#include <ktrace.h>


//** Concrete RvdLog file recovery state class
class RvdLogManagerImp::RvdOnDiskLog::AsyncMountLog::Recovery : public RvdLogRecovery
{
    K_FORCE_SHARED(Recovery);

public:
    Recovery(RvdLogManagerImp::RvdOnDiskLog::SPtr Parent)
        :   _Parent(Parent)
    {
    }

private:
    // Derivation interface overrides - all forward to _Parent
    NTSTATUS
    ValidateUserRecord(
        RvdLogAsn UserRecordAsn,
        ULONGLONG UserRecordAsnVersion,
        void* UserMetadata,
        ULONG UserMetadataSize,
        RvdLogStreamType& LogStreamType,
        KIoBuffer::SPtr& UserIoBuffer)
    {
        UNREFERENCED_PARAMETER(UserRecordAsn);
        UNREFERENCED_PARAMETER(UserRecordAsnVersion);
        
        if (!_LastVerificationCallback)
        {
            // Resolve a VerificationCallback for the passed RvdLogStreamType - caching the results
            _LastVerificationCallback = _Parent->_LogManager->QueryVerificationCallback(LogStreamType);
            _LastLogStreamType = LogStreamType;
        }

        if (_LastVerificationCallback && (_LastLogStreamType == LogStreamType))
        {
            return _LastVerificationCallback((UCHAR*)UserMetadata, UserMetadataSize, UserIoBuffer);
        }

        KDbgErrorWStatus(GetActivityId(), "Registered RvdLogManager::VerificationCallback Missing: ", STATUS_NOT_FOUND);
        return STATUS_NOT_FOUND;
    }

    //** Recovery/Validate failure notification interface
    VOID
    MemoryAllocationFailed(ULONG LineNumber) 
    {
        KTraceOutOfMemory(GetActivityId(), STATUS_INSUFFICIENT_RESOURCES, this, LineNumber, 0);
    }

    VOID
    AllocateTransferContextFailed(NTSTATUS Status, KBlockDevice::SPtr& BlockDevice, ULONG LineNumber)
    {
        UNREFERENCED_PARAMETER(BlockDevice);

        RvdLogUtility::AsciiGuid    logId(_LogId.Get());
        KDbgErrorWData(GetActivityId(), logId.Get(), Status, LineNumber, 0, 0, 0);
    }

    VOID
    PhysicalStartReadFailed(
        NTSTATUS Status, 
        KBlockDevice::SPtr& BlockDevice, 
        ULONGLONG FileOffset, 
        ULONG ReadSize, 
        ULONG LineNumber)
    {
        UNREFERENCED_PARAMETER(BlockDevice);

        RvdLogUtility::AsciiGuid    logId(_LogId.Get());
        KDbgErrorWData(GetActivityId(), logId.Get(), Status, LineNumber, FileOffset, ReadSize, 0);
    }

    VOID
    PhysicalReadFailed(
        NTSTATUS Status, 
        KBlockDevice::SPtr& BlockDevice, 
        ULONGLONG FileOffset, 
        ULONG ReadSize, 
        ULONG LineNumber)
    {
        UNREFERENCED_PARAMETER(BlockDevice);
        
        RvdLogUtility::AsciiGuid    logId(_LogId.Get());
        KDbgErrorWData(GetActivityId(), logId.Get(), Status, LineNumber, FileOffset, ReadSize, 0);
    }

    VOID
    LogRecordReaderCreationFailed(NTSTATUS Status, ULONG LineNumber)
    {
        RvdLogUtility::AsciiGuid    logId(_LogId.Get());
        KDbgErrorWData(GetActivityId(), logId.Get(), Status, LineNumber, 0, 0, 0);
    }

    VOID
    RecordReaderFailed(NTSTATUS Status, RvdLogLsn ReadAt, ULONG LineNumber)
    {
        RvdLogUtility::AsciiGuid    logId(_LogId.Get());
        KDbgErrorWData(GetActivityId(), logId.Get(), Status, LineNumber, ReadAt.Get(), 0, 0);
    }

    VOID
    RecordReaderFailed(NTSTATUS Status, ULONGLONG ReadAtOffset, ULONG LineNumber)
    {
        RvdLogUtility::AsciiGuid    logId(_LogId.Get());
        KDbgErrorWData(GetActivityId(), logId.Get(), Status, LineNumber, ReadAtOffset, 0, 0);
    }

    VOID
    UserRecordHeaderFault(RvdLogUserStreamRecordHeader& UserStreamRecordHeader, ULONG LineNumber)
    {
        RvdLogUtility::AsciiGuid    logId(_LogId.Get());
        KDbgErrorWData(
            GetActivityId(), 
            logId.Get(), 
            K_STATUS_LOG_STRUCTURE_FAULT, 
            LineNumber, 
            UserStreamRecordHeader.Lsn.Get(), 0, 0);
    }

    VOID
    AllocatedFileTooSmall(ULONGLONG FileSize, ULONGLONG MinFileSizeAllowed)
    {
        RvdLogUtility::AsciiGuid    logId(_LogId.Get());
        KDbgErrorWData(GetActivityId(), logId.Get(), K_STATUS_LOG_STRUCTURE_FAULT, 0, FileSize, MinFileSizeAllowed, 0);
    }

    VOID
    AllocatedFileLengthWrong(ULONGLONG FileSize, ULONGLONG ChunkSize)
    {
        RvdLogUtility::AsciiGuid    logId(_LogId.Get());
        KDbgErrorWData(GetActivityId(), logId.Get(), K_STATUS_LOG_STRUCTURE_FAULT, 0, FileSize, ChunkSize, 0);
    }

    VOID
    FirstMasterBlockValidationFailed(NTSTATUS Status, KBlockDevice::SPtr& BlockDevice, RvdLogMasterBlock& MasterBlk)
    {
        UNREFERENCED_PARAMETER(MasterBlk);
        UNREFERENCED_PARAMETER(BlockDevice);

        RvdLogUtility::AsciiGuid    logId(_LogId.Get());
        KDbgErrorWStatus(GetActivityId(), logId.Get(), Status);
    }

    VOID
    SecondMasterBlockValidationFailed(NTSTATUS Status, KBlockDevice::SPtr& BlockDevice, RvdLogMasterBlock& MasterBlk)
    {
        UNREFERENCED_PARAMETER(MasterBlk);
        UNREFERENCED_PARAMETER(BlockDevice);

        RvdLogUtility::AsciiGuid    logId(_LogId.Get());
        KDbgErrorWStatus(GetActivityId(), logId.Get(), Status);
    }

    VOID
    SplitReadFailed(NTSTATUS Status)
    {
        RvdLogUtility::AsciiGuid    logId(_LogId.Get());
        KDbgErrorWStatus(GetActivityId(), logId.Get(), Status);
    }

    VOID
    KIoBufferElementCreationFailed(NTSTATUS Status, ULONG LineNumber)
    {
        RvdLogUtility::AsciiGuid    logId(_LogId.Get());
        KDbgErrorWData(GetActivityId(), logId.Get(), Status, LineNumber, 0, 0, 0);
    }

    VOID
    MultipartReadFailed(NTSTATUS Status, ULONG LineNumber)
    {
        RvdLogUtility::AsciiGuid    logId(_LogId.Get());
        KDbgErrorWData(GetActivityId(), logId.Get(), Status, LineNumber, 0, 0, 0);
    }

    VOID
    LsnOrderingFailed(
        ULONGLONG RecordFileOffset, 
        RvdLogLsn RecordLsn, 
        RvdLogLsn RecordNextLsn, 
        RvdLogLsn ExpectedNextLsn, 
        ULONG LineNumber)
    {
        UNREFERENCED_PARAMETER(RecordLsn);

        RvdLogUtility::AsciiGuid    logId(_LogId.Get());
        KDbgErrorWData(
            GetActivityId(), 
            logId.Get(), 
            K_STATUS_LOG_STRUCTURE_FAULT, 
            LineNumber, 
            RecordFileOffset, 
            RecordNextLsn.Get(), 
            ExpectedNextLsn.Get());
    }

    VOID
    HighestCompletedLsnNotCoherentWithHighestLsn(RvdLogLsn HighestCompletedLsn, RvdLogLsn HighestLsn)
    {
        RvdLogUtility::AsciiGuid    logId(_LogId.Get());
        KDbgErrorWData(
            GetActivityId(), 
            logId.Get(), 
            K_STATUS_LOG_STRUCTURE_FAULT, 
            0, 
            HighestCompletedLsn.Get(), 
            HighestLsn.Get(), 
            0);
    }

    VOID
    PhysicalCheckpointRecordIoBufferSizeNotZero(RvdLogLsn ReadAt, ULONG LineNumber)
    {
        RvdLogUtility::AsciiGuid    logId(_LogId.Get());
        KDbgErrorWData(GetActivityId(), logId.Get(), K_STATUS_LOG_STRUCTURE_FAULT, LineNumber, ReadAt.Get(), 0, 0);
    }

    VOID
    ValidateUserRecordFailed(RvdLogRecordHeader& RecordHeader, ULONG LineNumber)
    {
        RvdLogUtility::AsciiGuid    logId(_LogId.Get());
        KDbgErrorWData(GetActivityId(), logId.Get(), K_STATUS_LOG_STRUCTURE_FAULT, LineNumber, RecordHeader.Lsn.Get(), 0, 0);
    }

    VOID
    HighestCheckpointLsnNotFound()
    {
        RvdLogUtility::AsciiGuid    logId(_LogId.Get());
        KDbgErrorWStatus(GetActivityId(), logId.Get(), K_STATUS_LOG_STRUCTURE_FAULT);
    }

    VOID
    PhysicalCheckpointRecordFault(RvdLogPhysicalCheckpointRecord& Record, ULONG LineNumber)
    {
        RvdLogUtility::AsciiGuid    logId(_LogId.Get());
        KDbgErrorWData(GetActivityId(), logId.Get(), K_STATUS_LOG_STRUCTURE_FAULT, LineNumber, Record.Lsn.Get(), 0, 0);
    }

    VOID
    InconsistentStreamInfoFound(RvdLogStreamInformation& CurrentInfo, RvdLogPhysicalCheckpointRecord& CpRecord)
    {
        RvdLogUtility::AsciiTwinGuid    logAndStreamId(_LogId.Get(), CurrentInfo.LogStreamId.Get());
        KDbgErrorWData(GetActivityId(), logAndStreamId.Get(), K_STATUS_LOG_STRUCTURE_FAULT, 0, CpRecord.Lsn.Get(), 0, 0);
    }

    VOID
    CheckpointStreamInfoMissingFromCheckpointRecord(RvdLogPhysicalCheckpointRecord& CpRecord)
    {
        RvdLogUtility::AsciiGuid    logId(_LogId.Get());
        KDbgErrorWData(GetActivityId(), logId.Get(), K_STATUS_LOG_STRUCTURE_FAULT, 0, CpRecord.Lsn.Get(), 0, 0);
    }

    VOID
    LowestLogLsnNotEstablishedInCheckpointRecord(RvdLogPhysicalCheckpointRecord& CpRecord)
    {
        RvdLogUtility::AsciiGuid    logId(_LogId.Get());
        KDbgErrorWData(GetActivityId(), logId.Get(), K_STATUS_LOG_STRUCTURE_FAULT, 0, CpRecord.Lsn.Get(), 0, 0);
    }

    VOID
    StreamMissingInValidRecord(RvdLogRecordHeader& RecordHeader)
    {
        RvdLogUtility::AsciiGuid    logId(_LogId.Get());
        KDbgErrorWData(GetActivityId(), logId.Get(), K_STATUS_LOG_STRUCTURE_FAULT, 0, RecordHeader.Lsn.Get(), 0, 0);
    }

    VOID
    InvalidStreamTypeInValidRecord(RvdLogRecordHeader& RecordHeader)
    {
        RvdLogUtility::AsciiGuid    logId(_LogId.Get());
        KDbgErrorWData(GetActivityId(), logId.Get(), K_STATUS_LOG_STRUCTURE_FAULT, 0, RecordHeader.Lsn.Get(), 0, 0);
    }

    VOID
    InvalidStreamLimitsInValidRecord(RvdLogRecordHeader& RecordHeader, RvdLogStreamInformation& StreamInfo)
    {
        UNREFERENCED_PARAMETER(RecordHeader);

        RvdLogUtility::AsciiTwinGuid    logAndStreamId(_LogId.Get(), StreamInfo.LogStreamId.Get());
        KDbgErrorWData(
            GetActivityId(), 
            logAndStreamId.Get(), 
            K_STATUS_LOG_STRUCTURE_FAULT, 
            0, StreamInfo.HighestLsn.Get(), 
            StreamInfo.LowestLsn.Get(), 
            StreamInfo.NextLsn.Get());
    }


    //** Recovery/Validate progress notification interface
    VOID
    ReportFileSizeLayout(
        KBlockDevice& BlockDevice,
        ULONGLONG FileSize,
        ULONG ChunkSize,
        ULONGLONG MaxFreeSpace,
        ULONGLONG MaxCheckPointLogRecordSize)
    {
        UNREFERENCED_PARAMETER(BlockDevice);

        RvdLogUtility::AsciiGuid    logId(_LogId.Get());
        KDbgPrintf(
            "AsyncMountLog::ReportFileSizeLayout: %s: FileSize: %I64u; "
            "ChunkSize: %u; MaxFreeSpace: %I64u; MaxCheckPointLogRecordSize: %I64u",
            logId.Get(),
            FileSize,
            ChunkSize,
            MaxFreeSpace,
            MaxCheckPointLogRecordSize);
    }

    VOID
    ReportFirstMasterBlockCorrect(RvdLogMasterBlock& MasterBlk)
    {
        UNREFERENCED_PARAMETER(MasterBlk);

        RvdLogUtility::AsciiGuid    logId(_LogId.Get());
        KDbgPrintf("AsyncMountLog::ReportFirstMasterBlockCorrect: %s", logId.Get());
    }

    VOID
    ReportSecondMasterBlockCorrect(RvdLogMasterBlock& MasterBlk)
    {
        UNREFERENCED_PARAMETER(MasterBlk);

        RvdLogUtility::AsciiGuid    logId(_LogId.Get());
        KDbgPrintf("AsyncMountLog::ReportSecondMasterBlockCorrect: %s", logId.Get());
    }

    VOID
    ReportSearchLowChunck(
        BOOLEAN IsContinuedSplitRead,
        ULONGLONG LowSearchChunkNumber,
        ULONG CurrentChunkOffset,
        ULONG ChunkSizeRead,
        BOOLEAN FoundValidRecord,
        BOOLEAN FoundHole)
    {
        RvdLogUtility::AsciiGuid    logId(_LogId.Get());
        KDbgPrintf(
            "AsyncMountLog::ReportSearchLowChunck: %s: IsContinued: %u; "
            "CurrChunk{Number: %I64u; Offset: %u; SizeRead: %u}; FoundValidRecord: %u; FoundHole: %u",
            logId.Get(),
            IsContinuedSplitRead,
            LowSearchChunkNumber,
            CurrentChunkOffset,
            ChunkSizeRead,
            FoundValidRecord,
            FoundHole);
    }

    VOID
    ReportEmptyLogFound(ULONG LineNumber)
    {
        RvdLogUtility::AsciiGuid    logId(_LogId.Get());
        KDbgPrintf(
            "AsyncMountLog::ReportEmptyLogFound: %s: Line: %u",
            logId.Get(),
            LineNumber);
    }

    VOID
    ReportHighestLsnRecordHeaderChunkFound(
        ULONGLONG Chunk,
        ULONGLONG FileOffset,
        RvdLogLsn HighestLsn,
        ULONGLONG HighLsnRecordSize,
        LONGLONG SizeOfSpacePreceedingHighLsn)
    {
        RvdLogUtility::AsciiGuid    logId(_LogId.Get());
        KDbgPrintf(
            "AsyncMountLog::ReportHighestLsnRecordHeaderChunkFound: %s: AtChunk: %I64u; FileOffset: %I64u; "
            "HighestLsn: %I64u; HighestLsnSize: %I64u; SizeOfSpacePreceedingHighLsn: %I64u",
            logId.Get(),
            Chunk,
            FileOffset,
            HighestLsn.Get(),
            HighLsnRecordSize,
            SizeOfSpacePreceedingHighLsn);
    }

    VOID
    ReportSearchingForFinalHighLsn(ULONGLONG FileOffset, ULONG RegionSize, BOOLEAN IsWrappedSpace)
    {
        RvdLogUtility::AsciiGuid    logId(_LogId.Get());
        KDbgPrintf(
            "AsyncMountLog::ReportSearchingForFinalHighLsn: %s: FileOffset: %I64u; RegionSize: %u; IsWrappedSpace: %u",
            logId.Get(),
            FileOffset,
            RegionSize,
            IsWrappedSpace);
    }

    VOID
    ReportHoleFoundSearchingForFinalHighLsn(ULONGLONG CurrentFileOffset, ULONG LineNumber)
    {
        RvdLogUtility::AsciiGuid    logId(_LogId.Get());
        KDbgPrintf(
            "AsyncMountLog::ReportHoleFoundSearchingForFinalHighLsn: %s: Line: %u: FileOffset: %I64u",
            logId.Get(),
            LineNumber,
            CurrentFileOffset);
    }

    VOID
    ReportLogHeadRegion(
        RvdLogLsn NextLsnToWrite,
        RvdLogLsn HighestLsn,
        RvdLogLsn HighestCompletedLsn,
        RvdLogLsn HighestCheckpointLsn)
    {
        RvdLogUtility::AsciiGuid    logId(_LogId.Get());
        KDbgPrintf(
            "AsyncMountLog::ReportLogHeadRegion: %s: NextLsnToWrite: %I64u; HighestLsn: %I64u; "
            "HighestCompletedLsn: %I64u; HighestCheckpointLsn: %I64u",
            logId.Get(),
            NextLsnToWrite.Get(),
            HighestLsn.Get(),
            HighestCompletedLsn.Get(),
            HighestCheckpointLsn.Get());
    }

    VOID
    ReportLowestHeadLsnCrcError(RvdLogLsn LowestHeadLsn)
    {
        RvdLogUtility::AsciiGuid    logId(_LogId.Get());
        KDbgPrintf(
            "AsyncMountLog::ReportLowestHeadLsnCrcError: %s: LowestHeadLsn: %I64u",
            logId.Get(),
            LowestHeadLsn.Get());
    }

    VOID
    ReportPhysicalCheckpointRecordFound(RvdLogLsn ReadAt, ULONG LineNumber)
    {
        RvdLogUtility::AsciiGuid    logId(_LogId.Get());
        KDbgPrintf(
            "AsyncMountLog::ReportPhysicalCheckpointRecordFound: %s Line: %u: ReadAtLsn: %I64u",
            logId.Get(),
            LineNumber,
            ReadAt.Get());
    }

    VOID
    ReportHighestLsnsEstablished(RvdLogLsn NextLsnToWrite, RvdLogLsn HighestCompletedLsn, RvdLogLsn HighestCheckpointLsn)
    {
        RvdLogUtility::AsciiGuid    logId(_LogId.Get());
        KDbgPrintf(
            "AsyncMountLog::ReportHighestLsnsEstablished: %s: NextLsnToWrite: %I64u; "
            "HighestCompletedLsn: %I64u; HighestCheckpointLsn: %I64u",
            logId.Get(),
            NextLsnToWrite.Get(),
            HighestCompletedLsn.Get(),
            HighestCheckpointLsn.Get());
    }

    VOID
    ReportHighestPhysicalCheckpoint(RvdLogPhysicalCheckpointRecord& Record)
    {
        RvdLogUtility::AsciiGuid    logId(_LogId.Get());
        KDbgPrintf(
            "AsyncMountLog::ReportHighestPhysicalCheckpoint: %s: AtLsn: %I64u; NumberOfStreams: %u",
            logId.Get(),
            Record.Lsn.Get(),
            Record.NumberOfLogStreams);
    }

    VOID
    ReportRecoveredStreamInfoInCheckpointRecord(ULONG StreamIx, RvdLogStreamInformation& CurrentInfo)
    {
        UNREFERENCED_PARAMETER(StreamIx);
        
        RvdLogUtility::AsciiGuid    streamId(CurrentInfo.LogStreamId.Get());
        RvdLogUtility::AsciiGuid    streamType(CurrentInfo.LogStreamType.Get());
        RvdLogUtility::AsciiGuid    logId(_LogId.Get());

        KDbgPrintf(
            "AsyncMountLog::ReportRecoveredStreamInfoInCheckpointRecord: %s\n"
            "\tStreamId: %s; \n"
            "\tStreamType: %s; LowestLsn: %I64u; HighestLsn: %I64u; NextLsn: %I64u",
            logId.Get(),
            streamId.Get(),
            streamType.Get(),
            CurrentInfo.LowestLsn.Get(),
            CurrentInfo.HighestLsn.Get(),
            CurrentInfo.NextLsn.Get());
    }

    VOID
    ReportPhysicalCheckpointStreamLimits(RvdLogLsn LowestLsn, RvdLogLsn HighestLsn, RvdLogLsn NextLsn)
    {
        RvdLogUtility::AsciiGuid    logId(_LogId.Get());
        KDbgPrintf(
            "AsyncMountLog::ReportPhysicalCheckpointStreamLimits: %s: LowestLsn: %I64u; HighestLsn: %I64u; NextLsn: %I64u",
            logId.Get(),
            LowestLsn.Get(),
            HighestLsn.Get(),
            NextLsn.Get());
    }

    VOID
    ReportLowestLogLsnFound(RvdLogLsn LowestLsn)
    {
        RvdLogUtility::AsciiGuid    logId(_LogId.Get());
        KDbgPrintf(
            "AsyncMountLog::ReportLowestLogLsnFound: %s: LowestLsn: %I64u",
            logId.Get(),
            LowestLsn.Get());
    }

    VOID
    ReportAllStreamLimitsRecovered(ULONG NumberOfStreams, RvdLogStreamInformation& StreamInfoArray)
    {
        RvdLogStreamInformation* StreamInfos = &StreamInfoArray;
        RvdLogUtility::AsciiGuid    logId(_LogId.Get());

        KDbgPrintf(
            "AsyncMountLog::ReportAllStreamLimitsRecovered: %s: NumberOfStreams: %u", 
            logId.Get(),
            NumberOfStreams);

        for(ULONG ix = 0; ix < NumberOfStreams; ix++)
        {
            RvdLogStreamInformation& streamInfo = StreamInfos[ix];
            RvdLogUtility::AsciiGuid    streamId(streamInfo.LogStreamId.Get());
            
            KDbgPrintf(
                "\t%s: StreamId: %s; LowestLsn: %I64u; HighestLsn: %I64u; NextLsn: %I64u",
                logId.Get(),
                streamId.Get(),
                streamInfo.LowestLsn.Get(),
                streamInfo.HighestLsn.Get(),
                streamInfo.NextLsn.Get());
        }
    }

    VOID
    ReportPhysicalLogRecovered(
        ULONGLONG Freespace,
        RvdLogLsn LowestLsn,
        RvdLogLsn NextLsnToWrite,
        RvdLogLsn HighestCompletedLsn,
        RvdLogLsn HighestCheckpointLsn)
    {
        RvdLogUtility::AsciiGuid    logId(_LogId.Get());

        KDbgPrintf(
            "AsyncMountLog::ReportPhysicalLogRecovered: %s:\n"
            "\tFreespace: %I64u; LowestLsn: %I64u; NextLsnToWrite: %I64u; HighestCompletedLsn: %I64u; HighestCheckpointLsn: %I64u;",
            logId.Get(),
            Freespace,
            LowestLsn.Get(),
            NextLsnToWrite.Get(),
            HighestCompletedLsn.Get(),
            HighestCheckpointLsn.Get());
    }

private:
    RvdLogManagerImp::RvdOnDiskLog::SPtr    _Parent;
    RvdLogManager::VerificationCallback     _LastVerificationCallback;
    RvdLogStreamType                        _LastLogStreamType;
};

RvdLogManagerImp::RvdOnDiskLog::AsyncMountLog::Recovery::~Recovery()
{
}


//** Mount Implementation
NTSTATUS
RvdLogManagerImp::RvdOnDiskLog::CreateAsyncMountLog(__out AsyncMountLog::SPtr& Context)
{
    Context = _new(KTL_TAG_LOGGER, GetThisAllocator()) AsyncMountLog(this);
    if (!Context)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS status = Context->Status();
    if (!NT_SUCCESS(status))
    {
        Context.Reset();
    }
    return status;    
}

RvdLogManagerImp::RvdOnDiskLog::AsyncMountLog::AsyncMountLog(__in RvdLogManagerImp::RvdOnDiskLog::SPtr Owner)
    :   _Owner(Ktl::Move(Owner))
{
}

RvdLogManagerImp::RvdOnDiskLog::AsyncMountLog::~AsyncMountLog()
{
}

VOID
RvdLogManagerImp::RvdOnDiskLog::AsyncMountLog::StartMount(
    __in_opt KAsyncContextBase* const ParentAsyncContext, 
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    Start(ParentAsyncContext, CallbackPtr);
}

VOID
RvdLogManagerImp::RvdOnDiskLog::AsyncMountLog::DoComplete(__in NTSTATUS Status)
{
    if (!_Owner->_IsOpen)
    {
        // Only perserve an underlying open _BlockDevice if we achieved "mounted"
        _Owner->_BlockDevice.Reset();
    }

    ReleaseLock(_Owner->_CreateOpenDeleteGate);
    _IoBuffer = nullptr;
    Complete(Status);
}

VOID
RvdLogManagerImp::RvdOnDiskLog::AsyncMountLog::OnStart()
//
// Continued from StartMount()
{
    AcquireLock(
        _Owner->_CreateOpenDeleteGate, 
        LockAcquiredCallback(this, &AsyncMountLog::GateAcquireComplete));
    // Continued @GateAcquireComplete
}

VOID
RvdLogManagerImp::RvdOnDiskLog::AsyncMountLog::GateAcquireComplete(
    __in BOOLEAN IsAcquired,
    __in KAsyncLock& LockAttempted)
//
// Continues from OnStart()
{
    UNREFERENCED_PARAMETER(LockAttempted);

    NTSTATUS status;

    if (!IsAcquired)
    {
        // Reacquire attempt
        // BUG, richhas, xxxxx, add trace output
        AcquireLock(
            _Owner->_CreateOpenDeleteGate, 
            LockAcquiredCallback(this, &AsyncMountLog::GateAcquireComplete));

        return;
    }

    // NOTE: From this point on DoComplete() MUST be used to finish the op
    if (_Owner->_IsOpen)
    {
        // Underlying RvdLogOnDisk has been opened already - just succeed
        DoComplete(STATUS_SUCCESS);
        return;
    }

    //
    // First open as a non-sparse file and check the masterblock to see if it is sparse and if so
    // then reopen it.
    //
    KAssert(_Owner->_Streams.IsEmpty());
    KAssert(!_Owner->_IsOpen);

    status = KBlockDevice::CreateDiskBlockDevice(
        _Owner->_DiskId,
        0,
        TRUE,
        FALSE,                              // ~CreateNew
        _Owner->_FullyQualifiedLogName,
        _Owner->_BlockDevice,
        CompletionCallback(this, &RvdLogManagerImp::RvdOnDiskLog::AsyncMountLog::LogFileCheckCreationFlags),
        this,
        GetThisAllocator(),
        KTL_TAG_LOGGER,
        NULL,
        FALSE,
        FALSE,
        RvdLogManagerImp::RvdOnDiskLog::ReadAheadSize,
        KBlockFile::CreateOptions::eShareRead);

    if (!K_ASYNC_SUCCESS(status))
    {
        DoComplete(status);
        return;
    }
    // Continued @LogFileCheckCreationFlags
}

VOID
RvdLogManagerImp::RvdOnDiskLog::AsyncMountLog::LogFileCheckCreationFlags(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingSubOp)
{
    UNREFERENCED_PARAMETER(Parent);

    KAssert(!_Owner->_IsOpen);
    NTSTATUS status = CompletingSubOp.Status();
    if (!NT_SUCCESS(status))
    {
        // CreateDiskBlockDevice() failed
        DoComplete(status);
        return;
    }

    //
    // Read the master block to extract the creation flags
    //
    status = _Owner->_BlockDevice->Read(
        0,
        RvdDiskLogConstants::MasterBlockSize,
        TRUE,                   // contiguous buffer
        _IoBuffer,
        KAsyncContextBase::CompletionCallback(this, &RvdLogManagerImp::RvdOnDiskLog::AsyncMountLog::LogFileReadMasterBlock),
        nullptr,
        nullptr);

    if (!K_ASYNC_SUCCESS(status))
    {
        KDbgCheckpointWData(0, "Failed Masterblock read", status, (ULONGLONG)this, 0, RvdDiskLogConstants::MasterBlockSize, 0);
        DoComplete(status);
        return;
    }

    // Continued @ LogFileReadMasterBlock
}

VOID
RvdLogManagerImp::RvdOnDiskLog::AsyncMountLog::LogFileReadMasterBlock(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingSubOp)
{
    UNREFERENCED_PARAMETER(Parent);

    KAssert(!_Owner->_IsOpen);
    NTSTATUS status = CompletingSubOp.Status();
    if (!NT_SUCCESS(status))
    {
        // CreateDiskBlockDevice() failed
        DoComplete(status);
        return;
    }

    // CONSIDER: Validating this master block and reading second if first is bad
    RvdLogMasterBlock* masterBlk = (RvdLogMasterBlock*)_IoBuffer->First()->GetBuffer();

    _Owner->_CreationFlags = 0;
    if ((masterBlk->MajorVersion > 1) ||
        (masterBlk->MajorVersion == 1) && (masterBlk->MinorVersion >= 1))
    {
        //
        // Remember creation flags if they are present
        //
        _Owner->_CreationFlags = masterBlk->CreationFlags;
    }
    
    BOOLEAN useSparseFile = ((_Owner->_CreationFlags & RvdLogManager::AsyncCreateLog::FlagSparseFile) == RvdLogManager::AsyncCreateLog::FlagSparseFile) ? TRUE : FALSE;
    if (useSparseFile)
    {
        ULONG createOptions = static_cast<ULONG>(KBlockFile::eNoIntermediateBuffering) |
                              static_cast<ULONG>(KBlockFile::CreateOptions::eShareRead);
        
        //
        // File is sparse, reopen this as sparse
        //
        _Owner->_BlockDevice = nullptr;
        status = KBlockDevice::CreateDiskBlockDevice(
            _Owner->_DiskId,
            0,
            TRUE,
            FALSE,                              // ~CreateNew
            _Owner->_FullyQualifiedLogName,
            _Owner->_BlockDevice,
            CompletionCallback(this, &AsyncMountLog::LogFileOpenComplete),
            this,
            GetThisAllocator(),
            KTL_TAG_LOGGER,
            NULL,
            FALSE,
            TRUE,
            RvdLogManagerImp::RvdOnDiskLog::ReadAheadSize,
            static_cast<KBlockFile::CreateOptions>(createOptions));            

        if (!K_ASYNC_SUCCESS(status))
        {
            DoComplete(status);
            return;
        }
        // Continued @LogFileOpenComplete
    }
    else 
    {
        //
        // File is not sparse, continue on to LogFileOpenComplete
        LogFileOpenComplete(Parent, CompletingSubOp);
    }
}

VOID
RvdLogManagerImp::RvdOnDiskLog::AsyncMountLog::LogFileOpenComplete(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingSubOp)
//
// Continues from GateAcquiredComplete()
{
    UNREFERENCED_PARAMETER(Parent);
    
    KAssert(!_Owner->_IsOpen);
    NTSTATUS status = CompletingSubOp.Status();
    if (!NT_SUCCESS(status))
    {
        // CreateDiskBlockDevice() failed
        DoComplete(status);
        return;
    }

    KAssert(_Owner->_BlockDevice);

    _RecoveryState = _new(KTL_TAG_LOGGER, GetThisAllocator()) Recovery(_Owner);
    if (_RecoveryState == nullptr)
    {
        DoComplete(STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    status = _RecoveryState->Status();
    if (!NT_SUCCESS(status))
    {
        _RecoveryState.Reset();
        DoComplete(status);
        return;
    }

    // Start the log file level recovery process
    _RecoveryState->StartRecovery(
        _Owner->_BlockDevice, 
        _Owner->_LogId, 
        _Owner->_CreationFlags,
        this, 
        CompletionCallback(this, &AsyncMountLog::RecoveryComplete));
    // Continued @LogFileOpenComplete
}

VOID
RvdLogManagerImp::RvdOnDiskLog::AsyncMountLog::RecoveryComplete(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingSubOp)
//
// Continues from LogFileOpenComplete()
{
    UNREFERENCED_PARAMETER(Parent);
    
    if (!NT_SUCCESS(CompletingSubOp.Status()))
    {
        // Recovery failed
        DoComplete(CompletingSubOp.Status());
        return;
    }

    // Recovered the log - get the recovered state and populate _Owner (log desc)
    LogState::SPtr      recoveredFileState;
    NTSTATUS            status = _RecoveryState->SnapLogState(recoveredFileState);
    if (!NT_SUCCESS(status))
    {
        DoComplete(status);
        return;
    }

    status = _Owner->PopulateLogState(recoveredFileState);
    if (!NT_SUCCESS(status))
    {
        DoComplete(status);
        return;
    }

    // Allocate the async op instance used to write physical log record into _CheckPointStream
    status = RvdLogStreamImp::AsyncWriteStream::CreateCheckpointStreamAsyncWriteContext(
        GetThisAllocator(),
        *(_Owner.RawPtr()),
        _Owner->_PhysicalCheckPointWriteOp);

    if (!NT_SUCCESS(status))
    {
        DoComplete(status);
        return;
    }
    
    _Owner->_IsOpen = TRUE;
    DoComplete(STATUS_SUCCESS);
}

