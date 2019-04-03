// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define LOG_OUTPUT(fmt, ...) Log::Comment(String().Format(fmt, __VA_ARGS__))

extern KAllocator* g_Allocator;

VOID
ManagerTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );

VOID
SetConfigurationTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );

VOID
VersionTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );

VOID
ForceCloseTest(
    KGuid diskId
);

VOID
ForceCloseOnDeleteContainerWaitTest(
    KGuid diskId
);


VOID
IoctlTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );

VOID
MissingStreamFileTest(
    UCHAR driveLetter,
    KtlLogManager::SPtr logManager
    );

VOID
OneBitLogCorruptionTest(
    UCHAR driveLetter,
    KtlLogManager::SPtr logManager
    );

VOID
CorruptedLCMBInfoTest(
    UCHAR driveLetter,
    KtlLogManager::SPtr logManager
    );

VOID
ContainerTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );

VOID
ContainerLimitsTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );

VOID
WrapStreamFileTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );

VOID
ContainerConfigurationTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );


VOID
PartialContainerCreateTest(
    UCHAR driveLetter,
    KtlLogManager::SPtr logManager
    );

VOID
ContainerWithPathTest(
    UCHAR driveLetter,
    KtlLogManager::SPtr logManager
    );

VOID
DeleteOpenStreamTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );

VOID
EnunerateStreamsTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );

VOID
StreamTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );

VOID
VerifySparseTruncateTest(
    UCHAR driveLetter,
    KtlLogManager::SPtr logManager
);

VOID
ConfiguredMappedLogTest(
    UCHAR driveLetter,
    KtlLogManager::SPtr logManager
);

VOID
WriteTruncateAndRecoverTest(
    UCHAR driveLetter,
    KtlLogManager::SPtr logManager
);

VOID
RetryOnSharedLogFullTest(
    KGuid& diskId
);

VOID
OpenCloseStreamTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );

VOID
ReadAsnTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );

VOID
StreamMetadataTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );

VOID
MultiRecordReadTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );

VOID
AliasTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );

VOID
AliasStress(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );

VOID
TruncateTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );

VOID
FailCoalesceFlushTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );

VOID
QueryRecordsTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );

VOID
ForcePreAllocReallocTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );

VOID
ManyKIoBufferElementsTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );

VOID
ManyVersionsTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );

VOID
ReservationsTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );

VOID
CloseTestVerifyContainerOpened(
    KtlLogContainer::SPtr logContainer
    );

VOID
CloseTestVerifyContainerClosed(
    KtlLogContainer::SPtr logContainer
    );

VOID
CloseTestVerifyStreamOpened(
    KtlLogStream::SPtr logStream
);

VOID
CloseTestVerifyStreamClosed(
    KtlLogStream::SPtr logStream
);

VOID
CloseTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );


VOID
CloseOpenRaceTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );

VOID
OpenWriteCloseTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );

VOID
OpenRightAfterCloseTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );

VOID
Close2Test(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );

VOID
ThresholdTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );

VOID
Stress1Test(
    KGuid diskId,
    ULONG logManagerCount,
    KtlLogManager::SPtr* logManagers
    );

VOID
CancelStressTest(
    ULONG NumberLoops,
    ULONG NumberAsyncs,
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );

VOID
DestagerTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );

VOID
LLRecordMarkerRecoveryTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );


VOID
LLDataOverwriteTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );

VOID
LLCoalesceDataTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
);

VOID
LLDataTruncateTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );

VOID
LLBarrierRecoveryTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );

VOID
LLDataWorkloadTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );

VOID
EnumContainersTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );

VOID CleanupTests(
    KGuid& DiskId,
    ULONG LogManagerCount,
    KtlLogManager::SPtr* LogManagers,
    ULONGLONG& StartingAllocs,
    KtlSystem*& System
    );

