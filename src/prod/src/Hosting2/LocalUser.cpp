// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace ServiceModel;

StringLiteral const TraceLocalUser("LocalUser");

// Use a prefix that contains multiple different characters,
// so the account password policy doesn't complain
GlobalWString LocalUser::PasswordPrefix = make_global<wstring>(L"ABCD!abcd@#$");
GlobalWString LocalUser::AccountNamePrefix = make_global<wstring>(L"WF-");

int const LocalUser::RandomAccountNameLength = 15;
DWORD const LocalUser::RandomDigitsInPassword = 10;

LocalUser::LocalUser(
    wstring const& applicationId, 
    SecurityUserDescription const & userDescription)
    : SecurityUser(applicationId, userDescription)
{
}

LocalUser::LocalUser(
    wstring const & applicationId,
    wstring const & name,
    wstring const & accountName,
    wstring const & password,
    bool isPasswordEncrypted,
    bool loadProfile,
    bool performInteractiveLogon,
    NTLMAuthenticationPolicyDescription const & ntlmAuthenticationPolicy,
    vector<wstring> parentApplicationGroups,
    vector<wstring> parentSystemGroups)
    : SecurityUser
    (
        applicationId,
        name,
        accountName,
        password,
        isPasswordEncrypted,
        ntlmAuthenticationPolicy,
        SecurityPrincipalAccountType::LocalUser,
        parentApplicationGroups,
        parentSystemGroups,
        loadProfile,
        performInteractiveLogon)
{
}

LocalUser::~LocalUser()
{
}

ErrorCode LocalUser::CreateAccountwithRandomName(wstring const& comment)
{
    ASSERT_IFNOT(AccountType == SecurityPrincipalAccountType::LocalUser, "Account with random name is only applicable to local user");
    AcquireExclusiveLock grab(userLock_);

#if !defined(PLATFORM_UNIX)
    SecureString securePassword;
    if(!AccountHelper::GenerateRandomPassword(securePassword))
    {
        return ErrorCode(ErrorCodeValue::OperationFailed);
    }

    password_ = securePassword.GetPlaintext();
#else
    password_ = L"";
    int passwordLength = 10;
    wstring charset = L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for(int i = 0; i < passwordLength; i++)
    {
        password_ += charset[rand() % (charset.length() - 1)];
    }
#endif

    bool done = false;
    ErrorCode error;
    wstring userNameTemp;
    while(!done)
    {
        done = true;
        // Create random account name
        wstring randomString;
        StringUtility::GenerateRandomString(LocalUser::RandomAccountNameLength, randomString);
        userNameTemp = (wstring)LocalUser::AccountNamePrefix;
        userNameTemp.append(randomString);
        error = SecurityPrincipalHelper::CreateUserAccount(userNameTemp, password_, comment);
        if (!error.IsSuccess())
        {
            if(error.IsError(ErrorCodeValue::AlreadyExists))
            {
                WriteWarning(
                    TraceLocalUser,
                    "{0}:User with name {1} already exists. Retrying.",
                    userNameTemp,
                    accountName_);
                done = false;
            }
            else
            {
                return error;
            }

            userNameTemp.clear();
        }
    }

    accountName_ = userNameTemp;
    return SetupGroupMembershipForNewUser();
}

ErrorCode LocalUser::CreateAccountwithName(wstring const& accountName, wstring const& comment)
{
    AcquireExclusiveLock grab(userLock_);

    auto error = GeneratePasswordForNTLMAuthenticatedUser(accountName, ntlmAuthenticationDesc_, password_);
    if(!error.IsSuccess())
    {
        WriteWarning(
            TraceLocalUser,
            "GeneratePasswordForNTLMAuthenticatedUser for AccountName={0} failed with {1}. NTLMAuthDesc={2}",
            accountName,
            error,
            ntlmAuthenticationDesc_);
        return error;
    }

    error = SecurityPrincipalHelper::CreateUserAccount(accountName, password_, comment);
    ASSERT_IF(
        error.IsError(ErrorCodeValue::AlreadyExists), 
        "{0}:CreateUserAccount in CreateAccountwithName for name {1} cannot fail with AlreadyExists",
        applicationId_,
        accountName);

    if(error.IsSuccess())
    {
        accountName_ = accountName;
        error = SetupGroupMembershipForNewUser();
    }

    return error;
}

