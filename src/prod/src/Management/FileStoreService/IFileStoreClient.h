// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {        
        class IFileStoreClient
        {
        public:
            virtual ~IFileStoreClient() {};

            virtual Common::ErrorCode SetSecurity(
                Transport::SecuritySettings && securitySettings) = 0;

            virtual Common::AsyncOperationSPtr BeginUpload(
                std::wstring const & sourceFullPath,
                std::wstring const & storeRelativePath,
                bool shouldOverwrite,
                Api::IFileStoreServiceClientProgressEventHandlerPtr &&,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const &callback,
                Common::AsyncOperationSPtr const &parent) = 0;
            virtual Common::ErrorCode EndUpload(
                Common::AsyncOperationSPtr const &operation) = 0;

            virtual Common::AsyncOperationSPtr BeginCopy(
                std::wstring const & sourceStoreRelativePath,
                StoreFileVersion const sourceVersion,
                std::wstring const & destinationStoreRelativePath,
                bool shouldOverwrite,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const &callback,
                Common::AsyncOperationSPtr const &parent) = 0;
            virtual Common::ErrorCode EndCopy(
                Common::AsyncOperationSPtr const &operation) = 0;

            virtual Common::AsyncOperationSPtr BeginDownload(
                std::wstring const & storeRelativePath,
                std::wstring const & destinationFullPath,
                StoreFileVersion const version,
                std::vector<std::wstring> const & availableShares,
                Api::IFileStoreServiceClientProgressEventHandlerPtr &&,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const &callback,
                Common::AsyncOperationSPtr const &parent) = 0;
            virtual Common::ErrorCode EndDownload(
                Common::AsyncOperationSPtr const &operation) = 0;            

            virtual Common::AsyncOperationSPtr BeginCheckExistence(
                std::wstring const & storeRelativePath,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const &callback,
                Common::AsyncOperationSPtr const &parent) = 0;
            virtual Common::ErrorCode EndCheckExistence(
                Common::AsyncOperationSPtr const &operation,
                __out bool & result) = 0;

            virtual Common::AsyncOperationSPtr BeginList(
                std::wstring const & storeRelativePath,
                std::wstring const & continuationToken,
                BOOLEAN const & shouldIncludeDetails,
                BOOLEAN const & isRecursive,
                bool isPaging,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const &callback,
                Common::AsyncOperationSPtr const &parent) = 0;
            virtual Common::ErrorCode EndList(
                Common::AsyncOperationSPtr const &operation,
                __out StoreFileInfoMap & availableFiles,
                __out std::vector<StoreFolderInfo> & availableFolders,
                __out std::vector<std::wstring> & availableShares,
                __out std::wstring & continuationToken) = 0;

            virtual Common::AsyncOperationSPtr BeginDelete(
                std::wstring const & storeRelativePath,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const &callback,
                Common::AsyncOperationSPtr const &parent) = 0;
            virtual Common::ErrorCode EndDelete(
                Common::AsyncOperationSPtr const &operation) = 0;
            
            virtual Common::AsyncOperationSPtr BeginListUploadSession(
                std::wstring const & storeRelativePath,
                Common::Guid const & sessionId,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent) = 0;
            virtual Common::ErrorCode EndListUploadSession(
                Common::AsyncOperationSPtr const & operation,
                __out UploadSession & uploadSession) = 0;

            virtual Common::AsyncOperationSPtr BeginCreateUploadSession(
                std::wstring const & storeRelativePath,
                Common::Guid const & sessionId,
                uint64 fileSize,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent) = 0;
            virtual Common::ErrorCode EndCreateUploadSession(
                Common::AsyncOperationSPtr const & operation) = 0;

            virtual Common::AsyncOperationSPtr BeginUploadChunk(
                std::wstring const & localSource,
                Common::Guid const & sessionId,
                uint64 startPosition,
                uint64 endPosition,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent) = 0;
            virtual Common::ErrorCode EndUploadChunk(
                Common::AsyncOperationSPtr const & operation) = 0;

            virtual Common::AsyncOperationSPtr BeginUploadChunkContent(
                __inout Transport::MessageUPtr &chunkContentMessage,
                __inout Management::FileStoreService::UploadChunkContentDescription & uploadChunkContentDescription,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent) = 0;
            virtual Common::ErrorCode EndUploadChunkContent(
                Common::AsyncOperationSPtr const & operation) = 0;

            virtual Common::AsyncOperationSPtr BeginDeleteUploadSession(
                Common::Guid const & sessionId,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent) = 0;
            virtual Common::ErrorCode EndDeleteUploadSession(
                Common::AsyncOperationSPtr const & operation) = 0;

            virtual Common::AsyncOperationSPtr BeginCommitUploadSession(
                std::wstring const & storeRelativePath,
                Common::Guid const & sessionId,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent) = 0;
            virtual Common::ErrorCode EndCommitUploadSession(
                Common::AsyncOperationSPtr const & operation) = 0;

        };
    }
}
