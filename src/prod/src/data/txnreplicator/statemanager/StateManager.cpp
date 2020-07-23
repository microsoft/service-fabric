// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "data/txnreplicator/stdafx.h"

using namespace ktl;
using namespace Data::Utilities;
using namespace TxnReplicator;
using namespace Data::StateManager;

const FABRIC_STATE_PROVIDER_ID StateManager::StateManagerId(LONG64_MIN);
const KUriView StateManager::StateManagerName(L"fabric:/StateManager");
const KStringView StateManager::InsertOperation = L"Insert";
const KStringView StateManager::DeleteOperation = L"Delete";

NTSTATUS StateManager::Create(
    __in PartitionedReplicaId const & traceId,
    __in IRuntimeFolders & runtimeFolders,
    __in IStatefulPartition & partition,
    __in IStateProvider2Factory & stateProviderFactory,
    __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
    __in bool hasPersistedState,
    __in KAllocator& allocator,
    __out SPtr & result) noexcept
{
    result = _new(STATE_MANAGER_TAG, allocator) StateManager(
        traceId, 
        runtimeFolders, 
        partition, 
        stateProviderFactory,
        transactionalReplicatorConfig,
        hasPersistedState);
    if (result == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(result->Status()))
    {
        // null Result while fetching failure status with no extra AddRefs or Releases; should opt very well
        return (SPtr(Ktl::Move(result)))->Status();
    }

    return STATUS_SUCCESS;
}

void StateManager::Initialize(
    __in ITransactionalReplicator & transactionalReplicator,
    __in ILoggingReplicator & loggingReplicator)
{
    NTSTATUS status = transactionalReplicator.GetWeakIfRef(transactionalReplicatorWRef_);
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"Initialize: transactionalReplicator GetWeakIfRef.",
        Helper::StateManager);

    status = loggingReplicator.GetWeakIfRef(loggingReplicatorWRef_);
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"Initialize: loggingReplicator GetWeakIfRef.",
        Helper::StateManager);
}

Awaitable<NTSTATUS> StateManager::OpenAsync(
    __in bool completeCheckpoint,
    __in bool cleanupRestore, 
    __in CancellationToken & cancellationToken) noexcept
{
    ASSERT_IFNOT(openParametersSPtr_ == nullptr, "{0}: Non-empty open parameters", TraceId);

    StateManagerEventSource::Events->OpenAsync(
        TracePartitionId,
        ReplicaId,
        completeCheckpoint,
        cleanupRestore);

    // Save open parameters in a member variable.
    NTSTATUS status = OpenParameters::Create(completeCheckpoint, cleanupRestore, GetThisAllocator(), openParametersSPtr_);
    if (NT_SUCCESS(status) == false)
    {
        co_return status;
    }

    kservice::OpenAwaiter::SPtr openAwaiter;
    status = kservice::OpenAwaiter::Create(GetThisAllocator(), GetThisAllocationTag(), *this, openAwaiter);
    if (NT_SUCCESS(status) == false)
    {
        co_return status;
    }

    status = co_await *openAwaiter;

    co_return status;
}

Awaitable<void> StateManager::ChangeRoleAsync(
    __in FABRIC_REPLICA_ROLE newRole,
    __in CancellationToken & cancellationToken)
{
    ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    FABRIC_REPLICA_ROLE role = GetCurrentRole();

    StateManagerEventSource::Events->ChangeRoleAsync(
        TracePartitionId,
        ReplicaId,
        static_cast<int>(role),
        static_cast<int>(newRole));

    // Check if this is a duplicate change role call (a retry).
    // If it is a duplicate call that was previously successful, noop.
    if (role == newRole)
    {
        ASSERT_IFNOT(
            changeRoleCompleted_ == true,
            "{0}: Change role should already have been completed on retry",
            TraceId);
        co_return;
    }

    // Previous call failed (likely threw).
    if (changeRoleCompleted_ == false)
    {
        // TODO: Once we support custom state providers, add retry for failed state providers.
        CODING_ASSERT("{0}: Previous change role was not completed.", TraceId);
    }

    changeRoleCompleted_ = false;

    status = co_await this->ChangeRoleOnStateProvidersAsync(newRole, cancellationToken);
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"ChangeRoleAsync: ChangeRoleOnStateProvidersAsync.",
        Helper::StateManager);

    if (newRole == FABRIC_REPLICA_ROLE_NONE)
    {
        status = co_await this->RemoveStateOnStateProvidersAsync(cancellationToken);
        Helper::ThrowIfNecessary(
            status,
            TracePartitionId,
            ReplicaId,
            L"ChangeRoleAsync: RemoveStateOnStateProvidersAsync",
            Helper::StateManager);
        
        checkpointManagerSPtr_->Clean();
        status = Helper::DeleteFolder(*workDirectoryPath_);
        Helper::ThrowIfNecessary(
            status,
            TracePartitionId,
            ReplicaId,
            L"ChangeRoleAsync: DeleteFolder workDirectoryPath.",
            Helper::StateManager);
    }

    {
        // Port Note: This lock does not exist on managed.
        // Since ChangeRoleAsync does not run concurrently with itself or in-flight ApplyAsync calls,
        // This lock is just used for defensive coding purposes.
        roleLock_.AcquireExclusive();
        KFinally([&] {roleLock_.ReleaseExclusive(); });

        changeRoleCompleted_ = true;
        role_ = newRole;
    }

    co_return;
}

Awaitable<NTSTATUS> StateManager::CloseAsync(__in CancellationToken & cancellationToken) noexcept
{
    StateManagerEventSource::Events->CloseAsync(
        TracePartitionId,
        ReplicaId);

    kservice::CloseAwaiter::SPtr closeAwaiter;
    NTSTATUS status = kservice::CloseAwaiter::Create(GetThisAllocator(), GetThisAllocationTag(), *this, closeAwaiter);
    ASSERT_IFNOT(
        NT_SUCCESS(status),
        "{0}: CloseAsync: CloseAwaiter creation failed. Status: {1}",
        TraceId,
        status);

    status = co_await *closeAwaiter;
    ASSERT_IFNOT(
        NT_SUCCESS(status),
        "{0}: CloseAsync: SM.CloseAsync returned unsuccessful status. Status: {1}",
        TraceId,
        status);

    co_return status;
}

NTSTATUS StateManager::RegisterStateManagerChangeHandler(
    __in IStateManagerChangeHandler& stateManagerChangeHandler) noexcept
{
    changeHandlerCache_.Put(&stateManagerChangeHandler);
    return STATUS_SUCCESS;
}

NTSTATUS StateManager::UnRegisterStateManagerChangeHandler() noexcept
{
    changeHandlerCache_.Put(nullptr);
    return STATUS_SUCCESS;
}

NTSTATUS StateManager::Get(
    __in KUriView const & stateProviderName, 
    __out IStateProvider2::SPtr & stateProvider) noexcept
{
    // Return NTSTATUS error code SF_STATUS_OBJECT_CLOSED if closed. If not, does not allow the state manager to close before this function is completed.
    ApiEntryReturn();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    // CheckIfNameIsValid will trace if it fails
    status = CheckIfNameIsValid(stateProviderName, false);
    RETURN_ON_FAILURE(status);

    // CheckIfReadable will trace if it fails
    status = CheckIfReadable(stateProviderName);
    RETURN_ON_FAILURE(status);

    // Copy will trace if it fails
    KUri::CSPtr copyName = nullptr;
    status = Copy(stateProviderName, copyName);
    RETURN_ON_FAILURE(status);

    // TryGetMetadata may throw exception. Catches it and returns NTSTATUS error code.
    try
    {
        Metadata::SPtr metadataSPtr = nullptr;
        status = metadataManagerSPtr_->TryGetMetadata(*copyName, false, metadataSPtr) ? STATUS_SUCCESS : SF_STATUS_NAME_DOES_NOT_EXIST;
        stateProvider = NT_SUCCESS(status) ? metadataSPtr->StateProvider : nullptr;
    }
    catch (Exception & exception)
    {
        StateManagerEventSource::Events->Error(
            TracePartitionId,
            ReplicaId,
            L"Get: MetadataManager::TryGetMetadata throws exception",
            exception.GetStatus());
        return exception.GetStatus();
    }

    return status;
}

NTSTATUS StateManager::CreateEnumerator(
    __in bool parentsOnly,
    __out Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, IStateProvider2::SPtr>>::SPtr & outEnumerator) noexcept
{
    // Return NTSTATUS error code SF_STATUS_OBJECT_CLOSED if closed. If not, does not allow the state manager to close before this function is completed.
    ApiEntryReturn();

    // Check read status before return the enumerator, the function CheckIfReadable will trace if it fails.
    NTSTATUS status = CheckIfReadable(StateManagerName);
    RETURN_ON_FAILURE(status);

    return CreateInternalEnumerator(parentsOnly, outEnumerator);
}

// Algorithms:
//   1.Validation
//   2.Acquire Read Lock
//   3.Check if StateProvider exists
//   4.If it exists, AddLockContext, return StateProvider, STATUS_SUCCESS and set alreadyExist to true
//   5.Release Read Lock, Acquire Write lock and AddLockContext
//   6.Check if SP exists
//   7.If it exists, return SP(under write lock), STATUS_SUCCESS and set alreadyExist to true
//   8.Lock, Transient Add and Replicate all children
//   9.Transient Add and Replicate StateProvider, return StateProvider, STATUS_SUCCESS and set alreadyExist to false
Awaitable<NTSTATUS> StateManager::GetOrAddAsync(
    __in Transaction & transaction,
    __in KUriView const & stateProviderName,
    __in KStringView const & stateProviderTypeName,
    __out IStateProvider2::SPtr & stateProvider,
    __out bool & alreadyExist,
    __in_opt OperationData const * const initializationParameters,
    __in Common::TimeSpan const & timeout,
    __in CancellationToken const & cancellationToken) noexcept
{
    // Return NTSTATUS error code SF_STATUS_OBJECT_CLOSED if closed. If not, does not allow the state manager to close before this function is completed.
    ApiEntryAsync();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    // CheckIfNameIsValid will trace if it fails
    status = CheckIfNameIsValid(stateProviderName, false);
    CO_RETURN_ON_FAILURE(status);

    // CheckIfWritable will trace if it fails
    status = CheckIfWritable("GetOrAddAsync", transaction, stateProviderName);
    CO_RETURN_ON_FAILURE(status);

    // In Managed serialization mode, the initializationParameters buffer count should only have 0 or 1 buffer and 0 indicates the byte array is empty.
    if (serializationMode_ == SerializationMode::Managed && initializationParameters != nullptr && initializationParameters->BufferCount > 1)
    {
        StateManagerEventSource::Events->ISPM_ApiError(
            TracePartitionId,
            ReplicaId,
            FABRIC_INVALID_STATE_PROVIDER_ID,
            "GetOrAddAsync: InitializationParameters is invalid.",
            ToStringLiteral(stateProviderName),
            transaction.TransactionId,
            STATUS_INVALID_PARAMETER);

        throw Exception(STATUS_INVALID_PARAMETER);
    }

    KUri::CSPtr copyStateProviderName = nullptr;
    status = Copy(stateProviderName, copyStateProviderName);
    CO_RETURN_ON_FAILURE(status);

    KString::CSPtr copyTypeName = nullptr;
    status = Copy(stateProviderTypeName, copyTypeName);
    CO_RETURN_ON_FAILURE(status);

    // Generate Id for the parent.
    FABRIC_STATE_PROVIDER_ID parentStateProviderId = CreateStateProviderId();

    // Acquire Read lock
    TransactionBase const & transactionBase = static_cast<TransactionBase const &>(transaction);
    StateManagerLockContext::SPtr smLockContextSPtr = nullptr;
    status = co_await metadataManagerSPtr_->LockForReadAsync(
        *copyStateProviderName,
        parentStateProviderId,
        transactionBase,
        timeout,
        cancellationToken,
        smLockContextSPtr);
    CO_RETURN_ON_FAILURE(status);

    StateManagerTransactionContext::SPtr parentLockContextSPtr;

    // Even though we checked write status here, it's not safe because Read/write status check is a polling mechanism.
    status = Get(*copyStateProviderName, stateProvider);

    // If Get returns NTSTATUS error code, the return value is not STATUS_SUCCESS or SF_STATUS_NAME_DOES_NOT_EXIST.
    if (NT_SUCCESS(status) == false && status != SF_STATUS_NAME_DOES_NOT_EXIST)
    {
        StateManagerEventSource::Events->ISPM_ApiError(
            TracePartitionId,
            ReplicaId,
            FABRIC_INVALID_STATE_PROVIDER_ID,
            "GetOrAddAsync: Could not read.",
            ToStringLiteral(stateProviderName),
            transaction.TransactionId,
            status);
        NTSTATUS releaseLockStatus = smLockContextSPtr->ReleaseLock(transaction.TransactionId);
        if (NT_SUCCESS(releaseLockStatus) == false)
        {
            TraceError(
                L"GetOrAddAsync: StateManagerLockContext::ReleaseLock",
                releaseLockStatus);
        }

        CO_RETURN_ON_FAILURE(status);
    }

    // If it exists, add the read lock context, stateProvider must be not null
    // Note: Preethas, use remove lock context instead of releasing the lock on catch as it is consistent with the rest of the code
    // since remove lock context on LR's ILockcontext has never been used/tested, do it in the next release, this is the safer option for now.
    if (NT_SUCCESS(status))
    {
        status = StateManagerTransactionContext::Create(
            transaction.TransactionId,
            *smLockContextSPtr,
            OperationType::Read,
            GetThisAllocator(),
            parentLockContextSPtr);
        if (NT_SUCCESS(status) == false)
        {
            // If Create failed, release the read lock first then return NTSTATUS error code, otherwise lock leak
            // #9019333, Add tests for the case, TransactionContext Create failed after getting the read/write lock
            StateManagerEventSource::Events->ISPM_ApiError(
                TracePartitionId,
                ReplicaId,
                parentStateProviderId,
                "GetOrAddAsync: Create StateManagerTransactionContext failed.",
                ToStringLiteral(stateProviderName),
                transaction.TransactionId,
                status);
            NTSTATUS releaseLockStatus = smLockContextSPtr->ReleaseLock(transaction.TransactionId);
            if (NT_SUCCESS(releaseLockStatus) == false)
            {
                TraceError(
                    L"GetOrAddAsync: StateManagerLockContext::ReleaseLock",
                    releaseLockStatus);
            }

            co_return status;
        }

        LockContext & lockContext = static_cast<LockContext &>(*parentLockContextSPtr);

        status = transaction.AddLockContext(lockContext);
        if (NT_SUCCESS(status) == false)
        {
            StateManagerEventSource::Events->ISPM_ApiError(
                TracePartitionId,
                ReplicaId,
                FABRIC_INVALID_STATE_PROVIDER_ID,
                "GetOrAddAsync: AddLockContext",
                ToStringLiteral(stateProviderName),
                transaction.TransactionId,
                status);
            NTSTATUS releaseLockStatus = smLockContextSPtr->ReleaseLock(transaction.TransactionId);
            if (NT_SUCCESS(releaseLockStatus) == false)
            {
                TraceError(
                    L"GetOrAddAsync: StateManagerLockContext::ReleaseLock",
                    releaseLockStatus);
            }

            co_return status;
        }

        ASSERT_IFNOT(
            stateProvider != nullptr,
            "{0}: GetOrAddAsync: stateProvider should not be nullptr.",
            TraceId);

        // If state provider exists, Get API will return STATUS_SUCCESS. GetOrAddAsync API should
        // return STATUS_SUCCESS and set out parameter alreadyExist as true.
        alreadyExist = true;
        co_return STATUS_SUCCESS;
    }

    // Release the read lock
    status = smLockContextSPtr->ReleaseLock(transaction.TransactionId);
    if (NT_SUCCESS(status) == false)
    {
        TraceError(
            L"GetOrAddAsync: StateManagerLockContext::ReleaseLock",
            status);
        CO_RETURN_ON_FAILURE(status);
    }

    // Acquire write lock
    // The read lock will be released in the LockForWriteAsync function
    status = co_await metadataManagerSPtr_->LockForWriteAsync(
        *copyStateProviderName,
        parentStateProviderId,
        transactionBase,
        timeout,
        cancellationToken,
        smLockContextSPtr);
    if (NT_SUCCESS(status) == false)
    {
        StateManagerEventSource::Events->ISPM_ApiError(
            TracePartitionId,
            ReplicaId,
            parentStateProviderId,
            "GetOrAddAsync: LockForWriteAsync failed.",
            ToStringLiteral(stateProviderName),
            transaction.TransactionId,
            status);
        co_return status;
    }

    status = StateManagerTransactionContext::Create(
        transaction.TransactionId,
        *smLockContextSPtr,
        OperationType::Add,
        GetThisAllocator(),
        parentLockContextSPtr);
    if (NT_SUCCESS(status) == false)
    {
        // If TransactionContext Create failed, release the write lock first then return NTSTATUS error code, otherwise lock leak
        // #9019333, Add tests for the case, TransactionContext Create failed after getting the read/write lock
        StateManagerEventSource::Events->ISPM_ApiError(
            TracePartitionId,
            ReplicaId,
            parentStateProviderId,
            "GetOrAddAsync: Create StateManagerTransactionContext failed.",
            ToStringLiteral(stateProviderName),
            transaction.TransactionId,
            status);
        NTSTATUS releaseLockStatus = smLockContextSPtr->ReleaseLock(transaction.TransactionId);
        if (NT_SUCCESS(releaseLockStatus) == false)
        {
            TraceError(
                L"GetOrAddAsync: StateManagerLockContext::ReleaseLock",
                releaseLockStatus);
        }

        co_return status;
    }

    // Add the add operation 
    LockContext & lockContext = static_cast<LockContext &>(*parentLockContextSPtr);

    status = transaction.AddLockContext(lockContext);
    if (NT_SUCCESS(status) == false)
    {
        StateManagerEventSource::Events->ISPM_ApiError(
            TracePartitionId,
            ReplicaId,
            FABRIC_INVALID_STATE_PROVIDER_ID,
            "GetOrAddAsync: AddLockContext",
            ToStringLiteral(stateProviderName),
            transaction.TransactionId,
            status);
        NTSTATUS releaseLockStatus = smLockContextSPtr->ReleaseLock(transaction.TransactionId);
        if (NT_SUCCESS(releaseLockStatus) == false)
        {
            TraceError(
                L"GetOrAddAsync: StateManagerLockContext::ReleaseLock",
                releaseLockStatus);
        }

        co_return status;
    }

    // If the parentStateProviderId does not match the lock context state provider id, update it.
    if (parentStateProviderId != smLockContextSPtr->StateProviderId)
    {
        StateManagerEventSource::Events->GetOrAddUpdateId(
            TracePartitionId,
            ReplicaId,
            smLockContextSPtr->StateProviderId);

        parentStateProviderId = smLockContextSPtr->StateProviderId;
    }

    // Check if the stateProvider exists or not again, it might be added before getting the write lock
    // If state provider exists, Get API will return STATUS_SUCCESS. GetOrAddAsync API should
    // return STATUS_SUCCESS and set out parameter alreadyExist as true.
    status = Get(*copyStateProviderName, stateProvider);

    // RDBug 10617854: Get might return SF_STATUS_NOT_READY when multiple GetOrAddAsync tasks run
    // into race condition.
    // If Get returns NTSTATUS error code, the return value is not STATUS_SUCCESS or SF_STATUS_NAME_DOES_NOT_EXIST.
    if (NT_SUCCESS(status) == false && status != SF_STATUS_NAME_DOES_NOT_EXIST)
    {
        StateManagerEventSource::Events->ISPM_ApiError(
            TracePartitionId,
            ReplicaId,
            smLockContextSPtr->StateProviderId,
            "GetOrAddAsync: Get failed",
            ToStringLiteral(stateProviderName),
            transaction.TransactionId,
            status);

        CO_RETURN_ON_FAILURE(status);
    }

    if (NT_SUCCESS(status))
    {
        alreadyExist = true;
        co_return STATUS_SUCCESS;
    }

    IStateProvider2::SPtr stateProviderSPtr = nullptr; 
    status = apiDispatcher_->CreateStateProvider(
        *copyStateProviderName, 
        parentStateProviderId,
        *copyTypeName, 
        initializationParameters,
        stateProviderSPtr);
    if (NT_SUCCESS(status) == false)
    {
        StateManagerEventSource::Events->ISPM_ApiError(
            TracePartitionId,
            ReplicaId,
            parentStateProviderId,
            "GetOrAddAsync: CreateStateProvider",
            ToStringLiteral(stateProviderName),
            transaction.TransactionId,
            status);
        co_return status;
    }
    
    KHashTable<KUri::CSPtr, StateProviderInfoInternal> stateProviders(
        Constants::NumberOfBuckets,
        K_DefaultHashFunction,
        Utilities::Comparer::Compare,
        GetThisAllocator());
    status = stateProviders.Status();
    if (NT_SUCCESS(status) == false)
    {
        StateManagerEventSource::Events->Error(
            TracePartitionId,
            ReplicaId,
            L"GetOrAddAsync: Create KHashTable.",
            status);
        co_return status;
    }

    status = GetChildren(*stateProviderSPtr, stateProviders);
    CO_RETURN_ON_FAILURE(status);

    // Prime enumeration.
    stateProviders.Reset();
    KUri::CSPtr childName;
    StateProviderInfoInternal childStateProviderInfo;

    // Process and replicate the children.
    while (NEXT_EXIST(stateProviders.Next(childName, childStateProviderInfo)))
    {
        KString::CSPtr childTypeName = nullptr;
        status = Copy(*childStateProviderInfo.TypeName, childTypeName);
        CO_RETURN_ON_FAILURE(status);

        // AddSingleAsync will trace if it fails
        status = co_await this->AddSingleAsync(
            transaction,
            *childName,
            *childTypeName,
            nullptr,
            *childStateProviderInfo.StateProviderSPtr,
            childStateProviderInfo.StateProviderId,
            true, // Take the lock.
            parentStateProviderId,
            timeout,
            cancellationToken);
        CO_RETURN_ON_FAILURE(status);
    }

    // AddSingleAsync will trace if it fails
    status = co_await this->AddSingleAsync(
        transaction,
        *copyStateProviderName,
        *copyTypeName,
        initializationParameters,
        *stateProviderSPtr,
        parentStateProviderId,
        false, // Lock is already taken.
        EmptyStateProviderId,
        timeout,
        cancellationToken);
    CO_RETURN_ON_FAILURE(status);

    // Set the out state provider.
    stateProvider = stateProviderSPtr;

    // Cannot close before the operation is over.
    alreadyExist = false;
    co_return STATUS_SUCCESS;
}

// Algorithms:
//   1.Validation
//   2.Create State provider, get all the children 
//   3.Acquire Write lock and AddLockContext
//   4.Lock, Transient Add and Replicate all children
//   5.Transient Add and Replicate StateProvider
//
Awaitable<NTSTATUS> StateManager::AddAsync(
    __in Transaction & transaction,
    __in KUriView const& stateProviderName,
    __in KStringView const & stateProviderTypeName,
    __in_opt OperationData const * const initializationParameters,
    __in Common::TimeSpan const & timeout,
    __in CancellationToken const & cancellationToken) noexcept
{
    // Return NTSTATUS error code SF_STATUS_OBJECT_CLOSED if closed. If not, does not allow the state manager to close before this function is completed.
    ApiEntryAsync();
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    // Return NTSTATUS error code SF_STATUS_INVALID_NAME_URI if state provider name is inapplicable (uses reserved state managed name)
    // CheckIfNameIsValid will trace if it fails
    status = CheckIfNameIsValid(stateProviderName, false);
    CO_RETURN_ON_FAILURE(status);

    // CheckIfWritable will trace if it fails
    status = CheckIfWritable("AddAsync", transaction, stateProviderName);
    CO_RETURN_ON_FAILURE(status);

    // In Managed serialization mode, the initializationParameters buffer count should only have 0 or 1 buffer and 0 indicates the byte array is empty.
    if (serializationMode_ == SerializationMode::Managed && initializationParameters != nullptr && initializationParameters->BufferCount > 1)
    {
        StateManagerEventSource::Events->ISPM_ApiError(
            TracePartitionId,
            ReplicaId,
            FABRIC_INVALID_STATE_PROVIDER_ID,
            "GetOrAddAsync: InitializationParameters is invalid.",
            ToStringLiteral(stateProviderName),
            transaction.TransactionId,
            STATUS_INVALID_PARAMETER);

        co_return STATUS_INVALID_PARAMETER;
    }

    // Copy will trace if it fails
    KUri::CSPtr copyStateProviderName = nullptr;
    status = Copy(stateProviderName, copyStateProviderName);
    CO_RETURN_ON_FAILURE(status);

    // Copy will trace if it fails
    KString::CSPtr copyTypeName = nullptr;
    status = Copy(stateProviderTypeName, copyTypeName);
    CO_RETURN_ON_FAILURE(status);

    FABRIC_STATE_PROVIDER_ID parentStateProviderId = CreateStateProviderId();

    // Can return NTSTATUS error code if the factory does not handle it.
    // ApiDispatcher::CreateStateProvider will trace if it fails
    IStateProvider2::SPtr stateProviderSPtr = nullptr;
    status = apiDispatcher_->CreateStateProvider(
        *copyStateProviderName, 
        parentStateProviderId,
        *copyTypeName, 
        initializationParameters,
        stateProviderSPtr);
    CO_RETURN_ON_FAILURE(status);

    // Can return NTSTATUS error code due to duplicate name.
    KHashTable<KUri::CSPtr, StateProviderInfoInternal> stateProviders(
        Constants::NumberOfBuckets,
        K_DefaultHashFunction,
        Utilities::Comparer::Compare,
        GetThisAllocator());
    status = stateProviders.Status();
    if (NT_SUCCESS(status) == false)
    {
        StateManagerEventSource::Events->Error(
            TracePartitionId,
            ReplicaId,
            L"AddAsync: Create KHashTable.",
            status);
        co_return status;
    }

    // GetChildren will trace if it fails
    status = GetChildren(*stateProviderSPtr, stateProviders);
    CO_RETURN_ON_FAILURE(status);

    // MetadataManager::LockForWriteAsync will trace if it fails
    TransactionBase const & transactionBase = static_cast<TransactionBase const &>(transaction);
    StateManagerLockContext::SPtr smLockContextSPtr = nullptr;
    status = co_await metadataManagerSPtr_->LockForWriteAsync(
        *copyStateProviderName,
        parentStateProviderId, 
        transactionBase, 
        timeout, 
        cancellationToken,
        smLockContextSPtr);
    CO_RETURN_ON_FAILURE(status);

    StateManagerTransactionContext::SPtr parentLockContextSPtr;
    status = StateManagerTransactionContext::Create(
        transaction.TransactionId, 
        *smLockContextSPtr, 
        OperationType::Add, 
        GetThisAllocator(), 
        parentLockContextSPtr);
    if (NT_SUCCESS(status) == false)
    {
        StateManagerEventSource::Events->ISPM_ApiError(
            TracePartitionId,
            ReplicaId,
            parentStateProviderId,
            "AddAsync: Create StateManagerTransactionContext failed.",
            ToStringLiteral(stateProviderName),
            transaction.TransactionId,
            status);

        // If TransactionContext Create failed, release the write lock first then return NTSTATUS error code, otherwise lock leak
        // #9019333, Add tests for the case, TransactionContext Create failed after getting the read/write lock
        NTSTATUS releaseLockStatus = smLockContextSPtr->ReleaseLock(transaction.TransactionId);
        if (NT_SUCCESS(releaseLockStatus) == false)
        {
            TraceError(
                L"AddAsync: StateManagerLockContext::ReleaseLock",
                releaseLockStatus);
        }

        co_return status;
    }
    
    LockContext & lockContext = static_cast<LockContext &>(*parentLockContextSPtr);
    status = transaction.AddLockContext(lockContext);
    if (NT_SUCCESS(status) == false)
    {
        StateManagerEventSource::Events->ISPM_ApiError(
            TracePartitionId,
            ReplicaId,
            parentStateProviderId,
            "AddAsync: AddLockContext.",
            ToStringLiteral(stateProviderName),
            transaction.TransactionId,
            status);
        NTSTATUS releaseLockStatus = smLockContextSPtr->ReleaseLock(transaction.TransactionId);
        if (NT_SUCCESS(releaseLockStatus) == false)
        {
            TraceError(
                L"AddAsync: StateManagerLockContext::ReleaseLock",
                releaseLockStatus);
        }

        co_return status;
    }

    // If the parentStateProviderId does not match the lock context state provider id, update it.
    if (parentStateProviderId != smLockContextSPtr->StateProviderId)
    {
        StateManagerEventSource::Events->AddUpdateId(
            TracePartitionId,
            ReplicaId,
            smLockContextSPtr->StateProviderId);

        parentStateProviderId = smLockContextSPtr->StateProviderId;
    }

    // Prime enumeration.
    stateProviders.Reset();
    KUri::CSPtr childName;
    StateProviderInfoInternal childStateProviderInfo;

    // Process and replicate the children.
    while(NEXT_EXIST(stateProviders.Next(childName, childStateProviderInfo)))
    {
        KString::CSPtr childTypeName = nullptr;

        // Copy will trace if it fails
        status = Copy(*childStateProviderInfo.TypeName, childTypeName);
        CO_RETURN_ON_FAILURE(status);

        // AddSingleAsync will trace if it fails
        status = co_await this->AddSingleAsync(
            transaction, 
            *childName,
            *childTypeName,
            nullptr,
            *childStateProviderInfo.StateProviderSPtr,
            childStateProviderInfo.StateProviderId,
            true, // Take the lock.
            parentStateProviderId, 
            timeout, 
            cancellationToken);
        CO_RETURN_ON_FAILURE(status);
    }

    // AddSingleAsync will trace if it fails
    status = co_await this->AddSingleAsync(
        transaction, 
        *copyStateProviderName,
        *copyTypeName,
        initializationParameters,
        *stateProviderSPtr,
        parentStateProviderId, 
        false, // Lock is already taken.
        EmptyStateProviderId,
        timeout, 
        cancellationToken);
    CO_RETURN_ON_FAILURE(status);

    // Cannot close before the operation is over.
    co_return status;
}

