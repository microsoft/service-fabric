// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ImageStore
    {
        class ImageStoreFactory
        {
        public:
            static Common::ErrorCode CreateImageStore(
                __out ImageStoreUPtr & imageStore,				
                Common::SecureString const & storeUri,
                int minTransferBPS,
                Api::IClientFactoryPtr const &,
				bool const isInternal,
                std::wstring const & workingDirectory,
                std::wstring const & localCachePath = L"",
                std::wstring const & localRoot = L"");

            static Common::ErrorCode CreateImageStore(
                __out ImageStoreUPtr & imageStore,				
                Common::SecureString const & storeUri,
                int minTransferBPS,
                std::wstring const & localCachePath = L"",
                std::wstring const & localRoot = L"");

            static bool TryParseFileImageStoreConnectionString(
                Common::SecureString const & imageStoreConnectionString,
                __out std::wstring & rootUri,
                __out std::wstring & accountName,
                __out Common::SecureString & accountPassword,
                __out ServiceModel::SecurityPrincipalAccountType::Enum & accountType);

            static Common::ErrorCode GetAccessToken(
                std::wstring const & accountName,
                Common::SecureString const & accountPassword,
                ServiceModel::SecurityPrincipalAccountType::Enum const & accountType,
                __out Common::AccessTokenSPtr & imageStoreAccessToken);

        private:
            static Common::GlobalWString AccountNameKey;
            static Common::GlobalWString AccountPasswordKey;
            static Common::GlobalWString AccountTypeKey;

            static Common::GlobalWString DomainUser_AccountType;
            static Common::GlobalWString ManagedServiceAccount_AccountType;
        };
    }
}
