// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace ServiceModel;

StringLiteral const TraceServiceAccount("ServiceAccount");

ServiceAccount::ServiceAccount(
    wstring const& applicationId, 
    SecurityUserDescription const & userDescription)
    : SecurityUser(applicationId, userDescription),
    serviceAccountName_(),
    domain_()
{
    auto error = AccountHelper::GetServiceAccountName(accountName_, serviceAccountName_, domain_, accountName_);
        if(!error.IsSuccess())
    {
        WriteWarning(TraceServiceAccount,
            "AccountName specified is invalid",
            accountName_
            );
    }
    password_ = SERVICE_ACCOUNT_PASSWORD;
}

ServiceAccount::ServiceAccount(
    wstring const & applicationId,
    wstring const & name,
    wstring const & accountName,
    wstring const & password,
    bool isPasswordEncrypted,
    bool loadProfile,
    SecurityPrincipalAccountType::Enum accountType,
    vector<wstring> parentApplicationGroups,
    vector<wstring> parentSystemGroups) : SecurityUser(applicationId, name, accountName, password, isPasswordEncrypted, accountType, parentApplicationGroups, parentSystemGroups, loadProfile)
{
    auto error = AccountHelper::GetServiceAccountName(accountName_, serviceAccountName_, domain_, accountName_);
    if(!error.IsSuccess())
    {
        WriteWarning(TraceServiceAccount,
            "AccountName specified is invalid",
            accountName_
            );
    }
    password_ = SERVICE_ACCOUNT_PASSWORD;
}

ServiceAccount::~ServiceAccount()
{
}

void ServiceAccount::SetupServiceAccount()
{
#ifdef PLATFORM_UNIX
    Assert::CodingError("not implemented");
#else
    wstring simpleAccountName = serviceAccountName_;

    StringUtility::Trim<wstring>(simpleAccountName, L"$");

    LPWSTR acctName = const_cast<LPWSTR>(simpleAccountName.c_str());

    BOOL accountExists = false;

    NTSTATUS status = ::NetIsServiceAccount(NULL, acctName, &accountExists);
    if(status == STATUS_SUCCESS)
    {
        if(!accountExists)
        {
            status = ::NetAddServiceAccount(NULL, acctName, NULL, SERVICE_ACCOUNT_FLAG_LINK_TO_HOST_ONLY);
            if(status!= STATUS_SUCCESS)
            {
                WriteWarning(TraceServiceAccount,
                    "NetAddServiceAccount failed for account {0} with error {1}. Continue with Logon in case caller does not have permission to import the account",
                    AccountName,
                    status);
            }
        }
    }
#endif
}

ErrorCode ServiceAccount::LoadAccount(wstring const& accountName)
{
    SetupServiceAccount();

    auto error = SecurityUser::LoadAccount(accountName);
    if(!error.IsSuccess())
    {
        WriteWarning(TraceServiceAccount,
            "LoadAccount failed for account {0} with error {1}.",
            AccountName,
            error);
    }
    return error;
}

ErrorCode ServiceAccount::CreateLogonToken(__out AccessTokenSPtr & userToken)
{
    AcquireExclusiveLock grab(userLock_);
    if (!userToken_)
    {
        // Create the user token
        WriteInfo(
            TraceServiceAccount,
            "{0}: Creating user token: Account={1}, AccountType {2}",
            applicationId_,
            AccountName,
            AccountType);

        auto error = AccessToken::CreateServiceAccountToken(serviceAccountName_, domain_, password_, this->LoadProfile, this->SidToAdd, userToken_);

        if (!error.IsSuccess())
        {
            WriteTrace(
                error.ToLogLevel(),
                TraceServiceAccount,
                "{0}: Create user token: Account={1}, AccountType {2} error = {3}",
                applicationId_,
                AccountName,
                AccountType,
                error);
            return error;
        }
    }

    password_.clear();

    userToken = userToken_;
    return ErrorCode(ErrorCodeValue::Success);
}