Awaitable<NTSTATUS> StateManager::RemoveAsync(
    __in Transaction & transaction,
    __in KUriView const & stateProviderName,
    __in Common::TimeSpan const & timeout,
    __in CancellationToken const & cancellationToken) noexcept
{
    // Return NTSTATUS error code SF_STATUS_OBJECT_CLOSED if closed. If not, does not allow the state manager to close before this function is completed.
    ApiEntryAsync();
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    // Assumption: transaction and stateProviderName cannot be Null.
    // CheckIfNameIsValid will trace if it fails
    status = CheckIfNameIsValid(stateProviderName, false);
    CO_RETURN_ON_FAILURE(status);
    
    // CheckIfWritable will trace if it fails
    status = CheckIfWritable("RemoveAsync", transaction, stateProviderName);
    CO_RETURN_ON_FAILURE(status);

    // Copy will trace if it fails
    KUri::CSPtr copyStateProviderName = nullptr;
    status = Copy(stateProviderName, copyStateProviderName);
    CO_RETURN_ON_FAILURE(status);

    Metadata::SPtr metadataSPtr = nullptr;
    bool isExist = false;
    // TryGetMetadata may throw exception. Catches it and returns NTSTATUS error code.
    try
    {
        // Verify that the state provider already exists.
        // Note: This existence check is a optimization to avoid GetChildren call if Metadata does not exist.
        isExist = metadataManagerSPtr_->TryGetMetadata(*copyStateProviderName, false, metadataSPtr);
        if (!isExist)
        {
            StateManagerEventSource::Events->ISPM_ApiError(
                TracePartitionId,
                ReplicaId,
                FABRIC_INVALID_STATE_PROVIDER_ID,
                "RemoveAsync: State provider does not exist.",
                ToStringLiteral(stateProviderName),
                transaction.TransactionId,
                SF_STATUS_NAME_DOES_NOT_EXIST);
            co_return SF_STATUS_NAME_DOES_NOT_EXIST;
        }
    }
    catch (Exception & exception)
    {
        StateManagerEventSource::Events->ISPM_ApiError(
            TracePartitionId,
            ReplicaId,
            FABRIC_INVALID_STATE_PROVIDER_ID,
            "RemoveAsync: MetadataManager::TryGetMetadata throws exception",
            ToStringLiteral(stateProviderName),
            transaction.TransactionId,
            exception.GetStatus());
        co_return exception.GetStatus();
    }

    // Note: PreethaS: This is a workaround until state manager supports hierarchy. Once hierarchy is supported,
    // hierarchical information should be available in the state manager and should not be queried from the state provider.
    // Query for children
    KHashTable<KUri::CSPtr, StateProviderInfoInternal> children(
        Constants::NumberOfBuckets,
        K_DefaultHashFunction,
        Utilities::Comparer::Compare,
        GetThisAllocator());
    status = children.Status();
    if (NT_SUCCESS(status) == false)
    {
        StateManagerEventSource::Events->Error(
            TracePartitionId,
            ReplicaId,
            L"RemoveAsync: Create KHashTable.",
            status);
        co_return status;
    }

    // GetChildren will trace if it fails
    status = GetChildren(*metadataSPtr->StateProvider, children);
    CO_RETURN_ON_FAILURE(status);

    // Call parent first.
    // RemoveSingleAsync will trace if it fails
    status = co_await this->RemoveSingleAsync(transaction, *copyStateProviderName, timeout, cancellationToken);
    CO_RETURN_ON_FAILURE(status);

    children.Reset();
    KUri::CSPtr childName;
    StateProviderInfoInternal childStateProviderInfo;

    // Process and replicate the children.
    while (NEXT_EXIST(children.Next(childName, childStateProviderInfo)))
    {
        // RemoveSingleAsync will trace if it fails
        status = co_await this->RemoveSingleAsync(
            transaction,
            *childName,
            timeout,
            cancellationToken);
        CO_RETURN_ON_FAILURE(status);
    }

    co_return status;
}

Awaitable<NTSTATUS> StateManager::BeginTransactionAsync(
    __in Transaction & transaction, 
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in_opt OperationData const * const metaData,
    __in_opt OperationData const * const undo,
    __in_opt OperationData const * const redo,
    __in_opt OperationContext const * const operationContext,
    __out FABRIC_SEQUENCE_NUMBER & commitLsn) noexcept
{
    ApiEntryAsync();

    try 
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        NamedOperationContext::SPtr namedOperationContextSPtr = nullptr;

        // IsRegistered will trace if it fails
        status = IsRegistered(stateProviderId, false, transaction.TransactionId);
        CO_RETURN_ON_FAILURE(status);

        // If operation context exist, wrapped around NamedOperationContext so that we can dispatch.
        if (operationContext != nullptr)
        {
            status = NamedOperationContext::Create(operationContext, stateProviderId, GetThisAllocator(), namedOperationContextSPtr);
            if (NT_SUCCESS(status) == false)
            {
                TraceError(L"BeginTransactionAsync: NamedOperationContext::Create", status);
                co_return status;
            }
        }

        NamedOperationData::CSPtr namedOperationDataCSPtr = nullptr;
        status = NamedOperationData::Create(
            stateProviderId,
            serializationMode_,
            GetThisAllocator(),
            metaData,
            namedOperationDataCSPtr);
        if (NT_SUCCESS(status) == false)
        {
            TraceError(L"BeginTransactionAsync: NamedOperationData::Create", status);
            co_return status;
        }

        OperationData const * operationDataPtr = static_cast<OperationData const *>(namedOperationDataCSPtr.RawPtr());

        // Since we do not add into the existing Operation Data and instead make a shallow copy,
        // We do not need to catch any exception and remove "Context".
        ILoggingReplicator::SPtr loggingReplicatorSPtr = GetReplicator();

        status = co_await loggingReplicatorSPtr->BeginTransactionAsync(
            transaction,
            operationDataPtr,
            undo,
            redo,
            stateProviderId,
            namedOperationContextSPtr.RawPtr(), 
            commitLsn);

        co_return status;
    }
    catch (Exception & e)
    {
        co_return e.GetStatus();
    }
}

NTSTATUS StateManager::BeginTransaction(
    __in Transaction& transaction,
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in_opt OperationData const* const metaData,
    __in_opt OperationData const* const undo,
    __in_opt OperationData const* const redo,
    __in_opt OperationContext const* const operationContext) noexcept
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    NamedOperationContext::SPtr namedOperationContextSPtr = nullptr;

    try
    {
        status = IsRegistered(stateProviderId, false, transaction.TransactionId);
        RETURN_ON_FAILURE(status);

        // If operation context exist, wrapped around NamedOperationContext so that we can dispatch.
        if (operationContext != nullptr)
        {
            status = NamedOperationContext::Create(operationContext, stateProviderId, GetThisAllocator(), namedOperationContextSPtr);
            if (NT_SUCCESS(status) == false)
            {
                TraceError(L"BeginTransactionAsync: NamedOperationContext", status);
                return status;
            }
        }

        NamedOperationData::CSPtr namedOperationDataCSPtr = nullptr;
        status = NamedOperationData::Create(
            stateProviderId,
            serializationMode_,
            GetThisAllocator(),
            metaData,
            namedOperationDataCSPtr);
        if (NT_SUCCESS(status) == false)
        {
            TraceError(L"BeginTransactionAsync: NamedOperationData", status);
            return status;
        }

        OperationData const * operationDataPtr = static_cast<OperationData const *>(namedOperationDataCSPtr.RawPtr());

        // Since we do not add into the existing Operation Data and instead make a shallow copy,
        // We do not need to catch any exception and remove "Context".
        ILoggingReplicator::SPtr loggingReplicatorSPtr = GetReplicator();

        return loggingReplicatorSPtr->BeginTransaction(
            transaction,
            operationDataPtr,
            undo,
            redo,
            stateProviderId,
            namedOperationContextSPtr.RawPtr());
    }
    catch (Exception & e)
    {
        return e.GetStatus();
    }
}

NTSTATUS StateManager::AddOperation(
    __in Transaction& transaction, 
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in_opt OperationData const* const metaData,
    __in_opt OperationData const* const undo,
    __in_opt OperationData const* const redo,
    __in_opt OperationContext const* const operationContext) noexcept
{
    try
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        NamedOperationContext::SPtr namedOperationContextSPtr = nullptr;

        status = IsRegistered(stateProviderId, false, transaction.TransactionId);
        RETURN_ON_FAILURE(status);

        // If operation context exist, wrapped around NamedOperationContext so that we can dispatch.
        if (operationContext != nullptr)
        {
            status = NamedOperationContext::Create(operationContext, stateProviderId, GetThisAllocator(), namedOperationContextSPtr);
            if (NT_SUCCESS(status) == false)
            {
                TraceError(L"AddOperation: NamedOperationContext::Create", status);
                return status;
            }
        }

        NamedOperationData::CSPtr namedOperationDataCSPtr = nullptr;
        status = NamedOperationData::Create(
            stateProviderId,
            serializationMode_,
            GetThisAllocator(),
            metaData,
            namedOperationDataCSPtr);
        if (NT_SUCCESS(status) == false)
        {
            TraceError(L"AddOperation: NamedOperationData::Create", status);
            return status;
        }

        OperationData const * operationDataPtr = static_cast<OperationData const *>(namedOperationDataCSPtr.RawPtr());

        // Since we do not add into the existing Operation Data and instead make a shallow copy,
        // We do not need to catch any exception and remove "Context".
        ILoggingReplicator::SPtr loggingReplicatorSPtr = GetReplicator();

        return loggingReplicatorSPtr->AddOperation(
            transaction,
            operationDataPtr,
            undo,
            redo,
            stateProviderId,
            namedOperationContextSPtr.RawPtr());
    }
    catch (Exception & e)
    {
        return e.GetStatus();
    }
}

Awaitable<NTSTATUS> StateManager::AddOperationAsync(
    __in AtomicOperation & atomicOperation,
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in_opt OperationData const * const metaData,
    __in_opt OperationData const * const undo,
    __in_opt OperationData const * const redo,
    __in_opt OperationContext const * const operationContext,
    __out FABRIC_SEQUENCE_NUMBER & commitLsn) noexcept
{
    ApiEntryAsync();

    try
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        NamedOperationContext::SPtr namedOperationContextSPtr = nullptr;

        // IsRegistered will trace if it fails
        status = IsRegistered(stateProviderId, false, atomicOperation.TransactionId);
        CO_RETURN_ON_FAILURE(status);

        // If operation context exist, wrapped around NamedOperationContext so that we can dispatch.
        if (operationContext != nullptr)
        {
            status = NamedOperationContext::Create(operationContext, stateProviderId, GetThisAllocator(), namedOperationContextSPtr);
            if (NT_SUCCESS(status) == false)
            {
                TraceError(L"AddOperationAsync: NamedOperationContext", status);
                co_return status;
            }
        }

        NamedOperationData::CSPtr namedOperationDataCSPtr = nullptr;
        status = NamedOperationData::Create(
            stateProviderId,
            serializationMode_,
            GetThisAllocator(),
            metaData,
            namedOperationDataCSPtr);
        if (NT_SUCCESS(status) == false)
        {
            TraceError(L"AddOperationAsync: NamedOperationData", status);
            co_return status;
        }

        OperationData const * operationDataPtr = static_cast<OperationData const *>(namedOperationDataCSPtr.RawPtr());

        // Since we do not add into the existing Operation Data and instead make a shallow copy,
        // We do not need to catch any exception and remove "Context".
        ILoggingReplicator::SPtr loggingReplicatorSPtr = GetReplicator();

        status = co_await loggingReplicatorSPtr->AddOperationAsync(
            atomicOperation,
            operationDataPtr,
            undo,
            redo,
            stateProviderId,
            namedOperationContextSPtr.RawPtr(),
            commitLsn);

        co_return status;
    }
    catch (Exception & e)
    {
        co_return e.GetStatus();
    }
}

Awaitable<NTSTATUS> StateManager::AddOperationAsync(
    __in AtomicRedoOperation & atomicRedoOperation,
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in_opt OperationData const * const metaData,
    __in_opt OperationData const * const redo,
    __in_opt OperationContext const * const operationContext,
    __out FABRIC_SEQUENCE_NUMBER & commitLsn) noexcept
{
    ApiEntryAsync();

    try
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        NamedOperationContext::SPtr namedOperationContextSPtr = nullptr;

        // IsRegistered will trace if it fails
        status = IsRegistered(stateProviderId, false, atomicRedoOperation.TransactionId);
        CO_RETURN_ON_FAILURE(status);

        // If operation context exist, wrapped around NamedOperationContext so that we can dispatch.
        if (operationContext != nullptr)
        {
            status = NamedOperationContext::Create(operationContext, stateProviderId, GetThisAllocator(), namedOperationContextSPtr);
            if (NT_SUCCESS(status) == false)
            {
                TraceError(L"AddOperationAsync: NamedOperationContext", status);
                co_return status;
            }
        }

        NamedOperationData::CSPtr namedOperationDataCSPtr = nullptr;
        status = NamedOperationData::Create(
            stateProviderId,
            serializationMode_,
            GetThisAllocator(),
            metaData,
            namedOperationDataCSPtr);
        if (NT_SUCCESS(status) == false)
        {
            TraceError(L"AddOperationAsync: NamedOperationData", status);
            co_return status;
        }

        OperationData const * operationDataPtr = static_cast<OperationData const *>(namedOperationDataCSPtr.RawPtr());

        // Since we do not add into the existing Operation Data and instead make a shallow copy,
        // We do not need to catch any exception and remove "Context".
        ILoggingReplicator::SPtr loggingReplicatorSPtr = GetReplicator();

        status = co_await loggingReplicatorSPtr->AddOperationAsync(
            atomicRedoOperation,
            operationDataPtr,
            redo,
            stateProviderId,
            namedOperationContextSPtr.RawPtr(),
            commitLsn);

        co_return status;
    }
    catch (Exception & e)
    {
        co_return e.GetStatus();
    }
}

#pragma region IStateProviderManager
Awaitable<NTSTATUS> StateManager::ApplyAsync(
    __in LONG64 logicalSequenceNumber,
    __in TransactionBase const & transactionBase,
    __in ApplyContext::Enum applyContext,
    __in_opt OperationData const * const metadataPtr,
    __in_opt OperationData const * const dataPtr,
    __out TxnReplicator::OperationContext::CSPtr & result) noexcept
{
    ApiEntryAsync();

    ASSERT_IFNOT(metadataPtr != nullptr, "{0}: Null metadata object", TraceId);

    try
    {
        ApplyContext::Enum roleType = static_cast<ApplyContext::Enum>(applyContext & ApplyContext::ROLE_MASK);

        FABRIC_REPLICA_ROLE role = GetCurrentRole();

        NamedOperationData::CSPtr namedOperationDataSPtr = nullptr;
        if (role == FABRIC_REPLICA_ROLE_PRIMARY)
        {
            ASSERT_IFNOT(transactionBase.IsPrimaryTransaction, "{0}: Non-Primary xact during apply", TraceId);
            ASSERT_IFNOT(roleType == ApplyContext::PRIMARY, "{0}: Role type is not Primary during apply", TraceId);

            NamedOperationData const * const namedOperationDataPtr = dynamic_cast<NamedOperationData const * const>(metadataPtr);
            namedOperationDataSPtr = namedOperationDataPtr;
        }
        else
        {
            ASSERT_IF(transactionBase.IsPrimaryTransaction, "{0}: Primary xact during apply on non primary role", TraceId);
            ASSERT_IFNOT(roleType != ApplyContext::PRIMARY, "{0}: Unexpected Primary role type encountered during apply", TraceId);

            NTSTATUS status = NamedOperationData::Create(GetThisAllocator(), metadataPtr, namedOperationDataSPtr);
            if (NT_SUCCESS(status) == false)
            {
                TraceError(L"ApplyAsync: NamedOperationData::Create", status);
                co_return status;
            }
        }

        result = co_await this->ApplyAsync(
            logicalSequenceNumber, 
            transactionBase,
            applyContext, 
            namedOperationDataSPtr->StateProviderId,
            namedOperationDataSPtr->UserOperationDataCSPtr.RawPtr(),
            dataPtr);
    }
    catch (Exception & e)
    {
        result = nullptr;
        co_return e.GetStatus();
    }

    co_return STATUS_SUCCESS;
}

