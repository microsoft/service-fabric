// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "Common/Uri.h"

using namespace HttpGateway;

namespace HttpGateway
{
    class HttpCertificateAuthHandlerTest
    {
    };

    BOOST_FIXTURE_TEST_SUITE(HttpCertificateAuthHandlerTestSuite, HttpCertificateAuthHandlerTest)

    BOOST_AUTO_TEST_CASE(HttpGatewayCertRefresh)
    {
        auto savedRefreshInterval = SecurityConfig::GetConfig().CertificateMonitorInterval;
        SecurityConfig::GetConfig().CertificateMonitorInterval = TimeSpan::FromSeconds(3);
        KFinally([&] { SecurityConfig::GetConfig().CertificateMonitorInterval = savedRefreshInterval; });

        auto cn = wformatString("CertRefresh.{0}", Guid::NewGuid());
        auto sn = wformatString("CN={0}", cn);
        InstallTestCertInScope c1(sn, nullptr, TimeSpan::FromMinutes(60 * 24 * 7 * 20), L"Root", L"", X509Default::StoreLocation());
        
        SecurityConfig::X509NameMap remoteNames;
        remoteNames.Add(cn, L"");
        SecurityConfig::IssuerStoreKeyValueMap issuerMap;
        issuerMap.Add(cn, L"Root");

        Transport::SecuritySettings securitySettings;
        auto error = Transport::SecuritySettings::FromConfiguration(
            L"X509",
            L"Root",
            wformatString(X509Default::StoreLocation()),
            wformatString(X509FindType::FindBySubjectName),
            cn,
            L"",
            wformatString(Transport::ProtectionLevel::EncryptAndSign),
            L"",
            remoteNames,
            issuerMap,
            L"",
            L"",
            L"",
            L"",
            securitySettings);
        VERIFY_IS_TRUE(error.IsSuccess());
        FabricClientWrapperSPtr client;
        HttpCertificateAuthHandler *handler = new HttpCertificateAuthHandler(Transport::SecurityProvider::Ssl, client);
        handler->OnInitialize(securitySettings);
        VERIFY_ARE_EQUAL2(securitySettings, *handler->Settings());

        // Wait for new c2 to be loaded
        InstallTestCertInScope c2(sn, nullptr, InstallTestCertInScope::DefaultCertExpiry() + TimeSpan::FromHours(48), L"Root", L"", X509Default::StoreLocation());
        ManualResetEvent().WaitOne(SecurityConfig::GetConfig().CertificateMonitorInterval + TimeSpan::FromSeconds(6));

        X509IdentitySet publicKeys;
        publicKeys.AddPublicKey(c2.CertContext());
        securitySettings.SetRemoteCertIssuerPublicKeys(publicKeys);
        VERIFY_ARE_EQUAL2(securitySettings, *handler->Settings());
    }
    BOOST_AUTO_TEST_SUITE_END()

}

