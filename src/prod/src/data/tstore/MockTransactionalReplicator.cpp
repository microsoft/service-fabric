// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::TStore;
using namespace TxnReplicator;
using namespace TStoreTests;

MockTransactionalReplicator::MockTransactionalReplicator(__in bool hasPersistedState)
    : KAsyncServiceBase()
    , KWeakRefType<MockTransactionalReplicator>()
    , delayAddOperationAndThrow_(false)
    , timeout_(5000)
    , hasPersistedState_(hasPersistedState)
{
    NTSTATUS status = Data::Utilities::ReaderWriterAsyncLock::Create(GetThisAllocator(), GetThisAllocationTag(), readerWriterLockSPtr_);
    this->SetConstructorStatus(status);

    status = Data::Utilities::ConcurrentDictionary<LONG64, KSharedArray<TestTransactionContext>::SPtr>::Create(
        GetThisAllocator(),
        inflightTransactionMapSPtr_);
    this->SetConstructorStatus(status);

    status = Data::Utilities::ConcurrentDictionary<LONG64, IStateProvider2::SPtr>::Create(
        this->GetThisAllocator(),
        stateProvidersSPtr_);
    this->SetConstructorStatus(status);

    status = Data::Utilities::ReaderWriterAsyncLock::Create(GetThisAllocator(), GetThisAllocationTag(), prepareApplyLockSPtr_);

    partition_ = Data::TestCommon::TestStatefulServicePartition::Create(GetThisAllocator());

    secondariesReplicatorSPtr_ = _new(TEST_TAG, GetThisAllocator()) KSharedArray<MockTransactionalReplicator::SPtr>();
    if (!secondariesReplicatorSPtr_)
    {
        this->SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
    }

    this->SetConstructorStatus(secondariesReplicatorSPtr_->Status());
}

MockTransactionalReplicator::~MockTransactionalReplicator()
{
   stateProvidersSPtr_ = nullptr;
   //stateProvidersSPtr_->Clear();
}

