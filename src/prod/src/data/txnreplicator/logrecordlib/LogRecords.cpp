// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::LogRecordLib;
using namespace TxnReplicator;
using namespace Data::Utilities;

LONG UpdateStartingPositionAfterBytes = 1024 * 1024;

LogRecords::LogRecords(
    __in Data::Utilities::PartitionedReplicaId const & traceId,
    __in ILogManagerReadOnly & logManager,
    __in ULONG64 enumerationStartingPosition,
    __in ULONG64 enumerationEndingPosition,
    __in LONG64 enumerationStartedAtLsn,
    __in LONG64 readAheadCacheSize,
    __in KStringView const & readerName,
    __in LogReaderType::Enum readerType,
    __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
    __in TRInternalSettingsSPtr const & config)
    : IAsyncEnumerator()
    , KObject()
    , KShared()
    , isDisposed_(false)
    , logManager_(&logManager)
    , enumerationEndingPosition_(enumerationEndingPosition)
    , readerName_()
    , readerType_(readerType)
    , readStream_(logManager.CreateReaderStream())
    , enumerationStartingPosition_(enumerationStartingPosition)
    , enumerationStartAtLsn_(enumerationStartedAtLsn)
    , currentRecord_()
    , lastPhysicalRecord_()
    , invalidLogRecords_(logManager.InvalidLogRecords)
    , transactionalReplicatorConfig_(config)
{
    ASSERT_IFNOT(
        enumerationStartingPosition <= enumerationEndingPosition,
        "LogRecords : Enumeration start position must be less than or equal to end position");

    BOOLEAN result = readerName_.Concat(readerName);

    ASSERT_IFNOT(
        result == TRUE,
        "LogRecords : Failed to concatenate readerName {0}",
        ToStringLiteral(readerName)); // otherwise the reader is too long

    Reset();

    ioMonitor_ = IOMonitor::Create(
        traceId,
        readerName_.operator LPCWSTR(),
        Common::SystemHealthReportCode::TR_SlowIO,
        transactionalReplicatorConfig_,
        healthClient,
        GetThisAllocator());

    bool ret = logManager.AddLogReader(
        enumerationStartedAtLsn,
        enumerationStartingPosition,
        enumerationEndingPosition,
        readerName,
        readerType);

    ASSERT_IFNOT(
        ret, 
        "LogRecords : Failed to add log reader");

    logManager.SetSequentialAccessReadSize(*readStream_, readAheadCacheSize);
}

LogRecords::LogRecords(
    __in PartitionedReplicaId const & traceId,
    __in Data::Log::ILogicalLogReadStream & readStream,
    __in InvalidLogRecords & invalidLogRecords,
    __in ULONG64 enumerationStartingPosition,
    __in ULONG64 enumerationEndingPosition,
    __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
    __in TRInternalSettingsSPtr const & config)
    : IAsyncEnumerator()
    , KObject()
    , KShared()
    , isDisposed_(false)
    , logManager_(nullptr)
    , enumerationEndingPosition_(enumerationEndingPosition)
    , readerName_()
    , readerType_(LogReaderType::Enum::Default)
    , readStream_(&readStream)
    , enumerationStartingPosition_(enumerationStartingPosition)
    , enumerationStartAtLsn_(MAXLONG64)
    , currentRecord_()
    , lastPhysicalRecord_()
    , invalidLogRecords_(&invalidLogRecords)
    , transactionalReplicatorConfig_(config)
{
    ASSERT_IFNOT(
        enumerationStartingPosition <= enumerationEndingPosition,
        "LogRecords : Enumeration start position must be less than or equal to end position");

    ioMonitor_ = IOMonitor::Create(
        traceId,
        Constants::SlowPhysicalLogReadOperationName,
        Common::SystemHealthReportCode::TR_SlowIO,
        transactionalReplicatorConfig_,
        healthClient,
        GetThisAllocator());

    Reset();
}

LogRecords::~LogRecords()
{
    ASSERT_IFNOT(
        isDisposed_, 
        "Log records object is not disposed on destructor");
}