NTSTATUS StateManager::Unlock(
    __in OperationContext const& operationContext) noexcept
{
    ApiEntryReturn();

    FABRIC_REPLICA_ROLE role = GetCurrentRole();

    const NamedOperationContext& namedOperationContext = dynamic_cast<NamedOperationContext const &>(operationContext);

    // If a state manager operation context
    if (namedOperationContext.StateProviderId == StateManagerId)
    {
        ASSERT_IFNOT(role != FABRIC_REPLICA_ROLE_PRIMARY, "{0}: this.Role != ReplicaRole.Primary", TraceId);
        return UnlockOnLocalState(namedOperationContext);
    }

    // User context.
    if (namedOperationContext.OperationContextCSPtr != nullptr)
    {
        // Note: The only exception thrown from TryGetMetadata is STATUS_INSUFFICIENT_RESOURCES. Since we
        // can not do much for this exception, remove the try-catch block.
        Metadata::SPtr metadataSPtr = nullptr;
        bool isExist = metadataManagerSPtr_->TryGetMetadata(namedOperationContext.StateProviderId, metadataSPtr);
        ASSERT_IFNOT(
            isExist,
            "{0}: MetadataSPtr cannot be nullptr. SPID {1}",
            TraceId,
            namedOperationContext.StateProviderId);

        try
        {
            metadataSPtr->StateProvider->Unlock(*namedOperationContext.OperationContextCSPtr);
        }
        catch (Exception & e)
        {
            return e.GetStatus();
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS StateManager::PrepareCheckpoint(
    __in FABRIC_SEQUENCE_NUMBER checkpointLSN) noexcept
{
    ApiEntryReturn();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    ASSERT_IFNOT(
        checkpointLSN >= 0, 
        "{0}: Checkpoint LSN must >= 0 during prepare, checkpointLSN: {1}.", 
        TraceId, 
        checkpointLSN);

    StateManagerEventSource::Events->PrepareCheckpointStart(
        TracePartitionId,
        ReplicaId,
        checkpointLSN);

    try
    {
        checkpointManagerSPtr_->PrepareCheckpoint(*metadataManagerSPtr_, checkpointLSN);
    }
    catch (Exception & e)
    {
        TraceException(L"PrepareCheckpoint: CheckpointManager::PrepareCheckpoint", e);
        return e.GetStatus();
    }

    status = PrepareCheckpointOnStateProviders(checkpointLSN);
    if (NT_SUCCESS(status) == false)
    {
        TraceError(L"PrepareCheckpoint: ISP2.PrepareCheckpoint", status);
        return status;
    }

    StateManagerEventSource::Events->PrepareCheckpointCompleted(
        TracePartitionId,
        ReplicaId,
        checkpointLSN);

    return status;
}

Awaitable<NTSTATUS> StateManager::PerformCheckpointAsync(
    __in CancellationToken const & cancellationToken) noexcept
{
    ApiEntryAsync();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    StateManagerEventSource::Events->PerformCheckpointStart(
        TracePartitionId,
        ReplicaId,
        checkpointManagerSPtr_->PrepareCheckpointLSN);

    status = co_await this->PerformCheckpointOnStateProvidersAsync(cancellationToken);
    if (NT_SUCCESS(status) == false)
    {
        TraceError(L"PerformCheckpointAsync: ISP2.PerformCheckpointAsync", status);
        co_return status;
    }

    try
    {
        co_await checkpointManagerSPtr_->PerformCheckpointAsync(*metadataManagerSPtr_, cancellationToken, hasPersistedState_);
    }
    catch (Exception & e)
    {
        TraceException(L"PerformCheckpointAsync: CheckpointManager::PerformCheckpointAsync", e);
        co_return e.GetStatus();
    }

    StateManagerEventSource::Events->PerformCheckpointCompleted(
        TracePartitionId,
        ReplicaId,
        checkpointManagerSPtr_->PrepareCheckpointLSN);

    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> StateManager::CompleteCheckpointAsync(
    __in CancellationToken const & cancellationToken) noexcept
{
    ApiEntryAsync();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    StateManagerEventSource::Events->CompleteCheckpointStart(
        TracePartitionId,
        ReplicaId,
        checkpointManagerSPtr_->PrepareCheckpointLSN);

    // MCoskun: Order of the following operations is significant.
    // #9291020: We depend on CompleteCheckpointLogRecord only being written after replace file is guaranteed to happen by the time
    // current file was to be re-openned even in case of abrupt failure like power loss.
    // CompleteCheckpointOnLocalStateAsync (SafeFileReplaceAsync) calls MoveFileEx(WriteThrough) which flushes the NTFS metadata log.
    // This ensures that replacing the current file is not lost in case of power failure after they have returned TRUE.
    // Note that we are taking a dependency on the fact that calling MoveFileEx with WriteThrough will flush the entire metadata log for the drive.
    // By taking this dependency, we do not have to use write-through on the state provider's replace file in CompleteCheckpoint.
    status = co_await this->CompleteCheckpointOnStateProvidersAsync(cancellationToken);
    if (NT_SUCCESS(status) == false)
    {
        TraceError(L"CompleteCheckpointAsync: ISP2.CompleteCheckpointAsync", status);
        co_return status;
    }

    try
    {
        co_await checkpointManagerSPtr_->CompleteCheckpointAsync(*metadataManagerSPtr_, cancellationToken);
        co_await this->CleanUpMetadataAsync(cancellationToken);
    }
    catch (Exception & e)
    {
        TraceError(L"CompleteCheckpointAsync: CheckpointManager::CompleteCheckpoint or CleanUpMetadataAsync", e.GetStatus());
        co_return e.GetStatus();
    }

    StateManagerEventSource::Events->CompleteCheckpointCompleted(
        TracePartitionId,
        ReplicaId,
        checkpointManagerSPtr_->PrepareCheckpointLSN);

    co_return STATUS_SUCCESS;
}

// Backup the state manager and state providers
// Algorithms:
//   1.Backup the state manager's checkpoint
//   2.Backup all active state providers checkpoints
ktl::Awaitable<NTSTATUS> StateManager::BackupCheckpointAsync(
    __in KString const & backupDirectory,
    __in CancellationToken const & cancellationToken) noexcept
{
    ApiEntryAsync();

    StateManagerEventSource::Events->BackupStart(
        TracePartitionId,
        ReplicaId);

    // Wait for all operations to drain.
    try
    {
        // Backup the state manager's checkpoint
        co_await checkpointManagerSPtr_->BackupCheckpointAsync(
            backupDirectory,
            cancellationToken);

        // Backup all active state providers in the CopyOrCheckpointMetadataSnapshot
        co_await checkpointManagerSPtr_->BackupActiveStateProviders(backupDirectory, cancellationToken);
    }
    catch (Exception & exception)
    {
        TraceException(L"BackupAsync threw exception during wait on task to complete", exception);
        co_return exception.GetStatus();
    }

    StateManagerEventSource::Events->BackupEnd(
        TracePartitionId,
        ReplicaId);

    co_return STATUS_SUCCESS;
}

// Restore the SM and the state providers from the backed-up state
// Algorithms:
//   1.Remove and Clear current state 
//   2.Re-initialize metadata manager
//   3.Restore on state manager
//   4.Restore on state providers
ktl::Awaitable<NTSTATUS> StateManager::RestoreCheckpointAsync(
    __in KString const & backupDirectory,
    __in CancellationToken const & cancellationToken) noexcept
{
    ApiEntryAsync();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    StateManagerEventSource::Events->RestoreStart(
        TracePartitionId,
        ReplicaId);

    // Remove the state of all current state providers.
    // Note: Traces failures.
    status = co_await this->ChangeRoleOnStateProvidersAsync(FABRIC_REPLICA_ROLE_NONE, cancellationToken);
    CO_RETURN_ON_FAILURE(status);

    // Remove state for all the state providers that have been recovered so far.
    // Note: Traces failures.
    status = co_await this->RemoveStateOnStateProvidersAsync(cancellationToken);
    CO_RETURN_ON_FAILURE(status);

    // Clear the current in-memory state
    // Note: Traces failures.
    status = co_await this->CloseStateProvidersAsync(cancellationToken);
    CO_RETURN_ON_FAILURE(status);

    try
    {
        checkpointManagerSPtr_->Clean();

        metadataManagerSPtr_->Dispose();
    }
    catch (Exception const & exception)
    {
        TraceException(L"RestoreAsync: Clean or Dispose", exception);
        co_return exception.GetStatus();
    }

    // re-initialize
    status = MetadataManager::Create(*PartitionedReplicaIdentifier, GetThisAllocator(), metadataManagerSPtr_);
    if (NT_SUCCESS(status) == false)
    {
        TraceError(L"RestoreCheckpointAsync: MetadataManager::Create", status);
        co_return status;
    }

    {
        // Port Note: Managed code does not do this because DSM does not maintain role.
        // Port Note: This lock does not exist on managed.
        // Since ChangeRoleAsync does not run concurrently with itself or in-flight ApplyAsync calls,
        // This lock is just used for defensive coding purposes.
        roleLock_.AcquireExclusive();
        KFinally([&] {roleLock_.ReleaseExclusive(); });

        changeRoleCompleted_ = true;
        role_ = FABRIC_REPLICA_ROLE_UNKNOWN;
    }

    // Start the restore process
    try
    {
        co_await checkpointManagerSPtr_->RestoreCheckpointAsync(
            backupDirectory,
            cancellationToken);

        co_await this->RecoverStateManagerCheckpointAsync(cancellationToken);
    }
    catch (Exception const & exception)
    {
        TraceException(L"RestoreAsync: RestoreCheckpointAsync or RecoverStateManagerCheckpointAsync ", exception);
        co_return exception.GetStatus();
    }

    // Note: Traces failures.
    status = co_await this->OpenStateProvidersAsync(cancellationToken);
    CO_RETURN_ON_FAILURE(status);

    status = co_await this->RestoreStateProvidersAsync(backupDirectory, cancellationToken);
    if (NT_SUCCESS(status) == false)
    {
        TraceError(L"RestoreCheckpointAsync: RestoreStateProvidersAsync", status);
        AbortStateProviders();
        co_return status;
    }

    status = co_await this->RecoverStateProvidersAsync(cancellationToken);
    if (NT_SUCCESS(status) == false)
    {
        TraceError(L"RestoreCheckpointAsync: RecoverStateProvidersAsync", status);
        AbortStateProviders();
        co_return status;
    }
    
    StateManagerEventSource::Events->RestoreEnd(
        TracePartitionId,
        ReplicaId);

    co_return STATUS_SUCCESS;
}

// Restore all state providers
ktl::Awaitable<NTSTATUS> StateManager::RestoreStateProvidersAsync(
    __in KString const & backupDirectory,
    __in CancellationToken const & cancellationToken) noexcept
{
    ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    StateManagerEventSource::Events->RestoreStateProviderStart(
        TracePartitionId,
        ReplicaId);

    KArray<Metadata::CSPtr> stateProviders = checkpointManagerSPtr_->GetCopyOrCheckpointActiveList();
    KArray<KString::CSPtr> backupDirectoryArray(GetThisAllocator(), stateProviders.Count());
    CO_RETURN_ON_FAILURE(backupDirectoryArray.Status());
    
    for (ULONG i = 0; i < stateProviders.Count(); i++)
    {
        Metadata::CSPtr const metadataCSPtr = stateProviders[i];

        KString::SPtr stateProviderIdStringSPtr = Helper::StateProviderIDToKStringSPtr(
            *PartitionedReplicaIdentifier,
            metadataCSPtr->StateProviderId,
            Helper::StateManager,
            GetThisAllocator());

        // Can only fail with OOM.
        KString::SPtr folderName = KPath::Combine(backupDirectory, *stateProviderIdStringSPtr, GetThisAllocator());

        status = backupDirectoryArray.Append(folderName.RawPtr());
        CO_RETURN_ON_FAILURE(status);
    }

    status = co_await apiDispatcher_->RestoreCheckpointAsync(stateProviders, backupDirectoryArray, cancellationToken);
    CO_RETURN_ON_FAILURE(status);

    StateManagerEventSource::Events->RestoreStateProviderEnd(
        TracePartitionId,
        ReplicaId);

    co_return status;
}

#pragma endregion 

Awaitable<OperationContext::CSPtr> StateManager::ApplyOnLocalStateAsync(
    __in FABRIC_SEQUENCE_NUMBER applyLSN,
    __in TransactionBase const & transactionBase,
    __in ApplyContext::Enum applyContext,
    __in_opt OperationData const * const metadataPtr,
    __in_opt OperationData const * const dataPtr)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    OperationContext::CSPtr transactionContextSPtr = nullptr;

    ApplyContext::Enum roleType = static_cast<ApplyContext::Enum>(applyContext & ApplyContext::ROLE_MASK);
    ApplyContext::Enum operationType = static_cast<ApplyContext::Enum>(applyContext & ApplyContext::OPERATION_MASK);

    ASSERT_IFNOT(
        metadataPtr != nullptr, 
        "{0}: ApplyOnLocalStateAsync: Null metadata on apply, ApplyLSN: {1} TxnId: {2} CommitLSN: {3} ApplyContext: {4}", 
        TraceId,
        applyLSN,
        transactionBase.TransactionId,
        transactionBase.CommitSequenceNumber,
        static_cast<int>(applyContext));
    ASSERT_IFNOT(
        metadataPtr->BufferCount > 0, 
        "{0}: ApplyOnLocalStateAsync: The buffer count of metadata operation should be positive. BufferCount: {1} ApplyLSN: {2} TxnId: {3} CommitLSN: {4} ApplyContext: {5}", 
        TraceId,
        metadataPtr->BufferCount,
        applyLSN,
        transactionBase.TransactionId,
        transactionBase.CommitSequenceNumber,
        static_cast<int>(applyContext));

    StateManagerEventSource::Events->ApplyAsyncStart(
        TracePartitionId,
        ReplicaId,
        applyLSN,
        applyContext,
        transactionBase.TransactionId,
        transactionBase.CommitSequenceNumber);

    if (roleType == ApplyContext::PRIMARY)
    {
        ASSERT_IFNOT(
            operationType == ApplyContext::REDO, 
            "{0}: Redo operation type expected during apply on Primary, ApplyLSN: {1} TxnId: {2} CommitLSN: {3} ApplyContext: {4}", 
            TraceId,
            applyLSN,
            transactionBase.TransactionId,
            transactionBase.CommitSequenceNumber,
            static_cast<int>(applyContext));

        MetadataOperationData const * const metadataOperationDataPtr = dynamic_cast<MetadataOperationData const * const>(metadataPtr);
        ASSERT_IFNOT(
            metadataOperationDataPtr != nullptr,
            "{0}: ApplyOnLocalStateAsync: Null MetadataOperationData after cast, ApplyLSN: {1} TxnId: {2} CommitLSN: {3} ApplyContext: {4}",
            TraceId,
            applyLSN,
            transactionBase.TransactionId,
            transactionBase.CommitSequenceNumber,
            static_cast<int>(applyContext));

        transactionContextSPtr = co_await this->ApplyOnPrimaryAsync(
            applyLSN,
            transactionBase,
            applyContext,
            *metadataOperationDataPtr,
            dataPtr);

        co_return transactionContextSPtr;
    }

    MetadataOperationData::CSPtr metadataOperationDataCSPtr = nullptr;
    status = MetadataOperationData::Create(GetThisAllocator(), metadataPtr, metadataOperationDataCSPtr);
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"ApplyOnLocalStateAsync: Create MetadataOperationData.",
        Helper::StateManager);

    if (operationType == ApplyContext::FALSE_PROGRESS)
    {
        ASSERT_IFNOT(dataPtr == nullptr, "{0}: DataPtr must be nullptr", TraceId);

        transactionContextSPtr = co_await this->ApplyOnFalseProgressAsync(
            applyLSN,
            transactionBase,
            applyContext,
            *metadataOperationDataCSPtr);

        co_return transactionContextSPtr;
    }
    else if (roleType == ApplyContext::SECONDARY)
    {
        // Divergence from Managed: Bug in managed. It cannot be FALSE_PROGRESS.
        ASSERT_IFNOT(operationType == ApplyContext::REDO, "{0}: Redo operation type expected during apply on Secondary", TraceId);

        transactionContextSPtr = co_await this->ApplyOnSecondaryAsync(
            applyLSN,
            transactionBase,
            applyContext,
            *metadataOperationDataCSPtr,
            dataPtr);
    }
    else
    {
        ASSERT_IFNOT(roleType == ApplyContext::RECOVERY, "{0}: Recovery role type expected", TraceId);
        ASSERT_IFNOT(operationType == ApplyContext::REDO, "{0}: Redo operation type expected", TraceId);

        transactionContextSPtr = co_await this->ApplyOnRecoveryAsync(
            applyLSN,
            transactionBase,
            applyContext,
            *metadataOperationDataCSPtr,
            dataPtr);

        co_return transactionContextSPtr;
    }

    co_return transactionContextSPtr;
}

Awaitable<OperationContext::CSPtr> StateManager::ApplyOnPrimaryAsync(
    __in FABRIC_SEQUENCE_NUMBER applyLSN,
    __in TransactionBase const & transactionBase,
    __in ApplyContext::Enum applyContext,
    __in MetadataOperationData const & metdataOperationData,
    __in OperationData const * const dataPtr)
{
    ASSERT_IFNOT(
        metdataOperationData.ApplyType == ApplyType::Insert || metdataOperationData.ApplyType == ApplyType::Delete,
        "{0}: Metadata operation apply type should be insert or delete, actual: {1}", 
        TraceId,
        metdataOperationData.ApplyType);

    Metadata::SPtr metadataSPtr = nullptr;
    bool isExist = metadataManagerSPtr_->TryGetMetadata(*metdataOperationData.StateProviderName, true, metadataSPtr);
    ASSERT_IFNOT(isExist, "{0}: Null metadata during apply on Primary", TraceId);
    ASSERT_IFNOT(
        metadataSPtr->StateProviderId == metdataOperationData.StateProviderId, 
        "{0}: Metadata state provider id: {1} not same as metadata operation data state provider id: {2}",
        TraceId,
        metadataSPtr->StateProviderId, 
        metdataOperationData.StateProviderId);

    if (metdataOperationData.ApplyType == ApplyType::Insert)
    {
        RedoOperationData const * const redoOperationDataPtr = dynamic_cast<RedoOperationData const * const>(dataPtr);
        ASSERT_IFNOT(
            redoOperationDataPtr != nullptr,
            "{0}: ApplyOnPrimaryAsync: Null RedoOperationData after cast.",
            TraceId);

        OperationContext::CSPtr result = co_await this->ApplyOnPrimaryInsertAsync(
            applyLSN, 
            transactionBase, 
            applyContext,
            metdataOperationData, 
            *redoOperationDataPtr, 
            *metadataSPtr);

        co_return result;
    }

    ASSERT_IFNOT(dataPtr == nullptr, "{0}: For remove operations, redo operation data is null", TraceId);
    OperationContext::CSPtr result = co_await this->ApplyOnPrimaryDeleteAsync(
        applyLSN, 
        transactionBase, 
        applyContext,
        metdataOperationData, 
        *metadataSPtr);

    co_return result;
}

Awaitable<OperationContext::CSPtr> StateManager::ApplyOnPrimaryInsertAsync(
    __in FABRIC_SEQUENCE_NUMBER applyLSN,
    __in TransactionBase const & transactionBase,
    __in ApplyContext::Enum applyContext,
    __in MetadataOperationData const & metdataOperationData,
    __in RedoOperationData const & redoOperationData,
    __in Metadata & metadata)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    ASSERT_IFNOT(metdataOperationData.ApplyType == ApplyType::Insert, "{0}: Must be an insert. ApplyType: {1}", TraceId, metdataOperationData.ApplyType);

    metadata.CreateLSN = applyLSN;
    co_await this->InitializeStateProvidersAsync(metadata, FALSE);

    status = NotifyStateManagerStateChanged(
        transactionBase,
        *metadata.Name,
        *metadata.StateProvider,
        metadata.ParentId,
        NotifyStateManagerChangedAction::Add);
    THROW_ON_FAILURE(status);

    StateManagerEventSource::Events->ApplyAsyncFinish(
        TracePartitionId,
        ReplicaId,
        applyLSN,
        applyContext,
        transactionBase.TransactionId,
        transactionBase.CommitSequenceNumber,
        metdataOperationData.StateProviderId,
        ToStringLiteral(*metadata.Name),
        ToStringLiteral(InsertOperation));

    co_return nullptr;
}

Awaitable<OperationContext::CSPtr> StateManager::ApplyOnPrimaryDeleteAsync(
    __in FABRIC_SEQUENCE_NUMBER applyLSN,
    __in TransactionBase const& transactionBase,
    __in ApplyContext::Enum applyContext,
    __in MetadataOperationData const& metdataOperationData,
    __in Metadata & metadata)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    ASSERT_IFNOT(metdataOperationData.ApplyType == ApplyType::Delete, "{0}: Must be a delete. ApplyType: {1}", TraceId, metdataOperationData.ApplyType);

    // Delete State Provider path.
    // close state provider. skipping it for now.
    // take the register lock along with the close lock, register lock should be taken first and then the close lock
    // register lock is needed as change role currently does not acquire lock for every state provider.
    metadata.DeleteLSN = applyLSN;
    metadataManagerSPtr_->SoftDelete(*metadata.Name, MetadataMode::DelayDelete);

    status = NotifyStateManagerStateChanged(
        transactionBase,
        *metadata.Name,
        *metadata.StateProvider,
        metadata.ParentId,
        NotifyStateManagerChangedAction::Remove);
    THROW_ON_FAILURE(status);

    StateManagerEventSource::Events->ApplyAsyncFinish(
        TracePartitionId,
        ReplicaId,
        applyLSN,
        applyContext,
        transactionBase.TransactionId,
        transactionBase.CommitSequenceNumber,
        metdataOperationData.StateProviderId,
        ToStringLiteral(*metadata.Name),
        ToStringLiteral(DeleteOperation));

    co_return nullptr;
}

Awaitable<OperationContext::CSPtr> StateManager::ApplyOnFalseProgressAsync(
    __in FABRIC_SEQUENCE_NUMBER applyLSN,
    __in TransactionBase const & transactionBase,
    __in ApplyContext::Enum applyContext,
    __in MetadataOperationData const & metdataOperationData)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    ApplyContext::Enum operationType = static_cast<ApplyContext::Enum>(applyContext & ApplyContext::OPERATION_MASK);
    ASSERT_IFNOT(operationType == ApplyContext::FALSE_PROGRESS, "{0}: operationType ({1}) == FALSE_PROGRESS", TraceId, static_cast<int>(operationType));

    ApplyType::Enum undoApplyType = metdataOperationData.ApplyType == ApplyType::Delete ? ApplyType::Insert : ApplyType::Delete;
    
    if (undoApplyType == ApplyType::Delete)
    {
        Metadata::SPtr metadataSPtr = nullptr;
        bool isExist = metadataManagerSPtr_->TryGetMetadata(*metdataOperationData.StateProviderName, false, metadataSPtr);
        ASSERT_IFNOT(
            isExist,
            "{0}: State Provider {1} is not in the active list",
            TraceId,
            metdataOperationData.StateProviderId);
        ASSERT_IFNOT(
            metadataSPtr->StateProviderId == metdataOperationData.StateProviderId,
            "{0}:  metadataSPtr->StateProviderId {1} == metdataOperationData.StateProviderId {2}",
            TraceId,
            metadataSPtr->StateProviderId,
            metdataOperationData.StateProviderId);

        metadataManagerSPtr_->SoftDelete(*metdataOperationData.StateProviderName, MetadataMode::FalseProgress);
        status = NotifyStateManagerStateChanged(
            transactionBase,
            *metadataSPtr->Name,
            *metadataSPtr->StateProvider,
            metadataSPtr->ParentId,
            NotifyStateManagerChangedAction::Remove);
        THROW_ON_FAILURE(status);

        StateManagerEventSource::Events->ApplyAsyncFinish(
            TracePartitionId,
            ReplicaId,
            applyLSN,
            applyContext,
            transactionBase.TransactionId,
            transactionBase.CommitSequenceNumber,
            metdataOperationData.StateProviderId,
            ToStringLiteral(*metadataSPtr->Name),
            ToStringLiteral(DeleteOperation));

        co_return nullptr;
    }

    Metadata::SPtr metadataSPtr = co_await metadataManagerSPtr_->ResurrectStateProviderAsync(
        metdataOperationData.StateProviderId,
        Common::TimeSpan::MaxValue,
        CancellationToken::None);

    FABRIC_REPLICA_ROLE role = GetCurrentRole();

    // Managed bug. The state provider must be promoted to Idle.
    ASSERT_IFNOT(
        role == FABRIC_REPLICA_ROLE_IDLE_SECONDARY,
        "{0}: False progress can only be called on idle secondary.",
        TraceId);
    status = co_await apiDispatcher_->ChangeRoleAsync(
        *metadataSPtr, 
        FABRIC_REPLICA_ROLE_IDLE_SECONDARY, 
        CancellationToken::None);
    THROW_ON_FAILURE(status);

    status = NotifyStateManagerStateChanged(
        transactionBase,
        *metadataSPtr->Name,
        *metadataSPtr->StateProvider,
        metadataSPtr->ParentId,
        NotifyStateManagerChangedAction::Add);
    THROW_ON_FAILURE(status);

    StateManagerEventSource::Events->ApplyAsyncFinish(
        TracePartitionId,
        ReplicaId,
        applyLSN,
        applyContext,
        transactionBase.TransactionId,
        transactionBase.CommitSequenceNumber,
        metdataOperationData.StateProviderId,
        ToStringLiteral(*metadataSPtr->Name),
        ToStringLiteral(InsertOperation));

    // call unpause api on the state provider
    co_return nullptr;
}

Awaitable<OperationContext::CSPtr> StateManager::ApplyOnSecondaryAsync(
    __in FABRIC_SEQUENCE_NUMBER applyLSN,
    __in TransactionBase const & transactionBase,
    __in ApplyContext::Enum applyContext,
    __in MetadataOperationData const & metadataOperationData,
    __in OperationData const * const dataPtr)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    ApplyContext::Enum operationType = static_cast<ApplyContext::Enum>(applyContext & ApplyContext::OPERATION_MASK);
    ASSERT_IFNOT(operationType == ApplyContext::REDO, "{0}: operation type should be redo", TraceId);

    if (metadataOperationData.ApplyType == ApplyType::Insert)
    {
        RedoOperationData::CSPtr redoOperationDataCSPtr = nullptr;
        status = RedoOperationData::Create(*PartitionedReplicaIdentifier, GetThisAllocator(), dataPtr, redoOperationDataCSPtr);
        Helper::ThrowIfNecessary(
            status,
            TracePartitionId,
            ReplicaId,
            L"ApplyOnSecondaryAsync: Create RedoOperationData.",
            Helper::StateManager);

        OperationContext::CSPtr operationContext = co_await this->ApplyOnSecondaryInsertAsync(
            applyLSN,
            transactionBase,
            applyContext,
            metadataOperationData,
            *redoOperationDataCSPtr);

        co_return operationContext;
    }
    
    OperationContext::CSPtr operationContext = co_await this->ApplyOnSecondaryDeleteAsync(
        applyLSN,
        transactionBase,
        applyContext,
        metadataOperationData);

    co_return operationContext;
}

Awaitable<OperationContext::CSPtr> StateManager::ApplyOnSecondaryInsertAsync(
    __in FABRIC_SEQUENCE_NUMBER applyLSN,
    __in TransactionBase const & transactionBase,
    __in ApplyContext::Enum applyContext,
    __in MetadataOperationData const & metadataOperationData,
    __in RedoOperationData const & redoOperationData)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    // Poor man's idempotency check.
    // #11632792: Use prepareCheckpointLSN.
    if (metadataManagerSPtr_->IsStateProviderDeletedOrStale(metadataOperationData.StateProviderId, MetadataMode::DelayDelete))
    {
        // RDBug 10631157: Add idempotent operation check
        StateManagerEventSource::Events->ApplyAsyncNoop(
            TracePartitionId,
            ReplicaId,
            applyLSN,
            applyContext,
            transactionBase.TransactionId,
            transactionBase.CommitSequenceNumber,
            metadataOperationData.StateProviderId,
            ToStringLiteral(InsertOperation),
            true);

        co_return nullptr;
    }

    if (metadataManagerSPtr_->ContainsKey(*metadataOperationData.StateProviderName))
    {
        Metadata::SPtr metadata = nullptr;
        bool isExist = metadataManagerSPtr_->TryGetMetadata(*metadataOperationData.StateProviderName, false, metadata);
        ASSERT_IFNOT(isExist, "{0}: Metadata must exist.", TraceId);
        ASSERT_IFNOT(
            metadata->StateProviderId == metadataOperationData.StateProviderId,
            "{0}: State provider id check failed: Replicated Id {1} and Active Id {2} ApplyLSN: {3} CommitLSN: {4}",
            TraceId,
            metadataOperationData.StateProviderId,
            metadata->StateProviderId,
            applyLSN,
            transactionBase.CommitSequenceNumber);

        // Managed sets commit lsn as create lsn. Native sets apply lsn as create lsn.
        // Make assert weaker here to make sure it doesn't fail during upgrade where
        // native can apply operations generated from managed and vice versa.
        ASSERT_IFNOT(
            metadata->CreateLSN == applyLSN || metadata->CreateLSN == transactionBase.CommitSequenceNumber,
            "ApplyOnSecondaryInsertAsync: {0}: Idempotency check failed: TxnID: {1}, SPID: {2}, CreateLSN: {3}, ApplyLSN: {4}, CommitLSN: {5}",
            TraceId,
            transactionBase.TransactionId,
            metadata->StateProviderId,
            metadata->CreateLSN,
            applyLSN,
            transactionBase.CommitSequenceNumber);

        StateManagerEventSource::Events->ApplyAsyncNoop(
            TracePartitionId,
            ReplicaId,
            applyLSN,
            applyContext,
            transactionBase.TransactionId,
            transactionBase.CommitSequenceNumber,
            metadataOperationData.StateProviderId,
            ToStringLiteral(InsertOperation),
            false);

        co_return nullptr;
    }

    StateManagerLockContext::SPtr lockContext = nullptr;
    status = co_await metadataManagerSPtr_->LockForWriteAsync(
        *metadataOperationData.StateProviderName,
        metadataOperationData.StateProviderId,
        transactionBase,
        Common::TimeSpan::MaxValue,
        CancellationToken::None,
        lockContext);
    THROW_ON_FAILURE(status);

    StateManagerTransactionContext::SPtr transactionContextSPtr = nullptr;
    status = StateManagerTransactionContext::Create(transactionBase.TransactionId, *lockContext, OperationType::Add, GetThisAllocator(), transactionContextSPtr);

    // If TransactionContext Create failed, release the write lock first then throw the exception, otherwise lock leak
    // #9019333, Add tests for the case, TransactionContext Create failed after getting the read/write lock
    ReleaseLockAndThrowIfOnFailure(transactionBase.TransactionId, *lockContext, status);

    // Resurrection optimization right after copy
    // Check if it is in the stale state. This check is needed only during build.
    Metadata::SPtr metadata;
    if (metadataManagerSPtr_->IsStateProviderDeletedOrStale(metadataOperationData.StateProviderId, MetadataMode::FalseProgress))
    {
        // New Optimization: In this code path we resurrect the stale state provider instead of re-creating it.
        // Managed bug: State Provider life-time: Close not called.
        // In managed the stale state provider is not resurrected and instead a new state provider is created.
        // The problem is that the stale state provider is not closed.
        bool isRemoved = metadataManagerSPtr_->TryRemoveDeleted(metadataOperationData.StateProviderId, metadata);
        ASSERT_IFNOT(
            isRemoved,
            "{0}: Metadata not found in softdeleted map. SPID: {1} ApplyLSN: {2} CommitLSN: {3}",
            TraceId,
            metadataOperationData.StateProviderId,
            applyLSN,
            transactionBase.CommitSequenceNumber);

        metadata->MetadataMode = MetadataMode::Active;
        bool result = metadataManagerSPtr_->TryAdd(*metadata->Name, *metadata);
        ASSERT_IFNOT(
            result,
            "{0}: Metadata already exists in metadata manager. SPID: {1} ApplyLSN: {2} CommitLSN: {3}",
            TraceId,
            metadataOperationData.StateProviderId,
            applyLSN,
            transactionBase.CommitSequenceNumber);

        // As part of this optimization, we still need to tell the replica that its existing not checkpointed state needs to be discarded.
        // Note: We could pass a bool/enum to indicate that this is a resurrection.
        status = co_await apiDispatcher_->BeginSettingCurrentStateAsync(*metadata);
        THROW_ON_FAILURE(status);

        status = co_await apiDispatcher_->EndSettingCurrentStateAsync(*metadata, CancellationToken::None);
        THROW_ON_FAILURE(status);

        FABRIC_REPLICA_ROLE role = GetCurrentRole();

        if (role == FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY)
        {
            status = co_await apiDispatcher_->ChangeRoleAsync(
                *metadata, 
                FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, 
                CancellationToken::None);
            THROW_ON_FAILURE(status);
        }
    }
    else
    {
        IStateProvider2::SPtr stateProvider = nullptr;
        status = apiDispatcher_->CreateStateProvider(
            *metadataOperationData.StateProviderName,
            metadataOperationData.StateProviderId,
            *redoOperationData.TypeName,
            redoOperationData.InitializationParameters.RawPtr(),
            stateProvider);
        Helper::ThrowIfNecessary(
            status,
            TracePartitionId,
            ReplicaId,
            L"ApplyOnSecondaryInsertAsync: CreateStateProvider",
            Helper::StateManager);

        status = Metadata::Create(
            *metadataOperationData.StateProviderName,
            *redoOperationData.TypeName,
            *stateProvider,
            redoOperationData.InitializationParameters.RawPtr(),
            metadataOperationData.StateProviderId,
            redoOperationData.ParentId,
            applyLSN,
            FABRIC_INVALID_SEQUENCE_NUMBER,
            MetadataMode::Active,
            false,
            GetThisAllocator(),
            metadata);
        Helper::ThrowIfNecessary(
            status,
            TracePartitionId,
            ReplicaId,
            L"ApplyOnSecondaryInsertAsync: Create Metadata.",
            Helper::StateManager);

        co_await this->InitializeStateProvidersAsync(*metadata, true);
    }

    ASSERT_IF(
        metadata == nullptr,
        "{0}: Metadata cannot be nullptr. SPID: {1} ApplyLSN: {2} CommitLSN: {3}",
        TraceId,
        metadataOperationData.StateProviderId,
        applyLSN,
        transactionBase.CommitSequenceNumber);

    UpdateLastStateProviderId(metadata->StateProviderId);

    status = NotifyStateManagerStateChanged(
        transactionBase,
        *metadata->Name,
        *metadata->StateProvider,
        metadata->ParentId,
        NotifyStateManagerChangedAction::Add);
    THROW_ON_FAILURE(status);

    StateManagerEventSource::Events->ApplyAsyncFinish(
        TracePartitionId,
        ReplicaId,
        applyLSN,
        applyContext,
        transactionBase.TransactionId,
        transactionBase.CommitSequenceNumber,
        metadataOperationData.StateProviderId,
        ToStringLiteral(*metadata->Name),
        ToStringLiteral(InsertOperation));

    co_return transactionContextSPtr.RawPtr();
}

