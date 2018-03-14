// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace FabricTest;
using namespace Transport;

GlobalWString ClientSecurityHelper::DefaultSslClientCredentials = make_global<wstring>(
    wformatString(
        "X509:{0}:{1}:FindBySubjectName:CN=WinFabric-Test-SAN1-Alice:EncryptAndSign:WinFabricRing.Rings.WinFabricTestDomain.com",
        X509Default::StoreLocation(), X509Default::StoreName()));

GlobalWString ClientSecurityHelper::DefaultWindowsCredentials = make_global<wstring>(L"Windows:EncryptAndSign");

void ClientSecurityHelper::ClearDefaultCertIssuers()
{
    SecurityConfig::GetConfig().ClusterCertIssuers = L"";
    SecurityConfig::GetConfig().ClientCertIssuers = L"";
    SecurityConfig::GetConfig().ServerCertIssuers = L"";
}

void ClientSecurityHelper::SetDefaultCertIssuers()
{
    SecurityConfig::GetConfig().ClusterCertIssuers = L"b3449b018d0f6839a2c5d62b5b6c6ac822b6f662";
    SecurityConfig::GetConfig().ClientCertIssuers = L"b3449b018d0f6839a2c5d62b5b6c6ac822b6f662";
    SecurityConfig::GetConfig().ServerCertIssuers = L"b3449b018d0f6839a2c5d62b5b6c6ac822b6f662";
}

bool ClientSecurityHelper::TryCreateClientSecuritySettings(
    wstring const & credentialsToParse,
    __out wstring & credentialsType,
    __out Transport::SecuritySettings & settingsResult,
    __out wstring & errorMessage)
{
    errorMessage.clear();

    StringCollection clientCredentials;
    StringUtility::Split<wstring>(credentialsToParse, clientCredentials, L":", false);

    if (clientCredentials.empty())
    {
        errorMessage = wformatString("Invalid params for TryCreateClientSecuritySettings: {0}", credentialsToParse);
        return false;
    }

    credentialsType = clientCredentials[0];

    if (StringUtility::CompareCaseInsensitive(clientCredentials[0], L"None") == 0)
    {
        ClearDefaultCertIssuers();
        return true;
    }

    if (StringUtility::CompareCaseInsensitive(clientCredentials[0], L"Windows") == 0)
    {
        if (clientCredentials.size() != 2)
        {
            errorMessage = wformatString("Invalid params for TryCreateClientSecuritySettings: {0}", credentialsToParse);
            return false;
        }

        ClearDefaultCertIssuers();
        if (SecurityConfig::GetConfig().NegotiateForWindowsSecurity)
        {
            return SecuritySettings::CreateNegotiate(
                TransportSecurity().LocalWindowsIdentity(),
                L"",
                clientCredentials[1],
                settingsResult).IsSuccess();
        }

        return SecuritySettings::CreateKerberos(
            TransportSecurity().LocalWindowsIdentity(),
            L"",
            clientCredentials[1],
            settingsResult).IsSuccess();
    }

    if (StringUtility::AreEqualCaseInsensitive(clientCredentials[0], L"Claims"))
    {
        if(clientCredentials.size() < 4)
        {
            errorMessage = wformatString("Invalid params for TryCreateClientSecuritySettings: {0}", credentialsToParse);
            return false;
        }

        SetDefaultCertIssuers();
        ErrorCode error = SecuritySettings::CreateClaimTokenClient(
            clientCredentials[1],
            L"", // server certificate thumbprints
            clientCredentials[2], // server certificate common name
            (clientCredentials.size() == 5) ? clientCredentials[4] : SecurityConfig::GetConfig().ServerCertIssuers,
            clientCredentials[3],
            settingsResult);

        if (!error.IsSuccess())
        {
            errorMessage = wformatString("Invalid params for TryCreateClientSecuritySettings: {0}: {1}", credentialsToParse, error);
            return false;
        }

        return true;
    }

    if (!StringUtility::AreEqualCaseInsensitive(clientCredentials[0], L"X509"))
    {
        errorMessage = wformatString("Invalid client credential type: {0}", clientCredentials[0]);
        return false;
    }

    if(clientCredentials.size() < 7)
    {
        errorMessage = wformatString("Invalid number of params for TryCreateClientSecuritySettings: count={0} expected>=7", clientCredentials.size());
        return false;
    }

    // todo, allow tests to specify store location
    SetDefaultCertIssuers();
    auto error = SecuritySettings::FromConfiguration(
        clientCredentials[0],
        clientCredentials[2],
        wformatString(X509Default::StoreLocation()),
        clientCredentials[3],
        clientCredentials[4],
        L"",
        clientCredentials[5],
        (clientCredentials.size() >= 9) ? clientCredentials[8] : L"",
        SecurityConfig::X509NameMap(),
        SecurityConfig::IssuerStoreKeyValueMap(),
        clientCredentials[6],
        (clientCredentials.size() >= 8) ? clientCredentials[7] : SecurityConfig::GetConfig().ServerCertIssuers,
        L"",
        L"",
        settingsResult);

    if (!error.IsSuccess())
    {
        errorMessage = wformatString("Invalid params for TryCreateClientSecuritySettings: {0}: {1}", credentialsToParse, error);
        return false;
    }

    return true;
}
