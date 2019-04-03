// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data;
using namespace Data::Utilities;
using namespace Data::StateManager;

NTSTATUS CheckpointManager::Create(
    __in Data::Utilities::PartitionedReplicaId const & traceType,
    __in ApiDispatcher & apiDispatcher,
    __in KStringView const & workFolderPath,
    __in KStringView const & replicaFolderPath,
    __in SerializationMode::Enum serializationMode,
    __in KAllocator & allocator,
    __out SPtr & result) noexcept
{
    result = _new(CHECKPOINT_MANAGER_TAG, allocator) CheckpointManager(traceType, apiDispatcher, workFolderPath, replicaFolderPath, serializationMode);

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

void CheckpointManager::PrepareCheckpoint(
    __in MetadataManager const & metadataManager, 
    __in FABRIC_SEQUENCE_NUMBER checkpointLSN)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    // Add active state providers to the list of state providers to checkpoint.
    KSharedArray<Metadata::CSPtr>::SPtr activeMetadataArraySPtr;
    status = metadataManager.GetInMemoryMetadataArray(
        StateProviderFilterMode::Active,
        activeMetadataArraySPtr);
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"PrepareCheckpoint: GetInMemoryMetadataArray.",
        Helper::CheckpointManager);

    // Clean up snapshot and reserve the size of active state providers.
    prepareCheckpointActiveMetadataSnapshot_.clear();
    prepareCheckpointActiveMetadataSnapshot_.reserve(static_cast<size_t>(activeMetadataArraySPtr->Count()));

    for (Metadata::CSPtr current : *activeMetadataArraySPtr)
    {
        Metadata::CSPtr copiedMetadata;
        status = Metadata::Create(
            *current->Name,
            *current->TypeString,
            *current->StateProvider,
            current->InitializationParameters.RawPtr(),
            current->StateProviderId,
            current->ParentId,
            current->CreateLSN,
            current->DeleteLSN,
            MetadataMode::Active,
            false,
            GetThisAllocator(),
            copiedMetadata);
        Helper::ThrowIfNecessary(
            status,
            TracePartitionId,
            ReplicaId,
            L"PrepareCheckpoint: Metadata::Create",
            Helper::CheckpointManager);

        auto result = prepareCheckpointActiveMetadataSnapshot_.insert(
            std::pair<FABRIC_STATE_PROVIDER_ID, Metadata::CSPtr>(
                copiedMetadata->StateProviderId, 
                copiedMetadata));
        ASSERT_IFNOT(
            result.second == true,
            "{0}: PrepareCheckpoint: Active StateProvider add to the PrepareCheckpointSnapshot failed, CheckpointLSN: {1}, SPId: {2}",
            TraceId,
            checkpointLSN,
            current->StateProviderId);
    }

    // Add soft deleted state providers to the list of state providers to checkpoint.
    // PrepareCheckpoint has to snap deleted state providers because copy uses this snap and copy needs to know
    // deletes stated providers to be able to handle duplicates that can be applied on copy.
    KSharedArray<Metadata::CSPtr>::SPtr deletedStateProviders;
    status = metadataManager.GetDeletedConstMetadataArray(deletedStateProviders);
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"PrepareCheckpoint: GetDeletedConstMetadataArray.",
        Helper::CheckpointManager);

    // Clean up snapshot and reserve the size of deleted state providers.
    prepareCheckpointDeletedMetadataSnapshot_.clear();
    prepareCheckpointDeletedMetadataSnapshot_.reserve(static_cast<size_t>(deletedStateProviders->Count()));

    for (Metadata::CSPtr current : *deletedStateProviders)
    {
        if (current->MetadataMode != MetadataMode::DelayDelete)
        {
            continue;
        }

        Metadata::CSPtr copiedMetadata;
        status = Metadata::Create(
            *current->Name,
            *current->TypeString,
            *current->StateProvider,
            current->InitializationParameters.RawPtr(),
            current->StateProviderId,
            current->ParentId,
            current->CreateLSN,
            current->DeleteLSN,
            current->MetadataMode,
            false,
            GetThisAllocator(),
            copiedMetadata);
        Helper::ThrowIfNecessary(
            status,
            TracePartitionId,
            ReplicaId,
            L"PrepareCheckpoint: Metadata::Create",
            Helper::CheckpointManager);

        auto result = prepareCheckpointDeletedMetadataSnapshot_.insert(
            std::pair<FABRIC_STATE_PROVIDER_ID, Metadata::CSPtr>(
                copiedMetadata->StateProviderId, 
                copiedMetadata));
        ASSERT_IFNOT(
            result.second == true,
            "{0}: PrepareCheckpoint: Deleted StateProvider add to the PrepareCheckpointSnapshot failed, CheckpointLSN: {1}, SPId: {2}",
            TraceId,
            checkpointLSN,
            current->StateProviderId);
    }

    prepareCheckpointLSN_ = checkpointLSN;
}