Awaitable<OperationContext::CSPtr> StateManager::ApplyOnSecondaryDeleteAsync(
    __in FABRIC_SEQUENCE_NUMBER applyLSN,
    __in TransactionBase const & transactionBase,
    __in ApplyContext::Enum applyContext,
    __in MetadataOperationData const & metadataOperationData)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    ASSERT_IFNOT(
        metadataOperationData.ApplyType == ApplyType::Delete,
        "{0}: ApplyOnSecondaryDeleteAsync: ApplyType {1} must have been Delete. SPID: {2} ApplyLSN: {3} CommitLSN: {4}",
        TraceId,
        metadataOperationData.ApplyType,
        metadataOperationData.StateProviderId,
        applyLSN,
        transactionBase.CommitSequenceNumber);

    // duplicate keys can come in during copy.
    if (metadataManagerSPtr_->IsStateProviderDeletedOrStale(metadataOperationData.StateProviderId, MetadataMode::DelayDelete))
    {
        Metadata::SPtr metadata;
        bool isExist = metadataManagerSPtr_->TryGetDeletedMetadata(metadataOperationData.StateProviderId, metadata);
        ASSERT_IFNOT(
            isExist,
            "{0}: Metadata cannot be found in soft deleted map. SPID: {1} ApplyLSN: {2} CommitLSN: {3}",
            TraceId,
            metadataOperationData.StateProviderId,
            applyLSN,
            transactionBase.CommitSequenceNumber);

        // Managed sets commit lsn as delete lsn. Native sets apply lsn as delete lsn.
        // Make assert weaker here to make sure it doesn't fail during upgrade where
        // native can apply operations generated from managed and vice versa.
        ASSERT_IFNOT(
            metadata->DeleteLSN == applyLSN || metadata->DeleteLSN == transactionBase.CommitSequenceNumber,
            "ApplyOnSecondaryDeleteAsync: {0}: Metadata cannot be found in soft deleted map. TxnID: {1}, SPID: {2}, DeleteLSN: {3}, ApplyLSN: {4}, CommitLSN: {5}",
            TraceId,
            transactionBase.TransactionId,
            metadataOperationData.StateProviderId,
            metadata->DeleteLSN,
            applyLSN,
            transactionBase.CommitSequenceNumber);

        StateManagerEventSource::Events->ApplyAsyncNoop(
            TracePartitionId,
            ReplicaId,
            applyLSN,
            applyContext,
            transactionBase.TransactionId,
            transactionBase.CommitSequenceNumber,
            metadataOperationData.StateProviderId,
            ToStringLiteral(DeleteOperation),
            true);

        co_return nullptr;
    }

    // Divergence from managed: If a delete call came in, then it must have either copied with Insert | Delete
    // Or Insert must have been applied as part of the copy log replay.
    // This is due to the SafeLSN guarantee.
    ASSERT_IF(
        metadataManagerSPtr_->IsStateProviderDeletedOrStale(metadataOperationData.StateProviderId, MetadataMode::FalseProgress),
        "{0}: ApplyOnSecondaryDeleteAsync: state provider cannot be stale. SPID: {1} ApplyLSN: {2} CommitLSN: {3}",
        TraceId,
        metadataOperationData.StateProviderId,
        applyLSN,
        transactionBase.CommitSequenceNumber);

    Metadata::SPtr metadataSPtr = nullptr;
    bool isExist = metadataManagerSPtr_->TryGetMetadata(*metadataOperationData.StateProviderName, false, metadataSPtr);
    ASSERT_IFNOT(
        isExist,
        "{0}: Metadata cannot be found. SPID: {1} ApplyLSN: {2} CommitLSN: {3}",
        TraceId,
        metadataOperationData.StateProviderId,
        applyLSN,
        transactionBase.CommitSequenceNumber);

    StateManagerLockContext::SPtr lockContext = nullptr;
    status = co_await metadataManagerSPtr_->LockForWriteAsync(
        *metadataOperationData.StateProviderName,
        metadataOperationData.StateProviderId,
        transactionBase,
        Common::TimeSpan::MaxValue,
        CancellationToken::None,
        lockContext);
    THROW_ON_FAILURE(status);

    StateManagerTransactionContext::SPtr transactionContextSPtr = nullptr;
    status = StateManagerTransactionContext::Create(
        transactionBase.TransactionId, 
        *lockContext,
        OperationType::Remove,
        GetThisAllocator(), 
        transactionContextSPtr);

    // If TransactionContext Create failed, release the write lock first then throw the exception, otherwise lock leak
    // #9019333, Add tests for the case, TransactionContext Create failed after getting the read/write lock
    ReleaseLockAndThrowIfOnFailure(transactionBase.TransactionId, *lockContext, status);

    metadataSPtr->DeleteLSN = applyLSN;
    metadataManagerSPtr_->SoftDelete(*metadataSPtr->Name, MetadataMode::DelayDelete);

    status = NotifyStateManagerStateChanged(
        transactionBase,
        *metadataSPtr->Name,
        *metadataSPtr->StateProvider,
        metadataSPtr->ParentId,
        NotifyStateManagerChangedAction::Remove);
    THROW_ON_FAILURE(status);

    StateManagerEventSource::Events->ApplyAsyncFinish(
        TracePartitionId,
        ReplicaId,
        applyLSN,
        applyContext,
        transactionBase.TransactionId,
        transactionBase.CommitSequenceNumber,
        metadataOperationData.StateProviderId,
        ToStringLiteral(*metadataSPtr->Name),
        ToStringLiteral(DeleteOperation));

    co_return transactionContextSPtr.RawPtr();
}

Awaitable<OperationContext::CSPtr> StateManager::ApplyOnRecoveryAsync(
    __in FABRIC_SEQUENCE_NUMBER applyLSN,
    __in TransactionBase const & transactionBase,
    __in ApplyContext::Enum applyContext,
    __in MetadataOperationData const & metadataOperationData,
    __in OperationData const * const dataPtr)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    try
    {
        if (metadataOperationData.ApplyType == ApplyType::Insert)
        {
            RedoOperationData::CSPtr redoOperationDataCSPtr = nullptr;
            status = RedoOperationData::Create(*PartitionedReplicaIdentifier, GetThisAllocator(), dataPtr, redoOperationDataCSPtr);
            Helper::ThrowIfNecessary(
                status,
                TracePartitionId,
                ReplicaId,
                L"ApplyOnRecoveryAsync: Create RedoOperationData.",
                Helper::StateManager);
            
            OperationContext::CSPtr result =  co_await this->ApplyOnRecoveryInsertAsync(
                applyLSN, 
                transactionBase, 
                applyContext,
                metadataOperationData, 
                *redoOperationDataCSPtr);
            co_return result;
        }

        // #10279752: Redo Operation should be nullptr here, since for applying the delete operation, we don't need it.
        // After managed code fix should add this assert back.
        //ASSERT_IFNOT(
        //    dataPtr == nullptr,
        //    "{0}: StateManager.ApplyOnRecoverAsync. dataPtr must be nullptr",
        //    TraceId);

        ASSERT_IFNOT(
            metadataOperationData.ApplyType == ApplyType::Delete,
            "{0}: StateManager.ApplyOnRecoverAsync. expected apply type is delete but the apply type is {1}",
            TraceId,
            metadataOperationData.ApplyType);

        OperationContext::CSPtr result = co_await this->ApplyOnRecoveryRemoveAsync(
            applyLSN, 
            transactionBase, 
            applyContext,
            metadataOperationData);
        co_return result;
    }
    catch( Exception & exception)
    {
        TraceException(L"ApplyOnRecoveryAsync", exception);
        throw;
    }
}

Awaitable<OperationContext::CSPtr> StateManager::ApplyOnRecoveryInsertAsync(
    __in FABRIC_SEQUENCE_NUMBER applyLSN,
    __in TransactionBase const& transactionBase,
    __in ApplyContext::Enum applyContext,
    __in MetadataOperationData const& metadataOperationData,
    __in RedoOperationData const& redoOperationData)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    // if it is already present in delete list, ignore it.
    if (metadataManagerSPtr_->IsStateProviderDeletedOrStale(metadataOperationData.StateProviderId, MetadataMode::DelayDelete))
    {
        StateManagerEventSource::Events->ApplyAsyncNoop(
            TracePartitionId,
            ReplicaId,
            applyLSN,
            applyContext,
            transactionBase.TransactionId,
            transactionBase.CommitSequenceNumber,
            metadataOperationData.StateProviderId,
            ToStringLiteral(InsertOperation),
            true);

        co_return nullptr;
    }

    Metadata::SPtr parentMetadataSPtr = nullptr;
    bool isExist = metadataManagerSPtr_->TryGetMetadata(*metadataOperationData.StateProviderName, false, parentMetadataSPtr);
    if (isExist)
    {
        // duplicate keys can come in during recovery.
        ASSERT_IFNOT(
            parentMetadataSPtr->StateProviderId == metadataOperationData.StateProviderId,
            "{0}: stateprovider id present in active state is {1}, obtained through replication is {2}",
            TraceId,
            parentMetadataSPtr->StateProviderId,
            metadataOperationData.StateProviderId);

        // Managed sets commit lsn as create lsn. Native sets apply lsn as create lsn.
        // Make assert weaker here to make sure it doesn't fail during upgrade where
        // native can apply operations generated from managed and vice versa.
        ASSERT_IFNOT(
            parentMetadataSPtr->CreateLSN == applyLSN || parentMetadataSPtr->CreateLSN == transactionBase.CommitSequenceNumber,
            "ApplyOnRecoveryInsertAsync: {0}: Idempotency check failed: TxnID: {1}, SPID: {2}, CreateLSN: {3}, ApplyLSN: {4}, CommitLSN: {5}",
            TraceId,
            transactionBase.TransactionId,
            metadataOperationData.StateProviderId,
            parentMetadataSPtr->CreateLSN,
            applyLSN,
            transactionBase.CommitSequenceNumber);

        StateManagerEventSource::Events->ApplyAsyncNoop(
            TracePartitionId,
            ReplicaId,
            applyLSN,
            applyContext,
            transactionBase.TransactionId,
            transactionBase.CommitSequenceNumber,
            metadataOperationData.StateProviderId,
            ToStringLiteral(InsertOperation),
            false);

        co_return nullptr;
    }

    StateManagerLockContext::SPtr lockContext = nullptr;
    status = co_await metadataManagerSPtr_->LockForWriteAsync(
        *metadataOperationData.StateProviderName,
        metadataOperationData.StateProviderId,
        transactionBase,
        Common::TimeSpan::MaxValue,
        CancellationToken::None,
        lockContext);
    THROW_ON_FAILURE(status);

    StateManagerTransactionContext::SPtr transactionContext = nullptr;
    status = StateManagerTransactionContext::Create(
        transactionBase.TransactionId,
        *lockContext,
        OperationType::Add,
        GetThisAllocator(),
        transactionContext);

    // If TransactionContext Create failed, release the write lock first then throw the exception, otherwise lock leak
    // #9019333, Add tests for the case, TransactionContext Create failed after getting the read/write lock
    ReleaseLockAndThrowIfOnFailure(transactionBase.TransactionId, *lockContext, status);

    IStateProvider2::SPtr stateProvider = nullptr;
    status = apiDispatcher_->CreateStateProvider(
        *metadataOperationData.StateProviderName,
        metadataOperationData.StateProviderId,
        *redoOperationData.TypeName,
        redoOperationData.InitializationParameters.RawPtr(),
        stateProvider);
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"ApplyOnRecoveryInsertAsync: Create Metadata.",
        Helper::StateManager);

    status = Metadata::Create(
        *metadataOperationData.StateProviderName,
        *redoOperationData.TypeName,
        *stateProvider,
        redoOperationData.InitializationParameters.RawPtr(),
        metadataOperationData.StateProviderId,
        redoOperationData.ParentId,
        applyLSN,
        -1,
        MetadataMode::Active,
        false,
        GetThisAllocator(),
        parentMetadataSPtr);
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"ApplyOnRecoveryInsertAsync: Create Metadata.",
        Helper::StateManager);

    KSharedArray<Metadata::SPtr>::SPtr stateProviderList = InitializeStateProvidersInOrder(*parentMetadataSPtr);

    // MCoskun: Note that state manager send the notification once the state provider is added but before it is opened.
    // This means that the state provider needs to allow operations that it expected notification user to use,
    // before it is opened.
    // We could also send this notification after Opening the state provider but that means that they cannot register to
    // notifications that might want to be fired during open.
    //
    // Note that we only send notification for parent state providers today.
    status = NotifyStateManagerStateChanged(
        transactionBase,
        *parentMetadataSPtr->Name,
        *parentMetadataSPtr->StateProvider,
        parentMetadataSPtr->ParentId,
        NotifyStateManagerChangedAction::Add);
    THROW_ON_FAILURE(status);

    // Invariant: If the state provider is a child, the stateProviderList should be nullptr, and 
    // thus we skip the OpenAsync and RecoverCheckpointAsync call and just return.
    if (stateProviderList == nullptr)
    {
        co_return transactionContext.RawPtr();
    }

    // Children will always initialize themselves before parent based on the name reverse order. 
    // Only parent call OpenAsync and RecoverCheckpointAsync on itself and all its children.
    for (Metadata::SPtr metadataSPtr : *stateProviderList)
    {
        // Note: If open fails, there is nothing to do but to report fault and let the exception bubble up.
        status = co_await apiDispatcher_->OpenAsync(*metadataSPtr, CancellationToken::None);
        THROW_ON_FAILURE(status);

        // #10019116: We must add a metadata to metadata manager only after opening it. 
        // Otherwise if Open throws we would call close on an unopened state provider.
        bool isAdded = metadataManagerSPtr_->TryAdd(*metadataSPtr->Name, *metadataSPtr);
        ASSERT_IFNOT(
            isAdded,
            "{0}: Add has failed because the key {1} is already present.",
            TraceId,
            ToStringLiteral(*metadataSPtr->Name));

        // Invariant: If a state provider is opened, it will terminate by either being closed / aborted or the process will go down.
        // In the case of recoverCheckpointAsync failing, all we need to do is report fault.
        // This is because the state providers already in metadataManagerSPtr.
        // Reporting fault will end up closing this state provider.
        status = co_await apiDispatcher_->RecoverCheckpointAsync(*metadataSPtr, CancellationToken::None);
        Helper::ThrowIfNecessary(
            status,
            TracePartitionId,
            ReplicaId,
            L"ApplyOnRecoveryInsertAsync: RecoverCheckpointAsync.",
            Helper::StateManager);

        UpdateLastStateProviderId(metadataSPtr->StateProviderId);

        StateManagerEventSource::Events->ApplyAsyncFinish(
            TracePartitionId,
            ReplicaId,
            applyLSN,
            applyContext,
            transactionBase.TransactionId,
            transactionBase.CommitSequenceNumber,
            metadataSPtr->StateProviderId,
            ToStringLiteral(*metadataSPtr->Name),
            ToStringLiteral(InsertOperation));
    }

    co_return transactionContext.RawPtr();
}

Awaitable<OperationContext::CSPtr> StateManager::ApplyOnRecoveryRemoveAsync(
    __in FABRIC_SEQUENCE_NUMBER applyLSN,
    __in TransactionBase const & transactionBase,
    __in ApplyContext::Enum applyContext,
    __in MetadataOperationData const & metdataOperationData)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    // if it is already present in delete list, ignore it.
    Metadata::SPtr metadataSPtr;
    bool isDeleted = metadataManagerSPtr_->TryGetDeletedMetadata(metdataOperationData.StateProviderId, metadataSPtr);
    if (isDeleted)
    {
        ASSERT_IFNOT(metadataSPtr != nullptr, "{0}: Metadata from deleted cannot be nullptr.", TraceId);

        // Managed sets commit lsn as delete lsn. Native sets apply lsn as delete lsn.
        // Make assert weaker here to make sure it doesn't fail during upgrade where
        // native can apply operations generated from managed and vice versa.
        ASSERT_IFNOT(
            metadataSPtr->DeleteLSN == applyLSN || metadataSPtr->DeleteLSN == transactionBase.CommitSequenceNumber,
            "ApplyOnRecoveryRemoveAsync: {0}: Idempotency check failed: TxnID: {1}, SPID: {2}, DeleteLSN: {3}, ApplyLSN: {4}, CommitLSN: {5}",
            TraceId,
            transactionBase.TransactionId,
            metdataOperationData.StateProviderId,
            metadataSPtr->DeleteLSN,
            applyLSN,
            transactionBase.CommitSequenceNumber);

        StateManagerEventSource::Events->ApplyAsyncNoop(
            TracePartitionId,
            ReplicaId,
            applyLSN,
            applyContext,
            transactionBase.TransactionId,
            transactionBase.CommitSequenceNumber,
            metdataOperationData.StateProviderId,
            ToStringLiteral(DeleteOperation),
            true);

        co_return nullptr;
    }

    bool isExist = metadataManagerSPtr_->TryGetMetadata(*metdataOperationData.StateProviderName, false, metadataSPtr);
    ASSERT_IFNOT(
        isExist,
        "{0}: State provider must exist. SPid: {1} Name: {2}",
        TraceId,
        metdataOperationData.StateProviderId,
        ToStringLiteral(*metdataOperationData.StateProviderName));

    StateManagerLockContext::SPtr lockContext = nullptr;
    status = co_await metadataManagerSPtr_->LockForWriteAsync(
        *metdataOperationData.StateProviderName,
        metdataOperationData.StateProviderId,
        transactionBase,
        Common::TimeSpan::MaxValue,
        CancellationToken::None,
        lockContext);
    THROW_ON_FAILURE(status);

    StateManagerTransactionContext::SPtr transactionContext = nullptr;
    status = StateManagerTransactionContext::Create(transactionBase.TransactionId, *lockContext, OperationType::Remove, GetThisAllocator(), transactionContext);

    // If TransactionContext Create failed, release the write lock first then throw the exception, otherwise lock leak
    // #9019333, Add tests for the case, TransactionContext Create failed after getting the read/write lock
    ReleaseLockAndThrowIfOnFailure(transactionBase.TransactionId, *lockContext, status);

    metadataSPtr->DeleteLSN = applyLSN;
    metadataManagerSPtr_->SoftDelete(*metadataSPtr->Name, MetadataMode::DelayDelete);

    status = NotifyStateManagerStateChanged(
        transactionBase,
        *metadataSPtr->Name,
        *metadataSPtr->StateProvider,
        metadataSPtr->ParentId,
        NotifyStateManagerChangedAction::Remove);
    THROW_ON_FAILURE(status);

    StateManagerEventSource::Events->ApplyAsyncFinish(
        TracePartitionId,
        ReplicaId,
        applyLSN,
        applyContext,
        transactionBase.TransactionId,
        transactionBase.CommitSequenceNumber,
        metadataSPtr->StateProviderId,
        ToStringLiteral(*metadataSPtr->Name),
        ToStringLiteral(DeleteOperation));

    co_return transactionContext.RawPtr();
}

NTSTATUS StateManager::UnlockOnLocalState(__in NamedOperationContext const & namedOperationContext) noexcept
{
    const StateManagerTransactionContext& transactionContext = dynamic_cast<StateManagerTransactionContext const &>(*namedOperationContext.OperationContextCSPtr);

    ASSERT_IFNOT(transactionContext.TransactionId != Constants::InvalidTransactionId , "{0}: Invalid xact id during unlock on local state", TraceId);

    StateManagerEventSource::Events->UnlockOnLocalState(
        TracePartitionId,
        ReplicaId,
        transactionContext.TransactionId);

    NTSTATUS status = metadataManagerSPtr_->Unlock(transactionContext);
    if (NT_SUCCESS(status) == false)
    {
        TRACE_ERRORS(
            status,
            TracePartitionId,
            ReplicaId,
            Helper::StateManager,
            "UnlockOnLocalState: MetadataManager::Unlock. TxnId: {0}",
            transactionContext.TransactionId);
    }

    return status;
}

FABRIC_REPLICA_ROLE StateManager::GetCurrentRole() const noexcept
{
    roleLock_.AcquireShared();
    KFinally([&] {roleLock_.ReleaseShared(); });

    return role_;
}

Awaitable<OperationContext::CSPtr> StateManager::ApplyAsync(
    __in FABRIC_SEQUENCE_NUMBER applyLSN,
    __in TransactionBase const & transactionBase,
    __in ApplyContext::Enum applyContext,
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in_opt OperationData const * const metadataPtr,
    __in_opt OperationData const * const dataPtr)
{
    ApiEntry();

    ASSERT_IFNOT(stateProviderId != EmptyStateProviderId, "{0}: Empty state provider during apply, StateProviderId: {1}.", TraceId, stateProviderId);

    OperationContext::CSPtr result = nullptr;
    if (stateProviderId == StateManagerId)
    {
        // Apply on SM
        result = co_await this->ApplyOnLocalStateAsync(
            applyLSN,
            transactionBase,
            applyContext,
            metadataPtr,
            dataPtr);
    }
    else
    {
        if (metadataManagerSPtr_->IsStateProviderDeletedOrStale(stateProviderId, MetadataMode::DelayDelete))
        {
            StateManagerEventSource::Events->ApplyOnStaleStateProvider(
                TracePartitionId,
                ReplicaId,
                applyLSN,
                transactionBase.TransactionId,
                stateProviderId);
        }
        else
        {
            // The method below asserts that the state provider is present.
            Metadata::SPtr metadataSPtr = nullptr;
            bool isExist = metadataManagerSPtr_->TryGetMetadata(stateProviderId, metadataSPtr);
            ASSERT_IFNOT(
                isExist,
                "{0}: MetadataSPtr cannot be nullptr. SPID {1}",
                TraceId,
                stateProviderId);

            result = co_await metadataSPtr->StateProvider->ApplyAsync(applyLSN, transactionBase, applyContext, metadataPtr, dataPtr);
        }
    }

    if (result == nullptr)
    {
        co_return result;
    }

    NamedOperationContext::SPtr operationContextSPtr;
    NTSTATUS status = NamedOperationContext::Create(result.RawPtr(), stateProviderId, GetThisAllocator(), operationContextSPtr);
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"ApplyAsync: Create NamedOperationContext.",
        Helper::StateManager);

    co_return operationContextSPtr.RawPtr();
}

VOID StateManager::OnServiceOpen()
{
    OnServiceOpenAsync();
}

VOID StateManager::OnServiceClose()
{
    OnServiceCloseAsync();
}

NTSTATUS StateManager::CreateInternalEnumerator(
    __in bool parentsOnly,
    __out Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, IStateProvider2::SPtr>>::SPtr & outEnumerator) noexcept
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    ConcurrentDictionary<KUri::CSPtr, IStateProvider2::SPtr>::SPtr result;

    status = ConcurrentDictionary<KUri::CSPtr, IStateProvider2::SPtr>::Create(GetThisAllocator(), result);
    RETURN_ON_FAILURE(status);

    Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, Metadata::SPtr>>::SPtr metadataEnumerator;

    try
    {
        // MetadataManager::GetMetadataCollection will trace if it fails
        metadataEnumerator = metadataManagerSPtr_->GetMetadataCollection();
    }
    catch (Exception & exception)
    {
        status = exception.GetStatus();
        return status;
    }

    try
    {

        if (parentsOnly)
        {
            while (metadataEnumerator->MoveNext())
            {
                const Metadata::SPtr metadata(metadataEnumerator->Current().Value);

                if (metadata->ParentId == EmptyStateProviderId)
                {
                    IStateProvider2::SPtr temp = metadata->StateProvider;

                    // ConcurrentDictionary::TryAdd may throw exception.
                    bool succeeded = result->TryAdd((metadataEnumerator->Current()).Key, metadata->StateProvider);
                    ASSERT_IFNOT(succeeded, "Failed to add key and value. Key already exists.");
                }
            }
        }
        else
        {
            while (metadataEnumerator->MoveNext())
            {
                IStateProvider2::SPtr temp = metadataEnumerator->Current().Value->StateProvider;

                // ConcurrentDictionary::TryAdd may throw exception.
                bool succeeded = result->TryAdd((metadataEnumerator->Current()).Key, temp);
                ASSERT_IFNOT(succeeded, "Failed to add key and value. Key already exists.");
            }
        }
    }
    catch (Exception & exception)
    {
        status = exception.GetStatus();
        StateManagerEventSource::Events->Error(
            TracePartitionId,
            ReplicaId,
            L"CreateInternalEnumerator: ConcurrentDictionary::Add.",
            status);
        return status;
    }

    outEnumerator = result->GetEnumerator();
    return status;
}

