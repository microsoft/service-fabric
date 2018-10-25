// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::Utilities;
using namespace TxnReplicator;
using namespace Data::StateManager;
using namespace Common;
using namespace FabricTest;
using namespace std;

#define TEST_STATE_PROVIDER_TAG 'PSTF'

#define WAIT_FOR_SIGNAL_RESET(a) \
    WaitForSignalReset(a); \

TestStateProvider2::SPtr TestStateProvider2::Create(
    __in std::wstring const & nodeId,
    __in std::wstring const & serviceName,
    __in TxnReplicator::TestCommon::TestStateProvider & innerStateProvider,
    __in KAllocator & allocator)
{
    TestStateProvider2 * pointer = _new(TEST_STATE_PROVIDER_TAG, allocator) TestStateProvider2(nodeId, serviceName, innerStateProvider);
    THROW_ON_ALLOCATION_FAILURE(pointer);

    return SPtr(pointer);
}

bool TestStateProvider2::ShouldFailOn(
    std::wstring operationName,
    ApiFaultHelper::FaultType faultType,
    bool isHighFrequencyApi) const
{
    if (isHighFrequencyApi)
    {
        return ApiFaultHelper::Get().ShouldFailOn(nodeId_, serviceName_, ApiFaultHelper::ComponentName::SP2HighFrequencyApi, operationName, faultType);
    }

    return ApiFaultHelper::Get().ShouldFailOn(nodeId_, serviceName_, ApiFaultHelper::ComponentName::SP2, operationName, faultType);
}

void TestStateProvider2::CheckForFailuresAndDelays(
    std::wstring const & operationName,
    bool isHighFrequencyApi) const
{
    if (ShouldFailOn(operationName, ApiFaultHelper::Delay, isHighFrequencyApi))               
    {         
        ::Sleep(static_cast<DWORD>(ApiFaultHelper::Get().GetApiDelayInterval().TotalMilliseconds()));
    }

    if (ShouldFailOn(operationName, ApiFaultHelper::Failure, isHighFrequencyApi))               
    {
        throw ktl::Exception(SF_STATUS_COMMUNICATION_ERROR);
    }
}

Awaitable<void> TestStateProvider2::CheckForFailuresAndDelaysAsync(
    std::wstring const & operationName,
    bool isHighFrequencyApi) const
{
    if (ShouldFailOn(operationName, ApiFaultHelper::Delay, isHighFrequencyApi))               
    {         
        NTSTATUS status = co_await KTimer::StartTimerAsync(GetThisAllocator(), TEST_STATE_PROVIDER_TAG, static_cast<ULONG>(ApiFaultHelper::Get().GetApiDelayInterval().TotalMilliseconds()), nullptr);
        TestSession::FailTestIfNot(NT_SUCCESS(status), "Failed to start ktl timer");
    }

    if (ShouldFailOn(operationName, ApiFaultHelper::Failure, isHighFrequencyApi))               
    {
        throw ktl::Exception(SF_STATUS_COMMUNICATION_ERROR);
    }

    co_return;
}

Awaitable<void> TestStateProvider2::GetAsync(
    __in Common::TimeSpan const & timeout,
    __out FABRIC_SEQUENCE_NUMBER & version,
    __out KString::SPtr & value)
{
    co_await innerStateProvider_->GetAsync(timeout, version, value);
    co_return;
}

Awaitable<bool> TestStateProvider2::TryUpdateAsync(
    __in Common::TimeSpan const & timeout,
    __in TxnReplicator::Transaction & transaction,
    __in KStringView const & newValue,
    __out FABRIC_SEQUENCE_NUMBER & currentVersionIfFailed,
    __in_opt FABRIC_SEQUENCE_NUMBER oldVersion)
{
    bool result = co_await innerStateProvider_->TryUpdateAsync(
        timeout,
        transaction,
        newValue,
        currentVersionIfFailed,
        oldVersion);

    co_return result;
}

Awaitable<bool> TestStateProvider2::TryRemoveMyself(
    __in Common::TimeSpan const & timeout,
    __in TxnReplicator::Transaction & transaction,
    __in ITransactionalReplicator & replicator,
    __out FABRIC_SEQUENCE_NUMBER & currentVersionIfFailed,
    __in_opt FABRIC_SEQUENCE_NUMBER oldVersion)
{ 
    bool result = co_await innerStateProvider_->TryRemoveMyself(
        timeout,
        transaction,
        replicator,
        currentVersionIfFailed,
        oldVersion);

    co_return result;
}