Awaitable<void> CheckpointManager::PerformCheckpointAsync(
    __in MetadataManager const & metadataManager,
    __in CancellationToken const & cancellationToken,
    __in bool hasPersistedState)
{
    KShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    KSharedArray<SerializableMetadata::CSPtr>::SPtr serializableMetadataCollection = _new(GetThisAllocationTag(), GetThisAllocator()) KSharedArray<SerializableMetadata::CSPtr>();;
    THROW_ON_ALLOCATION_FAILURE(serializableMetadataCollection);
    Helper::ThrowIfNecessary(
        serializableMetadataCollection->Status(),
        TracePartitionId,
        ReplicaId,
        L"PerformCheckpointAsync: Create serializableMetadataCollection KSharedArray.",
        Helper::CheckpointManager);

    checkpointStateLock_.AcquireExclusive();
    {
        KFinally([&] {checkpointStateLock_.ReleaseExclusive(); });

        copyOrCheckpointMetadataSnapshot_.clear();
        copyOrCheckpointMetadataSnapshot_.reserve(prepareCheckpointActiveMetadataSnapshot_.size() + prepareCheckpointDeletedMetadataSnapshot_.size());

        this->PopulateSnapshotAndMetadataCollection(*serializableMetadataCollection, MetadataMode::Active);
        this->PopulateSnapshotAndMetadataCollection(*serializableMetadataCollection, MetadataMode::DelayDelete);
    }

    if (hasPersistedState == false)
    {
        // No need to write to disk
        // TODO: RDBug 13398799: Trace PerformCheckpoint in volatile case
        co_return;
    }

    // In the checkpoint file write path, we need to pass in the PrepareCheckpointLSN and write in the file.
    KWString filePath = KWString(GetThisAllocator(), tempCheckpointFilePath_->ToUNICODE_STRING());
    CheckpointFile::SPtr checkpointFileSPtr = nullptr;
    status = CheckpointFile::Create(
        *(this->PartitionedReplicaIdentifier),
        filePath,
        GetThisAllocator(),
        checkpointFileSPtr);
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"PerformCheckpointAsync: Create CheckpointFile.",
        Helper::CheckpointManager);
    
    co_await checkpointFileSPtr->WriteAsync(
        *serializableMetadataCollection,
        serializationMode_,
        false,
        prepareCheckpointLSN_,
        cancellationToken);

    StateManagerEventSource::Events->CheckpointManagerPerformCheckpoint(
        TracePartitionId,
        ReplicaId,
        serializableMetadataCollection->Count(),
        checkpointFileSPtr->FileSize);

    co_return;
}

Awaitable<void> CheckpointManager::CompleteCheckpointAsync(
    __in MetadataManager const & metadataManager,
    __in CancellationToken const & cancellationToken)
{
    KShared$ApiEntry();

    StateManagerEventSource::Events->CheckpointManagerCompleteCheckpointStart(
        TracePartitionId,
        ReplicaId);

    if (Common::File::Exists(static_cast<LPCWSTR>(*tempCheckpointFilePath_)))
    {
        co_await CheckpointFile::SafeFileReplaceAsync(
            *PartitionedReplicaIdentifier,
            *currentCheckpointFilePath_,
            *tempCheckpointFilePath_,
            *backupCheckpointFilePath_,
            cancellationToken,
            GetThisAllocator());
    }
    else if (Common::File::Exists(static_cast<LPCWSTR>(*backupCheckpointFilePath_)))
    {
        bool currentCheckpointExists = Common::File::Exists(static_cast<LPCWSTR>(*currentCheckpointFilePath_));

        ASSERT_IFNOT(
            currentCheckpointExists, 
            "{0}: Checkpoint file: {1} does not exist during complete checkpoint",
            TraceId,
            static_cast<LPCWSTR>(*currentCheckpointFilePath_));

        Common::ErrorCode errorCode = Common::File::Delete2(static_cast<LPCWSTR>(*backupCheckpointFilePath_));
        if (!errorCode.IsSuccess())
        {
            Helper::ThrowIfNecessary(
                errorCode,
                TracePartitionId,
                ReplicaId,
                L"CompleteCheckpointAsync: Delete BackupCheckpointFile.",
                Helper::CheckpointManager);
            throw Exception(StatusConverter::Convert(errorCode.ToHResult()));
        }
    }
    else
    {
        StateManagerEventSource::Events->CheckpointManagerCompleteCheckpointNotRename(
            TracePartitionId,
            ReplicaId);
    }

    StateManagerEventSource::Events->CheckpointManagerCompleteCheckpointFinished(
        TracePartitionId,
        ReplicaId);

    co_return;
}

