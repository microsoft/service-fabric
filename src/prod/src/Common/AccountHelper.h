// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class AccountHelper
    {
    public:
        static Common::ErrorCode GetDomainUserAccountName(std::wstring accountName, __out std::wstring & username, __out std::wstring & domain, __out std::wstring & dlnFormatName);
        static Common::ErrorCode GetServiceAccountName(std::wstring accountName, __out std::wstring & username, __out std::wstring & domain, __out std::wstring & dlnFormatName);

        static Common::GlobalWString UPNDelimiter;
        static Common::GlobalWString DLNDelimiter;

        static bool GroupAllowsMemberAddition(std::wstring const & groupName);

        // If canSkipAddCounter is true, the counter is not added if the name size is 20 characters.
        static std::wstring GetAccountName(std::wstring const & name, ULONG applicationPackageCounter, bool canSkipAddCounter = false);

        static ErrorCode GetAccountNamesWithCertificateCommonName(
            std::wstring const& commonName,
            X509StoreLocation::Enum storeLocation,
            std::wstring const & storeName,
            bool generateV1Account,
            __inout std::map<std::wstring, std::wstring> & accountNamesMap); // key: account name, value: thumbprint

        static ErrorCode GeneratePasswordForNTLMAuthenticatedUser(
            std::wstring const& accountName,
            Common::SecureString const & passwordSecret,
            __inout Common::SecureString & password);

        static ErrorCode GeneratePasswordForNTLMAuthenticatedUser(
            std::wstring const& accountName,
            Common::SecureString const & passwordSecret,
            X509StoreLocation::Enum storeLocation,
            std::wstring const & x509StoreName,
            std::wstring const & x509Thumbprint,
            __inout Common::SecureString & password);

        static std::wstring GenerateUserAcountNameV1(
            Thumbprint::SPtr const & thumbprint);

        static std::wstring GenerateUserAcountName(
            Thumbprint::SPtr const & thumbprint,
            DateTime const & expirationTime);

        static void ReplaceAccountInvalidChars(__inout std::wstring & accountName);

#if !defined(PLATFORM_UNIX)
        static bool GenerateRandomPassword(__out Common::SecureString & password);
#endif

    private:
        static Common::GlobalWString PasswordPrefix;
        static DWORD const RandomDigitsInPassword;
        static size_t const MaxCharactersInAccountUsername;
        static int64 const ExpiryBaselineTicks;

        static Common::GlobalWString GroupNameLOCAL;
    };
}
