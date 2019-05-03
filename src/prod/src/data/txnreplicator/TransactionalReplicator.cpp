// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data;
using namespace Data::Utilities;
using namespace TxnReplicator;

Common::StringLiteral const TraceComponent("TransactionalReplicator");

#define ApiEntry() \
    if (!this->TryAcquireServiceActivity()) \
    { \
        return SF_STATUS_OBJECT_CLOSED;\
    } \
    KFinally([&] \
    { \
        this->ReleaseServiceActivity(); \
    }) \

#define ApiEntryAsync() \
    if (!this->TryAcquireServiceActivity()) \
    { \
        co_return SF_STATUS_OBJECT_CLOSED;\
    } \
    KFinally([&] \
    { \
        this->ReleaseServiceActivity(); \
    }) \

#define ApiEntry_THROW() \
    if (!this->TryAcquireServiceActivity()) \
    { \
        throw Exception(SF_STATUS_OBJECT_CLOSED);\
    } \
    KFinally([&] \
    { \
        this->ReleaseServiceActivity(); \
    }) \

TransactionalReplicator::TransactionalReplicator(
    __in PartitionedReplicaId const& traceId,
    __in IRuntimeFolders & runtimeFolders,
    __in IStatefulPartition & partition,
    __in LoggingReplicator::IStateReplicator & stateReplicator,
    __in StateManager::IStateProvider2Factory & stateProviderFactoryFunction,
    __in TRInternalSettingsSPtr && transactionalReplicatorConfig,
    __in SLInternalSettingsSPtr && ktlLoggerSharedLogConfig,
    __in Log::LogManager & logManager,
    __in IDataLossHandler & dataLossHandler,
    __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr && healthClient,
    __in bool hasPersistedState)
    : KAsyncServiceBase()
    , KWeakRefType<TransactionalReplicator>()
    , PartitionedReplicaTraceComponent(traceId)
    , transactionalReplicatorConfig_(std::move(transactionalReplicatorConfig))
    , ktlLoggerSharedLogConfig_(std::move(ktlLoggerSharedLogConfig))
    , runtimeFolders_(&runtimeFolders)
    , partition_(&partition)
    , healthClient_(std::move(healthClient))
    , loggingReplicator_(nullptr)
    , loggingReplicatorRecovered_(false)
    , stateProvider_(nullptr)
    , stateManager_(nullptr)
    , stateManagerOpened_(false)
    , perfCounters_(nullptr)
    , testHookContext_()
{
    NTSTATUS status = STATUS_SUCCESS;

    TREventSource::Events->Ctor(
        TracePartitionId,
        ReplicaId,
        L"TransactionalReplicator",
        reinterpret_cast<uintptr_t>(this));

    perfCounters_ = TRPerformanceCounters::CreateInstance(
        traceId.TracePartitionId.ToString(),
        traceId.ReplicaId);

    status = StateManager::Factory::Create(
        *PartitionedReplicaIdentifier, 
        runtimeFolders,
        partition, 
        stateProviderFactoryFunction, 
        transactionalReplicatorConfig_,
        hasPersistedState,
        GetThisAllocator(), 
        stateManager_);

    THROW_ON_FAILURE(status);

    KString::SPtr workDir;
    status = KString::Create(
        workDir,
        GetThisAllocator(),
        runtimeFolders.get_WorkDirectory());

    THROW_ON_FAILURE(status);

    loggingReplicator_ = LoggingReplicator::Factory::Create(
        *PartitionedReplicaIdentifier,
        runtimeFolders,
        *workDir,
        stateReplicator,
        *stateManager_,
        partition,
        transactionalReplicatorConfig_,
        ktlLoggerSharedLogConfig_,
        logManager,
        dataLossHandler,
        perfCounters_,
        healthClient_,
        *this,
        hasPersistedState,
        GetThisAllocator(),
        stateProvider_);
}

TransactionalReplicator::~TransactionalReplicator()
{
    TREventSource::Events->Dtor(
        TracePartitionId,
        ReplicaId,
        L"TransactionalReplicator",
        reinterpret_cast<uintptr_t>(this));

    loggingReplicator_->Dispose();
}

