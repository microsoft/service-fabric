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
using namespace Common;

Common::StringLiteral const TraceComponent("PhysicalLogWriterCallbackManager");

PhysicalLogWriterCallbackManager::PhysicalLogWriterCallbackManager(
    __in PartitionedReplicaId const & traceId)
    : KObject()
    , KShared()
    , PartitionedReplicaTraceComponent(traceId)
    , isDisposed_(false)
    , callbackLock_()
    , isCallingback_(false)
    , loggedRecordsArray_()
    , recordsToCallback_()
    , flushCallbackProcessor_()
    , flushedPsn_(Constants::InvalidLsn)
{
    EventSource::Events->Ctor(
        TracePartitionId,
        ReplicaId,
        L"PhysicalLogWriterCallbackManager",
        reinterpret_cast<uintptr_t>(this));
}

PhysicalLogWriterCallbackManager::~PhysicalLogWriterCallbackManager()
{
    EventSource::Events->Dtor(
        TracePartitionId,
        ReplicaId,
        L"PhysicalLogWriterCallbackManager",
        reinterpret_cast<uintptr_t>(this));
}

PhysicalLogWriterCallbackManager::SPtr PhysicalLogWriterCallbackManager::Create(
    __in PartitionedReplicaId const & traceId,
    __in KAllocator & allocator)
{
    PhysicalLogWriterCallbackManager * pointer = _new(PHYSICALLOGWRITERCALLBACKMGR_TAG, allocator)PhysicalLogWriterCallbackManager(
        traceId);

    THROW_ON_ALLOCATION_FAILURE(pointer);

    return PhysicalLogWriterCallbackManager::SPtr(pointer);
}

void PhysicalLogWriterCallbackManager::Dispose()
{
    if (isDisposed_)
    {
        return;
    }

    K_LOCK_BLOCK(callbackLock_)
    {
        if (loggedRecordsArray_ != nullptr)
        {
            for (ULONG i = 0; i < loggedRecordsArray_->Count(); i++)
            {
                ASSERT_IF(
                    (*loggedRecordsArray_)[i]->LogError == STATUS_SUCCESS,
                    "{0}:Log exception must exist during PhysicalLogWriterCallbackManager::Dispose",
                    TraceId);
            }
        }
    }

    isDisposed_ = true;
}

void PhysicalLogWriterCallbackManager::Execute()
{
    // Release the reference taken before starting the work item
    KFinally([&] { this->Release(); });

    ASSERT_IFNOT(
        recordsToCallback_->Count() != 0,
        "{0}:Expected recordsToCallback_->Count() == 0, found {1}",
        TraceId,
        recordsToCallback_->Count());

    while (true)
    {
        for (ULONG i = 0; i < recordsToCallback_->Count(); i++)
        {
            LoggedRecords::CSPtr & loggedRecords = (*recordsToCallback_)[i];
            
            if (!NT_SUCCESS(loggedRecords->LogError))
            {
                for (ULONG j = 0; j < loggedRecords->Count; j++)
                {
                    TraceLoggingException(
                        (*(*loggedRecords)[j]), 
                        loggedRecords->LogError);
                }
            }
            else
            {
                ASSERT_IFNOT(
                    flushedPsn_ == (*loggedRecords)[0]->Psn,
                    "{0}:Flushed psn: {1} not equal to log record psn: {2} in physical writer callback manager",
                    TraceId,
                    flushedPsn_,
                    (*loggedRecords)[0]->Psn);

                flushedPsn_ += loggedRecords->Count;
            }

            try
            {
                // This is needed because the logging replicator could have been destructed if there is a failed flush
                IFlushCallbackProcessor::SPtr lock = flushCallbackProcessor_->TryGetTarget();
                if (lock)
                {
                    lock->ProcessFlushedRecordsCallback(*loggedRecords);
                }
                else
                {
                    EventSource::Events->NullCallbackProcessor(
                        TracePartitionId,
                        ReplicaId,
                        (*loggedRecords)[0]->Psn,
                        (*loggedRecords)[loggedRecords->Count - 1]->Psn);

                    ASSERT_IF(
                        loggedRecords->LogError == STATUS_SUCCESS,
                        "{0}:Failed to lock the flushcallback processor when a successful flush was issued for range Psn:{1}-{2}. This means we cannot process this record. It is a bug on the caller where he is not awaiting for the processing of all records",
                        TraceId,
                        (*loggedRecords)[0]->Psn,
                        (*loggedRecords)[loggedRecords->Count - 1]->Psn);

                    std::wstring msg = wformatString(
                        "Flush for log records from Psn: {0}-{1} failed",
                        (*loggedRecords)[0]->Psn,
                        (*loggedRecords)[loggedRecords->Count - 1]->Psn);

                    LR_TRACE_EXCEPTION(
                        ToStringLiteral(msg),
                        loggedRecords->LogError);
                }
            }
            catch (Exception & ex)
            {
                LR_TRACE_UNEXPECTEDEXCEPTION(
                    L"Unexpected exception in Execute.",
                    ex.GetStatus());

                ASSERT_IFNOT(
                    false,
                    "{0}:Unexpected exception in Execute. Status:{1}",
                    TraceId,
                    ex.GetStatus());
            }
        }
        
        K_LOCK_BLOCK(callbackLock_)
        {
            ASSERT_IFNOT(
                isCallingback_,
                "{0}:isCallingback_ == true",
                TraceId);

            if (loggedRecordsArray_ == nullptr)
            {
                isCallingback_ = false;
                return;
            }

            recordsToCallback_ = loggedRecordsArray_;
            loggedRecordsArray_ = nullptr;
        }
    }
}

void PhysicalLogWriterCallbackManager::ProcessFlushedRecords(__in LoggedRecords const & flushedRecords)
{
    NTSTATUS status = STATUS_SUCCESS;

    K_LOCK_BLOCK(callbackLock_)
    {
        if (!isCallingback_)
        {
            ASSERT_IFNOT(
                loggedRecordsArray_ == nullptr,
                "{0}:Logged records array must be null",
                TraceId);

            isCallingback_ = true;
        }
        else
        {
            if (loggedRecordsArray_ == nullptr)
            {
                loggedRecordsArray_ = _new(PHYSICALLOGWRITERCALLBACKMGR_TAG, GetThisAllocator())KSharedArray<LoggedRecords::CSPtr>();
                THROW_ON_ALLOCATION_FAILURE(loggedRecordsArray_);
            }

            status = loggedRecordsArray_->Append(&flushedRecords);
            THROW_ON_FAILURE(status);
            return;
        }
    }

    recordsToCallback_ = _new(PHYSICALLOGWRITERCALLBACKMGR_TAG, GetThisAllocator())KSharedArray<LoggedRecords::CSPtr>();
    THROW_ON_ALLOCATION_FAILURE(recordsToCallback_);

    status = recordsToCallback_->Append(&flushedRecords);
    THROW_ON_FAILURE(status);

    if (flushedPsn_ == Constants::InvalidLsn)
    {
        flushedPsn_ = flushedRecords[0]->Psn;
    }

    KThreadPool & threadPool = GetThisAllocator().GetKtlSystem().DefaultSystemThreadPool();

    // Add ref on itself to ensure object does not go away while executing the background work item
    AddRef();
    threadPool.QueueWorkItem(*this);

    return;
}

void PhysicalLogWriterCallbackManager::TraceLoggingException(
    __in LogRecord const & record,
    __in NTSTATUS logError)
{
    LR_TRACE_EXCEPTIONRECORD(
        L"PhysicalLogWriterCallbackManager::TraceLoggingException",
        record,
        logError);
}
