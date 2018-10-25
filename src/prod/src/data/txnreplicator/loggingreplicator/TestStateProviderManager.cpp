// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestHeaders.h"

using namespace ktl;
using namespace Data::LoggingReplicator;
using namespace TxnReplicator;
using namespace Data::Utilities;
using namespace LoggingReplicatorTests;

#define TESTSPM_TAG 'MpST'

TestStateProviderManager::TestStateProviderManager(__in ApiFaultUtility & apiFaultUtility)
    : IStateProviderManager()
    , KObject()
    , KShared()
    , lock_()
    , expectedData_(GetThisAllocator())
    , applyCount_(0)
    , unlockCount_(0)
    , stateStreamSuccessfulOperationCount_(0)
    , beginSettingCurrentStateApiCount_(0)
    , setCurrentStateApiCount_(0)
    , endSettingCurrentStateApiCount_(0)
    , apiFaultUtility_(&apiFaultUtility)
{
    NTSTATUS status = expectedData_.Initialize(20011, K_DefaultHashFunction);
    CODING_ERROR_ASSERT(status == STATUS_SUCCESS);
}

TestStateProviderManager::~TestStateProviderManager()
{
}

TestStateProviderManager::SPtr TestStateProviderManager::Create(
    __in ApiFaultUtility & apiFaultUtility,
    __in KAllocator & allocator)
{
    TestStateProviderManager * pointer = _new(TESTSPM_TAG, allocator) TestStateProviderManager(apiFaultUtility);
    CODING_ERROR_ASSERT(pointer != nullptr);

    return TestStateProviderManager::SPtr(pointer);
}

void TestStateProviderManager::VerifyOperationData(
    __in_opt OperationData const * const actualData,
    __in_opt OperationData const * const expectedData)
{
    if (actualData == nullptr || expectedData == nullptr)
    {
        CODING_ERROR_ASSERT(actualData == nullptr);
        CODING_ERROR_ASSERT(expectedData == nullptr);
        return;
    }

    CODING_ERROR_ASSERT(actualData->BufferCount == expectedData->BufferCount);
    
    for (ULONG i = 0; i < actualData->BufferCount; i++)
    {
        CODING_ERROR_ASSERT((*actualData)[i]->QuerySize() == (*expectedData)[i]->QuerySize());

        byte const * actualBuffer = (byte const *)(*actualData)[i]->GetBuffer();
        byte const * expectedBuffer = (byte const *)(*expectedData)[i]->GetBuffer();
           
        for (ULONG index = 0; index < (*actualData)[i]->QuerySize(); index++)
        {
            // Verifies that the actual contents of the buffer match
            CODING_ERROR_ASSERT(*(actualBuffer + index) == *(expectedBuffer + index));
        }
    }
}

Awaitable<NTSTATUS> TestStateProviderManager::ApplyAsync(
    __in LONG64 logicalSequenceNumber,
    __in TransactionBase const & transactionBase,
    __in TxnReplicator::ApplyContext::Enum applyContext,
    __in_opt OperationData const * const metadataPtr,
    __in_opt OperationData const * const dataPtr,
    __out OperationContext::CSPtr & result) noexcept
{
    result = nullptr;
    CODING_ERROR_ASSERT(transactionBase.CommitSequenceNumber > 0); // Verify transaction is committed

    // Do verification of apply data only in non recovery scenarios
    if ((applyContext & TxnReplicator::ApplyContext::Enum::RECOVERY) == 0 && (applyContext & TxnReplicator::ApplyContext::Enum::FALSE_PROGRESS) == 0)
    {
        ExpectedDataValue * value = GetValue(transactionBase.TransactionId);
        if (value->IndexToVerify == value->IndexToThrowException)
        {
            value->IndexToVerify++;
            co_return SF_STATUS_OBJECT_CLOSED;
        }

        VerifyOperationData(metadataPtr, (*value->MetaData)[value->IndexToVerify].RawPtr());
        VerifyOperationData(dataPtr, (*value->ApplyData)[value->IndexToVerify].RawPtr());
        value->IndexToVerify++;
    }
    else
    {
        NTSTATUS status = co_await apiFaultUtility_->WaitUntilSignaledAsync(ApiName::ApplyAsync);
        if (!NT_SUCCESS(status))
        {
            co_return status;
        }
    }

    applyCount_++;
    co_return STATUS_SUCCESS;
}

