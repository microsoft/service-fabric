// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace ServiceModel;

StringLiteral const TraceSecurityUser("SecurityUser");

SecurityUserSPtr SecurityUser::CreateSecurityUser(
    wstring const & applicationId,
    SecurityUserDescription const & userDescription)
{
    SecurityUserSPtr securityUser;
    if(userDescription.AccountType == SecurityPrincipalAccountType::LocalUser)
    {
        securityUser = make_shared<LocalUser>(applicationId, userDescription);
    }
    else if(userDescription.AccountType == SecurityPrincipalAccountType::DomainUser)
    {
        securityUser = make_shared<DomainUser>(applicationId, userDescription);
    }
    else if(userDescription.AccountType == SecurityPrincipalAccountType::ManagedServiceAccount)
    {
        securityUser = make_shared<ServiceAccount>(applicationId, userDescription);
    }
    else if(userDescription.AccountType == SecurityPrincipalAccountType::LocalService ||
        userDescription.AccountType == SecurityPrincipalAccountType::NetworkService ||
        userDescription.AccountType == SecurityPrincipalAccountType::LocalSystem)
    {
        securityUser = make_shared<BuiltinServiceAccount>(applicationId, userDescription);
    }
    else
    {
        Assert::CodingError("Invalid accounttype {0}", userDescription.AccountType);
    }
    return securityUser;
}

SecurityUserSPtr SecurityUser::CreateSecurityUser(
    wstring const & applicationId,
    wstring const & name,
    wstring const & accountName,
    wstring const & password,
    bool isPasswordEncrypted,
    bool loadProfile,
    bool performInteractiveLogon,
    NTLMAuthenticationPolicyDescription const & ntlmAuthenticationPolicy,
    SecurityPrincipalAccountType::Enum accountType,
    vector<wstring> parentApplicationGroups,
    vector<wstring> parentSystemGroups)
{
    SecurityUserSPtr securityUser;
    if(accountType == SecurityPrincipalAccountType::LocalUser)
    {
        securityUser = make_shared<LocalUser>(
            applicationId, 
            name, 
            accountName, 
            password, 
            isPasswordEncrypted, 
            loadProfile, 
            performInteractiveLogon, 
            ntlmAuthenticationPolicy, 
            parentApplicationGroups, 
            parentSystemGroups);
    }
    else if(accountType == SecurityPrincipalAccountType::DomainUser)
    {
        securityUser = make_shared<DomainUser>(applicationId, name, accountName, password, isPasswordEncrypted, loadProfile, performInteractiveLogon, accountType, parentApplicationGroups, parentSystemGroups);
    }
    else if(accountType == SecurityPrincipalAccountType::ManagedServiceAccount)
    {
        securityUser = make_shared<ServiceAccount>(applicationId, name, accountName, password, isPasswordEncrypted, loadProfile, accountType, parentApplicationGroups, parentSystemGroups);
    }
    else if(accountType == SecurityPrincipalAccountType::LocalService ||
        accountType == SecurityPrincipalAccountType::NetworkService)
    {
        securityUser = make_shared<BuiltinServiceAccount>(applicationId, name, accountName, password, isPasswordEncrypted, loadProfile, accountType, parentApplicationGroups, parentSystemGroups);
    }
    else
    {
        Assert::CodingError("Invalid accounttype {0}", accountType);
    }
    return securityUser;
}

SecurityUser::SecurityUser(
    wstring const & applicationId,
    SecurityUserDescription const & userDescription)
    : applicationId_(applicationId),
    name_(userDescription.Name),
    accountName_(userDescription.AccountName),
    password_(userDescription.Password),
    isPasswordEncrypted_(userDescription.IsPasswordEncrypted),
    loadProfile_(userDescription.LoadProfile),
    performInteractiveLogon_(userDescription.PerformInteractiveLogon),
    ntlmAuthenticationDesc_(userDescription.NTLMAuthenticationPolicy),
    accountType_(userDescription.AccountType),
    parentApplicationGroups_(userDescription.ParentApplicationGroups),
    parentSystemGroups_(userDescription.ParentSystemGroups),
    sid_(),
    userToken_(),
    sidToAdd_()
{
}

SecurityUser::SecurityUser(
    wstring const & applicationId,
    wstring const & name,
    wstring const & accountName,
    wstring const & password,
    bool isPasswordEncrypted,
    SecurityPrincipalAccountType::Enum accountType,
    vector<wstring> parentApplicationGroups,
    vector<wstring> parentSystemGroups,
    bool loadProfile,
    bool performInteractiveLogon)
    : applicationId_(applicationId),
    name_(name),
    accountName_(accountName),
    password_(password),
    isPasswordEncrypted_(isPasswordEncrypted),
    loadProfile_(loadProfile),
    performInteractiveLogon_(performInteractiveLogon),
    accountType_(accountType),
    parentApplicationGroups_(parentApplicationGroups),
    parentSystemGroups_(parentSystemGroups),
    sidToAdd_()
{

}

SecurityUser::SecurityUser(
    wstring const & applicationId,
    wstring const & name,
    wstring const & accountName,
    wstring const & password,
    bool isPasswordEncrypted,
    NTLMAuthenticationPolicyDescription const & ntlmAuthenticationPolicy,
    SecurityPrincipalAccountType::Enum accountType,
    vector<wstring> parentApplicationGroups,
    vector<wstring> parentSystemGroups,
    bool loadProfile,
    bool performInteractiveLogon)
    : applicationId_(applicationId),
    name_(name),
    accountName_(accountName),
    password_(password),
    isPasswordEncrypted_(isPasswordEncrypted),
    loadProfile_(loadProfile),
    performInteractiveLogon_(performInteractiveLogon),
    ntlmAuthenticationDesc_(ntlmAuthenticationPolicy),
    accountType_(accountType),
    parentApplicationGroups_(parentApplicationGroups),
    parentSystemGroups_(parentSystemGroups),
    sidToAdd_()
{
}

