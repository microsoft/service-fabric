/*++

Copyright (c) Microsoft Corporation

Module Name:

    RvdLoggerVerifyTests.cpp

Abstract:

--*/
#pragma once
#include "RvdLoggerTests.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>
#include <InternalKtlLogger.h>
#include "RvdLogStreamAsyncIOTests.h"
#include <KtlLogRecovery.h>

#if (CONSOLE_TEST) || (DISPLAY_ON_CONSOLE)
#undef KDbgPrintf
#define KDbgPrintf(...)     printf(__VA_ARGS__)
#endif

//** Log File Validation and Recovery Test and Utility Support Classes
enum RvdLogRecoveryReportDetail
{
    Errors              = 0x01,
    ProgressReports     = 0x02,
    OtherReports        = 0x04,
    All                 = 0x07,
    Normal              = 0x03,
};

// BUG, richhas, xxxxxx, consider making LogValidator a common class used by log related utilties. If not,
//                       merge this class into UnitTestLogValidator to simplify the unit test code base.
class LogValidator : public KAsyncContextBase
{
    K_FORCE_SHARED_WITH_INHERITANCE(LogValidator);

public:
    VOID
    StartValidation(
        __in KBlockDevice::SPtr BlockDevice,
        __in RvdLogId& LogId,
        __in RvdLogRecoveryReportDetail ReportDetail,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

    NTSTATUS
    SnapLogState(__out LogState::SPtr& Result, __in ULONG const AllocationTag, __in KAllocator& Allocator);

protected:
    // Derivation interface
    virtual NTSTATUS
    ValidateUserRecord(
        RvdLogAsn UserRecordAsn,
        ULONGLONG UserRecordAsnVersion,
        void* UserMetadata,
        ULONG UserMetadataSize,
        RvdLogStreamType& LogStreamType,
        void* UserIoBufferData,
        ULONG UserIoBufferDataSize) = 0;


    //** Recovery/Validate failure notification interface
    virtual VOID
    AllocateTransferContextFailed(NTSTATUS Status, KBlockDevice::SPtr& BlockDevice, ULONG LineNumber)
    {
        UNREFERENCED_PARAMETER(LineNumber);
        UNREFERENCED_PARAMETER(BlockDevice);
        UNREFERENCED_PARAMETER(Status);
    }

    virtual VOID
    PhysicalStartReadFailed(NTSTATUS Status, KBlockDevice::SPtr& BlockDevice, ULONGLONG FileOffset, ULONG ReadSize, ULONG LineNumber)
    {
        UNREFERENCED_PARAMETER(LineNumber);
        UNREFERENCED_PARAMETER(ReadSize);
        UNREFERENCED_PARAMETER(FileOffset);
        UNREFERENCED_PARAMETER(BlockDevice);
        UNREFERENCED_PARAMETER(Status);
    }

    virtual VOID
    AllocatedFileTooSmall(ULONGLONG FileSize, ULONGLONG MinFileSizeAllowed)
    {
        UNREFERENCED_PARAMETER(MinFileSizeAllowed);
        UNREFERENCED_PARAMETER(FileSize);
    }

    virtual VOID
    AllocatedFileLengthWrong(ULONGLONG FileSize, ULONGLONG ChunkSize)
    {
        UNREFERENCED_PARAMETER(ChunkSize);
        UNREFERENCED_PARAMETER(FileSize);
    }

    virtual VOID
    PhysicalReadFailed(NTSTATUS Status, KBlockDevice::SPtr& BlockDevice, ULONGLONG FileOffset, ULONG ReadSize, ULONG LineNumber)
    {
        UNREFERENCED_PARAMETER(LineNumber);
        UNREFERENCED_PARAMETER(ReadSize);
        UNREFERENCED_PARAMETER(FileOffset);
        UNREFERENCED_PARAMETER(BlockDevice);
        UNREFERENCED_PARAMETER(Status);
    }

    virtual VOID
    FirstMasterBlockValidationFailed(NTSTATUS Status, KBlockDevice::SPtr& BlockDevice, RvdLogMasterBlock& MasterBlk)
    {
        UNREFERENCED_PARAMETER(MasterBlk);
        UNREFERENCED_PARAMETER(BlockDevice);
        UNREFERENCED_PARAMETER(Status);
    }

    virtual VOID
    SecondMasterBlockValidationFailed(NTSTATUS Status, KBlockDevice::SPtr& BlockDevice, RvdLogMasterBlock& MasterBlk)
    {
        UNREFERENCED_PARAMETER(MasterBlk);
        UNREFERENCED_PARAMETER(BlockDevice);
        UNREFERENCED_PARAMETER(Status);
    }

    virtual VOID
    LogRecordReaderCreationFailed(NTSTATUS Status, ULONG Line)
    {
        UNREFERENCED_PARAMETER(Line);
        UNREFERENCED_PARAMETER(Status);
    }

    virtual VOID
    SplitReadFailed(NTSTATUS Status)
    {
        UNREFERENCED_PARAMETER(Status);
    }

    virtual VOID
    KIoBufferElementCreationFailed(NTSTATUS Status, ULONG LineNumber)
    {
        UNREFERENCED_PARAMETER(LineNumber);
        UNREFERENCED_PARAMETER(Status);
    }

    virtual VOID
    MultipartReadFailed(NTSTATUS Status, ULONG LineNumber)
    {
        UNREFERENCED_PARAMETER(LineNumber);
        UNREFERENCED_PARAMETER(Status);
    }

    virtual VOID
    LsnOrderingFailed(ULONGLONG RecordFileOffset, RvdLogLsn RecordLsn, RvdLogLsn RecordNextLsn, RvdLogLsn ExpectedNextLsn, ULONG LineNumber)
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
    RecordReaderFailed(NTSTATUS Status, RvdLogLsn ReadAt, ULONG LineNumber)
    {
        UNREFERENCED_PARAMETER(LineNumber);
        UNREFERENCED_PARAMETER(ReadAt);
        UNREFERENCED_PARAMETER(Status);
    }

    virtual VOID
    RecordReaderFailed(NTSTATUS Status, ULONGLONG ReadAtOffset, ULONG LineNumber)
    {
        UNREFERENCED_PARAMETER(LineNumber);
        UNREFERENCED_PARAMETER(ReadAtOffset);
        UNREFERENCED_PARAMETER(Status);
    }

    virtual VOID
    PhysicalCheckpointRecordIoBufferSizeNotZero(RvdLogLsn ReadAt, ULONG LineNumber)
    {
        UNREFERENCED_PARAMETER(LineNumber);
        UNREFERENCED_PARAMETER(ReadAt);
    }

    virtual VOID
    InvalidMultiSegmentStreamCheckpointRecord(RvdLogUserStreamRecordHeader& RecordHeader, ULONG LineNumber)
    {
        UNREFERENCED_PARAMETER(LineNumber);
        UNREFERENCED_PARAMETER(RecordHeader);
    }

    virtual VOID
    UserRecordHeaderFault(RvdLogUserStreamRecordHeader& UserStreamRecordHeader, ULONG LineNumber)
    {
        UNREFERENCED_PARAMETER(LineNumber);
        UNREFERENCED_PARAMETER(UserStreamRecordHeader);
    }

    virtual VOID
    ValidateUserRecordFailed(RvdLogRecordHeader& RecordHeader, ULONG LineNumber)
    {
        UNREFERENCED_PARAMETER(LineNumber);
        UNREFERENCED_PARAMETER(RecordHeader);
    }

    virtual VOID
    HighestCheckpointLsnNotFound() {}

    virtual VOID
    PhysicalCheckpointRecordFault(RvdLogPhysicalCheckpointRecord& Record, ULONG LineNumber)
    {
        UNREFERENCED_PARAMETER(LineNumber);
        UNREFERENCED_PARAMETER(Record);
    }

    virtual VOID
    InconsistentStreamInfoFound(RvdLogStreamInformation& CurrentInfo, RvdLogPhysicalCheckpointRecord& CpRecord)
    {
        UNREFERENCED_PARAMETER(CurrentInfo);
        UNREFERENCED_PARAMETER(CpRecord);
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

    virtual VOID
    MemoryAllocationFailed(ULONG LineNumber)
    {
        UNREFERENCED_PARAMETER(LineNumber);
    }

    virtual VOID
    AddCurrentStreamDescFailed(NTSTATUS Status, ULONG LineNumber)
    {
        UNREFERENCED_PARAMETER(LineNumber);
        UNREFERENCED_PARAMETER(Status);
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
    ReportLogRecovered()
    {
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

    virtual VOID
    ReportRecoveredStream(LogState::StreamDescriptor& StreamDesc)
    {
        UNREFERENCED_PARAMETER(StreamDesc);
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
    DoComplete(NTSTATUS Status);

    VOID
    OnStart();

    VOID
    LogFileRecoveryComplete(KAsyncContextBase* const Parent, KAsyncContextBase& CompletedContext);

    VOID
    RecoverAllStreams();

    VOID
    StreamRecoveryComplete(KAsyncContextBase* const Parent, KAsyncContextBase& CompletedContext);

    VOID
    RecoveryCompleted();

    //** Common support functions
    NTSTATUS
    AddOrUpdateRvdAsnIndexEntry(RvdLogAsn Asn, ULONGLONG AsnVersion, ULONG IoBufferSize, RvdLogLsn Lsn);

    NTSTATUS
    AddCurrentStreamDesc();

private:
    // Concrete derivations of RvdLogRecovery and RvdLogStreamRecovery
    class LogRecovery;
    class StreamRecovery;

protected:
    RvdLogRecoveryReportDetail          _ReportDetail;

private:
    KBlockDevice::SPtr                  _BlockDevice;
    RvdLogId                            _LogId;

    // Recovered Log File State
    KSharedPtr<LogRecovery>             _LogFileRecovery;
    LogState::SPtr                      _LogState;

    // Current recovered stream state
    LogState::StreamDescriptor*         _CurrentStreamDesc;
    KSharedPtr<StreamRecovery>          _CurrentStreamRecovery;
};



#pragma region LogValidator logic

//* Concreate RvdLogRecovery imp -  this class is simply a forwarder of the
//  RvdLogRecovery's derivation interface into LogValidator.
class LogValidator::LogRecovery : public RvdLogRecovery
{
    K_FORCE_SHARED(LogRecovery);

public:
    LogRecovery(LogValidator& Parent)
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
        KInvariant(UserIoBuffer->QueryNumberOfIoBufferElements() == 1);
        return _Parent.ValidateUserRecord(
            UserRecordAsn,
            UserRecordAsnVersion,
            UserMetadata,
            UserMetadataSize,
            LogStreamType,
            (void*)(UserIoBuffer->First()->GetBuffer()),
            UserIoBuffer->QuerySize());
    }

    //** Recovery/Validate failure notification interface -  forwards to parent
    VOID
    AllocateTransferContextFailed(NTSTATUS Status, KBlockDevice::SPtr& BlockDevice, ULONG LineNumber)
    {
        _Parent.AllocateTransferContextFailed(Status, BlockDevice, LineNumber);
    }

    VOID
    PhysicalStartReadFailed(
        NTSTATUS Status,
        KBlockDevice::SPtr& BlockDevice,
        ULONGLONG FileOffset,
        ULONG ReadSize,
        ULONG LineNumber)
    {
        _Parent.PhysicalStartReadFailed(Status, BlockDevice, FileOffset, ReadSize, LineNumber);
    }

    VOID
    PhysicalReadFailed(
        NTSTATUS Status,
        KBlockDevice::SPtr& BlockDevice,
        ULONGLONG FileOffset,
        ULONG ReadSize,
        ULONG LineNumber)
    {
        _Parent.PhysicalReadFailed(Status, BlockDevice, FileOffset, ReadSize, LineNumber);
    }

    VOID
    LogRecordReaderCreationFailed(NTSTATUS Status, ULONG LineNumber)
    {
        _Parent.LogRecordReaderCreationFailed(Status, LineNumber);
    }

    VOID
    RecordReaderFailed(NTSTATUS Status, RvdLogLsn ReadAt, ULONG LineNumber)
    {
        _Parent.RecordReaderFailed(Status, ReadAt, LineNumber);
    }

    VOID
    RecordReaderFailed(NTSTATUS Status, ULONGLONG ReadAtOffset, ULONG LineNumber)
    {
        _Parent.RecordReaderFailed(Status, ReadAtOffset, LineNumber);
    }

    VOID
    UserRecordHeaderFault(RvdLogUserStreamRecordHeader& UserStreamRecordHeader, ULONG LineNumber)
    {
        _Parent.UserRecordHeaderFault(UserStreamRecordHeader, LineNumber);
    }

    VOID
    AllocatedFileTooSmall(ULONGLONG FileSize, ULONGLONG MinFileSizeAllowed)
    {
        _Parent.AllocatedFileTooSmall(FileSize, MinFileSizeAllowed);
    }

    VOID
    AllocatedFileLengthWrong(ULONGLONG FileSize, ULONGLONG ChunkSize)
    {
        _Parent.AllocatedFileLengthWrong(FileSize, ChunkSize);
    }

    VOID
    FirstMasterBlockValidationFailed(NTSTATUS Status, KBlockDevice::SPtr& BlockDevice, RvdLogMasterBlock& MasterBlk)
    {
        _Parent.FirstMasterBlockValidationFailed(Status, BlockDevice, MasterBlk);
    }

    VOID
    SecondMasterBlockValidationFailed(NTSTATUS Status, KBlockDevice::SPtr& BlockDevice, RvdLogMasterBlock& MasterBlk)
    {
        _Parent.SecondMasterBlockValidationFailed(Status, BlockDevice, MasterBlk);
    }

    VOID
    SplitReadFailed(NTSTATUS Status)
    {
        _Parent.SplitReadFailed(Status);
    }

    VOID
    KIoBufferElementCreationFailed(NTSTATUS Status, ULONG LineNumber)
    {
        _Parent.KIoBufferElementCreationFailed(Status, LineNumber);
    }

    VOID
    MultipartReadFailed(NTSTATUS Status, ULONG LineNumber)
    {
        _Parent.MultipartReadFailed(Status, LineNumber);
    }

    VOID
    LsnOrderingFailed(
        ULONGLONG RecordFileOffset,
        RvdLogLsn RecordLsn,
        RvdLogLsn RecordNextLsn,
        RvdLogLsn ExpectedNextLsn,
        ULONG LineNumber)
    {
        _Parent.LsnOrderingFailed(RecordFileOffset, RecordLsn, RecordNextLsn, ExpectedNextLsn, LineNumber);
    }

    VOID
    HighestCompletedLsnNotCoherentWithHighestLsn(RvdLogLsn HighestCompletedLsn, RvdLogLsn HighestLsn)
    {
        _Parent.HighestCompletedLsnNotCoherentWithHighestLsn(HighestCompletedLsn, HighestLsn);
    }

    VOID
    PhysicalCheckpointRecordIoBufferSizeNotZero(RvdLogLsn ReadAt, ULONG LineNumber)
    {
        _Parent.PhysicalCheckpointRecordIoBufferSizeNotZero(ReadAt, LineNumber);
    }

    VOID
    ValidateUserRecordFailed(RvdLogRecordHeader& RecordHeader, ULONG LineNumber)
    {
        _Parent.ValidateUserRecordFailed(RecordHeader, LineNumber);
    }

    VOID
    HighestCheckpointLsnNotFound()
    {
        _Parent.HighestCheckpointLsnNotFound();
    }

    VOID
    PhysicalCheckpointRecordFault(RvdLogPhysicalCheckpointRecord& Record, ULONG LineNumber)
    {
        _Parent.PhysicalCheckpointRecordFault(Record, LineNumber);
    }

    VOID
    InconsistentStreamInfoFound(RvdLogStreamInformation& CurrentInfo, RvdLogPhysicalCheckpointRecord& CpRecord)
    {
        _Parent.InconsistentStreamInfoFound(CurrentInfo, CpRecord);
    }

    VOID
    CheckpointStreamInfoMissingFromCheckpointRecord(RvdLogPhysicalCheckpointRecord& CpRecord)
    {
        _Parent.CheckpointStreamInfoMissingFromCheckpointRecord(CpRecord);
    }

    VOID
    LowestLogLsnNotEstablishedInCheckpointRecord(RvdLogPhysicalCheckpointRecord& CpRecord)
    {
        _Parent.LowestLogLsnNotEstablishedInCheckpointRecord(CpRecord);
    }

    VOID
    StreamMissingInValidRecord(RvdLogRecordHeader& RecordHeader)
    {
        _Parent.StreamMissingInValidRecord(RecordHeader);
    }

    VOID
    InvalidStreamTypeInValidRecord(RvdLogRecordHeader& RecordHeader)
    {
        _Parent.InvalidStreamTypeInValidRecord(RecordHeader);
    }

    VOID
    InvalidStreamLimitsInValidRecord(RvdLogRecordHeader& RecordHeader, RvdLogStreamInformation& StreamInfo)
    {
        _Parent.InvalidStreamLimitsInValidRecord(RecordHeader, StreamInfo);
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
        _Parent.ReportFileSizeLayout(BlockDevice, FileSize, ChunkSize, MaxFreeSpace, MaxCheckPointLogRecordSize);
    }

    VOID
    ReportFirstMasterBlockCorrect(RvdLogMasterBlock& MasterBlk)
    {
        _Parent.ReportFirstMasterBlockCorrect(MasterBlk);
    }

    VOID
    ReportSecondMasterBlockCorrect(RvdLogMasterBlock& MasterBlk)
    {
        _Parent.ReportSecondMasterBlockCorrect(MasterBlk);
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
        _Parent.ReportSearchLowChunck(
            IsContinuedSplitRead,
            LowSearchChunkNumber,
            CurrentChunkOffset,
            ChunkSizeRead,
            FoundValidRecord,
            FoundHole);
    }

    VOID
    ReportPhysicallyCorrectRecordHeader(ULONGLONG CurrentFileOffset, RvdLogRecordHeader& RecordHeader, ULONG LineNumber)
    {
        _Parent.ReportPhysicallyCorrectRecordHeader(CurrentFileOffset, RecordHeader, LineNumber);
    }

    VOID
    ReportPhysicallyCorrectAndPlacedRecordHeader(
        ULONGLONG CurrentFileOffset,
        RvdLogRecordHeader& RecordHeader,
        ULONG LineNumber,
        RvdLogLsn HighestCheckpointLsn)
    {
        _Parent.ReportPhysicallyCorrectAndPlacedRecordHeader(CurrentFileOffset, RecordHeader, LineNumber, HighestCheckpointLsn);
    }

    VOID
    ReportOldRecordHeader(ULONGLONG CurrentFileOffset, RvdLogRecordHeader& RecordHeader, ULONG LineNumber)
    {
        _Parent.ReportOldRecordHeader(CurrentFileOffset, RecordHeader, LineNumber);
    }

    VOID
    ReportHoleFoundInChunk(ULONGLONG CurrentFileOffset, ULONGLONG LowSearchChunkNumber, ULONG LineNumber)
    {
        _Parent.ReportHoleFoundInChunk(CurrentFileOffset, LowSearchChunkNumber, LineNumber);
    }

    VOID
    ReportEmptyLogFound(ULONG LineNumber)
    {
        _Parent.ReportEmptyLogFound(LineNumber);
    }

    VOID
    ReportHighestLsnRecordHeaderChunkFound(
        ULONGLONG Chunk,
        ULONGLONG FileOffset,
        RvdLogLsn HighestLsn,
        ULONGLONG HighLsnRecordSize,
        LONGLONG SizeOfSpacePreceedingHighLsn)
    {
        _Parent.ReportHighestLsnRecordHeaderChunkFound(
            Chunk,
            FileOffset,
            HighestLsn,
            HighLsnRecordSize,
            SizeOfSpacePreceedingHighLsn);
    }

    VOID
    ReportSearchingForFinalHighLsn(ULONGLONG FileOffset, ULONG RegionSize, BOOLEAN IsWrappedSpace)
    {
        _Parent.ReportSearchingForFinalHighLsn(FileOffset, RegionSize, IsWrappedSpace);
    }

    VOID
    ReportHoleFoundSearchingForFinalHighLsn(ULONGLONG CurrentFileOffset, ULONG LineNumber)
    {
        _Parent.ReportHoleFoundSearchingForFinalHighLsn(CurrentFileOffset, LineNumber);
    }

    VOID
    ReportLogHeadRegion(
        RvdLogLsn NextLsnToWrite,
        RvdLogLsn HighestLsn,
        RvdLogLsn HighestCompletedLsn,
        RvdLogLsn HighestCheckpointLsn)
    {
        _Parent.ReportLogHeadRegion(NextLsnToWrite, HighestLsn, HighestCompletedLsn, HighestCheckpointLsn);
    }

    VOID
    ReportLowestHeadLsnCrcError(RvdLogLsn LowestHeadLsn)
    {
        _Parent.ReportLowestHeadLsnCrcError(LowestHeadLsn);
    }

    VOID
    ReportPhysicalCheckpointRecordFound(RvdLogLsn ReadAt, ULONG LineNumber)
    {
        _Parent.ReportPhysicalCheckpointRecordFound(ReadAt, LineNumber);
    }

    VOID
    ReportHighestLsnsEstablished(RvdLogLsn NextLsnToWrite, RvdLogLsn HighestCompletedLsn, RvdLogLsn HighestCheckpointLsn)
    {
        _Parent.ReportHighestLsnsEstablished(NextLsnToWrite, HighestCompletedLsn, HighestCheckpointLsn);
    }

    VOID
    ReportHighestPhysicalCheckpoint(RvdLogPhysicalCheckpointRecord& Record)
    {
        _Parent.ReportHighestPhysicalCheckpoint(Record);
    }

    VOID
    ReportRecoveredStreamInfoInCheckpointRecord(ULONG StreamIx, RvdLogStreamInformation& CurrentInfo)
    {
        _Parent.ReportRecoveredStreamInfoInCheckpointRecord(StreamIx, CurrentInfo);
    }

    VOID
    ReportPhysicalCheckpointStreamLimits(RvdLogLsn LowestLsn, RvdLogLsn HighestLsn, RvdLogLsn NextLsn)
    {
        _Parent.ReportPhysicalCheckpointStreamLimits(LowestLsn, HighestLsn, NextLsn);
    }

    VOID
    ReportLowestLogLsnFound(RvdLogLsn LowestLsn)
    {
        _Parent.ReportLowestLogLsnFound(LowestLsn);
    }

    VOID
    ReportAllStreamLimitsRecovered(ULONG NumberOfStreams, RvdLogStreamInformation& StreamInfoArray)
    {
        _Parent.ReportAllStreamLimitsRecovered(NumberOfStreams, StreamInfoArray);
    }

    VOID
    ReportPhysicalLogRecovered(
        ULONGLONG Freespace,
        RvdLogLsn LowestLsn,
        RvdLogLsn NextLsnToWrite,
        RvdLogLsn HighestCompletedLsn,
        RvdLogLsn HighestCheckpointLsn)
    {
        _Parent.ReportPhysicalLogRecovered(Freespace, LowestLsn, NextLsnToWrite, HighestCompletedLsn, HighestCheckpointLsn);
    }

    VOID
    ReportRecordHeaderApprearsValid(RvdLogRecordHeader& RecordHeader, ULONGLONG CurrentFileOffset)
    {
        _Parent.ReportRecordHeaderApprearsValid(RecordHeader, CurrentFileOffset);
    }

    VOID
    ReportRecordHeaderIsInvalid(RvdLogRecordHeader& RecordHeader, ULONGLONG CurrentFileOffset)
    {
        _Parent.ReportRecordHeaderIsInvalid(RecordHeader, CurrentFileOffset);
    }

private:
    LogValidator&           _Parent;
};

LogValidator::LogRecovery::~LogRecovery()
{
}


//* Concreate RvdLogStreamRecovery imp -  this class is simply a forwarder of the
//  RvdLogRecovery's derivation interface into LogValidator.
class LogValidator::StreamRecovery : public RvdLogStreamRecovery
{
    K_FORCE_SHARED(StreamRecovery);

public:
    StreamRecovery(LogValidator& Parent)
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
        KInvariant(UserIoBuffer->QueryNumberOfIoBufferElements() == 1);
        return _Parent.ValidateUserRecord(
            UserRecordAsn,
            UserRecordAsnVersion,
            UserMetadata,
            UserMetadataSize,
            LogStreamType,
            (void*)(UserIoBuffer->First()->GetBuffer()),
            UserIoBuffer->QuerySize());
    }

    VOID
    AllocateTransferContextFailed(NTSTATUS Status, KBlockDevice::SPtr& BlockDevice, ULONG LineNumber)
    {
        _Parent.AllocateTransferContextFailed(Status, BlockDevice, LineNumber);
    }

    VOID
    PhysicalStartReadFailed(NTSTATUS Status, KBlockDevice::SPtr& BlockDevice, ULONGLONG FileOffset, ULONG ReadSize, ULONG LineNumber)
    {
        _Parent.PhysicalStartReadFailed(Status, BlockDevice, FileOffset, ReadSize, LineNumber);
    }

    VOID
    PhysicalReadFailed(NTSTATUS Status, KBlockDevice::SPtr& BlockDevice, ULONGLONG FileOffset, ULONG ReadSize, ULONG LineNumber)
    {
        _Parent.PhysicalReadFailed(Status, BlockDevice, FileOffset, ReadSize, LineNumber);
    }

    VOID
    LogRecordReaderCreationFailed(NTSTATUS Status, ULONG LineNumber)
    {
        _Parent.LogRecordReaderCreationFailed(Status, LineNumber);
    }

    VOID
    RecordReaderFailed(NTSTATUS Status, RvdLogLsn ReadAt, ULONG LineNumber)
    {
        _Parent.RecordReaderFailed(Status, ReadAt, LineNumber);
    }

    VOID
    RecordReaderFailed(NTSTATUS Status, ULONGLONG ReadAtOffset, ULONG LineNumber)
    {
        _Parent.RecordReaderFailed(Status, ReadAtOffset, LineNumber);
    }

    VOID
    UserRecordHeaderFault(RvdLogUserStreamRecordHeader& UserStreamRecordHeader, ULONG LineNumber)
    {
        _Parent.UserRecordHeaderFault(UserStreamRecordHeader, LineNumber);
    }

    VOID
    MemoryAllocationFailed(ULONG LineNumber)
    {
        _Parent.MemoryAllocationFailed(LineNumber);
    }

    VOID InvalidMultiSegmentStreamCheckpointRecord(RvdLogUserStreamRecordHeader& RecordHeader, ULONG LineNumber)
    {
        _Parent.InvalidMultiSegmentStreamCheckpointRecord(RecordHeader, LineNumber);
    }

    VOID
    InvalidStreamIdInValidRecord(RvdLogUserStreamRecordHeader& RecordHeader)
    {
        _Parent.InvalidStreamIdInValidRecord(RecordHeader);
    }

    VOID
    AddLowerLsnRecordIndexEntryFailed(NTSTATUS Status, RvdLogUserStreamRecordHeader& RecordHeader, ULONG LineNumber)
    {
        _Parent.AddLowerLsnRecordIndexEntryFailed(Status, RecordHeader, LineNumber);
    }

    VOID
    AddAsnRecordIndexEntryFailed(NTSTATUS Status, RvdLogAsn RecordAsn, ULONGLONG RecordVersion, ULONG LineNumber)
    {
        _Parent.AddAsnRecordIndexEntryFailed(Status, RecordAsn, RecordVersion, LineNumber);
    }

    VOID
    LsnIndexQueryRecordFailed(NTSTATUS Status)
    {
        _Parent.LsnIndexQueryRecordFailed(Status);
    }

    VOID
    LsnIndexRecordMissing(RvdLogLsn Lsn)
    {
        _Parent.LsnIndexRecordMissing(Lsn);
    }

    VOID
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
        _Parent.ReportLogStreamRecovered(
            StreamId,
            StreamType,
            LsnCount,
            LowestLsn,
            HighestLsn,
            NextLsn,
            HighestStreamCheckPoint,
            AsnCount,
            LowestAsn,
            HighestAsn,
            TruncationAsn,
            TruncationLsn);
    }

private:
    LogValidator&           _Parent;
};

LogValidator::StreamRecovery::~StreamRecovery()
{
}


//*** LogValidator implemenation
LogValidator::LogValidator()
    :   _CurrentStreamDesc(nullptr)
{
}

LogValidator::~LogValidator()
{
}

NTSTATUS
LogValidator::SnapLogState(__out LogState::SPtr& Result, __in ULONG const AllocationTag, __in KAllocator& Allocator)
{
    UNREFERENCED_PARAMETER(AllocationTag);
    UNREFERENCED_PARAMETER(Allocator);

    Result = _LogState;
    return STATUS_SUCCESS;
}

VOID
LogValidator::StartValidation(
    __in KBlockDevice::SPtr BlockDevice,
    __in RvdLogId& LogId,
    __in RvdLogRecoveryReportDetail ReportDetail,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _BlockDevice = Ktl::Move(BlockDevice);
    _LogId = LogId;
    _ReportDetail = ReportDetail;
    Start(ParentAsyncContext, CallbackPtr);
}

VOID
LogValidator::DoComplete(NTSTATUS Status)
{
    Complete(Status);
}

VOID
LogValidator::OnStart()
{
    _LogFileRecovery = _new(KTL_TAG_TEST, GetThisAllocator()) LogRecovery(*this);

    if (_LogFileRecovery == nullptr)
    {
        MemoryAllocationFailed(__LINE__);
        DoComplete(STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    // Fire off the physical log file recovery logic.
    _LogFileRecovery->StartRecovery(
        _BlockDevice,
        _LogId,
#if ALWAYS_SPARSE_FILES
        RvdLogManager::AsyncCreateLog::FlagSparseFile,
#else
        0,
#endif
        this,
        KAsyncContextBase::CompletionCallback(this, &LogValidator::LogFileRecoveryComplete));
    // Continued @ LogFileRecoveryComplete()
}

VOID
LogValidator::LogFileRecoveryComplete(KAsyncContextBase* const Parent, KAsyncContextBase& CompletedContext)
// Continued from OnStart()
{
    UNREFERENCED_PARAMETER(Parent);

    NTSTATUS status = CompletedContext.Status();

    if (!NT_SUCCESS(status))
    {
        DoComplete(status);
        return;
    }

    status = _LogFileRecovery->SnapLogState(_LogState);
    if (!NT_SUCCESS(status))
    {
        MemoryAllocationFailed(__LINE__);
        DoComplete(STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    // Next all stream record content indices can be recovered.
    _CurrentStreamDesc = &(_LogState->_StreamDescs[0]);
    RecoverAllStreams();
}

VOID
LogValidator::RecoverAllStreams()
{
    // For each stream attempt recovery
    //
    while (_CurrentStreamDesc < &(_LogState->_StreamDescs[_LogState->_NumberOfStreams]))
    {
        KAssert((_CurrentStreamDesc->_AsnIndex == nullptr) && (_CurrentStreamDesc->_LsnIndex == nullptr));

        _CurrentStreamRecovery = _new(KTL_TAG_TEST,  GetThisAllocator()) LogValidator::StreamRecovery(*this);
        if (_CurrentStreamRecovery == nullptr)
        {
            MemoryAllocationFailed(__LINE__);
            DoComplete(STATUS_INSUFFICIENT_RESOURCES);
            return;
        }

        _CurrentStreamRecovery->StartRecovery(
            _BlockDevice,
            _LogState->_Config,
            _LogId,
            _LogState->_LogFileLsnSpace,
            _LogState->_LowestLsn,
            _LogState->_NextLsnToWrite,
            _LogState->_LogSignature,
            _CurrentStreamDesc->_Info,
            this,
            KAsyncContextBase::CompletionCallback(this, &LogValidator::StreamRecoveryComplete));

        return;
        // Continued @ LogFileRecoveryComplete()
    }

    RecoveryCompleted();
}

VOID
LogValidator::StreamRecoveryComplete(KAsyncContextBase* const Parent, KAsyncContextBase& CompletedContext)
// Continued from RecoverAllStreams()
{
    UNREFERENCED_PARAMETER(Parent);

    NTSTATUS status = CompletedContext.Status();

    if (!NT_SUCCESS(status))
    {
        DoComplete(status);
        return;
    }

    RvdLogStreamInformation     streamInfo;
    RvdLogLsn                   highestCheckPointLsn;

    _CurrentStreamRecovery->GetRecoveredStreamInformation(
        streamInfo,
        _CurrentStreamDesc->_AsnIndex,
        _CurrentStreamDesc->_LsnIndex,
        _CurrentStreamDesc->_TruncationPoint,
        highestCheckPointLsn);

    KInvariant(RtlCompareMemory(&streamInfo, &_CurrentStreamDesc->_Info, sizeof(streamInfo)) == sizeof(streamInfo));

    _CurrentStreamDesc++;
    RecoverAllStreams();
}

VOID
LogValidator::RecoveryCompleted()
{
    ReportLogRecovered();
    DoComplete(STATUS_SUCCESS);
}

#pragma endregion LogValidator logic



class UnitTestLogValidator : public LogValidator
{
    K_FORCE_SHARED(UnitTestLogValidator);

public:
    enum VerificationMethod
    {
        TestMetadataVerification,
        StreamUnitTestVerification
    };

    static NTSTATUS
    Create(
        __out UnitTestLogValidator::SPtr& Result,
        __in ULONG const AllocationTag,
        __in KAllocator& Allocator,
        __in VerificationMethod VerificationMethod = TestMetadataVerification)
    {
        Result = _new(AllocationTag, Allocator) UnitTestLogValidator();
        if (Result == nullptr)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        NTSTATUS status = Result->Status();
        if (!NT_SUCCESS(status))
        {
            Result.Reset();
        }
        else
        {
            Result->_VerificationMethod = VerificationMethod;
        }
        return status;
    }

private:
    // BUG, richhas, xxxxx, fully implement ReportDetail support

    BOOLEAN __inline
    ReportOtherProgress()
    {
        return (_ReportDetail & RvdLogRecoveryReportDetail::OtherReports) == RvdLogRecoveryReportDetail::OtherReports;
    }

    NTSTATUS
    ValidateUserRecord(
        RvdLogAsn UserRecordAsn,
        ULONGLONG UserRecordAsnVersion,
        void* UserMetadata,
        ULONG UserMetadataSize,
        RvdLogStreamType& LogStreamType,
        void* UserIoBufferData,
        ULONG UserIoBufferDataSize)
    {
        UNREFERENCED_PARAMETER(LogStreamType);

        if (_VerificationMethod == VerificationMethod::TestMetadataVerification)
        {
            if (UserMetadataSize != sizeof(TestMetadata))
            {
                return K_STATUS_LOG_STRUCTURE_FAULT;
            }

            TestMetadata*       metadata = (TestMetadata*)UserMetadata;
            if (KChecksum::Crc64(UserIoBufferData, UserIoBufferDataSize) != metadata->DataChecksum)
            {
                return K_STATUS_LOG_STRUCTURE_FAULT;
            }

            return STATUS_SUCCESS;
        }

        if (_VerificationMethod == VerificationMethod::StreamUnitTestVerification)
        {
            if (!LogStreamIoContext::ValidateRecordContents(
                UserRecordAsn,
                UserRecordAsnVersion,
                UserMetadata,
                UserMetadataSize,
                UserIoBufferData,
                UserIoBufferDataSize))
            {
                return K_STATUS_LOG_STRUCTURE_FAULT;
            }

            return STATUS_SUCCESS;
        }

        return K_STATUS_LOG_STRUCTURE_FAULT;
    }

    //** Override of the Failure notification interface
    VOID
    AllocateTransferContextFailed(NTSTATUS Status, KBlockDevice::SPtr& BlockDevice, ULONG LineNumber)
    {
        UNREFERENCED_PARAMETER(BlockDevice);

        KDbgPrintf(
            "UnitTestLogValidator::AllocateTransferContextFailed @LogValidator Line: %i with Status: 0x%08X\n",
            LineNumber,
            Status);
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

        KDbgPrintf(
            "UnitTestLogValidator::PhysicalStartReadFailed @LogValidator Line: %i with Status: 0x%08X: Read(%I64u, %i)\n",
            LineNumber,
            Status,
            FileOffset,
            ReadSize);
    }

    VOID
    AllocatedFileTooSmall(ULONGLONG FileSize, ULONGLONG MinFileSizeAllowed)
    {
        KDbgPrintf(
            "UnitTestLogValidator::AllocatedFileTooSmall: FileSize: %I64u, MinAllowed: %I64u\n",
            FileSize,
            MinFileSizeAllowed);
    }

    VOID
    AllocatedFileLengthWrong(ULONGLONG FileSize, ULONGLONG ChunkSize)
    {
        KDbgPrintf(
            "UnitTestLogValidator::AllocatedFileTooSmall: FileSize: %I64u, ChunkSize: %I64u\n",
            FileSize,
            ChunkSize);
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

        KDbgPrintf(
            "UnitTestLogValidator::PhysicalReadFailed @LogValidator Line: %i with Status: 0x%08X: Read(%I64u, %i)\n",
            LineNumber,
            Status,
            FileOffset,
            ReadSize);
    }

    VOID
    FirstMasterBlockValidationFailed(NTSTATUS Status, KBlockDevice::SPtr& BlockDevice, RvdLogMasterBlock& MasterBlk)
    {
        UNREFERENCED_PARAMETER(MasterBlk);
        UNREFERENCED_PARAMETER(BlockDevice);

        KDbgPrintf("UnitTestLogValidator::FirstMasterBlockValidationFailed with Status: 0x%08X\n", Status);
    }

    VOID
    SecondMasterBlockValidationFailed(NTSTATUS Status, KBlockDevice::SPtr& BlockDevice, RvdLogMasterBlock& MasterBlk)
    {
        UNREFERENCED_PARAMETER(MasterBlk);
        UNREFERENCED_PARAMETER(BlockDevice);

        KDbgPrintf("UnitTestLogValidator::SecondMasterBlockValidationFailed with Status: 0x%08X\n", Status);
    }

    VOID
    LogRecordReaderCreationFailed(NTSTATUS Status, ULONG LineNumber)
    {
        KDbgPrintf(
            "UnitTestLogValidator::LogRecordReaderCreationFailed @LogValidator Line: %i with Status: 0x%08X\n",
            LineNumber,
            Status);
    }

    VOID
    SplitReadFailed(NTSTATUS Status)
    {
        KDbgPrintf("UnitTestLogValidator::SplitReadFailed with Status: 0x%08X\n", Status);
    }

    VOID
    KIoBufferElementCreationFailed(NTSTATUS Status, ULONG LineNumber)
    {
        KDbgPrintf(
            "UnitTestLogValidator::KIoBufferElementCreationFailed @LogValidator Line: %i with Status: 0x%08X\n",
            LineNumber,
            Status);
    }

    VOID
    MultipartReadFailed(NTSTATUS Status, ULONG LineNumber)
    {
        KDbgPrintf(
            "UnitTestLogValidator::MultipartReadFailed @LogValidator Line: %i with Status: 0x%08X\n",
            LineNumber,
            Status);
    }

    VOID
    LsnOrderingFailed(
        ULONGLONG RecordFileOffset,
        RvdLogLsn RecordLsn,
        RvdLogLsn RecordNextLsn,
        RvdLogLsn ExpectedNextLsn,
        ULONG LineNumber)
    {
        KDbgPrintf(
            "UnitTestLogValidator::LsnOrderingFailed @LogValidator Line: %i: "
            "FileOffset: %I64u, RecordLsn: %I64u, RecordNextLsn:  %I64u, ExpectedNextLsn:  %I64u\n",
            LineNumber,
            RecordFileOffset,
            RecordLsn.Get(),
            RecordNextLsn.Get(),
            ExpectedNextLsn.Get());
    }

    VOID
    HighestCompletedLsnNotCoherentWithHighestLsn(RvdLogLsn HighestCompletedLsn, RvdLogLsn HighestLsn)
    {
        KDbgPrintf(
            "UnitTestLogValidator::HighestCompletedLsnNotCoherentWithHighestLsn: HighestCompletedLsn: %I64u, HighestLsn: %I64u\n",
            HighestCompletedLsn.Get(),
            HighestLsn.Get());
    }

    VOID
    RecordReaderFailed(NTSTATUS Status, RvdLogLsn ReadAt, ULONG LineNumber)
    {
        KDbgPrintf(
            "UnitTestLogValidator::RecordReaderFailed @LogValidator Line: %i with Status: 0x%08X; ReadAtLsn: %I64u\n",
            LineNumber,
            Status,
            ReadAt.Get());
    }

    VOID
    RecordReaderFailed(NTSTATUS Status, ULONGLONG ReadAtOffset, ULONG LineNumber)
    {
        KDbgPrintf(
            "UnitTestLogValidator::RecordReaderFailed @LogValidator Line: %i with Status: 0x%08X; ReadAtOffset: %I64u\n",
            LineNumber,
            Status,
            ReadAtOffset);
    }

    VOID
    PhysicalCheckpointRecordIoBufferSizeNotZero(RvdLogLsn ReadAt, ULONG LineNumber)
    {
        KDbgPrintf(
            "UnitTestLogValidator::PhysicalCheckpointRecordIoBufferSizeNotZero @LogValidator Line: %i; ReadAtLsn: %I64u\n",
            LineNumber,
            ReadAt.Get());
    }

    VOID
    UserRecordHeaderFault(RvdLogUserStreamRecordHeader& UserStreamRecordHeader, ULONG LineNumber)
    {
        KWString streamId(KtlSystem::GlobalNonPagedAllocator(), UserStreamRecordHeader.LogStreamId.Get());
#if defined(PLATFORM_UNIX)
        KDbgPrintf(
            "UnitTestLogValidator::UserRecordHeaderFault @LogValidator Line: %i; Stream: %s; AtLsn: %I64u; Type: %i; MetadataSize: %i; IoBufferSize: %i\n",
            LineNumber,
            Utf16To8((WCHAR*)streamId).c_str(),
            UserStreamRecordHeader.Lsn.Get(),
            UserStreamRecordHeader.RecordType,
            UserStreamRecordHeader.MetaDataSize,
            UserStreamRecordHeader.IoBufferSize);
#else
        KDbgPrintf(
            "UnitTestLogValidator::UserRecordHeaderFault @LogValidator Line: %i; Stream: %S; AtLsn: %I64u; Type: %i; MetadataSize: %i; IoBufferSize: %i\n",
            LineNumber,
            (WCHAR*)streamId,
            UserStreamRecordHeader.Lsn.Get(),
            UserStreamRecordHeader.RecordType,
            UserStreamRecordHeader.MetaDataSize,
            UserStreamRecordHeader.IoBufferSize);
#endif
    }

    VOID
    ValidateUserRecordFailed(RvdLogRecordHeader& RecordHeader, ULONG LineNumber)
    {
        KDbgPrintf(
            "UnitTestLogValidator::ValidateUserRecordFailed @LogValidator Line: %i; AtLsn: %I64u\n",
            LineNumber,
            RecordHeader.Lsn.Get());
    }

    VOID
    HighestCheckpointLsnNotFound()
    {
        KDbgPrintf("UnitTestLogValidator::HighestCheckpointLsnNotFound\n");
    }

    VOID
    PhysicalCheckpointRecordFault(RvdLogPhysicalCheckpointRecord& Record, ULONG LineNumber)
    {
        UNREFERENCED_PARAMETER(LineNumber);

        KDbgPrintf(
            "UnitTestLogValidator::PhysicalCheckpointRecordFault; CpAtLsn: %I64i; IoBufferSize: %u; NumberOfLogStreams: %u\n",
            Record.Lsn.Get(),
            Record.IoBufferSize,
            Record.NumberOfLogStreams);
    }

    VOID
    InvalidMultiSegmentStreamCheckpointRecord(RvdLogUserStreamRecordHeader& RecordHeader, ULONG LineNumber)
    {
        KDbgPrintf(
            "UnitTestLogValidator::InvalidMultiSegmentStreamCheckpointRecord; Line: %u, Lsn: %I64i; RecType: %u; TruncPoint: %I64u\n",
            LineNumber,
            RecordHeader.Lsn.Get(),
            RecordHeader.RecordType,
            RecordHeader.TruncationPoint.Get());
    }

    VOID
    InconsistentStreamInfoFound(RvdLogStreamInformation& CurrentInfo, RvdLogPhysicalCheckpointRecord& CpRecord)
    {
        KWString streamId(KtlSystem::GlobalNonPagedAllocator(), CurrentInfo.LogStreamId.Get());
#if defined(PLATFORM_UNIX)
        KDbgPrintf(
            "UnitTestLogValidator::InconsistentStreamInfoFound; ForStreamId: %s; CpAtLsn: %I64u; LowestLsn: %I64u; HighestLsn: %I64u; NextLsn: %I64u\n",
            Utf16To8((WCHAR*)streamId).c_str(),
            CpRecord.Lsn.Get(),
            CurrentInfo.LowestLsn.Get(),
            CurrentInfo.HighestLsn.Get(),
            CurrentInfo.NextLsn.Get());
#else
        KDbgPrintf(
            "UnitTestLogValidator::InconsistentStreamInfoFound; ForStreamId: %S; CpAtLsn: %I64u; LowestLsn: %I64u; HighestLsn: %I64u; NextLsn: %I64u\n",
            (WCHAR*)streamId,
            CpRecord.Lsn.Get(),
            CurrentInfo.LowestLsn.Get(),
            CurrentInfo.HighestLsn.Get(),
            CurrentInfo.NextLsn.Get());
#endif
    }

    VOID
    CheckpointStreamInfoMissingFromCheckpointRecord(RvdLogPhysicalCheckpointRecord& CpRecord)
    {
        KDbgPrintf(
            "UnitTestLogValidator::CheckpointStreamInfoMissingFromCheckpointRecord; CpAtLsn: %I64u\n",
            CpRecord.Lsn.Get());
    }

    VOID
    LowestLogLsnNotEstablishedInCheckpointRecord(RvdLogPhysicalCheckpointRecord& CpRecord)
    {
        KDbgPrintf(
            "UnitTestLogValidator::LowestLogLsnNotEstablishedInCheckpointRecord; CpAtLsn: %I64u\n",
            CpRecord.Lsn.Get());
    }

    VOID
    StreamMissingInValidRecord(RvdLogRecordHeader& RecordHeader)
    {
        KWString streamId(KtlSystem::GlobalNonPagedAllocator(), RecordHeader.LogStreamId.Get());
#if defined(PLATFORM_UNIX)
        KDbgPrintf(
            "UnitTestLogValidator::StreamMissingInValidRecord; ForStreamId: %s; AtLsn: %I64u\n",
            Utf16To8((WCHAR*)streamId).c_str(),
            RecordHeader.Lsn.Get());
#else
        KDbgPrintf(
            "UnitTestLogValidator::StreamMissingInValidRecord; ForStreamId: %S; AtLsn: %I64u\n",
            (WCHAR*)streamId,
            RecordHeader.Lsn.Get());
#endif
    }

    VOID
    InvalidStreamTypeInValidRecord(RvdLogRecordHeader& RecordHeader)
    {
        KWString streamId(KtlSystem::GlobalNonPagedAllocator(), RecordHeader.LogStreamId.Get());
        KWString streamType(KtlSystem::GlobalNonPagedAllocator(), RecordHeader.LogStreamId.Get());
#if defined(PLATFORM_UNIX)
        KDbgPrintf(
            "UnitTestLogValidator::StreamMissingInValidRecord; ForStreamId: %s; StreamType: %s; AtLsn: %I64u\n",
            Utf16To8((WCHAR*)streamId).c_str(),
            Utf16To8((WCHAR*)streamType).c_str(),
            RecordHeader.Lsn.Get());
#else
        KDbgPrintf(
            "UnitTestLogValidator::StreamMissingInValidRecord; ForStreamId: %S; StreamType: %S; AtLsn: %I64u\n",
            (WCHAR*)streamId,
            (WCHAR*)streamType,
            RecordHeader.Lsn.Get());
#endif
    }

    VOID
    InvalidStreamLimitsInValidRecord(RvdLogRecordHeader& RecordHeader, RvdLogStreamInformation& StreamInfo)
    {
        KWString streamId(KtlSystem::GlobalNonPagedAllocator(), RecordHeader.LogStreamId.Get());
#if defined(PLATFORM_UNIX)
        KDbgPrintf(
            "UnitTestLogValidator::InvalidStreamLimitsInValidRecord; ForStreamId: %s; AtLsn: %I64u; "
            "PrevLsnInLogStream: %I64u; StreamInfo{HighestLsn: %I64u; NextLsn: %I64u}\n",
            Utf16To8((WCHAR*)streamId).c_str(),
            RecordHeader.Lsn.Get(),
            RecordHeader.PrevLsnInLogStream.Get(),
            StreamInfo.HighestLsn.Get(),
            StreamInfo.NextLsn.Get());
#else
        KDbgPrintf(
            "UnitTestLogValidator::InvalidStreamLimitsInValidRecord; ForStreamId: %S; AtLsn: %I64u; "
            "PrevLsnInLogStream: %I64u; StreamInfo{HighestLsn: %I64u; NextLsn: %I64u}\n",
            (WCHAR*)streamId,
            RecordHeader.Lsn.Get(),
            RecordHeader.PrevLsnInLogStream.Get(),
            StreamInfo.HighestLsn.Get(),
            StreamInfo.NextLsn.Get());
#endif
    }

    VOID
    MemoryAllocationFailed(ULONG LineNumber)
    {
        KDbgPrintf(
            "UnitTestLogValidator::MemoryAllocationFailed @LogValidator Line: %i\n",
            LineNumber);
    }

    VOID
    AddCurrentStreamDescFailed(NTSTATUS Status, ULONG LineNumber)
    {
        KDbgPrintf(
            "UnitTestLogValidator::AddCurrentStreamDescFailed @LogValidator Line: %i with Status: 0x%08X\n",
            LineNumber,
            Status);
    }

    VOID
    InvalidStreamIdInValidRecord(RvdLogUserStreamRecordHeader& RecordHeader)
    {
        KWString streamId(KtlSystem::GlobalNonPagedAllocator(), RecordHeader.LogStreamId.Get());
#if defined(PLATFORM_UNIX)
        KDbgPrintf(
            "UnitTestLogValidator::InvalidStreamLimitsInValidRecord; ForStreamId: %s; AtLsn: %I64u\n",
            Utf16To8((WCHAR*)streamId).c_str(),
            RecordHeader.Lsn.Get());
#else
        KDbgPrintf(
            "UnitTestLogValidator::InvalidStreamLimitsInValidRecord; ForStreamId: %S; AtLsn: %I64u\n",
            (WCHAR*)streamId,
            RecordHeader.Lsn.Get());
#endif
    }

    VOID
    AddLowerLsnRecordIndexEntryFailed(NTSTATUS Status, RvdLogUserStreamRecordHeader& RecordHeader, ULONG LineNumber)
    {
        KDbgPrintf(
            "UnitTestLogValidator::AddLowerLsnRecordIndexEntryFailed @LogValidator Line: %i with Status: 0x%08X; ForRecordAtLsn: %I64u\n",
            LineNumber,
            Status,
            RecordHeader.Lsn.Get());
    }

    VOID
    AddAsnRecordIndexEntryFailed(NTSTATUS Status, RvdLogAsn RecordAsn, ULONGLONG RecordVersion, ULONG LineNumber)
    {
        KDbgPrintf(
            "UnitTestLogValidator::AddAsnRecordIndexEntryFailed @LogValidator Line: %i with Status: 0x%08X; ForRecordAtAsn: %I64u; Version: %I64u\n",
            LineNumber,
            Status,
            RecordAsn.Get(),
            RecordVersion);
    }

    VOID
    LsnIndexQueryRecordFailed(NTSTATUS Status)
    {
        KDbgPrintf(
            "UnitTestLogValidator::LsnIndexQueryRecordFailed with Status: 0x%08X\n",
            Status);
    }

    VOID
    LsnIndexRecordMissing(RvdLogLsn Lsn)
    {
        KDbgPrintf(
            "UnitTestLogValidator::LsnIndexRecordMissing ForLsn: %I64u\n",
            Lsn.Get());
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

        KDbgPrintf(
            "UnitTestLogValidator::ReportFileSizeLayout: FileSize: %I64u; ChunkSize: %u; MaxFreeSpace: %I64u; MaxCheckPointLogRecordSize: %I64u\n",
            FileSize,
            ChunkSize,
            MaxFreeSpace,
            MaxCheckPointLogRecordSize);
    }

    VOID
    ReportFirstMasterBlockCorrect(RvdLogMasterBlock& MasterBlk)
    {
        UNREFERENCED_PARAMETER(MasterBlk);

        KDbgPrintf("UnitTestLogValidator::ReportFirstMasterBlockCorrect\n");
    }

    VOID
    ReportSecondMasterBlockCorrect(RvdLogMasterBlock& MasterBlk)
    {
        UNREFERENCED_PARAMETER(MasterBlk);

        KDbgPrintf("UnitTestLogValidator::ReportSecondMasterBlockCorrect\n");
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
        KDbgPrintf(
            "UnitTestLogValidator::ReportSearchLowChunck: IsContinued: %u; "
            "CurrChunk{Number: %I64u; Offset: %u; SizeRead: %u}; FoundValidRecord: %u; FoundHole: %u\n",
            IsContinuedSplitRead,
            LowSearchChunkNumber,
            CurrentChunkOffset,
            ChunkSizeRead,
            FoundValidRecord,
            FoundHole);
    }

    VOID
    ReportPhysicallyCorrectRecordHeader(ULONGLONG CurrentFileOffset, RvdLogRecordHeader& RecordHeader, ULONG LineNumber)
    {
        if (ReportOtherProgress())
        {
            KDbgPrintf(
                "UnitTestLogValidator::ReportPhysicallyCorrectRecordHeader @LogValidator Line: %u; FileOffset: %I64u; Lsn: %I64u\n",
                LineNumber,
                CurrentFileOffset,
                RecordHeader.Lsn.Get());
        }
    }

    VOID
    ReportPhysicallyCorrectAndPlacedRecordHeader(
        ULONGLONG CurrentFileOffset,
        RvdLogRecordHeader& RecordHeader,
        ULONG LineNumber,
        RvdLogLsn HighestCheckpointLsn)
    {
        UNREFERENCED_PARAMETER(RecordHeader);

        if (ReportOtherProgress())
        {
            KDbgPrintf(
                "UnitTestLogValidator::ReportPhysicallyCorrectAndPlacedRecordHeader @LogValidator Line: %u; FileOffset: %I64u; HighestCheckpointLsn: %I64u\n",
                LineNumber,
                CurrentFileOffset,
                HighestCheckpointLsn.Get());
        }
    }

    VOID
    ReportOldRecordHeader(ULONGLONG CurrentFileOffset, RvdLogRecordHeader& RecordHeader, ULONG LineNumber)
    {
        if (ReportOtherProgress())
        {
            KDbgPrintf(
                "UnitTestLogValidator::ReportOldRecordHeader @LogValidator Line: %u; FileOffset: %I64u; Lsn: %I64u\n",
                LineNumber,
                CurrentFileOffset,
                RecordHeader.Lsn.Get());
        }
    }

    VOID
    ReportHoleFoundInChunk(ULONGLONG CurrentFileOffset, ULONGLONG LowSearchChunkNumber, ULONG LineNumber)
    {
        if (ReportOtherProgress())
        {
            KDbgPrintf(
                "UnitTestLogValidator::ReportHoleFoundInChunk @LogValidator Line: %u; FileOffset: %I64u; LowSearchChunkNumber: %I64u\n",
                LineNumber,
                CurrentFileOffset,
                LowSearchChunkNumber);
        }
    }

    VOID
    ReportEmptyLogFound(ULONG LineNumber)
    {
        KDbgPrintf(
            "UnitTestLogValidator::ReportEmptyLogFound @LogValidator Line: %u\n",
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
        KDbgPrintf(
            "UnitTestLogValidator::ReportHighestLsnRecordHeaderChunkFound: AtChunk: %I64u; FileOffset: %I64u; "
            "HighestLsn: %I64u; HighestLsnSize: %I64u; SizeOfSpacePreceedingHighLsn: %I64u\n",
            Chunk,
            FileOffset,
            HighestLsn.Get(),
            HighLsnRecordSize,
            SizeOfSpacePreceedingHighLsn);
    }

    VOID
    ReportSearchingForFinalHighLsn(ULONGLONG FileOffset, ULONG RegionSize, BOOLEAN IsWrappedSpace)
    {
        KDbgPrintf(
            "UnitTestLogValidator::ReportSearchingForFinalHighLsn: FileOffset: %I64u; RegionSize: %u; IsWrappedSpace: %u\n",
            FileOffset,
            RegionSize,
            IsWrappedSpace);
    }

    VOID
    ReportHoleFoundSearchingForFinalHighLsn(ULONGLONG CurrentFileOffset, ULONG LineNumber)
    {
        KDbgPrintf(
            "UnitTestLogValidator::ReportHoleFoundSearchingForFinalHighLsn @LogValidator Line: %u: FileOffset: %I64u\n",
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
        KDbgPrintf(
            "UnitTestLogValidator::ReportLogHeadRegion: NextLsnToWrite: %I64u; HighestLsn: %I64u; HighestCompletedLsn: %I64u; HighestCheckpointLsn: %I64u\n",
            NextLsnToWrite.Get(),
            HighestLsn.Get(),
            HighestCompletedLsn.Get(),
            HighestCheckpointLsn.Get());
    }

    VOID
    ReportLowestHeadLsnCrcError(RvdLogLsn LowestHeadLsn)
    {
        KDbgPrintf(
            "UnitTestLogValidator::ReportLowestHeadLsnCrcError: LowestHeadLsn: %I64u\n",
            LowestHeadLsn.Get());
    }

    VOID
    ReportPhysicalCheckpointRecordFound(RvdLogLsn ReadAt, ULONG LineNumber)
    {
        KDbgPrintf(
            "UnitTestLogValidator::ReportPhysicalCheckpointRecordFound @LogValidator Line: %u: ReadAtLsn: %I64u\n",
            LineNumber,
            ReadAt.Get());
    }

    VOID
    ReportHighestLsnsEstablished(RvdLogLsn NextLsnToWrite, RvdLogLsn HighestCompletedLsn, RvdLogLsn HighestCheckpointLsn)
    {
        KDbgPrintf(
            "UnitTestLogValidator::ReportHighestLsnsEstablished: NextLsnToWrite: %I64u; HighestCompletedLsn: %I64u; HighestCheckpointLsn: %I64u\n",
            NextLsnToWrite.Get(),
            HighestCompletedLsn.Get(),
            HighestCheckpointLsn.Get());
    }

    VOID
    ReportHighestPhysicalCheckpoint(RvdLogPhysicalCheckpointRecord& Record)
    {
        KDbgPrintf(
            "UnitTestLogValidator::ReportHighestPhysicalCheckpoint: AtLsn: %I64u; NumberOfStreams: %u\n",
            Record.Lsn.Get(),
            Record.NumberOfLogStreams);
    }

    VOID
    ReportRecoveredStreamInfoInCheckpointRecord(ULONG StreamIx, RvdLogStreamInformation& CurrentInfo)
    {
        UNREFERENCED_PARAMETER(StreamIx);

        KWString streamId(KtlSystem::GlobalNonPagedAllocator(), CurrentInfo.LogStreamId.Get());
        KWString streamType(KtlSystem::GlobalNonPagedAllocator(), CurrentInfo.LogStreamType.Get());

#if defined(PLATFORM_UNIX)
        KDbgPrintf(
            "UnitTestLogValidator::ReportRecoveredStreamInfoInCheckpointRecord: \n"
            "\tStreamId: %s; \n"
            "\tStreamType: %s; LowestLsn: %I64u; HighestLsn: %I64u; NextLsn: %I64u\n",
            Utf16To8((WCHAR*)streamId).c_str(),
            Utf16To8((WCHAR*)streamType).c_str(),
            CurrentInfo.LowestLsn.Get(),
            CurrentInfo.HighestLsn.Get(),
            CurrentInfo.NextLsn.Get());
#else
        KDbgPrintf(
            "UnitTestLogValidator::ReportRecoveredStreamInfoInCheckpointRecord: \n"
            "\tStreamId: %S; \n"
            "\tStreamType: %S; LowestLsn: %I64u; HighestLsn: %I64u; NextLsn: %I64u\n",
            (WCHAR*)streamId,
            (WCHAR*)streamType,
            CurrentInfo.LowestLsn.Get(),
            CurrentInfo.HighestLsn.Get(),
            CurrentInfo.NextLsn.Get());
#endif
    }

    VOID
    ReportPhysicalCheckpointStreamLimits(RvdLogLsn LowestLsn, RvdLogLsn HighestLsn, RvdLogLsn NextLsn)
    {
        KDbgPrintf(
            "UnitTestLogValidator::ReportPhysicalCheckpointStreamLimits: LowestLsn: %I64u; HighestLsn: %I64u; NextLsn: %I64u\n",
            LowestLsn.Get(),
            HighestLsn.Get(),
            NextLsn.Get());
    }

    VOID
    ReportLowestLogLsnFound(RvdLogLsn LowestLsn)
    {
        KDbgPrintf(
            "UnitTestLogValidator::ReportLowestLogLsnFound: LowestLsn: %I64u\n",
            LowestLsn.Get());
    }

    VOID
    ReportRecordHeaderApprearsValid(RvdLogRecordHeader& RecordHeader, ULONGLONG CurrentFileOffset)
    {
        if (ReportOtherProgress())
        {
            KDbgPrintf(
                "UnitTestLogValidator::ReportRecordHeaderApprearsValid: AtFileOffset: %I64u; AtLsn: %I64u\n",
                CurrentFileOffset,
                RecordHeader.Lsn.Get());
        }
    }

    VOID
    ReportRecordHeaderIsInvalid(RvdLogRecordHeader& RecordHeader, ULONGLONG CurrentFileOffset)
    {
        UNREFERENCED_PARAMETER(RecordHeader);

        if (ReportOtherProgress())
        {
            KDbgPrintf(
                "UnitTestLogValidator::ReportRecordHeaderIsInvalid: AtFileOffset: %I64u\n",
                CurrentFileOffset);
        }
    }

    VOID
    ReportRecoveredStream(LogState::StreamDescriptor& StreamDesc)
    {
        KWString    streamId(KtlSystem::GlobalNonPagedAllocator(), StreamDesc._Info.LogStreamId.Get());
        BOOLEAN     isCheckpointStream = StreamDesc._Info.LogStreamId == RvdDiskLogConstants::CheckpointStreamId();
        ULONG       numberOfLSNs = isCheckpointStream ? 0 : StreamDesc._LsnIndex->QueryNumberOfRecords();

#if defined(PLATFORM_UNIX)
        KDbgPrintf(
            "UnitTestLogValidator::ReportRecoveredStream: StreamId: %s; LowestLsn: %I64u; HighestLsn: %I64u; NextLsn: %I64u; TPointAsn: %I64u; #LSNs: %u\n",
            Utf16To8((WCHAR*)streamId).c_str(),
            StreamDesc._Info.LowestLsn.Get(),
            StreamDesc._Info.HighestLsn.Get(),
            StreamDesc._Info.NextLsn.Get(),
            StreamDesc._TruncationPoint.Get(),
            numberOfLSNs);
#else
        KDbgPrintf(
            "UnitTestLogValidator::ReportRecoveredStream: StreamId: %S; LowestLsn: %I64u; HighestLsn: %I64u; NextLsn: %I64u; TPointAsn: %I64u; #LSNs: %u\n",
            (WCHAR*)streamId,
            StreamDesc._Info.LowestLsn.Get(),
            StreamDesc._Info.HighestLsn.Get(),
            StreamDesc._Info.NextLsn.Get(),
            StreamDesc._TruncationPoint.Get(),
            numberOfLSNs);
#endif

        if (!isCheckpointStream)
        {
            RvdAsnIndexEntry::SPtr lowestAsn = StreamDesc._AsnIndex->UnsafeFirst();
            RvdAsnIndexEntry::SPtr highestAsn = StreamDesc._AsnIndex->UnsafeLast();

            KDbgPrintf("\tAsnIndex: LowestAsn: %I64u {Lsn: %I64u}; HighestAsn: %I64u {Lsn: %I64u}\n",
                lowestAsn->GetAsn().Get(),
                lowestAsn->GetLsn().Get(),
                highestAsn->GetAsn().Get(),
                highestAsn->GetLsn().Get());
        }
    }

    VOID
    ReportAllStreamLimitsRecovered(ULONG NumberOfStreams, RvdLogStreamInformation& StreamInfoArray)
    {
        RvdLogStreamInformation* StreamInfos = &StreamInfoArray;
        KDbgPrintf("UnitTestLogValidator::ReportAllStreamLimitsRecovered: NumberOfStreams: %u\n", NumberOfStreams);
        for(ULONG ix = 0; ix < NumberOfStreams; ix++)
        {
            RvdLogStreamInformation& streamInfo = StreamInfos[ix];
            KWString streamId(KtlSystem::GlobalNonPagedAllocator(), streamInfo.LogStreamId.Get());
#if defined(PLATFORM_UNIX)
            KDbgPrintf(
                "\tStreamId: %s; LowestLsn: %I64u; HighestLsn: %I64u; NextLsn: %I64u\n",
                Utf16To8((WCHAR*)streamId).c_str(),
                streamInfo.LowestLsn.Get(),
                streamInfo.HighestLsn.Get(),
                streamInfo.NextLsn.Get());
#else
            KDbgPrintf(
                "\tStreamId: %S; LowestLsn: %I64u; HighestLsn: %I64u; NextLsn: %I64u\n",
                (WCHAR*)streamId,
                streamInfo.LowestLsn.Get(),
                streamInfo.HighestLsn.Get(),
                streamInfo.NextLsn.Get());
#endif
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
        KDbgPrintf(
            "UnitTestLogValidator::ReportPhysicalLogRecovered:\n"
            "\tFreespace: %I64u; LowestLsn: %I64u; NextLsnToWrite: %I64u; HighestCompletedLsn: %I64u; HighestCheckpointLsn: %I64u; \n",
            Freespace,
            LowestLsn.Get(),
            NextLsnToWrite.Get(),
            HighestCompletedLsn.Get(),
            HighestCheckpointLsn.Get());
    }

    VOID
    ReportLogRecovered()
    {
        KDbgPrintf("UnitTestLogValidator::ReportLogRecovered:\n");
    }

    VOID
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
        RvdLogUtility::AsciiGuid        streamId(StreamId.Get());
        RvdLogUtility::AsciiGuid        streamTypeId(StreamType.Get());

        KDbgPrintf(
            "UnitTestLogValidator::ReportLogStreamRecovered: %s: Type: %s\n"
                "\tLSNs: Count: %u; Low: %I64i; High: %I64i; Next: %I64i; HighStrCP: %I64i; Trunc: %I64i;\n"
                "\tASNs: Count: %u; Low: %I64u; High: %I64u; Trunc: %I64u\n",
            streamId.Get(),
            streamTypeId.Get(),
            LsnCount,
            LowestLsn.Get(),
            HighestLsn.Get(),
            NextLsn.Get(),
            HighestStreamCheckPoint.Get(),
            TruncationLsn.Get(),
            AsnCount,
            LowestAsn.Get(),
            HighestAsn.Get(),
            TruncationAsn.Get());
    }

private:
    VerificationMethod     _VerificationMethod;
};

UnitTestLogValidator::UnitTestLogValidator()
{
}

UnitTestLogValidator::~UnitTestLogValidator()
{
}





//** Common logger unit helpers
NTSTATUS
VerifyDiskLogFile(
    KAllocator& Allocator,
    WCHAR const *const TestDriveLetterStr,
    RvdLogRecoveryReportDetail ReportDetail,
    BOOLEAN UseStreamUnitTestVerification,
    LogState::SPtr *const ResultingLogState = nullptr)
{
    KWString        fullyQualifiedDiskFileName(KtlSystem::GlobalNonPagedAllocator());
    KGuid           driveGuid;
    RvdLogId        logId(TestLogIdGuid);
    NTSTATUS        status;

    if (ResultingLogState != nullptr)
    {
        *ResultingLogState = nullptr;
    }

    status = GetDriveAndPathOfLogFile(*TestDriveLetterStr, logId, driveGuid, fullyQualifiedDiskFileName);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("DiskLoggerStructureVerifyTests: GetDriveAndPathOfLogFile() Failed: %i\n", __LINE__);
        return status;
    }

    KBlockDevice::SPtr                      ioDevice;
    CompletionEvent                         openFileCompleteEvent;
    KAsyncContextBase::CompletionCallback   openFileDoneCallback;

    openFileDoneCallback.Bind(&openFileCompleteEvent, &CompletionEvent::EventCallback);
    status = KBlockDevice::CreateDiskBlockDevice(
        driveGuid,
        1024*1024*1024,
        TRUE,
        FALSE,
        fullyQualifiedDiskFileName,
        ioDevice,
        openFileDoneCallback,
        nullptr,
        Allocator,
        KTL_TAG_LOGGER,
        NULL,
        FALSE,
#ifdef ALWAYS_SPARSE_FILES
        TRUE
#else
        FALSE
#endif
                                                );

    if (!K_ASYNC_SUCCESS(status))
    {
        KDbgPrintf("DiskLoggerStructureVerifyTests: KBlockDevice::CreateDiskBlockDevice Failed: %i\n", __LINE__);
        return status;
    }
    status = openFileCompleteEvent.WaitUntilSet();
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("DiskLoggerStructureVerifyTests: KVolumeNamespace::QueryVolumeListEx Failed: %i\n", __LINE__);
        return status;
    }

    UnitTestLogValidator::SPtr              validator;
    CompletionEvent                         validateCompleteEvent;
    KAsyncContextBase::CompletionCallback   validateDoneCallback;
    validateDoneCallback.Bind(&validateCompleteEvent, &CompletionEvent::EventCallback);

    status = UnitTestLogValidator::Create(
        validator,
        KTL_TAG_LOGGER,
        Allocator,
        UseStreamUnitTestVerification
            ?
                UnitTestLogValidator::VerificationMethod::StreamUnitTestVerification
            :   UnitTestLogValidator::VerificationMethod::TestMetadataVerification);

    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("DiskLoggerStructureVerifyTests: LogValidator::Create Failed: %i\n", __LINE__);
        return status;
    }

    validator->StartValidation(ioDevice, logId, ReportDetail, nullptr, validateDoneCallback);
    status = validateCompleteEvent.WaitUntilSet();
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("DiskLoggerStructureVerifyTests: Validation Failed: %i\n", __LINE__);
        return status;
    }

    if (ResultingLogState != nullptr)
    {
        status = validator->SnapLogState(*ResultingLogState, KTL_TAG_LOGGER, Allocator);
    }

    return status;
}

NTSTATUS
ReportLogStateDifferences(__in LogState& RecoveredState, __in LogState& LogState)
{
    // BUG, richhas, xxxxx, enhance to include much more detail are the reason of difference reports
    BOOLEAN     comparedOk = TRUE;

    ULONG       sizeOfSig = sizeof(RecoveredState._LogSignature);
    if (RtlCompareMemory(&RecoveredState._LogSignature[0], &LogState._LogSignature[0], sizeOfSig) != sizeOfSig)
    {
        comparedOk = FALSE;
        KDbgPrintf("ReportLogStateDifferences: _LogSignature Compare Failed: %i\n", __LINE__);
    }

    if (RecoveredState._LogFileSize != LogState._LogFileSize)
    {
        comparedOk = FALSE;
        KDbgPrintf("ReportLogStateDifferences: _LogFileSize Compare Failed: %i\n", __LINE__);
    }

    if (RecoveredState._LogFileLsnSpace != LogState._LogFileLsnSpace)
    {
        comparedOk = FALSE;
        KDbgPrintf("ReportLogStateDifferences: _LogFileLsnSpace Compare Failed: %i\n", __LINE__);
    }

    if (RecoveredState._MaxCheckPointLogRecordSize != LogState._MaxCheckPointLogRecordSize)
    {
        comparedOk = FALSE;
        KDbgPrintf("ReportLogStateDifferences: _MaxCheckPointLogRecordSize Compare Failed: %i\n", __LINE__);
    }

    ULONGLONG     overRecoveredSpace = 0;

    if (RecoveredState._LogFileFreeSpace != LogState._LogFileFreeSpace)
    {
        if (LogState._LogFileFreeSpace < RecoveredState._LogFileFreeSpace)
        {
            // It is not valid if the freespace of the log file to be less than recovered state - implies
            // we did not recover all of the log
            comparedOk = FALSE;
            KDbgPrintf("ReportLogStateDifferences: _LogFileFreeSpace Compare Failed: %i\n", __LINE__);
        }
        else
        {
            KAssert(LogState._LogFileFreeSpace > RecoveredState._LogFileFreeSpace);
            KDbgPrintf("ReportLogStateDifferences: Warning _LogFileFreeSpace Compare Failed: May have over recovered: %i\n", __LINE__);
            overRecoveredSpace = LogState._LogFileFreeSpace - RecoveredState._LogFileFreeSpace;
        }
    }

    if (RecoveredState._LowestLsn != LogState._LowestLsn)
    {
        if (LogState._LowestLsn < RecoveredState._LowestLsn)
        {
            comparedOk = FALSE;
            KDbgPrintf("ReportLogStateDifferences: _LowestLsn Compare Failed: %i\n", __LINE__);
        }
        else
        {
            KAssert(LogState._LowestLsn.Get() > RecoveredState._LowestLsn.Get());
            ULONGLONG   overRecoveredLsnSpace = LogState._LowestLsn.Get() - RecoveredState._LowestLsn.Get();

            if (overRecoveredLsnSpace != overRecoveredSpace)
            {
                comparedOk = FALSE;
                KDbgPrintf("ReportLogStateDifferences: Under recovered LSN space does not match FreeSpace difference: %i\n", __LINE__);
            }
        }
    }

    if (RecoveredState._NextLsnToWrite != LogState._NextLsnToWrite)
    {
        comparedOk = FALSE;
        KDbgPrintf("ReportLogStateDifferences: _NextLsnToWrite Compare Failed: %i\n", __LINE__);
    }

    if (RecoveredState._HighestCompletedLsn != LogState._HighestCompletedLsn)
    {
        comparedOk = FALSE;
        KDbgPrintf("ReportLogStateDifferences: _HighestCompletedLsn Compare Failed: %i\n", __LINE__);
    }

    if (RecoveredState._HighestCheckpointLsn != LogState._HighestCheckpointLsn)
    {
        comparedOk = FALSE;
        KDbgPrintf("ReportLogStateDifferences: _HighestCheckpointLsn Compare Failed: %i\n", __LINE__);
    }

    if (RecoveredState._NumberOfStreams != LogState._NumberOfStreams)
    {
        comparedOk = FALSE;
        KDbgPrintf("ReportLogStateDifferences: _NumberOfStreams Compare Failed: %i\n", __LINE__);
    }

    if (!comparedOk)
    {
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    for (ULONG ix0 = 0; ix0 < RecoveredState._NumberOfStreams; ix0++)
    {
        KWString    streamId(KtlSystem::GlobalNonPagedAllocator(), RecoveredState._StreamDescs[ix0]._Info.LogStreamId.Get());
        ULONG       ix1;

        for (ix1 = 0; ix0 < LogState._NumberOfStreams; ix1++)
        {
            if (RecoveredState._StreamDescs[ix0]._Info.LogStreamId == LogState._StreamDescs[ix1]._Info.LogStreamId)
            {
                LogState::StreamDescriptor& recoveredDesc = RecoveredState._StreamDescs[ix0];
                LogState::StreamDescriptor& logDesc = LogState._StreamDescs[ix1];

                if (recoveredDesc._Info.LogStreamType != logDesc._Info.LogStreamType)
                {
                    comparedOk = FALSE;
#if defined(PLATFORM_UNIX)
                    KDbgPrintf(
                        "ReportLogStateDifferences: LogStreamType Compare Failed for stream %s: %i\n",
                        Utf16To8((WCHAR*)streamId).c_str(),
                        __LINE__);
#else
                    KDbgPrintf(
                        "ReportLogStateDifferences: LogStreamType Compare Failed for stream %S: %i\n",
                        (WCHAR*)streamId,
                        __LINE__);
#endif
                }

                if (recoveredDesc._Info.HighestLsn != logDesc._Info.HighestLsn)
                {
                    comparedOk = FALSE;
#if defined(PLATFORM_UNIX)
                    KDbgPrintf(
                        "ReportLogStateDifferences: HighestLsn Compare Failed for stream %s: %i\n",
                        Utf16To8((WCHAR*)streamId).c_str(),
                        __LINE__);
#else
                    KDbgPrintf(
                        "ReportLogStateDifferences: HighestLsn Compare Failed for stream %S: %i\n",
                        (WCHAR*)streamId,
                        __LINE__);
#endif
                }

                if (recoveredDesc._Info.NextLsn != logDesc._Info.NextLsn)
                {
                    comparedOk = FALSE;
#if defined(PLATFORM_UNIX)
                    KDbgPrintf(
                        "ReportLogStateDifferences: NextLsn Compare Failed for stream %s: %i\n",
                        Utf16To8((WCHAR*)streamId).c_str(),
                        __LINE__);
#else
                    KDbgPrintf(
                        "ReportLogStateDifferences: NextLsn Compare Failed for stream %S: %i\n",
                        (WCHAR*)streamId,
                        __LINE__);
#endif
                }

                if (recoveredDesc._Info.LowestLsn != logDesc._Info.LowestLsn)
                {
                    if (recoveredDesc._Info.LowestLsn.Get() > logDesc._Info.LowestLsn.Get())
                    {
                        // under recovered stream state
                        comparedOk = FALSE;
#if defined(PLATFORM_UNIX)
                        KDbgPrintf(
                            "ReportLogStateDifferences: LowestLsn Compare Failed (under recovered) for stream %s: %i\n",
                            Utf16To8((WCHAR*)streamId).c_str(),
                            __LINE__);
#else
                        KDbgPrintf(
                            "ReportLogStateDifferences: LowestLsn Compare Failed (under recovered) for stream %S: %i\n",
                            (WCHAR*)streamId,
                            __LINE__);
#endif
                    }
                }

                // Attempt to truncate over recovered state
                ULONGLONG                   streamOverRecoveredLsnSpace = 0;
                RvdAsnIndexEntry::SPtr      aEntry = recoveredDesc._AsnIndex->UnsafeFirst();

                while ((aEntry != nullptr) && (aEntry->GetAsn() <= logDesc._TruncationPoint))
                {
                    recoveredDesc._TruncationPoint = aEntry->GetAsn();
                    streamOverRecoveredLsnSpace += aEntry->GetIoBufferSizeHint();
                    aEntry = recoveredDesc._AsnIndex->UnsafeNext(aEntry);
                }

                if (streamOverRecoveredLsnSpace > 0)
                {
                    // Attempt to truncate the recovered state to the log state to account for the stream difference
                    KAssert(recoveredDesc._TruncationPoint == logDesc._TruncationPoint);
                    RvdLogLsn tLsn = recoveredDesc._AsnIndex->Truncate(logDesc._TruncationPoint, recoveredDesc._Info.HighestLsn);
                    KAssert(tLsn == logDesc._Info.LowestLsn);
                    recoveredDesc._LsnIndex->Truncate(tLsn);
                }

                if (recoveredDesc._AsnIndex->GetNumberOfEntries() != logDesc._AsnIndex->GetNumberOfEntries())
                {
                    comparedOk = FALSE;
#if defined(PLATFORM_UNIX)
                    KDbgPrintf(
                        "ReportLogStateDifferences: GetNumberOfEntries (Asn) Compare Failed for stream %s: %i\n",
                        Utf16To8((WCHAR*)streamId).c_str(),
                        __LINE__);
#else
                    KDbgPrintf(
                        "ReportLogStateDifferences: GetNumberOfEntries (Asn) Compare Failed for stream %S: %i\n",
                        (WCHAR*)streamId,
                        __LINE__);
#endif
                }

                if (recoveredDesc._LsnIndex->QueryNumberOfRecords() != logDesc._LsnIndex->QueryNumberOfRecords())
                {
                    comparedOk = FALSE;
#if defined(PLATFORM_UNIX)
                    KDbgPrintf(
                        "ReportLogStateDifferences: QueryNumberOfRecords (Lsn) Compare Failed for stream %s: %i\n",
                        Utf16To8((WCHAR*)streamId).c_str(),
                        __LINE__);
#else
                    KDbgPrintf(
                        "ReportLogStateDifferences: QueryNumberOfRecords (Lsn) Compare Failed for stream %S: %i\n",
                        (WCHAR*)streamId,
                        __LINE__);
#endif
                }

                if (recoveredDesc._TruncationPoint != logDesc._TruncationPoint)
                {
                    comparedOk = FALSE;
#if defined(PLATFORM_UNIX)
                    KDbgPrintf(
                        "ReportLogStateDifferences: _TruncationPoint Compare Failed for stream %s: %i\n",
                        Utf16To8((WCHAR*)streamId).c_str(),
                        __LINE__);
#else
                    KDbgPrintf(
                        "ReportLogStateDifferences: _TruncationPoint Compare Failed for stream %S: %i\n",
                        (WCHAR*)streamId,
                        __LINE__);
#endif
                }

                // Verify the _AsnIndex's match
                {
                    RvdAsnIndexEntry::SPtr  recoveredAsnEntry = recoveredDesc._AsnIndex->UnsafeFirst();
                    RvdAsnIndexEntry::SPtr  logAsnEntry = logDesc._AsnIndex->UnsafeFirst();

                    for (ULONG aIx = 0; aIx < logDesc._AsnIndex->GetNumberOfEntries(); aIx++)
                    {
                        if ((recoveredAsnEntry == nullptr) || (logAsnEntry == nullptr))
                        {
                            comparedOk = FALSE;
#if defined(PLATFORM_UNIX)
                            KDbgPrintf(
                                "ReportLogStateDifferences: UnsafeFirst() returned nullptr for stream %s: %i\n",
                                Utf16To8((WCHAR*)streamId).c_str(),
                                __LINE__);
#else
                            KDbgPrintf(
                                "ReportLogStateDifferences: UnsafeFirst() returned nullptr for stream %S: %i\n",
                                (WCHAR*)streamId,
                                __LINE__);
#endif

                            break;
                        }

                        if (recoveredAsnEntry->GetAsn() != logAsnEntry->GetAsn())
                        {
                            comparedOk = FALSE;
#if defined(PLATFORM_UNIX)
                            KDbgPrintf(
                                "ReportLogStateDifferences: GetAsn() mismatch for stream %s: %i\n",
                                Utf16To8((WCHAR*)streamId).c_str(),
                                __LINE__);
#else
                            KDbgPrintf(
                                "ReportLogStateDifferences: GetAsn() mismatch for stream %S: %i\n",
                                (WCHAR*)streamId,
                                __LINE__);
#endif

                            break;
                        }

                        if (recoveredAsnEntry->GetLsn() != logAsnEntry->GetLsn())
                        {
                            comparedOk = FALSE;
#if defined(PLATFORM_UNIX)
                            KDbgPrintf(
                                "ReportLogStateDifferences: GetLsn() mismatch for stream %s: %i\n",
                                Utf16To8((WCHAR*)streamId).c_str(),
                                __LINE__);
#else
                            KDbgPrintf(
                                "ReportLogStateDifferences: GetLsn() mismatch for stream %S: %i\n",
                                (WCHAR*)streamId,
                                __LINE__);
#endif

                            break;
                        }

                        if (recoveredAsnEntry->GetVersion() != logAsnEntry->GetVersion())
                        {
                            comparedOk = FALSE;
#if defined(PLATFORM_UNIX)
                            KDbgPrintf(
                                "ReportLogStateDifferences: GetVersion() mismatch for stream %s: %i\n",
                                Utf16To8((WCHAR*)streamId).c_str(),
                                __LINE__);
#else
                            KDbgPrintf(
                                "ReportLogStateDifferences: GetVersion() mismatch for stream %S: %i\n",
                                (WCHAR*)streamId,
                                __LINE__);
#endif

                            break;
                        }

                        if (recoveredAsnEntry->GetDisposition() != logAsnEntry->GetDisposition())
                        {
                            comparedOk = FALSE;
#if defined(PLATFORM_UNIX)
                            KDbgPrintf(
                                "ReportLogStateDifferences: GetDisposition() mismatch for stream %s: %i\n",
                                Utf16To8((WCHAR*)streamId).c_str(),
                                __LINE__);
#else
                            KDbgPrintf(
                                "ReportLogStateDifferences: GetDisposition() mismatch for stream %S: %i\n",
                                (WCHAR*)streamId,
                                __LINE__);
#endif

                            break;
                        }

                        if (recoveredAsnEntry->GetIoBufferSizeHint() != logAsnEntry->GetIoBufferSizeHint())
                        {
                            comparedOk = FALSE;
#if defined(PLATFORM_UNIX)
                            KDbgPrintf(
                                "ReportLogStateDifferences: GetIoBufferSizeHint() mismatch for stream %s: %i\n",
                                Utf16To8((WCHAR*)streamId).c_str(),
                                __LINE__);
#else
                            KDbgPrintf(
                                "ReportLogStateDifferences: GetIoBufferSizeHint() mismatch for stream %S: %i\n",
                                (WCHAR*)streamId,
                                __LINE__);
#endif

                            break;
                        }

                        if (recoveredAsnEntry->GetLowestLsnOfHigherASNs() != logAsnEntry->GetLowestLsnOfHigherASNs())
                        {
                            comparedOk = FALSE;
#if defined(PLATFORM_UNIX)
                            KDbgPrintf(
                                "ReportLogStateDifferences: GetLowestLsnOfHigherASNs() mismatch for stream %s: %i\n",
                                Utf16To8((WCHAR*)streamId).c_str(),
                                __LINE__);
#else
                            KDbgPrintf(
                                "ReportLogStateDifferences: GetLowestLsnOfHigherASNs() mismatch for stream %S: %i\n",
                                (WCHAR*)streamId,
                                __LINE__);
#endif

                            break;
                        }

                        recoveredAsnEntry = recoveredDesc._AsnIndex->UnsafeNext(recoveredAsnEntry);
                        logAsnEntry = logDesc._AsnIndex->UnsafeNext(logAsnEntry);
                    }
                }

                // Verify that the _LsnIndex's match
                {
                    ULONG       todo = __min(recoveredDesc._LsnIndex->QueryNumberOfRecords(), logDesc._LsnIndex->QueryNumberOfRecords());
                    for (ULONG lIx = 0; lIx < todo; lIx++)
                    {
                        NTSTATUS    status;

                        RvdLogLsn   lsn0;
                        ULONG       headerAndMetadataSize0;
                        ULONG       ioBufferSize0;

                        status = recoveredDesc._LsnIndex->QueryRecord(lIx, lsn0, &headerAndMetadataSize0, &ioBufferSize0);
                        if (!NT_SUCCESS(status))
                        {
                            comparedOk = FALSE;
#if defined(PLATFORM_UNIX)
                            KDbgPrintf(
                                "ReportLogStateDifferences: QueryRecord() Failed for stream %s: Status: 0x%08X, %i\n",
                                Utf16To8((WCHAR*)streamId).c_str(),
                                status,
                                __LINE__);
#else
                            KDbgPrintf(
                                "ReportLogStateDifferences: QueryRecord() Failed for stream %S: Status: 0x%08X, %i\n",
                                (WCHAR*)streamId,
                                status,
                                __LINE__);
#endif
                        }

                        RvdLogLsn   lsn1;
                        ULONG       headerAndMetadataSize1;
                        ULONG       ioBufferSize1;

                        status = logDesc._LsnIndex->QueryRecord(lIx, lsn1, &headerAndMetadataSize1, &ioBufferSize1);
                        if (!NT_SUCCESS(status))
                        {
                            comparedOk = FALSE;
#if defined(PLATFORM_UNIX)
                            KDbgPrintf(
                                "ReportLogStateDifferences: QueryRecord() Failed for stream %s: Status: 0x%08X, %i\n",
                                Utf16To8((WCHAR*)streamId).c_str(),
                                status,
                                __LINE__);
#else
                            KDbgPrintf(
                                "ReportLogStateDifferences: QueryRecord() Failed for stream %S: Status: 0x%08X, %i\n",
                                (WCHAR*)streamId,
                                status,
                                __LINE__);
#endif
                        }

                        if (lsn0 != lsn1)
                        {
                            comparedOk = FALSE;
#if defined(PLATFORM_UNIX)
                            KDbgPrintf(
                                "ReportLogStateDifferences: LSN mismatch for stream %s: %i\n",
                                Utf16To8((WCHAR*)streamId).c_str(),
                                __LINE__);
#else
                            KDbgPrintf(
                                "ReportLogStateDifferences: LSN mismatch for stream %S: %i\n",
                                (WCHAR*)streamId,
                                __LINE__);
#endif
                        }

                        if (headerAndMetadataSize0 != headerAndMetadataSize1)
                        {
                            comparedOk = FALSE;
#if defined(PLATFORM_UNIX)
                            KDbgPrintf(
                                "ReportLogStateDifferences: headerAndMetadataSize mismatch for stream %s: %i\n",
                                Utf16To8((WCHAR*)streamId).c_str(),
                                __LINE__);
#else
                            KDbgPrintf(
                                "ReportLogStateDifferences: headerAndMetadataSize mismatch for stream %S: %i\n",
                                (WCHAR*)streamId,
                                __LINE__);
#endif
                        }

                        if (ioBufferSize0 != ioBufferSize1)
                        {
                            comparedOk = FALSE;
#if defined(PLATFORM_UNIX)
                            KDbgPrintf(
                                "ReportLogStateDifferences: ioBufferSize mismatch for stream %s: %i\n",
                                Utf16To8((WCHAR*)streamId).c_str(),
                                __LINE__);
#else
                            KDbgPrintf(
                                "ReportLogStateDifferences: ioBufferSize mismatch for stream %S: %i\n",
                                (WCHAR*)streamId,
                                __LINE__);
#endif
                        }
                    }
                }

                break;
            }
        }

        if (ix1 ==  LogState._NumberOfStreams)
        {
            comparedOk = FALSE;
#if defined(PLATFORM_UNIX)
            KDbgPrintf("ReportLogStateDifferences: Stream %s is missing: %i\n", Utf16To8((WCHAR*)streamId).c_str(), __LINE__);
#else
            KDbgPrintf("ReportLogStateDifferences: Stream %S is missing: %i\n", (WCHAR*)streamId, __LINE__);
#endif
        }
    }

    return comparedOk ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}


//** Main Test Entry Point: DiskLoggerStructureVerifyTests
NTSTATUS
DiskLoggerStructureVerifyTests(__in int argc, __in_ecount(argc) WCHAR* args[])
{
    KDbgPrintf("DiskLoggerStructureVerifyTests: Starting\n");

    EventRegisterMicrosoft_Windows_KTL();
    KFinally([]()
    {
        EventUnregisterMicrosoft_Windows_KTL();
    });

    KtlSystem* underlyingSystem;
    NTSTATUS status = KtlSystem::Initialize(FALSE,
                                           &underlyingSystem);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("DiskLoggerStructureVerifyTests: KtlSystem::Initialize Failed: %i\n", __LINE__);
        return status;
    }

    KFinally([]()
    {
        KtlSystem::Shutdown();
    });

    if (argc != 1)
    {
        KDbgPrintf("DiskLoggerStructureVerifyTests: Drive Letter Test Parameter Missing: %i\n", __LINE__);
        return STATUS_INVALID_PARAMETER_2;
    }

    // Run basic stream test to create a log file to be examined.
    LogState::SPtr       logStateAfterTest;
    status = BasicLogStreamTest(*(args[0]), FALSE, &logStateAfterTest);     // FALSE: Leave the stream created undeleted
    if (NT_SUCCESS(status))
    {
        LogState::SPtr   verifiedState;
        status = VerifyDiskLogFile(
            KtlSystem::GlobalNonPagedAllocator(),
            args[0],
            RvdLogRecoveryReportDetail::Normal,
            FALSE,      // Use user record verification method for this test
            &verifiedState);

        if (NT_SUCCESS(status) && (logStateAfterTest != nullptr))
        {
            status = ReportLogStateDifferences(
                *verifiedState,
                *logStateAfterTest);

            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("DiskLoggerStructureVerifyTests: ReportLogStateDifferences failed: %i\n", __LINE__);
            }
        }

        verifiedState = nullptr;
    }
    logStateAfterTest = nullptr;

    KDbgPrintf("DiskLoggerStructureVerifyTests: Completed");
    return status;
}