NTSTATUS TestStateProviderManager::Unlock(__in OperationContext const & operationContext) noexcept
{
    UNREFERENCED_PARAMETER(operationContext);
    apiFaultUtility_->WaitUntilSignaled(ApiName::Unlock);
    unlockCount_++;
    return STATUS_SUCCESS;
}

void TestStateProviderManager::Reuse()
{
    K_LOCK_BLOCK(lock_)
    {
        expectedData_.Reset();
        applyCount_.store(0);
        unlockCount_.store(0);

        LONG64* txId;
        ExpectedDataValue* val;

        while (expectedData_.NextPtr(txId, val) != STATUS_NO_MORE_ENTRIES)
        {
            val->IndexToVerify = 0;
        }
    }
}

void TestStateProviderManager::AddExpectedTransactionExceptionAtNthApply(
    __in LONG64 txId,
    __in ULONG nthApply,
    __in KSharedArray<OperationData::CSPtr> & expectedMetadata,
    __in KSharedArray<OperationData::CSPtr> & expectedApplyData)
{
    K_LOCK_BLOCK(lock_)
    {
        ExpectedDataValue value;

        value.MetaData = &expectedMetadata;
        value.ApplyData = &expectedApplyData;
        value.IndexToVerify = 0;
        value.IndexToThrowException = nthApply;

        NTSTATUS status = expectedData_.Put(txId, value, FALSE);
        CODING_ERROR_ASSERT(status == STATUS_SUCCESS);
    }
}

void TestStateProviderManager::AddExpectedTransactionApplyData(
    __in LONG64 txId,
    __in OperationData::CSPtr & expectedMetadata,
    __in OperationData::CSPtr & expectedApplyData)
{ 
    K_LOCK_BLOCK(lock_)
    {
        ExpectedDataValue value;
        NTSTATUS status = expectedData_.Get(txId, value);

        if (status == STATUS_NOT_FOUND)
        {
            KSharedArray<OperationData::CSPtr>::SPtr metaData = _new(TESTSPM_TAG, GetThisAllocator())KSharedArray<OperationData::CSPtr>();
            KSharedArray<OperationData::CSPtr>::SPtr applyData = _new(TESTSPM_TAG, GetThisAllocator())KSharedArray<OperationData::CSPtr>();

            metaData->Append(expectedMetadata);
            applyData->Append(expectedApplyData);

            // First insert
            value.MetaData = metaData;
            value.ApplyData = applyData;
            value.IndexToVerify = 0;
            value.IndexToThrowException = MAXULONG;

            status = expectedData_.Put(txId, value, FALSE);
            CODING_ERROR_ASSERT(status == STATUS_SUCCESS);
            return;
        }

        CODING_ERROR_ASSERT(status == STATUS_SUCCESS);

        // Found existing entry. Append to the shared array
        value.MetaData->Append(expectedMetadata);
        value.ApplyData->Append(expectedApplyData);
    }
}

void TestStateProviderManager::AddExpectedTransactionApplyData(
    __in LONG64 txId,
    __in KSharedArray<OperationData::CSPtr> & expectedMetadata,
    __in KSharedArray<OperationData::CSPtr> & expectedApplyData)
{
    AddExpectedTransactionExceptionAtNthApply(
        txId,
        MAXULONG,
        expectedMetadata,
        expectedApplyData);
}

TestStateProviderManager::ExpectedDataValue * TestStateProviderManager::GetValue(__in LONG64 txId)
{
    ExpectedDataValue * value = nullptr;

    K_LOCK_BLOCK(lock_)
    {
        NTSTATUS status = expectedData_.GetPtr(txId, value);
        CODING_ERROR_ASSERT(status == STATUS_SUCCESS);
    }

    return value;
}

