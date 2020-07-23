// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Management;
using namespace ImageModel;
using namespace ImageStore;
using namespace FileStoreService;

StringLiteral TraceType_ImageStoreFactory = "ImageStoreFactory";

GlobalWString ImageStoreFactory::AccountTypeKey = make_global<wstring>(L"AccountType=");
GlobalWString ImageStoreFactory::AccountNameKey = make_global<wstring>(L"AccountName=");
GlobalWString ImageStoreFactory::AccountPasswordKey = make_global<wstring>(L"AccountPassword=");

ErrorCode ImageStoreFactory::CreateImageStore(
    __out ImageStoreUPtr & imageStoreUPtr,	
    SecureString const & storeUri,
    int minTransferBPS,
    wstring const & localCachePath,
    wstring const & localRoot)
{
    return CreateImageStore(imageStoreUPtr, storeUri, minTransferBPS, Api::IClientFactoryPtr(), false, L"", localCachePath, localRoot);
}

ErrorCode ImageStoreFactory::CreateImageStore(
    __out ImageStoreUPtr & imageStoreUPtr,	
    SecureString const & storeUri,
    int minTransferBPS,
    Api::IClientFactoryPtr const & clientFactory,
	bool const isInternal,
    wstring const & workingDirectory,
    wstring const & localCachePath,
    wstring const & localRoot)
{
    if (!localCachePath.empty() && !Directory::Exists(localCachePath))
    {
        auto error = Directory::Create2(localCachePath);
        if(!error.IsSuccess())
        {
            return error;
        }
    }

    if (StringUtility::StartsWithCaseInsensitive<wstring>(storeUri.GetPlaintext(), FileImageStore::SchemaTag))
    {                        
        wstring rootUri;
        wstring accountName;
        SecureString accountPassword;
        SecurityPrincipalAccountType::Enum accountType;

        if(!TryParseFileImageStoreConnectionString(storeUri, rootUri, accountName, accountPassword, accountType))
        {
            return ErrorCodeValue::ImageStoreInvalidStoreUri;
        }

        AccessTokenSPtr imageStoreAccessToken;
        auto error = GetAccessToken(accountName, accountPassword, accountType, imageStoreAccessToken);
        if(!error.IsSuccess()) { return error; }                

        ImageStoreAccessDescriptionUPtr accessDescription;
        if(imageStoreAccessToken)
        {
            error = ImageStoreAccessDescription::Create(
                imageStoreAccessToken,
                localCachePath,
                localRoot,
                accessDescription);
            if(!error.IsSuccess()) { return error; }
        }
        
        imageStoreUPtr = move(make_unique<FileImageStore>(rootUri, localCachePath, localRoot, move(accessDescription)));
        
        return ErrorCodeValue::Success;
    }
    else if (StringUtility::StartsWithCaseInsensitive<wstring>(storeUri.GetPlaintext(), AzureImageStore::SchemaTag))
    {        
        imageStoreUPtr = move(make_unique<AzureImageStore>(localCachePath, localRoot, workingDirectory, minTransferBPS));
        return ErrorCodeValue::Success;
    }
    else if (StringUtility::AreEqualCaseInsensitive(storeUri.GetPlaintext(), Management::ImageStore::Constants::NativeImageStoreSchemaTag))
    {        
        if (clientFactory.get() == nullptr)
        {
            return ErrorCodeValue::InvalidArgument;
        }

        return NativeImageStore::CreateNativeImageStoreClient(
			isInternal,
            localCachePath,
            workingDirectory,
            clientFactory,
            imageStoreUPtr);
    }

    return ErrorCodeValue::ImageStoreInvalidStoreUri;
}

