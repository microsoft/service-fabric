// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#if defined(PLATFORM_UNIX)
#include <pwd.h>
#include <grp.h>
#endif

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Management::FileStoreService;
using namespace ServiceModel;

StringLiteral const TraceComponent("Utility");


set<wstring, IsLessCaseInsensitiveComparer<wstring>> Utility::GetAllFiles(wstring const & root)
{
    auto versionedFileList  =  Directory::Find(root, L"*", UINT_MAX, true);

    return set<wstring, Common::IsLessCaseInsensitiveComparer<wstring>>(versionedFileList.begin(), versionedFileList.end());
}

wstring Utility::GetVersionedFileFullPath(std::wstring const & root, std::wstring const & relativeFilePath, StoreFileVersion const fileVersion)
{
    return GetVersionedFileFullPath(root, relativeFilePath, fileVersion.ToString());
}

wstring Utility::GetVersionedFileFullPath(std::wstring const & root, std::wstring const & relativeFilePath, wstring const & fileVersion)
{
    return Path::Combine(root, GetVersionedFile(relativeFilePath, fileVersion));
}

wstring Utility::GetVersionedFile(std::wstring const & relativeFilePath, std::wstring const & pattern)
{
    wstring fileName = Path::GetFileName(relativeFilePath);
    wstring directory = Path::GetDirectoryName(relativeFilePath);

    return Path::Combine(directory, wformatString("{0}.{1}", pattern, fileName));
}

wstring Utility::GetSharePath(std::wstring const & ipAddressOrFQDN, wstring const & shareName, FABRIC_REPLICA_ID const replicaId)
{
    wstring smbServerName;

    ASSERT_IF(ipAddressOrFQDN.empty(), "IPAddressOrFQDN cannot be emtpy.");

    Endpoint endpoint;
    auto error = Endpoint::TryParse(ipAddressOrFQDN, endpoint);
    if (error.IsSuccess())
    {
        smbServerName = endpoint.AsSmbServerName();
    }
    else
    {
        // If Endpoint parsing fails, then this should be FQDN
        smbServerName = wformatString("\\\\{0}", ipAddressOrFQDN);
    }

    StringUtility::TrimTrailing<wstring>(smbServerName, L"\\");

    return wformatString("{0}\\{1}\\{2}", smbServerName, shareName, replicaId);
}

ErrorCode Utility::DeleteVersionedStoreFile(std::wstring const & root, std::wstring const & versionedStoreRelativePath)
{
    wstring versionedFileFullPath = Path::Combine(root, versionedStoreRelativePath);

    ErrorCode error(ErrorCodeValue::Success);
    if(File::Exists(versionedFileFullPath))
    {
        error = File::Delete2(versionedFileFullPath, true /*deleteReadOnlyFiles*/);
        WriteTrace(
            error.IsSuccess() ? LogLevel::Info : LogLevel::Warning,
            TraceComponent,
            "Deleting File '{0}'. Error:{1}",
            versionedFileFullPath,
            error);
    }

    // delete the directories upto the parent of the relative path
    vector<wstring> tokens;
#if !defined(PLATFORM_UNIX)
    StringUtility::Split<wstring>(versionedStoreRelativePath, tokens, L"\\");
#else
    StringUtility::Split<wstring>(versionedStoreRelativePath, tokens, L"/");
#endif
    DeleteEmptyDirectory(Path::GetDirectoryName(versionedFileFullPath), static_cast<int>(tokens.size()) - 1);

    return error;
}