bool TestStateProviderManager::WaitForProcessingToComplete(
    __in LONG targetApplyCount,
    __in LONG targetUnlockCount,
    __in Common::TimeSpan const & timeout)
{
    Common::Stopwatch stopWatch;
    stopWatch.Start();

    do
    {
        Sleep(100);

        if (targetApplyCount == applyCount_.load() &&
            targetUnlockCount == unlockCount_.load() &&
            stopWatch.Elapsed > timeout)
        {
            return true;
        }
        else if 
            (targetApplyCount < applyCount_.load() || 
             targetUnlockCount < unlockCount_.load())
        {
            CODING_ERROR_ASSERT(false); // Apply/unlock count is more than the expected target
        }

        if (stopWatch.Elapsed > timeout &&
            (targetApplyCount > applyCount_.load() ||
             targetUnlockCount > unlockCount_.load()))
        {
            return false;
        }
    } while (true);
}

NTSTATUS TestStateProviderManager::PrepareCheckpoint(__in LONG64 lsn) noexcept
{
    UNREFERENCED_PARAMETER(lsn);
    NTSTATUS status = apiFaultUtility_->WaitUntilSignaled(ApiName::PrepareCheckpoint);
    return status;
}

Awaitable<NTSTATUS> TestStateProviderManager::PerformCheckpointAsync(__in CancellationToken const & cancellationToken) noexcept
{
    UNREFERENCED_PARAMETER(cancellationToken);
    NTSTATUS status = co_await apiFaultUtility_->WaitUntilSignaledAsync(ApiName::PerformCheckpoint);
    co_return status;
}

Awaitable<NTSTATUS> TestStateProviderManager::CompleteCheckpointAsync(__in CancellationToken const & cancellationToken) noexcept
{
    UNREFERENCED_PARAMETER(cancellationToken);

    NTSTATUS status = co_await apiFaultUtility_->WaitUntilSignaledAsync(ApiName::CompleteCheckpoint);
    co_return status;
}

NTSTATUS TestStateProviderManager::GetCurrentState(
    __in FABRIC_REPLICA_ID targetReplicaId,
    __out OperationDataStream::SPtr & result) noexcept
{
    UNREFERENCED_PARAMETER(targetReplicaId);

    TestStateStream::SPtr stateStream = TestStateStream::Create(GetThisAllocator(), stateStreamSuccessfulOperationCount_);
    result = stateStream.RawPtr();
    return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> TestStateProviderManager::BackupCheckpointAsync(
    __in KString const & backupDirectory,
    __in CancellationToken const & cancellationToken) noexcept
{
    KShared$ApiEntry();

    UNREFERENCED_PARAMETER(backupDirectory);
    UNREFERENCED_PARAMETER(cancellationToken);

    co_await suspend_never{};
    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> TestStateProviderManager::RestoreCheckpointAsync(
    __in KString const & backupDirectory,
    __in CancellationToken const & cancellationToken) noexcept
{
    KShared$ApiEntry();

    UNREFERENCED_PARAMETER(backupDirectory);
    UNREFERENCED_PARAMETER(cancellationToken);

    co_await suspend_never{};
    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> TestStateProviderManager::BeginSettingCurrentStateAsync() noexcept
{
    beginSettingCurrentStateApiCount_++;
    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> TestStateProviderManager::EndSettingCurrentState() noexcept
{
    endSettingCurrentStateApiCount_++;
    co_await suspend_never{};
    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> TestStateProviderManager::SetCurrentStateAsync(
    __in LONG64 stateRecordNumber,
    __in OperationData const & data) noexcept
{
    setCurrentStateApiCount_++;
    co_await suspend_never{};
    co_return STATUS_SUCCESS;
}

Awaitable<void> TestStateProviderManager::ChangeRoleAsync(
    __in FABRIC_REPLICA_ROLE newRole,
    __in ktl::CancellationToken & cancellationToken)
{
    UNREFERENCED_PARAMETER(newRole);
    UNREFERENCED_PARAMETER(cancellationToken);

    co_return;
}
