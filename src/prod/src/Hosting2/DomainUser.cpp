// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace ServiceModel;

StringLiteral const TraceDomainUser("DomainUser");

DomainUser::DomainUser(
    wstring const& applicationId, 
    SecurityUserDescription const & userDescription)
    : SecurityUser(applicationId, userDescription),
    username_(),
    domain_()
{
    auto error = AccountHelper::GetDomainUserAccountName(accountName_, username_, domain_, accountName_);    
    if(!error.IsSuccess())
    {
        WriteWarning(TraceDomainUser,
            "AccountName specified is invalid",
            accountName_
            );
    }
}

DomainUser::DomainUser(
    wstring const & applicationId,
    wstring const & name,
    wstring const & accountName,
    wstring const & password,
    bool isPasswordEncrypted,
    bool loadProfile,
    bool performInteractiveLogon,
    SecurityPrincipalAccountType::Enum accountType,
    vector<wstring> parentApplicationGroups,
    vector<wstring> parentSystemGroups) 
    : SecurityUser
    (
        applicationId,
        name,
        accountName,
        password,
        isPasswordEncrypted,
        accountType,
        parentApplicationGroups,
        parentSystemGroups,
        loadProfile,
        performInteractiveLogon
        ),
    domain_()
{
    auto error = AccountHelper::GetDomainUserAccountName(accountName_, username_, domain_, accountName_);    
    if(!error.IsSuccess())
    {
        WriteWarning(TraceDomainUser,
            "AccountName specified is invalid",
            accountName_
            );
    }
}

DomainUser::~DomainUser()
{
}

ErrorCode DomainUser::CreateLogonToken(__out AccessTokenSPtr & userToken)
{
//LINUXTODO
#if defined(PLATFORM_UNIX)
    userToken = make_shared<AccessToken>();
    return ErrorCode::Success();
#else       
    AcquireExclusiveLock grab(userLock_);
    if (!userToken_)
    {
        // Create the user token
        WriteInfo(
            TraceDomainUser,
            "{0}: Creating user token: Account={1}, AccountType {2}",
            applicationId_,
            AccountName,
            AccountType);

        SecureString decryptedPassword;
        
        if(this->IsPasswordEncrypted)
        {
            auto error = CryptoUtility::DecryptText(password_, decryptedPassword);
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceDomainUser,
                    "{0}: Decrypt text failed: Account={1}, AccountType {2} error = {3}",
                    applicationId_,
                    AccountName,
                    AccountType,
                    error);
                return error;
            }
        }
        else
        {
            decryptedPassword = SecureString(password_);
        }
        
        auto error = AccessToken::CreateDomainUserToken(
            username_,
            domain_,
            decryptedPassword,
            this->performInteractiveLogon_,
            this->loadProfile_,
            this->SidToAdd,
            userToken_);

        if (!error.IsSuccess())
        {
            WriteTrace(
                error.ToLogLevel(),
                TraceDomainUser,
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
#endif    
}

ErrorCode DomainUser::UpdateCachedCredentials()
{
//LINUXTODO
#if defined(PLATFORM_UNIX)
    return ErrorCode::Success();
#else
    wstring password;
    if(isPasswordEncrypted_)
    {
        SecureString decryptedPassword;
        CryptoUtility::DecryptText(password_, decryptedPassword);
        password = decryptedPassword.GetPlaintext();
    }
    auto error = AccessToken::UpdateCachedCredentials(username_, domain_, password);
 
    SecureZeroMemory((void *) password.c_str(), password.size() * sizeof(wchar_t));
    return error;
#endif    
}