TransactionalReplicator::SPtr TransactionalReplicator::Create(
    __in PartitionedReplicaId const& traceId, 
    __in IRuntimeFolders & runtimeFolders,
    __in IStatefulPartition & partition,
    __in LoggingReplicator::IStateReplicator & stateReplicator,
    __in StateManager::IStateProvider2Factory & stateProviderFactory,
    __in TRInternalSettingsSPtr && transactionalReplicatorConfig,
    __in SLInternalSettingsSPtr && ktlLoggerSharedLogConfig,
    __in Log::LogManager & logManager,
    __in IDataLossHandler & dataLossHandler,
    __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr && healthClient,
    __in bool hasPersistedState,
    __in KAllocator& allocator)
{
    TransactionalReplicator::SPtr result = _new(TRANSACTIONALREPLICATOR_TAG, allocator) TransactionalReplicator(
        traceId, 
        runtimeFolders, 
        partition, 
        stateReplicator,
        stateProviderFactory,
        move(transactionalReplicatorConfig),
        move(ktlLoggerSharedLogConfig),
        logManager,
        dataLossHandler,
        move(healthClient),
        hasPersistedState);

    THROW_ON_ALLOCATION_FAILURE(result);

    return result;
}

NTSTATUS TransactionalReplicator::RegisterTransactionChangeHandler(
    __in ITransactionChangeHandler& transactionChangeHandler) noexcept
{
    return loggingReplicator_->RegisterTransactionChangeHandler(transactionChangeHandler);
}

NTSTATUS TransactionalReplicator::UnRegisterTransactionChangeHandler() noexcept
{
    return loggingReplicator_->UnRegisterTransactionChangeHandler();
}

NTSTATUS TransactionalReplicator::RegisterStateManagerChangeHandler(
    __in IStateManagerChangeHandler& stateManagerChangeHandler) noexcept
{
    return stateManager_->RegisterStateManagerChangeHandler(stateManagerChangeHandler);
}

NTSTATUS TransactionalReplicator::UnRegisterStateManagerChangeHandler() noexcept
{
    return stateManager_->UnRegisterStateManagerChangeHandler();
}

Awaitable<NTSTATUS> TransactionalReplicator::BackupAsync(
    __in IBackupCallbackHandler & backupCallbackHandler,
    __out BackupInfo & result) noexcept
{
    ApiEntryAsync();

    NTSTATUS status = co_await loggingReplicator_->BackupAsync(backupCallbackHandler, result);

    co_return status;
}

Awaitable<NTSTATUS> TransactionalReplicator::BackupAsync(
    __in IBackupCallbackHandler & backupCallbackHandler,
    __in FABRIC_BACKUP_OPTION backupOption,
    __in Common::TimeSpan const & timeout,
    __in CancellationToken const & cancellationToken,
    __out BackupInfo & result) noexcept
{
    ApiEntryAsync();

    NTSTATUS status = co_await loggingReplicator_->BackupAsync(
        backupCallbackHandler,
        backupOption,
        timeout,
        cancellationToken,
        result);

    co_return status;
}

Awaitable<NTSTATUS> TransactionalReplicator::RestoreAsync(
    __in KString const & backupFolder) noexcept
{
    ApiEntryAsync();

    NTSTATUS status = co_await loggingReplicator_->RestoreAsync(backupFolder);

    co_return status;
}

Awaitable<NTSTATUS> TransactionalReplicator::RestoreAsync(
    __in KString const & backupFolder,
    __in FABRIC_RESTORE_POLICY restorePolicy,
    __in Common::TimeSpan const & timeout,
    __in CancellationToken const & cancellationToken) noexcept
{
    ApiEntryAsync();

    NTSTATUS status = co_await loggingReplicator_->RestoreAsync(backupFolder, restorePolicy, timeout, cancellationToken);

    co_return status;
}

