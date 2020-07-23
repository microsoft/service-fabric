// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class AccessToken;
    typedef std::unique_ptr<AccessToken> AccessTokenUPtr;
    typedef std::shared_ptr<AccessToken> AccessTokenSPtr;

#if !defined(PLATFORM_UNIX)
    typedef BOOL(__stdcall *LogonUserFunction)(PCWSTR lpszUsername, PCWSTR lpszDomain, PCWSTR lpszPassword, DWORD dwLogonType, DWORD dwLogonProvider, PTOKEN_GROUPS pTokenGroups, PHANDLE phToken, PSID *ppLogonSid, PVOID *ppProfileBuffer, LPDWORD pdwProfileLength, PQUOTA_LIMITS pQuotaLimits);
#endif

    class AccessToken : public std::enable_shared_from_this<AccessToken>
    {
        DENY_COPY(AccessToken)

    public:
#if !defined(PLATFORM_UNIX)
        AccessToken(TokenHandleSPtr const & tokenHandle);
        AccessToken(TokenHandleSPtr const & tokenHandle, ProfileHandleSPtr const & profileHandle);
        AccessToken(TokenHandleSPtr && tokenHandle);
        AccessToken(TokenHandleSPtr && tokenHandle, ProfileHandleSPtr && profileHandle);
#else
        AccessToken();
#endif
        virtual ~AccessToken();

        static Common::ErrorCode CreateUserToken(
            std::wstring const & accountName,
            std::wstring const & domain,
            std::wstring const & password,
            DWORD logonType,
            DWORD logonProvider,
            bool loadProfile,
            PSID const & sid,
            __out AccessTokenSPtr & accessToken);
        static Common::ErrorCode CreateUserToken(
            std::wstring const & accountName,
            std::wstring const & domain,
            std::wstring const & password,
            DWORD logonType,
            DWORD logonProvider,
            bool loadProfile,
            PSID const & sid,
            __out AccessTokenUPtr & accessToken);

        static Common::ErrorCode CreateDomainUserToken(
            std::wstring const & username,
            std::wstring const & domain,
            Common::SecureString const & password,
            bool interactiveLogon,
            bool loadProfile,
            PSID const & sid,
            __out AccessTokenSPtr & accessToken);

        static Common::ErrorCode CreateServiceAccountToken(
            std::wstring const & serviceAccountName,
            std::wstring const & domain,
            std::wstring const & password,
            bool loadProfile,
            PSID const & sid,
            __out AccessTokenSPtr & accessToken);

        static Common::ErrorCode CreateProcessToken(HANDLE processHandle, DWORD desiredAccess, __out AccessTokenSPtr & accessToken);

        static Common::ErrorCode UpdateCachedCredentials(std::wstring const & username, std::wstring const & domain, std::wstring const & password);

        __declspec(property(get = get_TokenHandle)) TokenHandleSPtr const & TokenHandle;
        TokenHandleSPtr const & get_TokenHandle() const { return this->tokenHandle_; }

        __declspec(property(get = get_ProfileHandle)) ProfileHandleSPtr const & ProfileHandle;
        ProfileHandleSPtr const & get_ProfileHandle() const { return this->profileHandle_; }

        Common::ErrorCode IsAllowedPrivilege(__in std::wstring const & privilege, __out bool & isAllowed);
        Common::ErrorCode EnablePrivilege(__in std::wstring const & privilege);
        Common::ErrorCode CreateRestrictedToken(__out AccessTokenSPtr & restrictedToken);
        Common::ErrorCode CreateRestrictedToken(PSID sidToDeny, __out AccessTokenSPtr & restrictedToken);
        Common::ErrorCode GetUserSid(__out SidSPtr & sid);
        Common::ErrorCode CreateSecurityDescriptor(DWORD access, __out SecurityDescriptorSPtr & sd);

        Common::ErrorCode Impersonate(__out ImpersonationContextUPtr & impersonationContext);

        Common::ErrorCode DuplicateToken(DWORD desiredAccess, __out AccessTokenSPtr & accessToken);

#if !defined(PLATFORM_UNIX)
    private:
        static Common::ErrorCode GetUserTokenAndProfileHandle(
            std::wstring const & accountName,
            std::wstring const & domain,
            std::wstring const & password,
            DWORD logonType,
            DWORD logonProvider,
            bool loadProfile,
            PSID const & sid,
            __out TokenHandleSPtr & tokenHandle,
            __out ProfileHandleSPtr & profileHandle);

        static Common::ErrorCode CreateUserTokenWithSid(
            std::wstring const & accountName,
            std::wstring const & domain,
            std::wstring const & password,
            DWORD logonType,
            DWORD logonProvider,
            PSID const & sidToAdd,
            __out TokenHandleSPtr & accessToken);

        static Common::ErrorCode FreeSidAndReturnError(
            PSID const& logonSid,
            Common::ErrorCode const& error);

        Common::ErrorCode GetSid(TOKEN_INFORMATION_CLASS const tokenClass, __out SidSPtr & sid);
        Common::ErrorCode GetLogonSid(__out SidSPtr & sid);
        Common::ErrorCode GetRestrictedGroups(__out Common::ByteBufferUPtr & tokenGroupsBuffer, __out std::vector<SID_AND_ATTRIBUTES> & groupsToDelete);
        Common::ErrorCode GetDefaultDacl(__out DaclSPtr & defaultDacl);
        Common::ErrorCode SetDefaultDacl(__in DaclSPtr const & dacl);

    private:
        class TokenDacl;
        class TokenSecurityDescriptor;
#endif

    private:
        TokenHandleSPtr const tokenHandle_;
        ProfileHandleSPtr const profileHandle_;

#if defined(PLATFORM_UNIX)
        uid_t uid_;
        wstring accountName_;
        wstring password_;

    public:
        __declspec(property(get = get_Uid)) uid_t const Uid;
        uid_t const get_Uid() const { return this->uid_; }

        __declspec(property(get = get_AccountName)) wstring const AccountName;
        wstring const get_AccountName() const { return this->accountName_; }

        __declspec(property(get = get_Password)) wstring const Password;
        wstring const get_Password() const { return this->password_; }

#endif
    };
}