ErrorCode Utility::DeleteAllVersionsOfStoreFile(std::wstring const & root, std::wstring const & storeRelativePath)
{
    wstring versionedFileFullPath = GetVersionedFileFullPath(root, storeRelativePath, L"*");

    wstring directoryToSearch = Path::GetDirectoryName(versionedFileFullPath);
    wstring versionedFileName = Path::GetFileName(versionedFileFullPath);

    vector<wstring> files = Directory::Find(
        directoryToSearch,
        versionedFileName,
        UINT_MAX,
        false);

    ErrorCode lastError = ErrorCodeValue::Success;
    for (auto const & file : files)
    {
        // Only match and delete those files with the following pattern - <StoreFileVersion>.storeRelativePath
        // skip the rest
        wstring fileName = Path::GetFileName(file);
        auto pos = fileName.find_first_of(L'.');
        if(pos == string::npos)
        {
            continue;
        }

        wstring fileVersion = fileName.substr(0, pos);
        wstring fileNameWithoutVersion = fileName.substr(pos+1, fileName.size());

        if(!StoreFileVersion::IsStoreFileVersion(fileVersion))
        {
            continue;
        }

        if(!StringUtility::AreEqualCaseInsensitive(fileNameWithoutVersion, Path::GetFileName(storeRelativePath)))
        {
            continue;
        }

        auto error = File::Delete2(file, true /*deleteReadOnlyFiles*/);
        WriteTrace(
            error.IsSuccess() ? LogLevel::Info : LogLevel::Warning,
            TraceComponent,
            "Deleting File '{0}'. Error:{1}",
            file,
            error);
        if(!error.IsSuccess())
        {
            lastError.Overwrite(error);
        }
    }

    if(lastError.IsSuccess())
    {
        // delete the directories upto the parent of the relative path
        vector<wstring> tokens;
#if !defined(PLATFORM_UNIX)
        StringUtility::Split<wstring>(storeRelativePath, tokens, L"\\");
#else
        StringUtility::Split<wstring>(storeRelativePath, tokens, L"/");
#endif
        DeleteEmptyDirectory(directoryToSearch, static_cast<int>(tokens.size()) - 1);
    }

    return lastError;
}

void Utility::DeleteEmptyDirectory(wstring const & directoryPath, int level)
{
    if(level < 1)
    {
        return;
    }

    vector<wstring> files = Directory::Find(
        directoryPath,
        L"*",
        1, /* even if one file exists do not delete directory*/
        true);
    if(files.size() == 0)
    {
        auto error = Directory::Delete(directoryPath, false, false);
        if(error.IsSuccess())
        {
            DeleteEmptyDirectory(Path::GetDirectoryName(directoryPath), level - 1);
        }
    }
}

ErrorCode Utility::RetriableOperation(std::function<ErrorCode()> const & operation, uint const maxRetryCount)
{
    uint retryCount = 0;
    ErrorCode result;
    do
    {
        ::Sleep(retryCount * 1000);

        result = operation();

        switch(result.ReadValue())
        {
            // Return on success or non retriable errors
        case ErrorCodeValue::Success:
        case ErrorCodeValue::GatewayUnreachable:
        case ErrorCodeValue::ObjectClosed:
        case ErrorCodeValue::InvalidState:
        case ErrorCodeValue::PropertyCheckFailed:
            return result;
        }
    }while(++retryCount < maxRetryCount);

    return result;
}

ErrorCode Utility::GetPrimaryAccessToken(__inout Common::AccessTokenSPtr & primaryAccessToken)
{
    return GetAccessToken(
        Constants::PrimaryFileStoreServiceUser,
        FileStoreServiceConfig::GetConfig().PrimaryAccountType,
        FileStoreServiceConfig::GetConfig().PrimaryAccountUserName,
        FileStoreServiceConfig::GetConfig().PrimaryAccountUserPassword,
        FileStoreServiceConfig::GetConfig().PrimaryAccountNTLMPasswordSecret,
        FileStoreServiceConfig::GetConfig().PrimaryAccountNTLMX509StoreLocation,
        FileStoreServiceConfig::GetConfig().PrimaryAccountNTLMX509StoreName,
        FileStoreServiceConfig::GetConfig().PrimaryAccountNTLMX509Thumbprint,
        primaryAccessToken);
}