VOID SetupTests(
    KGuid& DiskId,
    UCHAR& DriveLetter,
    ULONG LogManagerCount,
    KtlLogManager::SPtr* LogManagers,
    ULONGLONG& StartingAllocs,
    KtlSystem*& System
    );

VOID CleanupMiscTests(
    KGuid& DiskId,
    ULONGLONG& StartingAllocs,
    KtlSystem*& System
    );

VOID SetupMiscTests(
    KGuid& DiskId,
    UCHAR& DriveLetter,
    ULONGLONG& StartingAllocs,
    KtlSystem*& System
    );

NTSTATUS FindDiskIdForDriveLetter(
    UCHAR& DriveLetter,
    GUID& DiskIdGuid
    );

VOID
DLogReservationTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );

VOID
DeletedDedicatedLogTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
);


VOID
AccelerateFlushTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );

VOID
ServiceWrapperStressTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );

VOID
OpenSpecificStreamTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );


VOID
ThrottledLockedMemoryTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );

//
// MBINfo tests
//
VOID CleanupMBInfoTests(
    KGuid& DiskId,
    ULONGLONG& StartingAllocs,
    KtlSystem*& System
    );

VOID SetupMBInfoTests(
    KGuid& DiskId,
    ULONGLONG& StartingAllocs,
    KtlSystem*& System
    );

VOID BasicMBInfoTest(
    KGuid& DiskId,
    ULONG NumberOfLoops,
    ULONG NumberOfBlocks,
    ULONG BlockSize
);

VOID BasicMBInfoDataIntegrityTest(
    KGuid& DiskId
);

VOID FailOpenCorruptedMBInfoTest(
    KGuid& DiskId
    );

VOID SharedLCMBInfoTest(
    KGuid& DiskId,
    ULONG NumberOfLoops,
    ULONG NumberOfEntries
);

VOID DedicatedLCMBInfoTest(
    KGuid& DiskId,
    ULONG NumberOfLoops
);

//
// Raw Logger tests
//
void OpenCloseLogManagerStressTest();

VOID ReuseLogManagerTest(
    KGuid& DiskId,
    ULONG LogManagerCount
    );

VOID ReuseLogManager2Test(
    KGuid& DiskId
    );

VOID CreateLogContainerWithBadPathTest(
    UCHAR DriveLetter
    );

VOID SetupRawKtlLoggerTests(
    KGuid& DiskId,
    UCHAR& DriveLetter,
    ULONGLONG& StartingAllocs,
    KtlSystem*& System
    );

VOID CleanupRawKtlLoggerTests(
    KGuid& DiskId,
    ULONGLONG& StartingAllocs,
    KtlSystem*& System
    );

//
// OverlayStream Tests
//
VOID ReadTest(
    KGuid& diskId
    );

VOID RecoveryTest(
    KGuid& DiskId
    );

VOID Recovery2Test(
    KGuid& DiskId
    );

VOID ThrottledAllocatorTest(
    KGuid& DiskId
    );

VOID OfflineDestageContainerTest(
    KGuid& DiskId
    );

VOID MultiRecordCornerCaseTest(
    KGuid& DiskId
    );

VOID MultiRecordCornerCase2Test(
    KGuid& DiskId
    );

VOID RecoveryViaOpenContainerTest(
    KGuid& DiskId
    );

VOID RecoveryPartlyCreatedStreamTest(
    KGuid& DiskId
    );

VOID ReadRaceConditionsTest(
    KGuid& diskId
    );

VOID PeriodicTimerCloseRaceTest(
    KGuid& diskId
    );

VOID DiscontiguousRecordsRecoveryTest(
    KGuid& diskId
    );
   
VOID WriteStuckConditionsTest(
    KGuid& diskId
    );

VOID FlushAllRecordsForCloseWaitTest(
    KGuid& diskId
    );

VOID VerifyCopyFromSharedToBackupTest(
    UCHAR DriveLetter,
    KGuid& diskId
    );

VOID VerifySharedTruncateOnDedicatedFailureTest(
    KGuid& diskId
    );