Awaitable<KSharedArray<Metadata::SPtr>::SPtr> CheckpointManager::RecoverCheckpointAsync(
    __in MetadataManager & metadataManager,
    __in CancellationToken const & cancellationToken)
{
    KShared$ApiEntry();

    ASSERT_IFNOT(metadataManager.IsEmpty() == true, "{0}: In memory state is empty during recover checkpoint", TraceId);

    if (!Common::File::Exists(static_cast<LPCWSTR>(*currentCheckpointFilePath_)))
    {
        // Write the first (empty) checkpoint file, adding FABRIC_AUTO_SEQUENCE_NUMBER(0) to the PrepareCheckpointLSN.
        // In this way, we can assert with PrepareCheckpointLSN, it must be larger than FABRIC_INVALID_SEQUENCE_NUMBER(-1).
        prepareCheckpointLSN_ = FABRIC_AUTO_SEQUENCE_NUMBER;
        co_await PerformCheckpointAsync(metadataManager, cancellationToken);
        co_await CompleteCheckpointAsync(metadataManager, cancellationToken);
        co_return nullptr;
    }

    StateManagerEventSource::Events->CheckpointManagerRecover(
        TracePartitionId,
        ReplicaId,
        Utilities::ToStringLiteral(*currentCheckpointFilePath_));

    KWString filePath = KWString(GetThisAllocator(), currentCheckpointFilePath_->ToUNICODE_STRING());

    // Although we pass in the prepareCheckpointLSN, the checkpoint file is making sure it will not be used
    // since it's a reading path.
    CheckpointFile::SPtr checkpointFileSPtr = nullptr;
    NTSTATUS status = CheckpointFile::Create(
        *(this->PartitionedReplicaIdentifier),
        filePath,
        GetThisAllocator(),
        checkpointFileSPtr);
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"RecoverCheckpointAsync: Create CheckpointFileSPtr.",
        Helper::CheckpointManager);
    
    co_await checkpointFileSPtr->ReadAsync(cancellationToken);
    prepareCheckpointLSN_ = checkpointFileSPtr->PropertiesSPtr->PrepareCheckpointLSN;

    KSharedArray<Metadata::SPtr>::SPtr recoveredMetadataList = _new(GetThisAllocationTag(), GetThisAllocator()) KSharedArray<Metadata::SPtr>();
    THROW_ON_ALLOCATION_FAILURE(recoveredMetadataList);
    Helper::ThrowIfNecessary(
        recoveredMetadataList->Status(),
        TracePartitionId,
        ReplicaId,
        L"RecoverCheckpointAsync: Create recoveredMetadataList KSharedArray.",
        Helper::CheckpointManager);

    ULONG count = 0;
    ULONG activeCount = 0;
    ULONG deleteCount = 0;

    // Save the exception, close the enumerator and then throw the exception 
    Utilities::SharedException::CSPtr exceptionSPtr = nullptr;

    CheckpointFileAsyncEnumerator::SPtr enumerator = checkpointFileSPtr->GetAsyncEnumerator();

    while (true)
    {
        SerializableMetadata::CSPtr serializableMetadata = nullptr;
        status = co_await enumerator->GetNextAsync(cancellationToken, serializableMetadata);
        if (status == STATUS_NOT_FOUND)
        {
            // No more item in the Enumerator.
            break;
        }
        
        if (NT_SUCCESS(status) == false)
        {
            co_await enumerator->CloseAsync();

            Helper::ThrowIfNecessary(
                status,
                TracePartitionId,
                ReplicaId,
                L"RecoverCheckpointAsync: CheckpointFileAsyncEnumerator GetNextAsync failed.",
                Helper::CheckpointManager);
        }

        ++count;
        TxnReplicator::IStateProvider2::SPtr stateProvider;
        status = apiDispatcherSPtr_->CreateStateProvider(
            *serializableMetadata->Name,
            serializableMetadata->StateProviderId,
            *serializableMetadata->TypeString,
            serializableMetadata->InitializationParameters.RawPtr(),
            stateProvider);
        if (NT_SUCCESS(status) == false)
        {
            co_await enumerator->CloseAsync();

            Helper::ThrowIfNecessary(
                status,
                TracePartitionId,
                ReplicaId,
                L"RecoverCheckpointAsync: CreateStateProvider throw exception.",
                Helper::CheckpointManager);
        }

        Metadata::SPtr metadataSPtr = nullptr;
        status = Metadata::Create(
            *serializableMetadata->Name,
            *serializableMetadata->TypeString,
            *stateProvider,
            serializableMetadata->InitializationParameters.RawPtr(),
            serializableMetadata->StateProviderId,
            serializableMetadata->ParentStateProviderId,
            serializableMetadata->CreateLSN,
            serializableMetadata->DeleteLSN,
            serializableMetadata->MetadataMode,
            false,
            GetThisAllocator(),
            metadataSPtr);
        co_await CloseEnumeratorAndThrowIfOnFailureAsync(status, *enumerator);

        status = recoveredMetadataList->Append(metadataSPtr);
        co_await CloseEnumeratorAndThrowIfOnFailureAsync(status, *enumerator);

        if (metadataSPtr->MetadataMode == MetadataMode::Active)
        {
            co_await metadataManager.AcquireLockAndAddAsync(
                *metadataSPtr->Name,
                *metadataSPtr,
                Common::TimeSpan::MaxValue, 
                cancellationToken);

            ++activeCount;
        }
        else
        {
            if (metadataSPtr->MetadataMode != MetadataMode::DelayDelete && metadataSPtr->MetadataMode != MetadataMode::FalseProgress)
            {
                co_await enumerator->CloseAsync();
                Helper::ThrowIfNecessary(
                    SF_STATUS_INVALID_OPERATION,
                    TracePartitionId,
                    ReplicaId,
                    L"RecoverCheckpointAsync: Invalid metadata mode.",
                    Helper::CheckpointManager);

                throw Exception(SF_STATUS_INVALID_OPERATION); // "StateManager.LocalStateRecoverCheckpointAsync. Invalid metadata mode {0}"
            }

            bool isAdded = metadataManager.AddDeleted(metadataSPtr->StateProviderId, *metadataSPtr);
            ASSERT_IFNOT(isAdded, "{0}: Failed to add state provider: {1} to the delete list", TraceId, metadataSPtr->StateProviderId);

            ++deleteCount;
        }
    }

    co_await enumerator->CloseAsync();

    StateManagerEventSource::Events->CheckpointManagerRecoverSummary(
        TracePartitionId,
        ReplicaId,
        count,
        activeCount,
        deleteCount);

    checkpointStateLock_.AcquireExclusive();
    {
        KFinally([&] {checkpointStateLock_.ReleaseExclusive(); });

        KSharedArray<Metadata::CSPtr>::SPtr activeMetadataArrays = nullptr;
        status = metadataManager.GetInMemoryMetadataArray(
            StateProviderFilterMode::FilterMode::Active,
            activeMetadataArrays);
        Helper::ThrowIfNecessary(
            status,
            TracePartitionId,
            ReplicaId,
            L"RecoverCheckpointAsync: GetInMemoryMetadataArray.",
            Helper::CheckpointManager);

        for (Metadata::CSPtr current : *activeMetadataArrays)
        {
            Metadata::CSPtr copiedMetadata;
            status = Metadata::Create(
                *current->Name,
                *current->TypeString,
                *current->StateProvider,
                current->InitializationParameters.RawPtr(),
                current->StateProviderId,
                current->ParentId,
                current->CreateLSN,
                current->DeleteLSN,
                current->MetadataMode,
                false,
                GetThisAllocator(),
                copiedMetadata);
            Helper::ThrowIfNecessary(
                status,
                TracePartitionId,
                ReplicaId,
                L"RecoverCheckpointAsync: Create active Metadata.",
                Helper::CheckpointManager);

            auto result = copyOrCheckpointMetadataSnapshot_.insert(
                std::pair<FABRIC_STATE_PROVIDER_ID, Metadata::CSPtr>(
                    copiedMetadata->StateProviderId, 
                    copiedMetadata));
            ASSERT_IFNOT(
                result.second == true,
                "{0}: RecoverCheckpointAsync: Add Active StateProvider to the CopyOrCheckpointSnapshot failed, SPId: {1}",
                TraceId,
                current->StateProviderId);
        }

        KSharedArray<Metadata::CSPtr>::SPtr softDeletedStateProviders = nullptr;
        status = metadataManager.GetDeletedConstMetadataArray(softDeletedStateProviders);
        Helper::ThrowIfNecessary(
            status,
            TracePartitionId,
            ReplicaId,
            L"RecoverCheckpointAsync: GetDeletedConstMetadataArray.",
            Helper::CheckpointManager);

        for (Metadata::CSPtr current : *softDeletedStateProviders)
        {
            ASSERT_IFNOT(current->MetadataMode != MetadataMode::Active, "{0}: Metadata mode: {1} is not Active", TraceId, current->MetadataMode);

            Metadata::CSPtr copiedMetadata;
            status = Metadata::Create(
                *current->Name,
                *current->TypeString,
                *current->StateProvider,
                current->InitializationParameters.RawPtr(),
                current->StateProviderId,
                current->ParentId,
                current->CreateLSN,
                current->DeleteLSN,
                current->MetadataMode,
                false,
                GetThisAllocator(),
                copiedMetadata);
            Helper::ThrowIfNecessary(
                status,
                TracePartitionId,
                ReplicaId,
                L"RecoverCheckpointAsync: Create softDeleted Metadata.",
                Helper::CheckpointManager);

            auto result = copyOrCheckpointMetadataSnapshot_.insert(
                std::pair<FABRIC_STATE_PROVIDER_ID, Metadata::CSPtr>(
                    copiedMetadata->StateProviderId, 
                    copiedMetadata));
            ASSERT_IFNOT(
                result.second == true,
                "{0}: RecoverCheckpointAsync: Add Deleted StateProvider to the CopyOrCheckpointSnapshot failed, SPId: {1}",
                TraceId,
                current->StateProviderId);
        }
    }

    co_return recoveredMetadataList;
}

