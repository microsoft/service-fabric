// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestHeaders.h"

using namespace LoggingReplicatorTests;

using namespace TxnReplicator;
using namespace Data::LogRecordLib;
using namespace Data::LoggingReplicator;

TestReplicatedLogManager::SPtr TestReplicatedLogManager::Create(
    __in KAllocator& allocator,
    __in_opt IndexingLogRecord const * const indexingLogRecord) noexcept
{
    TestReplicatedLogManager * result = _new(TEST_REPLICATED_LOG_MANAGER_TAG, allocator) TestReplicatedLogManager(indexingLogRecord);

    CODING_ERROR_ASSERT(result != nullptr);
    CODING_ERROR_ASSERT(NT_SUCCESS(result->Status()));

    return SPtr(result);
}

void TestReplicatedLogManager::SetProgressVector(
    __in ProgressVector::SPtr && vector)
{
    progressVectorSPtr_ = Ktl::Move(vector);
}

Epoch TestReplicatedLogManager::get_CurrentLogTailEpoch() const
{
    return progressVectorSPtr_->LastProgressVectorEntry.CurrentEpoch;
}

FABRIC_SEQUENCE_NUMBER TestReplicatedLogManager::get_CurrentLogTailLsn() const
{
    return tailLSN_;
}

InformationLogRecord::SPtr TestReplicatedLogManager::get_LastInformationRecord() const
{
    CODING_ASSERT("Not Implemented");
}

Epoch TestReplicatedLogManager::GetEpoch(__in FABRIC_SEQUENCE_NUMBER LSN) const
{
    return progressVectorSPtr_->FindEpoch(LSN);
}

IndexingLogRecord::CSPtr TestReplicatedLogManager::GetIndexingLogRecordForBackup() const
{
    CODING_ERROR_ASSERT(indexingLogRecordCSPtr_ != nullptr);
    return indexingLogRecordCSPtr_;
}

Data::LogRecordLib::EndCheckpointLogRecord::SPtr TestReplicatedLogManager::get_LastCompletedEndCheckpointRecord() const
{
    return nullptr;
}

Data::LogRecordLib::ProgressVector::SPtr TestReplicatedLogManager::get_ProgressVectorValue() const
{
    return nullptr;
}

NTSTATUS TestReplicatedLogManager::ReplicateAndLog(
    __in LogicalLogRecord & record,
    __out LONG64 & bufferedBytes,
    __in_opt TransactionManager * const transactionManager)
{
    UNREFERENCED_PARAMETER(record);
    UNREFERENCED_PARAMETER(transactionManager);
    UNREFERENCED_PARAMETER(bufferedBytes);

    return STATUS_SUCCESS;
}

void TestReplicatedLogManager::Information(
    __in InformationEvent::Enum type)
{
    UNREFERENCED_PARAMETER(type);

    CODING_ASSERT("Not Implemented");
}

TestReplicatedLogManager::TestReplicatedLogManager(
    __in_opt IndexingLogRecord const * const indexingLogRecord)
    : KObject()
    , KShared()
    , IReplicatedLogManager()
    , progressVectorSPtr_(ProgressVector::CreateZeroProgressVector(GetThisAllocator()))
    , indexingLogRecordCSPtr_(indexingLogRecord)
{
}

TestReplicatedLogManager::~TestReplicatedLogManager()
{
}
