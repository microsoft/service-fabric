// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace StateManagerTests;
using namespace TxnReplicator;
using namespace Data::Utilities;

TestTransactionManager::SPtr TestTransactionManager::Create(
    __in KAllocator & allocator)
{
    SPtr result = _new(TEST_TAG, allocator) TestTransactionManager();
    CODING_ERROR_ASSERT(result != nullptr);
    CODING_ERROR_ASSERT(NT_SUCCESS(result->Status()));

    return result;
}

NTSTATUS TestTransactionManager::RegisterTransactionChangeHandler(
    __in ITransactionChangeHandler & transactionChangeHandler) noexcept
{
    UNREFERENCED_PARAMETER(transactionChangeHandler);

    CODING_ASSERT("STATUS_NOT_IMPLEMENTED");
}

NTSTATUS TestTransactionManager::UnRegisterTransactionChangeHandler() noexcept
{
    CODING_ASSERT("STATUS_NOT_IMPLEMENTED");
}

NTSTATUS TestTransactionManager::RegisterStateManagerChangeHandler(
    __in IStateManagerChangeHandler & stateManagerChangeHandler) noexcept
{
    return stateManagerSPtr_->RegisterStateManagerChangeHandler(stateManagerChangeHandler);
}

NTSTATUS TestTransactionManager::UnRegisterStateManagerChangeHandler() noexcept
{
    return stateManagerSPtr_->UnRegisterStateManagerChangeHandler();
}

ktl::Awaitable<NTSTATUS> TestTransactionManager::BackupAsync(
    __in TxnReplicator::IBackupCallbackHandler & backupCallbackHandler,
    __out TxnReplicator::BackupInfo & result) noexcept
{
    KShared$ApiEntry();

    co_return co_await loggingReplicatorSPtr_->BackupAsync(backupCallbackHandler, result);
}

ktl::Awaitable<NTSTATUS> TestTransactionManager::BackupAsync(
    __in TxnReplicator::IBackupCallbackHandler & backupCallbackHandler,
    __in FABRIC_BACKUP_OPTION backupOption,
    __in Common::TimeSpan const & timeout,
    __in CancellationToken const & cancellationToken,
    __out TxnReplicator::BackupInfo & result) noexcept
{
    KShared$ApiEntry();

    co_return co_await loggingReplicatorSPtr_->BackupAsync(backupCallbackHandler, backupOption, timeout, cancellationToken, result);
}

ktl::Awaitable<NTSTATUS> TestTransactionManager::RestoreAsync(
    __in KString const & backupFolder) noexcept
{
    KShared$ApiEntry();

    co_return co_await loggingReplicatorSPtr_->RestoreAsync(backupFolder);
}

ktl::Awaitable<NTSTATUS> TestTransactionManager::RestoreAsync(
    __in KString const & backupFolder,
    __in FABRIC_RESTORE_POLICY restorePolicy,
    __in Common::TimeSpan const & timeout,
    __in CancellationToken const & cancellationToken) noexcept
{
    KShared$ApiEntry();

    co_return co_await loggingReplicatorSPtr_->RestoreAsync(backupFolder, restorePolicy, timeout, cancellationToken);
}

NTSTATUS TestTransactionManager::Get(
    __in KUriView const & stateProviderName,
    __out IStateProvider2::SPtr & stateProvider) noexcept
{
    UNREFERENCED_PARAMETER(stateProviderName);
    UNREFERENCED_PARAMETER(stateProvider);

    CODING_ASSERT("STATUS_NOT_IMPLEMENTED");
}

Awaitable<NTSTATUS> TestTransactionManager::AddAsync(
    __in Transaction & transaction,
    __in KUriView const & stateProviderName,
    __in KStringView const & stateProvider,
    __in_opt OperationData const * const initializationParameters,
    __in_opt Common::TimeSpan const & timeout,
    __in_opt CancellationToken const & cancellationToken) noexcept
{
    UNREFERENCED_PARAMETER(transaction);
    UNREFERENCED_PARAMETER(stateProviderName);
    UNREFERENCED_PARAMETER(stateProvider);
    UNREFERENCED_PARAMETER(initializationParameters);
    UNREFERENCED_PARAMETER(timeout);
    UNREFERENCED_PARAMETER(cancellationToken);

    CODING_ASSERT("STATUS_NOT_IMPLEMENTED");
}

