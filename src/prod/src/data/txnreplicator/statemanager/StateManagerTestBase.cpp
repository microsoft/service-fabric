// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace StateManagerTests;

using namespace Data::Utilities;
using namespace TxnReplicator;
using namespace Data::StateManager;

TestTransactionManager::SPtr StateManagerTestBase::CreateReplica(
    __in PartitionedReplicaId const& traceId,
    __in IRuntimeFolders & runtimeFolders,
    __in IStatefulPartition& partition)
{
    auto internalSettings = TRInternalSettings::Create(
        move(this->CreateNativeTRSettings()),
        make_shared<TransactionalReplicatorConfig>());
    return CreateReplica(traceId, runtimeFolders, partition, internalSettings, true);
}

TestTransactionManager::SPtr StateManagerTestBase::CreateVolatileReplica(
    __in PartitionedReplicaId const& traceId,
    __in IRuntimeFolders & runtimeFolders,
    __in IStatefulPartition& partition)
{
    auto internalSettings = TRInternalSettings::Create(
        move(this->CreateNativeTRSettings()),
        make_shared<TransactionalReplicatorConfig>());
    return CreateReplica(traceId, runtimeFolders, partition, internalSettings, false);
}

TestTransactionManager::SPtr StateManagerTestBase::CreateReplica(
    __in PartitionedReplicaId const& traceId, 
    __in IRuntimeFolders & runtimeFolders, 
    __in IStatefulPartition& partition,
    __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
    __in bool hasPersistedState)
{
    auto txnReplicator = TestTransactionManager::Create(GetAllocator());
    auto testReplicator = MockLoggingReplicator::Create(hasPersistedState, GetAllocator());
    auto testStateManager = CreateStateManager(
        traceId,
        runtimeFolders,
        partition,
        transactionalReplicatorConfig,
        hasPersistedState);

    testStateManager->Initialize(*txnReplicator, *testReplicator);
    testReplicator->Initialize(*testStateManager);
    txnReplicator->Initialize(*testReplicator, *testStateManager);

    return Ktl::Move(txnReplicator);
}

StateManager::SPtr StateManagerTestBase::CreateStateManager(
    __in PartitionedReplicaId const & traceId,
    __in IRuntimeFolders & runtimeFolders, 
    __in IStatefulPartition & partition,
    __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
    __in bool hasPersistedState)
{
    StateManager::SPtr stateManagerSPtr = nullptr;
    NTSTATUS status = StateManager::Create(
        traceId,
        runtimeFolders,
        partition,
        *stateProviderFactorySPtr_,
        transactionalReplicatorConfig,
        hasPersistedState,
        underlyingSystem_->PagedAllocator(),
        stateManagerSPtr);

    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    return stateManagerSPtr;
}

KUri::CSPtr StateManagerTestBase::GetStateProviderName(__in NameType nameType, __in LONG64 parentID, __in LONG64 childrenID)
{
    NTSTATUS status;
    BOOLEAN isSuccess = TRUE;

    KString::SPtr rootString = nullptr;;
    switch (nameType)
    {
    case Null:
        return nullptr;
    case Empty:
        status = KString::Create(rootString, GetAllocator(), L"");
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        break;
    case Schemaless:
        status = KString::Create(rootString, GetAllocator(), L"schemalessname");
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        break;
    case Valid:
        status = KString::Create(rootString, GetAllocator(), L"fabric:/parent");
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        break;
    case Hierarchy:
        {
            // Parent ID start from 0, which can be fabric:/parent0, fabric:/parent1, fabric:/parent2...
            // Children ID start from 1 which can be fabric:/parent0/1, fabric:/parent0/2, fabric:/parent0/3...
            // Used Children ID 0 to check it is parent or children.
            status = KString::Create(rootString, GetAllocator(), L"fabric:/parent");
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            WCHAR concatParentID = static_cast<WCHAR>('0'+ parentID);

            BOOLEAN concatSuccess = rootString->AppendChar(concatParentID);
            CODING_ERROR_ASSERT(concatSuccess == TRUE);

            concatSuccess = rootString->SetNullTerminator();
            CODING_ERROR_ASSERT(concatSuccess == TRUE);

            if (childrenID > 0)
            {
                concatSuccess = rootString->Concat(L"/");
                CODING_ERROR_ASSERT(concatSuccess == TRUE);
                WCHAR concatChildrenID = static_cast<WCHAR>('0' + childrenID);
                concatSuccess = rootString->AppendChar(concatChildrenID);
                CODING_ERROR_ASSERT(concatSuccess == TRUE);

                concatSuccess = rootString->SetNullTerminator();
                CODING_ERROR_ASSERT(concatSuccess == TRUE);
            }
        }
        break;
    case Random:
        status = KString::Create(rootString, GetAllocator(), L"fabric:/parent/");
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        KGuid guid;
        guid.CreateNew();
        KLocalString<40> stateProviderId;
        isSuccess = stateProviderId.FromGUID(static_cast<GUID>(guid));
        CODING_ERROR_ASSERT(isSuccess == TRUE);

        isSuccess = rootString->Concat(stateProviderId);
        CODING_ERROR_ASSERT(isSuccess == TRUE);
        break;
    }

    KAllocator& allocator = underlyingSystem_->NonPagedAllocator();
    KUri::SPtr output = nullptr;
    status = KUri::Create(*rootString, allocator, output);
    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    return output.RawPtr();
}

