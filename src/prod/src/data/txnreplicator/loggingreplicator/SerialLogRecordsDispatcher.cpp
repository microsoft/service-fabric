// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::LoggingReplicator;
using namespace Data::LogRecordLib;
using namespace TxnReplicator;
using namespace Data::Utilities;

Common::StringLiteral const TraceComponent("SerialLogRecordsDispatcher");

SerialLogRecordsDispatcher::SerialLogRecordsDispatcher(
    __in PartitionedReplicaId const & traceId,
    __in IOperationProcessor & processor,
    __in TRInternalSettingsSPtr const & transactionalReplicatorConfig)
    : LogRecordsDispatcher(
        traceId,
        processor,
        transactionalReplicatorConfig)
{
    EventSource::Events->Ctor(
        TracePartitionId,
        ReplicaId,
        L"SerialLogRecordsDispatcher",
        reinterpret_cast<uintptr_t>(this));
}

SerialLogRecordsDispatcher::~SerialLogRecordsDispatcher()
{
    EventSource::Events->Dtor(
        TracePartitionId,
        ReplicaId,
        L"SerialLogRecordsDispatcher",
        reinterpret_cast<uintptr_t>(this));
}

LogRecordsDispatcher::SPtr SerialLogRecordsDispatcher::Create(
    __in PartitionedReplicaId const & traceId,
    __in IOperationProcessor & processor,
    __in TRInternalSettingsSPtr const & transactionalReplicatorConfig,
    __in KAllocator & allocator)
{
    SerialLogRecordsDispatcher * pointer = _new(LOGRECORDS_DISPATCHER_TAG, allocator)SerialLogRecordsDispatcher(
        traceId, 
        processor,
        transactionalReplicatorConfig);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return LogRecordsDispatcher::SPtr(pointer);
}

Awaitable<bool> SerialLogRecordsDispatcher::ProcessLoggedRecords()
{
    ULONG processingIndex = 0;

    while (processingRecords_->Count() > 0)
    {
        do
        {
            ASSERT_IFNOT(
                processingRecords_->Count() > 0,
                "{0}:ProcessLoggedRecords | Must be processing > 0 records",
                TraceId);

            LoggedRecords::CSPtr loggedRecords = (*processingRecords_)[0];

            if (NT_SUCCESS(loggedRecords->LogError))
            {
                LogRecord::SPtr record = (*loggedRecords)[processingIndex];

                bool isBarrierRecord = LogRecord::IsBarrierRecord(*record, transactionalReplicatorConfig_->EnableSecondaryCommitApplyAcknowledgement);

                // Pause dispatching if needed for state manager
                if (isBarrierRecord)
                {
                    co_await PauseDispatchingIfNeededAsync(*record);
                    recordsProcessor_->UpdateDispatchingBarrierTask(*record->GetAppliedTask());
                }

                RecordProcessingMode::Enum processingMode = recordsProcessor_->IdentifyProcessingModeForRecord(*record);

                // processingMode = Normal,ApplyImmediately
                if (processingMode < RecordProcessingMode::Enum::ProcessImmediately)
                {
                    PrepareToProcessRecord(*record, processingMode);
                }

                // processingMode == ApplyImmediately, ProcessImmediately
                if (processingMode > RecordProcessingMode::Enum::Normal)
                {
                    co_await recordsProcessor_->ImmediatelyProcessRecordAsync(*record, STATUS_SUCCESS, processingMode);
                }
                else
                {
                    co_await recordsProcessor_->ProcessLoggedRecordAsync(*record);
                }
            }
            else
            {
                LogRecord::SPtr record = (*loggedRecords)[processingIndex];

                co_await recordsProcessor_->ImmediatelyProcessRecordAsync(
                    *record,
                    loggedRecords->LogError,
                    RecordProcessingMode::Enum::ProcessImmediately);
            }

            ++processingIndex;

            if (processingIndex == loggedRecords->Count)
            {
                BOOLEAN result = processingRecords_->Remove(0);
                ASSERT_IFNOT(
                    result,
                    "{0}:ProcessLoggedRecords | Failed to remove record",
                    TraceId);

                processingIndex = 0;
            }

        } while (processingRecords_->Count() > 0);
    }

    co_return true;
}
