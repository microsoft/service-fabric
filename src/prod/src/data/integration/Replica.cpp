// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data::Integration;

using namespace ktl;
using namespace Common;
using namespace Data::Utilities;
using namespace TxnReplicator;

using namespace Data::TestCommon;
using namespace Data::TStore;

Replica::SPtr Replica::Create(
    __in KGuid const & pId,
    __in LONG64 replicaId,
    __in wstring const & workFolder,
    __in Data::Log::LogManager & logManager,
    __in KAllocator & allocator,
    __in_opt Data::StateManager::IStateProvider2Factory * stateProviderFactory,
    __in_opt TRANSACTIONAL_REPLICATOR_SETTINGS const * const settings,
    __in_opt TxnReplicator::IDataLossHandler * const dataLossHandler,
    __in_opt BOOLEAN hasPersistedState)
{
    Replica * value = _new(REPLICA_TAG, allocator) Replica(pId, replicaId, workFolder, logManager, stateProviderFactory, settings, dataLossHandler, hasPersistedState);
    THROW_ON_ALLOCATION_FAILURE(value);

    return Replica::SPtr(value);
}

TxnReplicator::ITransactionalReplicator::SPtr Replica::get_TxnReplicator() const
{
    return transactionalReplicator_;
}

ktl::Awaitable<void> Replica::OpenAsync()
{
    KShared$ApiEntry();

    AwaitableCompletionSource<HRESULT>::SPtr acs = nullptr;
    NTSTATUS status = ktl::AwaitableCompletionSource<HRESULT>::Create(GetThisAllocator(), GetThisAllocationTag(), acs);
    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    Awaitable<HRESULT> openAwaitable = acs->GetAwaitable();

    ComPointer<Data::TestCommon::ComAsyncOperationCallbackTestHelper > openCallback = make_com<Data::TestCommon::ComAsyncOperationCallbackTestHelper>(
        [this, acs](IFabricAsyncOperationContext * context)
    {
        ComPointer<IFabricStringResult> endpoint;
        HRESULT tmpHr = primaryReplicator_->EndOpen(context, endpoint.InitializationAddress());
        CODING_ERROR_ASSERT(SUCCEEDED(tmpHr));
        acs->SetResult(tmpHr);
    });

    ComPointer<IFabricAsyncOperationContext> openContext;
    HRESULT hr = primaryReplicator_->BeginOpen(openCallback.GetRawPointer(), openContext.InitializationAddress());
    CODING_ERROR_ASSERT(SUCCEEDED(hr));

    hr = co_await openAwaitable;
    CODING_ERROR_ASSERT(SUCCEEDED(hr));

    co_return;
}

ktl::Awaitable<void> Replica::ChangeRoleAsync(
    __in FABRIC_EPOCH epoch,
    __in FABRIC_REPLICA_ROLE newRole)
{
    KShared$ApiEntry();

    FABRIC_EPOCH tmpEpoch(epoch);

    AwaitableCompletionSource<HRESULT>::SPtr acs = nullptr;
    NTSTATUS status = ktl::AwaitableCompletionSource<HRESULT>::Create(GetThisAllocator(), GetThisAllocationTag(), acs);
    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    Awaitable<HRESULT> changeRoleAwaitable = acs->GetAwaitable();

    ComPointer<Data::TestCommon::ComAsyncOperationCallbackTestHelper> changeRoleCallback = make_com<Data::TestCommon::ComAsyncOperationCallbackTestHelper>(
        [this, acs](IFabricAsyncOperationContext * context)
    {
        HRESULT tmpHr = primaryReplicator_->EndChangeRole(context);
        CODING_ERROR_ASSERT(SUCCEEDED(tmpHr));
        acs->SetResult(tmpHr);
    });

    ComPointer<IFabricAsyncOperationContext> changeRoleContext;
    HRESULT hr = primaryReplicator_->BeginChangeRole(
        &tmpEpoch,
        newRole,
        changeRoleCallback.GetRawPointer(),
        changeRoleContext.InitializationAddress());
    CODING_ERROR_ASSERT(SUCCEEDED(hr));

    hr = co_await changeRoleAwaitable;
    CODING_ERROR_ASSERT(SUCCEEDED(hr));

    co_return;
};

ktl::Awaitable<void> Replica::CloseAsync()
{
    KShared$ApiEntry();

    AwaitableCompletionSource<HRESULT>::SPtr acs = nullptr;
    NTSTATUS status =  ktl::AwaitableCompletionSource<HRESULT>::Create(GetThisAllocator(), GetThisAllocationTag(), acs);
    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    Awaitable<HRESULT> closeAwaitable = acs->GetAwaitable();

    // Close the Replicator
    ComPointer<Data::TestCommon::ComAsyncOperationCallbackTestHelper > closeCallback = make_com<Data::TestCommon::ComAsyncOperationCallbackTestHelper>(
        [this, acs](IFabricAsyncOperationContext * context)
    {
        HRESULT tmpHr = primaryReplicator_->EndClose(context);
        CODING_ERROR_ASSERT(SUCCEEDED(tmpHr));
        acs->SetResult(tmpHr);
    });

    ComPointer<IFabricAsyncOperationContext> closeContext;
    HRESULT hr = primaryReplicator_->BeginClose(closeCallback.GetRawPointer(), closeContext.InitializationAddress());
    CODING_ERROR_ASSERT(SUCCEEDED(hr));

    hr = co_await closeAwaitable;
    CODING_ERROR_ASSERT(SUCCEEDED(hr));

    co_return;
}

