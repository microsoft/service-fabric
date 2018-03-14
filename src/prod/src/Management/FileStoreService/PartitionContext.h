// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {       
        class PartitionContext 
            : public Common::RootedObject
            , private Common::TextTraceComponent<Common::TraceTaskCodes::FileStoreService>
        {          
            DENY_COPY(PartitionContext)

        public:
            PartitionContext(
                Common::ComponentRoot const & root,
                Common::Guid const & partitionId,
                Api::IInternalFileStoreServiceClientPtr const & fileStoreServiceClient);
            ~PartitionContext();

            __declspec(property(get=get_PartitionId)) Common::Guid const & PartitionId;
            inline Common::Guid const & get_PartitionId() const { return partitionId_; }

            bool DoesMatchPartition(uint64 const partitionKey) const;

            Common::AsyncOperationSPtr BeginGetStagingLocation(
                StagingLocationInfo previousInfo,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const &callback,
                Common::AsyncOperationSPtr const & parent);
            Common::ErrorCode EndGetStagingLocation(
                Common::AsyncOperationSPtr const & operation,
                __out StagingLocationInfo & stagingLocationInfo);

        private:
            void StartRefresh(uint64 const currentSequenceNumber);
            void OnRefreshCompleted(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);

            class GetStagingLocationAsyncOperation;
            friend class GetStagingLocationAsyncOperation;

            class RefreshStagingLocationAsyncOperation;
            friend class RefreshStagingLocationAsyncOperation;
            typedef std::shared_ptr<RefreshStagingLocationAsyncOperation> RefreshStagingLocationAsyncOperationSPtr;

            class RefreshedStagingLocationEventArgs
            {
            public:                
                RefreshedStagingLocationEventArgs(StagingLocationInfo const & stagingLocationInfo, Common::ErrorCode const & errorCode)
                    : stagingLocationInfo_(stagingLocationInfo), error_(errorCode)
                {                    
                }

                RefreshedStagingLocationEventArgs(RefreshedStagingLocationEventArgs const & other)
                    : stagingLocationInfo_(other.stagingLocationInfo_), error_(other.error_)
                {
                }

                RefreshedStagingLocationEventArgs const & operator = (RefreshedStagingLocationEventArgs const & other)
                {
                    this->stagingLocationInfo_ = other.stagingLocationInfo_;
                    this->error_ = other.error_;
                }

                __declspec(property(get=get_StagingLocation)) StagingLocationInfo const & StagingLocation;
                StagingLocationInfo const & get_StagingLocation() const { return stagingLocationInfo_; }

                __declspec(property(get=get_Error)) Common::ErrorCode const & Error;
                Common::ErrorCode const & get_Error() const { return error_; }

            private:
                StagingLocationInfo stagingLocationInfo_;
                Common::ErrorCode error_;
            };

        private:            
            Common::Guid const partitionId_;
            Api::IInternalFileStoreServiceClientPtr fileStoreServiceClient_;                        
            Common::RwLock lock_;

            // Following members should always be accessed under lock
            StagingLocationInfo currentShareLocationInfo_;
            Common::AsyncOperationSPtr refreshAsyncOperation_;
        };
    }
}
