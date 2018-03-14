// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IFileStoreServiceClient
    {
    public:
        virtual ~IFileStoreServiceClient() {};        

        virtual Common::AsyncOperationSPtr BeginUploadFile(            
            std::wstring const & serviceName,
            std::wstring const & sourceFullPath,
            std::wstring const & storeRelativePath,
            bool shouldOverwrite,
            IFileStoreServiceClientProgressEventHandlerPtr &&,
            Common::TimeSpan const timeout, 
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndUploadFile(
            Common::AsyncOperationSPtr const &operation) = 0;

        virtual Common::AsyncOperationSPtr BeginDownloadFile(            
            std::wstring const & serviceName,            
            std::wstring const & storeRelativePath,
            std::wstring const & destinationFullPath,
            Management::FileStoreService::StoreFileVersion const & version,
            std::vector<std::wstring> const & availableShares,
            Common::TimeSpan const timeout, 
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndDownloadFile(
            Common::AsyncOperationSPtr const &operation) = 0;        
    };
}
