// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace LogTests
{
    class LogTestBase 
        : public KObject<LogTestBase>
        , public KShared<LogTestBase>
    {
        K_FORCE_SHARED_WITH_INHERITANCE(LogTestBase);

    public:

        static volatile bool g_UseKtlFilePrefix;
        static LPCWSTR GetTestDirectoryPath();

        virtual NTSTATUS CreateAndOpenLogManager(__out ILogManagerHandle::SPtr& logManager) = 0;
        virtual NTSTATUS CreateDefaultPhysicalLog(
            __in ILogManagerHandle& logManager,
            __out IPhysicalLogHandle::SPtr& physicalLog) = 0;
        virtual NTSTATUS CreatePhysicalLog(
            __in ILogManagerHandle& logManager,
            __in KGuid const & id,
            __in KString const & path,
            __out IPhysicalLogHandle::SPtr& physicalLog) = 0;
        virtual NTSTATUS OpenPhysicalLog(
            __in ILogManagerHandle& logManager,
            __in KGuid const & id,
            __in KString const & path,
            __out IPhysicalLogHandle::SPtr& physicalLog) = 0;
        virtual NTSTATUS OpenDefaultPhysicalLog(
            __in ILogManagerHandle& logManager,
            __out IPhysicalLogHandle::SPtr& physicalLog) = 0;
        virtual NTSTATUS CreateLogicalLog(
            __in IPhysicalLogHandle& physicalLog,
            __in KGuid const & id,
            __in KString const & path,
            __out ILogicalLog::SPtr& logicalLog) = 0;
        virtual NTSTATUS OpenLogicalLog(
            __in IPhysicalLogHandle& physicalLog,
            __in KGuid const & id,
            __in KString const & path,
            __out ILogicalLog::SPtr& logicalLog) = 0;
        virtual VOID VerifyState(
            __in_opt LogManager* logManager = nullptr,
            __in int logManagerHandlesCountExpected = -1,
            __in int logManagerLogsCountExpected = -1,
            __in BOOLEAN logManagerIsLoadedExpected = FALSE,
            __in_opt PhysicalLog* physicalLog0 = nullptr,
            __in int physicalLog0HandlesCountExpected = -1,
            __in int physicalLog0LogsCountExpected = -1,
            __in BOOLEAN physicalLog0IsOpenExpected = FALSE,
            __in_opt PhysicalLogHandle* physicalLogHandle0 = nullptr,
            __in BOOLEAN physicalLogHandle0IsFunctionalExpected = FALSE,
            __in_opt PhysicalLog* physicalLog1 = nullptr,
            __in int physicalLog1HandlesCountExpected = -1,
            __in int physicalLog1LogsCountExpected = -1,
            __in BOOLEAN physicalLog1IsOpenExpected = FALSE,
            __in_opt PhysicalLogHandle* physicalLogHandle1 = nullptr,
            __in BOOLEAN physicalLogHandle1IsFunctionalExpected = FALSE,
            __in BOOLEAN physicalLogHandlesOwnersEqualExpected = FALSE,
            __in_opt LogicalLog* logicalLog = nullptr,
            __in BOOLEAN logicalLogIsFunctionalExpected = FALSE) = 0;

        VOID OpenManyStreamsTest(__in KtlLoggerMode ktlLoggerMode);
        VOID BasicIOTest(__in KtlLoggerMode ktlLoggerMode, __in KGuid const & physicalLogId);
        VOID WriteAtTruncationPointsTest(__in KtlLoggerMode ktlLoggerMode);
        VOID RemoveFalseProgressTest(__in KtlLoggerMode ktlLoggerMode);
        VOID ReadAheadCacheTest(__in KtlLoggerMode ktlLoggerMode);
        VOID TruncateInDataBufferTest(__in KtlLoggerMode ktlLoggerMode);
        VOID SequentialAndRandomStreamTest(__in KtlLoggerMode ktlLoggerMode);
        VOID ReadWriteCloseRaceTest(__in KtlLoggerMode ktlLoggerMode);
        VOID UPassthroughErrorsTest(__in KtlLoggerMode ktlLoggerMode);

        ktl::Awaitable<void> ReadWriteCloseRaceWorker(
            __in ILogicalLog& logicalLog,
            __in int maxSize,
            __inout volatile ULONGLONG & numTruncates,
            __in int fileSize,
            __in int taskId,
            __in ktl::CancellationToken const & cancellationToken);

        VOID RestoreEOFState(
            __in Data::Log::ILogicalLog& log,
            __in LONG dataSize,
            __in LONG prefetchSize);

        VOID GenerateUniqueFilename(__out KString::SPtr& filename);

        VOID CreateTestDirectory();
        VOID DeleteTestDirectory();

        VOID BuildDataBuffer(
            __in KBuffer& buffer,
            __in LONGLONG positionInStream,
            __in LONG entropy = 0,
            __in UCHAR recordSize = 255);

        VOID ValidateDataBuffer(
            __in KBuffer& buffer,
            __in ULONG length,
            __in ULONG offsetInBuffer,
            __in LONGLONG positionInStream,
            __in ULONG entropy = 0,
            __in UCHAR recordSize = 255);

        VOID AllocBuffer(
            __in LONG length,
            __out KBuffer::SPtr& buffer,
            __out PUCHAR& bufferPtr);

    private:

        ktl::Awaitable<NTSTATUS> TruncateLogicalLogs(
            __in ILogicalLog::SPtr logicalLogs[],
            __in int numLogicalLogs);
    };

    inline LogTestBase::LogTestBase() {}
    inline LogTestBase::~LogTestBase() {}
}