ErrorCode Utility::GetSecondaryAccessToken(__inout Common::AccessTokenSPtr & secondaryAccessToken)
{
    return GetAccessToken(
        Constants::SecondaryFileStoreServiceUser,
        FileStoreServiceConfig::GetConfig().SecondaryAccountType,
        FileStoreServiceConfig::GetConfig().SecondaryAccountUserName,
        FileStoreServiceConfig::GetConfig().SecondaryAccountUserPassword,
        FileStoreServiceConfig::GetConfig().SecondaryAccountNTLMPasswordSecret,
        FileStoreServiceConfig::GetConfig().SecondaryAccountNTLMX509StoreLocation,
        FileStoreServiceConfig::GetConfig().SecondaryAccountNTLMX509StoreName,
        FileStoreServiceConfig::GetConfig().SecondaryAccountNTLMX509Thumbprint,
        secondaryAccessToken);
}

ErrorCode Utility::GetAccessTokens(
    __inout AccessTokensCollection & tokenMap,
    __inout AccessTokensList & newTokens)
{
    newTokens.clear();

    ErrorCode error(ErrorCodeValue::Success);
    if (!FileStoreServiceConfig::GetConfig().PrimaryAccountType.empty())
    {
        if (tokenMap.find(Constants::PrimaryFileStoreServiceUser) == tokenMap.end())
        {
            AccessTokenSPtr primaryAccessToken;
            error = Utility::GetPrimaryAccessToken(primaryAccessToken);
            if (error.IsSuccess())
            {
                ASSERT_IFNOT(primaryAccessToken, "GetPrimaryAccessToken returned success, but null token");
                newTokens.push_back(primaryAccessToken);
                tokenMap.insert(make_pair(Constants::PrimaryFileStoreServiceUser, move(primaryAccessToken)));
            }
        }

        if (!FileStoreServiceConfig::GetConfig().SecondaryAccountType.empty())
        {
            if (tokenMap.find(Constants::SecondaryFileStoreServiceUser) == tokenMap.end())
            {
                AccessTokenSPtr secondaryAccessToken;
                auto tokenError = Utility::GetSecondaryAccessToken(secondaryAccessToken);
                if (tokenError.IsSuccess())
                {
                    ASSERT_IFNOT(secondaryAccessToken, "GetSecondaryAccessToken returned success, but null token");
                    newTokens.push_back(secondaryAccessToken);
                    tokenMap.insert(make_pair(Constants::SecondaryFileStoreServiceUser, move(secondaryAccessToken)));
                }
                else
                {
                    error.Overwrite(move(tokenError));
                }
            }
        }
    }

    if (Utility::HasCommonNameAccountsConfigured())
    {
        auto tokenError = Utility::GetCommonNamesAccessTokens(tokenMap, newTokens);
        if (!tokenError.IsSuccess())
        {
            error.Overwrite(move(tokenError));
        }
    }

    return error;
}

std::vector<Utility::CommonNameConfig> Utility::GetCommonNamesFromConfig()
{
    vector<CommonNameConfig> result;

    wstring cn1 = FileStoreServiceConfig::GetConfig().CommonName1Ntlmx509CommonName;
    StringUtility::TrimWhitespaces(cn1);
    if (!cn1.empty())
    {
        result.push_back(CommonNameConfig(
            cn1,
            FileStoreServiceConfig::GetConfig().CommonName1Ntlmx509StoreLocation,
            FileStoreServiceConfig::GetConfig().CommonName1Ntlmx509StoreName,
            FileStoreServiceConfig::GetConfig().CommonNameNtlmPasswordSecret));
    }

    wstring cn2 = FileStoreServiceConfig::GetConfig().CommonName2Ntlmx509CommonName;
    StringUtility::TrimWhitespaces(cn2);
    if (!cn2.empty())
    {
        result.push_back(CommonNameConfig(
            cn2,
            FileStoreServiceConfig::GetConfig().CommonName2Ntlmx509StoreLocation,
            FileStoreServiceConfig::GetConfig().CommonName2Ntlmx509StoreName,
            FileStoreServiceConfig::GetConfig().CommonNameNtlmPasswordSecret));
    }

    if (result.empty())
    {
        WriteInfo(TraceComponent, "No common name is defined in FileStoreServiceConfig.");
    }

    return result;
}

