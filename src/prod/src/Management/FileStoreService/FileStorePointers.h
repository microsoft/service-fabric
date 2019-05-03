// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {  
        typedef std::unique_ptr<Store::StoreTransaction> StoreTransactionUPtr;

        class IFileStoreClient;
        typedef std::unique_ptr<IFileStoreClient> IFileStoreClientUPtr;

        class InternalFileStoreClient;
        typedef std::shared_ptr<InternalFileStoreClient> InternalFileStoreClientSPtr;

        class RequestManager;
        typedef std::shared_ptr<RequestManager> RequestManagerSPtr;
        
        class ReplicationManager;
        typedef std::shared_ptr<ReplicationManager> ReplicationManagerSPtr;

        class FileStoreServiceReplica;
        typedef Common::RootedObjectHolder<FileStoreServiceReplica> ImageStoreServiceReplicaHolder;

        class PartitionContext;
        typedef std::shared_ptr<PartitionContext> PartitionContextSPtr;

        class FileStoreServiceReplica;
        typedef std::shared_ptr<FileStoreServiceReplica> FileStoreServiceReplicaSPtr;

        class FileStoreServiceFactory;
        typedef Common::RootedObjectHolder<FileStoreServiceFactory> FileStoreServiceFactoryHolder;

        class FileStoreServiceCounters;
        typedef std::shared_ptr<FileStoreServiceCounters> FileStoreServiceCountersSPtr;

        class UploadSessionMetadata;
        typedef std::shared_ptr<UploadSessionMetadata> UploadSessionMetadataSPtr;
        typedef std::map<Common::Guid, UploadSessionMetadataSPtr> UploadSessionMetadataMap;
    }
}