KString::CSPtr StateManager::CreateReplicaFolderPath(
    __in LPCWSTR workFolder,
    __in KGuid partitionId,
    __in FABRIC_REPLICA_ID replicaId,
    __in KAllocator & allocator)
{
    ASSERT_IF(workFolder == nullptr, "{0}:{1} workFolder cannot be nullptr", Common::Guid(partitionId), replicaId);
    ASSERT_IF(wcslen(workFolder) == 0, "{0}:{1} workFolder cannot be empty", Common::Guid(partitionId), replicaId);

    KString::SPtr replicaFolderPath = KPath::CreatePath(workFolder, allocator);

    KString::SPtr folderName = nullptr;
    NTSTATUS status = KString::Create(folderName, allocator);
    Helper::ThrowIfNecessary(
        status,
        Common::Guid(partitionId),
        replicaId,
        L"CreateReplicaFolderPath: Create folderName.",
        Helper::StateManager);

    Common::Guid partitionIdGuid(partitionId);
    wstring partitionIdString = partitionIdGuid.ToString('N');

    BOOLEAN boolStatus = folderName->Concat(partitionIdString.c_str());
    ASSERT_IFNOT(boolStatus == TRUE, "{0}:{1} CreateWorkDirectory concat failed.", Common::Guid(partitionId), replicaId);

    // This can also be a separate folder. However, then we would need to clean both.
    boolStatus = folderName->Concat(L"_");
    ASSERT_IFNOT(boolStatus == TRUE, "{0}:{1} CreateWorkDirectory concat failed.", Common::Guid(partitionId), replicaId);

    KLocalString<20> replicaIdString;
    boolStatus = replicaIdString.FromLONGLONG(replicaId);
    ASSERT_IFNOT(boolStatus == TRUE, "{0}:{1} CreateWorkDirectory FromLONGLONG failed.", Common::Guid(partitionId), replicaId);

    boolStatus = folderName->Concat(replicaIdString);
    ASSERT_IFNOT(boolStatus == TRUE, "{0}:{1} CreateWorkDirectory concat failed.", Common::Guid(partitionId), replicaId);

    KPath::CombineInPlace(*replicaFolderPath, *folderName);
    return replicaFolderPath.RawPtr();
}

Task StateManager::OnServiceOpenAsync() noexcept
{
    NTSTATUS status = STATUS_SUCCESS;

    StateManagerEventSource::Events->ServiceOpen(
        TracePartitionId,
        ReplicaId,
        openParametersSPtr_->CompleteCheckpoint,
        openParametersSPtr_->CleanupRestore);

    SetDeferredCloseBehavior();
    
    if (hasPersistedState_ == false)
    {
        // Nothing to recover
        openParametersSPtr_ = nullptr;
        CompleteOpen(status);
        co_return;
    }

    // Recover State Manager Checkpoint.
    try
    {
        if (openParametersSPtr_->CompleteCheckpoint)
        {
            co_await checkpointManagerSPtr_->CompleteCheckpointAsync(*metadataManagerSPtr_, CancellationToken::None);
        }

        if (openParametersSPtr_->CleanupRestore)
        {
            co_await this->CleanUpIncompleteRestoreAsync(CancellationToken::None);
        }

        co_await this->RecoverStateManagerCheckpointAsync(CancellationToken::None);
    }
    catch (Exception & exception)
    {
        TraceException(L"OpenAsync: State Manager recovery", exception);
        openParametersSPtr_ = nullptr;
        CompleteOpen(exception.GetStatus());
        co_return;
    }

    // Open the state providers.
    status = co_await this->OpenStateProvidersAsync(CancellationToken::None);
    if (NT_SUCCESS(status) == false)
    {
        TraceError(L"OpenAsync: State Provider open.", status);
        openParametersSPtr_ = nullptr;
        CompleteOpen(status);
        co_return;
    }
    
    // Complete Checkpoint from old instance if necessary.
    if (openParametersSPtr_->CompleteCheckpoint)
    {
        status = co_await this->CompleteCheckpointOnStateProvidersAsync(CancellationToken::None);
        if (NT_SUCCESS(status) == false)
        {
            TraceError(L"OnServiceOpenAsync: ApiDispatcher::CompleteCheckpointAsync.", status);
            AbortStateProviders();
            openParametersSPtr_ = nullptr;
            CompleteOpen(status);
            co_return;
        }
    }

    // Recover the state providers.
    status = co_await this->RecoverStateProvidersAsync(CancellationToken::None);
    if (NT_SUCCESS(status) == false)
    {
        TraceError(L"OnServiceOpenAsync: ApiDispatcher::RecoverCheckpointAsync.", status);
        AbortStateProviders();
        openParametersSPtr_ = nullptr;
        CompleteOpen(status);
        co_return;
    }

    StateManagerEventSource::Events->ServiceOpenCompleted(
        TracePartitionId,
        ReplicaId);

    openParametersSPtr_ = nullptr;
    CompleteOpen(status);
    co_return;
}

Task StateManager::OnServiceCloseAsync() noexcept
{
    StateManagerEventSource::Events->ServiceCloseStarted(
        TracePartitionId,
        ReplicaId);
    
    NTSTATUS status = co_await this->CloseStateProvidersAsync(CancellationToken::None);
    ASSERT_IFNOT(
        NT_SUCCESS(status),
        "{0}: CloseStateProvidersAsync has failed with {1}",
        TraceId,
        status);

    try
    {
        // Dispose MetadataManager: Clear all the LockContext in MetadataManager
        // Using try-catch block since ReaderWriterAsyncLock::Close() may throw exception.
        metadataManagerSPtr_->Dispose();
    }
    catch (Exception const & exception)
    {
        // #10730828:Get the status and put in the CompleteClose.
        TraceException(L"OnServiceCloseAsync: MetadataManager Dispose", exception);
        CompleteOpen(exception.GetStatus());
        co_return;
    }

    StateManagerEventSource::Events->ServiceCloseCompleted(
        TracePartitionId,
        ReplicaId);

    CompleteClose(STATUS_SUCCESS);
    co_return;
}

Awaitable<void> StateManager::CleanUpIncompleteRestoreAsync(__in CancellationToken const & cancellationToken)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    // Recover StateManager's checkpoint. 
    // This also creates and initializes the state providers.
    co_await this->RecoverStateManagerCheckpointAsync(CancellationToken::None);

    // Now that we have recovered all state providers, we need to remote their state and close them.
    status = co_await this->OpenStateProvidersAsync(cancellationToken);
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"CleanUpIncompleteRestoreAsync: OpenStateProvidersAsync.",
        Helper::StateManager);

    status = co_await this->ChangeRoleOnStateProvidersAsync(FABRIC_REPLICA_ROLE_NONE, cancellationToken);
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"CleanUpIncompleteRestoreAsync: ChangeRoleOnStateProvidersAsync.",
        Helper::StateManager);

    status = co_await this->RemoveStateOnStateProvidersAsync(cancellationToken);
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"CleanUpIncompleteRestoreAsync: RemoveStateOnStateProvidersAsync.",
        Helper::StateManager);

    status = co_await this->CloseStateProvidersAsync(cancellationToken);
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"CleanUpIncompleteRestoreAsync: CloseStateProvidersAsync.",
        Helper::StateManager);

    // Now that state provider's have been cleaned up, we need to clean up the State Manager's state.
    checkpointManagerSPtr_->Clean();
    metadataManagerSPtr_->Dispose();

    // Re-initialize
    status = MetadataManager::Create(*PartitionedReplicaIdentifier, GetThisAllocator(), metadataManagerSPtr_);
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"CleanUpIncompleteRestoreAsync: Create MetadataManager.",
        Helper::StateManager);
    co_return;
}

Awaitable<NTSTATUS> StateManager::OpenStateProvidersAsync(
    __in CancellationToken const & cancellationToken) noexcept
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    StateManagerEventSource::Events->OpenSP(TracePartitionId, ReplicaId);

#ifdef DBG
    KSharedArray<Metadata::CSPtr>::SPtr transientStateProviders = nullptr;
    status = metadataManagerSPtr_->GetInMemoryMetadataArray(StateProviderFilterMode::Transient, transientStateProviders);
    CO_RETURN_ON_FAILURE(status);
    ASSERT_IFNOT(
        transientStateProviders->Count() == 0,
        "{0}: Number of transient state providers must be 0. First transient SPid: {1}",
        TraceId,
        (*transientStateProviders)[0]->StateProviderId);
#endif

    KArray<Awaitable<NTSTATUS>> awaitables(GetThisAllocator(), 2, 0);
    ASSERT_IFNOT(NT_SUCCESS(awaitables.Status()), "{0}: Could not allocate array of size two.", TraceId);

    // Open all active and deleted state providers.
    KSharedArray<Metadata::CSPtr>::SPtr activeStateProviders = nullptr;
    status = metadataManagerSPtr_->GetInMemoryMetadataArray(StateProviderFilterMode::Active, activeStateProviders);
    if (NT_SUCCESS(status) == false)
    {
        TraceError(L"OpenStateProvidersAsync: GetInMemoryMetadataArray.", status);
        co_return status;
    }

    KSharedArray<Metadata::CSPtr>::SPtr deletedStateProviders = nullptr;
    status =  metadataManagerSPtr_->GetDeletedConstMetadataArray(deletedStateProviders);
    if (NT_SUCCESS(status) == false)
    {
        TraceError(L"OpenStateProvidersAsync: GetDeletedMetadataArray.", status);
        co_return status;
    }

    Awaitable<NTSTATUS> activeAwaitable = apiDispatcher_->OpenAsync(*activeStateProviders, cancellationToken);
    Awaitable<NTSTATUS> deletedAwaitable = apiDispatcher_->OpenAsync(*deletedStateProviders, cancellationToken);
     
    status = awaitables.Append(Ktl::Move(activeAwaitable));
    ASSERT_IFNOT(NT_SUCCESS(awaitables.Status()), "{0}: Append failed even though array is reserved.", TraceId);

    status = awaitables.Append(Ktl::Move(deletedAwaitable));
    ASSERT_IFNOT(NT_SUCCESS(awaitables.Status()), "{0}: Append failed even though array is reserved.", TraceId);

    status = co_await Data::Utilities::TaskUtilities<NTSTATUS>::WhenAll_NoException(awaitables);
    co_return status;
}

Awaitable<NTSTATUS> StateManager::ChangeRoleOnStateProvidersAsync(
    __in FABRIC_REPLICA_ROLE newRole, 
    __in CancellationToken const& cancellationToken) noexcept
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    StateManagerEventSource::Events->ChangeRoleSP(
        TracePartitionId,
        ReplicaId,
        static_cast<int>(newRole));

    status = co_await this->AcquireChangeRoleLockAsync();
    if (NT_SUCCESS(status) == false)
    {
        TraceError(L"ChangeRoleOnStateProvidersAsync: AcquireChangeRoleLockAsync.", status);
        co_return status;
    }

    KFinally([&] { ReleaseChangeRoleLock(); });

    KSharedArray<Metadata::CSPtr>::SPtr activeStateProviders = nullptr;
    status = metadataManagerSPtr_->GetInMemoryMetadataArray(StateProviderFilterMode::Active, activeStateProviders);
    if (NT_SUCCESS(status) == false)
    {
        TraceError(L"ChangeRoleOnStateProvidersAsync: GetInMemoryMetadataArray.", status);
        co_return status;
    }

    status = co_await apiDispatcher_->ChangeRoleAsync(*activeStateProviders, newRole, cancellationToken);
    if (NT_SUCCESS(status) == false)
    {
        TraceError(L"ChangeRoleOnStateProvidersAsync: ChangeRoleAsync.", status);
        co_return status;
    }

    co_return status;
}

Awaitable<NTSTATUS> StateManager::ChangeRoleToNoneOnDeletedStateProvidersAsync(
    CancellationToken const& cancellationToken) noexcept
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    StateManagerEventSource::Events->ChangeRoleToNone(
        TracePartitionId,
        ReplicaId,
        static_cast<int>(FABRIC_REPLICA_ROLE_NONE));

    status = co_await this->AcquireChangeRoleLockAsync();
    if (NT_SUCCESS(status) == false)
    {
        TraceError(L"ChangeRoleToNoneOnDeletedStateProvidersAsync: AcquireChangeRoleLockAsync.", status);
        co_return status;
    }

    KFinally([&] { ReleaseChangeRoleLock(); });

    KSharedArray<Metadata::CSPtr>::SPtr deletedStateProviders = nullptr;
    status = metadataManagerSPtr_->GetDeletedConstMetadataArray(deletedStateProviders);
    if (NT_SUCCESS(status) == false)
    {
        TraceError(L"ChangeRoleToNoneOnDeletedStateProvidersAsync: GetDeletedMetadataArray.", status);
        co_return status;
    }

    status = co_await apiDispatcher_->ChangeRoleAsync(
        *deletedStateProviders, 
        FABRIC_REPLICA_ROLE_NONE, 
        cancellationToken);
    if (NT_SUCCESS(status) == false)
    {
        TraceError(L"ChangeRoleToNoneOnDeletedStateProvidersAsync: ChangeRoleAsync.", status);
        co_return status;
    }

    co_return status;
}

// Closes all state providers: (Active & TransientDeleted) +  Deleted state providers.
//
// This is an operation that is called as part of OnServiceCloseTask.
// In these cases, service close operation is already irreversible: 
// it will be considered closed whether or not we fail this operation.
// Because of this function must be failure free and in case of failure Assert.
//
// Algorithm:
// 1. Get state providers to be closed (Active and deleted)
// 2. Start closing all above state providers. (ApiDispatcher ensures close is failure free).
// 3. Populate the WhenAll array
// 4. Await the in-flight closes.
Awaitable<NTSTATUS> StateManager::CloseStateProvidersAsync(
    __in CancellationToken const & cancellationToken) noexcept
{
    NTSTATUS status = STATUS_SUCCESS;

    ULONGLONG time = KNt::GetTickCount64();

    // Note: We cannot assert that there are no transient state providers.
    // This is because Unlock is not tracked in Logging Replicator, which means it may come after CloseAsync/Abort.
    // On Primary, these transient state providers must not have been opened yet, so we should not close/abort them anyways.
    // On !Primary, there is no transient state.

    // Preallocate the Awaitable array.
    KArray<Awaitable<NTSTATUS>> awaitables(GetThisAllocator(), 2, 0);
    if (NT_SUCCESS(awaitables.Status()) == false)
    {
        StateManagerEventSource::Events->Dispatch_CloseAsync(
            TracePartitionId,
            ReplicaId,
            -1,
            -1,
            KNt::GetTickCount64() - time,
            awaitables.Status());
        co_return awaitables.Status();
    }

    // Stage 1. Get state providers to be closed (Active and deleted)
    KSharedArray<Metadata::CSPtr>::SPtr activeStateProviders = nullptr;
    status = metadataManagerSPtr_->GetInMemoryMetadataArray(StateProviderFilterMode::Active, activeStateProviders);
    if (NT_SUCCESS(status) == false)
    {
        StateManagerEventSource::Events->Dispatch_CloseAsync(
            TracePartitionId,
            ReplicaId,
            -1,
            -1,
            KNt::GetTickCount64() - time,
            status);
        co_return status;
    }

    KSharedArray<Metadata::CSPtr>::SPtr deletedStateProviders = nullptr;
    status = metadataManagerSPtr_->GetDeletedConstMetadataArray(deletedStateProviders);
    if (NT_SUCCESS(status) == false)
    {
        StateManagerEventSource::Events->Dispatch_CloseAsync(
            TracePartitionId,
            ReplicaId,
            activeStateProviders->Count(),
            -1,
            KNt::GetTickCount64() - time,
            status);
        co_return status;
    }

    // Stage 2: Start closing all deleted providers. (ApiDispatcher ensures close is failure free).
    Awaitable<NTSTATUS> activeAwaitable = apiDispatcher_->CloseAsync(
        *activeStateProviders,
        ApiDispatcher::FailureAction::AbortStateProvider,
        cancellationToken);

    Awaitable<NTSTATUS> deletedAwaitable = apiDispatcher_->CloseAsync(
        *deletedStateProviders, 
        ApiDispatcher::FailureAction::AbortStateProvider,
        cancellationToken);

    // 3. Populate the WhenAll array
    status = awaitables.Append(Ktl::Move(activeAwaitable));
    if (NT_SUCCESS(status) == false)
    {
        StateManagerEventSource::Events->Dispatch_CloseAsync(
            TracePartitionId,
            ReplicaId,
            activeStateProviders->Count(),
            deletedStateProviders->Count(),
            KNt::GetTickCount64() - time,
            status);
        co_return status;
    }

    status = awaitables.Append(Ktl::Move(deletedAwaitable));
    if (NT_SUCCESS(status) == false)
    {
        StateManagerEventSource::Events->Dispatch_CloseAsync(
            TracePartitionId,
            ReplicaId,
            activeStateProviders->Count(),
            deletedStateProviders->Count(),
            KNt::GetTickCount64() - time,
            status);
        co_return status;
    }

    // Stage 3: Await all in flight closes.
    co_await Data::Utilities::TaskUtilities<void>::WhenAll(awaitables);

#ifdef DBG
    for (ULONG index = 0; index < awaitables.Count(); index++)
    {
        status = co_await awaitables[index];
        ASSERT_IFNOT(
            NT_SUCCESS(status),
            "{0} ApiDispatcher::CloseAsync failed even though FailureAction is AbortStateProvider. Status: {1}",
            TraceId,
            status);
    }
#endif

    StateManagerEventSource::Events->Dispatch_CloseAsync(
        TracePartitionId,
        ReplicaId,
        activeStateProviders->Count(),
        deletedStateProviders->Count(),
        KNt::GetTickCount64() - time,
        status);
    co_return status;
}

void StateManager::AbortStateProviders() noexcept
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    LONG64 activeCount = -1;
    LONG64 deletedCount = -1;

    // Note that 1 tick is 1 ms.
    ULONGLONG time = KNt::GetTickCount64();

    // Note: We cannot assert that there are no transient state providers.
    // This is because Unlock is not tracked in Logging Replicator, which means it may come after CloseAsync/Abort.
    // On Primary, these transient state providers must not have been opened yet, so we should not close/abort them anyways.
    // On !Primary, there is no transient state.
    // Abort all active and deleted state providers.
    // Stage 1: Abort active state provider.
    {
        KSharedArray<Metadata::CSPtr>::SPtr activeStateProviders = nullptr;
        status = metadataManagerSPtr_->GetInMemoryMetadataArray(StateProviderFilterMode::Active, activeStateProviders);
        ASSERT_IFNOT(NT_SUCCESS(status), "{0}: AbortStateProviders: GetInMemoryMetadataArray failed. Status {1}.", TraceId, status);

        apiDispatcher_->Abort(*activeStateProviders, 0, activeStateProviders->Count());
        activeCount = activeStateProviders->Count();
    }

    // Stage 2: Abort deleted state providers.
    {
        KSharedArray<Metadata::CSPtr>::SPtr deletedStateProviders = nullptr;
        status = metadataManagerSPtr_->GetDeletedConstMetadataArray(deletedStateProviders);
        ASSERT_IFNOT(NT_SUCCESS(status), "{0}: AbortStateProviders: GetDeletedMetadataArray failed. Status {1}.", TraceId, status);

        apiDispatcher_->Abort(*deletedStateProviders, 0, deletedStateProviders->Count());
        deletedCount = deletedStateProviders->Count();
    }

    StateManagerEventSource::Events->Dispatch_Abort(
        TracePartitionId,
        ReplicaId,
        activeCount,
        deletedCount,
        KNt::GetTickCount64() - time);
}

Awaitable<NTSTATUS> StateManager::RecoverStateProvidersAsync(
    __in CancellationToken const & cancellationToken) noexcept
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    StateManagerEventSource::Events->RecoverSPStart(
        TracePartitionId,
        ReplicaId);

    KSharedArray<Metadata::CSPtr>::SPtr activeStateProviders = nullptr;
    status = metadataManagerSPtr_->GetInMemoryMetadataArray(StateProviderFilterMode::Active, activeStateProviders);
    if (NT_SUCCESS(status) == false)
    {
        TraceError(L"RecoverStateProvidersAsync: GetInMemoryMetadataArray", status);
        co_return status;
    }

    status = co_await apiDispatcher_->RecoverCheckpointAsync(*activeStateProviders, cancellationToken);
    co_return status;
}

Awaitable<NTSTATUS> StateManager::RemoveStateOnStateProvidersAsync(
    __in CancellationToken const & cancellationToken) noexcept
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    StateManagerEventSource::Events->RemoveSP(
        TracePartitionId,
        ReplicaId);

    // Note that deleted state providers are not called ChangeRole during change role.
    // In the managed, this causes them not to be changed role to none when replica is being deleted
    // before the remove state call.
    status = co_await this->ChangeRoleToNoneOnDeletedStateProvidersAsync(cancellationToken);
    CO_RETURN_ON_FAILURE(status);

    // Call Remove State on active and soft deleted state providers.
    KArray<Awaitable<NTSTATUS>> awaitables(GetThisAllocator(), 2, 0);
    ASSERT_IFNOT(NT_SUCCESS(awaitables.Status()), "{0}: Could not allocate array of size two.", TraceId);

    // Open all active and deleted state providers.
    KSharedArray<Metadata::CSPtr>::SPtr activeStateProviders = nullptr;
    status = metadataManagerSPtr_->GetInMemoryMetadataArray(StateProviderFilterMode::Active, activeStateProviders);
    CO_RETURN_ON_FAILURE(status);

    KSharedArray<Metadata::CSPtr>::SPtr deletedStateProviders = nullptr;
    status = metadataManagerSPtr_->GetDeletedConstMetadataArray(deletedStateProviders);
    CO_RETURN_ON_FAILURE(status);

    Awaitable<NTSTATUS> activeAwaitable = apiDispatcher_->RemoveStateAsync(*activeStateProviders, cancellationToken);
    Awaitable<NTSTATUS> deletedAwaitable = apiDispatcher_->RemoveStateAsync(*deletedStateProviders, cancellationToken);

    status = awaitables.Append(Ktl::Move(activeAwaitable));
    ASSERT_IFNOT(NT_SUCCESS(awaitables.Status()), "{0}: RemoveStateOnStateProvidersAsync: Append failed even though array is reserved.", TraceId);

    status = awaitables.Append(Ktl::Move(deletedAwaitable));
    ASSERT_IFNOT(NT_SUCCESS(awaitables.Status()), "{0}: RemoveStateOnStateProvidersAsync: Append failed even though array is reserved.", TraceId);

    status = co_await Data::Utilities::TaskUtilities<NTSTATUS>::WhenAll_NoException(awaitables);
    co_return status;
}

Awaitable<void> StateManager::RecoverStateManagerCheckpointAsync(__in CancellationToken const& cancellationToken)
{
    Common::Stopwatch stopwatch;
    stopwatch.Start();

    KSharedArray<Metadata::SPtr>::SPtr recoveredMetadataList = co_await checkpointManagerSPtr_->RecoverCheckpointAsync(
        *metadataManagerSPtr_,
        cancellationToken);

    stopwatch.Stop();
    int64 timer = stopwatch.ElapsedMilliseconds;

    if (recoveredMetadataList == nullptr)
    {
        StateManagerEventSource::Events->RecoverSMCheckpointEmpty(
            TracePartitionId,
            ReplicaId,
            timer);

        co_return;
    }

    // Sort the array recoveredMetadataList in the descending state provider name order to ensure 
    // InitializeStateProvidersInOrder works in case of hierarchical state providers.
    Sort<Metadata::SPtr>::QuickSort(false, Comparer::Compare, recoveredMetadataList);

    for (ULONG index = 0; index < recoveredMetadataList->Count(); index++)
    {
        // To make sure the metadata in the array are sorted in the descending order
        if (index != 0)
        {
            bool itemsAreSortedInDescendingOrder = *(*recoveredMetadataList)[index]->Name <= *(*recoveredMetadataList)[index - 1]->Name;
            ASSERT_IFNOT(
                itemsAreSortedInDescendingOrder,
                "{0}: Items must be in sorted order. Index: {1} Current: {2} Last: {3}",
                TraceId,
                index,
                ToStringLiteral(*(*recoveredMetadataList)[index]->Name),
                ToStringLiteral(*(*recoveredMetadataList)[index - 1]->Name));
        }

        KSharedPtr<Metadata> metadataSPtr = (*recoveredMetadataList)[index];
        InitializeStateProvidersInOrder(*metadataSPtr);
        UpdateLastStateProviderId(metadataSPtr->StateProviderId);
    }

    stopwatch.Restart();

    RemoveUnreferencedStateProviders();

    stopwatch.Stop();

    Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, IStateProvider2::SPtr>>::SPtr enumerator = nullptr;
    NTSTATUS status = CreateInternalEnumerator(true, enumerator);
    THROW_ON_FAILURE(status);

    status = NotifyStateManagerStateChanged(*enumerator);
    THROW_ON_FAILURE(status);

    StateManagerEventSource::Events->RecoverSMCheckpoint(
        TracePartitionId,
        ReplicaId,
        timer,
        stopwatch.ElapsedMilliseconds);

    co_return;
}

void StateManager::BeginSetCurrentLocalState()
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    StateManagerEventSource::Events->BeginSetState(
        TracePartitionId,
        ReplicaId);

    // Change mode of all deleted state providers to FalseProgress
    status = metadataManagerSPtr_->MarkAllDeletedStateProviders(MetadataMode::FalseProgress);
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"BeginSetCurrentLocalState: MarkAllDeletedStateProviders.",
        Helper::StateManager);

    // Save state to avoid creating state providers in case the same data is obtained through copy and 
    // track them to avoid state provider state file leaks.
    metadataManagerSPtr_->MoveStateProvidersToDeletedList();

    // Copy all the deleted state providers and saved in the local array, because metadataManager will be re-initialized
    KSharedArray<Metadata::SPtr>::SPtr copyOfDeletedStateProviders;
    status = metadataManagerSPtr_->GetDeletedMetadataArray(copyOfDeletedStateProviders);
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"BeginSetCurrentLocalState: Create GetDeletedMetadataArray.",
        Helper::StateManager);

    metadataManagerSPtr_->Dispose();
    
    // re-initialize
    // Note that this is safe since there is not concurrent operations on metadata manager during this operation.
    status = MetadataManager::Create(*PartitionedReplicaIdentifier, GetThisAllocator(), metadataManagerSPtr_);
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"BeginSetCurrentLocalState: Create MetadataManager.",
        Helper::StateManager);

    // Track these state providers for file clean up and avoid recreating them, if they are present in secondary.
    for (Metadata::SPtr metadata : *copyOfDeletedStateProviders)
    {
        ASSERT_IF(metadata == nullptr, "{0}: BeginSetCurrentLocalState: Metadata cannot be null", TraceId);
        bool isAdded = metadataManagerSPtr_->AddDeleted(metadata->StateProviderId, *metadata);
        ASSERT_IFNOT(isAdded, "{0}: BeginSetCurrentLocalState: Adding state provider {1} to deleted list has failed.", TraceId, metadata->StateProviderId);
    }
}

Awaitable<void> StateManager::SetCurrentLocalStateAsync(
    __in LONG64 stateRecordNumber, 
    __in OperationData const & data,
    __in CancellationToken const & cancellationToken)
{
    ApiEntry();

    SerializableMetadataEnumerator::SPtr serializableMetadataEnumerator = CheckpointFile::ReadCopyData(*PartitionedReplicaIdentifier, &data, GetThisAllocator());
    if (serializableMetadataEnumerator == nullptr)
    {
        co_return;
    }

    while (serializableMetadataEnumerator->MoveNext())
    {
        co_await this->CopyToLocalStateAsync(*(serializableMetadataEnumerator->Current()), cancellationToken);
    }

    co_return;
}

