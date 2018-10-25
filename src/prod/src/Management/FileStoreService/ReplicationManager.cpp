// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace ServiceModel;
using namespace Store;
using namespace Management::FileStoreService;
using namespace Serialization;
using namespace Api;
using namespace Naming;

StringLiteral const TraceComponent("ReplicationManager");

class ReplicationManager::ProcessCopyJobQueue : public JobQueue<unique_ptr<JobItem<ReplicationManager>>, ReplicationManager>
{
    DENY_COPY(ProcessCopyJobQueue)

public:
    ProcessCopyJobQueue(std::wstring const & name, ReplicationManager & root, int threadCount)
        : JobQueue(name, root, false, threadCount, nullptr, UINT64_MAX, DequePolicy::FifoLifo),
        onFinishEvent_()
    {
    }

    __declspec(property(get=get_OperationsFinishedEvent)) ManualResetEvent & OperationsFinishedAsyncEvent;
    ManualResetEvent & get_OperationsFinishedEvent() { return onFinishEvent_; }

protected:
    void OnFinishItems()
    {
        onFinishEvent_.Set();
    }

private:
    ManualResetEvent onFinishEvent_;
};

class ReplicationManager::ProcessCopyJobItem : public JobItem<ReplicationManager>
{
    DENY_COPY(ProcessCopyJobItem)

public:
    ProcessCopyJobItem(FileMetadata const & metadata, atomic_uint64 & failureCount)
        : metadata_(metadata)
        , failureCount_(failureCount)
    {
    }

    virtual void Process(ReplicationManager & replicationManager)
    {        
        if(!replicationManager.ReplicateFileAndDeleteOlderVersion(metadata_))
        {
            failureCount_++;
        }
    }

private:
    FileMetadata metadata_;
    atomic_uint64 & failureCount_;
};



ReplicationManager::ReplicationManager(
    ImageStoreServiceReplicaHolder const & serviceReplicaHolder)
    : serviceReplicaHolder_(serviceReplicaHolder)
    , currentPrimaryStoreLocation_()
    , secondaryStoreLocations_()
    , lock_()
{
    this->SetTraceId(serviceReplicaHolder.Value.PartitionedReplicaId.TraceId);
}

ReplicationManager::~ReplicationManager()
{
}

bool ReplicationManager::ProcessCopyNotification(IStoreItemEnumeratorPtr const & storeItemEnumerator)
{
    auto error = this->EnsurePrimaryStoreShareLocation();
    if(!error.IsSuccess())
    {
        return false;
    }

    // Get all the files in the store folder and put it in a set for faster lookup
    auto filesToDelete = Utility::GetAllFiles(this->ReplicaObj.StoreRoot);

    ProcessCopyJobQueue jobQueue(this->TraceId, *this, FileStoreServiceConfig::GetConfig().MaxCopyOperationThreads);
    atomic_uint64 failureCount(0);

    while ((error = storeItemEnumerator->MoveNext()).IsSuccess() && failureCount.load() == 0)
    {
        auto storeItem = storeItemEnumerator->GetCurrentItem();        
        if (storeItem->GetType() != Constants::StoreType::FileMetadata || storeItem->IsDelete())
        {
            continue;
        }

        FileMetadata metadata;
        vector<BYTE> data = storeItem->GetValue();
        error = FabricSerializer::Deserialize(metadata, data);

        ASSERT_IFNOT(error.IsSuccess(), "Deserialization of metadata object should succeed. Error:{0}", error);

        if (FileState::IsStable(metadata.State))
        {
            auto availableFile = Utility::GetVersionedFileFullPath(this->ReplicaObj.StoreRoot, metadata.StoreRelativeLocation, metadata.CurrentVersion);
            auto iter = filesToDelete.find(availableFile);
            if (iter != filesToDelete.end())
            {
                // The file is in a stable state and it is already present in the local file store
                // Hence we can skip copy and remove it from the list of files which will be deleted
                filesToDelete.erase(iter);
            }
            else
            {
                // Need to copy the file from another replica in the partition. Add it to job queue
                jobQueue.Enqueue(make_unique<ProcessCopyJobItem>(metadata, failureCount));
            }
        }
    }

    // Close the job queue since all items are already added
    jobQueue.Close();

    // See comments in src\prod\src\store\TSChangeHandler.cpp for why it's okay
    // to block processing of this callback (the replica will still observe
    // ChangeRole->Idle while this is blocking).
    //
    // Wait for all items in the job queue to be processed
    jobQueue.OperationsFinishedAsyncEvent.WaitOne();

    if(failureCount.load() != 0)
    {
        return false;
    }

    // delete all files in store for which are not in a stable state
    for (auto const & file : filesToDelete)
    {       
        wstring versionedStoreRelativePath = file.substr(this->ReplicaObj.StoreRoot.size(), file.size());
#if !defined(PLATFORM_UNIX)
        StringUtility::TrimLeading<wstring>(versionedStoreRelativePath, L"\\");
#else
        StringUtility::TrimLeading<wstring>(versionedStoreRelativePath, L"/");
#endif

        error = Utility::DeleteVersionedStoreFile(
            this->ReplicaObj.StoreRoot, 
            versionedStoreRelativePath);
        WriteTrace(
            error.ToLogLevel(),
            TraceComponent,
            TraceId,
            "Deleting file for which metadata does not exist. Path:{0}, Error:{1}",
            file,
            error);
    }    

    return true;
}

