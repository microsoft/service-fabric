// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#if defined(PLATFORM_UNIX)
#include <pwd.h>
#endif

using namespace std;
using namespace Common;
using namespace Hosting2;

StringLiteral const TraceSource("ActivatorRequestor");

ActivatorRequestor::ActivatorRequestor(
    ProcessActivationManagerHolder const & activatorHolder,
    FabricHostClientProcessTerminationCallback const & terminationCallback,
    ActivatorRequestorType::Enum requestorType,
    std::wstring const & requestorId,
    std::wstring const & callbackAddress,
    DWORD processId,
    BOOL obtainToken,
    uint64 nodeInstanceId)
    : activatorHolder_(activatorHolder)
    , terminationCallback_(terminationCallback)
    , requestorType_(requestorType)
    , requestorId_(requestorId)
    , callbackAddress_(callbackAddress)
    , processId_(processId)
    , obtainToken_(obtainToken)
    , processMonitor_()
    , accessTokenRestricted_()
    , accessTokenUnrestricted_()
    , nodeInstanceId_(nodeInstanceId)
{
}

ActivatorRequestor::~ActivatorRequestor()
{
}

ErrorCode ActivatorRequestor::GetDefaultSecurityUser(__out SecurityUserSPtr & secUser, bool isWinFabAdminGroupMember)
{
    AccessTokenSPtr accessToken;
    if (isWinFabAdminGroupMember)
    {
        accessToken = accessTokenUnrestricted_;
    }
    else
    {
        accessToken = accessTokenRestricted_;
    }

    if(accessToken)
    {
#if !defined(PLATFORM_UNIX)
        AccessTokenSPtr dupToken;
        auto error = accessToken->DuplicateToken(TOKEN_ALL_ACCESS, dupToken);
        if(!error.IsSuccess())
        {
            return error;
        }
        SecurityUserSPtr tokenUser = make_shared<TokenSecurityAccount>(move(dupToken));
        secUser = move(tokenUser);
#else
        ServiceModel::SecurityPrincipalAccountType::Enum runasAccountType = ServiceModel::SecurityPrincipalAccountType::LocalUser;
        vector<wstring> runasGroupMembership;
        if (isWinFabAdminGroupMember)
        {
            runasGroupMembership.push_back(FabricConstants::WindowsFabricAdministratorsGroupName);
        }
        uid_t uid = accessToken->Uid;
        struct passwd *pw = getpwuid(uid);
        wstring runasAccount;
        StringUtility::Utf8ToUtf16(pw->pw_name, runasAccount);

        secUser = SecurityUser::CreateSecurityUser(
                L"",
                runasAccount,
                runasAccount,
                L"",
                false,
                false,
                false,
                ServiceModel::NTLMAuthenticationPolicyDescription(),
                runasAccountType,
                runasGroupMembership);

        secUser->ConfigureAccount();
#endif
    }
    return ErrorCode(ErrorCodeValue::Success);
}

#if defined(PLATFORM_UNIX)
ErrorCode ActivatorRequestor::GetDefaultAppSecurityUser(__out SecurityUserSPtr & secUser)
{
    vector<wstring> runasGroupMembership;
    secUser = SecurityUser::CreateSecurityUser(
            L"",
            Constants::AppRunAsAccount,
            Constants::AppRunAsAccount,
            L"",
            false,
            false,
            false,
            ServiceModel::NTLMAuthenticationPolicyDescription(),
            ServiceModel::SecurityPrincipalAccountType::LocalUser,
            runasGroupMembership);

    secUser->ConfigureAccount();
    return ErrorCode(ErrorCodeValue::Success);
}
#endif