Awaitable<NTSTATUS> TestTransactionManager::RemoveAsync(
    __in Transaction & transaction,
    __in KUriView const & stateProviderName,
    __in_opt Common::TimeSpan const & timeout,
    __in_opt CancellationToken const & cancellationToken) noexcept
{
    UNREFERENCED_PARAMETER(transaction);
    UNREFERENCED_PARAMETER(stateProviderName);
    UNREFERENCED_PARAMETER(timeout);
    UNREFERENCED_PARAMETER(cancellationToken);

    CODING_ASSERT("STATUS_NOT_IMPLEMENTED");
}

NTSTATUS TestTransactionManager::CreateEnumerator(
    __in bool parentsOnly,
    __out Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, IStateProvider2::SPtr>>::SPtr & outEnumerator) noexcept 
{
    UNREFERENCED_PARAMETER(parentsOnly);
    UNREFERENCED_PARAMETER(outEnumerator);
    return STATUS_UNSUCCESSFUL;
}

Awaitable<NTSTATUS> TestTransactionManager::GetOrAddAsync(
    __in Transaction & transaction,
    __in KUriView const & stateProviderName,
    __in KStringView const & stateProviderTypeName,
    __out IStateProvider2::SPtr & stateProvider,
    __out bool & alreadyExist,
    __in_opt OperationData const * const initializationParameters,
    __in Common::TimeSpan const & timeout,
    __in CancellationToken const & cancellationToken) noexcept
{
    UNREFERENCED_PARAMETER(transaction);
    UNREFERENCED_PARAMETER(stateProviderName);
    UNREFERENCED_PARAMETER(stateProviderTypeName);
    UNREFERENCED_PARAMETER(stateProvider);
    UNREFERENCED_PARAMETER(alreadyExist);
    UNREFERENCED_PARAMETER(initializationParameters);
    UNREFERENCED_PARAMETER(timeout);
    UNREFERENCED_PARAMETER(cancellationToken);

    CODING_ASSERT("STATUS_NOT_IMPLEMENTED");
}

Awaitable<NTSTATUS> TestTransactionManager::TryRemoveCheckpointAsync(
    __in LONG64 checkpointLsnToBeRemoved,
    __in LONG64 nextCheckpointLsn) noexcept
{
    co_return co_await loggingReplicatorSPtr_->TryRemoveCheckpointAsync(checkpointLsnToBeRemoved, nextCheckpointLsn);
}

NTSTATUS TestTransactionManager::TryRemoveVersion(
    __in LONG64 stateProviderId,
    __in LONG64 commitLsn,
    __in LONG64 nextCommitLsn,
    __out TryRemoveVersionResult::SPtr & result) noexcept
{
    return loggingReplicatorSPtr_->TryRemoveVersion(stateProviderId, commitLsn, nextCommitLsn, result);
}

void TestTransactionManager::Initialize(
    __in MockLoggingReplicator & loggingReplicator, 
    __in Data::StateManager::StateManager & stateManager)
{
    loggingReplicatorSPtr_ = &loggingReplicator;
    stateManagerSPtr_ = &stateManager;
}

Awaitable<void> TestTransactionManager::OpenAsync(
    __in CancellationToken & cancellationToken,
    __in_opt bool completeCheckpoint,
    __in_opt bool cleanupRestore)
{
    loggingReplicatorSPtr_->Role = FABRIC_REPLICA_ROLE_UNKNOWN;

    NTSTATUS status = co_await stateManagerSPtr_->OpenAsync(completeCheckpoint, cleanupRestore, CancellationToken::None);
    THROW_ON_FAILURE(status);

    role_ = FABRIC_REPLICA_ROLE_UNKNOWN;

    co_return;
}

Awaitable<void> TestTransactionManager::ChangeRoleAsync(
    __in FABRIC_REPLICA_ROLE role,
    __in CancellationToken & cancellationToken)
{
    loggingReplicatorSPtr_->Role = role;

    co_await stateManagerSPtr_->ChangeRoleAsync(role, cancellationToken);

    role_ = role;

    co_return;
}

Awaitable<void> TestTransactionManager::CloseAsync(CancellationToken & cancellationToken)
{
    loggingReplicatorSPtr_->Role = FABRIC_REPLICA_ROLE_UNKNOWN;

    NTSTATUS status =  co_await stateManagerSPtr_->CloseAsync(cancellationToken);
    THROW_ON_FAILURE(status);

    role_ = FABRIC_REPLICA_ROLE_UNKNOWN;

    co_return;
}

