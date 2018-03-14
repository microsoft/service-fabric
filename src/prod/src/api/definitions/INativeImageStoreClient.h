// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class INativeImageStoreClient
    {
    public:
        virtual ~INativeImageStoreClient() {};

        virtual Common::ErrorCode SetSecurity(
            Transport::SecuritySettings && securitySettings) = 0;

        virtual Common::ErrorCode UploadContent(
            std::wstring const & remoteDestination,
            std::wstring const & localSource,
            Common::TimeSpan const timeout,
            Management::ImageStore::CopyFlag::Enum const & copyFlag) = 0;

        virtual Common::ErrorCode UploadContent(
            std::wstring const & remoteDestination,
            std::wstring const & localSource,
            INativeImageStoreProgressEventHandlerPtr const &,
            Common::TimeSpan const timeout,
            Management::ImageStore::CopyFlag::Enum const & copyFlag) = 0;

        virtual Common::AsyncOperationSPtr BeginUploadContent(
            std::wstring const & remoteDestination,
            std::wstring const & localSource,
            Common::TimeSpan const timeout,
            Management::ImageStore::CopyFlag::Enum copyFlag,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::ErrorCode EndUploadContent(
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::ErrorCode CopyContent(
            std::wstring const & remoteSource,
            std::wstring const & remoteDestination,
            Common::TimeSpan const timeout,
            std::shared_ptr<std::vector<std::wstring>> const &skipFiles,
            Management::ImageStore::CopyFlag::Enum const & copyFlag,
            BOOLEAN const & checkMarkFile) = 0;

        virtual Common::AsyncOperationSPtr BeginCopyContent(
            std::wstring const & remoteSource,
            std::wstring const & remoteDestination,
            std::shared_ptr<std::vector<std::wstring>> const & skipFiles,
            Management::ImageStore::CopyFlag::Enum copyFlag,
            BOOLEAN checkMarkFile,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::ErrorCode EndCopyContent(
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::ErrorCode DownloadContent(
            std::wstring const & remoteSource,
            std::wstring const & localDestination,
            Common::TimeSpan const timeout,
            Management::ImageStore::CopyFlag::Enum const & copyFlag) = 0;

        virtual Common::ErrorCode DownloadContent(
            std::wstring const & remoteSource,
            std::wstring const & localDestination,
            INativeImageStoreProgressEventHandlerPtr const &,
            Common::TimeSpan const timeout,
            Management::ImageStore::CopyFlag::Enum const & copyFlag) = 0;
        
        virtual Common::ErrorCode ListContent(
            std::wstring const & remoteLocation,
            BOOLEAN const & isRecursive,
            Common::TimeSpan const timeout,
            __out Management::FileStoreService::StoreContentInfo & result) = 0;

        virtual Common::ErrorCode ListPagedContent(
            Management::ImageStore::ImageStoreListDescription const & listDescription,
            Common::TimeSpan const timeout,
            __out Management::FileStoreService::StorePagedContentInfo & result) = 0;

        virtual Common::AsyncOperationSPtr BeginListContent(
            std::wstring const & remoteLocation,
            BOOLEAN const & isRecursive,        
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::ErrorCode EndListContent(
            Common::AsyncOperationSPtr const &,
            Management::FileStoreService::StoreContentInfo & result) = 0;

        virtual Common::AsyncOperationSPtr BeginListPagedContent(
            Management::ImageStore::ImageStoreListDescription const & listDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::ErrorCode EndListPagedContent(
            Common::AsyncOperationSPtr const &,
            Management::FileStoreService::StorePagedContentInfo & result) = 0;

        virtual Common::ErrorCode DoesContentExist(
            std::wstring const & remoteLocation,
            Common::TimeSpan const timeout,
            __out bool & result) = 0;

        virtual Common::ErrorCode DeleteContent(
            std::wstring const & remoteLocation,
            Common::TimeSpan const timeout) = 0;

        virtual Common::AsyncOperationSPtr BeginDeleteContent(
            std::wstring const & remoteLocation,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::ErrorCode EndDeleteContent(
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginUploadChunk(
            std::wstring const & remoteDestination,
            std::wstring const & localSource,
            Common::Guid const & sessionId,
            uint64 startPosition,
            uint64 endPosition,
            uint64 fileLength,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::ErrorCode EndUploadChunk(
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginListUploadSession(
            std::wstring const & remoteLocation,
            Common::Guid const & sessionId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::ErrorCode EndListUploadSession(
            Common::AsyncOperationSPtr const &,
            Management::FileStoreService::UploadSession & result) = 0;

        virtual Common::AsyncOperationSPtr BeginDeleteUploadSession(
            Common::Guid const & sessionId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::ErrorCode EndDeleteUploadSession(
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginCommitUploadSession(
            Common::Guid const & sessionId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::ErrorCode EndCommitUploadSession(
            Common::AsyncOperationSPtr const &) = 0;
    };
}