SecurityUser::~SecurityUser()
{
}

ErrorCode SecurityUser::SetupGroupMembershipForNewUser()
{
    ErrorCode error = SecurityPrincipalHelper::AddMemberToLocalGroup(FabricConstants::WindowsFabricAllowedUsersGroupName, AccountName);

    ASSERT_IF(error.IsError(ErrorCodeValue::AlreadyExists) && 
        AccountType == SecurityPrincipalAccountType::LocalUser,
        "AddMemberToLocalGroup cannot return AlreadyExists after a new account was created");

    if (!error.IsSuccess() &&
        !error.IsError(ErrorCodeValue::AlreadyExists))
    {
        return error;
    }

    error = BufferedSid::CreateSPtr(AccountName, sid_);
    if (!error.IsSuccess())
    {
        return error;
    }

    // Add the user to the desired groups
    for (auto it = parentApplicationGroups_.begin(); it != parentApplicationGroups_.end(); ++it)
    {
        // The group should have been created previously
        // by the app environment manager
        error = SecurityPrincipalHelper::AddMemberToLocalGroup(
            *it /*parentGroup*/, 
            AccountName /*memberToAdd*/);
        if (!error.IsSuccess() &&
            !error.IsError(ErrorCodeValue::AlreadyExists))
        {
            return error;
        }
    }

    for (auto it = parentSystemGroups_.begin(); it != parentSystemGroups_.end(); ++it)
    {
        if(AccountHelper::GroupAllowsMemberAddition(*it))
        {
            error = SecurityPrincipalHelper::AddMemberToLocalGroup(
                *it /*parentGroup*/, 
                AccountName /*memberToAdd*/);
            if (!error.IsSuccess() &&
                !error.IsError(ErrorCodeValue::AlreadyExists))
            {
                return error;
            }
        }
        else
        {
            WriteInfo(
                TraceSecurityUser,
                "Skip adding account {0} to group {1} since members cannot be added",
                AccountName,
                *it);
            error = BufferedSid::CreateSPtr(WinLocalSid, sidToAdd_);
            if(!error.IsSuccess())
            {
                return error;
            }
        }
    }
    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode SecurityUser::ConfigureAccount()
{
    return SetupGroupMembershipForNewUser();
}

ErrorCode SecurityUser::LoadAccount(wstring const& accountName)
{
    AcquireExclusiveLock grab(userLock_);
#if !defined(PLATFORM_UNIX)
    // Check that the account exists and has desired membership
    vector<wstring> memberOf;
    auto error = SecurityPrincipalHelper::GetMembership(accountName, memberOf);
    if (!error.IsSuccess())
    {
        return error;
    }

    for (auto it = parentApplicationGroups_.begin(); it != parentApplicationGroups_.end(); ++it)
    {
        wstring const& groupName = *it;
        auto itAppGroup = find_if(memberOf.begin(), memberOf.end(), FindCaseInsensitive<wstring>(groupName));
        if (itAppGroup == memberOf.end())
        {
            WriteWarning(
                TraceSecurityUser,
                "{0}:RetrieveAccount for account {1} : is not member of {2}. Actual {3}",
                applicationId_,
                accountName,
                groupName,
                SecurityPrincipalHelper::GetMembershipString(memberOf));

            return ErrorCode(ErrorCodeValue::NotFound);
        }
    }

    for (auto it = parentSystemGroups_.begin(); it != parentSystemGroups_.end(); ++it)
    {
        if(AccountHelper::GroupAllowsMemberAddition(*it))
        {
            auto itAppGroup = find_if(memberOf.begin(), memberOf.end(), FindCaseInsensitive<wstring>(*it));
            if (itAppGroup == memberOf.end())
            {
                WriteWarning(
                    TraceSecurityUser,
                    "{0}:RetrieveAccount for account {1} : is not member of {2}. Actual {3}",
                    applicationId_,
                    accountName,
                    *it,
                    SecurityPrincipalHelper::GetMembershipString(memberOf));

                return ErrorCode(ErrorCodeValue::NotFound);
            }
        }
        else
        {
            WriteInfo(
                TraceSecurityUser,
                "Ignoring check for group {0} since members cannot be added",
                *it);
        }
    }
#else
    auto
#endif

    error = BufferedSid::CreateSPtr(accountName, sid_);
    if (!error.IsSuccess())
    {
        return error;
    }

    accountName_ = accountName;
    return ErrorCode(ErrorCodeValue::Success);
}

void SecurityUser::UnloadAccount()
{
    AcquireExclusiveLock grab(userLock_);
    // Reset the user token, which will let go of the profile handle.
    // If the handle is used by someone else, the profile won't be unloaded, 
    // so the user won't be able to be removed. 
    userToken_.reset();
}

bool SecurityUser::IsPasswordUpdated(SecurityUserSPtr const & other)
{
    bool needUpdate = false;
    if(this->AccountType == SecurityPrincipalAccountType::DomainUser)
    {
        if(other)
        {
            wstring originalPswd = password_;
            wstring currentPswd = other->Password;

            if(this->isPasswordEncrypted_)
            {
                SecureString pswd;
                CryptoUtility::DecryptText(password_, pswd);
                originalPswd = pswd.GetPlaintext();
            }
            if(other->isPasswordEncrypted_)
            {
                SecureString pswd;
                CryptoUtility::DecryptText(other->password_, pswd);
                currentPswd = pswd.GetPlaintext();
            }

            needUpdate = (originalPswd != currentPswd);

            originalPswd.clear();
            currentPswd.clear();
        }
    }
    return needUpdate;
}
