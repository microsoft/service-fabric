// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ImageStore
    {        
        class NativeImageStore 
            : public IImageStore
            , public Api::INativeImageStoreClient
            , public Common::ComponentRoot
            , public Common::TextTraceComponent<Common::TraceTaskCodes::NativeImageStoreClient>
        {
            using Common::TextTraceComponent<Common::TraceTaskCodes::NativeImageStoreClient>::WriteNoise;
            using Common::TextTraceComponent<Common::TraceTaskCodes::NativeImageStoreClient>::WriteInfo;
            using Common::TextTraceComponent<Common::TraceTaskCodes::NativeImageStoreClient>::WriteWarning;
            using Common::TextTraceComponent<Common::TraceTaskCodes::NativeImageStoreClient>::WriteError;
            
        public:
            NativeImageStore(
                std::wstring const & rootUri, 
                std::wstring const & imageCache,
                std::wstring const & workingDirectory, 
                std::shared_ptr<FileStoreService::IFileStoreClient> const &);

            static Common::ErrorCode CreateNativeImageStoreClient(
                bool const isInternal,                
                std::wstring const & workingDirectory,
                Api::IClientFactoryPtr const & clientFactory,
                __out Api::INativeImageStoreClientPtr &);

            static Common::ErrorCode CreateNativeImageStoreClient(
                bool const isInternal,
                std::wstring const & imageCacheDirectory,
                std::wstring const & workingDirectory,
                Api::IClientFactoryPtr const & clientFactory,                
                __out ImageStoreUPtr &);

            // Marks completion of a directory upload
            static Common::GlobalWString DirectoryMarkerFileName;            

            //
            // INativeImageStoreClient
            //
            Common::ErrorCode SetSecurity(
                Transport::SecuritySettings && securitySettings);

            Common::ErrorCode UploadContent(
                std::wstring const & remoteDestination,
                std::wstring const & localSource,
                Common::TimeSpan const timeout,
                CopyFlag::Enum const & copyFlag);

            Common::AsyncOperationSPtr BeginUploadContent(
                std::wstring const & remoteDestination,
                std::wstring const & localSource,
                Common::TimeSpan const timeout,
                Management::ImageStore::CopyFlag::Enum copyFlag,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            Common::ErrorCode EndUploadContent(
                Common::AsyncOperationSPtr const &);

            Common::ErrorCode UploadContent(
                std::wstring const & remoteDestination,
                std::wstring const & localSource,
                Api::INativeImageStoreProgressEventHandlerPtr const &,
                Common::TimeSpan const timeout,
                CopyFlag::Enum const & copyFlag);

            Common::ErrorCode CopyContent(
                std::wstring const & remoteSource,
                std::wstring const & remoteDestination,
                Common::TimeSpan const timeout,
                std::shared_ptr<std::vector<std::wstring>> const & skipFiles,
                CopyFlag::Enum const & copyFlag,
                BOOLEAN const & checkMarkFile);

            Common::AsyncOperationSPtr BeginCopyContent(
                std::wstring const & remoteSource,
                std::wstring const & remoteDestination,
                std::shared_ptr<std::vector<std::wstring>> const & skipFiles,
                Management::ImageStore::CopyFlag::Enum copyFlag,
                BOOLEAN checkMarkFile,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            Common::ErrorCode EndCopyContent(
                Common::AsyncOperationSPtr const &);

            Common::ErrorCode DownloadContent(
                std::wstring const & remoteDestination,
                std::wstring const & localSource,
                Common::TimeSpan const timeout,
                CopyFlag::Enum const & copyFlag);

            Common::ErrorCode DownloadContent(
                std::wstring const & remoteDestination,
                std::wstring const & localSource,
                Api::INativeImageStoreProgressEventHandlerPtr const &,
                Common::TimeSpan const timeout,
                CopyFlag::Enum const & copyFlag);

            Common::ErrorCode ListContent(
                std::wstring const & remoteLocation,
                BOOLEAN const & isRecursive,
                Common::TimeSpan const timeout,
                __out Management::FileStoreService::StoreContentInfo & result);          

            Common::ErrorCode ListPagedContent(
                Management::ImageStore::ImageStoreListDescription const & listDescription,
                Common::TimeSpan const timeout,
                __out Management::FileStoreService::StorePagedContentInfo & result);

            Common::AsyncOperationSPtr BeginListContent(
                std::wstring const & remoteLocation,
                BOOLEAN const & isRecursive,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            Common::ErrorCode EndListContent(
                Common::AsyncOperationSPtr const &,
                __out Management::FileStoreService::StoreContentInfo & result);

            Common::AsyncOperationSPtr BeginListPagedContent(
                Management::ImageStore::ImageStoreListDescription const & listDescription,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            Common::ErrorCode EndListPagedContent(
                Common::AsyncOperationSPtr const &,
                __out Management::FileStoreService::StorePagedContentInfo & result);

            Common::ErrorCode DoesContentExist(
                std::wstring const & remoteLocation,
                Common::TimeSpan const timeout,
                __out bool & result);

            Common::ErrorCode DeleteContent(
                std::wstring const & remoteLocation,
                Common::TimeSpan const timeout);

            Common::AsyncOperationSPtr BeginDeleteContent(
                std::wstring const & remoteLocation,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            Common::ErrorCode EndDeleteContent(
                Common::AsyncOperationSPtr const &);

            Common::AsyncOperationSPtr BeginUploadChunk(
                std::wstring const & remoteDestination,
                std::wstring const & localSource,
                Common::Guid const & sessionId,
                uint64 startPosition,
                uint64 endPosition,
                uint64 fileLength,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            Common::ErrorCode EndUploadChunk(
                Common::AsyncOperationSPtr const &);

            Common::AsyncOperationSPtr BeginListUploadSession(
                std::wstring const & remoteLocation,
                Common::Guid const & sessionId,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);
             
            Common::ErrorCode EndListUploadSession(
                Common::AsyncOperationSPtr const & operation,
                __out Management::FileStoreService::UploadSession & result);

            Common::AsyncOperationSPtr BeginDeleteUploadSession(
                Common::Guid const & sessionId,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            Common::ErrorCode EndDeleteUploadSession(
                Common::AsyncOperationSPtr const &);

            Common::AsyncOperationSPtr BeginCommitUploadSession(
                Common::Guid const & sessionId,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            Common::ErrorCode EndCommitUploadSession(
                Common::AsyncOperationSPtr const &);

        protected:

            //
            // IImageStore
            //
            virtual Common::ErrorCode OnUploadToStore(
                std::vector<std::wstring> const & remoteObjects,
                std::vector<std::wstring> const & localObjects,
                std::vector<CopyFlag::Enum> const & flags,
                Common::TimeSpan const timeout);

            virtual Common::ErrorCode OnDownloadFromStore(
                std::vector<std::wstring> const & remoteObjects,
                std::vector<std::wstring> const & localObjects,
                std::vector<CopyFlag::Enum> const & flags,
                Common::TimeSpan const timeout);

            virtual Common::ErrorCode OnDoesContentExistInStore(
                std::vector<std::wstring> const & remoteObjects,
                Common::TimeSpan const timeout,
                __out std::map<std::wstring, bool> & objectExistsMap);

            virtual Common::ErrorCode OnRemoveObjectFromStore(
                std::vector<std::wstring> const & remoteObjects,
                Common::TimeSpan const timeout);

        private:

            class OperationBaseAsyncOperation;
            class FileExistsAsyncOperation;
            class DirectoryExistsAsyncOperation;
            class ObjectExistsAsyncOperation;
            class DeleteAsyncOperation;
            class ListFileAsyncOperation;
            class UploadFileAsyncOperation;
            class UploadDirectoryAsyncOperation;
            class UploadObjectAsyncOperation;
            class DownloadFileAsyncOperation;
            class DownloadObjectAsyncOperation;
            class CopyObjectAsyncOperation;
            class CopyFileAsyncOperation;
            class CopyDirectoryAsyncOperation;
            class ParallelOperationsBaseAsyncOperation;
            class ParallelUploadObjectsAsyncOperation;
            class ParallelDownloadFilesAsyncOperation;
            class ParallelDownloadObjectsAsyncOperation;
            class ParallelExistsAsyncOperation;
            class ParallelDeleteAsyncOperation;
            class ParallelCopyFilesAsyncOperation;
            class ListUploadSessionAsyncOperation;
            class UploadChunkAsyncOperation;
            class CommitUploadSessionAsyncOperation;
            class DeleteUploadSessionAsyncOperation;

            static std::wstring GetDirectoryMarkerFileName(std::wstring const &);

            static Common::ErrorCode GetFileStoreClient(
                bool const isInternal,
                Api::IClientFactoryPtr const & clientFactory,                 
                __out std::shared_ptr<FileStoreService::IFileStoreClient> & fileStoreClient);

            std::wstring RemoveSchemaTag(std::wstring const & uri);

            Common::ErrorCode SynchronousUpload(
                std::wstring const & src,
                std::wstring const & dest,
                Api::INativeImageStoreProgressEventHandlerPtr const & progressHandler,
                Common::TimeSpan const timeout,
                CopyFlag::Enum const flag,
                __out bool & uploaded);

            Common::ErrorCode SynchronousUpload(
                std::wstring const & src,
                std::wstring const & dest,
                Common::TimeSpan const timeout,
                CopyFlag::Enum const flag,
                __out bool & uploaded);

            Common::ErrorCode SynchronousUpload(
                std::vector<std::wstring> const & sources,
                std::vector<std::wstring> const & destinations,
                std::vector<CopyFlag::Enum> const & flags,
                Common::TimeSpan const timeout);

            Common::ErrorCode SynchronousCopy(
                std::wstring const & src,
                std::wstring const & dest,
                std::shared_ptr<std::vector<std::wstring>> const & skipFiles,
                CopyFlag::Enum const flag,
                BOOLEAN const & checkMarkFile,
                Common::TimeSpan const timeout,
                __out bool & copied);

            Common::ErrorCode SynchronousDownload(
                std::wstring const & src,
                std::wstring const & dest,
                CopyFlag::Enum const flag,
                Api::INativeImageStoreProgressEventHandlerPtr const &,
                Common::TimeSpan const timeout,
                __out bool & result);

            Common::ErrorCode SynchronousDownload(
                std::vector<std::wstring> const & sources,
                std::vector<std::wstring> const & destinations,
                std::vector<CopyFlag::Enum> const & flags,
                Common::TimeSpan const timeout);

            Common::ErrorCode SynchronousList(
                std::wstring const & target,
                BOOLEAN const & isRecursive,
                Common::TimeSpan const timeout,
                __out Management::FileStoreService::StoreContentInfo & result);

            Common::ErrorCode SynchronousList(
                Management::ImageStore::ImageStoreListDescription const & listDescription,
                Common::TimeSpan const timeout,
                __out Management::FileStoreService::StorePagedContentInfo & result);

            Common::ErrorCode SynchronousExists(
                std::wstring const & target,
                Common::TimeSpan const timeout,
                __out bool & exists);

            Common::ErrorCode SynchronousExists(
                std::vector<std::wstring> const & targets,
                Common::TimeSpan const timeout,
                __out std::map<std::wstring, bool> & exists);

            Common::ErrorCode SynchronousDelete(
                std::wstring const & target,
                Common::TimeSpan const timeout,
                __out bool & result);

            Common::ErrorCode SynchronousDelete(
                std::vector<std::wstring> const & target,
                Common::TimeSpan const timeout);

            Common::ErrorCode SynchronousUploadChunk(
                std::wstring const & remoteDestination,
                std::wstring const & localSource,
                Common::Guid const & sessionId,
                uint64 startPosition,
                uint64 endPosition,
                uint64 fileLength,
                Common::TimeSpan const timeout);

            Common::ErrorCode SynchronousListUploadSession(
                std::wstring const & remoteLocation,
                Common::Guid const & sessionId,
                Common::TimeSpan const timeout,
                __out Management::FileStoreService::UploadSession & result);

            Common::ErrorCode SynchronousDeleteUploadSession(
                Common::Guid const & sessionId,
                Common::TimeSpan const timeout);

            Common::ErrorCode SynchronousCommitUploadSession(
                Common::Guid const & sessionId,
                Common::TimeSpan const timeout);

            std::shared_ptr<FileStoreService::IFileStoreClient> client_;
            std::wstring workingDirectory_;            
        };

        typedef std::shared_ptr<NativeImageStore> NativeImageStoreSPtr;
    }
}