TxnReplicator::OperationDataStream::SPtr CheckpointManager::GetCurrentState(
    __in FABRIC_REPLICA_ID targetReplicaId)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    Common::Stopwatch stopwatch;

    stopwatch.Start();
    checkpointStateLock_.AcquireShared();
    KFinally([&] {checkpointStateLock_.ReleaseShared(); });

    NamedOperationDataStream::SPtr namedOperationDataStream;
    status = NamedOperationDataStream::Create(
        *PartitionedReplicaIdentifier, 
        serializationMode_,
        GetThisAllocator(), 
        namedOperationDataStream);
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"GetCurrentState: Create NamedOperationDataStream.",
        Helper::CheckpointManager);

    int64 time = stopwatch.ElapsedMilliseconds;
    KSharedArray<Metadata::CSPtr>::SPtr sortedMetadataArraySPtr = GetOrderedMetadataArray();
    time = stopwatch.ElapsedMilliseconds - time;
    
    KArray<SerializableMetadata::CSPtr> checkpointMetadataArray = KArray<SerializableMetadata::CSPtr>(GetThisAllocator(), sortedMetadataArraySPtr->Count());

    for (Metadata::CSPtr metadata : *sortedMetadataArraySPtr)
    {
        SerializableMetadata::CSPtr serializableMetadata;
        status = SerializableMetadata::Create(*metadata, GetThisAllocator(), serializableMetadata);
        Helper::ThrowIfNecessary(
            status,
            TracePartitionId,
            ReplicaId,
            L"GetCurrentState: Create SerializableMetadata.",
            Helper::CheckpointManager);

        // Insert in descending sorted order for hierarchy.
        status = checkpointMetadataArray.Append(serializableMetadata);
        Helper::ThrowIfNecessary(
            status,
            TracePartitionId,
            ReplicaId,
            L"GetCurrentState: Append SerializableMetadata to checkpointMetadataArray.",
            Helper::CheckpointManager);
    }

    // #9175765: Ktl should support creating with KUriView const
    KUri::CSPtr stateManagerName;
    status = KUri::Create(StateManager::StateManagerName.Get(KUriView::eRaw), GetThisAllocator(), stateManagerName);
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"GetCurrentState: Create stateManagerName.",
        Helper::CheckpointManager);

    StateProviderIdentifier stateManagerIdentifier(*stateManagerName, StateManager::StateManagerId);

    CopyOperationDataStream::SPtr copyOperationDataStream;
    status = CopyOperationDataStream::Create(*PartitionedReplicaIdentifier, checkpointMetadataArray, targetReplicaId, serializationMode_, GetThisAllocator(),copyOperationDataStream);
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"GetCurrentState: Create CopyOperationDataStream.",
        Helper::CheckpointManager);
    
    namedOperationDataStream->Add(stateManagerIdentifier, *copyOperationDataStream);

    for (Metadata::CSPtr metadata : *sortedMetadataArraySPtr)
    {
        if (metadata->MetadataMode != MetadataMode::Active)
        {
            continue;
        }

        // A state provider can have a empty state, in which case, the operationDataStream will be nullptr
        TxnReplicator::OperationDataStream::SPtr operationDataStream = metadata->StateProvider->GetCurrentState();

        // If the state provider operationDataStream is nullptr, just skip this state provider
        if (operationDataStream == nullptr)
        {
            StateManagerEventSource::Events->CheckpointManagerNullState(
                TracePartitionId,
                ReplicaId,
                metadata->StateProviderId);
            continue;
        }

        StateProviderIdentifier stateProviderIdentifier(*metadata->Name, metadata->StateProviderId);
        namedOperationDataStream->Add(stateProviderIdentifier, *operationDataStream);

        StateManagerEventSource::Events->CheckpointManagerNotNullState(
            TracePartitionId,
            ReplicaId,
            metadata->StateProviderId);
    }

    stopwatch.Stop();

    StateManagerEventSource::Events->CheckpointManagerGetStateSummary(
        TracePartitionId,
        ReplicaId,
        stopwatch.ElapsedMilliseconds,
        time);

    return namedOperationDataStream.RawPtr();
}

