// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace StateManagerTests;
using namespace TxnReplicator;
using namespace Data::Utilities;

MockLoggingReplicator::SPtr MockLoggingReplicator::Create(
    __in bool hasPersistedState,
    __in KAllocator& allocator)
{
    SPtr result = _new(TEST_TAG, allocator) MockLoggingReplicator(hasPersistedState);
    THROW_ON_ALLOCATION_FAILURE(result);
    THROW_ON_FAILURE(result->Status());

    return result;
}

void MockLoggingReplicator::Dispose()
{
    return;
}

bool MockLoggingReplicator::IsReadable() noexcept
{
    return isReadable_;
}

NTSTATUS MockLoggingReplicator::GetLastStableSequenceNumber(
    __out FABRIC_SEQUENCE_NUMBER & lsn) noexcept
{
    lsn = 0;
    return STATUS_SUCCESS;
}

NTSTATUS MockLoggingReplicator::GetLastCommittedSequenceNumber(
    __out FABRIC_SEQUENCE_NUMBER & lsn) noexcept
{
    lsn = lastLSN_;
    return STATUS_SUCCESS;
}

NTSTATUS MockLoggingReplicator::GetCurrentEpoch(
    __out FABRIC_EPOCH & result) noexcept
{
    FABRIC_EPOCH snap = { 0, 0 };
    result = snap;
    return STATUS_SUCCESS;
}

ktl::Awaitable<NTSTATUS> MockLoggingReplicator::BackupAsync(
    __in TxnReplicator::IBackupCallbackHandler & backupCallbackHandler,
    __out TxnReplicator::BackupInfo & result) noexcept
{
    ApiEntryAsync();
    
    co_return co_await BackupAsync(
        backupCallbackHandler, 
        FABRIC_BACKUP_OPTION_FULL, 
        Common::TimeSpan::FromSeconds(4), 
        CancellationToken::None,
        result);
}

ktl::Awaitable<NTSTATUS> MockLoggingReplicator::BackupAsync(
    __in TxnReplicator::IBackupCallbackHandler & backupCallbackHandler,
    __in FABRIC_BACKUP_OPTION backupOption,
    __in Common::TimeSpan const & timeout,
    __in CancellationToken const & cancellationToken,
    __out TxnReplicator::BackupInfo & result) noexcept
{
    ApiEntryAsync();

    UNREFERENCED_PARAMETER(timeout);

    auto stateManagerSPtr = stateManagerWRef_->TryGetTarget();
    CODING_ERROR_ASSERT(stateManagerSPtr != nullptr);

    co_await stateManagerSPtr->BackupCheckpointAsync(*backupFolderPath_, cancellationToken);

    FABRIC_EPOCH epoch;
    epoch.DataLossNumber = 1;
    epoch.ConfigurationNumber = 1;
    BackupVersion version(epoch, lastLSN_);

    Common::Guid backupId = Common::Guid::NewGuid();

    // Note: Mock LR pass in fake parentBackId and startVersion. 
    result = BackupInfo(*backupFolderPath_, backupOption, version, version, backupId, backupId, GetThisAllocator());

    co_return STATUS_SUCCESS;
}

ktl::Awaitable<NTSTATUS> MockLoggingReplicator::RestoreAsync(
    __in KString const & backupFolder) noexcept
{
    ApiEntryAsync();

    co_return co_await RestoreAsync(
        backupFolder, 
        FABRIC_RESTORE_POLICY_FORCE, 
        Common::TimeSpan::FromSeconds(4), 
        CancellationToken::None);
}

ktl::Awaitable<NTSTATUS> MockLoggingReplicator::RestoreAsync(
    __in KString const & backupFolder,
    __in FABRIC_RESTORE_POLICY restorePolicy,
    __in Common::TimeSpan const & timeout,
    __in CancellationToken const & cancellationToken) noexcept
{
    ApiEntryAsync();

    UNREFERENCED_PARAMETER(timeout);

    auto stateManagerSPtr = stateManagerWRef_->TryGetTarget();
    CODING_ERROR_ASSERT(stateManagerSPtr != nullptr);

    co_return co_await stateManagerSPtr->RestoreCheckpointAsync(*backupFolderPath_, cancellationToken);
}

