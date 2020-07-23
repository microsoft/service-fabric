// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ImageStore
    {
        class IImageStore : public Common::TextTraceComponent<Common::TraceTaskCodes::ImageStore>
        {
        public:

            virtual ~IImageStore() {}

            // We need a transacted Remove for the Store/Cache to avoid getting into a corrupted state.
            // The current implementation is not transacted and hence I am commenting out this code to not expose
            // this incorrect API. When we eventually need RemoveContent for cleanup we should implement this correctly.
            // Additionally this call removes the data from the store and the cache while for cleanup generally 
            // only the node data needs to be removed
            // Bug#809517 - Implement a transacted RemoveContent for IImageStore
            Common::ErrorCode RemoveRemoteContent(std::wstring const & object);
            Common::ErrorCode RemoveRemoteContent(std::vector<std::wstring> const & object);

            // Three different overloads for the upload function.
            Common::ErrorCode UploadContent(
                std::vector<std::wstring> const & remoteObjects,
                std::vector<std::wstring> const & localObjects,
                Common::TimeSpan const timeout,
                std::vector<CopyFlag::Enum> const & copyFlags);

            Common::ErrorCode UploadContent(
                std::wstring const & remoteObject,
                std::wstring const & localObject,
                Common::TimeSpan const timeout,
                CopyFlag::Enum copyFlag);

            Common::ErrorCode UploadContent(
                std::vector<std::wstring> const & remoteObjects,
                std::vector<std::wstring> const & localObjects,
                Common::TimeSpan const timeout,
                CopyFlag::Enum copyFlag = CopyFlag::Echo);

            // Three different overloads for the download function.
            Common::ErrorCode DownloadContent(
                std::wstring const & remoteObject,
                std::wstring const & localObject,
                Common::TimeSpan const timeout,
                std::wstring const & remoteChecksumObject = L"",
                std::wstring const & expectedChecksumValue = L"",
                bool refreshCache = false,
                CopyFlag::Enum copyFlag = CopyFlag::Echo,
                bool copyToLocalStoreLayoutOnly = false,
                bool checkForArchive = false);

            Common::ErrorCode DownloadContent(
                std::vector<std::wstring> const & remoteObjects,
                std::vector<std::wstring> const & localObjects,
                Common::TimeSpan const timeout,
                std::vector<CopyFlag::Enum> const & copyFlags);

            Common::ErrorCode DownloadContent(
                std::vector<std::wstring> const & remoteObjects,
                std::vector<std::wstring> const & localObjects,
                Common::TimeSpan const timeout,
                CopyFlag::Enum copyFlag);

            Common::ErrorCode DownloadContent(
                std::wstring const & remoteObject,
                std::wstring const & localObject,
                Common::TimeSpan const timeout,
                CopyFlag::Enum copyFlag);

            Common::ErrorCode DoesContentExistInCache(
                std::wstring const & object,
                __out bool & objectExists);

            Common::ErrorCode DoesContentExist(
                std::vector<std::wstring> const & remoteObjects,  
                Common::TimeSpan const timeout,
                __out std::map<std::wstring, bool> & objectExistsMap);

            Common::ErrorCode RemoveFromCache(std::wstring const & remoteObject);

            Common::ErrorCode GetStoreChecksumValue(
                std::wstring const & remoteObject,
                __inout std::wstring & checkSumValue);

            static Common::ErrorCode ReadChecksumFile(
                std::wstring const & fileName,
                __inout std::wstring & checksumValue);

        protected:
            IImageStore(std::wstring const & localRoot, std::wstring const & cacheRoot);

            virtual Common::ErrorCode OnRemoveObjectFromStore(
                std::vector<std::wstring> const & remoteObject,
                Common::TimeSpan const timeout = Common::TimeSpan::MaxValue) = 0;

            virtual Common::ErrorCode OnDownloadFromStore(
                std::vector<std::wstring> const & remoteObjects,
                std::vector<std::wstring> const & localObjects,
                std::vector<CopyFlag::Enum> const & flags,
                Common::TimeSpan const timeout = Common::TimeSpan::MaxValue) = 0;

            virtual Common::ErrorCode OnUploadToStore(
                std::vector<std::wstring> const & remoteObjects,
                std::vector<std::wstring> const & localObjects,
                std::vector<CopyFlag::Enum> const & flags,
                Common::TimeSpan const timeout = Common::TimeSpan::MaxValue) = 0;

            virtual Common::ErrorCode OnDoesContentExistInStore(
                std::vector<std::wstring> const & remoteObjects,
                Common::TimeSpan const timeout,
                __out std::map<std::wstring, bool> & objectExistsMap) = 0;

            Common::ErrorCode CopyObjects(
                std::vector<std::wstring> const & remoteObjects,
                std::vector<std::wstring> const & localObjects,
                std::vector<std::shared_ptr<std::vector<std::wstring>>> const & skipFiles,
                std::vector<CopyFlag::Enum> const & flags,
                std::vector<BOOLEAN> const & checkMarkFiles,
                Common::TimeSpan const timeout);

            Common::ErrorCode DeleteObjects(
                std::vector<std::wstring> const & tags, 
                Common::TimeSpan const timeout);
            
            std::wstring GetLocalFileNameForUpload(std::wstring const & tag);
            std::wstring GetLocalFileNameForDownload(std::wstring const & tag);
            
            std::wstring localRoot_;
            std::wstring markFile_;

        private:
            Common::ErrorCode DownloadToCache(
                std::wstring const & remoteObject,
                bool shouldOverwrite,
                bool checkForArchive,
                bool isStaging,
                Common::TimeSpan const timeout,
                __out std::wstring & cacheObject,
                __out bool & alreadyInCache);

            Common::ErrorCode ClearCache(std::vector<std::wstring> const & remoteObjects);

            std::wstring GetCacheFileName(std::wstring const & object);            

            static Common::ErrorCode ReplaceErrorCode(Common::ErrorCode const & errorCode);

            Common::ErrorCode DoesArchiveExistInStore(std::wstring const &, Common::TimeSpan const timeout, __out bool & result);
            Common::ErrorCode ExtractArchive(
                std::wstring const & src, 
                std::wstring const & dest, 
                bool overwrite = false, 
                std::wstring const & expectedChecksumValue = L"",
                std::wstring const & cacheChecksumObject = L"");

            Common::ErrorCode RemoveCorruptedContent(std::wstring const & cacheObject, Common::TimeoutHelper const & timeoutHelper);

            std::wstring cacheRoot_;

            static double const CacheLockAcquireTimeoutInSeconds;
            static double const CopyOperationLockAcquireTimeoutInSeconds;
            static std::wstring const CacheTempDirectory;
            static std::wstring const CacheStagingExtension;
        };

        using ImageStoreUPtr = std::unique_ptr<IImageStore>;
    } // end namespace ImageStore
}// end namespace Management
