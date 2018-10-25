// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::Utilities;
using namespace TxnReplicator;
using namespace Data::StateManager;
using namespace TestCommon;

Common::StringLiteral const TraceComponent = "NoopStateProvider";
KStringView const NoopStateProvider::TypeName = L"NoopStateProvider";

NoopStateProvider::SPtr NoopStateProvider::Create(
    __in FactoryArguments const & factoryArguments,
    __in KAllocator & allocator)
{
    NoopStateProvider * pointer = _new(TEST_STATE_PROVIDER_TAG, allocator) NoopStateProvider(factoryArguments);
    THROW_ON_ALLOCATION_FAILURE(pointer);

    return SPtr(pointer);
}

NTSTATUS NoopStateProvider::AddOperation(
    __in Transaction & transaction,
    __in Data::Utilities::OperationData const * const metadata,
    __in Data::Utilities::OperationData const * const undo,
    __in Data::Utilities::OperationData const * const redo,
    __in TxnReplicator::OperationContext const * const context)
{    
    return transaction.AddOperation(metadata, undo, redo, stateProviderId_, context);
}

KUriView const NoopStateProvider::GetName() const
{
    ASSERT_IFNOT(
        name_ != nullptr,
        "{0}:{1}:{2} GetName: name_ should not be nullptr.",
        TracePartitionId,
        ReplicaId,
        StateProviderId);

    return *name_;
}

OperationData::CSPtr NoopStateProvider::GetInitializationParameters() const
{
    return initializationParameters_;
}

KArray<StateProviderInfo> NoopStateProvider::GetChildren(
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

    KArray<StateProviderInfo> children = KArray<StateProviderInfo>(GetThisAllocator());
    TraceAndThrowOnFailure(children.Status(), L"GetChildren: Create children array failed.");

    return children;
}

void NoopStateProvider::Initialize(
    __in KWeakRef<ITransactionalReplicator> & transactionalReplicatorWRef,
    __in KStringView const& workFolder,
    __in KSharedArray<IStateProvider2::SPtr> const * const children)
{
    UNREFERENCED_PARAMETER(children);

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    replicatorWRef_ = &transactionalReplicatorWRef;
    ASSERT_IFNOT(
        replicatorWRef_ != nullptr,
        "{0}:{1}:{2} Initialize: replicatorWRef should not be nullptr.",
        TracePartitionId,
        ReplicaId,
        StateProviderId);
    ASSERT_IFNOT(
        replicatorWRef_->IsAlive(),
        "{0}:{1}:{2} Initialize: replicatorWRef is not alive.",
        TracePartitionId,
        ReplicaId,
        StateProviderId);

    KString::SPtr tmp = nullptr;
    status = KString::Create(tmp, GetThisAllocator(), workFolder);
    TraceAndThrowOnFailure(status, L"Initialize: Create workFolder string failed.");

    workFolder_ = KString::CSPtr(tmp.RawPtr());

    Trace.WriteInfo(
        TraceComponent,
        "{0}:{1}:{2} Api: InitializeAsync completed. Name: {3} WorkFolder: {4}",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        ToStringLiteral(*name_),
        ToStringLiteral(workFolder));
}

Awaitable<void> NoopStateProvider::OpenAsync(__in CancellationToken const& cancellationToken)
{
    UNREFERENCED_PARAMETER(cancellationToken);

    co_return;
}

Awaitable<void> NoopStateProvider::ChangeRoleAsync(
    __in FABRIC_REPLICA_ROLE newRole,
    __in CancellationToken const& cancellationToken)
{
    UNREFERENCED_PARAMETER(newRole);
    UNREFERENCED_PARAMETER(cancellationToken);

    co_return;
}

Awaitable<void> NoopStateProvider::CloseAsync(CancellationToken const& cancellationToken)
{
    UNREFERENCED_PARAMETER(cancellationToken);

    co_return;
}

void NoopStateProvider::Abort() noexcept
{
}

void NoopStateProvider::PrepareCheckpoint(__in FABRIC_SEQUENCE_NUMBER checkpointLSN)
{
    Trace.WriteInfo(
        TraceComponent,
        "{0}:{1}:{2} Api: PrepareCheckpoint {3}",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        checkpointLSN);
}

// PerformCheckpoint will create a actual file and save the information to the file
Awaitable<void> NoopStateProvider::PerformCheckpointAsync(__in CancellationToken const& cancellationToken)
{
    UNREFERENCED_PARAMETER(cancellationToken);

    KShared$ApiEntry();

    co_return;
}

