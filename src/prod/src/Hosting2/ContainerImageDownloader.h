// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ContainerImageDownloader :
        public Common::RootedObject,
        protected Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(ContainerImageDownloader)

    public:
        ContainerImageDownloader(
            IContainerActivator const & activator,
            std::wstring const & containerHostAddress,
            Common::ComponentRoot const & root);

        virtual ~ContainerImageDownloader();
        
        Common::AsyncOperationSPtr BeginDownloadImage(
            std::wstring const & image,
            ServiceModel::RepositoryCredentialsDescription const & credentials,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndDownloadImage(
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginDownloadImages(
            std::vector<ContainerImageDescription> const & images,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndDownloadImages(
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginDeleteImages(
            std::vector<std::wstring> const & images,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndDeleteImages(
            Common::AsyncOperationSPtr const & operation);

        void AddImageToCache(std::string const &);
        void RemoveImageFromCache(std::string const &);
        bool IsImageCached(std::string const &);
       

    private:
        class DownloadImageAsyncOperation;
        class DownloadImagesAsyncOperation;
        class DeleteImagesAsyncOperation;

    private:
        std::wstring containerHostAddress_;
        std::set<std::string> imageMap_;
        Common::RwLock lock_;
#if defined(PLATFORM_UNIX)
        std::unique_ptr<web::http::client::http_client> httpClient_;
#endif
        IContainerActivator const & containerActivator_;

    };
}
