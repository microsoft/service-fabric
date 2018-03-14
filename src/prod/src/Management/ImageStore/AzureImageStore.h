// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ImageStore
    {
        class AzureImageStore : public IImageStore
        {
            DENY_COPY(AzureImageStore)
        public:
            AzureImageStore(                
                std::wstring const & localCachePath,
                std::wstring const & localRoot,
                std::wstring const & workingDirectory,
                int minTransferBPS);

            static Common::GlobalWString SchemaTag;

        protected:
            virtual Common::ErrorCode OnRemoveObjectFromStore(
                std::vector<std::wstring> const & remoteObject,
                Common::TimeSpan const timeout);

            virtual Common::ErrorCode OnDownloadFromStore(
                std::vector<std::wstring> const & remoteObjects,
                std::vector<std::wstring> const & localObjects,
                std::vector<CopyFlag::Enum> const & flags, 
                Common::TimeSpan const timeout);

            virtual Common::ErrorCode OnUploadToStore(
                std::vector<std::wstring> const & remoteObjects,
                std::vector<std::wstring> const & localObjects,
                std::vector<CopyFlag::Enum> const & flags,
                Common::TimeSpan const timeout);

            virtual Common::ErrorCode OnDoesContentExistInStore(
                std::vector<std::wstring> const & remoteObjects,
                Common::TimeSpan const timeout,
                __out std::map<std::wstring, bool> & objectExistsMap);

        private:
            std::wstring PrepareCommandLine(
                std::vector<std::wstring> const & storeTags, 
                std::vector<std::wstring> const & cacheTags,
                std::vector<CopyFlag::Enum> const & flags,
                std::wstring const & command,
                std::wstring const & inputFile,
                std::wstring const & outputFile = L"");

            Common::ErrorCode RunStoreExe(
                std::wstring const & cmdLine,
                Common::TimeSpan const timeout);
            
        private:
            std::wstring workingDirectory_;
            int minTransferBPS_;

        };
    }
}
