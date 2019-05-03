// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::Log;
using namespace Data::LoggingReplicator;
using namespace Data::LogRecordLib;

Common::StringLiteral const TraceComponent("MemoryLogManager");

MemoryLogManager::MemoryLogManager(
    __in Data::Utilities::PartitionedReplicaId const & traceId, 
    __in TxnReplicator::TRPerformanceCountersSPtr const & perfCounters, 
    __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient, 
    __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig)
    : LogManager(traceId, perfCounters, healthClient, transactionalReplicatorConfig)
    , perfCounters_(perfCounters)
    , healthClient_(healthClient)
    , transactionalReplicatorConfig_(transactionalReplicatorConfig)
{
}

MemoryLogManager::~MemoryLogManager()
{
}

MemoryLogManager::SPtr MemoryLogManager::Create(
    __in Data::Utilities::PartitionedReplicaId const & traceId, 
    __in TxnReplicator::TRPerformanceCountersSPtr const & perfCounters, 
    __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient, 
    __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
    __in KAllocator & allocator)
{
    MemoryLogManager * pointer = _new(MEMORYLOGMANAGER_TAG, allocator) MemoryLogManager(traceId, perfCounters, healthClient, transactionalReplicatorConfig);
    THROW_ON_ALLOCATION_FAILURE(pointer);
    return MemoryLogManager::SPtr(pointer);
}

Awaitable<NTSTATUS> MemoryLogManager::CreateCopyLogAsync(
    __in TxnReplicator::Epoch const startingEpoch, 
    __in LONG64 startingLsn, 
    __in IndexingLogRecord::SPtr & newHead)
{
    IFlushCallbackProcessor::SPtr flushCallbackProcessor = physicalLogWriter_->CallbackManager->FlushCallbackProcessor;

    NTSTATUS status = co_await CloseCurrentLogAsync();
    CO_RETURN_ON_FAILURE(status);

    SetCurrentLogFileAliasToCopy();

    status = co_await CreateLogFileAsync(true, CancellationToken::None, logicalLog_);
    CO_RETURN_ON_FAILURE(status);

    PhysicalLogWriterCallbackManager::SPtr callbackManager = PhysicalLogWriterCallbackManager::Create(*PartitionedReplicaIdentifier, GetThisAllocator());
    callbackManager->FlushCallbackProcessor = *flushCallbackProcessor;

    physicalLogWriter_ = PhysicalLogWriter::Create(
        *PartitionedReplicaIdentifier,
        *logicalLog_,
        *callbackManager,
        LogManager::DefaultWriteCacheSizeMB * 1024 * 1024,
        false,
        *invalidLogRecords_->Inv_LogRecord,
        perfCounters_,
        healthClient_,
        transactionalReplicatorConfig_,
        GetThisAllocator());

    IndexingLogRecord::SPtr logHead = IndexingLogRecord::Create(
        startingEpoch, 
        startingLsn, 
        nullptr,
        *invalidLogRecords_->Inv_PhysicalLogRecord,
        GetThisAllocator());
    physicalLogWriter_->InsertBufferedRecord(*logHead);

    co_await physicalLogWriter_->FlushAsync(L"CreateCopyLogAsync");
    status = co_await logHead->AwaitFlush();

    CO_RETURN_ON_FAILURE(status);

    logHeadRecordPosition_ = logHead->RecordPosition;

    newHead = move(logHead);

    co_return status;
}

Awaitable<NTSTATUS> MemoryLogManager::DeleteLogAsync()
{
    if (physicalLogWriter_ != nullptr)
    {
        physicalLogWriter_->Dispose();
        physicalLogWriter_ = nullptr;
    }

    if (logicalLog_ != nullptr)
    {
        auto log = logicalLog_;
        logicalLog_ = nullptr;
        NTSTATUS status = co_await log->CloseAsync();
        CO_RETURN_ON_FAILURE(status);
    }

    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> MemoryLogManager::RenameCopyLogAtomicallyAsync()
{
    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> MemoryLogManager::CreateLogFileAsync(
    __in bool createNew, 
    __in ktl::CancellationToken const & cancellationToken, 
    __out Data::Log::ILogicalLog::SPtr & result)
{
    UNREFERENCED_PARAMETER(createNew);
    UNREFERENCED_PARAMETER(cancellationToken);

    MemoryLog::SPtr memoryLogSPtr;
    NTSTATUS status = MemoryLog::Create(*PartitionedReplicaIdentifier, GetThisAllocator(), memoryLogSPtr);

    result = NT_SUCCESS(status) ? static_cast<ILogicalLog *>(memoryLogSPtr.RawPtr()) : nullptr;

    co_return status;
}