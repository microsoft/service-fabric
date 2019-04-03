// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

StringLiteral TraceType_AccessToken = "AccessToken";

#if defined(PLATFORM_UNIX)
AccessToken::AccessToken() {}
AccessToken::~AccessToken() {}

ErrorCode AccessToken::Impersonate(__out ImpersonationContextUPtr & impersonationContext)
{
    impersonationContext = make_unique<ImpersonationContext>(nullptr);
    return ErrorCode::Success();
}

ErrorCode AccessToken::GetUserSid(__out SidSPtr & sid)
{
    SidUPtr sidUPtr;
    ErrorCode error = BufferedSid::GetCurrentUserSid(sidUPtr);
    if (error.IsSuccess())
    {
        BufferedSid::CreateSPtr(sidUPtr->PSid, sid);
    }
    return error;
}

ErrorCode AccessToken::DuplicateToken(DWORD desiredAccess, __out AccessTokenSPtr & accessToken)
{
    accessToken = make_shared<AccessToken>();
    accessToken->uid_ = this->uid_;
    return ErrorCode::Success();
}

ErrorCode AccessToken::CreateProcessToken(HANDLE processHandle, DWORD desiredAccess, __out AccessTokenSPtr & accessToken)
{
    uid_t uid = 0;
    DWORD processId = GetProcessId(processHandle);

    stringstream ss;
    ss << "/proc/" << processId << "/status";
    int fd = open(ss.str().c_str(), O_RDONLY | O_NOATIME);

    char buf[4096] = {};
    if (0 < read(fd, buf, 4095))
    {
        string s(buf);
        string uidtag("Uid:");
        int idx = s.find(uidtag);
        if (idx != string::npos)
        {
            idx += (int)uidtag.length();
            sscanf(s.c_str() + idx, "%d", &uid);
        }
    }

    accessToken = make_shared<AccessToken>();
    accessToken->uid_ = uid;
    return ErrorCode::Success();
}

ErrorCode AccessToken::CreateRestrictedToken(PSID sidToDeny, __out AccessTokenSPtr & restrictedToken)
{
    UNREFERENCED_PARAMETER(sidToDeny);

    restrictedToken = make_shared<AccessToken>();
    return ErrorCode::Success();
}

ErrorCode AccessToken::CreateServiceAccountToken(wstring const & serviceAccountName, wstring const & domain, wstring const & password, bool loadProfile, PSID const & sid, __out AccessTokenSPtr & accessToken)
{
    UNREFERENCED_PARAMETER(serviceAccountName);
    UNREFERENCED_PARAMETER(domain);
    UNREFERENCED_PARAMETER(password);
    UNREFERENCED_PARAMETER(loadProfile);
    UNREFERENCED_PARAMETER(sid);

    accessToken = make_shared<AccessToken>();
    return ErrorCode::Success();
}

ErrorCode AccessToken::CreateUserToken(
    wstring const & accountName,
    wstring const & domain,
    wstring const & password,
    DWORD logonType,
    DWORD logonProvider,
    bool loadProfile,
    PSID const & sid,
    __out AccessTokenSPtr & accessToken)
{
    accessToken = make_shared<AccessToken>();
    accessToken->accountName_ = accountName;
    accessToken->password_ = password;
    return ErrorCode(ErrorCodeValue::Success);
}

#else

#include <ntsecapi.h>
#include <winbasep.h>

class AccessToken::TokenDacl : public Dacl
{
    DENY_COPY(TokenDacl)

public:
    TokenDacl(ByteBuffer && buffer)
        : Dacl(GetPACL(buffer)),
        buffer_(move(buffer))
    {
    }

    virtual ~TokenDacl()
    {
    }

public:
    static ErrorCode CreateSPtr(ByteBuffer && buffer, __out DaclSPtr & dacl)
    {
        dacl = make_shared<TokenDacl>(move(buffer));
        return ErrorCode(ErrorCodeValue::Success);
    }

    static PACL GetPACL(ByteBuffer const & buffer)
    {
        if (buffer.size() == 0)
        {
            return NULL;
        }
        else
        {
            auto pByte = const_cast<BYTE*>(buffer.data());
            PTOKEN_DEFAULT_DACL defaultTokenDacl = reinterpret_cast<PTOKEN_DEFAULT_DACL>(pByte);
            return defaultTokenDacl->DefaultDacl;
        }
    }

private:
    ByteBuffer const buffer_;
};


class AccessToken::TokenSecurityDescriptor : public SecurityDescriptor
{
    DENY_COPY(TokenSecurityDescriptor)

public:
    explicit TokenSecurityDescriptor(unique_ptr<SECURITY_DESCRIPTOR> && descriptor, DaclSPtr const & dacl)
        : SecurityDescriptor(descriptor.get()),
        descriptor_(move(descriptor)),
        dacl_(dacl)
    {
        ASSERT_IF(dacl.get() == nullptr, "Specified Dacl must be valid.");
    }

    virtual ~TokenSecurityDescriptor()
    {
    }

