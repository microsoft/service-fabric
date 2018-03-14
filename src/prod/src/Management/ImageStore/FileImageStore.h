// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ImageStore
    {
        class FileImageStore : public IImageStore
        {
            DENY_COPY(FileImageStore)

        public:
            FileImageStore(                
                std::wstring const & rootUri,
                std::wstring const & cacheUri,
                std::wstring const & localRoot,
                ImageStoreAccessDescriptionUPtr && accessDescription);

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
            std::wstring GetRemoteFileName(std::wstring const & tag);
            std::wstring RemoveSchemaTag(std::wstring const & uri);

            static Common::ErrorCode CreateAndAclTempDirectory(                 
                Common::SidSPtr const & sid,
                DWORD const accessMask,
                __out std::unique_ptr<Common::ResourceHolder<std::wstring>> & directoryHolder);

            std::wstring rootUri_;
            ImageStoreAccessDescriptionUPtr accessDescription_;
        };
    }
}