bool ReplicationManager::ProcessReplicationNotification(IStoreNotificationEnumeratorPtr const & storeNotificationEnumerator)
{    
    auto error = this->EnsurePrimaryStoreShareLocation();
    if(!error.IsSuccess())
    {
        return false;
    }

    ProcessCopyJobQueue jobQueue(this->TraceId, *this, FileStoreServiceConfig::GetConfig().MaxCopyOperationThreads);

    atomic_uint64 failureCount(0);
    int jobCount = 0;

    while ((error = storeNotificationEnumerator->MoveNext()).IsSuccess() && failureCount.load() == 0)
    {
        auto storeItem = storeNotificationEnumerator->GetCurrentItem();
        if (storeItem->GetType() != Constants::StoreType::FileMetadata)
        {
            continue;
        }

        bool shouldDelete = storeItem->IsDelete();

        if (!shouldDelete)
        {
            FileMetadata metadata;
            vector<BYTE> data = storeItem->GetValue();
            error = FabricSerializer::Deserialize(metadata, data);

            ASSERT_IFNOT(error.IsSuccess(), "Deserialization of metadata object should succeed. Error:{0}", error);

            bool shouldCopy = false;

            this->GetReplicationFileAction(metadata.State, storeItem->GetKey(), shouldCopy, shouldDelete);

            if (shouldCopy)
            {
                ++jobCount;

                // Need to copy the file from primary. Add it to job queue
                jobQueue.Enqueue(make_unique<ProcessCopyJobItem>(metadata, failureCount));
            }
        }

        if (shouldDelete)
        {
            error = Utility::DeleteAllVersionsOfStoreFile(this->ReplicaObj.StoreRoot, storeItem->GetKey());
            WriteTrace(
                error.ToLogLevel(),
                TraceComponent,
                TraceId,
                "Deleting versioned files of {0} from secondary. Error:{1}",
                storeItem->GetKey(),
                error);
        }
    }

    // Close the job queue since all items are already added
    jobQueue.Close();

    if (jobCount > 0)
    {
        // Wait for all items in the job queue to be processed
        jobQueue.OperationsFinishedAsyncEvent.WaitOne();
    }

    if(failureCount.load() != 0)
    {
        WriteWarning(
            TraceComponent,
            TraceId,
            "{0} failures processing replication notification",
            failureCount.load());

        return false;
    }

    return true;
}

bool ReplicationManager::ReplicateFileAndDeleteOlderVersion(FileMetadata const & metadata)
{
    // TODO: Demote to noise after testing
    WriteInfo(
        TraceComponent,
        TraceId,
        "ReplicateFile: {0}",
        metadata);

    bool success = this->ReplicateFile(metadata);

    if(success && metadata.PreviousVersion != StoreFileVersion::Default)
    {
        wstring previousFilePath = Utility::GetVersionedFileFullPath(this->ReplicaObj.StoreRoot, metadata.StoreRelativeLocation, metadata.PreviousVersion);
        if(File::Exists(previousFilePath))
        {
            auto error = File::Delete2(previousFilePath, true /*deleteReadOnlyFiles*/);
            WriteTrace(
                error.IsSuccess() ? LogLevel::Info : LogLevel::Warning,
                TraceComponent,
                TraceId,
                "Deleting previous version of the file. PreviousFile:{0}, Error:{1}",
                previousFilePath,
                error);
        }
    }

    return success;
}

