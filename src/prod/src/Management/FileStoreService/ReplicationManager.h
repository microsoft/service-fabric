// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {                        
        class ReplicationManager :
            public Common::ComponentRoot,  
            private Common::TextTraceComponent<Common::TraceTaskCodes::FileStoreService>
        {
            DENY_COPY(ReplicationManager)      

        public:
            ReplicationManager(
                ImageStoreServiceReplicaHolder const & serviceReplicaHolder);
            ~ReplicationManager();

            bool ProcessCopyNotification(Api::IStoreItemEnumeratorPtr const & storeItemEnumerator);
            bool ProcessReplicationNotification(Api::IStoreNotificationEnumeratorPtr const & storeNotificationEnumerator);            

        private:
            class ProcessCopyJobQueue;
            class ProcessCopyJobItem;
            friend class ProcessCopyJobItem;

            bool ReplicateFileAndDeleteOlderVersion(FileMetadata const & metadata);
            bool ReplicateFile(FileMetadata const & metadata);            
            Common::ErrorCode EnsurePrimaryStoreShareLocation();
            void TryRefreshShareLocations(std::wstring const & relativeStorePath);
            Common::ErrorCode IsFilePresentInPrimary(
                std::wstring const & relativeStorePath, 
                __out bool & isPresent,
                __out FileState::Enum & fileState, 
                __out StoreFileVersion & currentFileVersion, 
                __out std::wstring & primaryStoreShare) const;

            void GetReplicationFileAction(FileState::Enum, std::wstring const & key, __out bool & shouldCopy, __out bool & shouldDelete);

            __declspec(property(get=get_ReplicaObj)) FileStoreServiceReplica const & ReplicaObj;
            inline FileStoreServiceReplica const & get_ReplicaObj() const { return serviceReplicaHolder_.RootedObject; }

        private:                             
            Common::RwLock lock_;
            std::wstring currentPrimaryStoreLocation_;
            std::vector<std::wstring> secondaryStoreLocations_;
            ImageStoreServiceReplicaHolder serviceReplicaHolder_;            
        };
    }
}