Awaitable<void> StateManager::CopyToLocalStateAsync(
    __in SerializableMetadata const & serializableMetadata, 
    __in CancellationToken const& cancellationToken)
{
    // MCoskun: This is redundant because this is always called from State Manager. 
    // However, I am adding it because below logic takes a dependency on this (see the comment on closing state providers.)
    ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    // set current state cannot obtain data such that it is present both in active and deleted list.
    if (serializableMetadata.MetadataMode == MetadataMode::Active)
    {
        // check if the state provider is already present in the recovered state.
        if (metadataManagerSPtr_->IsStateProviderDeletedOrStale(serializableMetadata.StateProviderId, MetadataMode::FalseProgress))
        {
            Metadata::SPtr metadata;
            bool isExist = metadataManagerSPtr_->TryGetDeletedMetadata(serializableMetadata.StateProviderId, metadata);
            ASSERT_IFNOT(isExist, "{0}: State item must exist. State Provider Id: {1}", TraceId, serializableMetadata.StateProviderId);

            ASSERT_IFNOT(
                metadata->MetadataMode == MetadataMode::FalseProgress,
                "{0}: StateManager.CopyToLocalStateAsync. StateProviderId: {1}. Expected metadata mode is false progress, but obtained is {2}",
                TraceId,
                serializableMetadata.StateProviderId,
                metadata->MetadataMode);

            // MCoskun: Order of the following operations is significant to prevent double closing a state provider.
            co_await metadataManagerSPtr_->AcquireLockAndAddAsync(
                *metadata->Name,
                *metadata,
                Common::TimeSpan::MaxValue,
                cancellationToken);
            Metadata::SPtr resurrectedMetadata = nullptr;
            bool isRemoved = metadataManagerSPtr_->TryRemoveDeleted(metadata->StateProviderId, resurrectedMetadata);

            // Note that if we throw instead of Asserting at this point, we would have to handle the case that during close, state manager
            // closes all state providers in the deleted list as well as the active/transient ones.
            // At the moment, if isRemoved == false, then this state provider is in both places.
            ASSERT_IFNOT(
                isRemoved,
                "{0}: CopyToLocalStateAsync: Failed to remove resurrected item from soft delete list. StateProviderId {1}",
                TraceId,
                metadata->StateProviderId);

            metadata->MetadataMode = MetadataMode::Active;

            // Note: Because U->I is called before BeginSettingCurrentState, we can assert that it must be idle secondary.
            status = co_await apiDispatcher_->BeginSettingCurrentStateAsync(*metadata);
            Helper::ThrowIfNecessary(
                status,
                TracePartitionId,
                ReplicaId,
                L"CopyToLocalStateAsync: BeginSettingCurrentState failed.",
                Helper::StateManager);

            status = copyProgressArray_.Append(metadata.RawPtr());
            Helper::ThrowIfNecessary(
                status,
                TracePartitionId,
                ReplicaId,
                L"CopyToLocalStateAsync: Put to copyProgressMap if metadata is stale.",
                Helper::StateManager);

            co_return;
        }

        IStateProvider2::SPtr stateProvider = nullptr;
        status = apiDispatcher_->CreateStateProvider(
            *serializableMetadata.Name, 
            serializableMetadata.StateProviderId,
            *serializableMetadata.TypeString, 
            serializableMetadata.InitializationParameters.RawPtr(),
            stateProvider);
        Helper::ThrowIfNecessary(
            status,
            TracePartitionId,
            ReplicaId,
            L"CopyToLocalStateAsync: CreateStateProvider",
            Helper::StateManager);

        Metadata::SPtr metadata = nullptr;
        status = Metadata::Create(serializableMetadata, *stateProvider, false, GetThisAllocator(), metadata);
        Helper::ThrowIfNecessary(
            status,
            TracePartitionId,
            ReplicaId,
            L"CopyToLocalStateAsync: Create Metadata.",
            Helper::StateManager);

        KSharedArray<Metadata::SPtr>::SPtr stateproviderMetadataArray = InitializeStateProvidersInOrder(*metadata);
        // If it is a child, metadata array will be null.
        if (stateproviderMetadataArray == nullptr)
        {
            co_return;
        }

        // Note that metadata array includes the parent.
        for (Metadata::SPtr stateProviderMetadataSPtr : *stateproviderMetadataArray)
        {
            // This is where a given state provider tree (parent and all the children) get opened and prepared one at a time.
            // 11908441: We can make this parallel: This will only help state providers that are hierarchical. 
            // MCoskun: order of the following operations is significant.
            // We need to Open the state provider before adding in to the metadata manager because, in case of a failure/fault
            // CloseAsync and Abort close all state providers in metadata manager (active, transient and deleted).
            // If we do not do these operations in this order a state provider that has never been opened, may get closeAsync.
            // Furthermore, it is important that we do not throw between these operations and that if AcquireLockAndAddAsync throws, 
            // we close the state provider.
            // Note that if ReportFault was called on a different thread, KAsyncService deferred close will protect a CloseAsync 
            // racing with the following operations.
            status = co_await apiDispatcher_->OpenAsync(*stateProviderMetadataSPtr, cancellationToken);
            THROW_ON_FAILURE(status);

            SharedException::CSPtr exceptionSPtr = nullptr;
            try 
            {
                co_await metadataManagerSPtr_->AcquireLockAndAddAsync(
                    *stateProviderMetadataSPtr->Name,
                    *stateProviderMetadataSPtr,
                    Common::TimeSpan::MaxValue,
                    cancellationToken);
            }
            catch (Exception & e)
            {
                exceptionSPtr = SharedException::Create(e, GetThisAllocator());
            }

            if (exceptionSPtr != nullptr)
            {
                status = co_await apiDispatcher_->CloseAsync(
                    *stateProviderMetadataSPtr, 
                    ApiDispatcher::FailureAction::AbortStateProvider,
                    cancellationToken);
                ASSERT_IFNOT(
                    NT_SUCCESS(status), 
                    "{0}: ApiDispatcher::CloseAsync returned failed status even though FaultAction = AbortStateProvider: {1}",
                    TraceId, 
                    status)

                throw exceptionSPtr->get_Info();
            }
            
            // End of the important block described above.
            status = co_await apiDispatcher_->RecoverCheckpointAsync(*stateProviderMetadataSPtr, cancellationToken);
            THROW_ON_FAILURE(status);
            
            // #9181140: BUG in managed. Must transition to idle secondary before BeginSettingCurrentState is called.
            status = co_await apiDispatcher_->ChangeRoleAsync(
                *stateProviderMetadataSPtr, 
                FABRIC_REPLICA_ROLE_IDLE_SECONDARY, 
                cancellationToken);
            THROW_ON_FAILURE(status);

            status = co_await apiDispatcher_->BeginSettingCurrentStateAsync(*stateProviderMetadataSPtr);
            Helper::ThrowIfNecessary(
                status,
                TracePartitionId,
                ReplicaId,
                L"CopyToLocalStateAsync: BeginSettingCurrentState",
                Helper::StateManager);

            status = copyProgressArray_.Append(stateProviderMetadataSPtr.RawPtr());
            Helper::ThrowIfNecessary(
                status,
                TracePartitionId,
                ReplicaId,
                L"CopyToLocalStateAsync: Put Metadata to copyProgressMap.",
                Helper::StateManager);
        }

        co_return;
    }

    // Check if it is already present in the stale state, it cannot be present in active and stale state
    if (metadataManagerSPtr_->IsStateProviderDeletedOrStale(serializableMetadata.StateProviderId, MetadataMode::FalseProgress))
    {
        Metadata::SPtr metadata;
        bool isExist = metadataManagerSPtr_->TryGetDeletedMetadata(serializableMetadata.StateProviderId, metadata);
        ASSERT_IFNOT(isExist, "{0}: State item must exist. State Provider Id: {1}", TraceId, serializableMetadata.StateProviderId);

        metadata->MetadataMode = serializableMetadata.MetadataMode;
        metadata->DeleteLSN = serializableMetadata.DeleteLSN;

        co_return;
    }

    IStateProvider2::SPtr stateProvider = nullptr;
    status = apiDispatcher_->CreateStateProvider(
        *serializableMetadata.Name, 
        serializableMetadata.StateProviderId,
        *serializableMetadata.TypeString, 
        serializableMetadata.InitializationParameters.RawPtr(),
        stateProvider);
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"CopyToLocalStateAsync: CreateStateProvider.",
        Helper::StateManager);

    Metadata::SPtr metadata = nullptr;
    status = Metadata::Create(serializableMetadata, *stateProvider, false, GetThisAllocator(), metadata);
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"CopyToLocalStateAsync: Create Metadata.",
        Helper::StateManager);

    // Difference from managed: Assert that they are same instead of setting it.
    ASSERT_IFNOT(
        metadata->MetadataMode == serializableMetadata.MetadataMode,
        "{0}: CopyToLocalStateAsync: Metadata mode mismatch for SPid: {1}: {2} == {3}", 
        TraceId, 
        serializableMetadata.StateProviderId,
        metadata->MetadataMode,
        serializableMetadata.MetadataMode);

    StateManagerEventSource::Events->CopyState(
        TracePartitionId,
        ReplicaId,
        metadata->StateProviderId);

    KSharedArray<Metadata::SPtr>::SPtr stateProviderMetadataArray = InitializeStateProvidersInOrder(*metadata);
    // If it is a child, metadata array will be null.
    if (stateProviderMetadataArray == nullptr)
    {
        co_return;
    }

    for (Metadata::SPtr stateProviderMetadata : *stateProviderMetadataArray)
    {
        // PreethaS: OpenAsync is called for now to be consistent with delete during replication.
        // MCoskun: Order of the following operations is significant to prevent potentially closing a replica that has not opened.
        // See above for more explanation.
        status = co_await apiDispatcher_->OpenAsync(*stateProviderMetadata, cancellationToken);
        THROW_ON_FAILURE(status);

        bool addToDeleteList = metadataManagerSPtr_->AddDeleted(stateProviderMetadata->StateProviderId, *stateProviderMetadata);

        // MCoskun: If we were not Assert here and instead throw, we would have to Close the above state provider.
        ASSERT_IFNOT(addToDeleteList, "{0}: CopyToLocalStateAsync: Failed to add state provider {1} to delete list", TraceId, stateProviderMetadata->StateProviderId);
    }

    co_return;
}

Awaitable<NTSTATUS> StateManager::EndSettingCurrentLocalStateAsync(
    __in CancellationToken const & cancellationToken) noexcept
{
    ApiEntryAsync();

    Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, IStateProvider2::SPtr>>::SPtr enumerator;
    NTSTATUS status = CreateInternalEnumerator(true, enumerator);
    CO_RETURN_ON_FAILURE(status);

    // NotifyStateManagerStateChanged will trace if it fails
    status = NotifyStateManagerStateChanged(*enumerator);
    co_return status;
}

NTSTATUS StateManager::GetChildren(
    __in IStateProvider2 & root,
    __inout KHashTable<KUri::CSPtr, StateProviderInfoInternal> & children) noexcept
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    StateManagerEventSource::Events->GetChildren(
        TracePartitionId,
        ReplicaId);

    KQueue<IStateProvider2::SPtr> treeEntries(GetThisAllocator());
    BOOLEAN queueOperationResult = treeEntries.Enq(&root);
    ASSERT_IFNOT(
        queueOperationResult == TRUE,
        "{0}: GetChildren: Enqueue failed.",
        TraceId);

    while (treeEntries.Count() > 0)
    {
        IStateProvider2::SPtr stateProvider;
        queueOperationResult = treeEntries.Deq(stateProvider);
        ASSERT_IFNOT(
            queueOperationResult == TRUE,
            "{0}: GetChildren: Dequeue failed.",
            TraceId);

        // In the following try catch block, the only function that may throw exception is IStateProvider2::GetChildren
        // #10260729: Make a GetChildren function in ApiDispatcher. Make sure KArray is passed in and every GetChildren
        // interface implementation needs to make changes accordingly.
        try
        {
            KArray<StateProviderInfo> childrenInfoArray = stateProvider->GetChildren(stateProvider->GetName());
            status = childrenInfoArray.Status();
            if (NT_SUCCESS(status) == false)
            {
                StateManagerEventSource::Events->Error(
                    TracePartitionId,
                    ReplicaId,
                    L"GetChildren: Create childrenInfoArray.",
                    status);
                return status;
            }

            if (childrenInfoArray.Count() == 0)
            {
                continue;
            }

            for (StateProviderInfo childInfo : childrenInfoArray)
            {
                FABRIC_STATE_PROVIDER_ID stateProviderId = CreateStateProviderId();

                IStateProvider2::SPtr entry = nullptr;
                status = apiDispatcher_->CreateStateProvider(
                    *childInfo.Name,
                    stateProviderId,
                    *childInfo.TypeName,
                    childInfo.InitializationParameters.RawPtr(),
                    entry);
                if (NT_SUCCESS(status) == false)
                {
                    StateManagerEventSource::Events->Error(
                        TracePartitionId,
                        ReplicaId,
                        L"GetChildren: CreateStateProvider.",
                        status);
                    return status;
                }

                ASSERT_IFNOT(
                    entry != nullptr,
                    "{0}: GetChildren: CreateStateProvider return nullptr.",
                    TraceId);
                queueOperationResult = treeEntries.Enq(entry);
                ASSERT_IFNOT(
                    queueOperationResult == TRUE,
                    "{0}: GetChildren: Children Enqueue failed.",
                    TraceId);

                StateProviderInfoInternal info(*childInfo.TypeName, *childInfo.Name, stateProviderId, *entry);
                status = children.Put(childInfo.Name, info, FALSE);
                if (NT_SUCCESS(status) == false && status != STATUS_OBJECT_NAME_COLLISION)
                {
                    StateManagerEventSource::Events->Error(
                        TracePartitionId,
                        ReplicaId,
                        L"GetChildren: Put StateProviderInfoInternal to KHashTable.",
                        status);
                    return status;
                }
            }
        }
        catch (Exception & exception)
        {
            StateManagerEventSource::Events->Error(
                TracePartitionId,
                ReplicaId,
                L"GetChildren: IStateProvider2::GetChildren",
                status);
            return exception.GetStatus();
        }
    }

    return status;
}

NTSTATUS StateManager::SingleOperationTransactionAbortUnlock(
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in OperationContext const & operationContext) noexcept
{
    ApiEntryReturn();

    try
    {
        if (stateProviderId == StateManagerId)
        {
            NTSTATUS status = Unlock(operationContext);
            return status;
        }

        // Note that we could avoid the copy here since it is an internal API.
        // Keeping it for now for safety.
        // TryGetMetadata may throw exception. Catches it and returns NTSTATUS error code.
        Metadata::SPtr metadataSPtr = nullptr;
        bool isExist = metadataManagerSPtr_->TryGetMetadata(stateProviderId, metadataSPtr);
        ASSERT_IFNOT(isExist, "{0}: Null metadata object", TraceId);

        metadataSPtr->StateProvider->Unlock(operationContext);
    }
    catch(Exception & exception)
    {
        TraceException(L"SingleOperationTransactionAbortUnlock", exception);
        partitionSPtr_->ReportFault(FABRIC_FAULT_TYPE_TRANSIENT);
        return exception.GetStatus();
    }

    return STATUS_SUCCESS;
}

NTSTATUS StateManager::ErrorIfStateProviderIsNotRegistered(
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in LONG64 transactionId) noexcept
{
    ApiEntryReturn();

    return IsRegistered(stateProviderId, true, transactionId);
}

__checkReturn Awaitable<NTSTATUS> StateManager::AddSingleAsync(
    __in Transaction & transaction, 
    __in KUri const& stateProviderName,
    __in KString const & stateProviderTypeName,
    __in_opt OperationData const * const initializationParameters,
    __in IStateProvider2 & stateProvider,
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in bool shouldAcquireLock, 
    __in FABRIC_STATE_PROVIDER_ID parentId,
    __in Common::TimeSpan const& timeout, 
    __in CancellationToken const& cancellationToken) noexcept
{   
    ApiEntryAsync();
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    
    // Check if the state provider name is valid.
    // CheckIfNameIsValid will trace if it fails
    status = CheckIfNameIsValid(stateProviderName, false);
    CO_RETURN_ON_FAILURE(status);
    
    if (shouldAcquireLock)
    {
        // MetadataManager::LockForWriteAsync will trace if it fails
        StateManagerLockContext::SPtr lockContextSPtr = nullptr;
        status = co_await metadataManagerSPtr_->LockForWriteAsync(
            stateProviderName,
            stateProviderId,
            transaction,
            timeout,
            cancellationToken,
            lockContextSPtr);
        CO_RETURN_ON_FAILURE(status);

        StateManagerTransactionContext::SPtr transactionLockContextSPtr = nullptr;
        status = StateManagerTransactionContext::Create(transaction.TransactionId, *lockContextSPtr, OperationType::Add, GetThisAllocator(), transactionLockContextSPtr);
        if (NT_SUCCESS(status) == false)
        {
            StateManagerEventSource::Events->ISPM_ApiError(
                TracePartitionId,
                ReplicaId,
                stateProviderId,
                "AddAsync: (Single) Add lock context failed.",
                ToStringLiteral(stateProviderName),
                transaction.TransactionId,
                status);

            // #9019333, Add tests for the case, StateManagerTransactionContext Create failed after getting the read/write lock
            NTSTATUS releaseLockStatus = lockContextSPtr->ReleaseLock(transaction.TransactionId);
            if (NT_SUCCESS(releaseLockStatus) == false)
            {
                TraceError(
                    L"AddAsync: (Single) StateManagerLockContext::ReleaseLock",
                    releaseLockStatus);
            }

            co_return status;
        }

        status = transaction.AddLockContext(*transactionLockContextSPtr);
        if (NT_SUCCESS(status) == false)
        {
            StateManagerEventSource::Events->ISPM_ApiError(
                TracePartitionId,
                ReplicaId,
                stateProviderId,
                "AddAsync: (Single) Add lock context failed.",
                ToStringLiteral(stateProviderName),
                transaction.TransactionId,
                status);
            NTSTATUS releaseLockStatus = lockContextSPtr->ReleaseLock(transaction.TransactionId);
            if (NT_SUCCESS(releaseLockStatus) == false)
            {
                TraceError(
                    L"AddAsync: (Single) StateManagerLockContext::ReleaseLock",
                    releaseLockStatus);
            }

            co_return status;
        }
    }

    MetadataOperationData::CSPtr metadataOperationDataCSPtr = nullptr;
    status = MetadataOperationData::Create(stateProviderId, stateProviderName, ApplyType::Insert, GetThisAllocator(), metadataOperationDataCSPtr);
    if (NT_SUCCESS(status) == false)
    {
        StateManagerEventSource::Events->ISPM_ApiError(
            TracePartitionId,
            ReplicaId,
            stateProviderId,
            "AddAsync: (Single) Create MetadataOperationData failed.",
            ToStringLiteral(stateProviderName),
            transaction.TransactionId,
            status);

        co_return status;
    }
    
    RedoOperationData::CSPtr redoOperationDataCSPtr = nullptr;
    status = RedoOperationData::Create(
        *PartitionedReplicaIdentifier, 
        stateProviderTypeName, 
        initializationParameters, 
        parentId, 
        serializationMode_,
        GetThisAllocator(), 
        redoOperationDataCSPtr);
    if (NT_SUCCESS(status) == false)
    {
        StateManagerEventSource::Events->ISPM_ApiError(
            TracePartitionId,
            ReplicaId,
            stateProviderId,
            "AddAsync: (Single) Create RedoOperationData.",
            ToStringLiteral(stateProviderName),
            transaction.TransactionId,
            status);

        co_return status;
    }
    
    Metadata::SPtr metadataSPtr = nullptr;
    status = Metadata::Create(
        stateProviderName,
        stateProviderTypeName,
        stateProvider,
        initializationParameters,
        stateProviderId,
        parentId,
        MetadataMode::Active,
        true,
        GetThisAllocator(),
        metadataSPtr);
    if (NT_SUCCESS(status) == false)
    {
        StateManagerEventSource::Events->ISPM_ApiError(
            TracePartitionId,
            ReplicaId,
            stateProviderId,
            "AddAsync: (Single) Create Metadata.",
            ToStringLiteral(stateProviderName),
            transaction.TransactionId,
            status);

        co_return status;
    }

    bool wasAdded = metadataManagerSPtr_->TryAdd(stateProviderName, *metadataSPtr);
    if (wasAdded == true)
    {
        StateManagerEventSource::Events->AddSingle(
            TracePartitionId,
            ReplicaId,
            stateProviderId,
            transaction.TransactionId,
            ToStringLiteral(stateProviderTypeName));
    }
    else
    {
        StateManagerEventSource::Events->AddSingleErr(
            TracePartitionId,
            ReplicaId,
            stateProviderId,
            transaction.TransactionId,
            ToStringLiteral(stateProviderTypeName),
            SF_STATUS_NAME_ALREADY_EXISTS);
        co_return SF_STATUS_NAME_ALREADY_EXISTS;
    }

    // ReplicateAsync will trace if it fails
    status = co_await this->ReplicateAsync(
        transaction, 
        *metadataOperationDataCSPtr, 
        redoOperationDataCSPtr.RawPtr(), 
        timeout, 
        cancellationToken);
    CO_RETURN_ON_FAILURE(status);

    co_return status;
}

// Note: CheckReturn: Return value v must not be ignored by callers of this function.
__checkReturn Awaitable<NTSTATUS> StateManager::RemoveSingleAsync(
    __in Transaction & transaction, 
    __in KUri const & stateProviderName, 
    __in Common::TimeSpan const & timeout, 
    __in CancellationToken const & cancellationToken) noexcept
{
    ApiEntryAsync();
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    // Note: Although there is already a existence check in the RemoveAsync call, this one is 
    // necessary, because when we delete a parent Metadata, we need to make sure all children exist.
    Metadata::SPtr metadataSPtr = nullptr;
    bool isExist = false;
    try
    {
        // TryGetMetadata may throw exception. Catches it and returns NTSTATUS error code.
        isExist = metadataManagerSPtr_->TryGetMetadata(stateProviderName, false, metadataSPtr);
        if (!isExist)
        {
            StateManagerEventSource::Events->ISPM_ApiError(
                TracePartitionId,
                ReplicaId,
                FABRIC_INVALID_STATE_PROVIDER_ID,
                "RemoveAsync: (Single) State Provider Name does not exist.",
                ToStringLiteral(stateProviderName),
                transaction.TransactionId,
                SF_STATUS_NAME_DOES_NOT_EXIST);
            co_return SF_STATUS_NAME_DOES_NOT_EXIST;
        }
    }
    catch (Exception & exception)
    {
        StateManagerEventSource::Events->ISPM_ApiError(
            TracePartitionId,
            ReplicaId,
            FABRIC_INVALID_STATE_PROVIDER_ID,
            "RemoveAsync: (Single) MetadataManager::TryGetMetadata throws exception",
            ToStringLiteral(stateProviderName),
            transaction.TransactionId,
            exception.GetStatus());
        co_return exception.GetStatus();
    }

    StateManagerEventSource::Events->RemoveSingle(
        TracePartitionId,
        ReplicaId,
        metadataSPtr->StateProviderId,
        ToStringLiteral(stateProviderName),
        transaction.TransactionId);

    // Acquire write lock on the metadata.
    // MetadataManager::LockForWriteAsync will trace if it fails
    StateManagerLockContext::SPtr lockContextSPtr = nullptr;
    status = co_await metadataManagerSPtr_->LockForWriteAsync(
        stateProviderName, 
        metadataSPtr->StateProviderId, 
        transaction, 
        timeout, 
        cancellationToken,
        lockContextSPtr);
    CO_RETURN_ON_FAILURE(status);

    // Register the lock with the replicator.
    StateManagerTransactionContext::SPtr transactionLockContextSPtr = nullptr;
    status = StateManagerTransactionContext::Create(transaction.TransactionId, *lockContextSPtr, OperationType::Remove, GetThisAllocator(), transactionLockContextSPtr);
    if (NT_SUCCESS(status) == false)
    {
        StateManagerEventSource::Events->ISPM_ApiError(
            TracePartitionId,
            ReplicaId,
            metadataSPtr->StateProviderId,
            "RemoveAsync: (Single) Create transaction context failed.",
            ToStringLiteral(stateProviderName),
            transaction.TransactionId,
            status);

        // #9019333, Add tests for the case, StateManagerTransactionContext Create failed after getting the read/write lock
        NTSTATUS releaseLockStatus = lockContextSPtr->ReleaseLock(transaction.TransactionId);
        if (NT_SUCCESS(releaseLockStatus) == false)
        {
            TraceError(
                L"RemoveAsync: (Single) StateManagerLockContext::ReleaseLock",
                releaseLockStatus);
        }

        co_return status;
    }

    status = transaction.AddLockContext(*transactionLockContextSPtr);
    if (NT_SUCCESS(status) == false)
    {
        StateManagerEventSource::Events->ISPM_ApiError(
            TracePartitionId,
            ReplicaId,
            metadataSPtr->StateProviderId,
            "RemoveAsync: (Single) Add lock context failed.",
            ToStringLiteral(stateProviderName),
            transaction.TransactionId,
            status);
        NTSTATUS releaseLockStatus = lockContextSPtr->ReleaseLock(transaction.TransactionId);
        if (NT_SUCCESS(releaseLockStatus) == false)
        {
            TraceError(
                L"RemoveAsync: (Single) StateManagerLockContext::ReleaseLock",
                releaseLockStatus);
        }

        co_return status;
    }

    try
    {
        // Verify the state provider is "active" under lock.
        // TryGetMetadata may throw exception. Catches it and returns NTSTATUS error code.
        isExist = metadataManagerSPtr_->TryGetMetadata(stateProviderName, false, metadataSPtr);
        if (!isExist)
        {
            StateManagerEventSource::Events->ISPM_ApiError(
                TracePartitionId,
                ReplicaId,
                FABRIC_INVALID_STATE_PROVIDER_ID,
                "RemoveAsync: (Single) State Provider Name does not exist under lock.",
                ToStringLiteral(stateProviderName),
                transaction.TransactionId,
                SF_STATUS_NAME_DOES_NOT_EXIST);
            co_return SF_STATUS_NAME_DOES_NOT_EXIST;
        }
    }
    catch (Exception & exception)
    {
        StateManagerEventSource::Events->ISPM_ApiError(
            TracePartitionId,
            ReplicaId,
            FABRIC_INVALID_STATE_PROVIDER_ID,
            "RemoveAsync: (Single) MetadataManager::TryGetMetadata throws exception",
            ToStringLiteral(stateProviderName),
            transaction.TransactionId,
            exception.GetStatus());
        co_return exception.GetStatus();
    }

    // Using invariant here instead of throw since the above GetMetadata under write lock does this check.
    ASSERT_IFNOT(
        metadataSPtr->TransientCreate == false,
        "{0}: RemoveSingleAsync: Metadata should not be TransientCreate.",
        TraceId);

    if (metadataSPtr->TransientDelete)
    {
        StateManagerEventSource::Events->ISPM_ApiError(
            TracePartitionId,
            ReplicaId,
            metadataSPtr->StateProviderId,
            "RemoveAsync: (Single) State Provider is being deleted.",
            ToStringLiteral(stateProviderName),
            transaction.TransactionId,
            SF_STATUS_NAME_DOES_NOT_EXIST);
        co_return SF_STATUS_NAME_DOES_NOT_EXIST;
    }

    // Mark transient delete
    metadataSPtr->TransientDelete = true;
    metadataSPtr->TransactionId = transaction.TransactionId;
    
    // ApiDispatcher::PrepareForRemoveAsync will trace if it fails
    status = co_await apiDispatcher_->PrepareForRemoveAsync(*metadataSPtr, transaction, cancellationToken);
    CO_RETURN_ON_FAILURE(status);

    MetadataOperationData::CSPtr metadataOperationDataCSPtr = nullptr;
    status = MetadataOperationData::Create(
        metadataSPtr->StateProviderId, 
        stateProviderName, 
        ApplyType::Delete, 
        GetThisAllocator(), 
        metadataOperationDataCSPtr);
    if (NT_SUCCESS(status) == false)
    {
        StateManagerEventSource::Events->ISPM_ApiError(
            TracePartitionId,
            ReplicaId,
            metadataSPtr->StateProviderId,
            "RemoveAsync: (Single) Create MetadataOperationData.",
            ToStringLiteral(stateProviderName),
            transaction.TransactionId,
            status);

        co_return status;
    }

    // ReplicateAsync will trace if it fails
    status = co_await this->ReplicateAsync(
        transaction, 
        *metadataOperationDataCSPtr, 
        nullptr, 
        timeout, 
        cancellationToken);
    CO_RETURN_ON_FAILURE(status);

    co_return status;
}