Awaitable<void> NoopStateProvider::CompleteCheckpointAsync(__in CancellationToken const& cancellationToken)
{
    UNREFERENCED_PARAMETER(cancellationToken);

    KShared$ApiEntry();

    co_return;
}

void NoopStateProvider::TraceAndThrowOnFailure(
    __in NTSTATUS status,
    __in wstring const & traceInfo) const
{
    if (!NT_SUCCESS(status))
    {
        Trace.WriteError(
            TraceComponent,
            "{0}:{1}:{2} {3} status: {4}.",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            traceInfo,
            status);
        throw ktl::Exception(status);
    }
}

Awaitable<void> NoopStateProvider::RecoverCheckpointAsync(__in CancellationToken const & cancellationToken)
{
    UNREFERENCED_PARAMETER(cancellationToken);

    KShared$ApiEntry();

    co_return;
}

Awaitable<void> NoopStateProvider::BackupCheckpointAsync(
    __in KString const & backupDirectory,
    __in CancellationToken const & cancellationToken)
{
    UNREFERENCED_PARAMETER(backupDirectory);
    UNREFERENCED_PARAMETER(cancellationToken);

    co_return;
}

Awaitable<void> NoopStateProvider::RestoreCheckpointAsync(
    __in KString const & backupDirectory,
    __in CancellationToken const & cancellationToken)
{
    UNREFERENCED_PARAMETER(cancellationToken);

    co_return;
}

Awaitable<void> NoopStateProvider::RemoveStateAsync(
    __in CancellationToken const& cancellationToken)
{
    UNREFERENCED_PARAMETER(cancellationToken);

    co_return;
}

OperationDataStream::SPtr NoopStateProvider::GetCurrentState()
{
    return nullptr;
}

ktl::Awaitable<void> NoopStateProvider::BeginSettingCurrentStateAsync()
{
    co_return;
}

Awaitable<void> NoopStateProvider::SetCurrentStateAsync(
    __in LONG64 stateRecordNumber,
    __in OperationData const & copyOperationData,
    __in CancellationToken const & cancellationToken)
{
    UNREFERENCED_PARAMETER(stateRecordNumber);
    UNREFERENCED_PARAMETER(copyOperationData);
    UNREFERENCED_PARAMETER(cancellationToken);

    co_return;
}

Awaitable<void> NoopStateProvider::EndSettingCurrentStateAsync(
    __in CancellationToken const & cancellationToken)
{
    UNREFERENCED_PARAMETER(cancellationToken);

    Trace.WriteInfo(
        TraceComponent,
        "{0}:{1}:{2} Api: EndSettingCurrentStateAsync.",
        TracePartitionId,
        ReplicaId,
        StateProviderId);

    co_return;
}

Awaitable<void> NoopStateProvider::PrepareForRemoveAsync(
    __in TransactionBase const & transactionBase,
    __in CancellationToken const & cancellationToken)
{
    UNREFERENCED_PARAMETER(transactionBase);
    UNREFERENCED_PARAMETER(cancellationToken);

    co_return;
}

Awaitable<OperationContext::CSPtr> NoopStateProvider::ApplyAsync(
    __in FABRIC_SEQUENCE_NUMBER logicalSequenceNumber,
    __in TransactionBase const & transactionBase,
    __in ApplyContext::Enum applyContext,
    __in_opt OperationData const * const metadataPtr,
    __in_opt OperationData const * const dataPtr)
{
    co_return nullptr;
}

void NoopStateProvider::Unlock(__in OperationContext const & operationContext)
{
    UNREFERENCED_PARAMETER(operationContext);
}

NoopStateProvider::NoopStateProvider(
    __in FactoryArguments const & factoryArguments)
    : KObject()
    , KShared()
    , name_()
    , stateProviderId_(factoryArguments.StateProviderId)
    , initializationParameters_(factoryArguments.InitializationParameters)
    , PartitionedReplicaTraceComponent(*PartitionedReplicaId::Create(factoryArguments.PartitionId, factoryArguments.ReplicaId, GetThisAllocator()))
    , replicatorWRef_(nullptr)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    status = KUri::Create(factoryArguments.Name->Get(KUriView::eRaw), GetThisAllocator(), name_);
    SetConstructorStatus(status);
}

NoopStateProvider::~NoopStateProvider()
{
    // We cannot assert the lifeState here due to injected crashes.
}
