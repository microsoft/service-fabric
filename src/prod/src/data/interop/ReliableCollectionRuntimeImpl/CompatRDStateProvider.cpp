// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data::Interop;
using namespace std;
using namespace ktl;
using namespace Data::Utilities;
using namespace TxnReplicator;
using namespace Data::StateManager;
using namespace Data::TStore;

Common::StringLiteral const TraceComponent = "CompatRDStateProvider";
const wstring CompatRDStateProvider::RelativePathForDataStore(L"dataStore");

NTSTATUS CompatRDStateProvider::Create(
    __in FactoryArguments const & factoryArguments,
    __in KAllocator & allocator,
    __out CompatRDStateProvider::SPtr& dummyStateProvider)
{
    CompatRDStateProvider *pointer = _new(RELIABLECOLLECTIONRUNTIME_TAG, allocator) CompatRDStateProvider(factoryArguments);
    if (pointer == nullptr)
        return STATUS_INSUFFICIENT_RESOURCES;

    dummyStateProvider = pointer;
    return STATUS_SUCCESS;
}

KUriView const CompatRDStateProvider::GetName() const
{
    return *name_;
}

KArray<TxnReplicator::StateProviderInfo> CompatRDStateProvider::GetChildren(
    __in KUriView const & rootName)
{
    ASSERT_IFNOT(
        rootName.Compare(*name_) == TRUE,
        "{0}:{1}:{2} GetChildren: rootName and state provider name should be same, RootName: {3}, StateProviderName: {4}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        ToStringLiteral(rootName),
        ToStringLiteral(*name_));

    KArray<TxnReplicator::StateProviderInfo> children = KArray<TxnReplicator::StateProviderInfo>(GetThisAllocator());
    TraceAndThrowOnFailure(children.Status(), L"GetChildren: Create children array failed.");

    KUri::SPtr relativePathForDataStoreUri;
    KUri::SPtr dataStoreName;
    KUriView name(*name_);

    NTSTATUS status = KUri::Create(RelativePathForDataStore.c_str(), GetThisAllocator(), relativePathForDataStoreUri);
    TraceAndThrowOnFailure(status, L"GetChildren: Create data store relative name failed.");

    status = KUri::Create(name, *relativePathForDataStoreUri, GetThisAllocator(), dataStoreName);
    TraceAndThrowOnFailure(status, L"GetChildren: Create data store name failed.");

    TxnReplicator::StateProviderInfo dataStoreStateProviderInfo(
        *typename_,
        const_cast<KUri const &>(*dataStoreName),
        initializationParameters_.RawPtr());

    status = children.Append(dataStoreStateProviderInfo);
    TraceAndThrowOnFailure(status, L"GetChildren: Append children array failed.");

    RCREventSource::Events->CompatRDStateProviderGetChildren(
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        ToStringLiteral(name.Get(KUriView::eRaw)),
        ToStringLiteral(dataStoreName->Get(KUriView::eRaw)));

    return children;
}

