// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace AzureActiveDirectory
{
    class ServerWrapper
    {
    public:

        static Common::ErrorCode ValidateClaims(
            std::wstring const &,
            __out std::wstring & claims,
            __out Common::TimeSpan & expiration);

        static bool TryOverrideClaimsConfigForAad();

        static bool IsAadEnabled();

        static bool IsAadServiceEnabled();

    private:

        static std::wstring CreateAdminRoleClaim(Common::SecurityConfig const &);
        static std::wstring CreateUserRoleClaim(Common::SecurityConfig const &);

#if !defined(PLATFORM_UNIX)
        static Common::ErrorCode ValidateClaims_Windows(
            std::wstring const &,
            __out std::wstring &,
            __out Common::TimeSpan &);
#endif

        static Common::ErrorCode ValidateClaims_Linux(
            std::wstring const &,
            __out std::wstring &,
            __out Common::TimeSpan &);
    };
}