KUriView const TestStateProvider2::GetName() const
{
    WAIT_FOR_SIGNAL_RESET(StateProviderGetNameBlock);
    CheckForFailuresAndDelays(L"getname", false);
    return innerStateProvider_->GetName();
}

KArray<StateProviderInfo> TestStateProvider2::GetChildren(
    __in KUriView const & rootName)
{
    WAIT_FOR_SIGNAL_RESET(StateProviderGetChildrenBlock);
    CheckForFailuresAndDelays(L"getchildren", false);
    return innerStateProvider_->GetChildren(rootName);
}

void TestStateProvider2::Initialize(
    __in KWeakRef<ITransactionalReplicator> & transactionalReplicatorWRef,
    __in KStringView const& workFolder,
    __in KSharedArray<IStateProvider2::SPtr> const* const children)
{
    WAIT_FOR_SIGNAL_RESET(StateProviderInitializeBlock);
    CheckForFailuresAndDelays(L"initialize", false);

    innerStateProvider_->Initialize(
        transactionalReplicatorWRef,
        workFolder,
        children);
}

Awaitable<void> TestStateProvider2::OpenAsync(
    __in CancellationToken const& cancellationToken)
{
    WAIT_FOR_SIGNAL_RESET(StateProviderOpenAsyncBlock);
    co_await CheckForFailuresAndDelaysAsync(L"openasync", false);
    co_await innerStateProvider_->OpenAsync(cancellationToken);
    co_return;
}

Awaitable<void> TestStateProvider2::ChangeRoleAsync(
    __in FABRIC_REPLICA_ROLE newRole,
    __in CancellationToken const& cancellationToken)
{
    WAIT_FOR_SIGNAL_RESET(StateProviderChangeRoleAsyncBlock);
    co_await CheckForFailuresAndDelaysAsync(L"changeroleasync", false);
    co_await innerStateProvider_->ChangeRoleAsync(newRole, cancellationToken);
    co_return;
}

Awaitable<void> TestStateProvider2::CloseAsync(
    __in CancellationToken const& cancellationToken)
{
    WAIT_FOR_SIGNAL_RESET(StateProviderCloseAsyncBlock);
    co_await CheckForFailuresAndDelaysAsync(L"closeasync", false);
    co_await innerStateProvider_->CloseAsync(cancellationToken);
    co_return;
}

void TestStateProvider2::Abort() noexcept
{
    // Note: Abort cannot throw.
    innerStateProvider_->Abort();
}

void TestStateProvider2::PrepareCheckpoint(
    __in LONG64 checkpointLSN)
{
    WAIT_FOR_SIGNAL_RESET(StateProviderPrepareCheckpointBlock);
    CheckForFailuresAndDelays(L"preparecheckpoint", false);
    innerStateProvider_->PrepareCheckpoint(checkpointLSN);
}

Awaitable<void> TestStateProvider2::PerformCheckpointAsync(
    __in CancellationToken const& cancellationToken)
{
    WAIT_FOR_SIGNAL_RESET(StateProviderPerformCheckpointBlock);
    co_await CheckForFailuresAndDelaysAsync(L"performcheckpointasync", false);
    co_await innerStateProvider_->PerformCheckpointAsync(cancellationToken);
    co_return;
}

Awaitable<void> TestStateProvider2::CompleteCheckpointAsync(
    __in CancellationToken const& cancellationToken)
{
    WAIT_FOR_SIGNAL_RESET(StateProviderCompleteCheckpointBlock);
    co_await CheckForFailuresAndDelaysAsync(L"completecheckpointasync", false);
    co_await innerStateProvider_->CompleteCheckpointAsync(cancellationToken);
    co_return;
}

Awaitable<void> TestStateProvider2::RecoverCheckpointAsync(__in CancellationToken const & cancellationToken)
{
    WAIT_FOR_SIGNAL_RESET(StateProviderRecoverCheckpointAsyncBlock);
    co_await CheckForFailuresAndDelaysAsync(L"recovercheckpointasync", false);
    co_await innerStateProvider_->RecoverCheckpointAsync(cancellationToken);
    co_return;
}

Awaitable<void> TestStateProvider2::BackupCheckpointAsync(
    __in KString const & backupDirectory,
    __in CancellationToken const & cancellationToken)
{
    WAIT_FOR_SIGNAL_RESET(StateProviderBackupCheckpointAsyncBlock);
    co_await CheckForFailuresAndDelaysAsync(L"backupcheckpointasync", false);
    co_await innerStateProvider_->BackupCheckpointAsync(backupDirectory, cancellationToken);
    co_return;
}