ktl::Awaitable<NTSTATUS> Replica::OnDataLossAsync(
    __out BOOLEAN & isStateChanged)
{
    KShared$ApiEntry();

    AwaitableCompletionSource<HRESULT>::SPtr acs = nullptr;
    NTSTATUS status = ktl::AwaitableCompletionSource<HRESULT>::Create(GetThisAllocator(), GetThisAllocationTag(), acs);
    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    Awaitable<HRESULT> onDataLossAwaitable = acs->GetAwaitable();

    ComPointer<Data::TestCommon::ComAsyncOperationCallbackTestHelper > dataLossCallback = make_com<Data::TestCommon::ComAsyncOperationCallbackTestHelper>(
        [this, acs, &isStateChanged](IFabricAsyncOperationContext * context)
    {
        HRESULT tmpHr = primaryReplicator_->EndOnDataLoss(context, &isStateChanged);
        acs->SetResult(tmpHr);
    });

    ComPointer<IFabricAsyncOperationContext> dataLossContext;
    HRESULT hr = primaryReplicator_->BeginOnDataLoss(dataLossCallback.GetRawPointer(), dataLossContext.InitializationAddress());
    CODING_ERROR_ASSERT(SUCCEEDED(hr));

    hr = co_await onDataLossAwaitable;
    co_return StatusConverter::Convert(hr);
}

void Replica::SetReadStatus(__in FABRIC_SERVICE_PARTITION_ACCESS_STATUS readStatus)
{
    partition_->SetReadStatus(readStatus);
}

void Replica::SetWriteStatus(__in FABRIC_SERVICE_PARTITION_ACCESS_STATUS writeStatus)
{
    partition_->SetWriteStatus(writeStatus);
}

Replica::Replica(
    __in KGuid const & partitionId,
    __in LONG64 replicaId,
    __in wstring const & workFolder,
    __in Data::Log::LogManager & logManager,
    __in Data::StateManager::IStateProvider2Factory * stateProviderFactory,
    __in TRANSACTIONAL_REPLICATOR_SETTINGS const * const settings,
    __in_opt TxnReplicator::IDataLossHandler * const dataLossHandler,
    __in_opt BOOLEAN hasPersistedState)
    : Common::ComponentRoot()
    , KObject()
    , KShared()
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    HANDLE handle = nullptr;

    PartitionedReplicaId::SPtr partitionedReplicaIdCSPtr = PartitionedReplicaId::Create(partitionId, replicaId, GetThisAllocator());

    partition_ = TestComStatefulServicePartition::Create(
        partitionId, 
        GetThisAllocator());
    partition_->SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY);
    partition_->SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY);

    TestComCodePackageActivationContext::SPtr codePackage = TestComCodePackageActivationContext::Create(
        workFolder.c_str(), GetThisAllocator());

    Reliability::ReplicationComponent::ReplicatorFactoryConstructorParameters replicatorFactoryConstructorParameters;
    replicatorFactoryConstructorParameters.Root = this;
    replicatorFactory_ = Reliability::ReplicationComponent::ReplicatorFactoryFactory(replicatorFactoryConstructorParameters);
    replicatorFactory_->Open(L"empty");

    TransactionalReplicatorFactoryConstructorParameters transactionalReplicatorFactoryConstructorParameters;
    transactionalReplicatorFactoryConstructorParameters.Root = this;
    transactionalReplicatorFactory_ = TransactionalReplicatorFactoryFactory(transactionalReplicatorFactoryConstructorParameters);
    transactionalReplicatorFactory_->Open(static_cast<KtlSystemBase &>(GetThisKtlSystem()), logManager);

    ComPointer<IFabricStateProvider2Factory> comFactory;

    if (stateProviderFactory == nullptr)
    {
        StoreStateProviderFactory::SPtr factory = StoreStateProviderFactory::CreateIntIntFactory(GetThisAllocator());
        comFactory = TxnReplicator::TestCommon::TestComStateProvider2Factory::Create(
            *factory, 
            GetThisAllocator());
    }
    else
    {
        comFactory = TxnReplicator::TestCommon::TestComStateProvider2Factory::Create(
            *stateProviderFactory, 
            GetThisAllocator());
    }

    TxnReplicator::IDataLossHandler::SPtr localDataLossHandlerSPtr = dataLossHandler;
    if (localDataLossHandlerSPtr == nullptr)
    {
        localDataLossHandlerSPtr = TxnReplicator::TestCommon::TestDataLossHandler::Create(GetThisAllocator()).RawPtr();
    }

    ComPointer<IFabricDataLossHandler> comProxyDataLossHandler = TxnReplicator::TestCommon::TestComProxyDataLossHandler::Create(
        GetThisAllocator(),
        localDataLossHandlerSPtr.RawPtr());
    
    status = transactionalReplicatorFactory_->CreateReplicator(
        replicaId,
        *replicatorFactory_,
        partition_.RawPtr(),
        nullptr,
        settings,
        nullptr,
        *codePackage,
        hasPersistedState,
        nullptr,
        comFactory.GetRawPointer(),
        comProxyDataLossHandler.GetRawPointer(),
        primaryReplicator_.InitializationAddress(),
        &handle);

    ITransactionalReplicator * txnReplicatorPtr = static_cast<ITransactionalReplicator *>(handle);
    CODING_ERROR_ASSERT(txnReplicatorPtr != nullptr);

    transactionalReplicator_.Attach(txnReplicatorPtr);

    CODING_ERROR_ASSERT(primaryReplicator_.GetRawPointer() != nullptr);
    CODING_ERROR_ASSERT(transactionalReplicator_ != nullptr);
}

Replica::~Replica()
{
    replicatorFactory_->Close();
    replicatorFactory_.reset();
    transactionalReplicatorFactory_->Close();
    transactionalReplicatorFactory_.reset();
    primaryReplicator_.Release();

    transactionalReplicator_.Reset();
}

