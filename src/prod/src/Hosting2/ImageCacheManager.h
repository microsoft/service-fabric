//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ImageCacheManager : 
        public Common::RootedObject,
        private Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(ImageCacheManager)

    public:
        explicit ImageCacheManager(
            Common::ComponentRoot const & root,
            __in Management::ImageStore::ImageStoreUPtr & imageStore);
        virtual ~ImageCacheManager() {}

    public:

        AsyncOperationSPtr BeginDownload(
            std::wstring const & remoteSourcePath,
            std::wstring const & localDestinationPath,
            std::wstring const & remoteChecksumObject,
            std::wstring const & expectedChecksumValue,
            bool refreshCache,
            Management::ImageStore::CopyFlag::Enum copyFlag,
            bool copyToLocalStoreLayoutOnly,
            bool checkForArchive,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndDownload(Common::AsyncOperationSPtr const &);

        AsyncOperationSPtr BeginGetChecksum(
            std::wstring const & storeManifestChechsumFilePath,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndGetChecksum(
            AsyncOperationSPtr const &,
            __out std::wstring & expectedChecksum);

        Common::ErrorCode GetStoreChecksumValue(
            std::wstring const & remoteObject,
            _Inout_ std::wstring & checksumValue) const;

    private:
        class DownloadLinkedAsyncOperation;
        class GetChecksumLinkedAsyncOperation;

        void RemoveActiveDownload(std::wstring const & localDestinationPath);
        void RemoveActiveGetCheckSum(std::wstring const & storeManifestChechsumFilePath);

    private:
        std::map<std::wstring, Common::AsyncOperationSPtr> activeDownloads_;
        std::map<std::wstring, Common::AsyncOperationSPtr> activeGetChecksum_;
        
        //Image store and image cache manager are both initialized and kept in the download manager.
        Management::ImageStore::ImageStoreUPtr & imageStore_;
        Common::RwLock downloadLock_;
        Common::RwLock checksumLock_;
    };
}