NTSTATUS TestTransactionManager::CreateTransaction(
    __out TxnReplicator::Transaction::SPtr & txn) noexcept
{
    txn = Transaction::CreateTransaction(*this, GetThisAllocator());
    return STATUS_SUCCESS;
}

NTSTATUS TestTransactionManager::CreateAtomicOperation(
    __out TxnReplicator::AtomicOperation::SPtr & txn) noexcept
{
    txn = AtomicOperation::CreateAtomicOperation(*this, GetThisAllocator());
    return STATUS_SUCCESS;
}

NTSTATUS TestTransactionManager::CreateAtomicRedoOperation(
    __out TxnReplicator::AtomicRedoOperation::SPtr & txn) noexcept
{
    txn = AtomicRedoOperation::CreateAtomicRedoOperation(*this, GetThisAllocator());
    return STATUS_SUCCESS;
}

NTSTATUS TestTransactionManager::SingleOperationTransactionAbortUnlock(
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in OperationContext const & operationContext) noexcept
{
    return stateManagerSPtr_->SingleOperationTransactionAbortUnlock(stateProviderId, operationContext);
}

NTSTATUS TestTransactionManager::ErrorIfStateProviderIsNotRegistered(
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in LONG64 transactionId) noexcept
{
    return this->stateManagerSPtr_->ErrorIfStateProviderIsNotRegistered(stateProviderId, transactionId);
}

NTSTATUS TestTransactionManager::BeginTransaction(
    __in Transaction & transaction, 
    __in_opt OperationData const * const metaData,
    __in_opt OperationData const * const undo,
    __in_opt OperationData const * const redo,
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in_opt OperationContext const * const operationContext) noexcept
{
    return stateManagerSPtr_->BeginTransaction(
        transaction,
        stateProviderId,
        metaData,
        undo,
        redo,
        operationContext);
}

Awaitable<NTSTATUS> TestTransactionManager::BeginTransactionAsync(
    __in Transaction & transaction,
    __in_opt OperationData const * const metaData,
    __in_opt OperationData const * const undo,
    __in_opt OperationData const * const redo,
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in_opt OperationContext const * const operationContext,
    __out FABRIC_SEQUENCE_NUMBER & lsn) noexcept
{
    co_return co_await stateManagerSPtr_->BeginTransactionAsync(
        transaction, 
        stateProviderId,
        metaData, 
        undo, 
        redo, 
        operationContext,
        lsn);
}

NTSTATUS TestTransactionManager::AddOperation(
    Transaction & transaction,
    OperationData const * const metaData,
    OperationData const * const undo,
    OperationData const * const redo,
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    OperationContext const * const operationContext) noexcept
{
    return stateManagerSPtr_->AddOperation(
        transaction,
        stateProviderId,
        metaData,
        undo,
        redo,
        operationContext);
}

Awaitable<NTSTATUS> TestTransactionManager::AddOperationAsync(
    AtomicOperation & atomicOperation,
    OperationData const * const metaData,
    OperationData const * const undo,
    OperationData const * const redo,
    __in LONG64 stateProviderId,
    OperationContext const * const operationContext,
    __out FABRIC_SEQUENCE_NUMBER & lsn) noexcept
{
    co_return co_await stateManagerSPtr_->AddOperationAsync(
        atomicOperation,
        stateProviderId,
        metaData,
        undo,
        redo,
        operationContext,
        lsn);
}

Awaitable<NTSTATUS> TestTransactionManager::AddOperationAsync(
    AtomicRedoOperation & atomicRedoOperation,
    OperationData const * const metaData,
    OperationData const * const redo,
    __in LONG64 stateProviderId,
    OperationContext const * const operationContext,
    __out FABRIC_SEQUENCE_NUMBER & lsn) noexcept
{
    co_return co_await stateManagerSPtr_->AddOperationAsync(
        atomicRedoOperation,
        stateProviderId,
        metaData,
        redo,
        operationContext,
        lsn);
}

Awaitable<NTSTATUS> TestTransactionManager::CommitTransactionAsync(
    __in Transaction & transaction,
    __out FABRIC_SEQUENCE_NUMBER & lsn) noexcept
{
    co_return co_await loggingReplicatorSPtr_->CommitTransactionAsync(transaction, lsn);
}

Awaitable<NTSTATUS> TestTransactionManager::AbortTransactionAsync(
    __in Transaction & transaction,
    __out FABRIC_SEQUENCE_NUMBER & lsn) noexcept
{
    co_return co_await loggingReplicatorSPtr_->AbortTransactionAsync(transaction, lsn);
}