IAsyncEnumerator<LogRecord::SPtr>::SPtr LogRecords::Create(
    __in PartitionedReplicaId const & traceId,
    __in ILogManagerReadOnly & logManager,
    __in ULONG64 enumerationStartingPosition,
    __in ULONG64 enumerationEndingPosition,
    __in LONG64 enumerationStartedAtLsn,
    __in KStringView const & readerName,
    __in LogReaderType::Enum readerType,
    __in LONG64 readAheadCacheSize,
    __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
    __in TRInternalSettingsSPtr const & transactionalReplicatorConfig,
    __in KAllocator & allocator)
{
    LogRecords * pointer = _new(LOGRECORDS_TAG, allocator) LogRecords(
        traceId,
        logManager, 
        enumerationStartingPosition, 
        enumerationEndingPosition, 
        enumerationStartedAtLsn,
        readAheadCacheSize,
        readerName, 
        readerType,
        healthClient,
        transactionalReplicatorConfig);
    
    THROW_ON_ALLOCATION_FAILURE(pointer);
    return IAsyncEnumerator<LogRecord::SPtr>::SPtr(pointer);
}

LogRecords::SPtr LogRecords::Create(
    __in PartitionedReplicaId const & traceId,
    __in Data::Log::ILogicalLogReadStream & readStream,
    __in InvalidLogRecords & invalidLogRecords,
    __in ULONG64 enumerationStartingPosition,
    __in ULONG64 enumerationEndingPosition,
    __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
    __in TRInternalSettingsSPtr const & transactionalReplicatorConfig,
    __in KAllocator & allocator)
{
    LogRecords * pointer = _new(LOGRECORDS_TAG, allocator) LogRecords(
        traceId,
        readStream, 
        invalidLogRecords, 
        enumerationStartingPosition, 
        enumerationEndingPosition,
        healthClient,
        transactionalReplicatorConfig);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return LogRecords::SPtr(pointer);
}

void LogRecords::Dispose()
{
    if (isDisposed_)
    {
        return;
    }

    if (logManager_ != nullptr)
    {
        logManager_->RemoveLogReader(enumerationStartingPosition_);

        if (readStream_ != nullptr)
        {
            DisposeReadStream();
        }
    }

    isDisposed_ = true;
}

Task LogRecords::DisposeReadStream()
{
    KCoShared$ApiEntry()

    co_await readStream_->CloseAsync();
    co_return;
}

void LogRecords::Reset()
{ 
    readStream_->SetPosition(LONGLONG(enumerationStartingPosition_));
    currentRecord_ = invalidLogRecords_->Inv_LogRecord;
    lastPhysicalRecord_ = nullptr;
}

Awaitable<bool> LogRecords::MoveNextAsync(__in CancellationToken const & cancellationToken)
{
    if (readStream_->GetPosition() <= (LONG64)enumerationEndingPosition_ && !isDisposed_)
    {
        Common::Stopwatch logReadWatch;
        logReadWatch.Start();

        LogRecord::SPtr record = co_await LogRecord::ReadNextRecordAsync(
            *readStream_,
            *invalidLogRecords_,
            GetThisAllocator());

        logReadWatch.Stop();
        if (logReadWatch.Elapsed > transactionalReplicatorConfig_->SlowLogIODuration)
        {
            ioMonitor_->OnSlowOperation();   
        }

        ASSERT_IFNOT(
            record != nullptr,
            "LogRecords::MoveNextAsync : record must not be null");

        PhysicalLogRecord * physicalRecord = record->AsPhysicalLogRecord();

        if (lastPhysicalRecord_ != nullptr)
        {
            record->PreviousPhysicalRecord = lastPhysicalRecord_.RawPtr();

            if (physicalRecord != nullptr)
            {
                lastPhysicalRecord_->NextPhysicalRecord = physicalRecord;
                lastPhysicalRecord_ = physicalRecord;
            }
        }
        else if (physicalRecord != nullptr)
        {
            lastPhysicalRecord_ = physicalRecord;
        }

        currentRecord_ = record;

        // If the starting lsn is not initialized (recovery reader case), initialize it to the first record's lsn
        if (enumerationStartAtLsn_ == MAXLONG64)
        {
            enumerationStartAtLsn_ = currentRecord_->Lsn;
        }

        // Trim the starting position for enabling possible truncations

        if (logManager_ != nullptr &&
            currentRecord_->RecordPosition - enumerationStartingPosition_ > UpdateStartingPositionAfterBytes)
        {
            // First add the new range and only then remove the older range

            bool isValid = logManager_->AddLogReader(
                enumerationStartAtLsn_,
                currentRecord_->RecordPosition,
                enumerationEndingPosition_,
                readerName_,
                readerType_);

            ASSERT_IFNOT(
                isValid,
                "LogRecords::MoveNextAsync : logManager_->AddLogReader must be valid");

            logManager_->RemoveLogReader(enumerationStartingPosition_);

            enumerationStartingPosition_ = currentRecord_->RecordPosition;
        }

        co_return true;
    }

    co_return false;
}