ErrorCode Utility::GetCommonNamesAccessTokens(
    __inout AccessTokensCollection & tokenMap,
    __inout AccessTokensList & newTokens)
{
    auto error(ErrorCode::Success());

    vector<CommonNameConfig> cns = Utility::GetCommonNamesFromConfig();
    if (cns.empty())
    {
        return error;
    }

    for (auto const & cn : cns)
    {
        // Create one user for each certificate matching the common name
        std::map<std::wstring, std::wstring> accountNamesMap;
        error = AccountHelper::GetAccountNamesWithCertificateCommonName(
            cn.CommonName,
            cn.StoreLocation,
            cn.StoreName,
            FileStoreServiceConfig::GetConfig().GenerateV1CommonNameAccount,
            accountNamesMap);

        if (error.IsSuccess())
        {
            for (auto && accountNamePair : accountNamesMap)
            {
                // Ignore the entries that already exist in the input map
                if (tokenMap.find(accountNamePair.first) != tokenMap.end())
                {
                    continue;
                }

                AccessTokenSPtr accessToken;
                auto accessTokenError = GetNTLMLocalUserAccessToken(
                    accountNamePair.first,
                    cn.NtlmPasswordSecret,
                    cn.StoreLocation,
                    cn.StoreName,
                    accountNamePair.second,
                    accessToken);
                if (!accessTokenError.IsSuccess())
                {
                    error.Overwrite((accessTokenError));
                    return error;
                }
                else
                {
                    newTokens.push_back(accessToken);
                    tokenMap.insert(make_pair(move(accountNamePair.first), move(accessToken)));
                }
            }
        }
        else if (error.IsError(ErrorCodeValue::CertificateNotFound) || error.IsError(ErrorCodeValue::CertificateNotMatched))
        {
            WriteWarning(TraceComponent, "GetCommonNamesAccessTokens: No certificates found to match common name {0}", cn.CommonName);
            continue;
        }
        else
        {
            WriteWarning(
                TraceComponent,
                "GetCommonNamesAccessTokens: error loading account names by certificate common names {0}: {1}",
                cn.CommonName,
                error);
            return error;
        }
    }

    return error;
}

bool Utility::HasAccountsConfigured()
{
    return Utility::HasThumbprintAccountsConfigured() || Utility::HasCommonNameAccountsConfigured();
}

bool Utility::HasCommonNameAccountsConfigured()
{
    return !FileStoreServiceConfig::GetConfig().CommonName1Ntlmx509CommonName.empty() ||
        !FileStoreServiceConfig::GetConfig().CommonName2Ntlmx509CommonName.empty();
}

bool Utility::HasThumbprintAccountsConfigured()
{
    return !FileStoreServiceConfig::GetConfig().PrimaryAccountType.empty();
}