void TransactionalReplicator::Test_SetTestHookContext(TestHooks::TestHookContext const & testHookContext)
{
    testHookContext_ = testHookContext;

    if (stateProvider_)
    {
        stateProvider_->Test_SetTestHookContext(testHookContext);
    }
}

TestHooks::TestHookContext const & TransactionalReplicator::Test_GetTestHookContext()
{
    return testHookContext_;
}

Awaitable<NTSTATUS> TransactionalReplicator::OpenAsync()
{
    kservice::OpenAwaiter::SPtr openAwaiter;
    kservice::OpenAwaiter::Create(GetThisAllocator(), GetThisAllocationTag(), *this, openAwaiter);

    // Finish opening the transactional replicator and then do recovery because state providers might call into TR during recovery
    // Without the open completed, any API Entry fails. An example of an API during recovery is TryRemoveVersion
    NTSTATUS status = co_await *openAwaiter;

    CO_RETURN_ON_FAILURE(status);

    RecoveryInformation recoveryInformation;
    status = co_await loggingReplicator_->OpenAsync(recoveryInformation);

    if (!NT_SUCCESS(status))
    {
        TREventSource::Events->Exception(
            TracePartitionId,
            ReplicaId,
            L"LoggingReplicator OpenAsync",
            status,
            L"");

        co_await CloseAsync();
        co_return status;
    }

    ASSERT_IFNOT(
        recoveryInformation.ShouldRemoveLocalStateDueToIncompleteRestore == false ||
        recoveryInformation.LogCompleteCheckpointAfterRecovery == false,
        "{0}:  TransactionalReplicator::OpenAsync: recoveryInformation.ShouldRemoveLocalStateDueToIncompleteRestore == false || recoveryInformation.LogCompleteCheckpointAfterRecovery == false",
        TraceId);

    status = co_await stateManager_->OpenAsync(
        recoveryInformation.LogCompleteCheckpointAfterRecovery,
        recoveryInformation.ShouldRemoveLocalStateDueToIncompleteRestore,
        CancellationToken::None);

    if (!NT_SUCCESS(status))
    {
        TREventSource::Events->Exception(
            TracePartitionId,
            ReplicaId,
            L"StateManager OpenAsync",
            status,
            L"");

        co_await CloseAsync();
        co_return status;
    }
    else 
    {
        stateManagerOpened_ = true;
    }
        
    if (!recoveryInformation.ShouldSkipRecoveryDueToIncompleteChangeRoleNone)
    {
        status = co_await loggingReplicator_->PerformRecoveryAsync(recoveryInformation);

        if (NT_SUCCESS(status))
        {
            loggingReplicatorRecovered_ = true;
        }
    }
    else
    {
        TREventSource::Events->SkippingRecovery(
            TracePartitionId,
            ReplicaId);
    }

    if (!NT_SUCCESS(status))
    {
        co_await CloseAsync();
    }
    
    co_return status;
}

Awaitable<NTSTATUS> TransactionalReplicator::CloseAsync()
{
    // This cosharedapi entry is needed because abort() spawns this and does not block the thread to wait for completion
    // This can cause nullref in the replicator as no one will be keep it alive
    KCoShared$ApiEntry()

    TREventSource::Events->CloseAsync(
        TracePartitionId,
        ReplicaId);
    //
    // Close the logging replicator before invoking Close on transactional replicator to ensure that any pending activity inside the replicator
    // completes. Without this, the activity count in the TR may never reach 0 because a pending commit may not have been processed due to failure to issue a barrier.
    //

    // Close the replicator only if it successfully recovered. Otherwise, just dispose the replicator
    NTSTATUS status = STATUS_SUCCESS;

    if (loggingReplicatorRecovered_)
    {
        status = co_await loggingReplicator_->CloseAsync();
    }

    if (!NT_SUCCESS(status))
    {
        TREventSource::Events->Exception(
            TracePartitionId,
            ReplicaId,
            L"LoggingReplicator CloseAsync",
            status,
            L"");
    }

    loggingReplicator_->Dispose();

    kservice::CloseAwaiter::SPtr closeAwaiter;
    kservice::CloseAwaiter::Create(GetThisAllocator(), GetThisAllocationTag(), *this, closeAwaiter);

    status = co_await *closeAwaiter;

    co_return status;
}

