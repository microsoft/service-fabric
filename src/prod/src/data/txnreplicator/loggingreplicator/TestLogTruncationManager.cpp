// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestHeaders.h"

using namespace ktl;
using namespace Data::LogRecordLib;
using namespace Data::LoggingReplicator;
using namespace TxnReplicator;
using namespace Data::Utilities;
using namespace LoggingReplicatorTests;

#define TESTLOGTRUNCATIONMANAGER_TAG 'MTLT'

TestLogTruncationManager::TestLogTruncationManager(
    __in int seed,
    __in Data::LoggingReplicator::ReplicatedLogManager & replicatedLogManager)
    : ILogTruncationManager()
    , KObject()
    , KShared()
    , rnd_(seed)
    , replicatedLogManager_(&replicatedLogManager)
    , isGoodLogHead_(OperationProbability::No)
    , shouldCheckpoint_(OperationProbability::No)
    , shouldTruncateHead_(OperationProbability::No)
    , shouldIndex_(OperationProbability::No)
{
}

TestLogTruncationManager::~TestLogTruncationManager()
{
}

TestLogTruncationManager::SPtr TestLogTruncationManager::Create(
    __in int seed,
    __in ReplicatedLogManager & replicatedLogManager,
    __in KAllocator & allocator)
{
    TestLogTruncationManager * pointer = _new(TESTLOGTRUNCATIONMANAGER_TAG, allocator) TestLogTruncationManager(seed, replicatedLogManager);
    CODING_ERROR_ASSERT(pointer != nullptr);

    return TestLogTruncationManager::SPtr(pointer);
}

Awaitable<void> TestLogTruncationManager::BlockSecondaryPumpIfNeeded(LONG64 lastStableLsn)
{
    ULONG delay = static_cast<ULONG>(rnd_.Next(0, 100));
    NTSTATUS status = co_await KTimer::StartTimerAsync(GetThisAllocator(), TESTLOGTRUNCATIONMANAGER_TAG, delay, nullptr);
    CODING_ERROR_ASSERT(status == STATUS_SUCCESS);
    co_return;
}

bool TestLogTruncationManager::ShouldBlockOperationsOnPrimary()
{
    return false;
}

KArray<BeginTransactionOperationLogRecord::SPtr> TestLogTruncationManager::GetOldTransactions(__in TransactionMap & transactionMap)
{
    UNREFERENCED_PARAMETER(transactionMap);
    return KArray<BeginTransactionOperationLogRecord::SPtr>(GetThisAllocator());
}

bool TestLogTruncationManager::ShouldIndex()
{
    return GetRandomBoolValue(shouldIndex_);
}

bool TestLogTruncationManager::ShouldCheckpointOnPrimary(
    __in TransactionMap & transactionMap,
    __out KArray<BeginTransactionOperationLogRecord::SPtr> & abortTxList)
{
    if (replicatedLogManager_->LastInProgressCheckpointRecord != nullptr)
    {
        return false;
    }

    UNREFERENCED_PARAMETER(transactionMap);
    UNREFERENCED_PARAMETER(abortTxList);
    return GetRandomBoolValue(shouldCheckpoint_);
}

bool TestLogTruncationManager::ShouldCheckpointOnSecondary(__in TransactionMap & transactionMap)
{
    if (replicatedLogManager_->LastInProgressCheckpointRecord != nullptr)
    {
        return false;
    }

    UNREFERENCED_PARAMETER(transactionMap);
    return GetRandomBoolValue(shouldCheckpoint_);
}

bool TestLogTruncationManager::ShouldTruncateHead()
{
    if (replicatedLogManager_->LastInProgressTruncateHeadRecord != nullptr)
    {
        return false;
    }

    return GetRandomBoolValue(shouldTruncateHead_);
}

ReplicatedLogManager::IsGoodLogHeadCandidateCalculator TestLogTruncationManager::GetGoodLogHeadCandidateCalculator()
{
    ReplicatedLogManager::IsGoodLogHeadCandidateCalculator callback(this, &TestLogTruncationManager::IsGoodLogHead);
    return callback;
}

bool TestLogTruncationManager::GetRandomBoolValue(OperationProbability & value)
{
    switch (value)
    {
    case OperationProbability::Yes:
        return true;
    case OperationProbability::No:
        return false;
    case OperationProbability::YesOnceThenNo:
        value = OperationProbability::No;
        return true;
    case OperationProbability::YesOnceThenRandom:
        value = OperationProbability::Random;
        return true;
    case OperationProbability::NoOnceThenYes:
        value = OperationProbability::Yes;
        return false;
    case OperationProbability::Random:
    {
        int val = rnd_.Next();
        if (val % 4 == 0)
        {
            return true;
        }
        return false;
    }
    default:
        CODING_ERROR_ASSERT(false);
    }

    return false;
}

bool TestLogTruncationManager::IsGoodLogHead(__in IndexingLogRecord & indexingRecord)
{
    UNREFERENCED_PARAMETER(indexingRecord);
    return GetRandomBoolValue(isGoodLogHead_);
}

void TestLogTruncationManager::OnCheckpointCompleted(
    __in NTSTATUS status,
    __in Data::LogRecordLib::CheckpointState::Enum checkpointState,
    __in bool isRecoveredCheckpoint)
{
    UNREFERENCED_PARAMETER(status);
    UNREFERENCED_PARAMETER(checkpointState);
    UNREFERENCED_PARAMETER(isRecoveredCheckpoint);
}

void TestLogTruncationManager::OnTruncationCompleted()
{
}

void TestLogTruncationManager::InitiatePeriodicCheckpoint()
{
}

void TestLogTruncationManager::SetIsGoodLogHead(OperationProbability value)
{
    isGoodLogHead_ = value;
}

void TestLogTruncationManager::SetCheckpoint(OperationProbability value)
{
    shouldCheckpoint_ = value;
}

void TestLogTruncationManager::SetIndex(OperationProbability value)
{
    shouldIndex_ = value;
}

void TestLogTruncationManager::SetTruncateHead(OperationProbability value)
{
    shouldTruncateHead_ = value;
}