ErrorCode Utility::GetPrincipalsDescriptionFromConfigByThumbprint(
    __inout ServiceModel::PrincipalsDescriptionUPtr & principalDescription)
{
    if (!Utility::HasThumbprintAccountsConfigured())
    {
        return ErrorCode::Success();
    }

    wstring defaultUserParentApplicationGroupName;
    principalDescription = CreateFileStoreServicePrincipalDescription(defaultUserParentApplicationGroupName);

    X509StoreLocation::Enum primaryStoreLocation;
    auto error = X509StoreLocation::Parse(FileStoreServiceConfig::GetConfig().PrimaryAccountNTLMX509StoreLocation, primaryStoreLocation);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "Parse of primary X509StoreLocation failed with {0}. Value={1}",
            error,
            FileStoreServiceConfig::GetConfig().PrimaryAccountNTLMX509StoreLocation);
        return error;
    }

    SecurityUserDescription primaryUserDesc = GetSecurityUser(
        Constants::PrimaryFileStoreServiceUser,
        FileStoreServiceConfig::GetConfig().PrimaryAccountType,
        FileStoreServiceConfig::GetConfig().PrimaryAccountUserName,
        FileStoreServiceConfig::GetConfig().PrimaryAccountUserPasswordEntry.IsEncrypted() ?
        FileStoreServiceConfig::GetConfig().PrimaryAccountUserPasswordEntry.GetEncryptedValue() : FileStoreServiceConfig::GetConfig().PrimaryAccountUserPassword.GetPlaintext(),
        FileStoreServiceConfig::GetConfig().PrimaryAccountUserPasswordEntry.IsEncrypted(),
        FileStoreServiceConfig::GetConfig().PrimaryAccountNTLMPasswordSecretEntry.IsEncrypted() ?
        FileStoreServiceConfig::GetConfig().PrimaryAccountNTLMPasswordSecretEntry.GetEncryptedValue() : FileStoreServiceConfig::GetConfig().PrimaryAccountNTLMPasswordSecret.GetPlaintext(),
        FileStoreServiceConfig::GetConfig().PrimaryAccountNTLMPasswordSecretEntry.IsEncrypted(),
        primaryStoreLocation,
        FileStoreServiceConfig::GetConfig().PrimaryAccountNTLMX509StoreName,
        FileStoreServiceConfig::GetConfig().PrimaryAccountNTLMX509Thumbprint);
    AddUser(principalDescription, move(primaryUserDesc), defaultUserParentApplicationGroupName);

    if (!FileStoreServiceConfig::GetConfig().SecondaryAccountType.empty())
    {
        X509StoreLocation::Enum secondaryStoreLocation;
        error = X509StoreLocation::Parse(FileStoreServiceConfig::GetConfig().SecondaryAccountNTLMX509StoreLocation, secondaryStoreLocation);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                "Parse of secondary X509StoreLocation failed with {0}. Value={1}",
                error,
                FileStoreServiceConfig::GetConfig().PrimaryAccountNTLMX509StoreLocation);
            return error;
        }

        SecurityUserDescription secondaryUserDesc = GetSecurityUser(
            Constants::SecondaryFileStoreServiceUser,
            FileStoreServiceConfig::GetConfig().SecondaryAccountType,
            FileStoreServiceConfig::GetConfig().SecondaryAccountUserName,
            FileStoreServiceConfig::GetConfig().SecondaryAccountUserPasswordEntry.IsEncrypted() ?
            FileStoreServiceConfig::GetConfig().SecondaryAccountUserPasswordEntry.GetEncryptedValue() : FileStoreServiceConfig::GetConfig().SecondaryAccountUserPassword.GetPlaintext(),
            FileStoreServiceConfig::GetConfig().SecondaryAccountUserPasswordEntry.IsEncrypted(),
            FileStoreServiceConfig::GetConfig().SecondaryAccountNTLMPasswordSecretEntry.IsEncrypted() ?
            FileStoreServiceConfig::GetConfig().SecondaryAccountNTLMPasswordSecretEntry.GetEncryptedValue() : FileStoreServiceConfig::GetConfig().SecondaryAccountNTLMPasswordSecret.GetPlaintext(),
            FileStoreServiceConfig::GetConfig().SecondaryAccountNTLMPasswordSecretEntry.IsEncrypted(),
            secondaryStoreLocation,
            FileStoreServiceConfig::GetConfig().SecondaryAccountNTLMX509StoreName,
            FileStoreServiceConfig::GetConfig().SecondaryAccountNTLMX509Thumbprint);
        AddUser(principalDescription, move(secondaryUserDesc), defaultUserParentApplicationGroupName);
    }

    return ErrorCode::Success();
}