void CheckpointManager::RemoveStateProvider(
    __in FABRIC_STATE_PROVIDER_ID stateProviderId)
{
    checkpointStateLock_.AcquireExclusive();
    {
        KFinally([&] {checkpointStateLock_.ReleaseExclusive(); });

        // Both item exist and non-exist case, the undered_map erase will successfully return,
        // so we don't need to check status and return as before.
        size_t count = copyOrCheckpointMetadataSnapshot_.erase(stateProviderId);
        ASSERT_IFNOT(count == 0 || count == 1, 
            "{0}:RemoveStateProvider: There should be only one state provider with the specific key, SPId: {1}",
            TraceId,
            stateProviderId);
    }
}

// Backup the state manager's checkpoint
// Algorithms:
//   1.Delete the file if exists
//   2.Get the metadatas in CopyOrCheckpointMetadataSnapshot
//   3.Write the checkpoint file
ktl::Awaitable<void> CheckpointManager::BackupCheckpointAsync(
    __in KString const & backupDirectory,
    __in CancellationToken const & cancellationToken)
{
    KShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;
    KString::SPtr backupFilePath = KPath::Combine(backupDirectory, L"backup.chkpt", GetThisAllocator());

    // Assert if the backup file already exists, because backup manager ensure the folder is empty.
    bool isExist = Common::File::Exists(static_cast<LPCWSTR>(*backupFilePath));
    ASSERT_IFNOT(isExist == false, "{0}: The backup folder should be empty.", TraceId);

    KSharedArray<SerializableMetadata::CSPtr>::SPtr serializableMetadataCollection = _new(GetThisAllocationTag(), GetThisAllocator()) KSharedArray<SerializableMetadata::CSPtr>();
    THROW_ON_ALLOCATION_FAILURE(serializableMetadataCollection);
    Helper::ThrowIfNecessary(
        serializableMetadataCollection->Status(),
        TracePartitionId,
        ReplicaId,
        L"BackupCheckpointAsync: Create serializableMetadataCollection.",
        Helper::CheckpointManager);

    checkpointStateLock_.AcquireShared();
    {
        KFinally([&] {checkpointStateLock_.ReleaseShared(); });

        // CopyOrCheckpointMetadataSnapshot gets updated only during perform checkpoint and replicator guarantees that perform 
        // and backup cannot happen at the same time, so it is safe to use it here.
        for (auto & row : dynamic_cast<const std::unordered_map<FABRIC_STATE_PROVIDER_ID,Metadata::CSPtr> &>(copyOrCheckpointMetadataSnapshot_))
        {
            Metadata::CSPtr metadataCSPtr = row.second;
            ASSERT_IFNOT(metadataCSPtr != nullptr, "{0}: Null metadata on apply", TraceId);
            SerializableMetadata::CSPtr serializableMetadataCSPtr = nullptr;
            status = SerializableMetadata::Create(*metadataCSPtr, GetThisAllocator(), serializableMetadataCSPtr);
            Helper::ThrowIfNecessary(
                status,
                TracePartitionId,
                ReplicaId,
                L"BackupCheckpointAsync: Create SerializableMetadata.",
                Helper::CheckpointManager);

            status = serializableMetadataCollection->Append(serializableMetadataCSPtr);
            Helper::ThrowIfNecessary(
                status,
                TracePartitionId,
                ReplicaId,
                L"BackupCheckpointAsync: Append SerializableMetadata to serializableMetadataCollection.",
                Helper::CheckpointManager);
        }
    }

    KWString backupCheckpointFileName(GetThisAllocator(), *backupFilePath);
    CheckpointFile::SPtr checkpointFileSPtr = nullptr;

    status= CheckpointFile::Create(
        *(this->PartitionedReplicaIdentifier), 
        backupCheckpointFileName, 
        GetThisAllocator(),
        checkpointFileSPtr);
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"BackupCheckpointAsync: CheckpointFile::Create.",
        Helper::CheckpointManager);

    // #12249219: Without the "allowPrepareCheckpointLSNToBeInvalid" being set to true in the backup code path, 
    // It is possible for all backups before the first checkpoint after the upgrade to fail.
    co_await checkpointFileSPtr->WriteAsync(
        *serializableMetadataCollection, 
        serializationMode_,
        true,
        prepareCheckpointLSN_,
        cancellationToken);

    co_return;
}

