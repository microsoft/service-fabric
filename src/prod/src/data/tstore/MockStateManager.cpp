// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data::TStore;
using namespace Data::Utilities;
using namespace TStoreTests;


MockStateManager::MockStateManager(
    __in TxnReplicator::ITransactionalReplicator& replicator,
    __in Data::StateManager::IStateProvider2Factory& factory)
    : stateProviderSPtr_(nullptr),
    spFactorySPtr_(&factory),
    initializationParameters(nullptr),
    replicatorSPtr_(&replicator)
{
}

MockStateManager::~MockStateManager()
{
}


MockStateManager::SPtr MockStateManager::Create(
    __in KAllocator & allocator,
    __in TxnReplicator::ITransactionalReplicator & replicator,
    __in Data::StateManager::IStateProvider2Factory& factory)
{
    SPtr result = _new(MOCK_SP_TAG, allocator) MockStateManager(replicator, factory);
    THROW_ON_ALLOCATION_FAILURE(result);
    Diagnostics::Validate(result->Status());

    return result;
}

TxnReplicator::Transaction::SPtr MockStateManager::CreateTransaction()
{
    return TxnReplicator::Transaction::CreateTransaction(*replicatorSPtr_, GetThisAllocator());
}

ktl::Awaitable<void> MockStateManager::CommitRemoveAsync(__in TxnReplicator::Transaction& txn)
{
    co_await suspend_never();
    txn.Dispose();
}

ktl::Awaitable<void> MockStateManager::CommitAddAsync(__in TxnReplicator::Transaction& txn)
{
    KInvariant(stateProviderSPtr_ != nullptr);
    KInvariant(initializationParameters != nullptr);

    // Set up the work folder.
    KGuid fileGuid;
    fileGuid.CreateNew();
    KString::SPtr fileName = nullptr;
    NTSTATUS status = KString::Create(fileName, GetThisAllocator(), KStringView::MaxGuidString);
    KInvariant(NT_SUCCESS(status));
    BOOLEAN result = fileName->FromGUID(fileGuid);
    KInvariant(result == TRUE);

    const KStringView fName(*fileName);
    auto currentFolderPath_ = CreateFileString(fName);
    Common::ErrorCode errorCode = Common::Directory::Create2(currentFolderPath_->operator LPCWSTR());
    if (errorCode.IsSuccess() == false)
    {
        throw ktl::Exception(errorCode.ToHResult());
    }

    KWeakRef<ITransactionalReplicator>::SPtr tmpWRef;
    status = replicatorSPtr_->GetWeakIfRef(tmpWRef);
    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    stateProviderSPtr_->Initialize(*tmpWRef, *currentFolderPath_, nullptr);

    co_await stateProviderSPtr_->OpenAsync(CancellationToken::None);
    co_await stateProviderSPtr_->RecoverCheckpointAsync(CancellationToken::None);
    co_await stateProviderSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE::FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
    txn.Dispose();
}

ktl::Awaitable<void> MockStateManager::AddAsync(
    __in TxnReplicator::Transaction & transaction,
    __in KUriView const & stateProviderName,
    __in KStringView const & stateProviderType,
    __in_opt Data::Utilities::OperationData const * const initializationParam,
    __in Common::TimeSpan const & timeout,
    __in CancellationToken const & cancellationToken)
{
    UNREFERENCED_PARAMETER(transaction);
    UNREFERENCED_PARAMETER(timeout);
    UNREFERENCED_PARAMETER(cancellationToken);

    KGuid partitionId;
    partitionId.CreateNew();
    LONG64 replicaId = 0;
    FABRIC_STATE_PROVIDER_ID stateProviderId = 5;

    KUri::CSPtr copyStateProviderName;
    NTSTATUS status = KUri::Create(const_cast<KUriView &>(stateProviderName), GetThisAllocator(), (KUri::SPtr&)copyStateProviderName);
    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    KString::SPtr copystateProviderType = KString::Create(stateProviderType, GetThisAllocator());

    Data::StateManager::FactoryArguments argument(*copyStateProviderName, stateProviderId, *copystateProviderType, partitionId, replicaId, initializationParam);
    TxnReplicator::IStateProvider2::SPtr stateProvider = nullptr;
    status = spFactorySPtr_->Create(argument, stateProvider);
    Diagnostics::Validate(status);

    //save it for commit.
    stateProviderName_ = stateProviderName;
    initializationParameters = initializationParam;
    stateProviderSPtr_ = stateProvider.RawPtr();
    co_await suspend_never();
}

ktl::Awaitable<void> MockStateManager::RemoveAsync(
    __in Transaction & transaction,
    __in KUriView const & stateProviderName,
    __in Common::TimeSpan const & timeout,
    __in CancellationToken const & cancellationToken)
{
    UNREFERENCED_PARAMETER(timeout);
    UNREFERENCED_PARAMETER(stateProviderName);

    co_await stateProviderSPtr_->PrepareForRemoveAsync(transaction, cancellationToken);
    co_return;
}

ktl::Awaitable<void> TStoreTests::MockStateManager::CloseAsync()
{
    co_await stateProviderSPtr_->RemoveStateAsync(CancellationToken::None);
    co_await stateProviderSPtr_->CloseAsync(CancellationToken::None);
    stateProviderSPtr_ = nullptr;
}

KString::CSPtr MockStateManager::CreateFileString(__in KStringView const & name)
{
    KString::SPtr fileName;
    KAllocator& allocator = GetThisAllocator();

    WCHAR currentDirectoryPathCharArray[MAX_PATH];
    GetCurrentDirectory(MAX_PATH, currentDirectoryPathCharArray);

#if !defined(PLATFORM_UNIX)
    NTSTATUS status = KString::Create(fileName, allocator, L"\\??\\");
#else
    NTSTATUS status = KString::Create(fileName, allocator, L"");
#endif
    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    BOOLEAN concatSuccess = fileName->Concat(currentDirectoryPathCharArray);
    CODING_ERROR_ASSERT(concatSuccess == TRUE);

    concatSuccess = fileName->Concat(Common::Path::GetPathSeparatorWstr().c_str());
    CODING_ERROR_ASSERT(concatSuccess == TRUE);

    concatSuccess = fileName->Concat(name);
    CODING_ERROR_ASSERT(concatSuccess == TRUE);

    return fileName.RawPtr();
}