void TransactionalReplicator::Abort()
{ 
    TREventSource::Events->Abort(
        TracePartitionId,
        ReplicaId);

    Awaitable<NTSTATUS> task = CloseAsync();
    ToTask(task);
}

Awaitable<NTSTATUS> TransactionalReplicator::ChangeRoleAsync(__in FABRIC_REPLICA_ROLE newRole)
{
    TREventSource::Events->ChangeRole(
        TracePartitionId,
        ReplicaId,
        loggingReplicator_->Role,
        newRole);

    ApiEntryAsync();

    NTSTATUS status = STATUS_SUCCESS;

    try
    {
        // Step 2 and 3: Invoke the change role on the logging replicator and state manager
        switch (newRole)
        {
        case FABRIC_REPLICA_ROLE_PRIMARY:
            status = co_await loggingReplicator_->BecomePrimaryAsync(false);

            if (!NT_SUCCESS(status))
            {
                break;
            }

            co_await stateManager_->ChangeRoleAsync(newRole, CancellationToken::None);
            break;
        case FABRIC_REPLICA_ROLE_IDLE_SECONDARY:
            status = co_await loggingReplicator_->BecomeIdleSecondaryAsync();

            if (!NT_SUCCESS(status))
            {
                break;
            }

            co_await stateManager_->ChangeRoleAsync(newRole, CancellationToken::None);
            break;
        case FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY:
        {
            ILoggingReplicator::StateManagerBecomeSecondaryDelegate stateManagerBecomeSecondaryDelegate;
            stateManagerBecomeSecondaryDelegate.Bind(this, &TransactionalReplicator::InvokeStateManagerChangeRoleDelegate);
            status = co_await loggingReplicator_->BecomeActiveSecondaryAsync(stateManagerBecomeSecondaryDelegate);
            break;
        }
        case FABRIC_REPLICA_ROLE_NONE:
            status = co_await loggingReplicator_->BecomeNoneAsync();

            if (!NT_SUCCESS(status))
            {
                break;
            }

            co_await stateManager_->ChangeRoleAsync(newRole, CancellationToken::None);

            status = co_await loggingReplicator_->DeleteLogAsync();

            if (!NT_SUCCESS(status))
            {
                break;
            }

            break;
        default:
            ASSERT_IFNOT(
                false,
                "{0}: TransactionalReplicator ChangeRole to an unexpected role {1}",
                TraceId,
                newRole);
        }
    }
    catch (Exception const & e)
    {
        status = e.GetStatus();
    }

    if (!NT_SUCCESS(status))
    {
        TREventSource::Events->Exception(
            TracePartitionId,
            ReplicaId,
            L"ChangeRoleAsync",
            status,
            L"Reporting fault transient.");

        partition_->ReportFault(FABRIC_FAULT_TYPE_TRANSIENT);
    }

    co_return status;
}

Awaitable<NTSTATUS> TransactionalReplicator::InvokeStateManagerChangeRoleDelegate(FABRIC_REPLICA_ROLE newRole, CancellationToken cancellationToken) noexcept
{
    NTSTATUS status = STATUS_SUCCESS;

    try
    {
        co_await stateManager_->ChangeRoleAsync(newRole, cancellationToken);
    }
    catch (Exception const & e)
    {
        status = e.GetStatus();
    }

    co_return status;
}

void TransactionalReplicator::OnServiceOpen()
{
    OnServiceOpenAsync();
}

void TransactionalReplicator::OnServiceClose()
{
    OnServiceCloseAsync();
}

Task TransactionalReplicator::OnServiceOpenAsync() noexcept
{
    TREventSource::Events->OnServiceOpenAsync(
        TracePartitionId,
        ReplicaId);

    SetDeferredCloseBehavior();

    stateManager_->Initialize(
        *this,
        *loggingReplicator_);

    BOOL isCompletionAccepted = CompleteOpen(STATUS_SUCCESS);
    ASSERT_IFNOT(isCompletionAccepted == TRUE, "Could not open transactional replicator");
    
    co_return;
}