Awaitable<NTSTATUS> TestTransactionManager::RegisterAsync(__out LONG64 & lsn) noexcept
{
    lsn = -1;
    co_return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS TestTransactionManager::UnRegister(__in LONG64 visibilityVersionNumber) noexcept
{
    UNREFERENCED_PARAMETER(visibilityVersionNumber);
    return STATUS_NOT_IMPLEMENTED;
}

Awaitable<LONG64> TestTransactionManager::PrepareCheckpointAsync()
{
    LONG64 lsn = co_await loggingReplicatorSPtr_->PrepareCheckpointAsync();
    co_return lsn;
}

Awaitable<void> TestTransactionManager::PerformCheckpointAsync(__in CancellationToken const & cancellationToken)
{
    co_await loggingReplicatorSPtr_->PerformCheckpointAsync(cancellationToken);
    co_return;
}

Awaitable<void> TestTransactionManager::CompleteCheckpointAsync(__in CancellationToken const & cancellationToken)
{
    co_await loggingReplicatorSPtr_->CompleteCheckpointAsync(cancellationToken);
    co_return;
}

TxnReplicator::Transaction::SPtr TestTransactionManager::CreateTransaction()
{
    return Transaction::CreateTransaction(*this, GetThisAllocator());
}

TxnReplicator::AtomicOperation::SPtr TestTransactionManager::CreateAtomicOperation()
{
    return AtomicOperation::CreateAtomicOperation(*this, GetThisAllocator());
}

TxnReplicator::AtomicRedoOperation::SPtr TestTransactionManager::CreateAtomicRedoOperation()
{
    return AtomicRedoOperation::CreateAtomicRedoOperation(*this, GetThisAllocator());
}

Awaitable<LONG64> TestTransactionManager::CheckpointAsync(__in CancellationToken const & cancellationToken)
{
    LONG64 checkpointLSN = co_await loggingReplicatorSPtr_->PrepareCheckpointAsync();

    co_await loggingReplicatorSPtr_->PerformCheckpointAsync(cancellationToken);
    co_await loggingReplicatorSPtr_->CompleteCheckpointAsync(cancellationToken);
    co_return checkpointLSN;
}

void TestTransactionManager::SetReplicationBlocking(bool blockReplication)
{
    loggingReplicatorSPtr_->SetReplicationBlocking(blockReplication);
}

void TestTransactionManager::SetSafeLSN(FABRIC_SEQUENCE_NUMBER safeLSN)
{
    loggingReplicatorSPtr_->SetSafeLSN(safeLSN);
}

Awaitable<void> TestTransactionManager::CopyAsync(
    __in OperationDataStream & operationDataStream, 
    __in CancellationToken const & cancellationToken)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    OperationDataStream::SPtr copyStream(&operationDataStream);

    status = co_await stateManagerSPtr_->BeginSettingCurrentStateAsync();
    THROW_ON_FAILURE(status);

    LONG64 stateRecordNumber = 0;
    while (true)
    {
        // GetNextAsync will return nullptr only if it's the end. 
        OperationData::CSPtr operationData;
        co_await copyStream->GetNextAsync(CancellationToken::None, operationData);
        if (operationData == nullptr)
        {
            break;
        }

        status = co_await stateManagerSPtr_->SetCurrentStateAsync(stateRecordNumber++, *operationData);
        THROW_ON_FAILURE(status);
    }

    status = co_await stateManagerSPtr_->EndSettingCurrentState();
    THROW_ON_FAILURE(status);
}

ktl::Awaitable<void> TestTransactionManager::CopyAsync(
    __in KArray<Data::Utilities::OperationData::SPtr> operationDataArray,
    __in ktl::CancellationToken const & cancellationToken)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    status = co_await stateManagerSPtr_->BeginSettingCurrentStateAsync();
    THROW_ON_FAILURE(status);

    LONG64 stateRecordNumber = 0;

    for (OperationData::SPtr operationData : operationDataArray)
    {
        status = co_await stateManagerSPtr_->SetCurrentStateAsync(stateRecordNumber++, *operationData);
        THROW_ON_FAILURE(status);
    }

    status = co_await stateManagerSPtr_->EndSettingCurrentState();
    THROW_ON_FAILURE(status);
}

TestTransactionManager::TestTransactionManager()
    : KObject()
    , KShared()
{
}

TestTransactionManager::~TestTransactionManager()
{
}