bool ReplicationManager::ReplicateFile(FileMetadata const & metadata)
{
    wstring currentDestinationFilePath = Utility::GetVersionedFileFullPath(this->ReplicaObj.StoreRoot, metadata.StoreRelativeLocation, metadata.CurrentVersion);
    if(File::Exists(currentDestinationFilePath))
    {
        // If file is already in the secondary's store, skip copy
        return true;
    }

    // If CopyDescription is present and the source file is available locally, attempt to copy locally
    if (!metadata.CopyDesc.IsEmpty())
    {        
        wstring localSourceFilePath = Utility::GetVersionedFileFullPath(this->ReplicaObj.StoreRoot, metadata.CopyDesc.StoreRelativeLocation, metadata.CopyDesc.Version);
        if (File::Exists(localSourceFilePath))
        {
            wstring directory = Path::GetDirectoryName(currentDestinationFilePath);
            if(!Directory::Exists(directory))
            {
                auto error = Directory::Create2(directory);
                if(!error.IsSuccess())
                {
                    WriteWarning(
                        TraceComponent,
                        TraceId,
                        "Failed to create directory during copy on secondary. Path:{0}, Error:{1}",
                        directory,
                        error);
                }
            }

            auto error = File::SafeCopy(localSourceFilePath, currentDestinationFilePath, true /*overwrite*/, false /*shouldAcquireLock*/);
            WriteTrace(
                error.IsSuccess() ? LogLevel::Info : LogLevel::Warning,
                TraceComponent,
                TraceId,
                "CopyFile: SecondaryDestPath:{0}, SecondarySourcePath:{1}, Error:{2}",
                currentDestinationFilePath,
                localSourceFilePath,
                error);

            if (error.IsSuccess())
            {                
                return true;
            }
        }
    }

    uint failureCount = 0;
    static uint chaosFileCopyCount = 0;

    do
    {
        vector<wstring> storeLocations;
        {
            AcquireReadLock lock(lock_);

            storeLocations.push_back(currentPrimaryStoreLocation_);
            storeLocations.insert(storeLocations.end(), secondaryStoreLocations_.begin(), secondaryStoreLocations_.end());
        }

        // Attempt secondaries after trying the primary since the primary itself
        // may still be recovering and not have the file yet.
        //
        for (auto ix=0; ix<storeLocations.size(); ++ix)
        {
            auto const & storeLocation = storeLocations[ix];

            if (storeLocation == this->ReplicaObj.ShareRoot)
            {
                continue;
            }

            auto currentSourceFilePath = Utility::GetVersionedFileFullPath(
                storeLocation, 
                metadata.StoreRelativeLocation, 
                metadata.CurrentVersion);

            if (FileStoreServiceConfig::GetConfig().EnableChaosDuringFileUpload)
            {
                ++chaosFileCopyCount;
                if (chaosFileCopyCount % 11 == 0)
                {
                    WriteWarning(
                        TraceComponent,
                        TraceId,
                        "Chaos enabled. Not copying file from primary nor retrying from any other share. Expecting retry to succeed next time: current src={0} dest={1}",
                        currentSourceFilePath,
                        currentDestinationFilePath);
                    break;
                }
            }

            auto error = this->ReplicaObj.SmbContext->CopyFileW(currentSourceFilePath, currentDestinationFilePath);

            if (error.IsSuccess()) 
            { 
                // TODO: Demote to noise when Image Store Service stabilizes
                WriteInfo(
                    TraceComponent,
                    TraceId,
                    "CopyFile succeeded: current src={0} dest={1}",
                    currentSourceFilePath,
                    currentDestinationFilePath);

                return true; 
            }
            else if (ix < storeLocations.size() - 1)
            {
                WriteInfo(
                    TraceComponent,
                    TraceId,
                    "CopyFile failed - retrying from a different replica: current src={0} dest={1} error={2}",
                    currentSourceFilePath,
                    currentDestinationFilePath,
                    error);
            }
        }

        // Since the file wasn't found on any replica, check the metadata state of the file
        // on the primary. There are two scenarios where the file may no longer exist on
        // the primary:
        //
        // 1) The file may have been deleted or modified by the primary while
        // this secondary was processing a notification. In which case, a replication operation
        // for this file should arrive later in the replication stream.
        //
        // 2) A dataloss event occurred and the primary has abandoned the recovery process,
        // deleting the metadata for files that no longer exist.
        //
        bool isPresent;
        FileState::Enum state;
        StoreFileVersion version;
        wstring storeLocation;
        auto error = this->IsFilePresentInPrimary(metadata.StoreRelativeLocation, isPresent, state, version, storeLocation);

        WriteTrace(
            error.ToLogLevel(LogLevel::Warning, LogLevel::Info),
            TraceComponent,
            TraceId,
            "IsFilePresentInPrimary for {0} in version {1} completed with {2} retryCount={3}.",
            metadata.StoreRelativeLocation,
            metadata.CurrentVersion,
            error,
            failureCount);

        if (error.IsSuccess())
        {
            {
                AcquireWriteLock lock(lock_);
                currentPrimaryStoreLocation_ = storeLocation;
            }

            // We should retry replicating the file if the filestate is in "Updating" state
            // since primary's local state was expected to be in "Updating" state 
            // until it gets quorum number of acks from secondaries to commit to stable state (Available).
            // Based on our replication logic secondary would have received the state as "Available",
            // even though primary's local state is in "Updating".(until it gets quorum acks to change it's local state to "Available").
            if (!isPresent || state == FileState::Deleting || version != metadata.CurrentVersion)
            {
                return true;
            }
        }
        else if (error.IsError(ErrorCodeValue::FSSPrimaryInDatalossRecovery))
        {
            // During dataloss, there is a possibility of additional metadata entries on replicated store than files that exist on disk.
            // In this scenario, primary's change role gets completed but it is stuck in recovery operation,
            // when trying to delete the metadata for the orphaned files (no write quorum).
            // Because secondaries are in IB state trying to catch upto primary by copying files,
            // but they are stuck in a loop trying to copy the missing files.
            // In this scenario both primary and secondary are unable to progress because they have a dependency on each other.

            // When this scenario is detected, we break the dependency by allowing secondary replication operation to succeed
            // even though it was unable to copy missing files so as to get secondaries to the ready state (but still inconsistent).
            // When primary retries to delete metadata entry it is successful now since secondaries are available for the write quorum.
            // Primary will then remove additional metadata entries as part of recovery process to get into a consistent state.
            // Primary then moves on to the active state where it can process fss requests.
            // Secondaries will receive metadata update through replication notification which makes its metadata consistent with the store.
            // The period of time where secondary is yet to receive the replication operation to delete the additional metadata entry is not observable,
            // since all metadata requests goes to primary.

            WriteInfo(
                TraceComponent,
                TraceId,
                "Data loss scenario detected. Primary stuck in recovery and the specified file {0} (version {1}) is present in disk.",
                metadata.StoreRelativeLocation,
                metadata.CurrentVersion);

            return true;
        }

        auto retryDelay = FileStoreServiceConfig::GetConfig().SecondaryFileCopyRetryDelayMilliseconds;

        WriteInfo(
            TraceComponent,
            TraceId,
            "Refreshing share locations: file={0} version={1} delay={2}ms",
            metadata.StoreRelativeLocation,
            metadata.CurrentVersion,
            retryDelay);

        ::Sleep(retryDelay);

        this->TryRefreshShareLocations(metadata.StoreRelativeLocation);

    } while (++failureCount < FileStoreServiceConfig::GetConfig().MaxSecondaryFileCopyFailureThreshold);

    WriteInfo(
        TraceComponent,
        TraceId,
        "Replication failed for {0}",
        metadata);

    return false;
}