void MockLoggingReplicator::SetReadable(bool value)
{
    isReadable_ = value;
}

Awaitable<NTSTATUS> MockLoggingReplicator::OpenAsync(
    __out RecoveryInformation & result) noexcept
{
    RecoveryInformation info(false, false);
    result = info;

    co_await suspend_never{};
    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> MockLoggingReplicator::CloseAsync() noexcept
{
    co_await suspend_never{};
    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> MockLoggingReplicator::PerformRecoveryAsync(
    __in RecoveryInformation const & recoveryInfo) noexcept
{
    UNREFERENCED_PARAMETER(recoveryInfo);
    co_await suspend_never{};
    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> MockLoggingReplicator::DeleteLogAsync() noexcept
{
    co_await suspend_never{};
    co_return STATUS_SUCCESS;
}

NTSTATUS MockLoggingReplicator::GetSafeLsnToRemoveStateProvider(
    __out FABRIC_SEQUENCE_NUMBER & safeLSN) noexcept
{
    safeLSN = safeLSN_;
    return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> MockLoggingReplicator::BecomeActiveSecondaryAsync(
    __in StateManagerBecomeSecondaryDelegate stateManagerBecomeSecondaryDelegate) noexcept
{
    UNREFERENCED_PARAMETER(stateManagerBecomeSecondaryDelegate);
    co_await suspend_never{};
    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> MockLoggingReplicator::BecomeIdleSecondaryAsync() noexcept
{
    co_await suspend_never{};
    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> MockLoggingReplicator::BecomeNoneAsync() noexcept
{
    co_await suspend_never{};
    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> MockLoggingReplicator::BecomePrimaryAsync(
    __in bool isForRestore) noexcept
{
    UNREFERENCED_PARAMETER(isForRestore);
    co_await suspend_never{};
    co_return STATUS_SUCCESS;
}

void MockLoggingReplicator::Initialize(
    __in IStateProviderManager& stateManager)
{
    auto status = stateManager.GetWeakIfRef(stateManagerWRef_);
    CODING_ERROR_ASSERT(NT_SUCCESS(status));
}

NTSTATUS MockLoggingReplicator::CreateTransaction(
     __out TxnReplicator::Transaction::SPtr & result) noexcept
{
    ApiEntryReturn();

    result = Transaction::CreateTransaction(*this, GetThisAllocator());

    return STATUS_SUCCESS;
}

NTSTATUS MockLoggingReplicator::CreateAtomicOperation(
    __out TxnReplicator::AtomicOperation::SPtr & result) noexcept
{
    ApiEntryReturn();

    result = AtomicOperation::CreateAtomicOperation(*this, GetThisAllocator());

    return STATUS_SUCCESS;
}

NTSTATUS MockLoggingReplicator::CreateAtomicRedoOperation(
    __out TxnReplicator::AtomicRedoOperation::SPtr & result) noexcept
{
    ApiEntryReturn();

    result = AtomicRedoOperation::CreateAtomicRedoOperation(*this, GetThisAllocator());

    return STATUS_SUCCESS;
}

NTSTATUS MockLoggingReplicator::SingleOperationTransactionAbortUnlock(
    __in LONG64 stateProviderId,
    __in OperationContext const & operationContext) noexcept
{
    UNREFERENCED_PARAMETER(stateProviderId);
    UNREFERENCED_PARAMETER(operationContext);

    CODING_ASSERT("NOT IMPLEMENTED");
}

NTSTATUS MockLoggingReplicator::ErrorIfStateProviderIsNotRegistered(
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in LONG64 transactionId) noexcept
{
    UNREFERENCED_PARAMETER(stateProviderId);
    UNREFERENCED_PARAMETER(transactionId);

    CODING_ASSERT("NOT IMPLEMENTED");
}

Awaitable<NTSTATUS> MockLoggingReplicator::BeginTransactionAsync(
    __in Transaction & transaction,
    __in_opt OperationData const * const metaData,
    __in_opt OperationData const * const undo,
    __in_opt OperationData const * const redo,
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in_opt OperationContext const * const operationContext,
    __out FABRIC_SEQUENCE_NUMBER & commitLSN) noexcept
{
    UNREFERENCED_PARAMETER(undo);
    UNREFERENCED_PARAMETER(stateProviderId);

    LONG64 snapLSN = InterlockedIncrement64(&lastLSN_);

    auto stateManagerSPtr = stateManagerWRef_->TryGetTarget();
    CODING_ERROR_ASSERT(stateManagerSPtr != nullptr);

    TransactionBase const & transactionBase = static_cast<TransactionBase const &>(transaction);

    // Acquire read lock: Multiple txn operation can happen at the same time.
    co_await readerWriterLockSPtr_->AcquireReadLockAsync(INT_MAX);
    KFinally([&]
    {
        readerWriterLockSPtr_->ReleaseReadLock();
    });

    OperationContext::CSPtr operationContextSPtr = nullptr; 
    NTSTATUS status = co_await stateManagerSPtr->ApplyAsync(
        snapLSN,
        transactionBase, 
        ApplyContext::PrimaryRedo, 
        metaData, 
        redo,
        operationContextSPtr);
    CO_RETURN_ON_FAILURE(status);
    CODING_ERROR_ASSERT(operationContextSPtr == nullptr);

    if (operationContext != nullptr)
    {
        status = stateManagerSPtr->Unlock(*operationContext);
        CO_RETURN_ON_FAILURE(status);
    }

    commitLSN = snapLSN;
    co_return STATUS_SUCCESS;
}

NTSTATUS MockLoggingReplicator::BeginTransaction(
    __in Transaction & transaction,
    __in_opt OperationData const * const metaData,
    __in_opt OperationData const * const undo,
    __in_opt OperationData const * const redo,
    __in LONG64 stateProviderId,
    __in_opt OperationContext const * const operationContext) noexcept
{
    UNREFERENCED_PARAMETER(stateProviderId);

    NTSTATUS status;

    LONG64 operationLSN = InterlockedIncrement64(&lastLSN_);

    TestReplicatedOperation operation(operationLSN, metaData, undo, redo, operationContext);

    KSharedArray<TestReplicatedOperation>::SPtr inflightOperationArray = _new(GetThisAllocationTag(), GetThisAllocator())KSharedArray<TestReplicatedOperation>();
    CODING_ERROR_ASSERT(inflightOperationArray != nullptr);
    CODING_ERROR_ASSERT(NT_SUCCESS(inflightOperationArray->Status()));

    status = inflightOperationArray->Append(operation);
    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    bool itemAdded = inflightTransactionMapSPtr_->TryAdd(transaction.TransactionId, inflightOperationArray);
    CODING_ERROR_ASSERT(itemAdded);

    return status;
}

NTSTATUS MockLoggingReplicator::AddOperation(
    __in Transaction & transaction,
    __in_opt OperationData const * const metaData,
    __in_opt OperationData const * const undo,
    __in_opt OperationData const * const redo,
    __in LONG64 stateProviderId,
    __in_opt OperationContext const * const operationContext) noexcept
{
    UNREFERENCED_PARAMETER(stateProviderId);

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    status = InjectFaultIfNecessary();
    RETURN_ON_FAILURE(status);

    LONG64 operationLSN = InterlockedIncrement64(&lastLSN_);

    TestReplicatedOperation operation(operationLSN, metaData, undo, redo, operationContext);

    KSharedArray<TestReplicatedOperation>::SPtr inflightOperations = nullptr;
    bool itemExists = inflightTransactionMapSPtr_->TryGetValue(transaction.TransactionId, inflightOperations);
    CODING_ERROR_ASSERT(itemExists);

    status = inflightOperations->Append(operation);
    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    return status;
}

Awaitable<NTSTATUS> MockLoggingReplicator::AddOperationAsync(
    __in AtomicOperation & atomicOperation,
    __in_opt OperationData const * const metaData,
    __in_opt OperationData const * const undo,
    __in_opt OperationData const * const redo,
    __in LONG64 stateProviderId,
    __in_opt OperationContext const * const operationContext,
    __out FABRIC_SEQUENCE_NUMBER & commitLsn) noexcept
{
    UNREFERENCED_PARAMETER(stateProviderId);

    NTSTATUS status;

    LONG64 snapLSN = InterlockedIncrement64(&lastLSN_);

    auto stateManagerSPtr = stateManagerWRef_->TryGetTarget();
    CODING_ERROR_ASSERT(stateManagerSPtr != nullptr);

    TransactionBase const & transactionBase = static_cast<TransactionBase const &>(atomicOperation);

    // Acquire read lock: Multile txn operation can happen at the same time.
    co_await readerWriterLockSPtr_->AcquireReadLockAsync(INT_MAX);
    KFinally([&]
    {
        readerWriterLockSPtr_->ReleaseReadLock();
    });

    OperationContext::CSPtr operationContextSPtr = nullptr;
    status = co_await stateManagerSPtr->ApplyAsync(
        snapLSN,
        transactionBase,
        ApplyContext::PrimaryRedo,
        metaData,
        redo,
        operationContextSPtr);
    CO_RETURN_ON_FAILURE(status);

    CODING_ERROR_ASSERT(operationContextSPtr == nullptr);
    if (operationContext != nullptr)
    {
        status = stateManagerSPtr->Unlock(*operationContext);
        CO_RETURN_ON_FAILURE(status);
    }

    commitLsn = status;
    co_return status;
}

Awaitable<NTSTATUS> MockLoggingReplicator::AddOperationAsync(
    __in AtomicRedoOperation & atomicRedoOperation,
    __in_opt OperationData const * const metaData,
    __in_opt OperationData const * const redo,
    __in LONG64 stateProviderId,
    __in_opt OperationContext const * const operationContext,
    __out FABRIC_SEQUENCE_NUMBER & commitLSN) noexcept
{
    UNREFERENCED_PARAMETER(stateProviderId);

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    LONG64 snapLSN = InterlockedIncrement64(&lastLSN_);

    auto stateManagerSPtr = stateManagerWRef_->TryGetTarget();
    CODING_ERROR_ASSERT(stateManagerSPtr != nullptr);

    TransactionBase const & transactionBase = static_cast<TransactionBase const &>(atomicRedoOperation);

    // Acquire read lock: Multile txn operation can happen at the same time.
    co_await readerWriterLockSPtr_->AcquireReadLockAsync(INT_MAX);
    KFinally([&] { readerWriterLockSPtr_->ReleaseReadLock(); });

    OperationContext::CSPtr operationContextSPtr = nullptr;
    status = co_await stateManagerSPtr->ApplyAsync(
        snapLSN,
        transactionBase,
        ApplyContext::PrimaryRedo,
        metaData,
        redo,
        operationContextSPtr);
    CO_RETURN_ON_FAILURE(status);

    CODING_ERROR_ASSERT(operationContextSPtr == nullptr);
    if (operationContext != nullptr)
    {
        status = stateManagerSPtr->Unlock(*operationContext);
        CO_RETURN_ON_FAILURE(status);
    }

    commitLSN = snapLSN;
    co_return status;
}

Awaitable<NTSTATUS> MockLoggingReplicator::CommitTransactionAsync(
    __in Transaction & transaction,
    __out FABRIC_SEQUENCE_NUMBER & commitLSN) noexcept
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    KSharedArray<TestReplicatedOperation>::SPtr inflightOperations = nullptr;
    auto transactionExists = inflightTransactionMapSPtr_->TryRemove(transaction.TransactionId, inflightOperations);
    CODING_ERROR_ASSERT(transactionExists);

    auto stateManagerSPtr = stateManagerWRef_->TryGetTarget();
    CODING_ERROR_ASSERT(stateManagerSPtr != nullptr);

    // Acquire read lock: Multiple txn operation can happen at the same time.
    co_await readerWriterLockSPtr_->AcquireReadLockAsync(INT_MAX);
    KFinally([&] { readerWriterLockSPtr_->ReleaseReadLock(); });

    LONG64 snapLSN = InterlockedIncrement64(&lastLSN_);

    for (ULONG i = 0; i < inflightOperations->Count(); i++)
    {
        auto operation = (*inflightOperations)[i];
        OperationContext::CSPtr operationContextSPtr = nullptr;
        status = co_await stateManagerSPtr->ApplyAsync(
            operation.SequenceNumber,
            transaction,
            ApplyContext::PrimaryRedo,
            operation.Metadata.RawPtr(),
            operation.Redo.RawPtr(),
            operationContextSPtr);
        CO_RETURN_ON_FAILURE(status);

        CODING_ERROR_ASSERT(operationContextSPtr == nullptr);
    }

    for (ULONG i = 0; i < inflightOperations->Count(); i++)
    {
        auto operation = (*inflightOperations)[i];
        if (operation.Context != nullptr)
        {
            status = stateManagerSPtr->Unlock(*operation.Context);
            CO_RETURN_ON_FAILURE(status);
        }
    }

    commitLSN = snapLSN;
    co_return status;
}

Awaitable<NTSTATUS> MockLoggingReplicator::AbortTransactionAsync(
    __in Transaction & transaction,
    __out FABRIC_SEQUENCE_NUMBER & commitLSN) noexcept
{
    KSharedArray<TestReplicatedOperation>::SPtr inflightOperations = nullptr;
    auto transactionExists = inflightTransactionMapSPtr_->TryRemove(transaction.TransactionId, inflightOperations);
    CODING_ERROR_ASSERT(transactionExists);

    auto stateManagerSPtr = stateManagerWRef_->TryGetTarget();
    CODING_ERROR_ASSERT(stateManagerSPtr != nullptr);

    // Acquire read lock: Multiple txn operation can happen at the same time.
    co_await readerWriterLockSPtr_->AcquireReadLockAsync(INT_MAX);
    KFinally([&] {readerWriterLockSPtr_->ReleaseReadLock(); });

    commitLSN = InterlockedIncrement64(&lastLSN_);
    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> MockLoggingReplicator::RegisterAsync(
    __out FABRIC_SEQUENCE_NUMBER & visibilityLSN) noexcept
{
    UNREFERENCED_PARAMETER(visibilityLSN);
    CODING_ASSERT("NOT IMPLEMENTED");
}

NTSTATUS MockLoggingReplicator::UnRegister(
    __in LONG64 visibilityVersionNumber) noexcept
{
    UNREFERENCED_PARAMETER(visibilityVersionNumber);
    CODING_ASSERT("NOT IMPLEMENTED");
}

ktl::Awaitable<NTSTATUS> MockLoggingReplicator::TryRemoveCheckpointAsync(
    __in FABRIC_SEQUENCE_NUMBER checkpointLsnToBeRemoved, 
    __in FABRIC_SEQUENCE_NUMBER nextCheckpointLsn) noexcept
{
    UNREFERENCED_PARAMETER(checkpointLsnToBeRemoved);
    UNREFERENCED_PARAMETER(nextCheckpointLsn);

    CODING_ASSERT("NOT IMPLEMENTED");
}

NTSTATUS MockLoggingReplicator::TryRemoveVersion(
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in FABRIC_SEQUENCE_NUMBER commitLsn,
    __in FABRIC_SEQUENCE_NUMBER nextCommitLsn,
    __out TryRemoveVersionResult::SPtr & result) noexcept
{
    UNREFERENCED_PARAMETER(stateProviderId);
    UNREFERENCED_PARAMETER(commitLsn);
    UNREFERENCED_PARAMETER(nextCommitLsn);

    result = nullptr;
    CODING_ASSERT("NOT IMPLEMENTED");
}

ktl::Awaitable<FABRIC_SEQUENCE_NUMBER> MockLoggingReplicator::PrepareCheckpointAsync()
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    auto stateManagerSPtr = stateManagerWRef_->TryGetTarget();
    CODING_ERROR_ASSERT(stateManagerSPtr != nullptr);

    co_await readerWriterLockSPtr_->AcquireWriteLockAsync(INT_MAX);
    KFinally([&] { readerWriterLockSPtr_->ReleaseWriteLock(); });

    // Simulate a barrier
    LONG64 barrierLSN = InterlockedIncrement64(&lastLSN_);
    status = stateManagerSPtr->PrepareCheckpoint(barrierLSN);
    THROW_ON_FAILURE(status);

    co_return barrierLSN;
}

Awaitable<void> MockLoggingReplicator::PerformCheckpointAsync(__in CancellationToken const & cancellationToken)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    auto stateManagerSPtr = stateManagerWRef_->TryGetTarget();
    CODING_ERROR_ASSERT(stateManagerSPtr != nullptr);

    status = co_await stateManagerSPtr->PerformCheckpointAsync(cancellationToken);
    THROW_ON_FAILURE(status);

    co_return;
}

Awaitable<void> MockLoggingReplicator::CompleteCheckpointAsync(__in CancellationToken const & cancellationToken)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    auto stateManagerSPtr = stateManagerWRef_->TryGetTarget();
    CODING_ERROR_ASSERT(stateManagerSPtr != nullptr);

    status = co_await stateManagerSPtr->CompleteCheckpointAsync(cancellationToken);
    THROW_ON_FAILURE(status);

    co_return;
}

void MockLoggingReplicator::SetReplicationBlocking(__in bool blockReplication)
{
    isReplicationBlocked_ = blockReplication;
}

void MockLoggingReplicator::SetSafeLSN(__in FABRIC_SEQUENCE_NUMBER safeLSN)
{
    safeLSN_ = safeLSN;
}

NTSTATUS MockLoggingReplicator::InjectFaultIfNecessary() noexcept
{
    if (isReplicationBlocked_)
    {
        return SF_STATUS_REPLICATION_QUEUE_FULL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS MockLoggingReplicator::RegisterTransactionChangeHandler(
    __in ITransactionChangeHandler & transactionChangeHandler) noexcept
{
    UNREFERENCED_PARAMETER(transactionChangeHandler);
    CODING_ASSERT("NOT IMPLEMENTED");
}

NTSTATUS MockLoggingReplicator::UnRegisterTransactionChangeHandler() noexcept
{
    CODING_ASSERT("NOT IMPLEMENTED");
}

MockLoggingReplicator::MockLoggingReplicator(__in bool hasPersistedState)
    : KAsyncServiceBase()
    , KWeakRefType<MockLoggingReplicator>()
    , hasPersistedState_(hasPersistedState)
{
    NTSTATUS status = Data::Utilities::ReaderWriterAsyncLock::Create(GetThisAllocator(), GetThisAllocationTag(), readerWriterLockSPtr_);
    CODING_ERROR_ASSERT(NT_SUCCESS(status));
    
    status = Data::Utilities::ConcurrentDictionary<LONG64, KSharedArray<TestReplicatedOperation>::SPtr>::Create(
        GetThisAllocator(), 
        inflightTransactionMapSPtr_);
    CODING_ERROR_ASSERT(NT_SUCCESS(status));
}

MockLoggingReplicator::~MockLoggingReplicator()
{
}