    static ErrorCode CreateSPtr(DaclSPtr const & dacl, __out SecurityDescriptorSPtr & sd)
    {
        auto descriptor = make_unique<SECURITY_DESCRIPTOR>();
        if (!::InitializeSecurityDescriptor(descriptor.get(), SECURITY_DESCRIPTOR_REVISION))
        {
            return ErrorCode::TraceReturn(
                ErrorCode::FromWin32Error(),
                TraceTaskCodes::Common,
                TraceType_AccessToken,
                "InitializeSecurityDescriptor");
        }

        if (!::SetSecurityDescriptorDacl(descriptor.get(), true, dacl->PAcl, FALSE))
        {
            return ErrorCode::TraceReturn(
                ErrorCode::FromWin32Error(),
                TraceTaskCodes::Common,
                TraceType_AccessToken,
                "SetSecurityDescriptorDacl");
        }

        sd = make_shared<TokenSecurityDescriptor>(move(descriptor), dacl);
        return ErrorCode(ErrorCodeValue::Success);
    }

private:
    unique_ptr<SECURITY_DESCRIPTOR> descriptor_;
    DaclSPtr const dacl_;
};

AccessToken::AccessToken(TokenHandleSPtr const & tokenHandle)
    : tokenHandle_(tokenHandle), profileHandle_()
{
}

AccessToken::AccessToken(TokenHandleSPtr const & tokenHandle, ProfileHandleSPtr const & profileHandle)
    : tokenHandle_(tokenHandle), profileHandle_(profileHandle)
{
}

AccessToken::AccessToken(TokenHandleSPtr && tokenHandle)
    : tokenHandle_(move(tokenHandle)), profileHandle_()
{
}

AccessToken::AccessToken(TokenHandleSPtr && tokenHandle, ProfileHandleSPtr && profileHandle)
    : tokenHandle_(move(tokenHandle)), profileHandle_(move(profileHandle))
{
}

AccessToken::~AccessToken()
{
}