Task TransactionalReplicator::OnServiceCloseAsync() noexcept
{ 
    TREventSource::Events->OnServiceCloseAsync(
        TracePartitionId,
        ReplicaId);

    NTSTATUS status = STATUS_SUCCESS;
    
    if (stateManagerOpened_)
    {
        status = co_await stateManager_->CloseAsync(CancellationToken::None);
    }

    if (NT_SUCCESS(status) == false)
    {
        TREventSource::Events->Exception(
            TracePartitionId,
            ReplicaId,
            L"StateManager CloseAsync",
            status,
            L"Closing KAsyncServiceBase");
    }

    BOOL isCompletionAccepted = CompleteClose(status);

    ASSERT_IFNOT(isCompletionAccepted == TRUE, "Could not close transanctional replicator");

    co_return;
}

Awaitable<NTSTATUS> TransactionalReplicator::TryRemoveCheckpointAsync(
    __in LONG64 checkpointLsnToBeRemoved,
    __in LONG64 nextCheckpointLsn) noexcept
{
    ApiEntryAsync();
    
    NTSTATUS status = co_await loggingReplicator_->TryRemoveCheckpointAsync(checkpointLsnToBeRemoved, nextCheckpointLsn);

    co_return status;
}

NTSTATUS TransactionalReplicator::TryRemoveVersion(
    __in LONG64 stateProviderId,
    __in LONG64 commitLsn,
    __in LONG64 nextCommitLsn,
    __out TryRemoveVersionResult::SPtr & result) noexcept
{
    ApiEntry();
    return loggingReplicator_->TryRemoveVersion(stateProviderId, commitLsn, nextCommitLsn, result);
}

Awaitable<NTSTATUS> TransactionalReplicator::RegisterAsync(__out LONG64 & lsn) noexcept
{
    ApiEntryAsync();
    
    NTSTATUS status = co_await loggingReplicator_->RegisterAsync(lsn);

    co_return status;
}

NTSTATUS TransactionalReplicator::UnRegister(__in LONG64 visibilityVersionNumber) noexcept
{
    ApiEntry();
    return loggingReplicator_->UnRegister(visibilityVersionNumber);
}

FABRIC_REPLICA_ROLE TransactionalReplicator::get_Role() const
{
    return loggingReplicator_->Role;
}

NTSTATUS TransactionalReplicator::CreateTransaction(Transaction::SPtr & result) noexcept
{
    ApiEntry();

    result = Transaction::CreateTransaction(
        *this,
        GetThisAllocator());

    return STATUS_SUCCESS;
}

NTSTATUS TransactionalReplicator::CreateAtomicOperation(AtomicOperation::SPtr & result) noexcept
{
    ApiEntry();

    result = AtomicOperation::CreateAtomicOperation(
        *this,
        GetThisAllocator());

    return STATUS_SUCCESS;
}

NTSTATUS TransactionalReplicator::CreateAtomicRedoOperation(AtomicRedoOperation::SPtr & result) noexcept
{
    ApiEntry();

    result = AtomicRedoOperation::CreateAtomicRedoOperation(
        *this,
        GetThisAllocator());

    return STATUS_SUCCESS;
}

NTSTATUS TransactionalReplicator::SingleOperationTransactionAbortUnlock(
    __in LONG64 stateProviderId,
    __in OperationContext const & operationContext) noexcept
{
    ApiEntry();

    stateManager_->SingleOperationTransactionAbortUnlock(stateProviderId, operationContext);

    return STATUS_SUCCESS;
}

NTSTATUS TransactionalReplicator::ErrorIfStateProviderIsNotRegistered(
    __in LONG64 stateProviderId,
    __in LONG64 transactionId) noexcept
{
    ApiEntry();

    return stateManager_->ErrorIfStateProviderIsNotRegistered(stateProviderId, transactionId);
}