// Restore process
// Algorithms:
//   1.Validate the backup file is not corrupt.
//   2.Replace the file
ktl::Awaitable<void> CheckpointManager::RestoreCheckpointAsync(
    __in KString const & backupDirectory,
    __in CancellationToken const & cancellationToken)
{
    KShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    StateManagerEventSource::Events->CheckpointManagerRestoreCheckpointStart(
        TracePartitionId,
        ReplicaId);

    // Validate the backup.
    KString::SPtr backupFilePath = KPath::Combine(backupDirectory, L"backup.chkpt", GetThisAllocator());

    if (!Common::File::Exists(static_cast<LPCWSTR>(*backupFilePath)))
    {
        Helper::ThrowIfNecessary(
            STATUS_INTERNAL_DB_CORRUPTION,
            TracePartitionId,
            ReplicaId,
            L"RestoreCheckpointAsync: BackupFile does not exist.",
            Helper::CheckpointManager);
    }

    KWString backupCheckpointFileName(GetThisAllocator(), *backupFilePath);
    Helper::ThrowIfNecessary(
        backupCheckpointFileName.Status(),
        TracePartitionId,
        ReplicaId,
        L"RestoreCheckpointAsync: Allocation of backup file name failed.",
        Helper::CheckpointManager);

    // Although we pass in the prepareCheckpointLSN, the checkpoint file is making sure it will not be used
    // since it's a reading path.
    CheckpointFile::SPtr checkpointFileSPtr = nullptr;
    status = CheckpointFile::Create(
        *(this->PartitionedReplicaIdentifier),
        backupCheckpointFileName,
        GetThisAllocator(),
        checkpointFileSPtr);
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"RestoreCheckpointAsync: CheckpointFile::Create",
        Helper::CheckpointManager);

    try
    {
        // Validate the backup file is not corrupt.
        co_await checkpointFileSPtr->ReadAsync(cancellationToken);
    }
    catch (Exception & exception)
    {
        StateManagerEventSource::Events->CheckpointManagerError(
            TracePartitionId,
            ReplicaId,
            L"RestoreCheckpointAsync: CheckpointFile::ReadAsync",
            exception.GetStatus());
        throw;
    }

    // Set PrepareCheckpointLSN to the value reading from the file.
    prepareCheckpointLSN_ = checkpointFileSPtr->PropertiesSPtr->PrepareCheckpointLSN;

    // Consistency checks.
    ASSERT_IFNOT(Common::File::Exists(static_cast<LPCWSTR>(*currentCheckpointFilePath_)) == false, "{0}: RestoreCheckpointAsync: Checkpoint file must not exist.", TraceId);
    ASSERT_IFNOT(Common::File::Exists(static_cast<LPCWSTR>(*tempCheckpointFilePath_)) == false, "{0}: RestoreCheckpointAsync: Temporary checkpoint file must not exist.", TraceId);

    // Now that it's safely copied, we can atomically move it to be the current checkpoint.
    // Note: Set Common::File::Move throwIfFail to false to invoke returning error code, otherwise it will throw system_error.
    Common::ErrorCode errorCode = Common::File::Move(
        static_cast<LPCWSTR>(*backupFilePath),
        static_cast<LPCWSTR>(*currentCheckpointFilePath_),
        false);

    if (errorCode.IsSuccess() == false)
    {
        Helper::ThrowIfNecessary(
            errorCode,
            TracePartitionId,
            ReplicaId,
            L"RestoreCheckpointAsync: Move backup to current.",
            Helper::CheckpointManager);
        throw Exception(Utilities::StatusConverter::Convert(errorCode.ToHResult()));
    }

    StateManagerEventSource::Events->CheckpointManagerRestoreCheckpointEnd(
        TracePartitionId,
        ReplicaId);

    co_return;
}

ktl::Awaitable<void> CheckpointManager::BackupActiveStateProviders(
    __in KString const & backupDirectory,
    __in CancellationToken const & cancellationToken)
{
    KShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    // Run all backup operations in parallel
    KArray<Awaitable<void>> tasks(GetThisAllocator());
    Helper::ThrowIfNecessary(
        tasks.Status(),
        TracePartitionId,
        ReplicaId,
        L"BackupActiveStateProviders: Create KArray.",
        Helper::CheckpointManager);

    checkpointStateLock_.AcquireShared();
    {
        KFinally([&] {checkpointStateLock_.ReleaseShared(); });

        // CopyOrCheckpointMetadataSnapshot gets updated only during perform checkpoint and replicator guarantees that perform and backup cannot happen at the same time,
        // so it is safe to use it here.
        // Backup all active state providers checkpoints
        for (auto & row : dynamic_cast<const std::unordered_map<FABRIC_STATE_PROVIDER_ID,Metadata::CSPtr> &>(copyOrCheckpointMetadataSnapshot_))
        {
            Metadata::CSPtr metadataCSPtr = row.second;
            if (metadataCSPtr->MetadataMode == MetadataMode::Active)
            {
                KString::SPtr stateProviderIdStringSPtr = Helper::StateProviderIDToKStringSPtr(
                    *PartitionedReplicaIdentifier,
                    metadataCSPtr->StateProviderId,
                    Helper::CheckpointManager,
                    GetThisAllocator());
                KString::SPtr folderName = KPath::Combine(backupDirectory, *stateProviderIdStringSPtr, GetThisAllocator());

                status = Helper::CreateFolder(*folderName);
                Helper::ThrowIfNecessary(
                    status,
                    TracePartitionId,
                    ReplicaId,
                    L"BackupActiveStateProviders: CreateFolder.",
                    Helper::CheckpointManager);

                status = tasks.Append(metadataCSPtr->StateProvider->BackupCheckpointAsync(*folderName, cancellationToken));
                Helper::ThrowIfNecessary(
                    status,
                    TracePartitionId,
                    ReplicaId,
                    L"BackupActiveStateProviders: Append BackupCheckpointAsync to task.",
                    Helper::CheckpointManager);
            }
        }
    }

    // Wait for all operations to drain.
    try
    {
        co_await TaskUtilities<void>::WhenAll(tasks);
    }
    catch (Exception & exception)
    {
        StateManagerEventSource::Events->CheckpointManagerError(
            TracePartitionId,
            ReplicaId,
            L"BackupActiveStateProviders threw exception during wait on task to complete",
            exception.GetStatus());

        throw;
    }

    co_return;
}

KArray<Metadata::CSPtr> CheckpointManager::GetPreparedActiveOrDeletedList(
    __in MetadataMode::Enum filterMode)
{
    auto & snapshot = filterMode == MetadataMode::Active ?
        dynamic_cast<const std::unordered_map<FABRIC_STATE_PROVIDER_ID, Metadata::CSPtr> &>(prepareCheckpointActiveMetadataSnapshot_) :
        dynamic_cast<const std::unordered_map<FABRIC_STATE_PROVIDER_ID, Metadata::CSPtr> &>(prepareCheckpointDeletedMetadataSnapshot_);

    KArray<Metadata::CSPtr> stateProviders(GetThisAllocator(), static_cast<ULONG>(snapshot.size()));
    Helper::ThrowIfNecessary(
        stateProviders.Status(),
        TracePartitionId,
        ReplicaId,
        L"GetPreparedActiveOrDeletedList: Create KArray.",
        Helper::CheckpointManager);

    for (auto & row : snapshot)
    {
        NTSTATUS status = stateProviders.Append(row.second);
        if (!NT_SUCCESS(status)) 
        { 
            TRACE_ERRORS(
                status,
                TracePartitionId,
                ReplicaId,
                Helper::CheckpointManager,
                "GetPreparedActiveOrDeletedList: Append metadata to array failed. SPId: {0}, FilterMode: {1}.",
                row.second->StateProviderId,
                static_cast<int>(filterMode));
            throw ktl::Exception(status); 
        } 
    }

    return stateProviders;
}