ErrorCode AccessToken::CreateProcessToken(HANDLE processHandle, DWORD desiredAccess, __out AccessTokenSPtr & accessToken)
{
    ASSERT_IF(processHandle == NULL, "ProcessHandle must be valid.");

    HANDLE token;
    if (!::OpenProcessToken(processHandle, desiredAccess, &token))
    {
        auto error = ErrorCode::FromWin32Error();
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_AccessToken,
            "OpenProcessToken failed. ErrorCode={0}",
            error);
        return error;
    }

    auto tokenHandle = TokenHandle::CreateSPtr(token);
    accessToken = make_shared<AccessToken>(move(tokenHandle));

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode AccessToken::GetUserTokenAndProfileHandle(
    wstring const & accountName,
    wstring const & domain,
    wstring const & password,
    DWORD logonType,
    DWORD logonProvider,
    bool loadProfile,
    PSID const & sid,
    __out TokenHandleSPtr & tokenHandle,
    __out ProfileHandleSPtr & profileHandle)
{
    ASSERT_IF(accountName.empty(), "AccountName must not be empty.");
    ASSERT_IF(password.empty(), "Password must not be empty.");

    HANDLE token;
    if (sid)
    {
        auto error = AccessToken::CreateUserTokenWithSid(accountName, domain, password, logonType, logonProvider, sid, tokenHandle);
        if (!error.IsSuccess())
        {
            return error;
        }
    }
    else
    {
        if (!::LogonUser(accountName.c_str(), domain.c_str(), password.c_str(), logonType, logonProvider, &token))
        {
            auto error = ErrorCode::FromWin32Error();
            TraceWarning(
                TraceTaskCodes::Common,
                TraceType_AccessToken,
                "LogonUser failed for {0}. ErrorCode={1}",
                accountName,
                error);
            return error;
        }
        tokenHandle = TokenHandle::CreateSPtr(token);
    }

    if (loadProfile)
    {
        PROFILEINFO profileInfo = { 0 };
        profileInfo.dwSize = sizeof(profileInfo);
        profileInfo.lpUserName = (LPWSTR)&accountName[0];

        if (!::LoadUserProfileW(tokenHandle->Value, &profileInfo))
        {
            auto error = ErrorCode::FromWin32Error();
            TraceWarning(
                TraceTaskCodes::Common,
                TraceType_AccessToken,
                "LoadUserProfileW failed for {0}. ErrorCode={1}",
                accountName,
                error);
            return error;
        }
        profileHandle = ProfileHandle::CreateSPtr(tokenHandle, profileInfo.hProfile);

        TraceInfo(
            TraceTaskCodes::Common,
            TraceType_AccessToken,
            "LoadUserProfileW succeeded for {0}.",
            accountName);
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode AccessToken::CreateUserToken(
    wstring const & accountName,
    wstring const & domain,
    wstring const & password,
    DWORD logonType,
    DWORD logonProvider,
    bool loadProfile,
    PSID const & sid,
    __out AccessTokenSPtr & accessToken)
{
    TokenHandleSPtr tokenHandle;
    ProfileHandleSPtr profileHandle = nullptr;

    auto error = GetUserTokenAndProfileHandle(
        accountName,
        domain,
        password,
        logonType,
        logonProvider,
        loadProfile,
        sid,
        tokenHandle,
        profileHandle);
    if (!error.IsSuccess())
    {
        return error;
    }

    accessToken = make_shared<AccessToken>(move(tokenHandle), move(profileHandle));

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode AccessToken::CreateUserToken(
    wstring const & accountName,
    wstring const & domain,
    wstring const & password,
    DWORD logonType,
    DWORD logonProvider,
    bool loadProfile,
    PSID const & sid,
    __out AccessTokenUPtr & accessToken)
{
    TokenHandleSPtr tokenHandle;
    ProfileHandleSPtr profileHandle = nullptr;
    auto error = GetUserTokenAndProfileHandle(
        accountName,
        domain,
        password,
        logonType,
        logonProvider,
        loadProfile,
        sid,
        tokenHandle,
        profileHandle);
    if (!error.IsSuccess())
    {
        return error;
    }

    accessToken = make_unique<AccessToken>(move(tokenHandle), move(profileHandle));

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode AccessToken::CreateUserTokenWithSid(
    wstring const & accountName,
    wstring const & domain,
    wstring const & password,
    DWORD logonType,
    DWORD logonProvider,
    PSID const & sidToAdd,
    __out TokenHandleSPtr & tokenHandle)
{
    PTOKEN_GROUPS tokenGroups = NULL;
    ByteBuffer buffer;
    PSID logonSid = NULL;
    LUID luid;

    ErrorCode error(ErrorCodeValue::Success);

    ASSERT_IF(sidToAdd == NULL, "Sid cannot be null");

    error = SecurityUtility::EnsurePrivilege(SE_TCB_NAME);
    if (!error.IsSuccess())
    {
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_AccessToken,
            "Ensure privilege check failed. ErrorCode={0}",
            error);
        return error;
    }

    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    if (!::AllocateLocallyUniqueId(&luid))
    {
        error = ErrorCode::FromWin32Error();
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_AccessToken,
            "AllocateLocallyUniqueId failed for {0}. ErrorCode={1}",
            accountName,
            error);
        return error;
    }

    if (!::AllocateAndInitializeSid(&ntAuthority,
        SECURITY_LOGON_IDS_RID_COUNT,
        SECURITY_LOGON_IDS_RID,
        luid.HighPart,
        luid.LowPart,
        0, 0, 0, 0, 0,
        &logonSid
    ))
    {
        error = ErrorCode::FromWin32Error();
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_AccessToken,
            "AllocateAndInitializeSid failed for {0}. ErrorCode={1}",
            accountName,
            error);
        return error;
    }
    buffer.resize(sizeof(TOKEN_GROUPS) + 2 * sizeof(SID_AND_ATTRIBUTES), 0);
    tokenGroups = reinterpret_cast<PTOKEN_GROUPS>(buffer.data());
    tokenGroups->GroupCount = 2;
    // Add the local SID and the logon SID - same as what LogonUser does
    tokenGroups->Groups[0].Sid = logonSid;
    tokenGroups->Groups[0].Attributes =
        SE_GROUP_MANDATORY | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_LOGON_ID | SE_GROUP_ENABLED;
    tokenGroups->Groups[1].Sid = sidToAdd;
    tokenGroups->Groups[1].Attributes =
        SE_GROUP_MANDATORY | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_ENABLED;

    HANDLE token;

    // As stated at https://docs.microsoft.com/en-us/windows/desktop/secauthn/logonuserexexw, LogonUserExExW method is not direclty available in any public header
    // You must use the GetModuleHandle and GetProcAddress functions to dynamically link to Advapi32.dll and invoke the function.
    auto moduleHandle = GetModuleHandle(L"Advapi32");
    if (moduleHandle == nullptr)
    {
        error = ErrorCode::FromWin32Error();
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_AccessToken,
            "GetModuleHandle for Advapi32 failed with ErrorCode={0}",
            error);

        return FreeSidAndReturnError(logonSid, error);
    }

    LogonUserFunction logonFunction = (LogonUserFunction)GetProcAddress(moduleHandle, "LogonUserExExW");
    if (logonFunction == nullptr)
    {
        error = ErrorCode::FromWin32Error();
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_AccessToken,
            "GetProcAddress for LogonUserExExW failed with ErrorCode={0}",
            error);

        return FreeSidAndReturnError(logonSid, error);
    }

    auto result = logonFunction(
        accountName.c_str(),
        domain.c_str(),
        password.c_str(),
        logonType,
        logonProvider,
        tokenGroups,
        &token,
        nullptr,
        nullptr,
        nullptr,
        nullptr);

    if (!result)
    {
        error = ErrorCode::FromWin32Error();
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_AccessToken,
            "LogonUserExExW failed for {0}. ErrorCode={1}",
            accountName,
            error);
    }
    else
    {
        TraceInfo(
            TraceTaskCodes::Common,
            TraceType_AccessToken,
            "LogonUserExExW call succeeded for {0}",
            accountName);
        tokenHandle = TokenHandle::CreateSPtr(token);
    }

    return FreeSidAndReturnError(logonSid, error);
}

ErrorCode AccessToken::FreeSidAndReturnError(
    PSID const& sid,
    Common::ErrorCode const& error)
{
    if (sid != NULL)
    {
        ::FreeSid(sid);
    }

    return error;
}

ErrorCode AccessToken::CreateDomainUserToken(
    wstring const & userName,
    wstring const & domain,
    SecureString const & password,
    bool interactiveLogon,
    bool loadProfile,
    PSID const & sid,
    __out AccessTokenSPtr & accessToken)
{
    DWORD logonType = interactiveLogon ? LOGON32_LOGON_INTERACTIVE : LOGON32_LOGON_NETWORK_CLEARTEXT;
    return AccessToken::CreateUserToken(userName, domain, password.GetPlaintext(), logonType, LOGON32_PROVIDER_DEFAULT, loadProfile, sid, accessToken);
}