NTSTATUS TransactionalReplicator::BeginTransaction(
    __in Transaction & transaction,
    __in_opt Utilities::OperationData const * const metaData,
    __in_opt Utilities::OperationData const * const undo,
    __in_opt Utilities::OperationData const * const redo,
    __in LONG64 stateProviderId,
    __in_opt OperationContext const * const operationContext) noexcept
{
    ApiEntry();

    return stateManager_->BeginTransaction(
        transaction,
        stateProviderId,
        metaData,
        undo,
        redo,
        operationContext);
}

Awaitable<NTSTATUS> TransactionalReplicator::BeginTransactionAsync(
    __in Transaction & transaction,
    __in_opt Utilities::OperationData const * const metaData,
    __in_opt Utilities::OperationData const * const undo,
    __in_opt Utilities::OperationData const * const redo,
    __in LONG64 stateProviderId,
    __in_opt OperationContext const * const operationContext,
    __out LONG64 & commitLsn) noexcept
{
    ApiEntryAsync();

    NTSTATUS status = co_await stateManager_->BeginTransactionAsync(
        transaction,
        stateProviderId,
        metaData,
        undo,
        redo,
        operationContext,
        commitLsn);

    co_return status;
}

NTSTATUS TransactionalReplicator::AddOperation(
    __in Transaction & transaction,
    __in_opt Utilities::OperationData const * const metaData,
    __in_opt Utilities::OperationData const * const undo,
    __in_opt Utilities::OperationData const * const redo,
    __in LONG64 stateProviderId,
    __in_opt OperationContext const * const operationContext) noexcept
{
    ApiEntry();

    return stateManager_->AddOperation(
        transaction,
        stateProviderId,
        metaData,
        undo,
        redo,
        operationContext);
}

Awaitable<NTSTATUS> TransactionalReplicator::AddOperationAsync(
    __in AtomicOperation & atomicOperation,
    __in_opt Utilities::OperationData const * const metaData,
    __in_opt Utilities::OperationData const * const undo,
    __in_opt Utilities::OperationData const * const redo,
    __in LONG64 stateProviderId,
    __in_opt OperationContext const * const operationContext,
    __out LONG64 & commitLsn) noexcept
{
    ApiEntryAsync();

    NTSTATUS status = co_await stateManager_->AddOperationAsync(
        atomicOperation,
        stateProviderId,
        metaData,
        undo,
        redo,
        operationContext,
        commitLsn);

    co_return status;
}

Awaitable<NTSTATUS> TransactionalReplicator::AddOperationAsync(
    __in AtomicRedoOperation & atomicRedoOperation,
    __in_opt Utilities::OperationData const * const metaData,
    __in_opt Utilities::OperationData const * const redo,
    __in LONG64 stateProviderId,
    __in_opt OperationContext const * const operationContext,
    __out LONG64 & commitLsn) noexcept
{
    ApiEntryAsync();

    NTSTATUS status = co_await stateManager_->AddOperationAsync(
        atomicRedoOperation,
        stateProviderId,
        metaData,
        redo,
        operationContext,
        commitLsn);

    co_return status;
}

Awaitable<NTSTATUS> TransactionalReplicator::CommitTransactionAsync(
    __in Transaction & transaction,
    __out LONG64 & commitLsn) noexcept
{
    ApiEntryAsync();

    NTSTATUS status = co_await loggingReplicator_->CommitTransactionAsync(transaction, commitLsn);

    co_return status;
}

Awaitable<NTSTATUS> TransactionalReplicator::AbortTransactionAsync(
    __in Transaction & transaction,
    __out LONG64 & abortLsn) noexcept
{
    ApiEntryAsync();

    NTSTATUS status = co_await loggingReplicator_->AbortTransactionAsync(transaction, abortLsn);

    co_return status;
}

NTSTATUS TransactionalReplicator::Get(
    __in KUriView const & stateProviderName,
    __out IStateProvider2::SPtr & stateProvider) noexcept
{
    ApiEntry();

    return stateManager_->Get(
        stateProviderName,
        stateProvider);
}