KArray<Metadata::CSPtr> CheckpointManager::GetCopyOrCheckpointActiveList()
{
    KArray<Metadata::CSPtr> activeStateProviders(GetThisAllocator());
    Helper::ThrowIfNecessary(
        activeStateProviders.Status(),
        TracePartitionId,
        ReplicaId,
        L"GetCopyOrCheckpointActiveList: Create KArray.",
        Helper::CheckpointManager);

    checkpointStateLock_.AcquireShared();
    {
        KFinally([&] {checkpointStateLock_.ReleaseShared(); });

        for (auto & row : dynamic_cast<const std::unordered_map<FABRIC_STATE_PROVIDER_ID,Metadata::CSPtr> &>(copyOrCheckpointMetadataSnapshot_))
        {
            Metadata::CSPtr metadataCSPtr = row.second;
            if (metadataCSPtr->MetadataMode == MetadataMode::Active)
            {
                NTSTATUS status = activeStateProviders.Append(metadataCSPtr);
                if (!NT_SUCCESS(status))
                {
                    TRACE_ERRORS(
                        status,
                        TracePartitionId,
                        ReplicaId,
                        Helper::CheckpointManager,
                        "GetCopyOrCheckpointActiveList: Append metadata to array failed. SPId: {0}.",
                        metadataCSPtr->StateProviderId);
                    throw ktl::Exception(status);
                }
            }
        }
    }

    return activeStateProviders;
}

// Used to clean up StateManager.cpt and StateManager.tmp files, when the replica changes role to NONE
// after cleaning the files, the work directory will be deleted too
void CheckpointManager::Clean()
{
    Common::ErrorCode errorCode = Common::File::Delete2(static_cast<LPCWSTR>(*tempCheckpointFilePath_), true);
    if (!errorCode.IsSuccess() && !errorCode.IsErrno(ERROR_FILE_NOT_FOUND) && !errorCode.IsErrno(ERROR_PATH_NOT_FOUND))
    {
        Helper::ThrowIfNecessary(
            errorCode,
            TracePartitionId,
            ReplicaId,
            L"Clean: Delete temp file.",
            Helper::CheckpointManager);
        throw Exception(Utilities::StatusConverter::Convert(errorCode.ToHResult()));
    }

    errorCode = Common::File::Delete2(static_cast<LPCWSTR>(*currentCheckpointFilePath_), true);
    if (!errorCode.IsSuccess() && !errorCode.IsErrno(ERROR_FILE_NOT_FOUND) && !errorCode.IsErrno(ERROR_PATH_NOT_FOUND))
    {
        Helper::ThrowIfNecessary(
            errorCode,
            TracePartitionId,
            ReplicaId,
            L"Clean: Delete current file.",
            Helper::CheckpointManager);
        throw Exception(Utilities::StatusConverter::Convert(errorCode.ToHResult()));
    }

    // Port Note: Difference with managed.
    // "copyOrCheckpointMetadataSnapshot_" gets cleaned when the MetadataManager is replaced with a new one.
    // In native, since this data structure is in Checkpoint Manager as oppose to MetadataManager, we need to clear it here.
    checkpointStateLock_.AcquireExclusive();
    {
        KFinally([&] {checkpointStateLock_.ReleaseExclusive(); });
        copyOrCheckpointMetadataSnapshot_.clear();
    }
}

KString::CSPtr CheckpointManager::GetCheckpointFilePath(__in KStringView const & fileName)
{
    KString::SPtr filePath = KPath::Combine(*workFolder_, fileName, GetThisAllocator());
    return filePath.RawPtr();
}

KSharedArray<Metadata::CSPtr>::SPtr CheckpointManager::GetOrderedMetadataArray()
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    KSharedArray<Metadata::CSPtr>::SPtr metadataArray = _new(GetThisAllocationTag(), GetThisAllocator()) KSharedArray<Metadata::CSPtr>();
    THROW_ON_ALLOCATION_FAILURE(metadataArray);
    Helper::ThrowIfNecessary(
        metadataArray->Status(),
        TracePartitionId,
        ReplicaId,
        L"GetOrderedMetadataArray: Create metadataArray.",
        Helper::CheckpointManager);

    // GetCurrentState already got the shared lock for copyOrCheckpointMetadataSnapshot, so don't
    // need to acquire the lock again.
    for (auto & row : dynamic_cast<const std::unordered_map<FABRIC_STATE_PROVIDER_ID,Metadata::CSPtr> &>(copyOrCheckpointMetadataSnapshot_))
    {
        Metadata::CSPtr metadataCSPtr = row.second;

        status = metadataArray->Append(metadataCSPtr);
        Helper::ThrowIfNecessary(
            status,
            TracePartitionId,
            ReplicaId,
            L"GetOrderedMetadataArray: Append metadata to copyOrCheckpointMetadataSnapshot.",
            Helper::CheckpointManager);
    }

    // Used QuickSort instead of inserting metadatas in order, time costs 92499 ms to 3397 ms when 
    // adding and sorting 50,000 items, which improves the performance by 96% (~27 times faster)
    Utilities::Sort<Metadata::CSPtr>::QuickSort(false, Comparer::Compare, metadataArray);
    return Ktl::Move(metadataArray);
}

