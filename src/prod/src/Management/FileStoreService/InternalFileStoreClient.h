// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {
        class InternalFileStoreClient : 
            public IFileStoreClient,
            public Common::ComponentRoot,
            protected Common::TextTraceComponent<Common::TraceTaskCodes::FileStoreService>
        {
            DENY_COPY(InternalFileStoreClient)

        public:            
            InternalFileStoreClient(
                std::wstring const & serviceName,
                Api::IClientFactoryPtr const & clientFactory,
                bool const shouldImpersonate = true);
            virtual ~InternalFileStoreClient();

            Common::ErrorCode SetSecurity(
                Transport::SecuritySettings && securitySettings);

            virtual Common::AsyncOperationSPtr BeginUpload(
                std::wstring const & sourceFullPath,
                std::wstring const & storeRelativePath,
                bool shouldOverwrite,
                Api::IFileStoreServiceClientProgressEventHandlerPtr &&,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const &callback,
                Common::AsyncOperationSPtr const &parent);
            virtual Common::ErrorCode EndUpload(
                Common::AsyncOperationSPtr const &operation);

            virtual Common::AsyncOperationSPtr BeginCopy(
                std::wstring const & sourceStoreRelativePath,
                StoreFileVersion const sourceVersion,
                std::wstring const & destinationStoreRelativePath,
                bool shouldOverwrite,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const &callback,
                Common::AsyncOperationSPtr const &parent);
            virtual Common::ErrorCode EndCopy(
                Common::AsyncOperationSPtr const &operation);

            virtual Common::AsyncOperationSPtr BeginDownload(
                std::wstring const & storeRelativePath,
                std::wstring const & destinationFullPath,
                StoreFileVersion const version,
                std::vector<std::wstring> const & availableShares,
                Api::IFileStoreServiceClientProgressEventHandlerPtr &&,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const &callback,
                Common::AsyncOperationSPtr const &parent);
            virtual Common::ErrorCode EndDownload(
                Common::AsyncOperationSPtr const &operation);

            virtual Common::AsyncOperationSPtr BeginCheckExistence(
                std::wstring const & storeRelativePath,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const &callback,
                Common::AsyncOperationSPtr const &parent);
            virtual Common::ErrorCode EndCheckExistence(
                Common::AsyncOperationSPtr const &operation,
                __out bool & result);

            virtual Common::AsyncOperationSPtr BeginList(
                std::wstring const & storeRelativePath,
                std::wstring const & continuationToken,
                BOOLEAN const & shouldIncludeDetails,
                BOOLEAN const & isRecursive,
                bool isPaging,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const &callback,
                Common::AsyncOperationSPtr const &parent);
            virtual Common::ErrorCode EndList(
                Common::AsyncOperationSPtr const &operation,
                __out StoreFileInfoMap & availableFiles,
                __out std::vector<Management::FileStoreService::StoreFolderInfo> & availableFolders,
                __out std::vector<std::wstring> & availableShares,
                __out std::wstring & continuationToken);

            virtual Common::AsyncOperationSPtr BeginDelete(
                std::wstring const & storeRelativePath,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const &callback,
                Common::AsyncOperationSPtr const &parent);
            virtual Common::ErrorCode EndDelete(
                Common::AsyncOperationSPtr const &operation);

            virtual Common::AsyncOperationSPtr BeginListUploadSession(
                std::wstring const & storeRelativePath,
                Common::Guid const & sessionId,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent) override;
            virtual Common::ErrorCode EndListUploadSession(
                Common::AsyncOperationSPtr const & operation,
                __out UploadSession & uploadSession) override;

            virtual Common::AsyncOperationSPtr BeginCreateUploadSession(
                std::wstring const & storeRelativePath,
                Common::Guid const & sessionId,
                uint64 fileSize,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent) override;
            virtual Common::ErrorCode EndCreateUploadSession(
                Common::AsyncOperationSPtr const & operation) override;

            virtual Common::AsyncOperationSPtr BeginUploadChunk(
                std::wstring const & localSource,
                Common::Guid const & sessionId,
                uint64 startPosition,
                uint64 endPosition,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent) override;
            virtual Common::ErrorCode EndUploadChunk(
                Common::AsyncOperationSPtr const & operation) override;

            virtual Common::AsyncOperationSPtr BeginUploadChunkContent(
                __inout Transport::MessageUPtr &chunkContentMessage,
                __inout Management::FileStoreService::UploadChunkContentDescription & uploadChunkContentDescription,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent) override;
            virtual Common::ErrorCode EndUploadChunkContent(
                Common::AsyncOperationSPtr const & operation) override;

            virtual Common::AsyncOperationSPtr BeginDeleteUploadSession(
                Common::Guid const & sessionId,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent) override;
            virtual Common::ErrorCode EndDeleteUploadSession(
                Common::AsyncOperationSPtr const & operation) override;

            virtual Common::AsyncOperationSPtr BeginCommitUploadSession(
                std::wstring const & storeRelativePath,
                Common::Guid const & sessionId,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent) override;
            virtual Common::ErrorCode EndCommitUploadSession(
                Common::AsyncOperationSPtr const & operation) override;

        private:
            class CopyToStagingLocationAsyncOperation;
            friend class CopyToStagingLocationAsyncOperation;

            class OpenAsyncOperation;

            class ClientBaseAsyncOperation;

            class UploadAsyncOperation;

            class CopyAsyncOperation;

            class DownloadAsyncOperation;

            class DeleteAsyncOperation;

            class CheckExistenceAsyncOperation;

            class ListAsyncOperation;

            class UploadChunkContentAsyncOperation;

            class ListUploadSessionAsyncOperation;

            class CreateUploadSessionAsyncOperation;
           
            class UploadChunkAsyncOperation;
           
            class DeleteUploadSessionAsyncOperation;
           
            class CommitUploadSessionAsyncOperation;
           
        private:
            std::wstring const serviceName_;
            Api::IClientFactoryPtr clientFactory_;
            bool const shouldImpersonate_;

            Api::IInternalFileStoreServiceClientPtr fileStoreServiceClient_;
            std::vector<PartitionContextSPtr> partitions_;
            ImpersonatedSMBCopyContextSPtr smbCopyContext_;

            Common::RwLock lock_;
            bool opened_;
            Common::AsyncOperationSPtr pendingOpenOperation_;

            std::unique_ptr<Common::DefaultTimedJobQueue<InternalFileStoreClient>> fileStoreJobQueue_;
        };
    }
}