ErrorCode LocalUser::DeleteAccount()
{
    AcquireExclusiveLock grab(userLock_);
    // Reset the user token, which will let go of the profile handle.
    // If the handle is used by someone else, the profile won't be unloaded, 
    // so the user won't be able to be removed. 
    userToken_.reset();
    ASSERT_IF(!sid_, "DeleteAccount cannot be called on a user that has not been Loaded or Created");

    auto error = SecurityPrincipalHelper::DeleteUserAccountIgnoreDeleteProfileError(accountName_, sid_);

    return error;
}
ErrorCode LocalUser::LoadAccount(wstring const& accountName)
{
    auto error = SecurityUser::LoadAccount(accountName);
    if(!error.IsSuccess())
    {
        return error;
    }

    error = GeneratePasswordForNTLMAuthenticatedUser(accountName, ntlmAuthenticationDesc_, password_);
    if(!error.IsSuccess())
    {
        return error;
    }

    accountName_ = accountName;
    return ErrorCode(ErrorCodeValue::Success);
}

void LocalUser::UnloadAccount()
{
    AcquireExclusiveLock grab(userLock_);
    // Reset the user token, which will let go of the profile handle.
    // If the handle is used by someone else, the profile won't be unloaded, 
    // so the user won't be able to be removed. 
    userToken_.reset();
}

ErrorCode LocalUser::CreateLogonToken(__out AccessTokenSPtr & userToken)
{
//LINUXTODO
#if !defined(PLATFORM_UNIX)
    AcquireExclusiveLock grab(userLock_);
    if (!userToken_)
    {
        // Create the user token
        WriteInfo(
            TraceLocalUser,
            "{0}: Creating user token: Account={1}, AccountType {2}",
            applicationId_,
            accountName_,
            AccountType);
        DWORD logonType = this->performInteractiveLogon_ ? LOGON32_LOGON_INTERACTIVE : LOGON32_LOGON_NETWORK_CLEARTEXT;
        auto error = AccessToken::CreateUserToken(
            accountName_,
            L".",
            password_,
            logonType,
            LOGON32_PROVIDER_DEFAULT,
            this->loadProfile_,
            this->SidToAdd,
            userToken_);
       
        if (!error.IsSuccess())
        {
            WriteTrace(
                error.ToLogLevel(),
                TraceLocalUser,
                "{0}: Create user token: Account={1}, AccountType {2} error = {3}",
                applicationId_,
                accountName_,
                AccountType,
                error);
            return error;
        }
    }
#endif    

#if DBG
    if (!HostingConfig::GetConfig().InteractiveRunAsEnabled)
    {
        // The password in only needed for interactive mode
        password_.clear();
    }
#else
    password_.clear();
#endif


    userToken = userToken_;
    return ErrorCode(ErrorCodeValue::Success);
}


ErrorCode LocalUser::GeneratePasswordForNTLMAuthenticatedUser(
    wstring const & accountName,
    NTLMAuthenticationPolicyDescription const & ntlmAuthenticationPolicy,
    __out wstring & password)
{
    SecureString passwordSecret;
    if(ntlmAuthenticationPolicy.IsPasswordSecretEncrypted)
    {
        auto error = CryptoUtility::DecryptText(ntlmAuthenticationPolicy.PasswordSecret, passwordSecret);
        if(!error.IsSuccess())
        {
            return error;
        }
    }
    else
    {
        passwordSecret = SecureString(ntlmAuthenticationPolicy.PasswordSecret);
    }

    ErrorCode errorCode;
    SecureString securePassword;
    if(ntlmAuthenticationPolicy.X509Thumbprint.empty())
    {
        errorCode = AccountHelper::GeneratePasswordForNTLMAuthenticatedUser(accountName, passwordSecret, securePassword);
    }
    else
    {
        errorCode = AccountHelper::GeneratePasswordForNTLMAuthenticatedUser(
            accountName,
            passwordSecret,
            ntlmAuthenticationPolicy.X509StoreLocation,
            ntlmAuthenticationPolicy.X509StoreName,
            ntlmAuthenticationPolicy.X509Thumbprint,
            securePassword);
    }

    if(errorCode.IsSuccess())
    {
        password = securePassword.GetPlaintext();
    }
    
    return errorCode;
}