ktl::Awaitable<void> CheckpointManager::CloseEnumeratorAndThrowIfOnFailureAsync(
    __in NTSTATUS status,
    __in CheckpointFileAsyncEnumerator & enumerator)
{
    KShared$ApiEntry();

    if (!NT_SUCCESS(status))
    {
        co_await enumerator.CloseAsync();
        Helper::ThrowIfNecessary(
            status,
            TracePartitionId,
            ReplicaId,
            L"CloseEnumeratorAndThrowIfOnFailureAsync: Close enumerator then throw.",
            Helper::CheckpointManager);
    }

    co_return;
}

// Summary: Moves files in managed path into native path.
NTSTATUS CheckpointManager::MoveManagedFileIfNecessary(
    __in KStringView const & workFolderPath) noexcept
{
    wstring workFolder(workFolderPath);

    wstring searchString;
    searchString.reserve(55);
    Common::StringWriter writer(searchString);
    writer.Write(TracePartitionId.ToString('N'));
    writer.Write(L"_");
    writer.Write(ReplicaId);
    writer.Write(L"_*_*");

    Common::StringCollection files = Common::Directory::Find(workFolder, searchString, 3, false);
    for (wstring file : files)
    {
        wstring endOfFile = file.substr(file.length() - ManagedCurrentCheckpointSuffix.length(), file.length());

        if (endOfFile == ManagedCurrentCheckpointSuffix)
        {
            Common::ErrorCode error = Common::File::Move(file, static_cast<LPCWSTR>(*currentCheckpointFilePath_), false);
            if (error.IsSuccess() == false)
            {
                return StatusConverter::Convert(error.ToHResult());
            }
        }
        else if (endOfFile == ManagedTmpCheckpointSuffix)
        {
            Common::ErrorCode error = Common::File::Move(file, static_cast<LPCWSTR>(*tempCheckpointFilePath_), false);
            if (error.IsSuccess() == false)
            {
                return StatusConverter::Convert(error.ToHResult());
            }
        }
        else if (endOfFile == ManagedBackupCheckpointSuffix)
        {
            Common::ErrorCode error = Common::File::Move(file, static_cast<LPCWSTR>(*backupCheckpointFilePath_), false);
            if (error.IsSuccess() == false)
            {
                return StatusConverter::Convert(error.ToHResult());
            }
        }
    }

    return STATUS_SUCCESS;
}

void CheckpointManager::PopulateSnapshotAndMetadataCollection(
    __in KSharedArray<SerializableMetadata::CSPtr> & metadataArray,
    __in MetadataMode::Enum filterMode)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    auto & prepareCheckpointSnapshot = (filterMode == MetadataMode::Active ?
        dynamic_cast<const std::unordered_map<FABRIC_STATE_PROVIDER_ID, Metadata::CSPtr> &>(prepareCheckpointActiveMetadataSnapshot_) :
        dynamic_cast<const std::unordered_map<FABRIC_STATE_PROVIDER_ID, Metadata::CSPtr> &>(prepareCheckpointDeletedMetadataSnapshot_));

    for (auto & row : prepareCheckpointSnapshot)
    {
        auto result = copyOrCheckpointMetadataSnapshot_.insert(row);
        ASSERT_IFNOT(
            result.second == true,
            "{0}: PopulateSnapshotAndMetadataCollection: Add StateProvider to the CopyOrCheckpointSnapshot failed, CheckpointLSN: {1}, SPId: {2}, MetadataMode: {3}",
            TraceId,
            prepareCheckpointLSN_,
            row.second->StateProviderId,
            static_cast<int>(filterMode));

        SerializableMetadata::CSPtr serializableMetadataCSPtr = nullptr;
        status = SerializableMetadata::Create(*row.second, GetThisAllocator(), serializableMetadataCSPtr);
        Helper::ThrowIfNecessary(
            status,
            TracePartitionId,
            ReplicaId,
            L"PerformCheckpointAsync: Create SerializableMetadata.",
            Helper::CheckpointManager);

        status = metadataArray.Append(serializableMetadataCSPtr);
        Helper::ThrowIfNecessary(
            status,
            TracePartitionId,
            ReplicaId,
            L"PerformCheckpointAsync: Append SerializableMetadata to serializableMetadataCollection.",
            Helper::CheckpointManager);
    }
}

FABRIC_SEQUENCE_NUMBER CheckpointManager::get_PrepareCheckpointLSN() const
{
    return prepareCheckpointLSN_;
}

CheckpointManager::CheckpointManager(
    __in Data::Utilities::PartitionedReplicaId const & partitionedReplicId,
    __in ApiDispatcher & apiDispatcher,
    __in KStringView const & workFolderPath,
    __in KStringView const & replicaFolderPath,
    __in SerializationMode::Enum serializationMode)
    : KObject()
    , KShared()
    , PartitionedReplicaTraceComponent(partitionedReplicId)
    , checkpointStateLock_()
    , serializationMode_(serializationMode)
    , apiDispatcherSPtr_(&apiDispatcher)
    , prepareCheckpointActiveMetadataSnapshot_()
    , prepareCheckpointDeletedMetadataSnapshot_()
    , copyOrCheckpointMetadataSnapshot_()
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    KString::SPtr tmp = nullptr;
    status = KString::Create(tmp, GetThisAllocator(), replicaFolderPath);
    if (NT_SUCCESS(status) == false)
    {
        SetConstructorStatus(status);
        return;
    }

    workFolder_ = KString::CSPtr(tmp.RawPtr());
    tempCheckpointFilePath_ = GetCheckpointFilePath(TempCheckpointPrefix);
    currentCheckpointFilePath_ = GetCheckpointFilePath(CurrentCheckpointPrefix);
    backupCheckpointFilePath_ = GetCheckpointFilePath(BackupCheckpointPrefix);

    status = MoveManagedFileIfNecessary(workFolderPath);
    if (NT_SUCCESS(status) == false)
    {
        SetConstructorStatus(status);
        return;
    }
}

CheckpointManager::~CheckpointManager()
{
}