ErrorCode ActivatorRequestor::OnOpen()
{
    HandleUPtr appHostProcessHandle;
    DWORD desiredAccess = SYNCHRONIZE;
    if(this->obtainToken_)
    {
        desiredAccess |= PROCESS_QUERY_INFORMATION;
    }
    auto err = ProcessUtility::OpenProcess(
        desiredAccess, 
        FALSE, 
        processId_, 
        appHostProcessHandle);
    if(!err.IsSuccess())
    {
        WriteWarning(
            TraceSource,
            "ProcessActivationManager: OpenProcess: ErrorCode={0}, ParentId={1}",
            err,
            processId_);
        return err;
    }

    AccessTokenSPtr requestorTokenRestricted;
    AccessTokenSPtr requestorTokenUnrestricted;
    if(this->obtainToken_)
    {
        err = this->GetRequestorUserToken(appHostProcessHandle, requestorTokenRestricted, requestorTokenUnrestricted);
        if(!err.IsSuccess())
        {
            return err;
        }
    }

    ProcessActivationManager & controller  = ActivationManager;

    auto root = controller.Root.CreateComponentRoot();
    DWORD processId = this->processId_;
    wstring requestorId = this->requestorId_;
    uint64 nodeInstanceId = this->nodeInstanceId_;

    auto hostMonitor = ProcessWait::CreateAndStart(
        move(*appHostProcessHandle),
        processId,
        [root, this, requestorId, nodeInstanceId](pid_t processId, ErrorCode const & waitResult, DWORD)
        {
            terminationCallback_(processId, requestorId, waitResult, nodeInstanceId);
        });

    this->accessTokenRestricted_ = move(requestorTokenRestricted);
    this->accessTokenUnrestricted_ = move(requestorTokenUnrestricted);

    this->processMonitor_ = move(hostMonitor);

    if (requestorType_ == ActivatorRequestorType::FabricNode)
    {
        if (HostingConfig::GetConfig().RunAsPolicyEnabled)
        {
            ApplicationPrincipals::CleanupEnvironment(requestorId);
        }

        //cleanup port acls from previous process if needed.
        controller.CleanupPortAclsForNode(requestorId_);
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode ActivatorRequestor::OnClose()
{
    return ErrorCode(ErrorCodeValue::Success);
}

void ActivatorRequestor::OnAbort()
{
}

ErrorCode ActivatorRequestor::GetRequestorUserToken(
    HandleUPtr const & appHostProcessHandle,
    __out AccessTokenSPtr & requestorTokenRestricted,
    __out AccessTokenSPtr & requestorTokenUnrestricted)
{
    if(this->obtainToken_)
    {
        auto err = SecurityUtility::EnsurePrivilege(SE_DEBUG_NAME);
        if(!err.IsSuccess())
        {
            TraceWarning(
                TraceTaskCodes::Hosting,
                TraceSource,
                "Ensure privilege check failed. ErrorCode={0}",
                err);
            return err;
        }
        err = AccessToken::CreateProcessToken(appHostProcessHandle->Value, READ_CONTROL|WRITE_DAC|TOKEN_DUPLICATE, requestorTokenUnrestricted);
        if(!err.IsSuccess())
        {
            WriteWarning(
                TraceSource,
                "ActivatorRequestor: Failed to retrieve process token: ErrorCode={0}, RequestorId={1}",
                err,
                requestorId_);
            return err;
        }

#if !defined(PLATFORM_UNIX)
        SidSPtr fabAdminSid;
        err = BufferedSid::CreateSPtr(FabricConstants::WindowsFabricAdministratorsGroupName, fabAdminSid);
        if(!err.IsSuccess())
        {
            WriteWarning(
                TraceSource,
                "ActivatorRequestor: Failed to retrieve Sid for FabricAdministratorsGroup: ErrorCode={0}",
                err);
            return err;
        }

        err = requestorTokenUnrestricted->CreateRestrictedToken(fabAdminSid->PSid, requestorTokenRestricted);
        if(!err.IsSuccess())
        {
            WriteWarning(
                TraceSource,
                "ActivatorRequestor: Failed to duplicate process token: ErrorCode={0}, RequestorId={1}",
                err,
                requestorId_);
            return err;
        }
#else
        //on Linux these two are the same
        requestorTokenRestricted = requestorTokenUnrestricted;
#endif
    }
    return ErrorCode(ErrorCodeValue::Success);
}