Awaitable<void> TestStateProvider2::RestoreCheckpointAsync(
    __in KString const & backupDirectory,
    __in CancellationToken const & cancellationToken)
{
    WAIT_FOR_SIGNAL_RESET(StateProviderRestoreCheckpointAsyncBlock);
    co_await CheckForFailuresAndDelaysAsync(L"restorecheckpointAsync", false);
    co_await innerStateProvider_->RestoreCheckpointAsync(backupDirectory, cancellationToken);
    co_return;
}

Awaitable<void> TestStateProvider2::RemoveStateAsync(
    __in CancellationToken const& cancellationToken)
{
    WAIT_FOR_SIGNAL_RESET(StateProviderRemoveStateAsyncBlock);
    co_await CheckForFailuresAndDelaysAsync(L"removestateAsync", false);
    co_await innerStateProvider_->RemoveStateAsync(cancellationToken);
    co_return;
}

OperationDataStream::SPtr TestStateProvider2::GetCurrentState()
{
    WAIT_FOR_SIGNAL_RESET(StateProviderGetCurrentStateBlock);
    CheckForFailuresAndDelays(L"getcurrentstate", false);
    return innerStateProvider_->GetCurrentState();
}

Awaitable<void> TestStateProvider2::BeginSettingCurrentStateAsync()
{
    WAIT_FOR_SIGNAL_RESET(StateProviderBeginSettingCurrentStateBlock);
    co_await CheckForFailuresAndDelaysAsync(L"beginsettingcurrentstateasync", false);
    co_await innerStateProvider_->BeginSettingCurrentStateAsync();
}

Awaitable<void> TestStateProvider2::SetCurrentStateAsync(
    __in LONG64 stateRecordNumber,
    __in OperationData const & operationData,
    __in CancellationToken const & cancellationToken)
{
    WAIT_FOR_SIGNAL_RESET(StateProviderSetCurrentCurrentStateAsyncBlock);
    co_await CheckForFailuresAndDelaysAsync(L"setcurrentstateasync", false);
    co_await innerStateProvider_->SetCurrentStateAsync(
        stateRecordNumber,
        operationData,
        cancellationToken);

    co_return;
}

Awaitable<void> TestStateProvider2::EndSettingCurrentStateAsync(
    __in CancellationToken const & cancellationToken)
{
    WAIT_FOR_SIGNAL_RESET(StateProviderEndSettingCurrentStateAsyncBlock);
    co_await CheckForFailuresAndDelaysAsync(L"endsettingcurrentstateasync", false);
    co_await innerStateProvider_->EndSettingCurrentStateAsync(cancellationToken);
    co_return;
}

Awaitable<void> TestStateProvider2::PrepareForRemoveAsync(
    __in TransactionBase const& transactionBase,
    __in CancellationToken const& cancellationToken)
{
    WAIT_FOR_SIGNAL_RESET(StateProviderPrepareForRemoveAsyncBlock);
    co_await CheckForFailuresAndDelaysAsync(L"prepareforremoveasync", false);
    co_await innerStateProvider_->PrepareForRemoveAsync(transactionBase, cancellationToken);
    co_return;
}

Awaitable<TxnReplicator::OperationContext::CSPtr> TestStateProvider2::ApplyAsync(
    __in LONG64 logicalSequenceNumber,
    __in TransactionBase const & transactionBase,
    __in ApplyContext::Enum applyContext,
    __in_opt OperationData const * const metadataPtr,
    __in_opt OperationData const * const dataPtr)
{
    co_await CheckForFailuresAndDelaysAsync(L"applyasync", true);
    TxnReplicator::OperationContext::CSPtr result = co_await innerStateProvider_->ApplyAsync(
        logicalSequenceNumber,
        transactionBase,
        applyContext,
        metadataPtr,
        dataPtr);

    co_return result;
}

void TestStateProvider2::Unlock(__in TxnReplicator::OperationContext const & operationContext)
{
    CheckForFailuresAndDelays(L"unlock", true);
    innerStateProvider_->Unlock(operationContext);
}

TestStateProvider2::TestStateProvider2(
    __in std::wstring const & nodeId,
    __in std::wstring const & serviceName,
    __in TxnReplicator::TestCommon::TestStateProvider & innerStateProvider) noexcept
    : KObject()
    , KShared()
    , innerStateProvider_(&innerStateProvider)
    , nodeId_(nodeId)
    , serviceName_(serviceName)
{
    // Note: Constructor cannot throw.
}

TestStateProvider2::~TestStateProvider2() noexcept
{
    // Note: Destructor cannot throw.
}