Awaitable<NTSTATUS> StateManager::ReplicateAsync(
    __in Transaction & transaction,
    __in OperationData const & metadataOperationData,
    __in OperationData const * const redoOperationData,
    __in Common::TimeSpan const & timeout,
    __in CancellationToken const & cancellationToken) noexcept
{
    ApiEntryAsync();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    ULONG32 exponentialBackoff = StartingBackOffInMs;
    ULONG32 retryCount = 0;

    Common::Stopwatch stopwatch;
    stopwatch.Start();

    while (cancellationToken.IsCancellationRequested == false)
    {
        status = transaction.AddOperation(&metadataOperationData, nullptr, redoOperationData, StateManagerId, nullptr);
        if (NT_SUCCESS(status))
        {
            // Success.
            co_return status;
        }

        if (TransactionBase::IsRetriableNTSTATUS(status) == false)
        {
            TraceError(L"ReplicateAsync: IsRetriableNTSTATUS", status);
            co_return status;
        }

        // Retry-able AddOperation error.
        if (stopwatch.Elapsed > timeout)
        {
            co_return SF_STATUS_TIMEOUT;
        }
        
        // Calculate the back off time.
        if (exponentialBackoff * 2 < MaxBackOffInMs)
        {
            exponentialBackoff *= 2;
        }
        else
        {
            exponentialBackoff = MaxBackOffInMs;
        }

        retryCount++;
        if (retryCount == MaxRetryCount)
        {
            // Note that we only trace that we are retrying.
            StateManagerEventSource::Events->AddOperation(
                TracePartitionId,
                ReplicaId,
                MaxRetryCount,
                transaction.TransactionId,
                exponentialBackoff);
            retryCount = 0;
        }

        status = co_await KTimer::StartTimerAsync(
            GetThisAllocator(), 
            GetThisAllocationTag(), 
            exponentialBackoff, 
            nullptr);
        if (NT_SUCCESS(status) == false)
        {
            TraceError(L"ReplicateAsync: StartTimerAsync", status);
            co_return status;
        }
    }

    co_return STATUS_CANCELLED;
}

void StateManager::ReleaseLockAndThrowIfOnFailure(
    __in LONG64 transactionId,
    __in StateManagerLockContext& stateManagerLockContext,
    __in NTSTATUS status) const
{
    if (NT_SUCCESS(status) == false)
    {
        NTSTATUS releaseLockStatus = stateManagerLockContext.ReleaseLock(transactionId);
        if (NT_SUCCESS(releaseLockStatus) == false)
        {
            TraceError(
                L"ReleaseLockAndThrowIfOnFailure: StateManagerLockContext::ReleaseLock",
                releaseLockStatus);
        }

        Helper::ThrowIfNecessary(
            status,
            TracePartitionId,
            ReplicaId,
            L"ReleaseLockAndThrowIfOnFailure.",
            Helper::StateManager);
        throw Exception(status);
    }
}

//
// Check the state provider is registered or not, should throw if not registered
//
// Algorithm:
// 1. If it is a state manager operation, return
// 2. Get the corresponding metadata
// 3. Throw if it is not Active or TransientDeleted
// 4. Throw if it is TransientDeleted and the transaction can see this uncommitted change when checkForDeleting marked true
//
NTSTATUS StateManager::IsRegistered(
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in bool checkForDeleting,
    __in LONG64 transactionId) noexcept
{
    // If it is the state manager operation, then it should not throw exception because the
    // state manager is conceptually registered by default.
    if (stateProviderId == StateManagerId)
    {
        return STATUS_SUCCESS;
    }

    Metadata::SPtr metadataSPtr = nullptr;
    try
    {
        // TryGetMetadata may throw exception. Catches it and returns NTSTATUS error code.
        bool isExist = metadataManagerSPtr_->TryGetMetadata(stateProviderId, metadataSPtr);
        if (!isExist)
        {
            // Activity acquired, so it cannot be closing.
            StateManagerEventSource::Events->NotRegistered(
                TracePartitionId,
                ReplicaId,
                stateProviderId,
                STATUS_INVALID_PARAMETER);
            return STATUS_INVALID_PARAMETER;
        }
    }
    catch (Exception & exception)
    {
        StateManagerEventSource::Events->NotRegistered(
            TracePartitionId,
            ReplicaId,
            stateProviderId,
            exception.GetStatus());
        return exception.GetStatus();
    }

    // If "checkForDeleting" is enabled, we need to check if the state provider is marked for transient delete 
    // and if this transaction can see this transient (uncommitted) delete. Note that when a transaction is inflight, 
    // its uncommitted changes should not be visible to other transactions (ISOLATION).
    if (checkForDeleting && metadataSPtr->TransientDelete && transactionId == metadataSPtr->TransactionId)
    {
        StateManagerEventSource::Events->NotRegistered(
            TracePartitionId,
            ReplicaId,
            stateProviderId,
            SF_STATUS_OBJECT_CLOSED);

        // 11908539: In managed this is FabricObjectClosedException. But SM is not closing. We should differentiate.
        return SF_STATUS_OBJECT_CLOSED;
    }

    return STATUS_SUCCESS;
}

__checkReturn NTSTATUS StateManager::CheckIfNameIsValid(
    __in KUriView const & stateProviderName,
    __in bool stateManagerNameAllowed) const noexcept
{
    if (stateProviderName.IsEmpty() == TRUE)
    {
        StateManagerEventSource::Events->Error(
            TracePartitionId,
            ReplicaId,
            L"CheckIfNameIsValid: state provider name is empty",
            SF_STATUS_INVALID_NAME_URI);
        return SF_STATUS_INVALID_NAME_URI;
    }

    if (stateProviderName.IsValid() == FALSE)
    {
        StateManagerEventSource::Events->Error(
            TracePartitionId,
            ReplicaId,
            L"CheckIfNameIsValid: state provider name is invalid",
            SF_STATUS_INVALID_NAME_URI);
        return SF_STATUS_INVALID_NAME_URI;
    }

    if (stateProviderName.Has(KUriView::UriFieldType::eScheme) == FALSE)
    {
        StateManagerEventSource::Events->Error(
            TracePartitionId,
            ReplicaId,
            L"CheckIfNameIsValid: state provider name doesn't have scheme",
            SF_STATUS_INVALID_NAME_URI);
        return SF_STATUS_INVALID_NAME_URI;
    }

    // Used a reserved name
    if ((!stateManagerNameAllowed) && (stateProviderName.Compare(StateManagerName) == TRUE))
    {
        StateManagerEventSource::Events->Error(
            TracePartitionId,
            ReplicaId,
            L"CheckIfNameIsValid: state provider name is reserved",
            SF_STATUS_INVALID_NAME_URI);
        return SF_STATUS_INVALID_NAME_URI;
    }

    return STATUS_SUCCESS;
}

__checkReturn NTSTATUS StateManager::CheckIfReadable(__in KUriView const & stateProviderName) const noexcept
{
    FABRIC_SERVICE_PARTITION_ACCESS_STATUS readStatus;
    NTSTATUS status = partitionSPtr_->GetReadStatus(&readStatus);

    if (NT_SUCCESS(status) == false)
    {
        StateManagerEventSource::Events->Error(
            TracePartitionId,
            ReplicaId,
            L"CheckIfReadable: partition GetReadStatus.",
            status);
        return status;
    }

    if (readStatus == FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED)
    {
        return STATUS_SUCCESS;
    }

    ILoggingReplicator::SPtr loggingReplicatorSPtr = GetReplicator();
    FABRIC_REPLICA_ROLE role = GetCurrentRole();

    // Note: Managed bug: Must not allow read if read status is not granted and the replica is Primary.
    // This can cause data integrity bug: For example during data loss.
    bool isReadable = loggingReplicatorSPtr->IsReadable();
    if (isReadable && role == FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY)
    {
        return STATUS_SUCCESS;
    }

    StateManagerEventSource::Events->ReadStatusNotGranted(
        TracePartitionId,
        ReplicaId,
        ToStringLiteral(stateProviderName),
        readStatus,
        isReadable,
        role);

    return SF_STATUS_NOT_READABLE;
}

// Checks whether a work should start.
// If write status is granted, or revoked due to potentially transient cases, let the write start.
// If not primary, return NTSTATUS error code.
NTSTATUS StateManager::CheckIfWritable(
    __in Common::StringLiteral const & apiName,
    __in Transaction const & transaction,
    __in KUriView const & stateProviderName) const noexcept
{
    FABRIC_SERVICE_PARTITION_ACCESS_STATUS writeStatus;
    NTSTATUS status = partitionSPtr_->GetWriteStatus(&writeStatus);
    if (NT_SUCCESS(status) == false)
    {
        StateManagerEventSource::Events->Error(
            TracePartitionId,
            ReplicaId,
            L"CheckIfWritable: partition GetWriteStatus.",
            status);
        return status;
    }

    ASSERT_IFNOT(
        writeStatus != FABRIC_SERVICE_PARTITION_ACCESS_STATUS_INVALID,
        "{0}: WriteStatus cannot be invalid.",
        TraceId);

    if (writeStatus == FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY)
    {
        StateManagerEventSource::Events->ISPM_ApiError(
            TracePartitionId,
            ReplicaId,
            FABRIC_INVALID_STATE_PROVIDER_ID,
            apiName,
            ToStringLiteral(stateProviderName),
            transaction.TransactionId,
            SF_STATUS_NOT_PRIMARY);

        return SF_STATUS_NOT_PRIMARY;
    }

    return STATUS_SUCCESS;
}

ILoggingReplicator::SPtr StateManager::GetReplicator() const
{
    KSharedPtr<ILoggingReplicator> loggingReplicatorSPtr = loggingReplicatorWRef_->TryGetTarget();
    ASSERT_IFNOT(loggingReplicatorSPtr != nullptr, "{0}: Null replicator object", TraceId);

    return Ktl::Move(loggingReplicatorSPtr);
}

void StateManager::TraceException(
    __in wstring const & message,
    __in Exception const & exception) const noexcept
{
    StateManagerEventSource::Events->Error(
        TracePartitionId,
        ReplicaId,
        message,
        exception.GetStatus());
}

void StateManager::TraceException(
    __in FABRIC_STATE_PROVIDER_ID stateProviderID,
    __in Common::WStringLiteral const & message,
    __in Exception const & exception) const noexcept
{ 
    StateManagerEventSource::Events->ISP2_ApiError(
        TracePartitionId,
        ReplicaId,
        stateProviderID,
        message,
        exception.GetStatus());
}

void StateManager::TraceError(
    __in wstring const & message,
    __in NTSTATUS status) const noexcept
{
    StateManagerEventSource::Events->Error(
        TracePartitionId,
        ReplicaId,
        message,
        status);
}

Awaitable<void> StateManager::InitializeStateProvidersAsync(
    __in Metadata & metadata, 
    __in bool shouldMetadataBeAdded)
{
    ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;
    KSharedArray<Metadata::SPtr>::SPtr stateProviderListSPtr = nullptr;

    ASSERT_IF(
        metadata.StateProvider == nullptr, 
        "{0}: Null state provider during initialization. SPid: {1} SecondaryInsertCase: {2}", 
        TraceId, 
        metadata.StateProviderId,
        shouldMetadataBeAdded);

    try
    {
        // Empty array indicates that this is a child.
        stateProviderListSPtr = InitializeStateProvidersInOrder(metadata);
        if (stateProviderListSPtr == nullptr)
        {
            co_return;
        }
    }
    catch (Exception & exception)
    {
        TraceException(L"InitializeStateProvidersAsync", exception);
        throw;
    }

    for (Metadata::SPtr stateProviderMetadataSPtr : *stateProviderListSPtr)
    {
        IStateProvider2::SPtr stateProviderSPtr = stateProviderMetadataSPtr->StateProvider;
        ASSERT_IFNOT(
            stateProviderMetadataSPtr->StateProvider != nullptr, 
            "{0}: Null state provider during initialization. SPId: {1}", 
            TraceId, 
            stateProviderMetadataSPtr->StateProviderId);

        // MCoskun: Order of the following operations is significant to avoid leaks (Leaving state providers not closed).
        // We must only add a state add to metadata manager once the state provider has been opened.
        status = co_await apiDispatcher_->OpenAsync(*stateProviderMetadataSPtr, CancellationToken::None);
        THROW_ON_FAILURE(status);

        status = co_await apiDispatcher_->RecoverCheckpointAsync(*stateProviderMetadataSPtr, CancellationToken::None);
        if (NT_SUCCESS(status) == false)
        {
            co_await this->CloseOpenedStateProviderOnFailureAsync(
                *stateProviderMetadataSPtr);
            THROW_ON_FAILURE(status);
        }

        status = co_await this->AcquireChangeRoleLockAsync();
        if (NT_SUCCESS(status) == false)
        {
            co_await this->CloseOpenedStateProviderOnFailureAsync(
                *stateProviderMetadataSPtr);
            THROW_ON_FAILURE(status);
        }
        KFinally([&] {changeRoleLockSPtr_->ReleaseWriteLock(); });

        FABRIC_REPLICA_ROLE role = GetCurrentRole();

        switch(role)
        {
        case FABRIC_REPLICA_ROLE_PRIMARY:
            status = co_await apiDispatcher_->ChangeRoleAsync(
                *stateProviderMetadataSPtr, 
                FABRIC_REPLICA_ROLE_PRIMARY, 
                CancellationToken::None);
            break;
        case FABRIC_REPLICA_ROLE_IDLE_SECONDARY:
            status = co_await apiDispatcher_->ChangeRoleAsync(
                *stateProviderMetadataSPtr, 
                FABRIC_REPLICA_ROLE_IDLE_SECONDARY, 
                CancellationToken::None);
            break;
        case FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY:
            status = co_await apiDispatcher_->ChangeRoleAsync(
                *stateProviderMetadataSPtr, 
                FABRIC_REPLICA_ROLE_IDLE_SECONDARY, 
                CancellationToken::None);
            if (NT_SUCCESS(status) == false)
            {
                co_await this->CloseOpenedStateProviderOnFailureAsync(
                    *stateProviderMetadataSPtr);
                THROW_ON_FAILURE(status);
            }

            status = co_await apiDispatcher_->ChangeRoleAsync(
                *stateProviderMetadataSPtr, 
                FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, 
                CancellationToken::None);
            break;
        default:
            ASSERT_IFNOT(
                false,
                "{0}: Unexpected Role {1}",
                TraceId,
                role);
            break;
        }

        // IF ChangeRoleAsync call on state provider failed, Close it if necessary and throw exception.
        if (NT_SUCCESS(status) == false)
        {
            co_await this->CloseOpenedStateProviderOnFailureAsync(
                *stateProviderMetadataSPtr);
            THROW_ON_FAILURE(status);
        }

        // Note: Move this part here instead of after Openasync since there is a race between Get(when TransientCreate set to false)
        // and state provider becomes available. Now it ensures the state provider is ready and then Get call can come in.
        // Here the state provider changed to active.
        if (shouldMetadataBeAdded)
        {
            bool result = metadataManagerSPtr_->TryAdd(*stateProviderMetadataSPtr->Name, *stateProviderMetadataSPtr);
            ASSERT_IFNOT(result, "{0}: Adding metadata for SPid {1} failed.", TraceId, stateProviderMetadataSPtr->StateProviderId);
        }
        else
        {
            metadataManagerSPtr_->ResetTransientState(*stateProviderMetadataSPtr->Name);
        }
    }

    co_return;
}

KSharedArray<Metadata::SPtr>::SPtr StateManager::InitializeStateProvidersInOrder(
    __in Metadata & metadata)
{
    NTSTATUS status;

    KSharedArray<IStateProvider2::SPtr>::SPtr childrenSPtr = nullptr;
    bool isParenStateProvider = metadataManagerSPtr_->GetChildren(metadata.StateProviderId, childrenSPtr);
    if (isParenStateProvider == false)
    {
        KString::CSPtr stateProviderWorkDirectory = CreateStateProviderWorkDirectory(metadata.StateProviderId);
        status = apiDispatcher_->Initialize(metadata, *transactionalReplicatorWRef_, *stateProviderWorkDirectory, nullptr);
        THROW_ON_FAILURE(status);
        // We can not remove it from parentToChildren because it not opened yet.
    }
    else
    {
        KString::CSPtr stateProviderWorkDirectory = CreateStateProviderWorkDirectory(metadata.StateProviderId);
        status = apiDispatcher_->Initialize(metadata, *transactionalReplicatorWRef_, *stateProviderWorkDirectory, childrenSPtr.RawPtr());
        THROW_ON_FAILURE(status);
    }

    // If this is a child, add it to the parent's children
    if (metadata.ParentId != EmptyStateProviderId)
    {
        metadataManagerSPtr_->AddChild(metadata.ParentId, metadata);
        return nullptr;
    }

    // If parent return all state Providers.
    KSharedArray<Metadata::SPtr>::SPtr output = _new(GetThisAllocationTag(), GetThisAllocator()) KSharedArray<Metadata::SPtr>();
    THROW_ON_ALLOCATION_FAILURE(output);
    Helper::ThrowIfNecessary(
        output->Status(),
        TracePartitionId,
        ReplicaId,
        L"InitializeStateProvidersInOrder: Create KSharedArray.",
        Helper::StateManager);

    AddChildrenToList(metadata, output);
    return output;
}

NTSTATUS StateManager::PrepareCheckpointOnStateProviders(
    __in FABRIC_SEQUENCE_NUMBER checkpointLSN) noexcept
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    KArray<Metadata::CSPtr> stateProviders(checkpointManagerSPtr_->GetPreparedActiveOrDeletedList(MetadataMode::Active));

    status = apiDispatcher_->PrepareCheckpoint(stateProviders, checkpointLSN);

    return status;
}

Awaitable<NTSTATUS> StateManager::PerformCheckpointOnStateProvidersAsync(
    __in CancellationToken const & cancellationToken) noexcept
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    // Call perform checkpoint on the relevant state providers.
    // Note: Managed bug. The sm should not read the "checkpointed state" out side of the lock.
    // Hence here instead we read from the prepareCheckpointMetadataSnapshot which cannot change since no PrepareCheckpoint, Close/Abort or 
    // CompleteCheckpoint could be inflight at this point of time.

    KArray<Metadata::CSPtr> stateProviders(checkpointManagerSPtr_->GetPreparedActiveOrDeletedList(MetadataMode::Active));
    status = co_await apiDispatcher_->PerformCheckpointAsync(stateProviders, cancellationToken);
 
    StateManagerEventSource::Events->PerformCheckpointSP(
        TracePartitionId,
        ReplicaId,
        stateProviders.Count());

    co_return status;
}

Awaitable<NTSTATUS> StateManager::CompleteCheckpointOnStateProvidersAsync(
    __in CancellationToken const& cancellationToken) noexcept
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    KArray<Metadata::CSPtr> stateProviders = checkpointManagerSPtr_->GetCopyOrCheckpointActiveList();
    status = co_await apiDispatcher_->CompleteCheckpointAsync(stateProviders, cancellationToken);

    co_return status;
}

void StateManager::AddChildrenToList(
    __in Metadata & metadata, 
    __inout KSharedArray<Metadata::SPtr>::SPtr& children)
{
    NTSTATUS status = children->Append(&metadata);
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"AddChildrenToList: Append metadata to children array.",
        Helper::StateManager);

    KSharedArray<Metadata::SPtr>::SPtr myChildren = nullptr;
    bool removeSuccess = metadataManagerSPtr_->TryRemoveParent(metadata.StateProviderId, myChildren);
    if (removeSuccess == false)
    {
        return;
    }

    for(ULONG32 i = 0; i < myChildren->Count(); i++)
    {
        AddChildrenToList(*(*myChildren)[i], children);
    }
}

KString::CSPtr StateManager::CreateStateProviderWorkDirectory(__in FABRIC_STATE_PROVIDER_ID stateProviderId)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    KString::CSPtr stateProviderWorkDirectory = GetStateProviderWorkDirectory(stateProviderId);

    if (hasPersistedState_ == false)
    {
        // Returning path in case volatile state provider really needs it
        return stateProviderWorkDirectory;
    }

    // #11908559: Once we have the serialization mode configuration plumbed through, we can check if SerailiationMode == Managed (v1).
    // If so we do not need to check if we need to upgrade the folder structure.
    {
        bool isMoved;
        status = UpgradeStateProviderFolderFrom0to1IfNecessary(
            stateProviderId, 
            *stateProviderWorkDirectory,
            isMoved);
        THROW_ON_FAILURE(status);

        if (isMoved == true)
        {
            return stateProviderWorkDirectory;
        }
    }

    status = Helper::CreateFolder(*stateProviderWorkDirectory);
    if (NT_SUCCESS(status) == false)
    {
        wstring message = Common::wformatString(
            "State Provider folder could not be created. SPID: {0} Folder Path: {1} Status: {2}",
            stateProviderId, 
            wstring(static_cast<LPCWSTR>(*stateProviderWorkDirectory)),
            status);

        StateManagerEventSource::Events->Warning(
            TracePartitionId,
            ReplicaId,
            message);

        throw ktl::Exception(status);
    }

    return stateProviderWorkDirectory;
}

NTSTATUS StateManager::UpgradeStateProviderFolderFrom0to1IfNecessary(
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in KString const & destination,
    __out bool & isMoved) noexcept
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    wstring version0StateProviderFolderPath;
    status = GetVersion0StateProviderFolderPath(stateProviderId, version0StateProviderFolderPath);
    RETURN_ON_FAILURE(status);

    bool folderExists = Common::Directory::Exists(version0StateProviderFolderPath);
    if (folderExists)
    {
        Common::ErrorCode errorCode = Common::Directory::Rename(
            version0StateProviderFolderPath,
            static_cast<LPCWSTR>(destination),
            false); // overwrite

        // Invariant: If version 0 exists, version 1 state provider format cannot exist.
        // This is because MoveFile on directory is atomic.
        ASSERT_IF(
            errorCode.IsErrno(ERROR_ALREADY_EXISTS),
            "{0}: Destination folder must not exist. Src: {1} Dest: {2}",
            TraceId,
            version0StateProviderFolderPath,
            ToStringLiteral(destination));

        if (errorCode.IsSuccess() == false)
        {
            return StatusConverter::Convert(errorCode.ToHResult());
        }

        isMoved = true;
        return status;
    }

    isMoved = false;
    return status;
}

// Version 0 format: ~work/<pid>_<rid>_<spid>
NTSTATUS StateManager::GetVersion0StateProviderFolderPath(
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __out wstring & path) noexcept
{
    ASSERT_IFNOT(path.empty(), "{0}: Input path must be empty. {1}", TraceId, path);

    wstring folderPath;
    folderPath.reserve(178);
    Common::StringWriter stringWriter(folderPath, 178);
    stringWriter.Write(TracePartitionId.ToString('N'));
    stringWriter.Write('_');
    stringWriter.Write(ReplicaId);
    stringWriter.Write('_');
    stringWriter.Write(stateProviderId);

    wstring workFolderPath(runtimeFolders_->get_WorkDirectory());
    Common::Path::CombineInPlace(workFolderPath, folderPath);

    path = workFolderPath;
    return STATUS_SUCCESS;
}

KString::CSPtr StateManager::GetStateProviderWorkDirectory(__in FABRIC_STATE_PROVIDER_ID stateProviderId) const
{
    KLocalString<20> stateProviderIdString;
    BOOLEAN boolStatus = stateProviderIdString.FromLONGLONG(stateProviderId);
    ASSERT_IFNOT(
        boolStatus == TRUE,
        "{0}: GetStateProviderWorkDirectory: StateProviderIdString build failed.",
        TraceId);

    KString::SPtr folderName = Utilities::KPath::Combine(*workDirectoryPath_, stateProviderIdString, GetThisAllocator());

    return folderName.RawPtr();
}

bool StateManager::TryDeleteStateProviderWorkDirectory(__in FABRIC_STATE_PROVIDER_ID stateProviderId)
{
    KString::CSPtr directory(GetStateProviderWorkDirectory(stateProviderId));

    Common::ErrorCode errorCode = Common::Directory::Delete(wstring(*directory), true);
    if (errorCode.IsSuccess() == false)
    {
        StateManagerEventSource::Events->DeleteSPWorkDirectory(
            TracePartitionId,
            ReplicaId,
            ToStringLiteral(*directory));

        return false;
    }

    return true;
}

