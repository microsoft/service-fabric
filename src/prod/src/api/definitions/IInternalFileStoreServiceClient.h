// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IInternalFileStoreServiceClient
    {
    public:
        virtual ~IInternalFileStoreServiceClient() {};        

        virtual Common::AsyncOperationSPtr BeginGetStagingLocation(             
            Common::Guid const & targetPartitionId,
            Common::TimeSpan const timeout, 
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndGetStagingLocation(
            Common::AsyncOperationSPtr const &operation,
            __out std::wstring & stagingLocationShare) = 0; 

        virtual Common::AsyncOperationSPtr BeginUpload(            
            Common::Guid const & targetPartitionId,
            std::wstring const & stagingRelativePath,
            std::wstring const & storeRelativePath,
            bool shouldOverwrite,
            Common::TimeSpan const timeout, 
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndUpload(
            Common::AsyncOperationSPtr const &operation) = 0;

        virtual Common::AsyncOperationSPtr BeginCopy(
            Common::Guid const & targetPartitionId,
            std::wstring const & sourceStoreRelativePath,
            Management::FileStoreService::StoreFileVersion sourceVersion,
            std::wstring const & destinationStoreRelativePath,
            bool shouldOverwrite,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndCopy(
            Common::AsyncOperationSPtr const &operation) = 0;

        virtual Common::AsyncOperationSPtr BeginCheckExistence(
            Common::Guid const & targetPartitionId,
            std::wstring const & storeRelativePath,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndCheckExistence(
            Common::AsyncOperationSPtr const &operation,
            __out bool & result) = 0;

        virtual Common::AsyncOperationSPtr BeginListFiles(
            Common::Guid const & targetPartitionId,
            std::wstring const & storeRelativePath,
            std::wstring const & continuationToken,
            BOOLEAN const & shouldIncludeDetail,
            BOOLEAN const & isRecursive,
            bool isPaging,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndListFiles(
            Common::AsyncOperationSPtr const &operation,
            __out std::vector<Management::FileStoreService::StoreFileInfo> & availableFiles,
            __out std::vector<Management::FileStoreService::StoreFolderInfo> & availableFolders,
            __out std::vector<std::wstring> & availableShares,
            __inout std::wstring & continuationToken) = 0;

        virtual Common::AsyncOperationSPtr BeginDelete(                  
            Common::Guid const & targetPartitionId,
            std::wstring const & storeRelativePath,
            Common::TimeSpan const timeout, 
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndDelete(
            Common::AsyncOperationSPtr const &operation) = 0;

        virtual Common::AsyncOperationSPtr BeginInternalGetStoreLocation(            
            Common::NamingUri const & serviceName,
            Common::Guid const & targetPartitionId,
            Common::TimeSpan const timeout, 
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndInternalGetStoreLocation(
            Common::AsyncOperationSPtr const &operation,
            __out std::wstring & storeLocationShare) = 0;

        virtual Common::AsyncOperationSPtr BeginInternalGetStoreLocations(            
            Common::Guid const & targetPartitionId,
            Common::TimeSpan const timeout, 
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndInternalGetStoreLocations(
            Common::AsyncOperationSPtr const &operation,
            __out std::vector<std::wstring> & secondaryShares) = 0;

        virtual Common::AsyncOperationSPtr BeginInternalListFile(
            Common::NamingUri const & serviceName,
            Common::Guid const & targetPartitionId,
            std::wstring const & storeRelativePath,
            Common::TimeSpan const timeout, 
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndInternalListFile(
            Common::AsyncOperationSPtr const &operation,
            __out bool & isPresent,
            __out Management::FileStoreService::FileState::Enum & currentState, 
            __out Management::FileStoreService::StoreFileVersion & currentVersion,
            __out std::wstring & storeShareLocation) = 0;

        virtual Common::AsyncOperationSPtr BeginListUploadSession(
            Common::Guid const & targetPartitionId,
            std::wstring const & storeRelativePath,
            Common::Guid const & sessionId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndListUploadSession(
            Common::AsyncOperationSPtr const & operation,
            __out Management::FileStoreService::UploadSession & uploadSession) = 0;

        virtual Common::AsyncOperationSPtr BeginCreateUploadSession(
            Common::Guid const & targetPartitionId,
            std::wstring const & storeRelativePath,
            Common::Guid const & sessionId,
            uint64 fileSize,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndCreateUploadSession(
            Common::AsyncOperationSPtr const & operation) = 0;

        virtual Common::AsyncOperationSPtr BeginUploadChunk(
            Common::Guid const & targetPartitionId,
            std::wstring const & stagingFullPath,
            Common::Guid const & sessionId,
            uint64 startPosition,
            uint64 endPosition,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndUploadChunk(
            Common::AsyncOperationSPtr const & operation) = 0;

        virtual Common::AsyncOperationSPtr BeginUploadChunkContent(
            Common::Guid const & targetPartitionId,
            __inout Transport::MessageUPtr &chunkContentMessage,
            __inout Management::FileStoreService::UploadChunkContentDescription & uploadChunkContentDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndUploadChunkContent(
            Common::AsyncOperationSPtr const & operation) = 0;

        virtual Common::AsyncOperationSPtr BeginDeleteUploadSession(
            Common::Guid const & targetPartitionId,
            Common::Guid const & sessionId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndDeleteUploadSession(
            Common::AsyncOperationSPtr const & operation) = 0;

        virtual Common::AsyncOperationSPtr BeginCommitUploadSession(
            Common::Guid const & targetPartitionId,
            std::wstring const & storeRelativePath,
            Common::Guid const & sessionId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndCommitUploadSession(
            Common::AsyncOperationSPtr const & operation) = 0;
    };
}