Common::ErrorCode Utility::GetPrincipalsDescriptionFromConfigByCommonName(
    std::vector<std::wstring> const & currentThumbprints,
    __inout ServiceModel::PrincipalsDescriptionUPtr & principalDescription,
    __inout std::vector<std::wstring> & newThumbprints)
{
    auto error(ErrorCode::Success());
    newThumbprints.clear();

    vector<CommonNameConfig> cns = Utility::GetCommonNamesFromConfig();
    if (cns.empty())
    {
        return error;
    }

    wstring defaultUserParentApplicationGroupName;
    principalDescription = CreateFileStoreServicePrincipalDescription(defaultUserParentApplicationGroupName);

    for (auto const & cn : cns)
    {
        // Create one user for each certificate that matches the common name
        map<wstring, wstring> accountNamesMap;
        error = AccountHelper::GetAccountNamesWithCertificateCommonName(
            cn.CommonName,
            cn.StoreLocation,
            cn.StoreName,
            FileStoreServiceConfig::GetConfig().GenerateV1CommonNameAccount,
            accountNamesMap);

        if (error.IsSuccess())
        {
            for (auto && accountNamePair : accountNamesMap)
            {
                auto it = find(currentThumbprints.begin(), currentThumbprints.end(), accountNamePair.second);
                if (it == currentThumbprints.end())
                {
                    SecurityUserDescription user = GetNTLMSecurityUser(
                        accountNamePair.first,
                        cn.NtlmPasswordSecret.GetPlaintext(),
                        false /*isNTLMPasswordSecretEncrypted*/,
                        cn.StoreLocation,
                        cn.StoreName,
                        accountNamePair.second);
                    AddUser(principalDescription, move(user), defaultUserParentApplicationGroupName);

                    // If both V1 and V2 user name generation algorithms are enabled,
                    // same thumbprint will be present 2 times in the map.
                    auto itNew = find(newThumbprints.begin(), newThumbprints.end(), accountNamePair.second);
                    if (itNew == newThumbprints.end())
                    {
                        newThumbprints.push_back(move(accountNamePair.second));
                    }
                }
            }
        }
        else if (error.IsError(ErrorCodeValue::CertificateNotFound) || error.IsError(ErrorCodeValue::CertificateNotMatched))
        {
            WriteWarning(TraceComponent, "GetPrincipalsDescriptionFromConfigByCommonName: No certificates found to match common name {0}", cn.CommonName);
            continue;
        }
        else
        {
            // Unexpected error
            return error;
        }
    }

    WriteInfo(TraceComponent, "GetPrincipalsDescriptionFromConfigByCommonName: Found {0} new certificates", newThumbprints.size());   
    return ErrorCode::Success();
}

PrincipalsDescriptionUPtr Utility::CreateFileStoreServicePrincipalDescription(
    __out std::wstring & defaultUserParentApplicationGroupName)
{
    SecurityGroupDescription fileStoreServiceGroupDescription;
    fileStoreServiceGroupDescription.Name = *Constants::FileStoreServiceUserGroup;
    fileStoreServiceGroupDescription.NTLMAuthenticationPolicy.IsEnabled = true;

    auto principalDescription = make_unique<PrincipalsDescription>();
    principalDescription->Groups.push_back(fileStoreServiceGroupDescription);

#if defined(PLATFORM_UNIX)
    Common::SidSPtr sfuser;
    auto err = BufferedSid::CreateSPtr(WinNetworkServiceSid, sfuser);
    if (err.IsSuccess())
    {
        ULONG uid = ((PLSID)sfuser->PSid)->SubAuthority[1];
        struct passwd *pwd = getpwuid(uid);
        string sfuserUnameA(pwd->pw_name);
        wstring sfuserUname;
        StringUtility::Utf8ToUtf16(sfuserUnameA, sfuserUname);
        fileStoreServiceGroupDescription.DomainUserMembers.push_back(sfuserUname);

        struct group *grp = getgrgid(pwd->pw_gid);
        string sfuserGroupNameA(grp->gr_name);
        StringUtility::Utf8ToUtf16(sfuserGroupNameA, defaultUserParentApplicationGroupName);
    }

#else
    defaultUserParentApplicationGroupName.clear();
#endif

    return principalDescription;
}

void Utility::AddUser(
    ServiceModel::PrincipalsDescriptionUPtr const & principalDescription,
    ServiceModel::SecurityUserDescription && user,
    std::wstring const & defaultUserParentApplicationGroupName)
{
    ASSERT_IFNOT(principalDescription, "AddUser: principalDescription should not be null");

#if defined(PLATFORM_UNIX)
    ASSERT_IF(defaultUserParentApplicationGroupName.empty(), "AddUser: defaultUserParentApplicationGroupName is empty");
    user.ParentApplicationGroups.push_back(defaultUserParentApplicationGroupName);
#else
    ASSERT_IFNOT(defaultUserParentApplicationGroupName.empty(), "AddUser: defaultUserParentApplicationGroupName is not empty");
#endif

    principalDescription->Users.push_back(move(user));
}

