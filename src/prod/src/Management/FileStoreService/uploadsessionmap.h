// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {
        class UploadSessionMap
            : public Store::ReplicaActivityTraceComponent<Common::TraceTaskCodes::FileStoreService>
        {
            DENY_COPY(UploadSessionMap)

        public:
            UploadSessionMap(
                __in RequestManager & requestManager,
                Common::ComponentRootSPtr const & root);

            ~UploadSessionMap();

            Common::ErrorCode GetUploadSessionMapEntry(
                std::wstring const & storeRelativePath,
                __out std::vector<Management::FileStoreService::UploadSessionMetadataSPtr> & sessions);

            Common::ErrorCode GetUploadSessionMapEntry(
                Common::Guid const & sessionId,
                __out Management::FileStoreService::UploadSessionMetadataSPtr & session);

            Common::ErrorCode DeleteUploadSessionMapEntry(
                Common::Guid const & sessionId, 
                __out std::vector<std::wstring> & stagingLocation);

            Common::ErrorCode CreateUploadSessionMapEntry(
                std::wstring const & storeRelativePath,
                Common::Guid const & sessionId,
                uint64 fileSize);

            Common::ErrorCode UpdateUploadSessionMapEntry(
                Common::Guid const & sessionId,
                uint64 startPosition,
                uint64 endPosition,
                std::wstring const & stagingLocation);

            bool IsEmpty() const
            {
                Common::AcquireReadLock grab(this->lock_);
                return this->map_.empty();
            }

            std::size_t GetCount() const
            {
                Common::AcquireReadLock grab(this->lock_);
                return this->map_.size();
            }

        private:

            std::vector<std::wstring> && ProcessCleanupPeriodicTask();
            void ScheduleProcessCleanupPeriodicTask(Common::TimeSpan const delay);
            void ProcessDeleteChunksTask(std::vector<std::wstring> && chunkStagingLocations);

            Common::ComponentRootSPtr root_;
            Management::FileStoreService::UploadSessionMetadataMap map_;
            mutable Common::RwLock lock_;
            Common::TimerSPtr processPendingCleanupTimer_;
            std::vector<std::wstring> toDeleteChunks_;
        };
    }
}