VOID WriteThrottleUnitTest(
    KGuid& diskId
    );

VOID ReadFromCoalesceBuffersTest(
    KGuid& diskId
    );

VOID ReadWriteRaceTest(
    KGuid& diskId
    );

VOID ReadFromCoalesceBuffersCornerCase1Test(
    KGuid& diskId
    );

VOID ReadFromCoalesceBuffersTruncateTailTest(
    KGuid& diskId
    );

VOID VerifySharedLogUsageThrottlingTest(
    KGuid& diskId
    );

VOID DestagedWriteTest(
    KGuid& diskId
    );

VOID CoalescedWriteTest(
    KGuid& diskId
    );
VOID CoalescedWrite2Test(
    KGuid& diskId
    );

VOID SetupOverlayLogTests(
    KGuid& DiskId,
    UCHAR& driveLetter,
    ULONGLONG& StartingAllocs,
    KtlSystem*& System
    );

VOID CleanupOverlayLogTests(
    KGuid& DiskId,
    UCHAR& driveLetter,
    ULONGLONG& StartingAllocs,
    KtlSystem*& System
    );

VOID StreamQuotaTest(
    KGuid& diskId
    );

VOID DeleteTruncatedTailRecords1Test(
    KGuid& diskId
    );

//
// Raw Core logger tests
//
VOID TruncateToBadLogLowLsnTest(
    KGuid DiskId
    );

VOID StreamCheckpointBoundaryTest(
    KGuid DiskId
    );

VOID QueryLogTypeTest(
    KGuid DiskId
    );

VOID DeleteRecordsTest(
    KGuid DiskId
    );

VOID StreamCheckpointAtEndOfLogTest(
    KGuid DiskId
    );

VOID DuplicateRecordInLogTest(
    KGuid DiskId
    );

VOID CorruptedRecordTest(
    KGuid DiskId
    );

VOID SetupRawCoreLoggerTests(
    KGuid& DiskId,
    ULONGLONG& StartingAllocs,
    KtlSystem*& System
    );

VOID CleanupRawCoreLoggerTests(
    KGuid& DiskId,
    ULONGLONG& StartingAllocs,
    KtlSystem*& System
    );


//
// Useful functions
//
NTSTATUS CompareKIoBuffers(
    KIoBuffer& IoBufferA,
    KIoBuffer& IoBufferB,
    ULONG HeaderBytesToSkip = 0
);

NTSTATUS CreateAndFillIoBuffer(
    UCHAR Entropy,
    ULONG BufferSize,
    KIoBuffer::SPtr& IoBuffer
);

NTSTATUS TruncateStream(
    KtlLogStream::SPtr& logStream,
    KtlLogAsn recordAsn
);

VOID DeleteAllContainersOnDisk(
    KGuid diskId
    );

VOID DeleteAllContainersOnDisk(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );

NTSTATUS ValidateRecordData(
    ULONGLONG StreamOffsetExpected,
    ULONG DataSizeExpected,
    PUCHAR DataExpected,
    ULONGLONG VersionExpected,
    KtlLogStreamId& StreamIdExpected,
    ULONG metadataSizeRead,
    KIoBuffer::SPtr metadataIoBufferRead,
    KIoBuffer::SPtr dataIoBufferRead,
    ULONG reservedMetadataSize,
    BOOLEAN exactMatch
    );

NTSTATUS
VerifyRawRecordCallback(
    __in_bcount(MetaDataBufferSize) UCHAR const *const MetaDataBuffer,
    __in ULONG MetaDataBufferSize,
    __in const KIoBuffer::SPtr& IoBuffer
);

VOID
CreateStreamAndContainerPathnames(
    __in UCHAR DriveLetter,
    __out KString::SPtr& containerFullPathName,
    __out KtlLogContainerId& LogContainerId,
    __out KString::SPtr& streamFullPathName,
    __out KtlLogStreamId& LogStreamId
);