ServiceModel::SecurityUserDescription Utility::GetSecurityUser(
    std::wstring const & accountName,
    std::wstring const & accountTypeString,
    std::wstring const & accountUserName,
    std::wstring const & accountUserPassword,
    bool isUserPasswordEncrypted,
    std::wstring const & ntlmPasswordSecret,
    bool isNTLMPasswordSecretEncrypted,
    Common::X509StoreLocation::Enum x509StoreLocation,
    std::wstring const & x509StoreName,
    std::wstring const & x509Thumbprint)
{
    SecurityPrincipalAccountType::Enum accountType;
    auto error = SecurityPrincipalAccountType::FromString(accountTypeString, accountType);
    ASSERT_IFNOT(error.IsSuccess(), "Invalid AccountType : {0}", accountTypeString);

    SecurityUserDescription user;
    user.Name = accountName;
    user.AccountType = accountType;

    if(accountType == SecurityPrincipalAccountType::Enum::LocalUser)
    {
        return GetNTLMSecurityUser(
            accountName,
            ntlmPasswordSecret,
            isNTLMPasswordSecretEncrypted,
            x509StoreLocation,
            x509StoreName,
            x509Thumbprint);
    }
    else if(accountType == SecurityPrincipalAccountType::Enum::DomainUser)
    {
        user.AccountName = accountUserName;
        user.Password = accountUserPassword;
        user.IsPasswordEncrypted = isUserPasswordEncrypted;
    }
    else if(accountType == SecurityPrincipalAccountType::Enum::ManagedServiceAccount)
    {
        user.AccountName = accountUserName;
    }
    else
    {
        Assert::CodingError("AccountType={0} is not expected", accountTypeString);
    }

    user.ParentSystemGroups.push_back(FabricConstants::WindowsFabricAdministratorsGroupName);
    user.ParentApplicationGroups.push_back(Constants::FileStoreServiceUserGroup);

    return user;
}

ServiceModel::SecurityUserDescription Utility::GetNTLMSecurityUser(
    std::wstring const & accountName,
    std::wstring const & ntlmPasswordSecret,
    bool isNTLMPasswordSecretEncrypted,
    Common::X509StoreLocation::Enum x509StoreLocation,
    std::wstring const & x509StoreName,
    std::wstring const & x509Thumbprint)
{
    SecurityUserDescription user;
    user.Name = accountName;
    user.AccountType = SecurityPrincipalAccountType::LocalUser;
    user.NTLMAuthenticationPolicy.IsEnabled = true;
    user.NTLMAuthenticationPolicy.PasswordSecret = ntlmPasswordSecret;
    user.NTLMAuthenticationPolicy.IsPasswordSecretEncrypted = isNTLMPasswordSecretEncrypted;
    user.NTLMAuthenticationPolicy.X509StoreLocation = x509StoreLocation;
    user.NTLMAuthenticationPolicy.X509StoreName = x509StoreName;
    user.NTLMAuthenticationPolicy.X509Thumbprint = x509Thumbprint;

    user.ParentSystemGroups.push_back(FabricConstants::WindowsFabricAdministratorsGroupName);
    user.ParentApplicationGroups.push_back(Constants::FileStoreServiceUserGroup);

    return user;
}

