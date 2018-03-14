// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace ServiceModel;

StringLiteral const TraceSecurityGroup("SecurityGroup");

int const SecurityGroup::GroupNameLength = 15;
GlobalWString SecurityGroup::GroupNamePrefix = make_global<wstring>(L"WF-");

SecurityGroup::SecurityGroup(
    wstring const & applicationId, 
    SecurityGroupDescription const & groupDescription)
    : groupDescription_(groupDescription),
    applicationId_(applicationId),
    groupName_(),
    sid_()
{
}

SecurityGroup::~SecurityGroup()
{
}

ErrorCode SecurityGroup::CreateAccountwithRandomName(wstring const& comment)
{
    bool done = false;
    ErrorCode error;
    wstring groupNameTemp;
    while(!done)
    {
        done = true;
        // Create random account name
        wstring randomString;
        StringUtility::GenerateRandomString(SecurityGroup::GroupNameLength, randomString);
        groupNameTemp = (wstring)SecurityGroup::GroupNamePrefix;
        groupNameTemp.append(randomString);
        error = SecurityPrincipalHelper::CreateGroupAccount(groupNameTemp, comment);
        if (!error.IsSuccess())
        {
            if(error.IsError(ErrorCodeValue::AlreadyExists))
            {
                WriteWarning(
                    TraceSecurityGroup,
                    "{0}:Group with name {1} already exists. Retrying.",
                    applicationId_,
                    groupNameTemp);
                done = false;
            }
            else
            {
                return error;
            }

            groupNameTemp.clear();
        }
    }

    groupName_ = groupNameTemp;
    error = BufferedSid::CreateUPtr(AccountName, sid_);
    if (!error.IsSuccess())
    {
        return error;
    }

    return SetupMembershipForNewGroup();
}

ErrorCode SecurityGroup::CreateAccountwithName(wstring const& accountName, wstring const& comment)
{
    ErrorCode error = SecurityPrincipalHelper::CreateGroupAccount(accountName, comment);
    ASSERT_IF(
        error.IsError(ErrorCodeValue::AlreadyExists), 
        "{0}:CreateGroupAccount in CreateAccountwithName for name {1} cannot fail with AlreadyExists",
        applicationId_,
        accountName);

    if(!error.IsSuccess())
    {
        return error;
    }

    groupName_ = accountName;
    error = BufferedSid::CreateUPtr(AccountName, sid_);
    if (!error.IsSuccess())
    {
        return error;
    }

    return SetupMembershipForNewGroup();
}

Common::ErrorCode SecurityGroup::SetupMembershipForNewGroup()
{
    // Create the desired membership
    vector<wstring> & domainUsers = groupDescription_.DomainUserMembers;
    for (auto it = domainUsers.begin(); it != domainUsers.end(); ++it)
    {
        // Add the user to the newly created group
        auto error = SecurityPrincipalHelper::AddMemberToLocalGroup(
            AccountName /*parentGroup*/, 
            *it /*memberToAdd*/);
        if (!error.IsSuccess())
        {
            return error;
        }
    }

    vector<wstring> & domainGroups = groupDescription_.DomainGroupMembers;
    for (auto it = domainGroups.begin(); it != domainGroups.end(); ++it)
    {
        auto error = SecurityPrincipalHelper::AddMemberToLocalGroup(
            AccountName /*parentGroup*/,
            *it /*memberToAdd*/);
        if (!error.IsSuccess())
        {
            return error;
        }
    }

#if !defined(PLATFORM_UNIX)
    vector<wstring> & systemGroups = groupDescription_.SystemGroupMembers;
    for (auto it = systemGroups.begin(); it != systemGroups.end(); ++it)
    {
        auto error = SecurityPrincipalHelper::AddMemberToLocalGroup(
            *it  /*parentGroup*/,
            AccountName /*memberToAdd*/);
        if (!error.IsSuccess())
        {
            return error;
        }
    }
#endif

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode SecurityGroup::DeleteAccount()
{
    return SecurityPrincipalHelper::DeleteGroupAccount(AccountName);
}

// Does not actually load the account but just validates the memebership
ErrorCode SecurityGroup::LoadAccount(std::wstring const& accountName)
{
    vector<wstring> membership;
    auto error = SecurityPrincipalHelper::GetMembers(accountName, membership);
    if (!error.IsSuccess())
    {
        return error;
    }

    // Check membership
    vector<wstring> & domainUsers = groupDescription_.DomainUserMembers;
    for (auto it = domainUsers.begin(); it != domainUsers.end(); ++it)
    {
        auto itAppGroup = find_if(membership.begin(), membership.end(), FindCaseInsensitive<wstring>(*it));
        if (itAppGroup == membership.end())
        {
//LINUXTODO
#if !defined(PLATFORM_UNIX)        
             WriteWarning(
                TraceSecurityGroup,
                "{0}: LoadExistingGroup for {1} does not have member {2}. Actual {3}",
                applicationId_,
                accountName,
                *it,
                SecurityPrincipalHelper::GetMembershipString(membership));
#endif
             return ErrorCode(ErrorCodeValue::NotFound);
        }
    }

    vector<wstring> & domainGroups = groupDescription_.DomainGroupMembers;
    for (auto it = domainGroups.begin(); it != domainGroups.end(); ++it)
    {
        auto itAppGroup = find_if(membership.begin(), membership.end(), FindCaseInsensitive<wstring>(*it));
        if (itAppGroup == membership.end())
        {
//LINUXTODO
#if !defined(PLATFORM_UNIX)           
             WriteWarning(
                TraceSecurityGroup,
                "{0}: LoadExistingGroup for {1} doesn not have member {2}. Actual {3}",
                applicationId_,
                accountName,
                *it,
                SecurityPrincipalHelper::GetMembershipString(membership));
#endif
             return ErrorCode(ErrorCodeValue::NotFound);
        }
    }

    vector<wstring> & systemGroups = groupDescription_.SystemGroupMembers;
    for (auto it = systemGroups.begin(); it != systemGroups.end(); ++it)
    {
        vector<wstring> sysGroupMembers;
         error = SecurityPrincipalHelper::GetMembers(*it, sysGroupMembers);
        if (!error.IsSuccess())
        {
            return error;
        }
        auto itSysGroup = find_if(sysGroupMembers.begin(), sysGroupMembers.end(), FindCaseInsensitive<wstring>(accountName));
        if (itSysGroup == sysGroupMembers.end())
        {
//LINUXTODO
#if !defined(PLATFORM_UNIX)   
             WriteWarning(
                TraceSecurityGroup,
                "{0}: LoadExistingGroup for {1} doesn not have member {2}. Actual {3}",
                applicationId_,
                *it,
                accountName,
                SecurityPrincipalHelper::GetMembershipString(sysGroupMembers));
#endif
             return ErrorCode(ErrorCodeValue::NotFound);
        }
    }

    error = BufferedSid::CreateUPtr(accountName, sid_);
    if (!error.IsSuccess())
    {
        return error;
    }

    groupName_ = accountName;
    return ErrorCode(ErrorCodeValue::Success);
}

void SecurityGroup::UnloadAccount()
{
    // Do nothing since the account is not actually loaded
}

PSID const SecurityGroup::TryGetSid() const
{
    if (sid_)
    {
        return sid_->PSid;
    }

    return NULL;
}