void CompatRDStateProvider::Initialize(
    __in KWeakRef<ITransactionalReplicator> & transactionalReplicatorWRef,
    __in KStringView const& workFolder,
    __in KSharedArray<IStateProvider2::SPtr> const * const children)
{
    UNREFERENCED_PARAMETER(workFolder);
    UNREFERENCED_PARAMETER(transactionalReplicatorWRef.IsAlive());
    KUriView name(*name_);
    KUri::SPtr dataStoreName;
    KUri::SPtr relativePathForDataStoreUri;

    if (children == nullptr)
        TraceAndThrowOnFailure(STATUS_INVALID_PARAMETER, L"Initialize: Children array is null");

    if (children->Count() != 1)
        TraceAndThrowOnFailure(STATUS_INVALID_PARAMETER, L"Initialize: Children array count is not 1");

    NTSTATUS status = KUri::Create(RelativePathForDataStore.c_str(), GetThisAllocator(), relativePathForDataStoreUri);
    TraceAndThrowOnFailure(status, L"GetChildren: Create data store relative name failed.");

    status = KUri::Create(name, *relativePathForDataStoreUri, GetThisAllocator(), dataStoreName);
    TraceAndThrowOnFailure(status, L"Initialize: Create data store name failed.");

    IStateProvider2::SPtr childStateProvider = (*children)[0];
    KUriView childStateProviderName = childStateProvider->GetName();
    if (!childStateProviderName.Compare(*dataStoreName))
        TraceAndThrowOnFailure(STATUS_INVALID_PARAMETER, L"Initialize: child state provider name does not match");

    dataStore_ = dynamic_cast<IStore<KString::SPtr, KBuffer::SPtr>*>(childStateProvider.RawPtr());
    if (dataStore_ == nullptr)
        TraceAndThrowOnFailure(STATUS_INVALID_PARAMETER, L"Initialize: dynamic_cast to IStore failed.");

    RCREventSource::Events->CompatRDStateProviderInitialize(
        TracePartitionId,
        ReplicaId,
        stateProviderId_,
        ToStringLiteral(name_->Get(KUriView::eRaw)));
}

Awaitable<void> CompatRDStateProvider::OpenAsync(__in CancellationToken const& cancellationToken)
{
    UNREFERENCED_PARAMETER(cancellationToken);

    co_return;
}

Awaitable<void> CompatRDStateProvider::ChangeRoleAsync(
    __in FABRIC_REPLICA_ROLE newRole,
    __in CancellationToken const& cancellationToken)
{
    UNREFERENCED_PARAMETER(newRole);
    UNREFERENCED_PARAMETER(cancellationToken);
    co_return;
}

Awaitable<void> CompatRDStateProvider::CloseAsync(CancellationToken const& cancellationToken)
{
    UNREFERENCED_PARAMETER(cancellationToken);

    co_return;
}

void CompatRDStateProvider::Abort() noexcept
{
}

void CompatRDStateProvider::PrepareCheckpoint(__in FABRIC_SEQUENCE_NUMBER checkpointLSN)
{
    UNREFERENCED_PARAMETER(checkpointLSN);
}

Awaitable<void> CompatRDStateProvider::PerformCheckpointAsync(__in CancellationToken const& cancellationToken)
{
    UNREFERENCED_PARAMETER(cancellationToken);

    co_return;
}

Awaitable<void> CompatRDStateProvider::CompleteCheckpointAsync(__in CancellationToken const& cancellationToken)
{
    UNREFERENCED_PARAMETER(cancellationToken);

    co_return;
}


void CompatRDStateProvider::TraceAndThrowOnFailure(
    __in NTSTATUS status,
    __in wstring const & traceInfo) const
{
    if (!NT_SUCCESS(status))
    {
        RCREventSource::Events->CompatRDStateProviderError(
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            traceInfo,
            status);
        throw ktl::Exception(status);
    }
}


Awaitable<void> CompatRDStateProvider::RecoverCheckpointAsync(__in CancellationToken const & cancellationToken)
{
    UNREFERENCED_PARAMETER(cancellationToken);

    co_return;
}

Awaitable<void> CompatRDStateProvider::BackupCheckpointAsync(
    __in KString const & backupDirectory,
    __in CancellationToken const & cancellationToken)
{
    UNREFERENCED_PARAMETER(backupDirectory);
    UNREFERENCED_PARAMETER(cancellationToken);

    co_return;
}

Awaitable<void> CompatRDStateProvider::RestoreCheckpointAsync(
    __in KString const & backupDirectory,
    __in CancellationToken const & cancellationToken)
{
    UNREFERENCED_PARAMETER(backupDirectory);
    UNREFERENCED_PARAMETER(cancellationToken);

    co_return;
}