KHashSet<KString::CSPtr>::SPtr StateManager::GetCurrentStateProviderDirectories()
{
    ULONG stateCount = metadataManagerSPtr_->GetInMemoryStateProvidersCount() + metadataManagerSPtr_->GetDeletedStateProvidersCount();

    // Compare the stateCount with default NumberOfBuckets to avoid the situation that the stateCount is 0.
    KHashSet<KString::CSPtr>::SPtr referencedDirectories;
    NTSTATUS status = KHashSet<KString::CSPtr>::Create(
        stateCount > Constants::NumberOfBuckets ? stateCount : Constants::NumberOfBuckets,
        K_DefaultHashFunction,
        Comparer::Equals,
        GetThisAllocator(),
        referencedDirectories);
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"GetCurrentStateProviderDirectories: Create referencedDirectories KHashSet.",
        Helper::StateManager);

    auto activeStateProviderEnumerator = metadataManagerSPtr_->GetMetadataCollection();
    while (activeStateProviderEnumerator->MoveNext())
    {
        auto row = activeStateProviderEnumerator->Current();
        FABRIC_STATE_PROVIDER_ID stateProviderId = row.Value->StateProviderId;
        referencedDirectories->TryAdd(GetStateProviderWorkDirectory(stateProviderId));
    }

    KSharedArray<Metadata::CSPtr>::SPtr deletedStateProviders = nullptr;
    status= metadataManagerSPtr_->GetDeletedConstMetadataArray(deletedStateProviders);
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"GetCurrentStateProviderDirectories: GetDeletedConstMetadataArray.",
        Helper::StateManager);

    for (Metadata::CSPtr metadata : *deletedStateProviders)
    {
        FABRIC_STATE_PROVIDER_ID stateProviderId = metadata->StateProviderId;
        referencedDirectories->TryAdd(GetStateProviderWorkDirectory(stateProviderId));
    }

    return Ktl::Move(referencedDirectories);
}

KArray<KString::CSPtr> StateManager::GetDirectoriesInWorkDirectory()
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    KArray<KString::CSPtr> workDirectoryArray(GetThisAllocator());
    Helper::ThrowIfNecessary(
        workDirectoryArray.Status(),
        TracePartitionId,
        ReplicaId,
        L"GetDirectoriesInWorkDirectory: Create workDirectoryArray.",
        Helper::StateManager);

    // #8734667: Use KVolumeNamespace::QueryDirectories()
    wstring smFolder(*workDirectoryPath_);

    vector<wstring> directoryVector = Common::Directory::GetSubDirectories(smFolder, L"*", true, true);
    for (wstring folder : directoryVector)
    {
        KString::SPtr folderPath;
        status = KString::Create(folderPath, GetThisAllocator(), KStringView(folder.c_str()));
        Helper::ThrowIfNecessary(
            status,
            TracePartitionId,
            ReplicaId,
            L"GetDirectoriesInWorkDirectory: Create folder KStringSPtr.",
            Helper::StateManager);

        status = workDirectoryArray.Append(folderPath.RawPtr());
        Helper::ThrowIfNecessary(
            status,
            TracePartitionId,
            ReplicaId,
            L"GetDirectoriesInWorkDirectory: Append folderPath to workDirectoryArray.",
            Helper::StateManager);
    }

    return Ktl::Move(workDirectoryArray);
}

FABRIC_STATE_PROVIDER_ID StateManager::CreateStateProviderId() noexcept
{
    FABRIC_STATE_PROVIDER_ID stateProviderId = KDateTime::Now();
    K_LOCK_BLOCK(stateProviderIdLock_)
    {
        if (stateProviderId > lastStateProviderId_)
        {
            lastStateProviderId_ = stateProviderId;
        }
        else
        {
            lastStateProviderId_++;
            stateProviderId = lastStateProviderId_;
        }
    }

    return stateProviderId;
}

void StateManager::UpdateLastStateProviderId(__in FABRIC_STATE_PROVIDER_ID id) noexcept
{
    K_LOCK_BLOCK(stateProviderIdLock_)
    {
        if (id > lastStateProviderId_)
        {
            lastStateProviderId_ = id;
        }
    }
}

// Delete tombstones: Delete state providers that no longer are needed for copy or recovery.
//
// Algorithm:
// 1. Get the Safe LSN from the logging replicator.
// 2. Get all stale state providers.
// 3. Mark and delete the persisted state of state providers whose LSN < SafeLSN and close them.
// 4. Remove marked stale state providers from the stale list and copy snap.
Awaitable<void> StateManager::CleanUpMetadataAsync(
    __in CancellationToken const & cancellationToken)
{
    ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    try
    {
        // 1. Get the Safe LSN from the logging replicator.
        ILoggingReplicator::SPtr loggingReplicator = GetReplicator();
        FABRIC_SEQUENCE_NUMBER cleanUpLSN = FABRIC_INVALID_SEQUENCE_NUMBER;
        status = loggingReplicator->GetSafeLsnToRemoveStateProvider(cleanUpLSN);
        THROW_ON_FAILURE(status);

        StateManagerEventSource::Events->Cleanup(
            TracePartitionId,
            ReplicaId,
            static_cast<int>(cleanUpLSN));

        // 2. Get all stale state providers.
        KSharedArray<Metadata::CSPtr>::SPtr staleStateProviders = nullptr;
        status = metadataManagerSPtr_->GetDeletedConstMetadataArray(staleStateProviders);
        THROW_ON_FAILURE(status);

        // 3. Mark and delete the persisted state of state providers whose LSN < SafeLSN and close them.
        for (Metadata::CSPtr metadata : *staleStateProviders)
        {
            bool shouldBeDeleted = false;

            if (metadata->MetadataMode == MetadataMode::DelayDelete)
            {
                ASSERT_IFNOT(
                    metadata->DeleteLSN != FABRIC_INVALID_SEQUENCE_NUMBER,
                    "{0}: Delete LSN has not been set. SPid: {1} Name: {2} Mode: {3}",
                    TraceId,
                    metadata->StateProviderId,
                    ToStringLiteral(*(metadata->Name)),
                    metadata->MetadataMode);

                if (metadata->DeleteLSN < cleanUpLSN)
                {
                    shouldBeDeleted = true;
                }
            }
            else
            {
                // For stale state providers, delete LSN might or might not be present.
                // It is important to check the LSN before calling remove state as copy replay 
                // can happen during checkpoint.
                FABRIC_SEQUENCE_NUMBER removeLSN = metadata->DeleteLSN;
                if (removeLSN == FABRIC_INVALID_SEQUENCE_NUMBER)
                {
                    removeLSN = metadata->CreateLSN;
                }

                ASSERT_IFNOT(
                    metadata->CreateLSN != FABRIC_INVALID_SEQUENCE_NUMBER,
                    "{0}: Create LSN has not been set. SPid: {1} Name: {2} Mode: {3}",
                    TraceId,
                    metadata->StateProviderId,
                    ToStringLiteral(*(metadata->Name)),
                    metadata->MetadataMode);

                if (removeLSN < cleanUpLSN)
                {
                    shouldBeDeleted = true;
                }
            }

            // Delete the persisted state of state providers whose LSN < SafeLSN and close them.
            if (shouldBeDeleted)
            {
                // The deleted state provider array may have DelayDelete, FalseProgress or Active(surrect). But
                // after the CleanupLSN check, it must only contain the actual deleted which can only be either DelayDelete or FalseProgress
                ASSERT_IFNOT(
                    metadata->MetadataMode == MetadataMode::DelayDelete || metadata->MetadataMode == MetadataMode::FalseProgress,
                    "{0}: Metadata mode must be DelayDelete | FalseProgress. SPid: {1} Name: {2} Mode: {3}",
                    TraceId,
                    metadata->StateProviderId,
                    ToStringLiteral(*(metadata->Name)),
                    metadata->MetadataMode);

                StateManagerEventSource::Events->CleanupDelete(
                    TracePartitionId,
                    ReplicaId,
                    metadata->StateProviderId,
                    static_cast<int>(cleanUpLSN));

                // Note: Difference with managed: Change Role to None before RemoveStateAsync.
                status = co_await apiDispatcher_->ChangeRoleAsync(
                    *metadata,
                    FABRIC_REPLICA_ROLE_NONE, 
                    cancellationToken);
                THROW_ON_FAILURE(status);

                status = co_await apiDispatcher_->RemoveStateAsync(*metadata, cancellationToken);
                THROW_ON_FAILURE(status);

                // Note: Difference with managed: Close the state provider gracefully.
                status = co_await apiDispatcher_->CloseAsync(
                    *metadata, 
                    ApiDispatcher::FailureAction::AbortStateProvider,
                    cancellationToken);
                ASSERT_IFNOT(
                    NT_SUCCESS(status),
                    "{0}: ApiDispatcher::CloseAsync returned failed status even though FaultAction = AbortStateProvider: {1}",
                    TraceId,
                    status);

                // Note: Difference with managed: Delete the state providers folder.
                TryDeleteStateProviderWorkDirectory(metadata->StateProviderId);

                // 4. Remove marked stale state providers from the stale list and copy snap.
                // #10202670: In managed we used to remove these in a separate loop.
                // That is both in-efficient and causes some state providers to be closed if any part of this (CR, RemoveStateAsync or CloseAsync) fail.
                // Since there may be successfully closed state providers that are not removed.
                Metadata::SPtr removedMetadata = nullptr;
                bool isRemoved = metadataManagerSPtr_->TryRemoveDeleted(metadata->StateProviderId, removedMetadata);
                ASSERT_IFNOT(isRemoved, "{0}: Failed to remove SPID {1} from the delete list", TraceId, metadata->StateProviderId);

                checkpointManagerSPtr_->RemoveStateProvider(metadata->StateProviderId);
            }
        }
    }
    catch (Exception & exception)
    {
        TraceException(L"CleanUpMetadataAsync", exception);
        throw;
    }

    co_return;
}

// Removes state provider folders for state providers that do not exist in the memory.
// Note that stale is considered referenced because it may be resurrected.
// 
// Algorithm:
// 1. Get all state provider folders that are expected to exist (active or stale in memory)
// 2. Get all state provider folders from file system.
// 3. Mark all state provider folders in the file system but not in-memory as "to be deleted"
// 4. Delete all marked folders.
void StateManager::RemoveUnreferencedStateProviders()
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    StateManagerEventSource::Events->RemoveUnreferencedSP(
        TracePartitionId,
        ReplicaId,
        ToStringLiteral(*workDirectoryPath_));

    Common::Stopwatch stopwatch;
    stopwatch.Start();

    // 1. Get all state provider folders that are expected to exist (active or stale in memory)
    KHashSet<KString::CSPtr>::SPtr referencedDirectories(GetCurrentStateProviderDirectories());
    KArray<KString::CSPtr> directoriesToBeDeleted(GetThisAllocator());

    // 2. Get all state provider folders from file system.
    KArray<KString::CSPtr> currentStateProviderDirectories(GetDirectoriesInWorkDirectory());

    // 3. Mark all state provider folders in the file system but not in-memory as "to be deleted"
    for (ULONG32 index = 0; index < currentStateProviderDirectories.Count(); index++)
    {
        KString::CSPtr currentFolder = currentStateProviderDirectories[index];

        // If this table file does not exist in the hash set, it must not be referenced - so delete it.
        bool isReferenced = referencedDirectories->TryRemove(currentFolder);
        if (isReferenced == false)
        {
            status = directoriesToBeDeleted.Append(currentFolder);
            Helper::ThrowIfNecessary(
                status,
                TracePartitionId,
                ReplicaId,
                L"RemoveUnreferencedStateProviders: Append currentFolder to directoriesToBeDeleted array.",
                Helper::StateManager);

            StateManagerEventSource::Events->RemoveUnreferencedSPMark(
                TracePartitionId,
                ReplicaId,
                ToStringLiteral(*currentFolder));
        }
    }

    // We expect all referencedDirectories to exist in the file system.
    ASSERT_IFNOT(
        referencedDirectories->Count() == 0,
        "{0}: Failed to find all referenced directories when trimming unreferenced directories. NOT deleting any directory.",
        TraceId);

    // 4. Delete all marked folders.
    for (ULONG32 i = 0; i < directoriesToBeDeleted.Count(); i++)
    {
        KString::CSPtr directory = directoriesToBeDeleted[i];
        StateManagerEventSource::Events->RemoveUnreferencedSPDelete(
            TracePartitionId,
            ReplicaId,
            ToStringLiteral(*directory));

        Common::ErrorCode errorCode = Common::Directory::Delete(wstring(*directory), true);
        if (errorCode.IsSuccess() == false)
        {
            // MCoskun: Consider ignoring this since it is not fatal.
            status = StatusConverter::Convert(errorCode.ToHResult());
            StateManagerEventSource::Events->RemoveUnreferencedSPDeleteErr(
                TracePartitionId,
                ReplicaId,
                ToStringLiteral(*directory),
                status);

            throw Exception(status);
        }
    }

    stopwatch.Stop();

#ifdef DBG
    ASSERT_IF(
        stopwatch.ElapsedMilliseconds >= Constants::MaxRemoveUnreferencedStateProvidersTime,
        "{0}: RemoveUnreferencedStateProviders must take less than 30 seconds.Total: {1} ms",
        TraceId,
        stopwatch.ElapsedMilliseconds);
#else
    if (stopwatch.ElapsedMilliseconds >= Constants::MaxRemoveUnreferencedStateProvidersTime)
    {
        StateManagerEventSource::Events->RemoveUnreferencedSPTooLong(
            TracePartitionId,
            ReplicaId,
            stopwatch.ElapsedMilliseconds);
    }
#endif

    StateManagerEventSource::Events->RemoveUnreferencedSPCompleted(
        TracePartitionId,
        ReplicaId,
        ToStringLiteral(*workDirectoryPath_),
        stopwatch.ElapsedMilliseconds);
}

bool StateManager::Contains(
    __in KArray<Metadata::CSPtr> const & metadataArray,
    __in FABRIC_STATE_PROVIDER_ID stateProviderId)
{
    for (Metadata::CSPtr metadata : metadataArray)
    {
        if (metadata->StateProviderId == stateProviderId)
        {
            return true;
        }
    }

    return false;
}

Awaitable<NTSTATUS> StateManager::AcquireChangeRoleLockAsync() noexcept
{
    try
    {
        co_await changeRoleLockSPtr_->AcquireWriteLockAsync(LONG_MAX);
    }
    catch (Exception & e)
    {
        TraceException(L"AcquireChangeRoleLockAsync", e);
        co_return e.GetStatus();
    }

    co_return STATUS_SUCCESS;
}

void StateManager::ReleaseChangeRoleLock() noexcept
{
    // Assumption: If the lock has been taken, release cannot fail.
    changeRoleLockSPtr_->ReleaseWriteLock();
}

ktl::Awaitable<void> StateManager::CloseOpenedStateProviderOnFailureAsync(
    __in Metadata & metadata) noexcept
{
    NTSTATUS closeStatus = co_await apiDispatcher_->CloseAsync(
        metadata,
        ApiDispatcher::FailureAction::AbortStateProvider,
        CancellationToken::None);
    ASSERT_IFNOT(
        NT_SUCCESS(closeStatus),
        "{0}: ApiDispatcher::CloseAsync returned failed status even though FaultAction = AbortStateProvider: {1}, SPId: {2}",
        TraceId,
        closeStatus,
        metadata.StateProviderId);
}

 NTSTATUS StateManager::Copy(
     __in KUriView const & uriView,
     __out KUri::CSPtr & outStateProviderName) const noexcept
{
    // #9022212 KUri.Create to support const KUriView
    KUriView & tmpUriView = const_cast<KUriView &>(uriView);
    KUri::CSPtr copyStateProviderName;
    NTSTATUS status = KUri::Create(tmpUriView, GetThisAllocator(), (KUri::SPtr&)copyStateProviderName);
    if (NT_SUCCESS(status) == false)
    {
        StateManagerEventSource::Events->Error(
            TracePartitionId,
            ReplicaId,
            L"Copy: Create outStateProviderName KUriCSPtr.",
            status);
        return status;
    }

    outStateProviderName = copyStateProviderName;
    return status;
}

NTSTATUS StateManager::Copy(
    __in KStringView const & stringView,
    __out KString::CSPtr & outString) const noexcept
{
    KString::SPtr copyString;
    NTSTATUS status = KString::Create(copyString, GetThisAllocator(), stringView);
    if (NT_SUCCESS(status) == false)
    {
        StateManagerEventSource::Events->Error(
            TracePartitionId,
            ReplicaId,
            L"Copy: Create outString KStringCSPtr.",
            status);
        return status;
    }

    outString = copyString.RawPtr();
    return status;
}

NTSTATUS StateManager::GetCurrentState(
    __in FABRIC_REPLICA_ID targetReplicaId,
    __out OperationDataStream::SPtr & result) noexcept
{
    ApiEntryReturn();

    StateManagerEventSource::Events->GetCurrentState(
        TracePartitionId,
        ReplicaId,
        targetReplicaId);

    try
    {
        result = checkpointManagerSPtr_->GetCurrentState(targetReplicaId);
    }
    catch (Exception & e)
    {
        return e.GetStatus();
    }

    return STATUS_SUCCESS;
}

ktl::Awaitable<NTSTATUS> StateManager::BeginSettingCurrentStateAsync() noexcept
{
    ApiEntryAsync();

    try
    {
        BeginSetCurrentLocalState();
    }
    catch (Exception & e)
    {
        co_return e.GetStatus();
    }

    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> StateManager::SetCurrentStateAsync(
    __in LONG64 stateRecordNumber,
    __in OperationData const & data) noexcept
{
    ApiEntryAsync();
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    CopyNamedOperationData::CSPtr namedOperationData;
    status = CopyNamedOperationData::Create(GetThisAllocator(), data, namedOperationData);
    if (NT_SUCCESS(status) == false)
    {
        TraceError(L"SetCurrentStateAsync: Create NamedOperationData.", status);
        co_return status;
    }

    ASSERT_IF(
        namedOperationData->StateProviderId == EmptyStateProviderId,
        "{0}: state manager de-serialization error on set current state",
        TraceId);

    StateManagerEventSource::Events->SetCurrentState(
        TracePartitionId,
        ReplicaId,
        stateRecordNumber,
        namedOperationData->StateProviderId);

    try
    {
        if (namedOperationData->StateProviderId == StateManagerId)
        {
            co_await SetCurrentLocalStateAsync(stateRecordNumber, *namedOperationData->UserOperationDataCSPtr, CancellationToken::None);
            co_return STATUS_SUCCESS;
        }

        // check if it is in the delete list
        if (metadataManagerSPtr_->IsStateProviderDeletedOrStale(namedOperationData->StateProviderId, MetadataMode::DelayDelete))
        {
            Metadata::SPtr metadata;
            bool isDeleted = metadataManagerSPtr_->TryGetDeletedMetadata(namedOperationData->StateProviderId, metadata);
            ASSERT_IFNOT(isDeleted, "{0}: State Provider {1} must be in the deleted list.", TraceId, namedOperationData->StateProviderId)
            co_return SF_STATUS_OBJECT_CLOSED; 
        }

        // TryGetMetadata may throw exception. Catches it and returns NTSTATUS error code.
        Metadata::SPtr metadataSPtr = nullptr;
        bool isExist = metadataManagerSPtr_->TryGetMetadata(namedOperationData->StateProviderId, metadataSPtr);
        ASSERT_IFNOT(
            isExist,
            "{0}: MetadataSPtr cannot be null. SPID {1}",
            TraceId,
            namedOperationData->StateProviderId);
        ASSERT_IFNOT(
            metadataSPtr != nullptr,
            "{0}: MetadataSPtr can not be null if TryGetMetadata return true. SPID {1}",
            TraceId,
            namedOperationData->StateProviderId);

#ifdef DBG
        ASSERT_IFNOT(
            Contains(copyProgressArray_, namedOperationData->StateProviderId),
            "{0}: SPid {1} not in the copy progress",
            TraceId,
            namedOperationData->StateProviderId);
#endif

        status = co_await apiDispatcher_->SetCurrentStateAsync(
            *metadataSPtr,
            stateRecordNumber,
            *namedOperationData->UserOperationDataCSPtr,
            CancellationToken::None);
        CO_RETURN_ON_FAILURE(status);
    }
    catch (Exception & e)
    {
        co_return e.GetStatus();
    }
 
    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> StateManager::EndSettingCurrentState() noexcept
{
    ApiEntryAsync();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    status = co_await this->EndSettingCurrentLocalStateAsync(CancellationToken::None);
    CO_RETURN_ON_FAILURE(status);

    // EndSetCurrentState needs to be called on all state providers where BeginSetCurrentState was called. 
    // It cannot be called on all state providers present in in-memory state since there can be 
    // state providers created after SetCurrentState and BeginSetCurrentState has not be called for them.
    status = co_await apiDispatcher_->EndSettingCurrentStateAsync(copyProgressArray_, CancellationToken::None);
    CO_RETURN_ON_FAILURE(status);

    copyProgressArray_.Clear();

    StateManagerEventSource::Events->EndSettingCurrentState(
        TracePartitionId,
        ReplicaId);

    co_return STATUS_SUCCESS;
}

#pragma region Notifications
// Notifies user change handler that state manager has been rebuilt.
// 
// Algorithm:
// 1. Check if an eventHandler is registered. If not return.
// 2. Get TransactionalReplicator to be passed in as "source"
// 3. Call OnRebuilt
// Assumption: Input state providers enumerator is parents only.
NTSTATUS StateManager::NotifyStateManagerStateChanged(
    __in Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, IStateProvider2::SPtr>> & stateProviders) noexcept
{
    // If there is no registered handler, return.
    IStateManagerChangeHandler::SPtr eventHandler = changeHandlerCache_.Get();
    if (eventHandler == nullptr)
    {
        return STATUS_SUCCESS;
    }

    ITransactionalReplicator::SPtr txnReplicator;
    txnReplicator = transactionalReplicatorWRef_->TryGetTarget();
    ASSERT_IFNOT(txnReplicator != nullptr, "{0}: TransactionalReplicator cannot have been closed.", TraceId);

    try
    {
        // #11908602: We should add ApiMonitoring here.
        eventHandler->OnRebuilt(*txnReplicator, stateProviders);
    }
    catch (Exception & e)
    {
        TraceError(L"Api.OnRebuilt", e.GetStatus());
        return e.GetStatus();
    }

    return STATUS_SUCCESS;
}

// Notifies user change handler that a state provider has been added or removed.
// Note that this is called before Unlock.
//
// Algorithm:
// 1. Check if parent (we do not provide children notifications today to avoid customers seeing internal details.)
//    *This may have to change with customer state provider support.
// 2. Check if an eventHandler is registered. If not return.
// 3. Get TransactionalReplicator to be passed in as "source"
// 4. Call OnAdded | OnRemoved.
NTSTATUS StateManager::NotifyStateManagerStateChanged(
    __in TransactionBase const & transactionBase,
    __in KUri const & name,
    __in IStateProvider2 & stateProvider,
    __in FABRIC_STATE_PROVIDER_ID parentStateProviderId,
    __in NotifyStateManagerChangedAction::Enum action) noexcept
{
    // Avoid the lock if not necessary
    if (parentStateProviderId != Constants::EmptyStateProviderId)
    {
        return STATUS_SUCCESS;
    }

    // If there is no registered handler, return.
    IStateManagerChangeHandler::SPtr eventHandler = changeHandlerCache_.Get();
    if (eventHandler == nullptr)
    {
        return STATUS_SUCCESS;
    }

    ITransactionalReplicator::SPtr txnReplicator;
    txnReplicator = transactionalReplicatorWRef_->TryGetTarget();
    ASSERT_IFNOT(txnReplicator != nullptr, "{0}: TransactionalReplicator cannot have been closed.", TraceId);

    ITransaction const * txnPtr = dynamic_cast<ITransaction const *>(&transactionBase);
    ASSERT_IFNOT(txnPtr != nullptr, "{0}: TxnBase must be an ITransaction.", TraceId);

    ASSERT_IFNOT(
        action == NotifyStateManagerChangedAction::Add || NotifyStateManagerChangedAction::Remove,
        "{0}: Unexpected action",
        TraceId);

    try
    {
        // #11908602: We should add ApiMonitoring here.
        if (action == NotifyStateManagerChangedAction::Add)
        {
            eventHandler->OnAdded(*txnReplicator, *txnPtr, name, stateProvider);
            return STATUS_SUCCESS;
        }

        eventHandler->OnRemoved(*txnReplicator, *txnPtr, name, stateProvider);
    }
    catch (Exception & e)
    {
        TraceError(L"Api.OnAdded or OnRemoved", e.GetStatus());
        return e.GetStatus();
    }

    return STATUS_SUCCESS;
}
#pragma endregion Notifications

FAILABLE StateManager::StateManager(
    __in PartitionedReplicaId const & traceId,
    __in IRuntimeFolders & runtimeFolders,
    __in IStatefulPartition & partition,
    __in IStateProvider2Factory & stateProviderFactory,
    __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
    __in bool hasPersistedState) noexcept
    : KAsyncServiceBase()
    , KWeakRefType<StateManager>()
    , PartitionedReplicaTraceComponent(traceId)
    , runtimeFolders_(&runtimeFolders)
    , partitionSPtr_(&partition)
    , workDirectoryPath_(Ktl::Move(CreateReplicaFolderPath(runtimeFolders.get_WorkDirectory(), traceId.PartitionId, traceId.ReplicaId, GetThisAllocator())))
    , copyProgressArray_(GetThisAllocator())
    , changeHandlerCache_(nullptr)
    , apiDispatcher_(ApiDispatcher::Create(traceId, stateProviderFactory, GetThisAllocator()))
    , transactionalReplicatorConfig_(transactionalReplicatorConfig)
    , serializationMode_(static_cast<SerializationMode::Enum>(transactionalReplicatorConfig->SerializationVersion))
    , hasPersistedState_(hasPersistedState)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (hasPersistedState)
    {
        status = Helper::CreateFolder(*workDirectoryPath_);
        if (NT_SUCCESS(status) == false)
        {
            SetConstructorStatus(status);
            return;
        }
    }

    if (NT_SUCCESS(copyProgressArray_.Status()) == false)
    {
        SetConstructorStatus(copyProgressArray_.Status());
        return;
    }

    status = MetadataManager::Create(*(this->PartitionedReplicaIdentifier), GetThisAllocator(), metadataManagerSPtr_);
    if (NT_SUCCESS(status) == false)
    {
        SetConstructorStatus(status);
        return;
    }

    status = CheckpointManager::Create(
        *(this->PartitionedReplicaIdentifier), 
        *apiDispatcher_,
        runtimeFolders.get_WorkDirectory(),
        *workDirectoryPath_,
        serializationMode_,
        GetThisAllocator(),
        checkpointManagerSPtr_);

    if (NT_SUCCESS(status) == false)
    {
        SetConstructorStatus(status);
        return;
    }

    status = ReaderWriterAsyncLock::Create(GetThisAllocator(), GetThisAllocationTag(), changeRoleLockSPtr_);
    if (NT_SUCCESS(status) == false)
    {
        SetConstructorStatus(status);
        return;
    }

    StateManagerEventSource::Events->Ctor(
        Common::Guid(traceId.PartitionId),
        traceId.ReplicaId,
        ToStringLiteral(*workDirectoryPath_),
        static_cast<int>(serializationMode_));

    // Happy path return.
    SetConstructorStatus(status);
}

StateManager::~StateManager()
{
    StateManagerEventSource::Events->Dtor(
        TracePartitionId,
        ReplicaId,
        ToStringLiteral(*workDirectoryPath_),
        static_cast<int>(serializationMode_));
}