KAllocator& StateManagerTestBase::GetAllocator()
{
    return underlyingSystem_->PagedAllocator();
}

ktl::Awaitable<void> StateManagerTestBase::PopulateAsync(
    __in KArray<KUri::CSPtr> const & nameList, 
    __in_opt OperationData const * const initializationParameters)
{
    auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

    for (ULONG i = 0; i < nameList.Count(); i++)
    {
        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] {txnSPtr->Dispose(); });

            NTSTATUS status = co_await stateManagerSPtr->AddAsync(
                *txnSPtr,
                *nameList[i],
                TestStateProvider::TypeName,
                initializationParameters);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            TEST_COMMIT_ASYNC(txnSPtr);
        }
    }

    co_return;
}

ktl::Awaitable<void> StateManagerTestBase::DepopulateAsync(
    __in KArray<KUri::CSPtr> const& nameList,
    __in_opt TxnReplicator::Transaction * const transaction)
{
    if (nameList.Count() == 0)
    {
        co_return;
    }

    Transaction::SPtr txn = transaction;
    if (txn == nullptr)
    {
        testTransactionalReplicatorSPtr_->CreateTransaction(txn);
    }

    auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

    for (ULONG i = 0; i < nameList.Count(); i++)
    {
        NTSTATUS status = co_await stateManagerSPtr->RemoveAsync(
            *txn,
            *nameList[i]);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    if (transaction == nullptr)
    {
        TEST_COMMIT_ASYNC(txn);
        txn->Dispose();
    }

    co_return;
}

KArray<LONG64> StateManagerTestBase::PopulateStateProviderIdArray(__in KArray<KUri::CSPtr> const& nameList)
{
    KArray<LONG64> array(GetAllocator(), nameList.Count());

    for (ULONG32 i = 0; i < nameList.Count(); i++)
    {
        IStateProvider2::SPtr stateProvider;
        NTSTATUS status = testTransactionalReplicatorSPtr_->StateManager->Get(*(nameList[i]), stateProvider);
        VERIFY_IS_TRUE(NT_SUCCESS(status), L"Item must exist");
        VERIFY_IS_NOT_NULL(stateProvider);
        VERIFY_ARE_EQUAL(*(nameList[i]), stateProvider->GetName());

        TestStateProvider& testStateProvider = dynamic_cast<TestStateProvider &>(*stateProvider);
        status = array.Append(testStateProvider.StateProviderId);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    return Ktl::Move(array);
}

void StateManagerTestBase::VerifyExist(
    __in KArray<KUri::CSPtr> const & nameList, 
    __in bool existanceExpected,
    __in_opt IStateManager * const stateManager,
    __in bool isInitParameterNull)
{
    IStateManager::SPtr stateManagerSPtr;
    if (stateManager == nullptr)
    {
        stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager.RawPtr();
    }
    else
    {
        stateManagerSPtr = stateManager;
    }

    for (ULONG i = 0; i < nameList.Count(); i++)
    {
        IStateProvider2::SPtr stateProvider;
        NTSTATUS status = stateManagerSPtr->Get(*nameList[i], stateProvider);

        if (existanceExpected)
        {
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_NOT_NULL(stateProvider);
            VERIFY_ARE_EQUAL(*nameList[i], stateProvider->GetName());
        }
        else
        {
            VERIFY_ARE_EQUAL(status, SF_STATUS_NAME_DOES_NOT_EXIST);
            VERIFY_IS_NULL(stateProvider);
        }

        if (NT_SUCCESS(status))
        {
            TestStateProvider & testStateProvider = dynamic_cast<TestStateProvider &>(*stateProvider);
            if (isInitParameterNull)
            {
                CODING_ERROR_ASSERT(testStateProvider.GetInitializationParameters() == nullptr);
            }
            else
            {
                TestHelper::VerifyInitParameters(*testStateProvider.GetInitializationParameters());
            }
        }
    }
}

void StateManagerTestBase::DisableStateProviderCheckpointing(
    __in KArray<KUri::CSPtr> const & nameList)
{
    for (ULONG32 i = 0; i < nameList.Count(); i++)
    {
        IStateProvider2::SPtr stateProvider;
        NTSTATUS status = testTransactionalReplicatorSPtr_->StateManager->Get(*(nameList[i]), stateProvider);
        VERIFY_IS_TRUE(NT_SUCCESS(status), L"Item must exist");
        VERIFY_IS_NOT_NULL(stateProvider);
        VERIFY_ARE_EQUAL(*(nameList[i]), stateProvider->GetName());

        TestStateProvider& testStateProvider = dynamic_cast<TestStateProvider &>(*stateProvider);
        testStateProvider.Checkpointing = false;
    }
}

NamedOperationData::CSPtr StateManagerTestBase::GenerateNamedMetadataOperationData(
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in KUri const & name,
    __in ApplyType::Enum applyType,
    __in_opt SerializationMode::Enum serializationMode) noexcept
{
    NTSTATUS status;

    MetadataOperationData::CSPtr metadataOperationData;
    status = MetadataOperationData::Create(stateProviderId, name, applyType, GetAllocator(), metadataOperationData);
    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    NamedOperationData::CSPtr namedMetadataOperationData;
    status = NamedOperationData::Create(
        LONG64_MIN, 
        serializationMode, 
        GetAllocator(), 
        metadataOperationData.RawPtr(), 
        namedMetadataOperationData);
    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    return namedMetadataOperationData;
}

RedoOperationData::CSPtr StateManagerTestBase::GenerateRedoOperationData(
    __in KStringView const & input, 
    __in_opt OperationData const * const initializationParameters,
    __in_opt LONG64 parentId,
    __in_opt SerializationMode::Enum serializationMode) noexcept
{
    KString::SPtr typeName;
    NTSTATUS status = KString::Create(typeName, GetAllocator(), input);
    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    RedoOperationData::CSPtr redoOperationData;
    status = RedoOperationData::Create(
        *partitionedReplicaIdCSPtr_,
        *typeName,
        initializationParameters,
        parentId,
        serializationMode,
        GetAllocator(),
        redoOperationData);
    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    return redoOperationData;
}

ktl::Awaitable<OperationContext::CSPtr> StateManagerTestBase::ApplyAsync(
    __in ktl::AwaitableCompletionSource<void> & tcs,
    __in LONG64 applyLSN,
    __in TransactionBase const & transactionBase,
    __in ApplyContext::Enum applyContext,
    __in_opt OperationData const * const metadataPtr,
    __in_opt OperationData const * const dataPtr,
    __in_opt IStateManager * const stateManager)
{
    IStateManager::SPtr tmpStateManager = stateManager == nullptr ? testTransactionalReplicatorSPtr_->StateManager.RawPtr() : stateManager;

    // snap input
    TransactionBase::CSPtr transactionBaseCSPtr(&transactionBase);
    OperationData::CSPtr metadataCSPtr(metadataPtr);
    OperationData::CSPtr dataCSPtr(dataPtr);

    co_await tcs.GetAwaitable();

    OperationContext::CSPtr operationContextCSPtr;
    NTSTATUS status = co_await tmpStateManager->ApplyAsync(
        applyLSN,
        *transactionBaseCSPtr,
        applyContext,
        metadataCSPtr.RawPtr(),
        dataCSPtr.RawPtr(),
        operationContextCSPtr);
    THROW_ON_FAILURE(status);

    co_return operationContextCSPtr;
}

ktl::Awaitable<KSharedArray<OperationContext::CSPtr>::SPtr> StateManagerTestBase::ApplyAddAsync(
    __in KArray<KUri::CSPtr> const & nameList,
    __in ULONG32 startingOffset,
    __in ULONG32 count,
    __in LONG64 startingTransactionId,
    __in LONG64 startingLSN,
    __in LONG64 startingStateProviderId,
    __in ApplyContext::Enum applyContext,
    __in_opt SerializationMode::Enum serializationMode)
{
    NTSTATUS status;
    KSharedArray<OperationContext::CSPtr>::SPtr operationContextList = _new(TEST_TAG, GetAllocator()) KSharedArray<OperationContext::CSPtr>();

    CODING_ERROR_ASSERT(startingOffset + count <= nameList.Count());

    // First do all the adds. Since each is a distinct state provider, we know they cannot be conflicting.
    // So potentially they could have been in the same group commit.
    ktl::AwaitableCompletionSource<void>::SPtr addApplyTaskListTcs;
    status = ktl::AwaitableCompletionSource<void>::Create(GetAllocator(), TEST_TAG, addApplyTaskListTcs);
    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    KArray<ktl::Awaitable<OperationContext::CSPtr>> addTaskList(GetAllocator(), count);

    for (LONG64 i = startingOffset; i < count; i++)
    {
        const KUri::CSPtr name = nameList[static_cast<ULONG>(i)];

        const LONG64 iterationCount = i - startingOffset;
        const LONG64 transactionId = iterationCount + startingTransactionId;
        const LONG64 applyLSN = iterationCount + startingLSN;
        const LONG64 stateProviderId = iterationCount + startingStateProviderId;

        auto txn = TransactionBase::CreateTransaction(transactionId, false, GetAllocator());
        auto namedMetadataOperationData = GenerateNamedMetadataOperationData(stateProviderId, *name, ApplyType::Insert, serializationMode);
        auto redoOperationData = GenerateRedoOperationData(TestStateProvider::TypeName, nullptr, Constants::EmptyStateProviderId, serializationMode);

        status = addTaskList.Append(ApplyAsync(
            *addApplyTaskListTcs,
            applyLSN,
            *txn,
            applyContext,
            namedMetadataOperationData.RawPtr(),
            redoOperationData.RawPtr()));

        CODING_ERROR_ASSERT(NT_SUCCESS(status));
    }

    // Open the flood gates for add operations.
    addApplyTaskListTcs->Set();

    // Ensure that all add applies are completed before we start the remove applies.
    co_await TaskUtilities<OperationContext::CSPtr>::WhenAll(addTaskList);

    for (LONG32 i = addTaskList.Count() - 1; i >= 0; i--)
    {
        OperationContext::CSPtr operationContext = co_await addTaskList[i];
        VERIFY_IS_NOT_NULL(operationContext);
        status = operationContextList->Append(operationContext);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
    }

    co_return Ktl::Move(operationContextList);
}

ktl::Awaitable<KSharedArray<OperationContext::CSPtr>::SPtr> StateManagerTestBase::ApplyRemoveAsync(
    __in KArray<KUri::CSPtr> const & nameList,
    __in ULONG32 startingOffset,
    __in ULONG32 count,
    __in LONG64 startingTransactionId,
    __in LONG64 startingLSN,
    __in LONG64 startingStateProviderId,
    __in ApplyContext::Enum applyContext)
{
    NTSTATUS status;
    KSharedArray<OperationContext::CSPtr>::SPtr operationContextList = _new(TEST_TAG, GetAllocator()) KSharedArray<OperationContext::CSPtr>();

    CODING_ERROR_ASSERT(startingOffset + count <= nameList.Count());

    // First do all the adds. Since each is a distict state provider, we know they cannot be conflicting.
    // So potentially they could have been in the same group commit.
    ktl::AwaitableCompletionSource<void>::SPtr tcs;
    status = ktl::AwaitableCompletionSource<void>::Create(GetAllocator(), TEST_TAG, tcs);
    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    KArray<ktl::Awaitable<OperationContext::CSPtr>> taskList(GetAllocator(), count);

    for (LONG64 i = startingOffset; i < count; i++)
    {
        const KUri::CSPtr name = nameList[static_cast<ULONG>(i)];

        const LONG64 iterationCount = i - startingOffset;
        const LONG64 transactionId = iterationCount + startingTransactionId;
        const LONG64 applyLSN = iterationCount + startingLSN;
        const LONG64 stateProviderId = iterationCount + startingStateProviderId;

        auto txn = TransactionBase::CreateTransaction(transactionId, false, GetAllocator());
        auto namedMetadataOperationData = GenerateNamedMetadataOperationData(stateProviderId, *name, ApplyType::Delete);

        status = taskList.Append(ApplyAsync(
            *tcs,
            applyLSN,
            *txn,
            applyContext,
            namedMetadataOperationData.RawPtr(),
            nullptr));

        CODING_ERROR_ASSERT(NT_SUCCESS(status));
    }

    // Open the flood gates for add operations.
    tcs->Set();

    // Ensure that all add applies are completed before we start the remove applies.
    co_await TaskUtilities<OperationContext::CSPtr>::WhenAll(taskList);

    for (LONG32 i = taskList.Count() - 1; i >= 0; i--)
    {
        OperationContext::CSPtr operationContext = co_await taskList[i];
        if (operationContext == nullptr)
        {
            continue;
        }

        status = operationContextList->Append(operationContext);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
    }

    co_return Ktl::Move(operationContextList);
}

ktl::Awaitable<KSharedArray<OperationContext::CSPtr>::SPtr> StateManagerTestBase::ApplyMultipleAsync(
    __in KArray<KUri::CSPtr> const& nameList, 
    __in KArray<LONG64> const& stateProviderIdList, 
    __in ULONG32 startingOffset, 
    __in ULONG32 count, 
    __in LONG64 startingTransactionId, 
    __in LONG64 startingLSN, 
    __in ApplyType::Enum applyType,
    __in ApplyContext::Enum applyContext,
    __in_opt IStateManager * const stateManager)
{
    NTSTATUS status;

    CODING_ERROR_ASSERT(startingOffset + count <= nameList.Count());
    CODING_ERROR_ASSERT(startingOffset + count <= stateProviderIdList.Count());

    KSharedArray<OperationContext::CSPtr>::SPtr operationContextList = _new(TEST_TAG, GetAllocator()) KSharedArray<OperationContext::CSPtr>();

    // First do all the adds. Since each is a distict state provider, we know they cannot be conflicting.
    // So potentially they could have been in the same group commit.
    ktl::AwaitableCompletionSource<void>::SPtr tcs;
    status = ktl::AwaitableCompletionSource<void>::Create(GetAllocator(), TEST_TAG, tcs);
    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    KArray<ktl::Awaitable<OperationContext::CSPtr>> taskList(GetAllocator(), count);

    for (LONG64 i = startingOffset; i < count; i++)
    {
        const KUri::CSPtr name = nameList[static_cast<ULONG>(i)];
        const LONG64 stateProviderId = stateProviderIdList[static_cast<ULONG>(i)];

        const LONG64 iterationCount = i - startingOffset;
        const LONG64 transactionId = iterationCount + startingTransactionId;
        const LONG64 applyLSN = iterationCount + startingLSN;

        auto type = applyContext & ApplyContext::FALSE_PROGRESS;

        auto txn = TransactionBase::CreateTransaction(transactionId, false, GetAllocator());
        auto namedMetadataOperationData = GenerateNamedMetadataOperationData(stateProviderId, *name, applyType);
        auto redoOperationData = (applyType == ApplyType::Insert) && (type != ApplyContext::FALSE_PROGRESS) 
                                 ? GenerateRedoOperationData(TestStateProvider::TypeName)
                                 : nullptr;

        status = taskList.Append(ApplyAsync(
            *tcs,
            applyLSN,
            *txn,
            applyContext,
            namedMetadataOperationData.RawPtr(),
            redoOperationData.RawPtr(),
            stateManager));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
    }

    // Open the flood gates for add operations.
    tcs->Set();

    // Ensure that all add applies are completed before we start the remove applies.
    co_await TaskUtilities<OperationContext::CSPtr>::WhenAll(taskList);

    for (LONG32 i = taskList.Count() - 1; i >= 0; i--)
    {
        OperationContext::CSPtr operationContext = co_await taskList[i];
        if (operationContext == nullptr)
        {
            continue;
        }

        status = operationContextList->Append(operationContext);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
    }

    co_return Ktl::Move(operationContextList);
}

void StateManagerTestBase::Unlock(
    __in KSharedArray<OperationContext::CSPtr> const & contextArray,
    __in_opt IStateManager * const stateManager)
{
    IStateManager::SPtr tmpStateManager = stateManager == nullptr ? 
        testTransactionalReplicatorSPtr_->StateManager.RawPtr() : 
        stateManager;

    for (ULONG32 i = 0; i < contextArray.Count(); i++)
    {
        tmpStateManager->Unlock(*contextArray[i]);
    }
}

TxnReplicator::TransactionalReplicatorSettingsUPtr StateManagerTestBase::CreateNativeTRSettings()
{
    TRANSACTIONAL_REPLICATOR_SETTINGS txnReplicatorSettings = { 0 };
    txnReplicatorSettings.Flags = FABRIC_TRANSACTIONAL_REPLICATOR_SERIALIZATION_VERSION;
    // All the native test should set the SerializationVersion to 1.
    txnReplicatorSettings.SerializationVersion = 1;

    TxnReplicator::TransactionalReplicatorSettingsUPtr txnReplicatorSettingsUPtr;
    TransactionalReplicatorSettings::FromPublicApi(txnReplicatorSettings, txnReplicatorSettingsUPtr);

    return txnReplicatorSettingsUPtr;
}

StateManagerTestBase::StateManagerTestBase() : StateManagerTestBase(TRInternalSettings::Create(move(this->CreateNativeTRSettings()), make_shared<TransactionalReplicatorConfig>()), true)
{
}

StateManagerTestBase::StateManagerTestBase(__in bool hasPersistedState) : StateManagerTestBase(TRInternalSettings::Create(move(this->CreateNativeTRSettings()), make_shared<TransactionalReplicatorConfig>()), hasPersistedState)
{
}

StateManagerTestBase::StateManagerTestBase(
    __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
    __in bool hasPersistedState)
{
    underlyingSystem_ = TestHelper::CreateKtlSystem();
    underlyingSystem_->SetDefaultSystemThreadPoolUsage(FALSE);
    KAllocator& allocator = underlyingSystem_->PagedAllocator();

    KGuid partitionId;
    partitionId.CreateNew();
    auto replicaId = random_.Next();

    partitionedReplicaIdCSPtr_ = PartitionedReplicaId::Create(partitionId, replicaId, allocator);
    runtimeFolders_ = TestHelper::CreateRuntimeFolders(allocator);
    partitionSPtr_ = TestHelper::CreateStatefulServicePartition(allocator);

    // Setup
    stateProviderFactorySPtr_ = TestStateProviderFactory::Create(allocator);

    testTransactionalReplicatorSPtr_ = CreateReplica(
        *partitionedReplicaIdCSPtr_,
        *runtimeFolders_,
        *partitionSPtr_,
        transactionalReplicatorConfig,
        hasPersistedState);
}

StateManagerTestBase::~StateManagerTestBase()
{
    BOOL result = testTransactionalReplicatorSPtr_.Reset();
    CODING_ERROR_ASSERT(result == TRUE);

    result = stateProviderFactorySPtr_.Reset();
    result = partitionedReplicaIdCSPtr_.Reset();
    result = runtimeFolders_.Reset();
    result = partitionSPtr_.Reset();

    MemoryBarrier();

    underlyingSystem_->Shutdown();
}
