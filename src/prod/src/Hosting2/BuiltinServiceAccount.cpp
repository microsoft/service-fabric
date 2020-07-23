// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace ServiceModel;

StringLiteral const TraceBuiltinServiceAccount("BuiltinServiceAccount");

BuiltinServiceAccount::BuiltinServiceAccount(
    wstring const& applicationId, 
    SecurityUserDescription const & userDescription)
    : SecurityUser(applicationId, userDescription),
    configured_(false)
{
    accountName_ = userDescription.AccountName;
}

BuiltinServiceAccount::BuiltinServiceAccount(
    wstring const & applicationId,
    wstring const & name,
    wstring const & accountName,
    wstring const & password,
    bool isPasswordEncrypted,
    bool loadProfile,
    SecurityPrincipalAccountType::Enum accountType,
    vector<wstring> parentApplicationGroups,
    vector<wstring> parentSystemGroups)
    : SecurityUser(
        applicationId,
        name,
        accountName,
        password,
        isPasswordEncrypted,
        accountType,
        parentApplicationGroups,
        parentSystemGroups,
        loadProfile),
    configured_(false)
{
}

BuiltinServiceAccount::~BuiltinServiceAccount()
{
}

ErrorCode BuiltinServiceAccount::LoadAccount(wstring const& accountName)
{
    UNREFERENCED_PARAMETER(accountName);
    if(sid_)
    {
        return ConfigureAccount();
    }
    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode BuiltinServiceAccount::ConfigureAccount()
{
    ErrorCode error(ErrorCodeValue::Success);
    AcquireExclusiveLock grab(userLock_);
    if(!this->configured_)
    {
        WELL_KNOWN_SID_TYPE sidType = WinNetworkServiceSid;

        if(this->AccountType == SecurityPrincipalAccountType::LocalService)
        {
            sidType = WinLocalServiceSid;
        }
        else if(this->AccountType == SecurityPrincipalAccountType::LocalSystem)
        {
            sidType = WinLocalSystemSid;
        }
        error = BufferedSid::CreateSPtr(sidType, sid_);

        if(!error.IsSuccess())
        {
            WriteTrace(error.ToLogLevel(),
                TraceBuiltinServiceAccount,
                "Failed to create sid for builtin service {0} . Error {1}",
                this->AccountType,
                error);
            return error;
        }

        error = Sid::LookupAccount(sid_->PSid, domain_, accountName_);

        WriteTrace(error.ToLogLevel(),
            TraceBuiltinServiceAccount,
            "Failed to lookup account by sid for builtin service {0}, Name {1}. Error {2}",
            this->AccountType,
            this->Name,
            error);
        if(error.IsSuccess())
        {
            error = SecurityUser::SetupGroupMembershipForNewUser();
        }
        if(error.IsSuccess())
        {
            this->configured_ = true;
        }
    }
    return error;
}

ErrorCode BuiltinServiceAccount::CreateLogonToken(__out AccessTokenSPtr & userToken)
{
    AcquireExclusiveLock grab(userLock_);
    ErrorCode error;

    if (!userToken_)
    {
        // Create the user token
        WriteInfo(
            TraceBuiltinServiceAccount,
            "{0}: Creating user token: Account={1}, AccountType {2}",
            applicationId_,
            AccountName,
            AccountType);

        error = AccessToken::CreateServiceAccountToken(accountName_, domain_, password_, this->LoadProfile, this->SidToAdd, userToken_);

        if (!error.IsSuccess())
        {
            WriteTrace(
                error.ToLogLevel(),
                TraceBuiltinServiceAccount,
                "{0}: Create user token: Account={1}, AccountType {2} error = {3}",
                applicationId_,
                AccountName,
                AccountType,
                error);
            return error;
        }
    }

    userToken = userToken_;
    return ErrorCode(ErrorCodeValue::Success);
}