ErrorCode Utility::GetAccessToken(
    wstring const & name,
    wstring const & accountTypeString,
    wstring const & accountName,
    SecureString const & accountPassword,
    SecureString const & ntlmPasswordSecret,
    wstring const & x509StoreLocationString,
    wstring const & x509StoreName,
    wstring const & x509Thumbprint,
    __inout Common::AccessTokenSPtr & accessToken)
{
    ASSERT_IF(accountTypeString.empty(), "GetAccessToken: accountTypeString is empty for {0}", name);

    SecurityPrincipalAccountType::Enum accountType;
    auto error = SecurityPrincipalAccountType::FromString(accountTypeString, accountType);

    if (error.IsSuccess())
    {
        wstring userName, domainName, dlnFormatName;
        if (accountType == SecurityPrincipalAccountType::ManagedServiceAccount)
        {
            error = AccountHelper::GetServiceAccountName(accountName, userName, domainName, dlnFormatName);
            if (error.IsSuccess())
            {
                error = AccessToken::CreateServiceAccountToken(userName, domainName, SERVICE_ACCOUNT_PASSWORD, false, NULL, accessToken);
            }
        }
        else if (accountType == SecurityPrincipalAccountType::DomainUser)
        {
            error = AccountHelper::GetDomainUserAccountName(accountName, userName, domainName, dlnFormatName);
            
#if !defined(PLATFORM_UNIX)
            if (error.IsSuccess())
            {
                error = AccessToken::CreateDomainUserToken(userName, domainName, accountPassword, false, false, NULL, accessToken);
            }
#endif
        }
        else if (accountType == SecurityPrincipalAccountType::Enum::LocalUser)
        {
            error = GetNTLMLocalUserAccessToken(name, ntlmPasswordSecret, x509StoreLocationString, x509StoreName, x509Thumbprint, accessToken);
        }
        else
        {
            error = ErrorCode(
                ErrorCodeValue::InvalidArgument,
                wformatString(GET_FSS_RC(Invalid_SecurityPrincipalAccountType), accountTypeString));
        }
    }

    WriteTrace(
        error.IsSuccess() ? LogLevel::Info : LogLevel::Warning,
        TraceComponent,
        "GetAccessToken for Name='{0}', AccountType='{1}' completed with {2} {3}",
        name,
        accountTypeString,
        error,
        error.Message);

    return error;
}

ErrorCode Utility::GetNTLMLocalUserAccessToken(
    wstring const & name,
    SecureString const & ntlmPasswordSecret,
    wstring const & x509StoreLocationString,
    wstring const & x509StoreName,
    wstring const & x509Thumbprint,
    __inout Common::AccessTokenSPtr & accessToken)
{
    X509StoreLocation::Enum storeLocation;
    auto error = X509StoreLocation::Parse(x509StoreLocationString, storeLocation);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "X509StoreLocation parsing failed with {0}. X509StoreLocationString={1}",
            error,
            x509StoreLocationString);
        return error;
    }

    return Utility::GetNTLMLocalUserAccessToken(name, ntlmPasswordSecret, storeLocation, x509StoreName, x509Thumbprint, accessToken);
}

ErrorCode Utility::GetNTLMLocalUserAccessToken(
    wstring const & name,
    SecureString const & ntlmPasswordSecret,
    X509StoreLocation::Enum x509StoreLocation,
    wstring const & x509StoreName,
    wstring const & x509Thumbprint,
    __inout Common::AccessTokenSPtr & accessToken)
{
    wstring ntlmAccountName = AccountHelper::GetAccountName(name, ApplicationIdentifier::FabricSystemAppId->ApplicationNumber, true /*canSkipAddIdentifier*/);
    ErrorCode error;
    SecureString ntlmAccountPassword;
    if(x509Thumbprint.empty())
    {
        error = AccountHelper::GeneratePasswordForNTLMAuthenticatedUser(
            ntlmAccountName,
            ntlmPasswordSecret,
            ntlmAccountPassword);
    }
    else
    {
        error = AccountHelper::GeneratePasswordForNTLMAuthenticatedUser(
            ntlmAccountName,
            ntlmPasswordSecret,
            x509StoreLocation,
            x509StoreName,
            x509Thumbprint,
            ntlmAccountPassword);
    }

    if(error.IsSuccess())
    {
        error = AccessToken::CreateUserToken(
            ntlmAccountName,
            L".",
            ntlmAccountPassword.GetPlaintext(),
            LOGON32_LOGON_NETWORK_CLEARTEXT,
            LOGON32_PROVIDER_DEFAULT,
            false,
            NULL,
            accessToken);
    }

    WriteTrace(
        error.IsSuccess() ? LogLevel::Info : LogLevel::Warning,
        TraceComponent,
        "GetAccessToken for LocalAccount Name='{0}' (ntlmAccountName='{1}') completed with Error={2}",
        name,
        ntlmAccountName,
        error);

    return error;
}