Awaitable<void> CompatRDStateProvider::RemoveStateAsync(
    __in CancellationToken const& cancellationToken)
{
    UNREFERENCED_PARAMETER(cancellationToken);

    co_return;
}

OperationDataStream::SPtr CompatRDStateProvider::GetCurrentState()
{
    return nullptr;
}

ktl::Awaitable<void> CompatRDStateProvider::BeginSettingCurrentStateAsync()
{
    co_return;
}

ktl::Awaitable<void> CompatRDStateProvider::EndSettingCurrentStateAsync(
    __in ktl::CancellationToken const& cancellationToken)
{
    co_return;
}

Awaitable<void> CompatRDStateProvider::SetCurrentStateAsync(
    __in LONG64 stateRecordNumber,
    __in OperationData const & copyOperationData,
    __in CancellationToken const & cancellationToken)
{
    UNREFERENCED_PARAMETER(stateRecordNumber);
    UNREFERENCED_PARAMETER(copyOperationData);
    UNREFERENCED_PARAMETER(cancellationToken);

    co_return;
}

Awaitable<void> CompatRDStateProvider::PrepareForRemoveAsync(
    __in TransactionBase const& transactionBase,
    __in CancellationToken const& cancellationToken)
{
    UNREFERENCED_PARAMETER(transactionBase);
    UNREFERENCED_PARAMETER(cancellationToken);

    co_return;
}

Awaitable<OperationContext::CSPtr> CompatRDStateProvider::ApplyAsync(
    __in LONG64 logicalSequenceNumber,
    __in TransactionBase const & transactionBase,
    __in ApplyContext::Enum applyContext,
    __in_opt OperationData const * const metadataPtr,
    __in_opt OperationData const * const dataPtr)
{
    UNREFERENCED_PARAMETER(logicalSequenceNumber);
    UNREFERENCED_PARAMETER(transactionBase);
    UNREFERENCED_PARAMETER(applyContext);
    UNREFERENCED_PARAMETER(metadataPtr);
    UNREFERENCED_PARAMETER(dataPtr);
    // TODO ASSERT
    co_return nullptr;
}

void CompatRDStateProvider::Unlock(__in OperationContext const & operationContext)
{
    UNREFERENCED_PARAMETER(operationContext);
    // TODO ASSERT
}

#pragma region IStateProviderInfo
KString* CompatRDStateProvider::GetLangTypeInfo(KStringView const & lang) const
{
    UNREFERENCED_PARAMETER(lang);
    return const_cast<KString*>(typename_.RawPtr());
}

NTSTATUS CompatRDStateProvider::SetLangTypeInfo(KStringView const & lang, KStringView const & typeInfo)
{
    UNREFERENCED_PARAMETER(lang);
    UNREFERENCED_PARAMETER(typeInfo);
    return STATUS_SUCCESS;
}

Data::StateProviderKind CompatRDStateProvider::GetKind() const
{
    return Data::StateProviderKind::ReliableDictionary_Compat;
}
#pragma endregion IStateProviderInfo

CompatRDStateProvider::CompatRDStateProvider(
    __in FactoryArguments const & factoryArguments)
     : KObject()
    , KShared()
    , name_(factoryArguments.Name)
    , dataStore_()
    , typename_(factoryArguments.TypeName)
    , stateProviderId_(factoryArguments.StateProviderId)
    , initializationParameters_(factoryArguments.InitializationParameters)
    , partitionId_(factoryArguments.PartitionId)
    , replicaId_(factoryArguments.ReplicaId)
    , PartitionedReplicaTraceComponent(*PartitionedReplicaId::Create(factoryArguments.PartitionId, factoryArguments.ReplicaId, GetThisAllocator()))
{
    RCREventSource::Events->CompatRDStateProviderCtor(
        TracePartitionId,
        ReplicaId,
        stateProviderId_,
        ToStringLiteral(name_->Get(KUriView::eRaw)));
}

CompatRDStateProvider::~CompatRDStateProvider()
{
}