NTSTATUS MockTransactionalReplicator::Create(
   __in KAllocator& allocator,
   __in bool hasPersistedState,
   __out MockTransactionalReplicator::SPtr& result)
{
   NTSTATUS status;
   SPtr output = _new(TEST_TAG, allocator) MockTransactionalReplicator(hasPersistedState);

   if (!output)
   {
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   status = output->Status();
   if (!NT_SUCCESS(status))
   {
      return status;
   }

   result = Ktl::Move(output);
   return STATUS_SUCCESS;
}

void MockTransactionalReplicator::Initialize(__in PartitionedReplicaId const & traceId)
{
   IVersionProvider::SPtr versionProvider = static_cast<IVersionProvider *>(this);
   NTSTATUS status = Data::LoggingReplicator::VersionManagerFactory::Create(traceId, *versionProvider, GetThisAllocator(), versionManager_);
   KInvariant(NT_SUCCESS(status));
}

void MockTransactionalReplicator::RegisterSecondary(__in MockTransactionalReplicator & replicator)
{
    CODING_ERROR_ASSERT(role_ == FABRIC_REPLICA_ROLE_PRIMARY);

    MockTransactionalReplicator::SPtr replicatorSPtr (&replicator);
    NTSTATUS status = secondariesReplicatorSPtr_->Append(replicatorSPtr);
    CODING_ERROR_ASSERT(NT_SUCCESS(status));
}

ktl::Awaitable<void> MockTransactionalReplicator::CommitAsync(__in TxnReplicator::Transaction & tx)
   {
    if (role_ == FABRIC_REPLICA_ROLE_PRIMARY)
    {
        co_await ReplicateCommitAsync(tx);
    }

    KSharedArray<TestTransactionContext>::SPtr inflightOperations = nullptr;
    auto transactionExists = inflightTransactionMapSPtr_->TryRemove(tx.TransactionId, inflightOperations);
    KInvariant(transactionExists);

    // Acquire read lock: Multile txn operation can happen at the same time.
    bool isWriteLockAcquired = co_await readerWriterLockSPtr_->AcquireReadLockAsync(INT_MAX);
    KInvariant(isWriteLockAcquired);
    KFinally([&]
    {
        readerWriterLockSPtr_->ReleaseReadLock();
    });

    KArray<TxnReplicator::OperationContext::CSPtr> contexts(GetThisAllocator());

    // todo: Remove the below
    IStateProvider2::SPtr spSPtr = stateProvider_;

    for (ULONG i = 0; i < inflightOperations->Count(); i++)
    {
        auto operation = (*inflightOperations)[i];
        //Data::StateManager::IStateProvider2::SPtr spSPtr = nullptr;
        //bool success = stateProvidersSPtr_->TryGetValue(operation.Name, spSPtr);
        //KInvariant(success);
     
        bool acquired = false;
        if (shouldSynchPrepareAndApply_)
        {
            acquired = co_await prepareApplyLockSPtr_->AcquireReadLockAsync(100);
        }

        KFinally([&] {
            if (acquired)
            {
                prepareApplyLockSPtr_->ReleaseReadLock();
            }
        });

        auto operationContextSPtr = co_await spSPtr->ApplyAsync(
            operation.SequenceNumber,
            tx,
            role_ == FABRIC_REPLICA_ROLE_PRIMARY ? ApplyContext::PrimaryRedo: ApplyContext::SecondaryRedo,
            operation.MetaData.RawPtr(),
            operation.Redo.RawPtr());


        if (operationContextSPtr != nullptr)
        {
            contexts.Append(operationContextSPtr);
        }
    }
}

void MockTransactionalReplicator::ReplicateTransaction(
    __in TxnReplicator::Transaction & tx,
    __in LONG64 stateProviderId,
    __in TestTransactionContext operation)
{
    CODING_ERROR_ASSERT(role_ == FABRIC_REPLICA_ROLE_PRIMARY);

    for (ULONG i = 0; i < secondariesReplicatorSPtr_->Count(); i++)
    {
        (*secondariesReplicatorSPtr_)[i]->AddTransaction(tx, stateProviderId, operation);
    }

    return;
}

ktl::Awaitable<void> MockTransactionalReplicator::ReplicateCommitAsync(__in TxnReplicator::Transaction & tx)
{
    CODING_ERROR_ASSERT(role_ == FABRIC_REPLICA_ROLE_PRIMARY);

    for (ULONG i = 0; i < secondariesReplicatorSPtr_->Count(); i++)
    {
        co_await (*secondariesReplicatorSPtr_)[i]->CommitAsync(tx);
    }
}

bool MockTransactionalReplicator::IsReadable()
{
    return isReadable_;
}

bool MockTransactionalReplicator::get_IsReadable() const
{
    return isReadable_;
}

void MockTransactionalReplicator::SetReadable(bool value)
{
    isReadable_ = value;
}

IStatefulPartition::SPtr MockTransactionalReplicator::get_StatefulPartition() const
{
    IStatefulPartition::SPtr statefulServicePartitionSPtr(partition_.RawPtr());
    return statefulServicePartitionSPtr;
}

void MockTransactionalReplicator::SetWriteStatus(__in FABRIC_SERVICE_PARTITION_ACCESS_STATUS writeStatus)
{
    partition_->SetWriteStatus(writeStatus);
}

void MockTransactionalReplicator::SetReadStatus(__in FABRIC_SERVICE_PARTITION_ACCESS_STATUS readStatus)
{
    partition_->SetReadStatus(readStatus);
}

void MockTransactionalReplicator::ClearSecondary()
{
    secondariesReplicatorSPtr_->Clear();
}

ktl::Awaitable<void> MockTransactionalReplicator::SimulateFalseProgressAsync(__in TxnReplicator::TransactionBase& tx)
{
    // Remove the transaction on the Primary - we are simulating false progress, so no operations should be applied.
    KInvariant(role_ == FABRIC_REPLICA_ROLE_PRIMARY);
    KSharedArray<TestTransactionContext>::SPtr inflightOperationArray = nullptr;
    bool success = inflightTransactionMapSPtr_->TryRemove(tx.TransactionId, inflightOperationArray);
    KInvariant(success);

    LONG64 commitSequenceNumber = IncrementAndGetCommitSequenceNumber();
    tx.CommitSequenceNumber = commitSequenceNumber;

    // Coordinate simulating false progress on all secondaries
    for (ULONG i = 0; i < secondariesReplicatorSPtr_->Count(); i++)
    {
        co_await (*secondariesReplicatorSPtr_)[i]->SimulateSecondaryFalseProgressAsync(tx);
    }
}

ktl::Awaitable<void> MockTransactionalReplicator::SimulateSecondaryFalseProgressAsync(__in TxnReplicator::TransactionBase& tx)
{
    KInvariant(role_ != FABRIC_REPLICA_ROLE_PRIMARY);

    KSharedArray<TestTransactionContext>::SPtr inflightOperationArray = nullptr;
    bool success = inflightTransactionMapSPtr_->TryRemove(tx.TransactionId, inflightOperationArray);
    KInvariant(success);

    // First perform forward progress
    KArray<TxnReplicator::OperationContext::CSPtr> contexts(GetThisAllocator());
    IStateProvider2::SPtr spSPtr = stateProvider_;

    for (ULONG i = 0; i < inflightOperationArray->Count(); i++)
    {
        auto operation = (*inflightOperationArray)[i];

        auto operationContextSPtr = co_await spSPtr->ApplyAsync(
            operation.SequenceNumber,
            tx,
            ApplyContext::SecondaryRedo,
            operation.MetaData.RawPtr(),
            operation.Redo.RawPtr());

        if (operationContextSPtr != nullptr)
        {
            contexts.Append(operationContextSPtr);
        }
    }

    // Unlock contexts if given by the state provider
    for (ULONG32 i = 0; i < contexts.Count(); i++)
    {
        spSPtr->Unlock(*contexts[i]);
    }

    // Undo false progress in reverse order
    contexts.Clear();
    for (LONG i = inflightOperationArray->Count() - 1; i >= 0; i--)
    {
        auto operation = (*inflightOperationArray)[i];

        auto operationContextSPtr = co_await spSPtr->ApplyAsync(
            operation.SequenceNumber,
            tx,
            ApplyContext::SecondaryFalseProgress,
            operation.MetaData.RawPtr(),
            operation.Undo.RawPtr());

        if (operationContextSPtr != nullptr)
        {
            contexts.Append(operationContextSPtr);
        }
    }

    // Unlock contexts if given by the state provider
    for (ULONG32 i = 0; i < contexts.Count(); i++)
    {
        spSPtr->Unlock(*contexts[i]);
    }
}

ktl::Awaitable<void> MockTransactionalReplicator::OpenAsync()
{
    //todo: open all stateproviders
    co_await stateProvider_->OpenAsync(CancellationToken::None);
    co_await stateProvider_->RecoverCheckpointAsync(CancellationToken::None);
    co_await stateProvider_->ChangeRoleAsync(role_, CancellationToken::None);
}

ktl::Awaitable<void> MockTransactionalReplicator::RemoveStateAndCloseAsync(bool doNotRemoveState)
{
    try
    {
        co_await prepareApplyLockSPtr_->AcquireWriteLockAsync(10'000);
        prepareApplyLockSPtr_->ReleaseWriteLock();
    }
    catch (ktl::Exception const &)
    {
        // Swallow exceptions
    }

    //todo: remove all stateproviders
    co_await stateProvider_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);

    if (!doNotRemoveState)
    {
       co_await stateProvider_->RemoveStateAsync(CancellationToken::None);
    }

    co_await stateProvider_->CloseAsync(CancellationToken::None);
}

NTSTATUS MockTransactionalReplicator::RegisterTransactionChangeHandler(
    __in ITransactionChangeHandler& transactionChangeHandler) noexcept
{
    UNREFERENCED_PARAMETER(transactionChangeHandler);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS MockTransactionalReplicator::UnRegisterTransactionChangeHandler() noexcept
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS MockTransactionalReplicator::RegisterStateManagerChangeHandler(
    __in IStateManagerChangeHandler& stateManagerChangeHandler) noexcept
{
    UNREFERENCED_PARAMETER(stateManagerChangeHandler);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS MockTransactionalReplicator::UnRegisterStateManagerChangeHandler() noexcept
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS MockTransactionalReplicator::CreateTransaction(Transaction::SPtr & result) noexcept
{
    result = Transaction::CreateTransaction(
        *this,
        GetThisAllocator());

    return STATUS_SUCCESS;
}

NTSTATUS MockTransactionalReplicator::CreateAtomicOperation(AtomicOperation::SPtr & result) noexcept
{
    result = AtomicOperation::CreateAtomicOperation(
        *this,
        GetThisAllocator());

    return STATUS_SUCCESS;
}

NTSTATUS MockTransactionalReplicator::CreateAtomicRedoOperation(AtomicRedoOperation::SPtr & result) noexcept
{
    result = AtomicRedoOperation::CreateAtomicRedoOperation(
        *this,
        GetThisAllocator());

    return STATUS_SUCCESS;
}

NTSTATUS MockTransactionalReplicator::SingleOperationTransactionAbortUnlock(
    __in LONG64 stateProviderId,
    OperationContext const & operationContext) noexcept
{
    IStateProvider2::SPtr spSPtr = nullptr;
    bool success = stateProvidersSPtr_->TryGetValue(stateProviderId, spSPtr);
    KInvariant(success);
    spSPtr->Unlock(operationContext);

    return STATUS_SUCCESS;
}

NTSTATUS MockTransactionalReplicator::ErrorIfStateProviderIsNotRegistered(
    __in LONG64 stateProviderId,
    __in LONG64 transactionId) noexcept
{
    UNREFERENCED_PARAMETER(stateProviderId);
    UNREFERENCED_PARAMETER(transactionId);
    
    return STATUS_SUCCESS;
}

bool MockTransactionalReplicator::TryGetStateProvider(
    __in LONG64 stateProviderId,
    __out IStateProvider2::SPtr& result)
{
    bool success = stateProvidersSPtr_->TryGetValue(stateProviderId, result);
    return success;
}

ktl::Awaitable<void> MockTransactionalReplicator::AddAsync(
    __in Transaction& transaction, 
    __in LONG64 stateProviderId,
    __in TxnReplicator::IStateProvider2& stateProvider,
    __in_opt Data::Utilities::OperationData const * const initializationParameters)
{
    IStateProvider2::SPtr spSPtr(&stateProvider);
    bool success = stateProvidersSPtr_->TryAdd(stateProviderId, spSPtr);
    KInvariant(success);
    co_await suspend_never();
}

Awaitable<NTSTATUS> MockTransactionalReplicator::BeginTransactionAsync(
    __in Transaction & transaction,
    __in_opt OperationData const * const metaData,
    __in_opt OperationData const * const undo,
    __in_opt OperationData const * const redo,
    __in LONG64 stateProviderId,
    __in_opt OperationContext const * const operationContext,
    __out LONG64 & result) noexcept
{
    NTSTATUS status = BeginTransaction(transaction, metaData, undo, redo, stateProviderId, operationContext);
    status = co_await CommitTransactionAsync(transaction, result);
    co_return status;
}

NTSTATUS MockTransactionalReplicator::BeginTransaction(
    __in Transaction & transaction,
    __in_opt OperationData const * const metaData,
    __in OperationData const * const undo,
    __in OperationData const * const redo,
    __in LONG64 stateProviderId,
    __in_opt OperationContext const * const operationContext) noexcept
{
    LONG64 operationLSN = InterlockedIncrement64(&lastLSN_);
    TestTransactionContext operation(operationLSN, metaData, undo, redo, operationContext, stateProviderId);
    AddTransaction(transaction, stateProviderId, operation);
    return STATUS_SUCCESS;
}

ktl::Awaitable<NTSTATUS> MockTransactionalReplicator::WaitForTimeoutAndThrowAsync()
{
    NTSTATUS status = co_await KTimer::StartTimerAsync(
        this->GetThisAllocator(),
        this->GetThisAllocationTag(),
        timeout_,
        nullptr);
    KInvariant(NT_SUCCESS(status));
    co_return SF_STATUS_NO_WRITE_QUORUM;
}

NTSTATUS MockTransactionalReplicator::AddOperation(
    Transaction & transaction,
    OperationData const * const metaData,
    OperationData const * const undo,
    OperationData const * const redo,
    __in LONG64 stateProviderId,
    OperationContext const * const operationContext) noexcept
{
    if (delayAddOperationAndThrow_)
    {
        KNt::Sleep(timeout_);
        return SF_STATUS_NO_WRITE_QUORUM;
    }

    LONG64 operationLSN = InterlockedIncrement64(&lastLSN_);
    TestTransactionContext operation(operationLSN, metaData, undo, redo, operationContext, stateProviderId);
    AddTransaction(transaction, stateProviderId, operation);

    return STATUS_SUCCESS;
}

void MockTransactionalReplicator::AddTransaction(
    Transaction & transaction,
    __in LONG64 stateProviderId,
    TestTransactionContext operation)
{
    KSharedArray<TestTransactionContext>::SPtr inflightOperations = nullptr;
    bool itemExists = inflightTransactionMapSPtr_->TryGetValue(transaction.TransactionId, inflightOperations);

    NTSTATUS status;
    if (itemExists)
    {
        status = inflightOperations->Append(operation);
        KInvariant(NT_SUCCESS(status));
    }
    else
    {
        KSharedArray<TestTransactionContext>::SPtr inflightOperationArray = _new(GetThisAllocationTag(), GetThisAllocator())KSharedArray<TestTransactionContext>();
        KInvariant(inflightOperationArray != nullptr);
        KInvariant(NT_SUCCESS(inflightOperationArray->Status()));

        status = inflightOperationArray->Append(operation);
        KInvariant(NT_SUCCESS(status));

        bool itemAdded = inflightTransactionMapSPtr_->TryAdd(transaction.TransactionId, inflightOperationArray);
        KInvariant(itemAdded);
    }

    if (role_ == FABRIC_REPLICA_ROLE_PRIMARY)
    {
        ReplicateTransaction(transaction, stateProviderId, operation);
    }
}

Awaitable<NTSTATUS> MockTransactionalReplicator::AddOperationAsync(
    AtomicOperation & atomicOperation,
    OperationData const * const metaData,
    OperationData const * const undo,
    OperationData const * const redo,
    __in LONG64 stateProviderId,
    OperationContext const * const operationContext,
    __out LONG64 & lsn) noexcept
{
    lsn = -1;
    //not used in transaction/store 
    UNREFERENCED_PARAMETER(atomicOperation);
    UNREFERENCED_PARAMETER(metaData);
    UNREFERENCED_PARAMETER(undo);
    UNREFERENCED_PARAMETER(redo);
    UNREFERENCED_PARAMETER(stateProviderId);
    UNREFERENCED_PARAMETER(operationContext);
    co_return STATUS_NOT_IMPLEMENTED;
}

Awaitable<NTSTATUS> MockTransactionalReplicator::AddOperationAsync(
    AtomicRedoOperation & atomicRedoOperation,
    OperationData const * const metaData,
    OperationData const * const redo,
    __in LONG64 stateProviderId,
    OperationContext const * const operationContext,
    __out LONG64 & lsn) noexcept
{
    lsn = -1;
    //Method no need since atomicRedoOperation is not used in TStore anymore. TStore won't support clearAll method.
    UNREFERENCED_PARAMETER(atomicRedoOperation);
    UNREFERENCED_PARAMETER(metaData);
    UNREFERENCED_PARAMETER(redo);
    UNREFERENCED_PARAMETER(stateProviderId);
    UNREFERENCED_PARAMETER(operationContext);
    co_return STATUS_NOT_IMPLEMENTED;
}

Awaitable<NTSTATUS> MockTransactionalReplicator::CommitTransactionAsync(
    __in Transaction & transaction,
    __out LONG64 & lsn) noexcept
{
    lsn = InterlockedIncrement64(&lastLSN_);
    transaction.CommitSequenceNumber = lsn;
    co_await CommitAsync(transaction);
    UpdateSecondaryLSN();
    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> MockTransactionalReplicator::AbortTransactionAsync(
    Transaction & transaction,
    __out LONG64 & lsn) noexcept
{
    lsn = -1;
    UNREFERENCED_PARAMETER(transaction);
    co_return STATUS_NOT_IMPLEMENTED;
}

Awaitable<NTSTATUS> MockTransactionalReplicator::RegisterAsync(__out LONG64 & lsn) noexcept
{
     ITransactionVersionManager::SPtr txnVersionManager = dynamic_cast<ITransactionVersionManager *>(versionManager_.RawPtr());
     co_return co_await txnVersionManager->RegisterAsync(lsn);
}

NTSTATUS MockTransactionalReplicator::UnRegister(LONG64 visibilityVersionNumber) noexcept
{
   ITransactionVersionManager::SPtr txnVersionManager = dynamic_cast<ITransactionVersionManager *>(versionManager_.RawPtr());
   return txnVersionManager->UnRegister(visibilityVersionNumber);
}

Awaitable<NTSTATUS> MockTransactionalReplicator::TryRemoveCheckpointAsync(
    LONG64 checkpointLsnToBeRemoved,
    LONG64 nextCheckpointLsn) noexcept
{
   return versionManager_->TryRemoveCheckpointAsync(checkpointLsnToBeRemoved, nextCheckpointLsn);
}

NTSTATUS MockTransactionalReplicator::TryRemoveVersion(
    LONG64 stateProviderId,
    LONG64 commitLsn,
    LONG64 nextCommitLsn,
    __out TryRemoveVersionResult::SPtr & result) noexcept
{
   return versionManager_->TryRemoveVersion(stateProviderId, commitLsn, nextCommitLsn, result);
}

NTSTATUS MockTransactionalReplicator::GetLastStableSequenceNumber(__out LONG64 & lsn) noexcept
{
    lsn = 0;
    return STATUS_SUCCESS;
}

NTSTATUS MockTransactionalReplicator::GetLastCommittedSequenceNumber(__out LONG64 & lsn) noexcept
{
    lsn = lastLSN_;
    return STATUS_SUCCESS;
}

NTSTATUS MockTransactionalReplicator::GetCurrentEpoch(__out FABRIC_EPOCH & result) noexcept
{
    FABRIC_EPOCH e = {0, 0};
    result = e;
    return STATUS_SUCCESS;
}

LONG64 MockTransactionalReplicator::IncrementAndGetCommitSequenceNumber()
{
    KInvariant(role_ == FABRIC_REPLICA_ROLE_PRIMARY);
    auto lsn = InterlockedIncrement64(&lastLSN_);
    UpdateSecondaryLSN();
    return lsn;
}

NTSTATUS MockTransactionalReplicator::Get(
    __in KUriView const & stateProviderName,
    __out IStateProvider2::SPtr & stateProvider) noexcept
{
    UNREFERENCED_PARAMETER(stateProviderName);
    UNREFERENCED_PARAMETER(stateProvider);

    return STATUS_NOT_IMPLEMENTED;
}

Awaitable<NTSTATUS> MockTransactionalReplicator::AddAsync(
    __in TxnReplicator::Transaction & transaction,
    __in KUriView const & stateProviderName,
    __in KStringView const & stateProvider,
    __in_opt Data::Utilities::OperationData const * const initializationParameters,
    __in Common::TimeSpan const & timeout,
    __in CancellationToken const & cancellationToken) noexcept
{
    UNREFERENCED_PARAMETER(transaction);
    UNREFERENCED_PARAMETER(stateProviderName);
    UNREFERENCED_PARAMETER(stateProvider);
    UNREFERENCED_PARAMETER(initializationParameters);
    UNREFERENCED_PARAMETER(timeout);
    UNREFERENCED_PARAMETER(cancellationToken);

    co_await suspend_never{};
    co_return STATUS_NOT_IMPLEMENTED;
}

void MockTransactionalReplicator::RegisterStateProvider(
    __in KUriView const&  stateProviderName,
    __in IStateProvider2& stateProvider)
{
    UNREFERENCED_PARAMETER(stateProviderName);

    IStateProvider2::SPtr stateProviderSPtr(&stateProvider);

    //bool add = stateProvidersSPtr_->TryAdd(stateProviderName, stateProviderSPtr);
    //KInvariant(add == true);
    stateProvider_ = stateProviderSPtr;
}

Awaitable<NTSTATUS> MockTransactionalReplicator::RemoveAsync(
    __in TxnReplicator::Transaction & transaction,
    __in KUriView const & stateProviderName,
    __in Common::TimeSpan const & timeout,
    __in CancellationToken const & cancellationToken) noexcept
{
    UNREFERENCED_PARAMETER(transaction);
    UNREFERENCED_PARAMETER(stateProviderName);
    UNREFERENCED_PARAMETER(timeout);
    UNREFERENCED_PARAMETER(cancellationToken);

    co_await suspend_never{};
    co_return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS MockTransactionalReplicator::GetVersion(__out LONG64 & version) const noexcept
{
   version = lastLSN_;
   return STATUS_SUCCESS;
}

void MockTransactionalReplicator::UpdateSecondaryLSN()
{
    if (secondariesReplicatorSPtr_ == nullptr)
    {
        return;
    }

    for (ULONG i = 0; i < secondariesReplicatorSPtr_->Count(); i++)
    {
        (*secondariesReplicatorSPtr_)[i]->UpdateLSN(lastLSN_);
    }
}

void MockTransactionalReplicator::UpdateLSN(__in LONG64 lsn)
{
    KInvariant(role_ != FABRIC_REPLICA_ROLE_PRIMARY);
    lastLSN_ = lsn;
}

ktl::Awaitable<NTSTATUS> MockTransactionalReplicator::BackupAsync(
    __in TxnReplicator::IBackupCallbackHandler & backupCallbackHandler,
    __out TxnReplicator::BackupInfo & result) noexcept
{
    ApiEntry();

    co_return co_await BackupAsync(backupCallbackHandler, FABRIC_BACKUP_OPTION_FULL, Common::TimeSpan::FromSeconds(4), CancellationToken::None, result);
}

ktl::Awaitable<NTSTATUS> MockTransactionalReplicator::BackupAsync(
    __in TxnReplicator::IBackupCallbackHandler & backupCallbackHandler,
    __in FABRIC_BACKUP_OPTION backupOption,
    __in Common::TimeSpan const & timeout,
    __in CancellationToken const & cancellationToken,
    __out TxnReplicator::BackupInfo & result) noexcept
{
    UNREFERENCED_PARAMETER(result);
    UNREFERENCED_PARAMETER(backupCallbackHandler);
    UNREFERENCED_PARAMETER(backupOption);
    UNREFERENCED_PARAMETER(timeout);
    UNREFERENCED_PARAMETER(cancellationToken);

    co_return STATUS_NOT_IMPLEMENTED;
}

ktl::Awaitable<NTSTATUS> MockTransactionalReplicator::RestoreAsync(
    __in KString const & backupFolder) noexcept
{
    ApiEntry();

    co_return co_await RestoreAsync(backupFolder, FABRIC_RESTORE_POLICY_FORCE, Common::TimeSpan::FromSeconds(4), CancellationToken::None);
}

ktl::Awaitable<NTSTATUS> MockTransactionalReplicator::RestoreAsync(
    __in KString const & backupFolder,
    __in FABRIC_RESTORE_POLICY restorePolicy,
    __in Common::TimeSpan const & timeout,
    __in CancellationToken const & cancellationToken) noexcept
{
    UNREFERENCED_PARAMETER(backupFolder);
    UNREFERENCED_PARAMETER(restorePolicy);
    UNREFERENCED_PARAMETER(timeout);
    UNREFERENCED_PARAMETER(cancellationToken);

    co_return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS MockTransactionalReplicator::CreateEnumerator(
    __in bool parentsOnly,
    __out Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, IStateProvider2::SPtr>>::SPtr & outEnumerator) noexcept
{
    UNREFERENCED_PARAMETER(parentsOnly);
    UNREFERENCED_PARAMETER(outEnumerator);
    return STATUS_UNSUCCESSFUL;
}

Awaitable<NTSTATUS> MockTransactionalReplicator::GetOrAddAsync(
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

    co_return STATUS_NOT_IMPLEMENTED;
}