ErrorCode AccessToken::CreateServiceAccountToken(wstring const & serviceAccountName, wstring const & domain, wstring const & password, bool loadProfile, PSID const & sid, __out AccessTokenSPtr & accessToken)
{
    TokenHandleSPtr tokenHandle = nullptr;
    ProfileHandleSPtr profileHandle = nullptr;
    HANDLE token = nullptr;

    if (sid)
    {
        auto error = AccessToken::CreateUserTokenWithSid(
            serviceAccountName.c_str(),
            domain.c_str(),
            password.c_str(),
            LOGON32_LOGON_SERVICE,
            LOGON32_PROVIDER_DEFAULT,
            sid,
            tokenHandle);
        if (!error.IsSuccess())
        {
            return error;
        }
    }
    else
    {
        if (!::LogonUser(serviceAccountName.c_str(), domain.c_str(), password.c_str(), LOGON32_LOGON_SERVICE, LOGON32_PROVIDER_DEFAULT, &token))
        {
            auto error = ErrorCode::FromWin32Error();
            TraceWarning(
                TraceTaskCodes::Common,
                TraceType_AccessToken,
                "LogonUser failed for {0}. ErrorCode={1}",
                serviceAccountName,
                error);
            return error;
        }
        tokenHandle = TokenHandle::CreateSPtr(token);
    }

    if (loadProfile)
    {
        PROFILEINFO profileInfo = { 0 };
        profileInfo.dwSize = sizeof(profileInfo);
        profileInfo.lpUserName = (LPWSTR)&serviceAccountName[0];

        if (!::LoadUserProfileW(tokenHandle->Value, &profileInfo))
        {
            auto error = ErrorCode::FromWin32Error();
            TraceWarning(
                TraceTaskCodes::Common,
                TraceType_AccessToken,
                "LoadUserProfileW failed for {0}. ErrorCode={1}",
                serviceAccountName,
                error);
            return error;
        }
        profileHandle = ProfileHandle::CreateSPtr(tokenHandle, profileInfo.hProfile);

        TraceInfo(
            TraceTaskCodes::Common,
            TraceType_AccessToken,
            "LoadUserProfileW succeeded for {0}.",
            serviceAccountName);
    }

    accessToken = make_unique<AccessToken>(move(tokenHandle), move(profileHandle));
    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode AccessToken::DuplicateToken(DWORD desiredAccess, __out AccessTokenSPtr & accessToken)
{
    HANDLE token;
    if (!::DuplicateTokenEx(tokenHandle_->Value,
        desiredAccess,
        NULL,
        SecurityImpersonation,
        TokenPrimary,
        &token))
    {
        return ErrorCode::FromWin32Error();
    }
    TokenHandleSPtr tokenHandle = TokenHandle::CreateSPtr(token);
    accessToken = make_shared<AccessToken>(move(tokenHandle));
    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode AccessToken::IsAllowedPrivilege(__in wstring const & privilege, __out bool & isAllowed)
{
    LUID luid = { 0 };
    if (!::LookupPrivilegeValue(NULL, &privilege[0], &luid))
    {
        return ErrorCode::TraceReturn(
            ErrorCode::FromWin32Error(),
            TraceTaskCodes::Common,
            TraceType_AccessToken,
            "LookupPrivilegeValue");
    }

    BOOL result = FALSE;
    PRIVILEGE_SET privilegeSet = { 0 };
    privilegeSet.Control = PRIVILEGE_SET_ALL_NECESSARY;
    privilegeSet.PrivilegeCount = 1;
    privilegeSet.Privilege[0].Attributes = SE_PRIVILEGE_ENABLED;
    privilegeSet.Privilege[0].Luid = luid;

    if (!PrivilegeCheck(TokenHandle->Value, &privilegeSet, &result))
    {
        return ErrorCode::TraceReturn(
            ErrorCode::FromWin32Error(),
            TraceTaskCodes::Common,
            TraceType_AccessToken,
            "PrivilegeCheck");
    }

    isAllowed = (result == TRUE);
    return ErrorCode(ErrorCode(ErrorCodeValue::Success));
}

ErrorCode AccessToken::EnablePrivilege(__in wstring const & privilege)
{
    if (privilege.empty())
    {
        return ErrorCode::FromHResult(E_INVALIDARG);
    }

    LUID luid = { 0 };
    if (!::LookupPrivilegeValue(NULL, &privilege[0], &luid))
    {
        return ErrorCode::TraceReturn(
            ErrorCode::FromWin32Error(),
            TraceTaskCodes::Common,
            TraceType_AccessToken,
            "LookupPrivilegeValue");
    }

    TOKEN_PRIVILEGES tokenPrivilege = { 0 };
    tokenPrivilege.PrivilegeCount = 1;
    tokenPrivilege.Privileges[0].Luid = luid;
    tokenPrivilege.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!::AdjustTokenPrivileges(TokenHandle->Value, FALSE, &tokenPrivilege, sizeof(TOKEN_PRIVILEGES), (PTOKEN_PRIVILEGES)NULL, (PDWORD)NULL))
    {
        return ErrorCode::TraceReturn(
            ErrorCode::FromWin32Error(),
            TraceTaskCodes::Common,
            TraceType_AccessToken,
            "AdjustTokenPrivileges");
    }

    if (::GetLastError() == ERROR_NOT_ALL_ASSIGNED)
    {
        return ErrorCode(ErrorCodeValue::OperationFailed);
    }
    else
    {
        return ErrorCode(ErrorCodeValue::Success);
    }
}

ErrorCode AccessToken::CreateRestrictedToken(__out AccessTokenSPtr & restrictedToken)
{
    ByteBufferUPtr originalTokenGroupsBuffer;
    vector<SID_AND_ATTRIBUTES> groupsToDelete;
    auto error = this->GetRestrictedGroups(originalTokenGroupsBuffer, groupsToDelete);
    if (!error.IsSuccess()) { return error; }

    HANDLE tHandle;
    if (!::CreateRestrictedToken(
        TokenHandle->Value,
        DISABLE_MAX_PRIVILEGE,
        (DWORD)groupsToDelete.size(),
        groupsToDelete.data(),
        0,
        NULL,
        0,
        NULL,
        &tHandle))
    {
        return ErrorCode::TraceReturn(
            ErrorCode::FromWin32Error(),
            TraceTaskCodes::Common,
            TraceType_AccessToken,
            "GetSid:CreateRestrictedToken");
    }

    auto restricatedTokenHandle = TokenHandle::CreateSPtr(tHandle);
    restrictedToken = make_shared<AccessToken>(move(restricatedTokenHandle));

    // Now get the default DACL, add usersid and logon sid to DACL and apply updated DACL
    DaclSPtr defaultDacl;
    error = restrictedToken->GetDefaultDacl(defaultDacl);
    if (!error.IsSuccess()) { return error; }

    SidSPtr logonSid;
    error = this->GetLogonSid(logonSid);
    if (!error.IsSuccess()) { return error; }

    SidSPtr userSid;
    error = this->GetSid(TokenUser, userSid);
    if (!error.IsSuccess()) { return error; }

    vector<Dacl::ACEInfo> additionalACEs;

    additionalACEs.push_back(Dacl::ACEInfo(ACCESS_ALLOWED_ACE_TYPE, 0, GENERIC_ALL, logonSid->PSid));
    additionalACEs.push_back(Dacl::ACEInfo(ACCESS_ALLOWED_ACE_TYPE, 0, GENERIC_ALL, userSid->PSid));

    DaclSPtr updatedDacl;
    error = BufferedDacl::CreateSPtr(defaultDacl->PAcl, additionalACEs, updatedDacl);
    if (!error.IsSuccess()) { return error; }

    error = restrictedToken->SetDefaultDacl(updatedDacl);
    return error;
}

ErrorCode AccessToken::CreateRestrictedToken(PSID sidToDeny, __out AccessTokenSPtr & restrictedToken)
{
    SID_AND_ATTRIBUTES disableList[1] = {};

    disableList[0].Sid = sidToDeny;

    HANDLE tokenHandle;
    if (!::CreateRestrictedToken(
        this->tokenHandle_->Value,
        0,
        1,
        disableList,
        0, nullptr,
        0, nullptr,
        &tokenHandle))
    {
        return ErrorCode::TraceReturn(
            ErrorCode::FromWin32Error(),
            TraceTaskCodes::Common,
            TraceType_AccessToken,
            "CreateRestrictedToken failed");
    }
    auto handleSPtr = TokenHandle::CreateSPtr(tokenHandle);

    //duplicate the token to get primary token
    HANDLE token;
    if (!::DuplicateTokenEx(tokenHandle,
        TOKEN_ALL_ACCESS,
        NULL,
        SecurityImpersonation,
        TokenPrimary,
        &token))
    {
        return ErrorCode::FromWin32Error();
    }
    auto restrictedTokenHandle = TokenHandle::CreateSPtr(token);
    restrictedToken = make_shared<AccessToken>(move(restrictedTokenHandle));
    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode AccessToken::GetUserSid(__out SidSPtr & sid)
{
    return GetSid(TokenUser, sid);
}

ErrorCode AccessToken::GetSid(TOKEN_INFORMATION_CLASS const tokenClass, __out SidSPtr & sid)
{
    if (tokenClass != TokenUser &&
        tokenClass != TokenOwner &&
        tokenClass != TokenPrimaryGroup)
    {
        return ErrorCode::FromHResult(E_INVALIDARG);
    }

    ULONG length = 0;
    ::GetTokenInformation(TokenHandle->Value, tokenClass, NULL, 0, &length);
    DWORD win32Error = ::GetLastError();
    if (win32Error != ERROR_INSUFFICIENT_BUFFER)
    {
        return ErrorCode::TraceReturn(
            ErrorCode::FromWin32Error(win32Error),
            TraceTaskCodes::Common,
            TraceType_AccessToken,
            "GetSid:GetTokenInformation-Size");
    }

    ByteBuffer buffer;
    buffer.resize(length, 0);
    if (!::GetTokenInformation(TokenHandle->Value, tokenClass, buffer.data(), length, &length))
    {
        return ErrorCode::TraceReturn(
            ErrorCode::FromWin32Error(),
            TraceTaskCodes::Common,
            TraceType_AccessToken,
            "GetSid:GetTokenInformation");
    }

    PSID pSid = NULL;
    if (tokenClass == TokenUser)
    {
        PTOKEN_USER tokenUser = reinterpret_cast<PTOKEN_USER>(buffer.data());
        pSid = tokenUser->User.Sid;
    }
    else if (tokenClass == TokenOwner)
    {
        PTOKEN_OWNER tokenOwner = reinterpret_cast<PTOKEN_OWNER>(buffer.data());
        pSid = tokenOwner->Owner;
    }
    else if (tokenClass == TokenPrimaryGroup)
    {
        PTOKEN_PRIMARY_GROUP tokenPrimaryGroup = reinterpret_cast<PTOKEN_PRIMARY_GROUP>(buffer.data());
        pSid = tokenPrimaryGroup->PrimaryGroup;
    }

    return BufferedSid::CreateSPtr(pSid, sid);
}

ErrorCode AccessToken::GetLogonSid(__out SidSPtr & sid)
{
    DWORD length;
    ::GetTokenInformation(TokenHandle->Value, ::TokenGroups, NULL, 0, &length);
    DWORD win32Error = ::GetLastError();
    if (win32Error != ERROR_INSUFFICIENT_BUFFER)
    {
        return ErrorCode::TraceReturn(
            ErrorCode::FromWin32Error(win32Error),
            TraceTaskCodes::Common,
            TraceType_AccessToken,
            "GetLogonSid:GetTokenInformation-Size");
    }

    ByteBuffer buffer;
    buffer.resize(length, 0);
    TOKEN_GROUPS *pGroups = reinterpret_cast<TOKEN_GROUPS*>(buffer.data());
    if (::GetTokenInformation(TokenHandle->Value, ::TokenGroups, pGroups, length, &length))
    {
        for (UINT i = 0; i < pGroups->GroupCount; i++)
        {
            if (pGroups->Groups[i].Attributes & SE_GROUP_LOGON_ID)
            {
                PSID pSid = static_cast<PSID>(pGroups->Groups[i].Sid);
                return BufferedSid::CreateSPtr(pSid, sid);
            }
        }

        return ErrorCode(ErrorCodeValue::OperationFailed);
    }
    else
    {
        return ErrorCode::TraceReturn(
            ErrorCode::FromWin32Error(),
            TraceTaskCodes::Common,
            TraceType_AccessToken,
            "GetLogOnSid:GetTokenInformation");
    }
}

ErrorCode AccessToken::GetRestrictedGroups(__out ByteBufferUPtr & tokenGroupsBuffer, __out vector<SID_AND_ATTRIBUTES> & groupsToDelete)
{
    DWORD length;
    ::GetTokenInformation(TokenHandle->Value, ::TokenGroups, NULL, 0, &length);
    DWORD win32Error = ::GetLastError();
    if (win32Error != ERROR_INSUFFICIENT_BUFFER)
    {
        return ErrorCode::TraceReturn(
            ErrorCode::FromWin32Error(win32Error),
            TraceTaskCodes::Common,
            TraceType_AccessToken,
            "GetRestrictedGroups:GetTokenInformation-Size");
    }

    ByteBufferUPtr buffer = make_unique<ByteBuffer>();
    buffer->resize(length);
    PBYTE tokenInformationBuffer = (*buffer).data();
    if (!::GetTokenInformation(TokenHandle->Value, ::TokenGroups, tokenInformationBuffer, length, &length))
    {
        return ErrorCode::TraceReturn(
            ErrorCode::FromWin32Error(),
            TraceTaskCodes::Common,
            TraceType_AccessToken,
            "GetRestrictedGroups:GetTokenInformation");
    }

    PTOKEN_GROUPS originalTokenGroups = reinterpret_cast<PTOKEN_GROUPS>(tokenInformationBuffer);
    for (UINT index = 0; index < originalTokenGroups->GroupCount; index++)
    {
        PSID_AND_ATTRIBUTES tokenGroup = &originalTokenGroups->Groups[index];

        // first, let's look at the identifer authority and then the first RID
        SID const & sid = *(static_cast<SID*>(tokenGroup->Sid));
        BYTE IdAuth = sid.IdentifierAuthority.Value[5];
        bool disable = true;
        switch (IdAuth)
        {
        case 0: // SECURITY_NULL_SID_AUTHORITY
            if (sid.SubAuthority[0] == SECURITY_NULL_RID) //anonymous SID
                disable = false;
            break;

        case 1: // SECURITY_WORLD_SID_AUTHORITY
            if (sid.SubAuthority[0] == SECURITY_WORLD_RID) //everyone SID
                disable = false;
            break;

        case 2: // SECURITY_LOCAL_SID_AUTHORITY
            if (sid.SubAuthority[0] == SECURITY_LOCAL_RID) //local SID
                disable = false;
            break;

        case 3: // SECURITY_CREATOR_SID_AUTHORITY - should not find these
        case 4: // SECURITY_NON_UNIQUE_AUTHORITY, also should not find these
        case 9: // SECURITY_RESOURCE_MANAGER_AUTHORITY
            Assert::CodingError("Sids are not supported. IdAuth={0}", IdAuth);
            break;

        case 5: // SECURITY_NT_AUTHORITY - most common
        {
            // now consider the first RID
            switch (sid.SubAuthority[0])
            {
            case SECURITY_NETWORK_RID: // network
            case SECURITY_AUTHENTICATED_USER_RID: // auth user
                disable = false;
                break;

            case SECURITY_BUILTIN_DOMAIN_RID:
                // built-in
                if (sid.SubAuthorityCount == 2)
                {
                    switch (sid.SubAuthority[1])
                    {
                    case DOMAIN_ALIAS_RID_USERS:
                    case DOMAIN_ALIAS_RID_GUESTS:
                        disable = false;
                        break;
                    default:
                        break;
                    }
                }
                break;

            default:
                disable = true;
                break;
            }
        }
        break;

        default:
            break;
        }

        if (disable)
        {
            groupsToDelete.push_back(*tokenGroup);
        }
    }

    tokenGroupsBuffer = move(buffer);
    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode AccessToken::GetDefaultDacl(__out DaclSPtr & defaultDacl)
{
    DWORD length;
    ::GetTokenInformation(TokenHandle->Value, ::TokenDefaultDacl, NULL, 0, &length);
    DWORD win32Error = ::GetLastError();
    if (win32Error != ERROR_INSUFFICIENT_BUFFER)
    {
        return ErrorCode::TraceReturn(
            ErrorCode::FromWin32Error(win32Error),
            TraceTaskCodes::Common,
            TraceType_AccessToken,
            "GetDefaultDacl:GetTokenInformation-Size");
    }

    ByteBuffer buffer;
    buffer.resize(length, 0);
    if (!::GetTokenInformation(TokenHandle->Value, ::TokenDefaultDacl, buffer.data(), length, &length))
    {
        return ErrorCode::TraceReturn(
            ErrorCode::FromWin32Error(),
            TraceTaskCodes::Common,
            TraceType_AccessToken,
            "GetDefaultDacl:GetTokenInformation");
    }

    return TokenDacl::CreateSPtr(move(buffer), defaultDacl);
}

ErrorCode AccessToken::SetDefaultDacl(__in DaclSPtr const & dacl)
{
    ASSERT_IFNOT((dacl.get() != nullptr && Dacl::IsValid(dacl->PAcl)), "The supplied DACL must be valid.");

    TOKEN_DEFAULT_DACL tokenDefaultAcl;
    tokenDefaultAcl.DefaultDacl = dacl->PAcl;
    if (!::SetTokenInformation(TokenHandle->Value, ::TokenDefaultDacl, &tokenDefaultAcl, sizeof(TOKEN_DEFAULT_DACL)))
    {
        return ErrorCode::TraceReturn(
            ErrorCode::FromWin32Error(),
            TraceTaskCodes::Common,
            TraceType_AccessToken,
            "SetDefaultDacl:SetTokenInformation");
    }

    return ErrorCode(ErrorCode(ErrorCodeValue::Success));
}

ErrorCode AccessToken::CreateSecurityDescriptor(DWORD access, __out SecurityDescriptorSPtr & sd)
{
    static const BYTE inheritAceFlags = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE;

    // get user Sid for user ACE
    SidSPtr userSid;
    auto error = GetSid(TokenUser, userSid);
    if (!error.IsSuccess()) { return error; }


    //TODO: what is the purpose of these?
    // get admin Sid for admin ACE
    vector<DWORD> subauthorities;
    subauthorities.push_back(SECURITY_BUILTIN_DOMAIN_RID);
    subauthorities.push_back(DOMAIN_ALIAS_RID_ADMINS);
    SID_IDENTIFIER_AUTHORITY ntauthority = SECURITY_NT_AUTHORITY;
    SidSPtr adminSid;
    error = BufferedSid::CreateSPtr(ntauthority, subauthorities, adminSid);
    if (!error.IsSuccess()) { return error; }

    // get creator Sid for creator ACE
    subauthorities.clear();
    subauthorities.push_back(SECURITY_CREATOR_OWNER_RID);
    SID_IDENTIFIER_AUTHORITY creatorAuthority = SECURITY_CREATOR_SID_AUTHORITY;
    SidSPtr creatorSid;
    error = BufferedSid::CreateSPtr(creatorAuthority, subauthorities, creatorSid);
    if (!error.IsSuccess()) { return error; }

    vector<Dacl::ACEInfo> aceEntries;
    aceEntries.push_back(Dacl::ACEInfo(ACCESS_ALLOWED_ACE_TYPE, 0, GENERIC_ALL, adminSid->PSid));
    aceEntries.push_back(Dacl::ACEInfo(ACCESS_ALLOWED_ACE_TYPE, 0, GENERIC_ALL, creatorSid->PSid));
    aceEntries.push_back(Dacl::ACEInfo(ACCESS_ALLOWED_ACE_TYPE, 0, access, userSid->PSid));
    aceEntries.push_back(Dacl::ACEInfo(ACCESS_ALLOWED_ACE_TYPE, inheritAceFlags, GENERIC_ALL, adminSid->PSid));
    aceEntries.push_back(Dacl::ACEInfo(ACCESS_ALLOWED_ACE_TYPE, inheritAceFlags, GENERIC_ALL, creatorSid->PSid));
    aceEntries.push_back(Dacl::ACEInfo(ACCESS_ALLOWED_ACE_TYPE, inheritAceFlags, access, userSid->PSid));

    DaclSPtr dacl;
    error = BufferedDacl::CreateSPtr(aceEntries, dacl);
    if (!error.IsSuccess()) { return error; }

    return TokenSecurityDescriptor::CreateSPtr(dacl, sd);
}

ErrorCode AccessToken::UpdateCachedCredentials(wstring const & username, wstring const & domain, wstring const & password)
{
    ErrorCode error(ErrorCodeValue::Success);

    MSV1_0_CHANGEPASSWORD_REQUEST* pLogonInfo;
    PMSV1_0_CHANGEPASSWORD_RESPONSE pResponse = NULL;

    DWORD cbResponse = sizeof(*pResponse);

    size_t cbLogonInfo;
    size_t usernameLen = username.size();
    size_t domainLen = domain.size();
    size_t passwordLen = password.size();

    size_t usernameBufSize = (usernameLen + 1) * sizeof(WCHAR);
    size_t domainBufSize = (domainLen + 1) * sizeof(WCHAR);
    size_t passwordBufSize = (passwordLen + 1) * sizeof(WCHAR);


    cbLogonInfo = sizeof(MSV1_0_CHANGEPASSWORD_REQUEST);
    cbLogonInfo += usernameBufSize;
    cbLogonInfo += domainBufSize;
    cbLogonInfo += passwordBufSize;

    ByteBuffer buffer;
    buffer.resize(cbLogonInfo);

    pLogonInfo = reinterpret_cast<MSV1_0_CHANGEPASSWORD_REQUEST*>(buffer.data());

    memset(pLogonInfo, 0, cbLogonInfo);

    pLogonInfo->MessageType = MsV1_0ChangeCachedPassword;

    // concat on end of struct  
    memcpy_s(
        pLogonInfo + 1,
        cbLogonInfo - 1,
        username.c_str(),
        usernameBufSize);

    memcpy_s(
        (PBYTE)(pLogonInfo + 1) + usernameBufSize,
        cbLogonInfo - (1 + usernameBufSize),
        domain.c_str(),
        domainBufSize);

    memcpy_s((PBYTE)(pLogonInfo + 1) + usernameBufSize + domainBufSize,
        cbLogonInfo - (1 + usernameBufSize + domainBufSize),
        password.c_str(),
        passwordBufSize);



    pLogonInfo->AccountName.Length = (USHORT)usernameLen * sizeof(WCHAR);
    pLogonInfo->AccountName.MaximumLength = (USHORT)usernameLen * sizeof(WCHAR);
    pLogonInfo->AccountName.Buffer = (WCHAR*)(pLogonInfo + 1);

    pLogonInfo->DomainName.Length = (USHORT)domainLen * sizeof(WCHAR);
    pLogonInfo->DomainName.MaximumLength = (USHORT)domainLen * sizeof(WCHAR);
    pLogonInfo->DomainName.Buffer = (WCHAR*)((PBYTE)(pLogonInfo + 1) + usernameBufSize);

    pLogonInfo->NewPassword.Length = (USHORT)passwordLen * sizeof(WCHAR);
    pLogonInfo->NewPassword.MaximumLength = (USHORT)passwordLen * sizeof(WCHAR);
    pLogonInfo->NewPassword.Buffer = (WCHAR*)((PBYTE)(pLogonInfo + 1) + usernameBufSize + domainBufSize);

    pLogonInfo->OldPassword.Length = 0;
    pLogonInfo->OldPassword.MaximumLength = 0;
    pLogonInfo->OldPassword.Buffer = NULL;

    STRING logonName;
    string appName = "WinFabric"; //Logon application name, just used for auditing purposes.   
    LSA_OPERATIONAL_MODE securityModeIgnored = 0;  //This is ignored

    RtlInitString(&logonName, appName.c_str());

    HANDLE hLsa;

    NTSTATUS status = ::LsaRegisterLogonProcess(
        &logonName,
        &hLsa,
        &securityModeIgnored
    );

    if (status != STATUS_SUCCESS)
    {
        error = ErrorCode::FromNtStatus(status);

    }
    else
    {
        STRING packageName;
        string package = MSV1_0_PACKAGE_NAME;
        ULONG authPkgId;

        ::RtlInitString(&packageName, package.c_str());

        status = LsaLookupAuthenticationPackage(
            hLsa,
            &packageName,
            &authPkgId);

        if (status != STATUS_SUCCESS)
        {
            error = ErrorCode::FromNtStatus(status);
        }
        else
        {
            NTSTATUS substatus;

            status = LsaCallAuthenticationPackage(
                hLsa,
                authPkgId,
                pLogonInfo,
                static_cast<ULONG>(cbLogonInfo),
                (PVOID *)&pResponse,
                &cbResponse,
                &substatus);

            if (status != STATUS_SUCCESS ||
                substatus != STATUS_SUCCESS)
            {
                if (status != STATUS_SUCCESS)
                {
                    error = ErrorCode::FromNtStatus(status);
                }
                else
                {
                    error = ErrorCode::FromNtStatus(status);
                }
            }

            RtlSecureZeroMemory(pLogonInfo->NewPassword.Buffer,
                pLogonInfo->NewPassword.MaximumLength);

            //Free the LSA result.
            if (pResponse)
            {
                LsaFreeReturnBuffer(pResponse);
            }
        }
    }
    if (hLsa)
    {
        status = ::LsaDeregisterLogonProcess(hLsa);
        if (status != STATUS_SUCCESS)
        {
            TraceError(TraceTaskCodes::Common,
                TraceType_AccessToken,
                "LsaDeregisterLogonProcess failed. Returned error {0}",
                ErrorCode::FromNtStatus(status));

        }
    }
    return error;
}

ErrorCode AccessToken::Impersonate(__out ImpersonationContextUPtr & impersonationContext)
{
    impersonationContext = make_unique<ImpersonationContext>(this->TokenHandle);
    return impersonationContext->ImpersonateLoggedOnUser();
}
#endif

