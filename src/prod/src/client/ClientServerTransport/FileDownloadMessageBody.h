// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class FileDownloadMessageBody : public ServiceModel::ClientServerMessageBody
    {
    public:
        FileDownloadMessageBody()
        {
        }

        explicit FileDownloadMessageBody(
            std::wstring const & serviceName,
            std::wstring const & storeRelativePath,
            Management::FileStoreService::StoreFileVersion const & storeFileVersion,
            std::vector<std::wstring> const & availableShares)
            : serviceName_(serviceName)
            , storeRelativePath_(storeRelativePath)
            , storeFileVersion_(storeFileVersion)
            , availableShares_(availableShares)
        {
        }

        __declspec(property(get=get_ServiceName)) std::wstring const & ServiceName;
        std::wstring const & get_ServiceName() { return serviceName_; }

        __declspec(property(get=get_StoreRelativePath)) std::wstring const & StoreRelativePath;
        std::wstring const & get_StoreRelativePath() { return storeRelativePath_; }

        __declspec(property(get=get_StoreFileVersion)) Management::FileStoreService::StoreFileVersion const & StoreFileVersion;
        Management::FileStoreService::StoreFileVersion const & get_StoreFileVersion() { return storeFileVersion_; }

        __declspec(property(get=get_AvailableShares)) std::vector<std::wstring> & AvailableShares;
        std::vector<std::wstring> & get_AvailableShares() { return availableShares_; }

        FABRIC_FIELDS_04(serviceName_, storeRelativePath_, storeFileVersion_, availableShares_);

    private:
        std::wstring serviceName_;
        std::wstring storeRelativePath_;
        Management::FileStoreService::StoreFileVersion storeFileVersion_;
        std::vector<std::wstring> availableShares_;
    };

}
