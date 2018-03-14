// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Management::FileStoreService;

StringLiteral const TraceComponent("UploadSessionMap");

UploadSessionMap::UploadSessionMap(
    __in RequestManager & requestManager,
    Common::ComponentRootSPtr const & root)
    : ReplicaActivityTraceComponent(requestManager.ReplicaObj.PartitionedReplicaId, Common::ActivityId(Guid::NewGuid()))
    , root_(root)
    , toDeleteChunks_()
{
}

UploadSessionMap::~UploadSessionMap()
{
}

ErrorCode UploadSessionMap::GetUploadSessionMapEntry(
    Common::Guid const & sessionId,
    UploadSessionMetadataSPtr & session)
{
    Common::AcquireReadLock grab(this->lock_);

    if (this->map_.empty())
    {
        return ErrorCodeValue::NotFound;
    }

    std::map<Common::Guid, UploadSessionMetadataSPtr>::const_iterator it = this->map_.find(sessionId);
    if (it == this->map_.end())
    {
        return ErrorCodeValue::NotFound;
    }

    session = it->second;
    return ErrorCodeValue::Success;
}

ErrorCode UploadSessionMap::GetUploadSessionMapEntry(
    std::wstring const & storeRelativePath,
    vector<UploadSessionMetadataSPtr> & sessions)
{
    Common::AcquireReadLock grab(this->lock_);

    if (this->map_.empty())
    {
        return ErrorCodeValue::NotFound;
    }

    for (std::map<Common::Guid, UploadSessionMetadataSPtr>::const_iterator it = this->map_.begin(); it != this->map_.end(); ++it)
    {
        if (storeRelativePath == it->second->StoreRelativePath)
        {
            UploadSessionMetadataSPtr metadata(it->second);
            sessions.push_back(std::move(metadata));
        }
    }

    return sessions.empty() ? ErrorCodeValue::NotFound : ErrorCodeValue::Success;
}

ErrorCode UploadSessionMap::CreateUploadSessionMapEntry(
    std::wstring const & storeRelativePath,
    Common::Guid const & sessionId,
    uint64 fileSize)
{
    Common::AcquireWriteLock grab(this->lock_);
    if (this->map_.empty())
    {
        this->ScheduleProcessCleanupPeriodicTask(FileStoreServiceConfig::GetConfig().UploadSessionTimeout);
    }

    std::map<Common::Guid, UploadSessionMetadataSPtr>::const_iterator it = this->map_.find(sessionId);
    if (it == this->map_.end())
    {
        UploadSessionMetadata metadata(storeRelativePath, sessionId, fileSize);
        this->map_.insert(make_pair(sessionId, make_shared<UploadSessionMetadata>(metadata)));
    }

    return ErrorCodeValue::Success;
}

ErrorCode UploadSessionMap::DeleteUploadSessionMapEntry(
    Common::Guid const & sessionId,
    __out std::vector<std::wstring> & stagingLocation)
{
    Common::AcquireWriteLock grab(this->lock_);

    if (this->map_.empty()) 
    {
        return ErrorCodeValue::NotFound;
    }

    std::map<Common::Guid, UploadSessionMetadataSPtr>::const_iterator it = this->map_.find(sessionId);
    if (it == this->map_.end())
    {
        return ErrorCodeValue::NotFound;
    }

    it->second->GetStagingLocation(stagingLocation);
    this->map_.erase(sessionId);
    return ErrorCodeValue::Success;
}

ErrorCode UploadSessionMap::UpdateUploadSessionMapEntry(
    Common::Guid const & sessionId,
    uint64 startPosition,
    uint64 endPosition,
    std::wstring const & stagingLocation)
{
    Common::AcquireWriteLock grab(this->lock_);
    std::map<Common::Guid, UploadSessionMetadataSPtr>::const_iterator it = this->map_.find(sessionId);
    if (it == this->map_.end())
    {
        return ErrorCodeValue::NotFound;
    }

    return it->second->AddRange(startPosition, endPosition, stagingLocation);
}

void UploadSessionMap::ScheduleProcessCleanupPeriodicTask(TimeSpan const delay)
{
    if (this->processPendingCleanupTimer_)
    {
        this->processPendingCleanupTimer_->Cancel();
    }

    WriteInfo(
        TraceComponent,
        TraceId,
        "Initialize processPendingCleanupTimer_ with delay {0}",
        delay.ToString());

    auto componentRoot = this->root_->CreateComponentRoot();
    this->processPendingCleanupTimer_ = Timer::Create(TimerTagDefault, [this, componentRoot](Common::TimerSPtr const&) 
    { 
        this->ProcessDeleteChunksTask(this->ProcessCleanupPeriodicTask());
    }, 
        true);

    this->processPendingCleanupTimer_->Change(delay);
}

std::vector<std::wstring> && UploadSessionMap::ProcessCleanupPeriodicTask()
{
    Common::AcquireWriteLock grab(this->lock_);
    Common::TimeSpan uploadSessionTimeout = FileStoreServiceConfig::GetConfig().UploadSessionTimeout;

    std::vector<Common::Guid> toRemoveSessionIds;
    DateTime currentTime = Common::DateTime::get_Now();
    for (std::map<Common::Guid, UploadSessionMetadataSPtr>::const_iterator it = this->map_.begin(); it != this->map_.end(); ++it)
    {
        if (it->second->LastModifiedTime + uploadSessionTimeout <= currentTime)
        {
            toRemoveSessionIds.push_back(it->first);
            std::vector<std::wstring> stagingLocation;
            it->second->GetStagingLocation(stagingLocation);
            std::move(stagingLocation.begin(), stagingLocation.end(), back_inserter(this->toDeleteChunks_));
        }
    }

    if (toRemoveSessionIds.size() == this->map_.size())
    {
        WriteInfo(
            TraceComponent,
            TraceId,
            "Clear SessionMap");

        this->map_.clear();
        this->processPendingCleanupTimer_->Cancel();
    }
    else
    {
        WriteInfo(
            TraceComponent,
            TraceId,
            "Reset SessionMap:ChunksToBeRemoved:{0},SessionToBeRemoved:{1}",
            this->toDeleteChunks_.size(),
            toRemoveSessionIds.size());

        if (!toRemoveSessionIds.empty())
        {
            for (std::vector<Common::Guid>::const_iterator it = toRemoveSessionIds.begin(); it != toRemoveSessionIds.end(); ++it)
            {
                this->map_.erase(*it);
            }
        }

        this->ScheduleProcessCleanupPeriodicTask(FileStoreServiceConfig::GetConfig().ProcessPendingCleanupInterval);
    }

    return std::move(this->toDeleteChunks_);
}

void UploadSessionMap::ProcessDeleteChunksTask(std::vector<std::wstring> && chunkStagingLocations)
{
    if (!chunkStagingLocations.empty())
    {
        ErrorCode error;
        for (std::vector<std::wstring>::iterator it = chunkStagingLocations.begin(); it != chunkStagingLocations.end(); ++it)
        {
            if (File::Exists(*it))
            {
                error = File::Delete2(*it, true);
                if (!error.IsSuccess())
                {
                    WriteWarning(
                        TraceComponent,
                        TraceId,
                        "Deleting chunk:{0}, Error:{1}",
                        *it,
                        error);
                }
            }
        }

        chunkStagingLocations.clear();
    }
}
