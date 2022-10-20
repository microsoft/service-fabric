// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace ktl;
using namespace Data;
using namespace Data::Utilities;
using namespace TxnReplicator;
using namespace Data::StateManager;
using namespace StateManagerTests;

KStringView const TestStateProvider::TypeName = L"TestStateProvider";

NTSTATUS TestStateProvider::Create(
    __in Data::StateManager::FactoryArguments const & factoryArguments,
    __in KAllocator & allocator,
    __in FaultStateProviderType::FaultAPI faultyAPI,
    __out TestStateProvider::SPtr & result)
{
    // Fault the constructor if necessary.
    if (faultyAPI == FaultStateProviderType::Constructor)
    {
        return SF_STATUS_COMMUNICATION_ERROR;
    }

    result = _new(STATE_MANAGER_TAG, allocator) TestStateProvider(
        factoryArguments,
        faultyAPI);
    if (result == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(result->Status()))
    {
        return (SPtr(Ktl::Move(result)))->Status();
    }

    return STATUS_SUCCESS;
}

OperationData::SPtr TestStateProvider::CreateInitializationParameters(
    __in ULONG childrenCount,
    __in KAllocator & allocator)
{
    // Create the initParameters for the test state provider
    // This function is used for the hierarchy test, the initParameters will
    // have one buffer in the array with children count
    OperationData::SPtr initParameters = OperationData::Create(allocator);
    ASSERT_IFNOT(
        NT_SUCCESS(initParameters->Status()),
        "CreateInitializationParameters: Create OperationData failed. Status: {0}.",
        initParameters->Status());

    BinaryWriter writer(allocator);
    ASSERT_IFNOT(
        NT_SUCCESS(writer.Status()),
        "CreateInitializationParameters: Create BinaryWriter failed. Status: {0}.",
        writer.Status());

    writer.Write(static_cast<ULONG32>(childrenCount));

    initParameters->Append(*writer.GetBuffer(0, writer.Position));

    return initParameters;
}

FABRIC_STATE_PROVIDER_ID TestStateProvider::get_StateProviderId() const
{
    return stateProviderId_;
}

Awaitable<void> TestStateProvider::AddAsync(
    __in Transaction & transaction, 
    __in TestOperation const & testOperation)
{
    KShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    // Acquire the write lock and add to the lock context.
    // In this way, the write lock will cover all the transaction which prevents the 
    // remove state provider call during the AddOperation.
    if (transaction.TransactionId != lastAcquiredLockTxnId)
    {
        // Do not update lastAcquiredLockTxnId here, need to use it to differentiate 
        // whether it is a reentrant. 
        co_await lock_->AcquireWriteLockAsync(INT_MAX);
    }

    try
    {
        AddOperationToMap(transaction.TransactionId, testOperation, *infligtOperationMaps_);

        status = transaction.AddOperation(
            testOperation.Metadata.RawPtr(),
            testOperation.Undo.RawPtr(),
            testOperation.Redo.RawPtr(),
            stateProviderId_,
            testOperation.Context.RawPtr());
        THROW_ON_FAILURE(status);

        // If it is a reentrant, do not need to add to lock context.
        if (transaction.TransactionId != lastAcquiredLockTxnId)
        {
            // Adding lock context as the last step so that we know no more failures can happen after this point
            TestLockContext::SPtr lockContext = TestLockContext::Create(*lock_, GetThisAllocator());
            ASSERT_IFNOT(
                lockContext != nullptr,
                "{0}:{1}:{2} AddAsync: Create LockContext failed, TxnId: {3}.",
                TracePartitionId,
                ReplicaId,
                StateProviderId,
                transaction.TransactionId);

            // Create lockContext may fail, we should release the lock before throw.
            THROW_ON_FAILURE(NT_SUCCESS(lockContext->Status()));

            // Add lock context may throw exception if add operation fail. So keep it in the try lock, 
            // if it throws, release the lock and throw exception.
            status = transaction.AddLockContext(*lockContext);
            THROW_ON_FAILURE(status);
        }
    }
    catch (Exception &)
    {
        lock_->ReleaseWriteLock();
        
        // If the exception throw, reset the lastAcquiredLockTxnId to the invalid number.
        lastAcquiredLockTxnId = -1;

        throw;
    }

    // At this point, acquired the write lock so update lastAcquiredLockTxnId.
    lastAcquiredLockTxnId = transaction.TransactionId;

    // The lock will be released when the transaction committed or aborted.
    co_return;
}

Awaitable<void> TestStateProvider::AddAsync(
    __in AtomicOperation & atomicOperation,
    __in TestOperation const & testOperation)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    AddOperationToMap(atomicOperation.TransactionId, testOperation, *infligtOperationMaps_);

    status = co_await atomicOperation.AddOperationAsync(
        testOperation.Metadata.RawPtr(),
        testOperation.Undo.RawPtr(),
        testOperation.Redo.RawPtr(),
        stateProviderId_,
        testOperation.Context.RawPtr());
    THROW_ON_FAILURE(status);

    co_return;
}

Awaitable<void> TestStateProvider::AddAsync(
    __in AtomicRedoOperation & atomicRedoOperation,
    __in TestOperation const & testOperation)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    AddOperationToMap(atomicRedoOperation.TransactionId, testOperation, *infligtOperationMaps_);

    status = co_await atomicRedoOperation.AddOperationAsync(
        testOperation.Metadata.RawPtr(),
        testOperation.Redo.RawPtr(),
        stateProviderId_,
        testOperation.Context.RawPtr());
    THROW_ON_FAILURE(status);

    co_return;
}

OperationData::CSPtr TestStateProvider::GetInitializationParameters() const
{
    return initializationParameters_;
}