void ReplicationManager::TryRefreshShareLocations(wstring const & relativeStorePath)
{
    ManualResetEvent event(false);

    auto operation = this->ReplicaObj.StoreClient->BeginInternalGetStoreLocations(
        this->ReplicaObj.PartitionId,
        FileStoreServiceConfig::GetConfig().InternalServiceCallTimeout,
        [this, &event] (AsyncOperationSPtr const &) { event.Set(); },
        this->CreateAsyncOperationRoot());

    event.WaitOne();

    vector<wstring> secondaryShares;
    auto error = this->ReplicaObj.StoreClient->EndInternalGetStoreLocations(operation, secondaryShares);

    if (error.IsSuccess())
    {
        AcquireReadLock lock(lock_);

        secondaryStoreLocations_ = secondaryShares;
    }

    WriteTrace(
        error.ToLogLevel(LogLevel::Warning, LogLevel::Info),
        TraceComponent,
        TraceId,
        "TryRefreshShareLocations: RelativeStorePath={0} secondaries={1} error={2}",
        relativeStorePath,
        secondaryShares,
        error);
}

// Currently, FileStoreService uses a Naming property for resolving the primary
// store location. Since the V2 storage stack notifications do not block reconfiguration,
// this can be optimized to use the normal service resolution mechanism (i.e. combine
// this operation with InternalGetStoreLocations).
//
ErrorCode ReplicationManager::IsFilePresentInPrimary(
    std::wstring const & relativeStorePath,
    __out bool & isPresent,
    __out FileState::Enum & fileState,
    __out StoreFileVersion & currentFileVersion,
    __out std::wstring & primaryStoreShare) const
{
    ManualResetEvent operationCompleted;
    ErrorCode error;
    ReplicaObj.StoreClient->BeginInternalListFile(
        NamingUri(ReplicaObj.ServiceName),
        ReplicaObj.PartitionId,
        relativeStorePath,
        FileStoreServiceConfig::GetConfig().InternalServiceCallTimeout,
        [this, &operationCompleted, &error, &isPresent, &fileState, &currentFileVersion, &primaryStoreShare] (AsyncOperationSPtr const & operation)
        {
            error = ReplicaObj.StoreClient->EndInternalListFile(operation, isPresent, fileState, currentFileVersion, primaryStoreShare);
            operationCompleted.Set();
        },
        this->CreateAsyncOperationRoot());

    operationCompleted.WaitOne();

    WriteTrace(
        error.IsSuccess() ? LogLevel::Info : LogLevel::Warning,
        TraceComponent,
        TraceId,
        "InternalListFile: RelativeStorePath={0}, IsPresent={1}, FileState={2}, CurrentVersion={3}, PrimaryStoreShare:{4}, Error:{5}",
        relativeStorePath,
        isPresent,
        fileState,
        currentFileVersion,
        primaryStoreShare,
        error);

    ASSERT_IF(error.IsSuccess() && primaryStoreShare.empty(), "primaryStoreShare cannot be empty");

    return error;
}

