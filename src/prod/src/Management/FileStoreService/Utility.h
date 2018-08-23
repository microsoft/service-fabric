// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    { 
        // Map with user tokens per user account name
        using AccessTokensCollection = std::map<std::wstring, Common::AccessTokenSPtr>;
        using AccessTokensList = std::vector<Common::AccessTokenSPtr>;

        class Utility
            : private Common::TextTraceComponent<Common::TraceTaskCodes::FileStoreService>
        {
        public:            
            static std::set<std::wstring, Common::IsLessCaseInsensitiveComparer<std::wstring>> GetAllFiles(std::wstring const & root);

            static std::wstring GetVersionedFileFullPath(std::wstring const & root, std::wstring const & relativeFilePath, StoreFileVersion const version);            
            static std::wstring GetVersionedFileFullPath(std::wstring const & root, std::wstring const & relativeFilePath, std::wstring const & version);            
            static std::wstring GetVersionedFile(std::wstring const & relativeFilePath, std::wstring const & pattern);

            static std::wstring GetSharePath(std::wstring const & ipAddressOrFQDN, std::wstring const & shareName, FABRIC_REPLICA_ID const replicaId);

            static Common::ErrorCode DeleteVersionedStoreFile(std::wstring const & root, std::wstring const & versionedStoreRelativePath);
            static Common::ErrorCode DeleteAllVersionsOfStoreFile(std::wstring const & root, std::wstring const & storeRelativePath);

            static void DeleteEmptyDirectory(std::wstring const & directoryPath, int level);

            static Common::ErrorCode RetriableOperation(std::function<Common::ErrorCode()> const & operation, uint const maxRetryCount);

            // tokenMap is the full list, while newTokens contains the values which are not in the original tokenMap
            static Common::ErrorCode GetAccessTokens(
                __inout AccessTokensCollection & tokenMap,
                __inout AccessTokensList & newTokens);

            static bool HasAccountsConfigured();
            static bool HasCommonNameAccountsConfigured();
            static bool HasThumbprintAccountsConfigured();

            static Common::ErrorCode GetPrincipalsDescriptionFromConfigByThumbprint(
                __inout ServiceModel::PrincipalsDescriptionUPtr & principalDescription);

            // newThumbprints contains values which are not in currentThumbprints
            static Common::ErrorCode GetPrincipalsDescriptionFromConfigByCommonName(
                std::vector<std::wstring> const & currentThumbprints,
                __inout ServiceModel::PrincipalsDescriptionUPtr & principalDescription,
                __inout std::vector<std::wstring> & newThumbprints);

        private:

            class CommonNameConfig
            {

            public:
                CommonNameConfig(
                    std::wstring const & commonName,
                    std::wstring const & storeLocation,
                    std::wstring const & storeName,
                    Common::SecureString const & ntlmPasswordSecret)
                    : commonName_(commonName)
                    , storeName_(storeName)
                    , ntlmPasswordSecret_(ntlmPasswordSecret)
                {
                    X509StoreLocation::Parse(storeLocation, storeLocation_);
                }

                __declspec(property(get = get_CommonName)) std::wstring const & CommonName;
                std::wstring const & get_CommonName() const { return commonName_; }

                __declspec(property(get = get_StoreLocation)) X509StoreLocation::Enum StoreLocation;
                X509StoreLocation::Enum get_StoreLocation() const { return storeLocation_; }

                __declspec(property(get = get_StoreName)) std::wstring const & StoreName;
                std::wstring const & get_StoreName() const { return storeName_; }

                __declspec(property(get = get_NtlmPasswordSecret)) Common::SecureString const & NtlmPasswordSecret;
                Common::SecureString const & get_NtlmPasswordSecret() const { return ntlmPasswordSecret_; }

            private:
                std::wstring commonName_;
                X509StoreLocation::Enum storeLocation_;
                std::wstring storeName_;
                Common::SecureString ntlmPasswordSecret_;
            };

            static std::vector<Utility::CommonNameConfig> GetCommonNamesFromConfig();

            static ServiceModel::PrincipalsDescriptionUPtr CreateFileStoreServicePrincipalDescription(
                __out std::wstring & defaultUserParentApplicationGroupName);

            static void AddUser(
                ServiceModel::PrincipalsDescriptionUPtr const & principalDescription,
                ServiceModel::SecurityUserDescription && user,
                std::wstring const & defaultUserParentApplicationGroupName);

            static Common::ErrorCode GetPrimaryAccessToken(
                __inout Common::AccessTokenSPtr & primaryAccessToken);
            static Common::ErrorCode GetSecondaryAccessToken(
                __inout Common::AccessTokenSPtr & secondaryAccessToken);
            static Common::ErrorCode GetCommonNamesAccessTokens(
                __inout AccessTokensCollection & tokenMap,
                __inout AccessTokensList & newTokens);

            static Common::ErrorCode GetAccessToken(
                std::wstring const & name,
                std::wstring const & accountTypeString,
                std::wstring const & accountName,
                Common::SecureString const & accountPassword,
                Common::SecureString const & ntlmPasswordSecret,
                std::wstring const & x509StoreLocation,
                std::wstring const & x509StoreName,
                std::wstring const & x509Thumbprint,
                __inout Common::AccessTokenSPtr & accessToken);

            static ServiceModel::SecurityUserDescription GetSecurityUser(
                std::wstring const & accountName,
                std::wstring const & accountType,
                std::wstring const & accountUserName,
                std::wstring const & accountUserPassword,
                bool isUserPasswordEncrypted,
                std::wstring const & ntlmPasswordSecret,
                bool isNTLMPasswordSecretEncrypted,
                Common::X509StoreLocation::Enum x509StoreLocation,
                std::wstring const & x509StoreName,
                std::wstring const & x509Thumbprint);

            static ServiceModel::SecurityUserDescription GetNTLMSecurityUser(
                std::wstring const & accountName,
                std::wstring const & ntlmPasswordSecret,
                bool isNTLMPasswordSecretEncrypted,
                Common::X509StoreLocation::Enum x509StoreLocation,
                std::wstring const & x509StoreName,
                std::wstring const & x509Thumbprint);

            static Common::ErrorCode GetNTLMLocalUserAccessToken(
                std::wstring const & name,
                Common::SecureString const & ntlmPasswordSecret,
                std::wstring const & x509StoreLocationString,
                std::wstring const & x509StoreName,
                std::wstring const & x509Thumbprint,
                __inout Common::AccessTokenSPtr & accessToken);

            static Common::ErrorCode GetNTLMLocalUserAccessToken(
                std::wstring const & name,
                Common::SecureString const & ntlmPasswordSecret,
                Common::X509StoreLocation::Enum x509StoreLocation,
                std::wstring const & x509StoreName,
                std::wstring const & x509Thumbprint,
                __inout Common::AccessTokenSPtr & accessToken);
        };
    }
}