void TestStateProvider::ExpectCommit(__in LONG64 transactionId)
{
    KSharedArray<TestOperation>::SPtr operationArraySPtr = nullptr;
    bool isRemoved = infligtOperationMaps_->TryRemove(transactionId, operationArraySPtr);
    ASSERT_IFNOT(
        isRemoved,
        "{0}:{1}:{2} ExpectCommit: Remove from infligtOperationMaps failed, TxnId: {3}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        transactionId);

    bool isAdded = expectedToBeAppliedMap_->TryAdd(transactionId, operationArraySPtr);
    ASSERT_IFNOT(
        isAdded,
        "{0}:{1}:{2} ExpectCommit: Add to expectedToBeAppliedMap failed, TxnId: {3}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        transactionId);
}

void TestStateProvider::ExpectCheckpoint(
    __in LONG64 checkpointLSN,
    __in_opt bool completeCheckpointOnly)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    // Add completeCheckpointOnly flag for checking whether CompleteCheckpointAsync is called as 
    // expected. During recovering, CompleteCheckpointAsync will be called if specified completeCheckpoint to StateManager
    // OpenPath, so in the test state provider, the CompleteCheckpointArray will be added a UnknownLSN to indicate the 
    // CompleteCheckpointAsync is called and other two arrays are empty.
    if (completeCheckpointOnly)
    {
        status = expectedCompleteCheckpointArray.Append(checkpointLSN);
        ASSERT_IFNOT(
            NT_SUCCESS(status),
            "{0}:{1}:{2} ExpectCheckpoint: Append to expectedCompleteCheckpointArray failed, checkpointLSN: {3}, status: {4}.",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            checkpointLSN,
            status);
        return;
    }

    status = expectedPrepareCheckpointArray.Append(checkpointLSN);
    ASSERT_IFNOT(
        NT_SUCCESS(status),
        "{0}:{1}:{2} ExpectCheckpoint: Append to expectedPrepareCheckpointArray failed, checkpointLSN: {3}, status: {4}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        checkpointLSN,
        status);

    status = expectedPerformCheckpointArray.Append(checkpointLSN);
    ASSERT_IFNOT(
        NT_SUCCESS(status),
        "{0}:{1}:{2} ExpectCheckpoint: Append to expectedPerformCheckpointArray failed, checkpointLSN: {3}, status: {4}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        checkpointLSN,
        status);

    status = expectedCompleteCheckpointArray.Append(checkpointLSN);
    ASSERT_IFNOT(
        NT_SUCCESS(status),
        "{0}:{1}:{2} ExpectCheckpoint: Append to expectedCompleteCheckpointArray failed, checkpointLSN: {3}, status: {4}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        checkpointLSN,
        status);
}

void TestStateProvider::Verify(
    __in_opt bool verifyCheckpoint,
    __in_opt bool verifyCompleteCheckpoint)
{
    ASSERT_IFNOT(
        expectedToBeAppliedMap_->Count == appliedMap_->Count,
        "{0}:{1}:{2} Verify: Applied count is not as expected, expectedCount: {3}, appliedCount: {4}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        expectedToBeAppliedMap_->Count,
        appliedMap_->Count);

    IEnumerator<KeyValuePair<LONG64, KSharedArray<TestOperation>::SPtr>>::SPtr enumerator = expectedToBeAppliedMap_->GetEnumerator();
    while (enumerator->MoveNext())
    {
        KeyValuePair<LONG64, KSharedArray<TestOperation>::SPtr> entry = enumerator->Current();
        KSharedArray<TestOperation>::SPtr appliedOperationArraySPtr = nullptr;
        bool transactionExists = appliedMap_->TryGetValue(entry.Key, appliedOperationArraySPtr);
        KInvariant(transactionExists);
        VerifyOperationList(*(entry.Value), *appliedOperationArraySPtr);

        // Note that we are passing in expectedOperationArraySptr since the applied will not contain the Context 
        // (it is not given)
        VerifyUnlocked(*entry.Value);
    }

    if (verifyCheckpoint)
    {
        ASSERT_IFNOT(
            expectedPrepareCheckpointArray.Count() == prepareCheckpointArray.Count(),
            "{0}:{1}:{2} Verify: PrepareCheckpointArrayCount is not as expected, expectedCount: {3}, actualCount: {4}.",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            expectedPrepareCheckpointArray.Count(),
            prepareCheckpointArray.Count());

        ASSERT_IFNOT(
            expectedPerformCheckpointArray.Count() == performCheckpointArray.Count(),
            "{0}:{1}:{2} Verify: PerformCheckpointArrayCount is not as expected, expectedCount: {3}, actualCount: {4}.",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            expectedPerformCheckpointArray.Count(),
            performCheckpointArray.Count());

        ASSERT_IFNOT(
            expectedCompleteCheckpointArray.Count() == completeCheckpointArray.Count(),
            "{0}:{1}:{2} Verify: CompleteCheckpointArrayCount is not as expected, expectedCount: {3}, actualCount: {4}, verifyCompleteCheckpoint: {5}.",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            expectedCompleteCheckpointArray.Count(),
            completeCheckpointArray.Count(),
            verifyCompleteCheckpoint);
    }

    if (verifyCompleteCheckpoint)
    {
        ASSERT_IFNOT(
            expectedCompleteCheckpointArray.Count() == completeCheckpointArray.Count(),
            "{0}:{1}:{2} Verify: CompleteCheckpointArrayCount is not as expected, expectedCount: {3}, actualCount: {4}, verifyCompleteCheckpoint: {5}.",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            expectedCompleteCheckpointArray.Count(),
            completeCheckpointArray.Count(),
            verifyCompleteCheckpoint);
    }
}

void TestStateProvider::Verify(__in LONG64 transactionId)
{
    KSharedArray<TestOperation>::SPtr expectedOperationArraySPtr;
    bool transactionExists = expectedToBeAppliedMap_->TryGetValue(transactionId, expectedOperationArraySPtr);
    ASSERT_IFNOT(
        transactionExists,
        "{0}:{1}:{2} Verify: Get Operation from expected map failed, TxnId: {3}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        transactionId);

    KSharedArray<TestOperation>::SPtr appliedOperationArraySPtr;
    transactionExists = appliedMap_->TryGetValue(transactionId, appliedOperationArraySPtr);
    ASSERT_IFNOT(
        transactionExists,
        "{0}:{1}:{2} Verify: Get Operation from actual map failed, TxnId: {3}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        transactionId);

    VerifyOperationList(*expectedOperationArraySPtr, *appliedOperationArraySPtr);

    // Note that we are passing in expectedOperationArraySptr since the applied will not contain the Context 
    // (it is not given)
    VerifyUnlocked(*expectedOperationArraySPtr);
}

void TestStateProvider::VerifyRecoveredStates(__in KArray<LONG64> const & expectedArray)
{
    ASSERT_IFNOT(
        expectedArray.Count() == recoverCheckpointArray.Count(),
        "{0}:{1}:{2} VerifyRecoveredStates: Recovered states count should be the same, expectedCount: {3}, actualCount: {4}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        expectedArray.Count(),
        recoverCheckpointArray.Count());

    for (ULONG i = 0; i < expectedArray.Count(); i++)
    {
        ASSERT_IFNOT(
            expectedArray[i] == recoverCheckpointArray[i],
            "{0}:{1}:{2} VerifyRecoveredStates: Recovered states should be the same, expectedValue: {3}, actualValue: {4}.",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            expectedArray[i],
            recoverCheckpointArray[i]);
    }
}

KUriView const TestStateProvider::GetName() const
{
    return name_;
}

KArray<TxnReplicator::StateProviderInfo> TestStateProvider::GetChildren(
    __in KUriView const & rootName)
{
    ULONG32 childrenCount = 0;
 
    if(initializationParameters_!=nullptr)
    {
        KBuffer::CSPtr childrenCountBuffer = (*initializationParameters_)[0];
        BinaryReader reader(*childrenCountBuffer, GetThisAllocator());

        reader.Read(childrenCount);
    }

    ASSERT_IFNOT(
        rootName.Compare(name_) == TRUE,
        "{0}:{1}:{2} GetChildren: RootName should be the same as the name of state provider, RootName: {3}, Name: {4}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        ToStringLiteral(rootName),
        ToStringLiteral(name_));

    KArray<StateProviderInfo> children = KArray<StateProviderInfo>(GetThisAllocator());
    ASSERT_IFNOT(
        NT_SUCCESS(children.Status()),
        "{0}:{1}:{2} GetChildren: Create children array failed, RootName: {3}, status: {4}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        ToStringLiteral(rootName),
        children.Status());

    // If the childrenCount is 0, it means no child
    for (ULONG32 i = 1; i <= childrenCount; i++)
    {
        KString::SPtr copyString;
        KStringView const childTypeName = L"TestStateProvider";
        NTSTATUS status = KString::Create(copyString, GetThisAllocator(), childTypeName);
        ASSERT_IFNOT(
            NT_SUCCESS(status),
            "{0}:{1}:{2} GetChildren: Create type name string failed, RootName: {3}, status: {4}.",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            ToStringLiteral(rootName),
            status);

        KString::SPtr childrenNameKString;
        status = KString::Create(childrenNameKString, GetThisAllocator(), rootName.Get(KUriView::eRaw));
        ASSERT_IFNOT(
            NT_SUCCESS(status),
            "{0}:{1}:{2} GetChildren: Create child name string failed, RootName: {3}, status: {4}.",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            ToStringLiteral(rootName),
            status);

        BOOLEAN concatSuccess = childrenNameKString->Concat(L"/");
        ASSERT_IFNOT(
            concatSuccess == TRUE,
            "{0}:{1}:{2} GetChildren: Child name append '/' failed, RootName: {3}.",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            ToStringLiteral(rootName));
       
        WCHAR concatChildrenID = static_cast<WCHAR>('0' + i);
        concatSuccess = childrenNameKString->AppendChar(concatChildrenID);
        ASSERT_IFNOT(
            concatSuccess == TRUE,
            "{0}:{1}:{2} GetChildren: Child name append ID failed, RootName: {3}.",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            ToStringLiteral(rootName));

        concatSuccess = childrenNameKString->SetNullTerminator();
        ASSERT_IFNOT(
            concatSuccess == TRUE,
            "{0}:{1}:{2} GetChildren: Child name append Null Terminator failed, RootName: {3}.",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            ToStringLiteral(rootName));

        KUri::SPtr childrenNameKUri;
        status = KUri::Create(*childrenNameKString, GetThisAllocator(), childrenNameKUri);
        ASSERT_IFNOT(
            NT_SUCCESS(status),
            "{0}:{1}:{2} GetChildren: Create child name KUri failed, RootName: {3}, statue: {4}.",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            ToStringLiteral(rootName),
            status);

        StateProviderInfo childInfo(*copyString, *childrenNameKUri, nullptr);

        status = children.Append(childInfo);
        ASSERT_IFNOT(
            NT_SUCCESS(status),
            "{0}:{1}:{2} GetChildren: append childInfo failed, RootName: {3}, statue: {4}.",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            ToStringLiteral(rootName),
            status);
    }

    return children;
}

void TestStateProvider::Initialize(
    __in KWeakRef<ITransactionalReplicator> & transactionalReplicatorWRef,
    __in KStringView const& workFolder,
    __in KSharedArray<IStateProvider2::SPtr> const* const children)
{
    ThrowExceptionIfFaultyAPI(FaultStateProviderType::FaultAPI::Initialize);

    UNREFERENCED_PARAMETER(children);

    KWeakRef<ITransactionalReplicator>::SPtr tmpReplicatorWRef(&transactionalReplicatorWRef);
    ASSERT_IFNOT(
        tmpReplicatorWRef->IsAlive(),
        "{0}:{1}:{2} Initialize: ReplicatorWRef must be alive.",
        TracePartitionId,
        ReplicaId,
        StateProviderId);

    hasPersistedState_ = tmpReplicatorWRef->TryGetTarget()->HasPeristedState;

    ASSERT_IFNOT(
        lifeState_ == Created,
        "{0}:{1}:{2} Initialize: lifeState must be Created, lifeState: {3}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        static_cast<LONG32>(lifeState_));

    // Create and Save WorkFolderCSPtr
    KString::SPtr tmp = nullptr;
    KString::Create(tmp, GetThisAllocator(), workFolder);

    workFolder_ = KString::CSPtr(tmp.RawPtr());

    tempCheckpointFilePath_ = GetCheckpointFilePath(TempCheckpointPrefix);
    currentCheckpointFilePath_ = GetCheckpointFilePath(CurrentCheckpointPrefix);
    backupCheckpointFilePath_ = GetCheckpointFilePath(BackupCheckpointPrefix);

    lifeState_ = Initialized;
}

Awaitable<void> TestStateProvider::OpenAsync(
    __in CancellationToken const& cancellationToken)
{
    UNREFERENCED_PARAMETER(cancellationToken);
    ThrowExceptionIfFaultyAPI(FaultStateProviderType::FaultAPI::BeginOpenAsync);

    if (faultyAPI_ == FaultStateProviderType::EndOpenAsync)
    {
        co_await KTimer::StartTimerAsync(GetThisAllocator(), GetThisAllocationTag(), 1000, nullptr);
        ThrowExceptionIfFaultyAPI(FaultStateProviderType::FaultAPI::EndOpenAsync);
    }

    kservice::OpenAwaiter::SPtr openAwaiter;
    NTSTATUS status = kservice::OpenAwaiter::Create(GetThisAllocator(), GetThisAllocationTag(), *this, openAwaiter);
    ASSERT_IFNOT(
        NT_SUCCESS(status),
        "{0}:{1}:{2} OpenAsync: Create OpenAwaiter failed, status: {3}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        status);

    status = co_await *openAwaiter;
    ASSERT_IFNOT(
        NT_SUCCESS(status),
        "{0}:{1}:{2} OpenAsync: Open failed, status: {3}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        status);

    co_return;
}

Awaitable<void> TestStateProvider::ChangeRoleAsync(
    __in FABRIC_REPLICA_ROLE newRole, 
    __in CancellationToken const& cancellationToken)
{
    ThrowExceptionIfFaultyAPI(FaultStateProviderType::FaultAPI::ChangeRoleAsync);

    UNREFERENCED_PARAMETER(cancellationToken);

    // Generic life cycle check.
    ASSERT_IFNOT(
        (lifeState_ == Open) || (lifeState_ == Idle) || (lifeState_ == Secondary) || (lifeState_ == Primary),
        "{0}:{1}:{2} ChangeRoleAsync: LifeState check failed, lifeState: {3}, newRole: {4}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        static_cast<LONG32>(lifeState_),
        static_cast<LONG32>(newRole));

    if (newRole == FABRIC_REPLICA_ROLE::FABRIC_REPLICA_ROLE_PRIMARY)
    {
        ASSERT_IFNOT(
            (lifeState_ == Open) || (lifeState_ == Secondary),
            "{0}:{1}:{2} ChangeRoleAsync: LifeState check failed, lifeState: {3}, newRole: FABRIC_REPLICA_ROLE_PRIMARY.",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            static_cast<LONG32>(lifeState_));
        lifeState_ = Primary;
    }
    else if (newRole == FABRIC_REPLICA_ROLE::FABRIC_REPLICA_ROLE_NONE)
    {
        ASSERT_IFNOT(
            (lifeState_ == Open) || (lifeState_ == Idle) || (lifeState_ == Secondary) || (lifeState_ == Primary),
            "{0}:{1}:{2} ChangeRoleAsync: LifeState check failed, lifeState: {3}, newRole: FABRIC_REPLICA_ROLE_NONE.",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            static_cast<LONG32>(lifeState_));
        lifeState_ = None;
    }
    else if (newRole == FABRIC_REPLICA_ROLE::FABRIC_REPLICA_ROLE_IDLE_SECONDARY)
    {
        ASSERT_IFNOT(
            (lifeState_ == Open),
            "{0}:{1}:{2} ChangeRoleAsync: LifeState check failed, lifeState: {3}, newRole: FABRIC_REPLICA_ROLE_IDLE_SECONDARY.",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            static_cast<LONG32>(lifeState_));
        lifeState_ = Idle;
    }
    else if (newRole == FABRIC_REPLICA_ROLE::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY)
    {
        // Both IdelSecondary and Primary can become ActiveSecondary, Primary changes to ActiveSecondary may due to load balancing.
        ASSERT_IFNOT(
            (lifeState_ == Idle || lifeState_ == Primary),
            "{0}:{1}:{2} ChangeRoleAsync: LifeState check failed, lifeState: {3}, newRole: FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY.",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            static_cast<LONG32>(lifeState_));
        lifeState_ = Secondary;
    }
    else
    {
        throw Exception(STATUS_NOT_IMPLEMENTED);
    }

    co_return;
}

Awaitable<void> TestStateProvider::CloseAsync(
    __in CancellationToken const& cancellationToken)
{
    ThrowExceptionIfFaultyAPI(FaultStateProviderType::FaultAPI::BeginCloseAsync);

    if (faultyAPI_ == FaultStateProviderType::EndCloseAsync)
    {
        co_await KTimer::StartTimerAsync(GetThisAllocator(), GetThisAllocationTag(), 1000, nullptr);
        ThrowExceptionIfFaultyAPI(FaultStateProviderType::FaultAPI::EndCloseAsync);
    }

    co_await CloseInternalAsync(cancellationToken);
    co_return;
}

void TestStateProvider::Abort() noexcept
{
    Awaitable<void> awaitable = CloseInternalAsync(CancellationToken::None);
    ToTask(awaitable);
}

void TestStateProvider::PrepareCheckpoint(__in LONG64 checkpointLSN)
{
    ThrowExceptionIfFaultyAPI(FaultStateProviderType::FaultAPI::PrepareCheckpoint);

    ASSERT_IFNOT(
        (lifeState_ == Open) || (lifeState_ == Idle) || (lifeState_ == Secondary) || (lifeState_ == Primary),
        "{0}:{1}:{2} PrepareCheckpoint: LifeState check failed, lifeState: {3}, checkpointLSN: {4}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        static_cast<LONG32>(lifeState_),
        checkpointLSN);

    NTSTATUS status = prepareCheckpointArray.Append(checkpointLSN);
    ASSERT_IFNOT(
        NT_SUCCESS(status),
        "{0}:{1}:{2} PrepareCheckpoint: Append checkpointLSN failed, status: {3}, checkpointLSN: {4}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        status,
        checkpointLSN);
}

Awaitable<void> TestStateProvider::PerformCheckpointAsync(
    __in CancellationToken const& cancellationToken)
{
    UNREFERENCED_PARAMETER(cancellationToken);

    ThrowExceptionIfFaultyAPI(FaultStateProviderType::FaultAPI::PerformCheckpointAsync);

    ASSERT_IFNOT(
        (lifeState_ == Open) || (lifeState_ == Idle) || (lifeState_ == Secondary) || (lifeState_ == Primary),
        "{0}:{1}:{2} PerformCheckpointAsync: LifeState check failed, lifeState: {3}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        static_cast<LONG32>(lifeState_));
    ASSERT_IFNOT(
        prepareCheckpointArray.Count(),
        "{0}:{1}:{2} PerformCheckpointAsync: prepareCheckpointArray count must greater than 0, Count: {3}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        prepareCheckpointArray.Count());

    auto lastPrepareCheckpointLSN = prepareCheckpointArray[prepareCheckpointArray.Count() - 1];

    NTSTATUS status = performCheckpointArray.Append(lastPrepareCheckpointLSN);
    ASSERT_IFNOT(
        NT_SUCCESS(status),
        "{0}:{1}:{2} PerformCheckpointAsync: Append checkpointLSN failed, status: {3}, checkpointLSN: {4}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        status,
        lastPrepareCheckpointLSN);

    if (checkpointing_ && hasPersistedState_)
    {
        co_await this->WriteAsync();
    }

    co_return;
}

Awaitable<void> TestStateProvider::CompleteCheckpointAsync(
    __in CancellationToken const& cancellationToken)
{
    ThrowExceptionIfFaultyAPI(FaultStateProviderType::FaultAPI::CompleteCheckpointAsync);

    UNREFERENCED_PARAMETER(cancellationToken);

    ASSERT_IFNOT(
        (lifeState_ == Open) || (lifeState_ == Idle) || (lifeState_ == Secondary) || (lifeState_ == Primary),
        "{0}:{1}:{2} CompleteCheckpointAsync: LifeState check failed, lifeState: {3}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        static_cast<LONG32>(lifeState_));

    // In normal case, checkpoint LSN should be passed to all prepareCheckpointArray, performCheckpointArray and completeCheckpointArray,
    // and in the test we can verify the checkpointLSN is as expected or not.
    // But in the case that we called prepare and perform but then the replica went down and up again, and if it specified completeCheckpoint,  
    // the state provider CompleteCheckpointAsync will be invoked and we can not get the checkpoint LSN from performCheckpointArray because 
    // at this time all three arrays are empty. In this special case to verfiy CompleteCheckpointAsync is called as expected in the test,  
    // we insert UnknownLSN(-1) to the array.
    LONG64 lastPerformCheckpointLSN = UnknownLSN;
    if (performCheckpointArray.Count() > 0)
    {
        lastPerformCheckpointLSN = performCheckpointArray[performCheckpointArray.Count() - 1];
    }

    NTSTATUS status = completeCheckpointArray.Append(lastPerformCheckpointLSN);
    ASSERT_IFNOT(
        NT_SUCCESS(status),
        "{0}:{1}:{2} CompleteCheckpointAsync: Append checkpointLSN failed, status: {3}, checkpointLSN: {4}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        status,
        lastPerformCheckpointLSN);

    if (Common::File::Exists(static_cast<LPCWSTR>(*tempCheckpointFilePath_)))
    {
        co_await SafeFileReplaceAsync(
            *currentCheckpointFilePath_,
            *tempCheckpointFilePath_,
            *backupCheckpointFilePath_);
    }

    co_return;
}

Awaitable<void> TestStateProvider::RecoverCheckpointAsync(__in CancellationToken const & cancellationToken)
{
    ThrowExceptionIfFaultyAPI(FaultStateProviderType::FaultAPI::RecoverCheckpointAsync);

    UNREFERENCED_PARAMETER(cancellationToken);

    ASSERT_IFNOT(
        lifeState_ == Open,
        "{0}:{1}:{2} RecoverCheckpointAsync: LifeState check failed, lifeState: {3}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        static_cast<LONG32>(lifeState_));

    if (hasPersistedState_)
    {
        KWString filePath = KWString(GetThisAllocator(), currentCheckpointFilePath_->ToUNICODE_STRING());
        co_await this->ReadAsync(filePath);
    }

    recoverCheckpoint_ = true;

    co_return;
}

Awaitable<void> TestStateProvider::BackupCheckpointAsync(
    __in KString const & backupDirectory,
    __in CancellationToken const & cancellationToken)
{
    ThrowExceptionIfFaultyAPI(FaultStateProviderType::FaultAPI::RestoreCheckpointAsync);

    UNREFERENCED_PARAMETER(backupDirectory);
    UNREFERENCED_PARAMETER(cancellationToken);

    co_return;
}

Awaitable<void> TestStateProvider::RestoreCheckpointAsync(
    __in KString const & backupDirectory,
    __in CancellationToken const & cancellationToken)
{
    ThrowExceptionIfFaultyAPI(FaultStateProviderType::FaultAPI::RestoreCheckpointAsync);

    UNREFERENCED_PARAMETER(backupDirectory);
    UNREFERENCED_PARAMETER(cancellationToken);

    co_return;
}

Awaitable<void> TestStateProvider::RemoveStateAsync(
    __in CancellationToken const& cancellationToken)
{
    ThrowExceptionIfFaultyAPI(FaultStateProviderType::FaultAPI::RemoveStateAsync);

    UNREFERENCED_PARAMETER(cancellationToken);

    ASSERT_IFNOT(
        lifeState_ == None,
        "{0}:{1}:{2} RemoveStateAsync: LifeState check failed, lifeState: {3}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        static_cast<LONG32>(lifeState_));

    checkpointState_ = Removed;

    co_return;
}

OperationDataStream::SPtr TestStateProvider::GetCurrentState()
{
    ThrowExceptionIfFaultyAPI(FaultStateProviderType::FaultAPI::GetCurrentState);

    // If the state provider has nothing to copy, then just return nullptr.
    ULONG arrayCount = completeCheckpointArray.Count();
    if (arrayCount == 0)
    {
        return nullptr;
    }

    auto lastCompleteCheckpointLSN = completeCheckpointArray[arrayCount - 1];
    return TestOperationDataStream::Create(lastCompleteCheckpointLSN, GetThisAllocator()).RawPtr();
}

ktl::Awaitable<void> TestStateProvider::BeginSettingCurrentStateAsync()
{
    ThrowExceptionIfFaultyAPI(FaultStateProviderType::FaultAPI::BeginSettingCurrentState);

    isEndSettingCurrentStateCalled_ = false;
    isBeginSettingCurrentStateCalled_ = true;

    co_return;
}

Awaitable<void> TestStateProvider::SetCurrentStateAsync(
    __in LONG64 stateRecordNumber, 
    __in OperationData const & operationData,
    __in CancellationToken const & cancellationToken)
{
    UNREFERENCED_PARAMETER(stateRecordNumber);
    UNREFERENCED_PARAMETER(cancellationToken);

    ThrowExceptionIfFaultyAPI(FaultStateProviderType::FaultAPI::SetCurrentStateAsync);

    userOperationDataArray_.Append(&operationData);

    if (operationData.BufferCount > 0 && operationData[0]->QuerySize() >= sizeof(LONG64))
    {
        BinaryReader reader(*operationData[0], GetThisAllocator());
        reader.Read(copiedLSN_);

        ASSERT_IFNOT(
            copiedLSN_ > FABRIC_INVALID_SEQUENCE_NUMBER,
            "{0}:{1}:{2} SetCurrentStateAsync: copiedLSN must be greater than FABRIC_INVALID_SEQUENCE_NUMBER, copiedLSN: {3}.",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            copiedLSN_);
    }

    co_return;
}

Awaitable<void> TestStateProvider::EndSettingCurrentStateAsync(
    __in CancellationToken const & cancellationToken)
{
    ThrowExceptionIfFaultyAPI(FaultStateProviderType::FaultAPI::EndSettingCurrentStateAsync);

    UNREFERENCED_PARAMETER(cancellationToken);

    // If the state provider has nothing to copy, the copiedLSN_ has not been set, 
    // thus, assert only if SP has copied something.
    if (completeCheckpointArray.Count() > 0)
    {
        ASSERT_IFNOT(
            copiedLSN_ > FABRIC_INVALID_SEQUENCE_NUMBER,
            "{0}:{1}:{2} EndSettingCurrentStateAsync: copiedLSN must be greater than FABRIC_INVALID_SEQUENCE_NUMBER, copiedLSN: {3}.",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            copiedLSN_);
    }

    ASSERT_IFNOT(
        isBeginSettingCurrentStateCalled_,
        "{0}:{1}:{2} EndSettingCurrentStateAsync: isBeginSettingCurrentStateCalled must be true.",
        TracePartitionId,
        ReplicaId,
        StateProviderId);

    isEndSettingCurrentStateCalled_ = true;
    isBeginSettingCurrentStateCalled_ = false;

    co_return;
}

Awaitable<void> TestStateProvider::PrepareForRemoveAsync(
    __in TransactionBase const& transactionBase, 
    __in CancellationToken const& cancellationToken)
{
    ThrowExceptionIfFaultyAPI(FaultStateProviderType::FaultAPI::PrepareForRemoveAsync);

    UNREFERENCED_PARAMETER(cancellationToken);

    // StateProvider PrepareForRemoveAsync is only called in RemoveAsync, which will acquire write lock of the 
    // metadata. Thus no two PrepareForRemoveAsync call at the same time.
    co_await lock_->AcquireWriteLockAsync(INT_MAX);

    try
    {
        TestLockContext::SPtr lockContext = TestLockContext::Create(*lock_, GetThisAllocator());
        ASSERT_IFNOT(
            lockContext != nullptr,
            "{0}:{1}:{2} PrepareForRemoveAsync: Create TestLockContext failed, TxnId: {3}.",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            transactionBase.TransactionId);

        // Create lockContext may fail, we should release the lock before throw.
        THROW_ON_FAILURE(NT_SUCCESS(lockContext->Status()));

        // Add lock context may throw exception if add operation fail. So keep it in the try lock, 
        // if it throws, release the lock and throw exception.

        TransactionBase & nonConstTransactionBase = const_cast<TransactionBase &>(transactionBase);
        NTSTATUS status = nonConstTransactionBase.AddLockContext(*lockContext);
        THROW_ON_FAILURE(status);
    }
    catch (Exception &)
    {
        lock_->ReleaseWriteLock();

        throw;
    }

    co_return;
}

Awaitable<OperationContext::CSPtr> TestStateProvider::ApplyAsync(
    __in LONG64 logicalSequenceNumber,
    __in TransactionBase const & transactionBase,
    __in ApplyContext::Enum applyContext,
    __in_opt OperationData const * const metadataPtr,
    __in_opt OperationData const * const dataPtr)
{
    UNREFERENCED_PARAMETER(logicalSequenceNumber);
    UNREFERENCED_PARAMETER(applyContext);

    // Use apply context to make more specific.
    ASSERT_IFNOT(
        (lifeState_ == Open) || (lifeState_ == Idle) || (lifeState_ == Secondary) || (lifeState_ == Primary),
        "{0}:{1}:{2} ApplyAsync: LifeState check failed, lifeState: {3}, logicalSequenceNumber: {4}, TxnId: {5}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        static_cast<LONG32>(lifeState_),
        logicalSequenceNumber,
        transactionBase.TransactionId);

    userMetadataOperationData_ = metadataPtr;
    userRedoOrUndoOperationData_ = dataPtr;

    TestOperation operation(metadataPtr, nullptr, dataPtr, nullptr);
    
    AddOperationToMap(transactionBase.TransactionId, operation, *appliedMap_);

    co_return nullptr;
}

void TestStateProvider::Unlock(
    __in OperationContext const & operationContext)
{
    TestOperationContext const & testOperationContex = dynamic_cast<TestOperationContext const &>(operationContext);

    TestOperationContext & nonConstOperationContext = const_cast<TestOperationContext &>(testOperationContex);

    nonConstOperationContext.Unlock(stateProviderId_);
}

void TestStateProvider::OnServiceOpen()
{
    SetDeferredCloseBehavior();

    ASSERT_IFNOT(
        lifeState_ == Initialized,
        "{0}:{1}:{2} OnServiceOpen: LifeState check failed, lifeState: {3}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        static_cast<LONG32>(lifeState_));
    lifeState_ = Open;

    CompleteOpen(STATUS_SUCCESS);
}

void TestStateProvider::OnServiceClose()
{
    // Only check the life state if no faulty APIs. Since faulty API may happen at any life state.
    ASSERT_IFNOT(
        (faultyAPI_ != FaultStateProviderType::None) || (lifeState_ == Open) || (lifeState_ == Idle) || (lifeState_ == Secondary) || (lifeState_ == Primary) || (lifeState_ == None),
        "{0}:{1}:{2} OnServiceClose: LifeState check failed, lifeState: {3}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        static_cast<LONG32>(lifeState_));

    ASSERT_IFNOT(
        faultyAPI_ != FaultStateProviderType::None || lifeState_ != None || checkpointState_ == Removed,
        "{0}:{1}:{2} OnServiceClose: LifeState check failed, lifeState: {3}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        static_cast<LONG32>(lifeState_));

    lifeState_ = Closed;

    CompleteClose(STATUS_SUCCESS);
}

Awaitable<void> TestStateProvider::CloseInternalAsync(CancellationToken const& cancellationToken)
{
    UNREFERENCED_PARAMETER(cancellationToken);

    kservice::CloseAwaiter::SPtr closeAwaiter = nullptr;
    NTSTATUS status = kservice::CloseAwaiter::Create(GetThisAllocator(), GetThisAllocationTag(), *this, closeAwaiter);
    ASSERT_IFNOT(
        NT_SUCCESS(status),
        "{0}:{1}:{2} CloseInternalAsync: Create CloseAwaiter failed, status: {3}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        status);

    status = co_await *closeAwaiter;
    ASSERT_IFNOT(
        NT_SUCCESS(status),
        "{0}:{1}:{2} CloseInternalAsync: closeAwaiter failed, status: {3}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        status);

    co_return;
}

Awaitable<void> TestStateProvider::WriteAsync()
{
    KShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;
    KBlockFile::SPtr FileSPtr = nullptr;
    io::KFileStream::SPtr fileStreamSPtr = nullptr;

    KWString filePath = KWString(GetThisAllocator(), tempCheckpointFilePath_->ToUNICODE_STRING());
    ASSERT_IFNOT(
        NT_SUCCESS(filePath.Status()),
        "{0}:{1}:{2} WriteAsync: Create filePath failed, status: {3}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        filePath.Status());

    status = co_await KBlockFile::CreateSparseFileAsync(
        filePath,
        FALSE, // IsWriteThrough
        KBlockFile::eCreateAlways,
        KBlockFile::eInheritFileSecurity,
        FileSPtr,
        nullptr,
        GetThisAllocator(),
        GetThisAllocationTag());

    ASSERT_IFNOT(
        NT_SUCCESS(status),
        "{0}:{1}:{2} WriteAsync: Create SparseFile failed, status: {3}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        status);

    KFinally([&] {FileSPtr->Close(); });

    status = io::KFileStream::Create(fileStreamSPtr, GetThisAllocator(), GetThisAllocationTag());
    ASSERT_IFNOT(
        NT_SUCCESS(status),
        "{0}:{1}:{2} WriteAsync: Create fileStream failed, status: {3}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        status);

    status = co_await fileStreamSPtr->OpenAsync(*FileSPtr);
    ASSERT_IFNOT(
        NT_SUCCESS(status),
        "{0}:{1}:{2} WriteAsync: Open fileStream failed, status: {3}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        status);

    SharedException::CSPtr exceptionSPtr = nullptr;
    try
    {
        // First write the test state provider data: currentVersion_, currentValue_.
        BinaryWriter itemWriter(GetThisAllocator());
        ASSERT_IFNOT(
            NT_SUCCESS(itemWriter.Status()),
            "{0}:{1}:{2} WriteAsync: Create BinaryWriter failed, status: {3}.",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            itemWriter.Status());

        // Write the count of the prepareCheckpointArray
        itemWriter.Write(static_cast<LONG64>(prepareCheckpointArray.Count()));

        // Write the value of the prepareCheckpointArray
        for (ULONG i = 0; i < prepareCheckpointArray.Count(); i++)
        {
            itemWriter.Write(prepareCheckpointArray[i]);
        }

        ULONG32 recordBlockSize = static_cast<ULONG32>(itemWriter.Position);
        ULONG64 recordChecksum = CRC64::ToCRC64(*itemWriter.GetBuffer(0), 0, recordBlockSize);
        itemWriter.Write(recordChecksum);
        ULONG32 recordBlockSizeWithChecksum = static_cast<ULONG32>(itemWriter.Position);

        // Updata the TestStateProviderProperties TestStateProviderInfoHandle
        TestStateProviderProperties::SPtr testStateProviderProperties = nullptr;
        status = TestStateProviderProperties::Create(GetThisAllocator(), testStateProviderProperties);
        ASSERT_IFNOT(
            NT_SUCCESS(status),
            "{0}:{1}:{2} WriteAsync: Create TestStateProviderProperties failed, status: {3}.",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            status);

        BlockHandle::SPtr testStateProviderInfoHandle = nullptr;
        status = BlockHandle::Create(0, recordBlockSize, GetThisAllocator(), testStateProviderInfoHandle);
        ASSERT_IFNOT(
            NT_SUCCESS(status),
            "{0}:{1}:{2} WriteAsync: Create BlockHandle failed, status: {3}.",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            status);

        testStateProviderProperties->TestStateProviderInfoHandle = *testStateProviderInfoHandle;

        status = co_await fileStreamSPtr->WriteAsync(*itemWriter.GetBuffer(0), 0, recordBlockSizeWithChecksum);
        ASSERT_IFNOT(
            NT_SUCCESS(status),
            "{0}:{1}:{2} WriteAsync: Write fileStreamSPtr failed, status: {3}.",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            status);

        // Write the properties.
        BlockHandle::SPtr propertiesHandle;
        FileBlock<BlockHandle::SPtr>::SerializerFunc propertiesSerializerFunction(testStateProviderProperties.RawPtr(), &TestStateProviderProperties::Write);
        status = co_await FileBlock<BlockHandle::SPtr>::WriteBlockAsync(*fileStreamSPtr, propertiesSerializerFunction, GetThisAllocator(), CancellationToken::None, propertiesHandle);
        ASSERT_IFNOT(
            NT_SUCCESS(status),
            "{0}:{1}:{2} WriteAsync: Write properties failed, status: {3}.",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            status);

        // Write the footer, set the footer version = 0
        FileFooter::SPtr fileFooter = nullptr;
        status = FileFooter::Create(*propertiesHandle, 0, GetThisAllocator(), fileFooter);
        ASSERT_IFNOT(
            NT_SUCCESS(status),
            "{0}:{1}:{2} WriteAsync: Create FileFooter failed, status: {3}.",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            status);

        BlockHandle::SPtr returnBlockHandle;
        FileBlock<FileFooter::SPtr>::SerializerFunc footerSerializerFunction(fileFooter.RawPtr(), &FileFooter::Write);
        status = co_await FileBlock<BlockHandle::SPtr>::WriteBlockAsync(*fileStreamSPtr, footerSerializerFunction, GetThisAllocator(), CancellationToken::None, returnBlockHandle);
        ASSERT_IFNOT(
            NT_SUCCESS(status),
            "{0}:{1}:{2} WriteAsync: Write footer failed, status: {3}.",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            status);

        status = co_await fileStreamSPtr->FlushAsync();
        ASSERT_IFNOT(
            NT_SUCCESS(status),
            "{0}:{1}:{2} WriteAsync: Flush fileStream failed, status: {3}.",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            status);
    }
    catch (ktl::Exception & e)
    {
        exceptionSPtr = SharedException::Create(e, GetThisAllocator());
    }

    // Note: CloseAsync may fail in two situation, 1. OOM 2. Flush-on-close.
    // We always explicitly FlushAsync before close, so we can use assert (OOM) instead here instead of throwing
    status = co_await fileStreamSPtr->CloseAsync();
    ASSERT_IFNOT(
        NT_SUCCESS(status),
        "{0}:{1}:{2} WriteAsync: Close fileStream failed, status: {3}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        status);

    if (exceptionSPtr != nullptr)
    {
        ktl::Exception exp = exceptionSPtr->Info;
        throw exp;
    }

    co_return;
}

ktl::Awaitable<void> TestStateProvider::ReadAsync(__in KWString & filePath)
{
    KShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;
    KBlockFile::SPtr FileSPtr = nullptr;
    io::KFileStream::SPtr fileStreamSPtr = nullptr;

    status = co_await KBlockFile::CreateSparseFileAsync(
        filePath,
        FALSE,
        KBlockFile::eOpenExisting,
        FileSPtr,
        nullptr,
        GetThisAllocator(),
        GetThisAllocationTag());

    // If trying to open the file and the file doesn't exist, return 
    // Other errors should throw.
    if (status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        co_return;
    }

    KFinally([&] {FileSPtr->Close(); });

    status = io::KFileStream::Create(fileStreamSPtr, GetThisAllocator(), GetThisAllocationTag());
    ASSERT_IFNOT(
        NT_SUCCESS(status),
        "{0}:{1}:{2} ReadAsync: Create FileStream failed, status: {3}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        status);

    status = co_await fileStreamSPtr->OpenAsync(*FileSPtr);
    ASSERT_IFNOT(
        NT_SUCCESS(status),
        "{0}:{1}:{2} ReadAsync: Open FileStream failed, status: {3}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        status);

    SharedException::CSPtr exceptionSPtr = nullptr;
    try
    {
        // Read and validate the Footer section.  The footer is always at the end of the stream, minus space for the checksum.
        BlockHandle::SPtr footerHandle = nullptr;
        LONG64 tmpSize = fileStreamSPtr->GetLength();

        ULONG64 fileSize = static_cast<ULONG64>(tmpSize);
        ULONG32 footerSize = FileFooter::SerializedSize();
        if (fileSize < (footerSize + sizeof(ULONG64)))
        {
            // File is too small to contain even a footer.
            throw Exception(STATUS_INTERNAL_DB_CORRUPTION);
        }

        // Create a block handle for the footer.
        // Note that size required is footer size + the checksum (sizeof(ULONG64))
        ULONG64 footerStartingOffset = fileSize - footerSize - sizeof(ULONG64);
        status = BlockHandle::Create(
            footerStartingOffset,
            footerSize,
            GetThisAllocator(),
            footerHandle);
        ASSERT_IFNOT(
            NT_SUCCESS(status),
            "{0}:{1}:{2} ReadAsync: Create BlockHandle failed, status: {3}.",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            status);

        FileBlock<FileFooter::SPtr>::DeserializerFunc deserializerFunction(&FileFooter::Read);
        FileFooter::SPtr footerSPtr = (co_await FileBlock<FileFooter::SPtr>::ReadBlockAsync(*fileStreamSPtr, *footerHandle, deserializerFunction, GetThisAllocator(), CancellationToken::None));

        // Read and validate the properties section.
        BlockHandle::SPtr propertiesHandle = footerSPtr->PropertiesHandle;

        TestStateProviderProperties::SPtr testStateProviderProperties;
        TestStateProviderProperties::Create(GetThisAllocator(), testStateProviderProperties);
        ASSERT_IFNOT(
            NT_SUCCESS(status),
            "{0}:{1}:{2} ReadAsync: Create TestStateProviderProperties failed, status: {3}.",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            status);

        FileBlock<TestStateProviderProperties::SPtr>::DeserializerFunc propertiesDeserializerFunction(&FileProperties::Read);
        testStateProviderProperties = (co_await FileBlock<TestStateProviderProperties::SPtr>::ReadBlockAsync(*fileStreamSPtr, *propertiesHandle, propertiesDeserializerFunction, GetThisAllocator(), CancellationToken::None));

        ULONG bytesToRead = static_cast<ULONG>(testStateProviderProperties->TestStateProviderInfoHandle->Size + sizeof(ULONG64));
        KBuffer::SPtr itemStream = nullptr;
        status = KBuffer::Create(bytesToRead, itemStream, GetThisAllocator(), GetThisAllocationTag());
        ASSERT_IFNOT(
            NT_SUCCESS(status),
            "{0}:{1}:{2} ReadAsync: Create KBuffer failed, status: {3}.",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            status);

        fileStreamSPtr->SetPosition(testStateProviderProperties->TestStateProviderInfoHandle->Offset);

        ULONG bytesRead = 0;
        status = co_await fileStreamSPtr->ReadAsync(*itemStream, bytesRead, 0, bytesToRead);
        ASSERT_IFNOT(
            NT_SUCCESS(status),
            "{0}:{1}:{2} ReadAsync: ReadAsync fileStream failed, status: {3}.",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            status);

        BinaryReader itemReader(*itemStream, GetThisAllocator());
        ASSERT_IFNOT(
            NT_SUCCESS(itemReader.Status()),
            "{0}:{1}:{2} ReadAsync: Create BinaryReader failed, status: {3}.",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            itemReader.Status());

        LONG64 readCount;
        itemReader.Read(readCount);
        
        for (LONG64 i = 0; i < readCount; i++)
        {
            LONG64 value;
            itemReader.Read(value);
            status = recoverCheckpointArray.Append(value);
            ASSERT_IFNOT(
                NT_SUCCESS(status),
                "{0}:{1}:{2} ReadAsync: Append CheckpointLSN failed, status: {3}.",
                TracePartitionId,
                ReplicaId,
                StateProviderId,
                status);
        }

        ULONG64 checksum;
        itemReader.Read(checksum);

        ULONG64 recordChecksum = CRC64::ToCRC64(*itemStream, 0, static_cast<ULONG32>(testStateProviderProperties->TestStateProviderInfoHandle->Size));
        ASSERT_IFNOT(
            checksum == recordChecksum,
            "{0}:{1}:{2} ReadAsync: Checksum should be the same.",
            TracePartitionId,
            ReplicaId,
            StateProviderId);
    }
    catch (ktl::Exception & e)
    {
        exceptionSPtr = SharedException::Create(e, GetThisAllocator());
    }

    // Note: CloseAsync may fail in two situation, 1. OOM 2. Flush-on-close.
    // We always explicitly FlushAsync before close, so we can use assert (OOM) instead here instead of throwing
    status = co_await fileStreamSPtr->CloseAsync();
    ASSERT_IFNOT(
        NT_SUCCESS(status),
        "{0}:{1}:{2} ReadAsync: Close fileStream failed, status: {3}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        status);

    if (exceptionSPtr != nullptr)
    {
        ktl::Exception exp = exceptionSPtr->Info;

        throw exp;
    }

    co_return;
}

ktl::Awaitable<void> TestStateProvider::SafeFileReplaceAsync(
    __in KString const & checkpointFileName,
    __in KString const & temporaryCheckpointFileName,
    __in KString const & backupCheckpointFileName)
{
    KShared$ApiEntry();

    // Delete previous backup, if it exists.
    Common::ErrorCode errorCode;
    if (Common::File::Exists(static_cast<LPCWSTR>(backupCheckpointFileName)))
    {
        errorCode = Common::File::Delete2(static_cast<LPCWSTR>(backupCheckpointFileName));
        if (errorCode.IsSuccess() == false)
        {
            throw Exception(StatusConverter::Convert(errorCode.ToHResult()));
        }
    }

    // Previous replace could have failed in the middle before the next checkpoint file got renamed to current.
    if (Common::File::Exists(static_cast<LPCWSTR>(checkpointFileName)) == false)
    {
        // Next checkpoint file is valid.  Move it to be current.
        errorCode = Common::File::Move(static_cast<LPCWSTR>(temporaryCheckpointFileName), static_cast<LPCWSTR>(checkpointFileName), false);
        if (errorCode.IsSuccess() == false)
        {
            throw Exception(StatusConverter::Convert(errorCode.ToHResult()));
        }

        co_return;
    }

    // Current exists, so we must have gotten here only after writing a valid next checkpoint file.
    errorCode = Common::File::Move(
        static_cast<LPCWSTR>(checkpointFileName),
        static_cast<LPCWSTR>(backupCheckpointFileName),
        false);
    if (errorCode.IsSuccess() == false)
    {
        throw Exception(StatusConverter::Convert(errorCode.ToHResult()));
    }

    errorCode = Common::File::Move(
        static_cast<LPCWSTR>(temporaryCheckpointFileName),
        static_cast<LPCWSTR>(checkpointFileName),
        false);
    if (errorCode.IsSuccess() == false)
    {
        throw Exception(StatusConverter::Convert(errorCode.ToHResult()));
    }

    // Delete the backup, now that we've safely replaced the file.
    if (Common::File::Exists(static_cast<LPCWSTR>(backupCheckpointFileName)))
    {
        errorCode = Common::File::Delete2(static_cast<LPCWSTR>(backupCheckpointFileName));
        if (errorCode.IsSuccess() == false)
        {
            throw Exception(StatusConverter::Convert(errorCode.ToHResult()));
        }
    }

    co_return;
}

KString::CSPtr TestStateProvider::GetCheckpointFilePath(__in KStringView const & fileName)
{
    BOOLEAN boolStatus;

    KString::SPtr filePath = nullptr;
    NTSTATUS status = KString::Create(filePath, GetThisAllocator(), *workFolder_);
    ASSERT_IFNOT(
        NT_SUCCESS(status),
        "{0}:{1}:{2} GetCheckpointFilePath: Create KString failed, status: {3}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        status);

    boolStatus = filePath->Concat(Common::Path::GetPathSeparatorWstr().c_str());
    boolStatus = filePath->Concat(fileName);
    return filePath.RawPtr();
}

void TestStateProvider::AddOperationToMap(
    __in LONG64 transactionId,
    __in TestOperation const & testOperation, 
    __in ConcurrentDictionary<LONG64, KSharedArray<TestOperation>::SPtr>& map)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    KSharedArray<TestOperation>::SPtr inflightOperations = nullptr;
    bool itemExists = map.TryGetValue(transactionId, inflightOperations);
    if (itemExists)
    {
        status = inflightOperations->Append(testOperation);
        THROW_ON_FAILURE(NT_SUCCESS(status));
        return;
    }

    KSharedArray<TestOperation>::SPtr inflightOperationArray = _new(GetThisAllocationTag(), GetThisAllocator())KSharedArray<TestOperation>();
    ASSERT_IFNOT(
        inflightOperationArray != nullptr,
        "{0}:{1}:{2} AddOperationToMap: InflightOperationArray should not be nullptr.",
        TracePartitionId,
        ReplicaId,
        StateProviderId);
    ASSERT_IFNOT(
        NT_SUCCESS(inflightOperationArray->Status()),
        "{0}:{1}:{2} AddOperationToMap: Create InflightOperationArray failed, status: {3}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        inflightOperationArray->Status());

    status = inflightOperationArray->Append(testOperation);
    ASSERT_IFNOT(
        NT_SUCCESS(status),
        "{0}:{1}:{2} AddOperationToMap: Append to InflightOperationArray failed, status: {3}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        status);

    bool itemAdded = map.TryAdd(transactionId, inflightOperationArray);
    ASSERT_IFNOT(
        itemAdded,
        "{0}:{1}:{2} AddOperationToMap: Add to InflightOperationArray failed.",
        TracePartitionId,
        ReplicaId,
        StateProviderId);
}

void TestStateProvider::VerifyOperationList(
    __in KSharedArray<TestOperation> const& listOne, 
    __in KSharedArray<TestOperation> const& listTwo) const
{
    ASSERT_IFNOT(
        listOne.Count() == listTwo.Count(),
        "{0}:{1}:{2} VerifyOperationList: Count must be the same, listOne: {3}, listTwo: {4}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        listOne.Count(),
        listTwo.Count());

    for (ULONG i = 0; i < listOne.Count(); i++)
    {
        TestOperation operationOne = listOne[i];
        TestOperation operationTwo = listTwo[i];

        ASSERT_IFNOT(
            operationOne == operationTwo,
            "{0}:{1}:{2} VerifyOperationList: Operation must be the same.",
            TracePartitionId,
            ReplicaId,
            StateProviderId);
    }
}

void TestStateProvider::VerifyUnlocked(
    __in KSharedArray<TestOperation> const & listOne) const
{
    for (ULONG i = 0; i < listOne.Count(); i++)
    {
        TestOperation operationOne = listOne[i];

        if (operationOne.Context != nullptr)
        {
            TestOperationContext const & testOperationContext = dynamic_cast<TestOperationContext const &>(*operationOne.Context);
            VERIFY_IS_TRUE(testOperationContext.IsUnlocked);
        }
    }
}

void TestStateProvider::ThrowExceptionIfFaultyAPI(__in FaultStateProviderType::FaultAPI currentAPI) const
{
    if (faultyAPI_ == currentAPI)
    {
        throw ktl::Exception(SF_STATUS_COMMUNICATION_ERROR);
    }
}

KStringView TestStateProvider::GetInputTypeName() const
{
    return typeName_;
}

TestStateProvider::TestStateProvider(
    __in Data::StateManager::FactoryArguments const & factoryArguments,
    __in FaultStateProviderType::FaultAPI faultyAPI)
    : PartitionedReplicaTraceComponent(*PartitionedReplicaId::Create(factoryArguments.PartitionId, factoryArguments.ReplicaId, GetThisAllocator()))
    , name_(*factoryArguments.Name)
    , typeName_(*factoryArguments.TypeName)
    , stateProviderId_(factoryArguments.StateProviderId)
    , partitionId_(factoryArguments.PartitionId)
    , replicaId_(factoryArguments.ReplicaId)
    , faultyAPI_(faultyAPI)
    , initializationParameters_(factoryArguments.InitializationParameters.RawPtr())
    , userOperationDataArray_(GetThisAllocator())
    , userMetadataOperationData_(nullptr)
    , userRedoOrUndoOperationData_(nullptr)
    , lock_()
    , prepareCheckpointArray(GetThisAllocator())
    , performCheckpointArray(GetThisAllocator())
    , completeCheckpointArray(GetThisAllocator())
    , recoverCheckpointArray(GetThisAllocator())
    , expectedPrepareCheckpointArray(GetThisAllocator())
    , expectedPerformCheckpointArray(GetThisAllocator())
    , expectedCompleteCheckpointArray(GetThisAllocator())
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    if (faultyAPI_ == FaultStateProviderType::FaultAPI::Constructor)
    {
        SetConstructorStatus(SF_STATUS_COMMUNICATION_ERROR);
        return;
    }

    status = ConcurrentDictionary<LONG64, KSharedArray<TestOperation>::SPtr>::Create(
        GetThisAllocator(),
        infligtOperationMaps_);
    ASSERT_IFNOT(
        NT_SUCCESS(status),
        "{0}:{1}:{2} Ctor: Create infligtOperationMaps failed, status: {3}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        status);

    status = ConcurrentDictionary<LONG64, KSharedArray<TestOperation>::SPtr>::Create(
        GetThisAllocator(),
        expectedToBeAppliedMap_);
    ASSERT_IFNOT(
        NT_SUCCESS(status),
        "{0}:{1}:{2} Ctor: Create expectedToBeAppliedMap failed, status: {3}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        status);

    status = ConcurrentDictionary<LONG64, KSharedArray<TestOperation>::SPtr>::Create(
        GetThisAllocator(),
        appliedMap_);
    ASSERT_IFNOT(
        NT_SUCCESS(status),
        "{0}:{1}:{2} Ctor: Create appliedMap failed, status: {3}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        status);

    status = ReaderWriterAsyncLock::Create(
        GetThisAllocator(), 
        GetThisAllocationTag(),
        lock_);
    ASSERT_IFNOT(
        NT_SUCCESS(status),
        "{0}:{1}:{2} Ctor: Create lock failed, status: {3}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        status);
}

TestStateProvider::~TestStateProvider()
{
    // Only check the life state if no faulty APIs. Since faulty API may happen at any life state.
    ASSERT_IFNOT(
        faultyAPI_ != FaultStateProviderType::None || lifeState_ == Created || lifeState_ == Initialized || lifeState_ == Closed,
        "{0}:{1}:{2} Dtor: LifeState check, lifeState: {3}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        static_cast<LONG32>(lifeState_));
}