ErrorCode ReplicationManager::EnsurePrimaryStoreShareLocation()
{
    {
        AcquireReadLock lock(lock_);
        if(!currentPrimaryStoreLocation_.empty())
        {
            return ErrorCodeValue::Success;
        }
    }

    ManualResetEvent event(false);

    auto operation = ReplicaObj.StoreClient->BeginInternalGetStoreLocation(
        NamingUri(ReplicaObj.ServiceName),
        ReplicaObj.PartitionId,
        FileStoreServiceConfig::GetConfig().InternalServiceCallTimeout,
        [this, &event] (AsyncOperationSPtr const &) { event.Set(); },
        this->CreateAsyncOperationRoot());

    event.WaitOne();

    wstring primaryStoreShareLocation;
    auto error = ReplicaObj.StoreClient->EndInternalGetStoreLocation(operation, primaryStoreShareLocation);

    WriteTrace(
        error.ToLogLevel(LogLevel::Warning, LogLevel::Info),
        TraceComponent,
        TraceId,
        "BeginInternalGetStoreLocation: RelativeStorePath={0}, Error:{1}",
        primaryStoreShareLocation,
        error);

    if(error.IsSuccess())
    {
        ASSERT_IF(primaryStoreShareLocation.empty(), "primaryStoreShare cannot be empty");

        AcquireWriteLock lock(lock_);        
        currentPrimaryStoreLocation_ = primaryStoreShareLocation;
    }

    return error;
}

void ReplicationManager::GetReplicationFileAction(
    FileState::Enum fileState, 
    wstring const & key, 
    __out bool & shouldCopy, 
    __out bool & shouldDelete)
{
    switch (fileState)
    {
    case FileState::Updating:
    case FileState::Committed:
        shouldCopy = false;
        shouldDelete = false;
        break;

    case FileState::Available_V1:
    case FileState::Replicating:
        shouldCopy = true;
        shouldDelete = false;
        break;

    case FileState::Deleting:
        shouldCopy = false;
        shouldDelete = true;
        break;

    default:
        WriteInfo(
            TraceComponent,
            TraceId,
            "Ignoring unrecognized state {0} on {1}",
            fileState,
            key);

        shouldCopy = false;
        shouldDelete = false;
        break;
    }
}