Awaitable<NTSTATUS> TransactionalReplicator::AddAsync(
    __in Transaction & transaction,
    __in KUriView const & stateProviderName,
    __in KStringView const & stateProvider,
    __in_opt OperationData const * const initializationParameters,
    __in Common::TimeSpan const & timeout,
    __in CancellationToken const & cancellationToken) noexcept
{
    ApiEntryAsync();

    NTSTATUS status = co_await stateManager_->AddAsync(
        transaction,
        stateProviderName,
        stateProvider,
        initializationParameters,
        timeout,
        cancellationToken);

    co_return status;
}

Awaitable<NTSTATUS> TransactionalReplicator::RemoveAsync(
    __in Transaction & transaction,
    __in KUriView const & stateProviderName,
    __in Common::TimeSpan const & timeout,
    __in CancellationToken const & cancellationToken) noexcept
{
    ApiEntryAsync();

    NTSTATUS status = co_await stateManager_->RemoveAsync(
        transaction,
        stateProviderName,
        timeout,
        cancellationToken);

    co_return status;
}

NTSTATUS TransactionalReplicator::CreateEnumerator(
    __in bool parentsOnly,
    __out IEnumerator<KeyValuePair<KUri::CSPtr, IStateProvider2::SPtr>>::SPtr & outEnumerator) noexcept
{
    ApiEntry();

    return stateManager_->CreateEnumerator(parentsOnly, outEnumerator);
}

Awaitable<NTSTATUS> TransactionalReplicator::GetOrAddAsync(
    __in Transaction & transaction,
    __in KUriView const& stateProviderName,
    __in KStringView const & stateProviderTypeName,
    __out IStateProvider2::SPtr & stateProvider,
    __out bool & alreadyExist,
    __in_opt OperationData const * const initializationParameters,
    __in Common::TimeSpan const & timeout,
    __in CancellationToken const & cancellationToken) noexcept
{
    ApiEntryAsync();

    CancellationToken tmpCancellationToken(cancellationToken);

    NTSTATUS status = co_await stateManager_->GetOrAddAsync(
        transaction,
        stateProviderName,
        stateProviderTypeName,
        stateProvider,
        alreadyExist,
        initializationParameters,
        timeout,
        tmpCancellationToken);

    co_return status;
}

bool TransactionalReplicator::get_IsReadable() const noexcept
{
    return loggingReplicator_->IsReadable();
}

bool TransactionalReplicator::get_HasPersistedState() const noexcept
{
    return loggingReplicator_->HasPersistedState;
}


IStatefulPartition::SPtr TransactionalReplicator::get_StatefulPartition() const
{
    return partition_;
}

NTSTATUS TransactionalReplicator::GetLastStableSequenceNumber(__out LONG64 & lsn) noexcept
{
    ApiEntry();

    return loggingReplicator_->GetLastStableSequenceNumber(lsn);
}

NTSTATUS TransactionalReplicator::GetLastCommittedSequenceNumber(__out LONG64 & lsn) noexcept
{
    ApiEntry();

    return loggingReplicator_->GetLastCommittedSequenceNumber(lsn);
}

NTSTATUS TransactionalReplicator::GetCurrentEpoch(__out FABRIC_EPOCH & result) noexcept
{
    ApiEntry();

    return loggingReplicator_->GetCurrentEpoch(result);
}

NTSTATUS TransactionalReplicator::Test_RequestCheckpointAfterNextTransaction() noexcept
{
    ApiEntry();

    return loggingReplicator_->Test_RequestCheckpointAfterNextTransaction();
}

NTSTATUS TransactionalReplicator::Test_GetPeriodicCheckpointAndTruncationTimestampTicks(
    __out LONG64 & lastPeriodicCheckpointTimeTicks,
    __out LONG64 & lastPeriodicTruncationTimeTicks) noexcept
{
    return loggingReplicator_->Test_GetPeriodicCheckpointAndTruncationTimestampTicks(
        lastPeriodicCheckpointTimeTicks,
        lastPeriodicTruncationTimeTicks);
}