Common::ErrorCode ImageStoreFactory::GetAccessToken(
    std::wstring const & accountName,
    SecureString const & accountPassword,
    SecurityPrincipalAccountType::Enum const & accountType,
    __out AccessTokenSPtr & imageStoreAccessToken)
{
#if defined(PLATFORM_UNIX)
    imageStoreAccessToken = make_shared<AccessToken>();
#else
    if(!accountName.empty())
    {            
        auto error = SecurityUtility::EnsurePrivilege(SE_IMPERSONATE_NAME);
        if(!error.IsSuccess()) { return error; }

        wstring userName, domainName, dlnFormatName;                
        if(accountType == SecurityPrincipalAccountType::ManagedServiceAccount)
        {
            error = AccountHelper::GetServiceAccountName(accountName, userName, domainName, dlnFormatName);
            if(!error.IsSuccess()) { return error; }

            error = AccessToken::CreateServiceAccountToken(userName, domainName, accountPassword.GetPlaintext(), false, NULL, imageStoreAccessToken);
        }
        else if(accountType == SecurityPrincipalAccountType::DomainUser)
        {
            error = AccountHelper::GetDomainUserAccountName(accountName, userName, domainName, dlnFormatName);
            if(!error.IsSuccess()) { return error; }

            error = AccessToken::CreateDomainUserToken(userName, domainName, accountPassword, false, false,  NULL, imageStoreAccessToken);
        }
        else if(accountType == SecurityPrincipalAccountType::LocalUser)
        {
            error = AccessToken::CreateUserToken(
                accountName,
                L".",
                accountPassword.GetPlaintext(),
                LOGON32_LOGON_NETWORK_CLEARTEXT,
                LOGON32_PROVIDER_DEFAULT,
                false,
                NULL,
                imageStoreAccessToken);
        }
        else
        {
            return ErrorCodeValue::OperationFailed;
        }

        if(!error.IsSuccess()) { return error; }        
    }
#endif
    return ErrorCodeValue::Success;
}

bool ImageStoreFactory::TryParseFileImageStoreConnectionString(
    SecureString const & imageStoreConnectionString,
    __out wstring & rootUri,
    __out wstring & accountName,
    __out SecureString & accountPassword,
    __out SecurityPrincipalAccountType::Enum & accountType)
{    
    vector<wstring> tokens;
    StringUtility::Split<wstring>(imageStoreConnectionString.GetPlaintext(), tokens, L"|");
    rootUri = tokens[0];

    if(tokens.size() == 1)
    {
        return true;
    }

    wstring accountTypeString;
    for(auto iter = tokens.begin(); iter != tokens.end(); ++iter)
    {
        if(StringUtility::StartsWithCaseInsensitive<wstring>(*iter, AccountTypeKey))
        {
            accountTypeString = iter->substr(AccountTypeKey->size());
        } 
        else if(StringUtility::StartsWithCaseInsensitive<wstring>(*iter, AccountNameKey))            
        {
            accountName = iter->substr(AccountNameKey->size());
        }        
        else if(StringUtility::StartsWithCaseInsensitive<wstring>(*iter, AccountPasswordKey))
        {
            accountPassword = SecureString(move(iter->substr(AccountPasswordKey->size())));
            ::SecureZeroMemory((void *) iter->c_str(), iter->size() * sizeof(WCHAR));
        }
        else if(StringUtility::StartsWithCaseInsensitive<wstring>(*iter, FileImageStore::SchemaTag))
        {
            continue;
        }
        else
        {
            TraceError(
                TraceTaskCodes::ImageStore,
                TraceType_ImageStoreFactory,
                "ImageStoreConnectionString is invalid. {0} is not expected.",
                *iter);
            return false;
        }
    }
    
    accountType = SecurityPrincipalAccountType::DomainUser;
    if(!accountTypeString.empty())
    {
        auto error = SecurityPrincipalAccountType::FromString(accountTypeString, accountType);
        if(!error.IsSuccess() ||
            accountType != SecurityPrincipalAccountType::DomainUser || 
            accountType != SecurityPrincipalAccountType::ManagedServiceAccount)
        {
            TraceError(
                TraceTaskCodes::ImageStore,
                TraceType_ImageStoreFactory,
                "AccountType={0} is invalid in the ImageStoreConnectionString.",
                accountType);
            return false;
        }
    }

    if(accountName.empty())
    {
        TraceError(
            TraceTaskCodes::ImageStore,
            TraceType_ImageStoreFactory,
            "ImageStoreConnectionString in invalid. AccountName is missing.",
            accountType);
        return false;
    }

    if(accountType == SecurityPrincipalAccountType::ManagedServiceAccount)
    {
#if defined(PLATFORM_UNIX)
        Assert::CodingError("ManagedServiceAccount not supported on Linux");
#else
        if(!accountPassword.GetPlaintext().empty())
        {
            TraceError(
                TraceTaskCodes::ImageStore,
                TraceType_ImageStoreFactory,
                "ImageStoreConnectionString is invalid. AccountPassword should not be set for AccountType={0}. AccountName={1}.",
                accountType,
                accountName);
            return false;
        }

        accountPassword = SecureString(SERVICE_ACCOUNT_PASSWORD);
#endif
    }
    else
    {
        if (accountPassword.GetPlaintext().empty())
        {
            TraceError(
                TraceTaskCodes::ImageStore,
                TraceType_ImageStoreFactory,
                "ImageStoreConnectionString is invalid. AccountPassword should be set for AccountType={0}. AccountName={1}.",
                accountType,
                accountName);
            return false;            
        }        
    }

    return true;
}
