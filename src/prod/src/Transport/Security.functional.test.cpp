// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TestCommon.h"

using namespace Transport;
using namespace Common;
using namespace std;

class SecurityTest
{
protected:
    void X509CertChecking(SecuritySettings const & senderSecuritySettings, bool localChecking = false, bool failureExpected = false);
    void FindValueSecondaryTestWithThumbprint(
        wstring const & primary,
        wstring const & secondary,
        int expected /*-1: none, 0: primary, 1: secondary*/);

    void FindValueSecondaryTestWithCommonName(
        wstring const & primary,
        wstring const & secondary,
        int expected /*-1: none, 0: primary, 1: secondary*/);

    void SendTestMsg(
        IDatagramTransportSPtr const & sender,
        ISendTarget::SPtr const & target,
        wstring const & msgAction);

    void FullCertChainValidationWithThumbprintAuth_SelfSignedCert(
        wstring const & certStoreName,
        bool expectFailureOnFullCertChainValidation);

    void LoadByFullSubjectNameVsCommonName(wstring const & commonName, wstring const & fullSubjectName);

    void LoadBySubjectAltName(vector<wstring> const & subjectAltNames);

    void CrlTest(
        wstring const & senderCN,
        wstring const & receiverCN,
        bool ignoreCrlOffline);
};

BOOST_FIXTURE_TEST_SUITE2(SecurityTestSuite,SecurityTest)

BOOST_AUTO_TEST_CASE(LoadByNonExistentSubjectName)
{
    ENTER;

    wstring subjectName = L"CN=LoadByNonExistentSubjectName, O=Microsoft Corporation, L=Redmond, S=Washington, C=US";
    X509FindValue::SPtr findValue;
    auto error = X509FindValue::Create(X509FindType::FindBySubjectName, subjectName, findValue);
    VERIFY_IS_TRUE(error.IsSuccess());

    CertContextUPtr certificate;
    error = CryptoUtility::GetCertificate(
        X509Default::StoreLocation(), 
        X509Default::StoreName(),
        findValue,
        certificate);

    VERIFY_IS_FALSE(error.IsSuccess());

    LEAVE;
}

BOOST_AUTO_TEST_CASE(LoadByFullSubjectNameVsCommonName1)
{
    ENTER;
    wstring commonName = L"SVC";
    wstring fullSubjectName = L"CN=" + commonName + L", O=Microsoft, L=Redmond, C=US";
    Trace.WriteInfo(TraceType, "loading by full subject name starting with 'CN=': {0}", fullSubjectName);
    LoadByFullSubjectNameVsCommonName(commonName, fullSubjectName);
    LEAVE;
}

BOOST_AUTO_TEST_CASE(LoadByFullSubjectNameVsCommonName2)
{
    ENTER;
    wstring commonName = L"SVC2";
    wstring fullSubjectName = L"C=US, L=Redmond, O=Microsoft, CN=" + commonName;
    Trace.WriteInfo(TraceType, "loading by full subject name ending with 'CN=': {0}", fullSubjectName);
    LoadByFullSubjectNameVsCommonName(commonName, fullSubjectName);
    LEAVE;
}

BOOST_AUTO_TEST_CASE(CredExpirationLoadTest)
{
    DateTime certExpiration = DateTime::Now() + InstallTestCertInScope::DefaultCertExpiry(); 
    InstallTestCertInScope cert;
    DateTime expirationLoaded;
    auto error = SecurityUtil::GetX509SvrCredExpiration(
        cert.StoreLocation(),
        cert.StoreName(),
        cert.Thumbprint(),
        expirationLoaded);

    VERIFY_IS_TRUE(error.IsSuccess());

    Trace.WriteInfo(TraceType, "certExpiration = {0}, expirationLoaded = {1}", certExpiration, expirationLoaded);
    TimeSpan expirationDiff = certExpiration - expirationLoaded;
    VERIFY_IS_TRUE(TimeSpan::FromMinutes(-1) < expirationDiff);
    VERIFY_IS_TRUE(expirationDiff < TimeSpan::FromMinutes(1));
}

BOOST_AUTO_TEST_CASE(FindValueSecondaryWithThumbprint)
{
    ENTER;

    Trace.WriteInfo(TraceType, "bad primary, good secondary");
    InstallTestCertInScope c1;
    FindValueSecondaryTestWithThumbprint(
        L"ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff",
        c1.Thumbprint()->PrimaryToString(),
        1);

    Trace.WriteInfo(TraceType, "good primary, bad secondary");
    FindValueSecondaryTestWithThumbprint(
        c1.Thumbprint()->PrimaryToString(),
        L"ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff",
        0);

    Trace.WriteInfo(TraceType, "good primary, good secondary, different expiration");
    InstallTestCertInScope c2(L"", nullptr, InstallTestCertInScope::DefaultCertExpiry() + InstallTestCertInScope::DefaultCertExpiry());
    FindValueSecondaryTestWithThumbprint(
        c1.Thumbprint()->PrimaryToString(),
        c2.Thumbprint()->PrimaryToString(),
        0);

    FindValueSecondaryTestWithThumbprint(
        c2.Thumbprint()->PrimaryToString(),
        c1.Thumbprint()->PrimaryToString(),
        0);

    Trace.WriteInfo(TraceType, "expired primary, good secondary");
    InstallTestCertInScope ce(L"", nullptr, TimeSpan::Zero -InstallTestCertInScope::DefaultCertExpiry());
    FindValueSecondaryTestWithThumbprint(
        ce.Thumbprint()->PrimaryToString(),
        c1.Thumbprint()->PrimaryToString(),
        1);

    Trace.WriteInfo(TraceType, "good primary, expired secondary");
    FindValueSecondaryTestWithThumbprint(
        c1.Thumbprint()->PrimaryToString(),
        ce.Thumbprint()->PrimaryToString(),
        0);

    Trace.WriteInfo(TraceType, "bad primary, bad secondary");
    FindValueSecondaryTestWithThumbprint(
        L"ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff",
        L"00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00",
        -1);

    LEAVE;
}

BOOST_AUTO_TEST_CASE(TestInstallTestCertInScope)
{
    ENTER;

    wstring commonName = Guid::NewGuid().ToString();
    wstring subjectName = L"CN=" + commonName;
    Thumbprint::SPtr certThumbprint;
    wstring storeName;
    X509StoreLocation::Enum storeLocation;
    {
        InstallTestCertInScope testCert(subjectName);

        certThumbprint = testCert.Thumbprint();
        storeName = testCert.StoreName();
        storeLocation = testCert.StoreLocation();

        Trace.WriteInfo(TraceType, "verify we can load the cert from store and verify common name");
        wstring certCommonName;
        auto error = CryptoUtility::GetCertificateCommonName(
            storeLocation,
            storeName,
            certThumbprint,
            certCommonName);

        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(commonName, certCommonName));

#ifndef PLATFORM_UNIX
        Trace.WriteInfo(TraceType, "verify we can load the private key of the cert");
        wstring privateKeyFileName;
        error = CryptoUtility::GetCertificatePrivateKeyFileName(
            testCert.StoreLocation(),
            testCert.StoreName(),
            testCert.Thumbprint(),
            privateKeyFileName);

        VERIFY_IS_TRUE(error.IsSuccess());
        Trace.WriteInfo(TraceType, "private key file name: {0}", privateKeyFileName);
        VERIFY_IS_TRUE(!privateKeyFileName.empty());
#endif
    }

    CertContextUPtr certLoaded;
    auto error = CryptoUtility::GetCertificate(
        storeLocation,
        storeName,
        certThumbprint,
        certLoaded);

    VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::CertificateNotFound));

    LEAVE;
}

BOOST_AUTO_TEST_CASE(CertRolloverWithThumbprints)
{
    ENTER;

    auto savedRefreshInterval = SecurityConfig::GetConfig().CertificateMonitorInterval;
    SecurityConfig::GetConfig().CertificateMonitorInterval = TimeSpan::FromSeconds(3);
    KFinally([&] { SecurityConfig::GetConfig().CertificateMonitorInterval = savedRefreshInterval; });

    Trace.WriteInfo(TraceType, "Installing certificates for initial setup");
    InstallTestCertInScope serverCert(L"CN=server.CertRolloverWithThumbprints");
    InstallTestCertInScope clientCert(
        L"CN=client@CertRolloverWithThumbprints",
        nullptr,
        InstallTestCertInScope::DefaultCertExpiry(),
        X509Default::StoreName(),
        L"client@CertRolloverWithThumbprints"/*use non-default key container name to get a pair of key different from server*/);

    auto svrSecSettings = TTestUtil::CreateX509SettingsTp(
        serverCert.Thumbprint()->PrimaryToString(),
        L"",
        clientCert.Thumbprint()->PrimaryToString(),
        L"");

    Trace.WriteInfo(TraceType, "create server transport");
    auto server = DatagramTransportFactory::CreateTcp(L"127.0.0.1:0");
    auto error = server->SetSecurity(svrSecSettings);
    VERIFY_IS_TRUE(error.IsSuccess());

    wstring action = Guid::NewGuid().ToString();
    TTestUtil::SetMessageHandler(
        server,
        action,
        [&server, &action](MessageUPtr & message, ISendTarget::SPtr const & target) -> void
        {
            Trace.WriteInfo(TraceType, "server received {0}", message->TraceId());
            auto reply = make_unique<Message>();
            reply->Headers.Add(MessageIdHeader());
            reply->Headers.Add(ActionHeader(action)),
            server->SendOneWay(target, move(reply));
        });

    Trace.WriteInfo(TraceType, "create another server whose config update will drag behind");
    auto serverLaggedBehind = DatagramTransportFactory::CreateTcp(L"127.0.0.1:0");
    error = serverLaggedBehind->SetSecurity(svrSecSettings);
    VERIFY_IS_TRUE(error.IsSuccess());

    TTestUtil::SetMessageHandler(
        serverLaggedBehind,
        action,
        [&serverLaggedBehind, &action](MessageUPtr & message, ISendTarget::SPtr const & target) -> void
        {
            Trace.WriteInfo(TraceType, "serverLaggedBehind received {0}", message->TraceId());
            auto reply = make_unique<Message>();
            reply->Headers.Add(MessageIdHeader());
            reply->Headers.Add(ActionHeader(action)),
            serverLaggedBehind->SendOneWay(target, move(reply));
        });

    auto cltSecSettings = TTestUtil::CreateX509SettingsTp(
        clientCert.Thumbprint()->PrimaryToString(),
        L"",
        serverCert.Thumbprint()->PrimaryToString(),
        L"");

    auto client = DatagramTransportFactory::CreateTcpClient();
    error = client->SetSecurity(cltSecSettings);
    VERIFY_IS_TRUE(error.IsSuccess());

    AutoResetEvent cltReceivedMsg(false);
    TTestUtil::SetMessageHandler(
        client,
        action,
        [&cltReceivedMsg](MessageUPtr & message, ISendTarget::SPtr const &) -> void
        {
            Trace.WriteInfo(TraceType, "client received {0}", message->TraceId());
            cltReceivedMsg.Set();
        });

    auto clientLaggedBehind = DatagramTransportFactory::CreateTcp(L"127.0.0.1:0");
    error = clientLaggedBehind->SetSecurity(cltSecSettings);
    VERIFY_IS_TRUE(error.IsSuccess());

    AutoResetEvent cltLaggedBehindReceivedMsg(false);
    TTestUtil::SetMessageHandler(
        clientLaggedBehind,
        action,
        [&cltLaggedBehindReceivedMsg](MessageUPtr & message, ISendTarget::SPtr const &) -> void
    {
        Trace.WriteInfo(TraceType, "clientLaggedBehind received {0}", message->TraceId());
        cltLaggedBehindReceivedMsg.Set();
    });

    error = server->Start();
    VERIFY_IS_TRUE(error.IsSuccess());
    error = serverLaggedBehind->Start();
    VERIFY_IS_TRUE(error.IsSuccess());
    error = client->Start();
    VERIFY_IS_TRUE(error.IsSuccess());
    error = clientLaggedBehind->Start();
    VERIFY_IS_TRUE(error.IsSuccess());

    vector<ISendTarget::SPtr> targetsInClient;
    targetsInClient.push_back(client->ResolveTarget(server->ListenAddress()));
    targetsInClient.push_back(client->ResolveTarget(serverLaggedBehind->ListenAddress()));
    vector<ISendTarget::SPtr> targetsInClientLaggedBehind;
    targetsInClientLaggedBehind.push_back(clientLaggedBehind->ResolveTarget(server->ListenAddress()));
    targetsInClientLaggedBehind.push_back(clientLaggedBehind->ResolveTarget(serverLaggedBehind->ListenAddress()));

    Trace.WriteInfo(TraceType, "test with a round of message exchange");
    for (auto const & target : targetsInClient)
    {
        SendTestMsg(client, target, action);
        VERIFY_IS_TRUE(cltReceivedMsg.WaitOne(5000));
    }
    for (auto const & target : targetsInClientLaggedBehind)
    {
        SendTestMsg(clientLaggedBehind, target, action);
        VERIFY_IS_TRUE(cltLaggedBehindReceivedMsg.WaitOne(5000));
    }

    DateTime serverCertExpiry;
    error = CryptoUtility::GetCertificateExpiration(
        serverCert.StoreLocation(),
        serverCert.StoreName(),
        L"FindByThumbprint",
        serverCert.Thumbprint()->PrimaryToString(),
        serverCertExpiry);

    VERIFY_IS_TRUE(error.IsSuccess());

    Trace.WriteInfo(TraceType, "Creating new certificates with further expiration for rollover");
    CertContextUPtr svrCert2;
    wstring svrCert2KeyContainer = L"server2.CertRolloverWithThumbprints";
    error = CryptoUtility::CreateSelfSignedCertificate(
        L"CN=server2.CertRolloverWithThumbprints",
        serverCertExpiry + InstallTestCertInScope::DefaultCertExpiry(),
        svrCert2KeyContainer,
        svrCert2);

    VERIFY_IS_TRUE(error.IsSuccess());
    Thumbprint::SPtr svrCertThumbprint2;
    error = Thumbprint::Create(svrCert2.get(), svrCertThumbprint2);
    VERIFY_IS_TRUE(error.IsSuccess());

    DateTime clientCertExpiry;
    error = CryptoUtility::GetCertificateExpiration(
        clientCert.StoreLocation(),
        clientCert.StoreName(),
        L"FindByThumbprint",
        clientCert.Thumbprint()->PrimaryToString(),
        clientCertExpiry);

    VERIFY_IS_TRUE(error.IsSuccess());

    CertContextUPtr cltCert2;
    wstring cltCert2KeyContainer = L"client2@CertRolloverWithThumbprints";
    error = CryptoUtility::CreateSelfSignedCertificate(
        L"CN=client2@CertRolloverWithThumbprints",
        clientCertExpiry + InstallTestCertInScope::DefaultCertExpiry(),
        cltCert2KeyContainer,
        cltCert2);

    VERIFY_IS_TRUE(error.IsSuccess());
    Thumbprint::SPtr cltCertThumbprint2;
    error = Thumbprint::Create(cltCert2.get(), cltCertThumbprint2);
    VERIFY_IS_TRUE(error.IsSuccess());

    auto svrSecSettings2 = TTestUtil::CreateX509SettingsTp(
        wformatString(svrCertThumbprint2),
        serverCert.Thumbprint()->PrimaryToString(),
        wformatString(cltCertThumbprint2),
        clientCert.Thumbprint()->PrimaryToString());

    error = server->SetSecurity(svrSecSettings2);
    VERIFY_IS_TRUE(error.IsSuccess());

    auto cltSecSettings2 = TTestUtil::CreateX509SettingsTp(
        wformatString(cltCertThumbprint2),
        clientCert.Thumbprint()->PrimaryToString(),
        wformatString(svrCertThumbprint2),
        serverCert.Thumbprint()->PrimaryToString());

    error = client->SetSecurity(cltSecSettings2);
    VERIFY_IS_TRUE(error.IsSuccess());

    Trace.WriteInfo(TraceType, "still prefer old server certificate on serverLaggedBehind, but allow both old and new on incoming client certificates");
    auto svrSecSettingsOldLocalNewRemote = TTestUtil::CreateX509SettingsTp(
        serverCert.Thumbprint()->PrimaryToString(),
        wformatString(svrCertThumbprint2),
        clientCert.Thumbprint()->PrimaryToString(),
        wformatString(cltCertThumbprint2));

    error = serverLaggedBehind->SetSecurity(svrSecSettingsOldLocalNewRemote);
    VERIFY_IS_TRUE(error.IsSuccess());

    Trace.WriteInfo(TraceType, "still prefer old client certificate on clientLaggedBehind, but allow both old and new on incoming server certificates");
    auto cltSecSettingsOldLocalNewRemote = TTestUtil::CreateX509SettingsTp(
        clientCert.Thumbprint()->PrimaryToString(),
        wformatString(cltCertThumbprint2),
        serverCert.Thumbprint()->PrimaryToString(),
        wformatString(svrCertThumbprint2));

    error = clientLaggedBehind->SetSecurity(cltSecSettingsOldLocalNewRemote);
    VERIFY_IS_TRUE(error.IsSuccess());

    Trace.WriteInfo(TraceType, "test with a second round of message exchange");
    for (auto const & target : targetsInClient)
    {
        SendTestMsg(client, target, action);
        VERIFY_IS_TRUE(cltReceivedMsg.WaitOne(5000));
    }
    for (auto const & target : targetsInClientLaggedBehind)
    {
        SendTestMsg(clientLaggedBehind, target, action);
        VERIFY_IS_TRUE(cltLaggedBehindReceivedMsg.WaitOne(5000));
    }

    Trace.WriteInfo(TraceType, "loaded certificate thumbprints should remain unchanged as new certificates are not installed yet");
    SecurityCredentialsSPtr svrCredentials = ((TcpDatagramTransport&)(*server)).Security()->Credentails();
    Trace.WriteInfo(TraceType, "server cert thumbprint: {0}", svrCredentials->X509CertThumbprint());
    VERIFY_IS_TRUE(svrCredentials->X509CertThumbprint() == *serverCert.Thumbprint());

    SecurityCredentialsSPtr cltCredentials = ((TcpDatagramTransport&)(*client)).Security()->Credentails();
    Trace.WriteInfo(TraceType, "client cert thumbprint: {0}", cltCredentials->X509CertThumbprint());
    VERIFY_IS_TRUE(cltCredentials->X509CertThumbprint() == *clientCert.Thumbprint());

    Trace.WriteInfo(TraceType, "install new certificates");
    error = CryptoUtility::InstallCertificate(
        svrCert2,
        serverCert.StoreLocation(),
        serverCert.StoreName());
    VERIFY_IS_TRUE(error.IsSuccess());
    KFinally([&]{ CryptoUtility::UninstallCertificate(serverCert.StoreLocation(), serverCert.StoreName(), svrCertThumbprint2, svrCert2KeyContainer); });

    error = CryptoUtility::InstallCertificate(
        cltCert2,
        clientCert.StoreLocation(),
        clientCert.StoreName());
    VERIFY_IS_TRUE(error.IsSuccess());
    KFinally([&]{ CryptoUtility::UninstallCertificate(clientCert.StoreLocation(), clientCert.StoreName(), cltCertThumbprint2, cltCert2KeyContainer); });

    Trace.WriteInfo(TraceType, "wait for new certificate to be loaded");
    bool svrCertRefreshed = TTestUtil::WaitUntil(
        [&] {
            svrCredentials = ((TcpDatagramTransport&)(*server)).Security()->Credentails();
            return svrCredentials->X509CertThumbprint() == *svrCertThumbprint2; },
        SecurityConfig::GetConfig().CertificateMonitorInterval + TimeSpan::FromSeconds(3));
    VERIFY_IS_TRUE(svrCertRefreshed);

    bool cltCertRefreshed = TTestUtil::WaitUntil(
        [&] {
            cltCredentials = ((TcpDatagramTransport&)(*client)).Security()->Credentails();
            return cltCredentials->X509CertThumbprint() == *cltCertThumbprint2; },
        SecurityConfig::GetConfig().CertificateMonitorInterval + TimeSpan::FromSeconds(3));
    VERIFY_IS_TRUE(cltCertRefreshed);

    Trace.WriteInfo(TraceType, "test with a third round of message exchange after certificate refresh");
    for (auto const & target : targetsInClient)
    {
        SendTestMsg(client, target, action);
        VERIFY_IS_TRUE(cltReceivedMsg.WaitOne(5000));
    }
    for (auto const & target : targetsInClientLaggedBehind)
    {
        SendTestMsg(clientLaggedBehind, target, action);
        VERIFY_IS_TRUE(cltLaggedBehindReceivedMsg.WaitOne(5000));
    }

    Trace.WriteInfo(TraceType, "uninstall old certificates");
    serverCert.Uninstall();
    CertContextUPtr savedClientCertContext = clientCert.DetachCertContext();
    clientCert.Uninstall(false); // keep key container for reinstall later

    Trace.WriteInfo(TraceType, "old certificates uninstalled, new certificate will be loaded by lagged behind");
    bool svrLaggedBehindCertRefreshed = TTestUtil::WaitUntil(
        [&] {
            svrCredentials = ((TcpDatagramTransport&)(*serverLaggedBehind)).Security()->Credentails();
            return svrCredentials->X509CertThumbprint() == *svrCertThumbprint2; },
        SecurityConfig::GetConfig().CertificateMonitorInterval + TimeSpan::FromSeconds(3));
    VERIFY_IS_TRUE(svrLaggedBehindCertRefreshed);

    bool cltLaggedBehindCertRefreshed = TTestUtil::WaitUntil(
        [&] {
            cltCredentials = ((TcpDatagramTransport&)(*clientLaggedBehind)).Security()->Credentails();
            return cltCredentials->X509CertThumbprint() == *cltCertThumbprint2; },
        SecurityConfig::GetConfig().CertificateMonitorInterval + TimeSpan::FromSeconds(3));
    VERIFY_IS_TRUE(cltLaggedBehindCertRefreshed);

    Trace.WriteInfo(TraceType, "test with a fourth round of message exchange after serverLaggedBehind loads new certificate");
    for (auto const & target : targetsInClient)
    {
        SendTestMsg(client, target, action);
        VERIFY_IS_TRUE(cltReceivedMsg.WaitOne(5000));
    }
    for (auto const & target : targetsInClientLaggedBehind)
    {
        SendTestMsg(clientLaggedBehind, target, action);
        VERIFY_IS_TRUE(cltLaggedBehindReceivedMsg.WaitOne(5000));
    }

    Trace.WriteInfo(TraceType, "remove old certificates from settings on all clients and servers");
    auto svrSecSettings3 = TTestUtil::CreateX509SettingsTp(
        wformatString(svrCertThumbprint2),
        L"",
        wformatString(cltCertThumbprint2),
        L"");

    error = server->SetSecurity(svrSecSettings3);
    VERIFY_IS_TRUE(error.IsSuccess());
    error = serverLaggedBehind->SetSecurity(svrSecSettings3);
    VERIFY_IS_TRUE(error.IsSuccess());

    auto cltSecSettings3 = TTestUtil::CreateX509SettingsTp(
        wformatString(cltCertThumbprint2),
        L"",
        wformatString(svrCertThumbprint2),
        L"");

    error = client->SetSecurity(cltSecSettings3);
    VERIFY_IS_TRUE(error.IsSuccess());
    error = clientLaggedBehind->SetSecurity(cltSecSettings3);
    VERIFY_IS_TRUE(error.IsSuccess());

    Trace.WriteInfo(TraceType, "test with another round of message exchange after old certificates removed from settings");
    for (auto const & target : targetsInClient)
    {
        SendTestMsg(client, target, action);
        VERIFY_IS_TRUE(cltReceivedMsg.WaitOne(5000));
    }
    for (auto const & target : targetsInClientLaggedBehind)
    {
        SendTestMsg(clientLaggedBehind, target, action);
        VERIFY_IS_TRUE(cltLaggedBehindReceivedMsg.WaitOne(5000));
    }

    Trace.WriteInfo(TraceType, "install old client certificate again");
    error = CryptoUtility::InstallCertificate(savedClientCertContext);
    VERIFY_IS_TRUE(error.IsSuccess());
    KFinally([&] { CryptoUtility::UninstallCertificate(clientCert.StoreLocation(), clientCert.StoreName(), clientCert.Thumbprint()); });

    Trace.WriteInfo(TraceType, "update sec settings on clientLaggedBehind to make it load old client certificate");
    auto cltSecSettings4 = TTestUtil::CreateX509SettingsTp(
        clientCert.Thumbprint()->PrimaryToString(),
        L"",
        wformatString(svrCertThumbprint2),
        L"");

    error = clientLaggedBehind->SetSecurity(cltSecSettings4);
    VERIFY_IS_TRUE(error.IsSuccess());
    SendTestMsg(clientLaggedBehind, targetsInClientLaggedBehind.front(), action);
    VERIFY_IS_FALSE(cltLaggedBehindReceivedMsg.WaitOne(SecurityConfig::GetConfig().CertificateMonitorInterval));

    Trace.WriteInfo(TraceType, "test with a final round of message exchange after another certificate monitor interval");
    for (auto const & target : targetsInClient)
    {
        SendTestMsg(client, target, action);
        VERIFY_IS_TRUE(cltReceivedMsg.WaitOne(5000));
    }

    error = clientLaggedBehind->SetSecurity(cltSecSettings3);
    for (auto const & target : targetsInClientLaggedBehind)
    {
        SendTestMsg(clientLaggedBehind, target, action);
        VERIFY_IS_TRUE(cltLaggedBehindReceivedMsg.WaitOne(5000));
    }

    auto credRefreshCount = TransportSecurity::GetCredentialRefreshCount();
    Trace.WriteInfo(TraceType, "credential refresh count = {0}", credRefreshCount);
    VERIFY_IS_TRUE(credRefreshCount == 4);

    Trace.WriteInfo(TraceType, "!!! Test completed !!!");
    client->Stop();
    server->Stop();

    LEAVE;
}

#ifndef PLATFORM_UNIX

BOOST_AUTO_TEST_CASE(CrlTest_OfflineErrIgnored)
{
    ENTER;
    CrlTest(L"WinFabric-Test-User@microsoft.com", L"WinFabric-Test-SAN1-Bob", true);
    LEAVE;
}

BOOST_AUTO_TEST_CASE(CrlTest_OfflineErrIgnored2) // test with server certs on both sides
{
    ENTER;
    CrlTest(L"WinFabric-Test-SAN2-Charlie", L"WinFabric-Test-SAN1-Bob", true);
    LEAVE;
}

BOOST_AUTO_TEST_CASE(CrlTest_OfflineErrNotIgnored)
{
    ENTER;
    CrlTest(L"WinFabric-Test-User@microsoft.com", L"WinFabric-Test-SAN1-Bob", false);
    LEAVE;
}

BOOST_AUTO_TEST_CASE(CrlTest_OfflineErrNotIgnored2) // test with server certs on both sides
{
    ENTER;
    CrlTest(L"WinFabric-Test-SAN2-Charlie", L"WinFabric-Test-SAN1-Bob", false);
    LEAVE;
}

BOOST_AUTO_TEST_CASE(PartialChainTest)
{
    ENTER;

    Trace.WriteInfo(TraceType, "Test thumbprint matching on partial chain");
    X509FindValue::SPtr findValue;
    auto error = X509FindValue::Create(X509FindType::FindByThumbprint, L"59 ec 79 20 04 c5 62 25 dd 66 91 13 2c 71 31 94 d2 80 98 f1", findValue);
    VERIFY_IS_TRUE(error.IsSuccess());
    CertContextUPtr certificate;
    error = CryptoUtility::GetCertificate(
        X509Default::StoreLocation(), 
        X509Default::StoreName(),
        findValue,
        certificate);

    VERIFY_IS_TRUE(error.IsSuccess());
    VERIFY_IS_TRUE(certificate != nullptr);

    ThumbprintSet thumbprintSetToMatch;
    error = thumbprintSetToMatch.Add(L"59 ec 79 20 04 c5 62 25 dd 66 91 13 2c 71 31 94 d2 80 98 f1");
    VERIFY_IS_TRUE(error.IsSuccess());

    SECURITY_STATUS status = SecurityContextSsl::VerifyCertificate(
        L"PartialChainTest",
        certificate.get(),
        SecurityConfig::GetConfig().CrlCheckingFlag,
        false,
        false,
        thumbprintSetToMatch,
        SecurityConfig::X509NameMap(),
        true);

    VERIFY_IS_TRUE(status == SEC_E_OK);

    LEAVE;
}

BOOST_AUTO_TEST_CASE(PartialChainTest2)
{
    ENTER;

    Trace.WriteInfo(TraceType, "Test thumbprint matching on partial chain with chain validation");
    X509FindValue::SPtr findValue;
    auto error = X509FindValue::Create(X509FindType::FindByThumbprint, L"59 ec 79 20 04 c5 62 25 dd 66 91 13 2c 71 31 94 d2 80 98 f1", findValue);
    VERIFY_IS_TRUE(error.IsSuccess());
    CertContextUPtr certificate;
    error = CryptoUtility::GetCertificate(
        X509Default::StoreLocation(), 
        X509Default::StoreName(),
        findValue,
        certificate);

    VERIFY_IS_TRUE(error.IsSuccess());
    VERIFY_IS_TRUE(certificate != nullptr);

    ThumbprintSet thumbprintSetToMatch;
    error = thumbprintSetToMatch.Add(L"59 ec 79 20 04 c5 62 25 dd 66 91 13 2c 71 31 94 d2 80 98 f1?true");
    VERIFY_IS_TRUE(error.IsSuccess());

    SECURITY_STATUS status = SecurityContextSsl::VerifyCertificate(
        L"PartialChainTest2",
        certificate.get(),
        SecurityConfig::GetConfig().CrlCheckingFlag,
        false,
        false,
        thumbprintSetToMatch,
        SecurityConfig::X509NameMap(),
        true);

    VERIFY_IS_TRUE(status == CERT_E_UNTRUSTEDROOT);

    LEAVE;
}

BOOST_AUTO_TEST_CASE(FullCertChainValidationWithThumbprintAuth)
{
    ENTER;

    Trace.WriteInfo(TraceType, "Test with a CA signed cert");
    {
        X509FindValue::SPtr findValue;
        auto error = X509FindValue::Create(X509FindType::FindByThumbprint, L"9d c9 06 b1 69 dc 4f af fd 16 97 ac 78 1e 80 67 90 74 9d 2f", findValue);
        VERIFY_IS_TRUE(error.IsSuccess());
        CertContextUPtr certificate;
        error = CryptoUtility::GetCertificate(
            X509Default::StoreLocation(), 
            X509Default::StoreName(),
            findValue,
            certificate);

        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(certificate != nullptr);

        ThumbprintSet thumbprintSetToMatch;
        error = thumbprintSetToMatch.Add(L"9d c9 06 b1 69 dc 4f af fd 16 97 ac 78 1e 80 67 90 74 9d 2f ? true");
        VERIFY_IS_TRUE(error.IsSuccess());

        SECURITY_STATUS status = SecurityContextSsl::VerifyCertificate(
            L"FullCertChainValidationWithThumbprintAuth",
            certificate.get(),
            SecurityConfig::GetConfig().CrlCheckingFlag,
            false,
            false,
            thumbprintSetToMatch,
            SecurityConfig::X509NameMap(),
            true);

        VERIFY_IS_TRUE(status == SEC_E_OK);
    }

    Trace.WriteInfo(TraceType, "Test with a self-signed cert installed to '{0}'", X509Default::StoreName());
    FullCertChainValidationWithThumbprintAuth_SelfSignedCert(X509Default::StoreName(), true);

    // trusted-people based chain validation fails intermittently on Azure VMs, re-enable the following
    // tests when the issue is figured out (our generated self-certifcate is probably not compliant),
    // tracking bug:
    // RDBug 11674715:[Test Failure Investigation]Transport.Functional.Test.exe test failed in SingleMachine-Functional
    // http://vstfrd:8080/Azure/RD/_workitems?_a=edit&id=11674715
    //Trace.WriteInfo(TraceType, "Test with a self-signed cert installed to TrustedPeople");
    //FullCertChainValidationWithThumbprintAuth_SelfSignedCert(L"TrustedPeople", false);

    //Trace.WriteInfo(TraceType, "Test with a self-signed cert installed to Root");
    //FullCertChainValidationWithThumbprintAuth_SelfSignedCert(L"Root", false);

    LEAVE;
}


BOOST_AUTO_TEST_CASE(LoadBySubjectAltNameTest)
{
    ENTER;

    Trace.WriteInfo(TraceType, "Find by Subject Alt Name");
    vector<wstring> subjectAltNames;
    subjectAltNames.push_back(L"TestDns");
    subjectAltNames.push_back(L"TestDns.TestSubDomain");
    subjectAltNames.push_back(L"TestDns.TestSubDomain.Microsoft.Com");
    subjectAltNames.push_back(L"TestDns.TestSubDomain.Microsoft");

    LoadBySubjectAltName(subjectAltNames);

    LEAVE;
}

BOOST_AUTO_TEST_CASE(MessagingTestWithLoadBySubjectAltName)
{
    ENTER;

    vector<wstring> node1DnsName(1, L"node1.test.com");
    vector<wstring> node2DnsName(1, L"node2.test.com");

    wstring const storeName = L"Root"; //install to root store for chain validation
    InstallTestCertInScope node1Cert(
        L"CN=node1",
        &node1DnsName,
        InstallTestCertInScope::DefaultCertExpiry(),
        storeName);

    InstallTestCertInScope node2Cert(
        L"CN=node2",
        &node2DnsName,
        InstallTestCertInScope::DefaultCertExpiry(),
        storeName);

    auto node1 = DatagramTransportFactory::CreateTcp(L"127.0.0.1:0");
    auto error = node1->SetSecurity(TTestUtil::CreateX509SettingsBySan(storeName, node1DnsName.front(), node2DnsName.front()));
    VERIFY_IS_TRUE(error.IsSuccess());

    AutoResetEvent node1ReceivedMsg;
    wstring testAction = TTestUtil::GetGuidAction();
    TTestUtil::SetMessageHandler(
        node1,
        testAction,
        [&node1ReceivedMsg](MessageUPtr & message, ISendTarget::SPtr const & st) -> void
        {
            Trace.WriteInfo(TraceType, "node1 got a message {0} from {1}", message->TraceId(), st->Address());
            node1ReceivedMsg.Set();
        });

    error = node1->Start();
    VERIFY_IS_TRUE(error.IsSuccess());

    auto node2 = DatagramTransportFactory::CreateTcp(L"127.0.0.1:0");
    error = node2->SetSecurity(TTestUtil::CreateX509SettingsBySan(storeName, node2DnsName.front(), node1DnsName.front()));
    VERIFY_IS_TRUE(error.IsSuccess());

    AutoResetEvent node2ReceivedMsg;
    TTestUtil::SetMessageHandler(
        node2,
        testAction,
        [&node2ReceivedMsg](MessageUPtr & message, ISendTarget::SPtr const & st) -> void
        {
            Trace.WriteInfo(TraceType, "node1 got a message {0} from {1}", message->TraceId(), st->Address());
            node2ReceivedMsg.Set();
        });

    error = node2->Start();
    VERIFY_IS_TRUE(error.IsSuccess());

    auto target1 = node2->ResolveTarget(node1->ListenAddress());
    auto message = make_unique<Message>();
    message->Headers.Add(MessageIdHeader());
    message->Headers.Add(ActionHeader(testAction));
    error = node2->SendOneWay(target1, move(message));
    VERIFY_IS_TRUE(error.IsSuccess());

    auto target2 = node1->ResolveTarget(node2->ListenAddress());
    message = make_unique<Message>();
    message->Headers.Add(MessageIdHeader());
    message->Headers.Add(ActionHeader(testAction));
    error = node1->SendOneWay(target2, move(message));
    VERIFY_IS_TRUE(error.IsSuccess());

    VERIFY_IS_TRUE(node1ReceivedMsg.WaitOne(TimeSpan::FromSeconds(16)));
    VERIFY_IS_TRUE(node2ReceivedMsg.WaitOne(TimeSpan::FromSeconds(8)));

    LEAVE;
}

BOOST_AUTO_TEST_CASE(FindValueSecondaryWithCommonName)
{
    ENTER;

    Trace.WriteInfo(TraceType, "bad primary, good secondary");
    FindValueSecondaryTestWithCommonName(
        L"NoSuchCertificateIHope",
        L"WinFabric-Test-SAN1-Alice",
        1);

    Trace.WriteInfo(TraceType, "good primary, bad secondary");
    FindValueSecondaryTestWithCommonName(
        L"WinFabric-Test-SAN1-Alice",
        L"NoSuchCertificateIHope",
        0);

    Trace.WriteInfo(TraceType, "good primary, good secondary");
    FindValueSecondaryTestWithCommonName(
        L"WinFabric-Test-SAN1-Bob",
        L"WinFabric-Test-SAN1-Alice",
        0);

    Trace.WriteInfo(TraceType, "expired primary, good secondary");
    FindValueSecondaryTestWithCommonName(
        L"WinFabric-Test-Expired",
        L"WinFabric-Test-SAN1-Alice",
        1);

    Trace.WriteInfo(TraceType, "good primary, expired secondary");
    FindValueSecondaryTestWithCommonName(
        L"WinFabric-Test-SAN1-Alice",
        L"WinFabric-Test-Expired",
        0);

    Trace.WriteInfo(TraceType, "bad primary, bad secondary");
    FindValueSecondaryTestWithCommonName(
        L"NoSuchCertificateIHope",
        L"NoSuchCertificateAgain",
        -1);

    LEAVE;
}

BOOST_AUTO_TEST_CASE(X509CertificateLoadTests)
{
    ENTER;

    Trace.WriteInfo(TraceType, "loading a valid certificate by subject name ...");
    {
        X509FindValue::SPtr subjectName;
        auto error = X509FindValue::Create(X509FindType::FindBySubjectName, L"CN = WinFabric-Test-SAN1-Alice", subjectName);
        VERIFY_IS_TRUE(error.IsSuccess());
        CertContextUPtr certificate;
        error = CryptoUtility::GetCertificate(
            X509Default::StoreLocation(), 
            X509Default::StoreName(),
            subjectName,
            certificate);

        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(certificate != nullptr);

        Thumbprint loadedThumbprint;
        error = loadedThumbprint.Initialize(certificate.get());
        VERIFY_IS_TRUE(error.IsSuccess());

        Thumbprint expectedThumbprint;
        error = expectedThumbprint.Initialize(L"78 12 20 5a 39 d2 23 76 da a0 37 f0 5a ed e3 60 1a 7e 64 bf");
        VERIFY_IS_TRUE(error.IsSuccess());

        // Verify the certificate with the furthest expiration is loaded, there are multiple matches for "CN=WinFabric-Test-SAN1-Alice".
        VERIFY_IS_TRUE(loadedThumbprint == expectedThumbprint);

        wstring commonName;
        error = CryptoUtility::GetCertificateCommonName(certificate.get(), commonName);
        VERIFY_IS_TRUE(error.IsSuccess());

        // Verify the certificate has proper subject name and subject alternative name (if any), and check CRL
        SECURITY_STATUS status = SecurityContextSsl::Test_VerifyCertificate(certificate.get(), commonName);
        VERIFY_IS_TRUE(status == SEC_E_OK);
    }

    Trace.WriteInfo(TraceType, "loading a valid certificate by common name ...");
    {
        X509FindValue::SPtr findValue;
        auto error = X509FindValue::Create(X509FindType::FindBySubjectName, L"WinFabric-Test-SAN1-Alice", findValue);
        VERIFY_IS_TRUE(error.IsSuccess());
        CertContextUPtr certificate;
        error = CryptoUtility::GetCertificate(
            X509Default::StoreLocation(), 
            X509Default::StoreName(),
            findValue,
            certificate);

        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(certificate != nullptr);

        Thumbprint loadedThumbprint;
        error = loadedThumbprint.Initialize(certificate.get());
        VERIFY_IS_TRUE(error.IsSuccess());

        Thumbprint expectedThumbprint;
        error = expectedThumbprint.Initialize(L"78 12 20 5a 39 d2 23 76 da a0 37 f0 5a ed e3 60 1a 7e 64 bf");
        VERIFY_IS_TRUE(error.IsSuccess());

        // Verify the certificate with the furthest expiration is loaded, there are multiple matches for "CN=WinFabric-Test-SAN1-Alice".
        VERIFY_IS_TRUE(loadedThumbprint == expectedThumbprint);

        wstring commonName;
        error = CryptoUtility::GetCertificateCommonName(certificate.get(), commonName);
        VERIFY_IS_TRUE(error.IsSuccess());

        // Verify the certificate has proper subject name and subject alternative name (if any), and check CRL
        SECURITY_STATUS status = SecurityContextSsl::Test_VerifyCertificate(certificate.get(), commonName);
        VERIFY_IS_TRUE(status == SEC_E_OK);
    }

    Trace.WriteInfo(TraceType, "loading a valid client certificate by thumbprint...");
    {
        CertContextUPtr certificate;
        X509FindValue::SPtr thumbprint;
        auto error = X509FindValue::Create(X509FindType::FindByThumbprint, L"6f 4a a9 61 8a ea 95 ab 5d ce 8c 77 26 09 38 65 3e e1 5f d7", thumbprint);
        VERIFY_IS_TRUE(error.IsSuccess());

        error = CryptoUtility::GetCertificate(
            X509Default::StoreLocation(), 
            X509Default::StoreName(),
            thumbprint,
            certificate);

        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(certificate != nullptr);

        SECURITY_STATUS status = SecurityContextSsl::Test_VerifyCertificate(certificate.get(), L"WiNfAbRiC-tEsT-uSeR@mIcRoSoFt.CoM");
        VERIFY_IS_TRUE(status == SEC_E_OK);
    }

    Trace.WriteInfo(TraceType, "loading an expired certificate ...");
    {
        X509FindValue::SPtr findValue;
        auto error = X509FindValue::Create(X509FindType::FindByThumbprint, L"ad fc 91 97 13 16 8d 9f a8 ee 71 2b a2 f4 37 62 00 03 49 0d", findValue);
        VERIFY_IS_TRUE(error.IsSuccess());

        CertContextUPtr certificate;
        error = CryptoUtility::GetCertificate(
            X509Default::StoreLocation(), 
            X509Default::StoreName(),
            findValue,
            certificate);

        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(certificate != nullptr);

        wstring commonName;
        error = CryptoUtility::GetCertificateCommonName(certificate.get(), commonName);
        VERIFY_IS_TRUE(error.IsSuccess());

        // Verify the certificate has proper subject name and subject alternative name (if any), and check CRL
        SECURITY_STATUS status = SecurityContextSsl::Test_VerifyCertificate(certificate.get(), commonName);
        VERIFY_ARE_EQUAL2(status, CERT_E_EXPIRED);
    }

    Trace.WriteInfo(TraceType, "loading a revoked certificate ...");
    {
        int savedValue = SecurityConfig::GetConfig().CrlCheckingFlag;
        SecurityConfig::GetConfig().CrlCheckingFlag = 0x40000000;
        KFinally([=] { SecurityConfig::GetConfig().CrlCheckingFlag = savedValue; });

        shared_ptr<SubjectName> subjectName;
        auto error = SubjectName::Create(L"CN=WinFabric-Test-SAN9-Revoked", subjectName);
        VERIFY_IS_TRUE(error.IsSuccess());

        CertContextUPtr certificate;
        error = CryptoUtility::GetCertificate(
            X509Default::StoreLocation(), 
            X509Default::StoreName(),
            subjectName,
            certificate);

        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(certificate != nullptr);

        wstring commonName;
        error = CryptoUtility::GetCertificateCommonName(certificate.get(), commonName);
        VERIFY_IS_TRUE(error.IsSuccess());

        // Verify the certificate has proper subject name and subject alternative name (if any), and check CRL
        SECURITY_STATUS status = SecurityContextSsl::Test_VerifyCertificate(certificate.get(), commonName);
        VERIFY_ARE_EQUAL2(status, CRYPT_E_REVOKED);
    }

    LEAVE;
}

BOOST_AUTO_TEST_CASE(X509CertRemoteCheckingTests)
{
    ENTER;

    bool saved = SecurityConfig::GetConfig().SkipX509CredentialExpirationChecking;
    SecurityConfig::GetConfig().SkipX509CredentialExpirationChecking = true;
    KFinally([=] { SecurityConfig::GetConfig().SkipX509CredentialExpirationChecking = saved;});

    Trace.WriteInfo(TraceType, "testing with a server auth certificate");
    {
        SecuritySettings senderSecuritySettings = TTestUtil::CreateX509Settings(
            L"CN=WinFabric-Test-SAN1-Bob",
            L"WinFabric-Test-SAN2-Charlie");

        X509CertChecking(senderSecuritySettings);
    }

    Trace.WriteInfo(TraceType, "testing with a client auth certificate");
    {
        SecuritySettings senderSecuritySettings = TTestUtil::CreateX509Settings(
            L"WinFabric-Test-User@microsoft.com",
            L"WinFabric-Test-SAN2-Charlie");

        X509CertChecking(senderSecuritySettings);
    }

    Trace.WriteInfo(TraceType, "testing with an expired certificate");
    {
        SecuritySettings senderSecuritySettings = TTestUtil::CreateX509SettingsExpired(L"WinFabric-Test-SAN1-Alice");
        X509CertChecking(senderSecuritySettings, false, true);
    }

    Trace.WriteInfo(TraceType, "testing with a revoked certificate");
    {
        int savedValue = SecurityConfig::GetConfig().CrlCheckingFlag;
        SecurityConfig::GetConfig().CrlCheckingFlag = 0x40000000;
        KFinally([=] { SecurityConfig::GetConfig().CrlCheckingFlag = savedValue; });

        SecuritySettings senderSecuritySettings = TTestUtil::CreateX509Settings(
            L"CN=WinFabric-Test-SAN9-Revoked",
            L"WinFabric-Test-SAN1-Alice");

        X509CertChecking(senderSecuritySettings, false, true);
    }

    LEAVE;
}

BOOST_AUTO_TEST_CASE(X509CredentialExpirationChecking)
{
    ENTER;

    SecuritySettings securitySettings = TTestUtil::CreateX509SettingsExpired();
    auto transport = TcpDatagramTransport::CreateClient();
    VERIFY_IS_TRUE(transport->SetSecurity(securitySettings).IsError(ErrorCodeValue::InvalidCredentials));

    LEAVE;
}

BOOST_AUTO_TEST_CASE(X509CertRemoteCheckingTests_ClientCert)
{
    ENTER;

    SecuritySettings senderSecuritySettings = TTestUtil::CreateX509Settings2(
        L"6f 4a a9 61 8a ea 95 ab 5d ce 8c 77 26 09 38 65 3e e1 5f d7",
        L"WinFabric-Test-SAN2-Charlie");

    X509CertChecking(senderSecuritySettings);

    LEAVE;
}

BOOST_AUTO_TEST_CASE(SelfSignedCertManagement)
{
    ENTER;

    wstring commonName = Guid::NewGuid().ToString();
    wstring const trustedPeopleStore = L"TrustedPeople";
    Thumbprint::SPtr thumbprint;

    Trace.WriteInfo(TraceType, "Testing certificate create: CN = {0}", commonName);
    CertContextUPtr certCreated;
    {
        auto error = CryptoUtility::CreateSelfSignedCertificate(
            L"CN=" + commonName,
            DateTime::Now() + InstallTestCertInScope::DefaultCertExpiry(),
            certCreated);

        VERIFY_IS_TRUE(error.IsSuccess());

        wstring certCommonName;
        error = CryptoUtility::GetCertificateCommonName(certCreated.get(), certCommonName);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_ARE_EQUAL2(commonName, certCommonName);

        error = Thumbprint::Create(certCreated.get(), thumbprint);
        VERIFY_IS_TRUE(error.IsSuccess());
        Trace.WriteInfo(TraceType, "certificate thumbprint: {0}", thumbprint);

        auto status = SecurityContextSsl::Test_VerifyCertificate(certCreated.get(), commonName);
        VERIFY_IS_TRUE(status == ErrorCodeValue::CertificateNotMatched); // self signed

        Trace.WriteInfo(TraceType, "Testing certificate install");
        error = CryptoUtility::InstallCertificate(
            certCreated,
            X509Default::StoreLocation(),
            trustedPeopleStore);

        VERIFY_IS_TRUE(error.IsSuccess());

        for (int i = 0; i < 10; ++i)
        {
            Trace.WriteInfo(TraceType, "Try loading installed certificate");
            CertContextUPtr certLoaded;
            error = CryptoUtility::GetCertificate(
                X509Default::StoreLocation(),
                trustedPeopleStore,
                thumbprint,
                certLoaded);

            VERIFY_IS_TRUE(error.IsSuccess());

            status = SecurityContextSsl::Test_VerifyCertificate(certCreated.get(), commonName);
            if (status == SEC_E_OK) break;

            Trace.WriteError(TraceType, "Certificate verification failed: {0}", error);
            error = CryptoUtility::GetCertificate(
                X509Default::StoreLocation(),
                trustedPeopleStore,
                thumbprint,
                certLoaded);

            Trace.WriteInfo(TraceType, "loading certificate from trusted people returned {0}", error);
            // trusted-people based chain validation fails intermittently on Azure VMs, re-enable the following
            // verification once the issue is figured out, tracking bug:
            // RD: RDBug 4359599: [cit] SecurityTest::FullCertChainValidationWithThumbprintAuth faliure
            //VERIFY_IS_TRUE(false);

            ::Sleep(100);
        }

        Trace.WriteInfo(TraceType, "Certificate now chained to trusted people");
    }

    Trace.WriteInfo(TraceType, "Testing certificate uninstall");
    auto error = CryptoUtility::UninstallCertificate(
        X509Default::StoreLocation(),
        trustedPeopleStore,
        thumbprint);

    VERIFY_IS_TRUE(error.IsSuccess());

    Trace.WriteInfo(TraceType, "load should fail now");
    CertContextUPtr certLoaded;
    error = CryptoUtility::GetCertificate(
        X509Default::StoreLocation(),
        trustedPeopleStore,
        thumbprint,
        certLoaded);

    VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::CertificateNotFound));

    LEAVE;
}

BOOST_AUTO_TEST_CASE(ExpiredCertCredLoadTest)
{
    ENTER;

    InstallTestCertInScope expiredCert(L"CN=Expired@ExpiredCertCredLoadTest", nullptr, DateTime::Zero - DateTime::Now());

    auto secSettings = TTestUtil::CreateX509SettingsTp(
        expiredCert.Thumbprint()->PrimaryToString(),
        L"",
        expiredCert.Thumbprint()->PrimaryToString(),
        L"");

    Trace.WriteInfo(TraceType, "create server transport");
    auto client = DatagramTransportFactory::CreateTcpClient();
    auto error = client->SetSecurity(secSettings);
    VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidCredentials));

    LEAVE;
}

BOOST_AUTO_TEST_CASE(IssuerPinningByPubKey)
{
    ENTER;

    wstring clientCName = L"WinFabric-Test-SAN2-Charlie";
    wstring serverCName = L"WinFabric-Test-SAN3-Oscar";

    CommonName::SPtr issuerCommonName;
    auto error = CommonName::Create(L"WinFabric-Test-TA-CA", issuerCommonName);
    VERIFY_ARE_EQUAL2(error.ReadValue(), ErrorCodeValue::Success);

    CertContextUPtr issuerCert;
    error = CryptoUtility::GetCertificate(
        X509Default::StoreLocation(),
        L"Root",
        issuerCommonName,
        issuerCert);
    VERIFY_ARE_EQUAL2(error.ReadValue(), ErrorCodeValue::Success);

    SecurityConfig::X509NameMap serverX509Name;
    serverX509Name.Add(serverCName, make_shared<X509PubKey>(issuerCert.get()));

    SecuritySettings clientSecuritySettings;
    error = SecuritySettings::FromConfiguration(
        L"X509",
        X509Default::StoreName(),
        wformatString(X509Default::StoreLocation()),
        wformatString(X509FindType::FindBySubjectName),
        clientCName,
        L"",
        wformatString(ProtectionLevel::EncryptAndSign),
        L"",
        serverX509Name,
        SecurityConfig::IssuerStoreKeyValueMap(),
        L"",
        L"",
        L"",
        L"",
        clientSecuritySettings);

    ASSERT_IFNOT(error.IsSuccess(), "SecuritySettings::FromConfiguration failed: {0}", error);

    SecurityConfig::X509NameMap clientX509Name;
    clientX509Name.Add(clientCName, make_shared<X509PubKey>(issuerCert.get()));

    SecuritySettings serverSecuritySettings;
    error = SecuritySettings::FromConfiguration(
        L"X509",
        X509Default::StoreName(),
        wformatString(X509Default::StoreLocation()),
        wformatString(X509FindType::FindBySubjectName),
        serverCName,
        L"",
        wformatString(ProtectionLevel::EncryptAndSign),
        L"",
        clientX509Name,
        SecurityConfig::IssuerStoreKeyValueMap(),
        L"",
        L"",
        L"",
        L"",
        serverSecuritySettings);

    ASSERT_IFNOT(error.IsSuccess(), "SecuritySettings::FromConfiguration failed: {0}", error);

    auto server = DatagramTransportFactory::CreateTcp(L"127.0.0.1:0");
    error = server->SetSecurity(serverSecuritySettings);
    VERIFY_IS_TRUE(error.IsSuccess());

    wstring action = Guid::NewGuid().ToString();
    AutoResetEvent serverReceivedMessage;
    TTestUtil::SetMessageHandler(
        server,
        action,
        [&server, &action, &serverReceivedMessage](MessageUPtr & message, ISendTarget::SPtr const &) -> void
        {
            Trace.WriteInfo(TraceType, "server received {0}", message->TraceId());
            serverReceivedMessage.Set();
        });

    error = server->Start();
    VERIFY_IS_TRUE(error.IsSuccess());

    auto client = DatagramTransportFactory::CreateTcpClient();
    error = client->SetSecurity(clientSecuritySettings);
    VERIFY_IS_TRUE(error.IsSuccess());

    error = client->Start();
    VERIFY_IS_TRUE(error.IsSuccess());

    auto target = client->ResolveTarget(server->ListenAddress());
    VERIFY_IS_TRUE(target != nullptr);

    auto message = make_unique<Message>();
    message->Headers.Add(ActionHeader(action));
    message->Headers.Add(MessageIdHeader());

    error = client->SendOneWay(target, move(message));
    Trace.WriteInfo(TraceType, "client SendOneWay returned {0}", error);

    VERIFY_IS_TRUE(serverReceivedMessage.WaitOne(TimeSpan::FromSeconds(3)));

    LEAVE;
}

BOOST_AUTO_TEST_CASE(ThumbprintMatchWithExpiredSelfSignedCert)
{
    auto saved = SecurityConfig::GetConfig().SkipX509CredentialExpirationChecking;
    SecurityConfig::GetConfig().SkipX509CredentialExpirationChecking = true;
    KFinally([&] { SecurityConfig::GetConfig().SkipX509CredentialExpirationChecking = saved; });

    InstallTestCertInScope serverCert(L"CN=server.TestWithExpiredSelfSignedCert");
    InstallTestCertInScope clientCert(L"CN=client@TestWithExpiredSelfSignedCert", nullptr, DateTime::Zero - DateTime::Now());

    auto svrSecSettings = TTestUtil::CreateX509SettingsTp(
        serverCert.Thumbprint()->PrimaryToString(),
        L"",
        clientCert.Thumbprint()->PrimaryToString(),
        L"");

    Trace.WriteInfo(TraceType, "create server transport");
    auto server = DatagramTransportFactory::CreateTcp(L"127.0.0.1:0");
    auto error = server->SetSecurity(svrSecSettings);
    VERIFY_IS_TRUE(error.IsSuccess());

    wstring action = Guid::NewGuid().ToString();
    AutoResetEvent serverReceivedMessage;
    TTestUtil::SetMessageHandler(
        server,
        action,
        [&server, &action, &serverReceivedMessage](MessageUPtr & message, ISendTarget::SPtr const &) -> void
        {
            Trace.WriteInfo(TraceType, "server received {0}", message->TraceId());
            serverReceivedMessage.Set();
        });

    auto cltSecSettings = TTestUtil::CreateX509SettingsTp(
        clientCert.Thumbprint()->PrimaryToString(),
        L"",
        serverCert.Thumbprint()->PrimaryToString(),
        L"");

    auto client = DatagramTransportFactory::CreateTcpClient();
    error = client->SetSecurity(cltSecSettings);
    VERIFY_IS_TRUE(error.IsSuccess());

    error = server->Start();
    VERIFY_IS_TRUE(error.IsSuccess());
    error = client->Start();
    VERIFY_IS_TRUE(error.IsSuccess());

    auto target = client->ResolveTarget(server->ListenAddress());
    VERIFY_IS_TRUE(target != nullptr);

    AutoResetEvent connectionFaulted(false);
    client->SetConnectionFaultHandler([&connectionFaulted, &target](ISendTarget const & faultedTarget, ErrorCode fault)
    {
        Trace.WriteInfo(TraceType, "connection to {0} faulted: {1}:{2}", faultedTarget.Address(), fault, fault.Message);
        VERIFY_IS_TRUE(target.get() == &faultedTarget);
        VERIFY_IS_TRUE(fault.IsError(ErrorCodeValue::ConnectionDenied));
        connectionFaulted.Set();
    });

    auto message = make_unique<Message>();
    message->Headers.Add(ActionHeader(action));
    message->Headers.Add(MessageIdHeader());

    error = client->SendOneWay(target, move(message));
    Trace.WriteInfo(TraceType, "client SendOneWay returned {0}", error);

    VERIFY_IS_FALSE(serverReceivedMessage.WaitOne(TimeSpan::FromMilliseconds(100)));
    VERIFY_IS_TRUE(connectionFaulted.WaitOne(TimeSpan::FromSeconds(3)));
}

static void ThumbprintTestWithRevokedClientCert(bool verifyChain) 
{
    int savedValue = SecurityConfig::GetConfig().CrlCheckingFlag;
    SecurityConfig::GetConfig().CrlCheckingFlag = 0x40000000;
    KFinally([=] { SecurityConfig::GetConfig().CrlCheckingFlag = savedValue; });

    InstallTestCertInScope serverCert(L"CN=server.ThumbprintTestWithRevokedClientCert");

    auto clientCertThumbprint = wformatString("9f 1b 74 0d 5a fc 49 eb cd a9 62 dd ef 65 bc 05 b9 57 2a 7c?{0}", verifyChain);
    auto svrSecSettings = TTestUtil::CreateX509SettingsTp(
        serverCert.Thumbprint()->PrimaryToString(),
        L"",
        clientCertThumbprint,
        L"");

    Trace.WriteInfo(TraceType, "create server transport");
    auto server = DatagramTransportFactory::CreateTcp(L"127.0.0.1:0");
    auto error = server->SetSecurity(svrSecSettings);
    VERIFY_IS_TRUE(error.IsSuccess());

    wstring action = Guid::NewGuid().ToString();
    AutoResetEvent serverReceivedMessage;
    TTestUtil::SetMessageHandler(
        server,
        action,
        [&server, &action, &serverReceivedMessage](MessageUPtr & message, ISendTarget::SPtr const &) -> void
        {
            Trace.WriteInfo(TraceType, "server received {0}", message->TraceId());
            serverReceivedMessage.Set();
        });

    auto cltSecSettings = TTestUtil::CreateX509SettingsTp(
        clientCertThumbprint,
        L"",
        serverCert.Thumbprint()->PrimaryToString(),
        L"");

    auto client = DatagramTransportFactory::CreateTcpClient();
    error = client->SetSecurity(cltSecSettings);
    VERIFY_IS_TRUE(error.IsSuccess());

    error = server->Start();
    VERIFY_IS_TRUE(error.IsSuccess());
    error = client->Start();
    VERIFY_IS_TRUE(error.IsSuccess());

    auto target = client->ResolveTarget(server->ListenAddress());
    VERIFY_IS_TRUE(target != nullptr);

    AutoResetEvent connectionFaulted(false);
    client->SetConnectionFaultHandler([&connectionFaulted, &target](ISendTarget const & faultedTarget, ErrorCode fault)
    {
        Trace.WriteInfo(TraceType, "connection to {0} faulted: {1}:{2}", faultedTarget.Address(), fault, fault.Message);
        VERIFY_IS_TRUE(target.get() == &faultedTarget);
        VERIFY_IS_TRUE(fault.IsError(ErrorCodeValue::ConnectionDenied));
        connectionFaulted.Set();
    });

    auto message = make_unique<Message>();
    message->Headers.Add(ActionHeader(action));
    message->Headers.Add(MessageIdHeader());

    error = client->SendOneWay(target, move(message));
    Trace.WriteInfo(TraceType, "client SendOneWay returned {0}", error);

    VERIFY_IS_TRUE(connectionFaulted.WaitOne(TimeSpan::FromSeconds(3)));
    VERIFY_IS_FALSE(serverReceivedMessage.WaitOne(TimeSpan::FromMilliseconds(100)));
}

BOOST_AUTO_TEST_CASE(ThumbprintMatchOnly)
{
    ENTER;
    ThumbprintTestWithRevokedClientCert(false);
    LEAVE;
}

BOOST_AUTO_TEST_CASE(ThumbprintMatchWithChainValidation)
{
    ENTER;
    ThumbprintTestWithRevokedClientCert(true);
    LEAVE;
}

#endif

BOOST_AUTO_TEST_SUITE_END()

void SecurityTest::LoadByFullSubjectNameVsCommonName(wstring const & commonName, wstring const & fullSubjectName)
{
    InstallTestCertInScope cert(fullSubjectName);
    X509FindValue::SPtr findValue;
    auto error = X509FindValue::Create(X509FindType::FindBySubjectName, fullSubjectName, findValue);
    VERIFY_IS_TRUE(error.IsSuccess());
    {
        CertContextUPtr certificate;
        error = CryptoUtility::GetCertificate(
            X509Default::StoreLocation(), 
            X509Default::StoreName(),
            findValue,
            certificate);

        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(certificate.get() != nullptr);

        Thumbprint loadedThumbprint;
        error = loadedThumbprint.Initialize(certificate.get());
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(loadedThumbprint == *cert.Thumbprint());

        wstring certCommonName;
        error = CryptoUtility::GetCertificateCommonName(certificate.get(), certCommonName);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(commonName, certCommonName));
    }

    X509FindValue::SPtr findValue2;
    error = X509FindValue::Create(X509FindType::FindBySubjectName, commonName, findValue2);
    VERIFY_IS_TRUE(error.IsSuccess());
    {
        CertContextUPtr certificate;
        error = CryptoUtility::GetCertificate(
            X509Default::StoreLocation(), 
            X509Default::StoreName(),
            findValue2,
            certificate);

        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(certificate.get() != nullptr);

        Thumbprint loadedThumbprint;
        error = loadedThumbprint.Initialize(certificate.get());
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(loadedThumbprint == *cert.Thumbprint());

        wstring certCommonName;
        error = CryptoUtility::GetCertificateCommonName(certificate.get(), certCommonName);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(commonName, certCommonName));
    }
}

void SecurityTest::LoadBySubjectAltName(vector<wstring> const & subjectAltNames)
{
    InstallTestCertInScope cert(L"", &subjectAltNames);

    for (int i = 0; i < subjectAltNames.size(); i++)
    {
        // Try searching search string as is, its lower case variant and upper case varient.
        for (int j = 0; j < 3; j++)
        {
            wstring searchString = subjectAltNames[i];
            switch (j)
            {
            case 1:
                // Convert search string to lower case.
                StringUtility::ToLower(searchString);
                break;
            case 2:
                // Convert search string to upper case.
                StringUtility::ToUpper(searchString);
                break;
            }

            X509FindValue::SPtr findValue;
            wstring findValueString = L"2.5.29.17:3=" + searchString;
            auto error = X509FindValue::Create(X509FindType::FindByExtension, findValueString, findValue);
            VERIFY_IS_TRUE(error.IsSuccess());
            {
                CertContextUPtr certificate;
                error = CryptoUtility::GetCertificate(
                    X509Default::StoreLocation(), 
                    X509Default::StoreName(),
                    findValue,
                    certificate);

                VERIFY_IS_TRUE(error.IsSuccess());
                VERIFY_IS_TRUE(certificate.get() != nullptr);

                Thumbprint loadedThumbprint;
                error = loadedThumbprint.Initialize(certificate.get());
                VERIFY_IS_TRUE(error.IsSuccess());
                VERIFY_IS_TRUE(loadedThumbprint == *cert.Thumbprint());
            }
        }
    }
}

void SecurityTest::CrlTest(wstring const & senderCN, wstring const & receiverCN, bool ignoreCrlOffline)
{
    auto& config = SecurityConfig::GetConfig();
    auto saved0 = config.CrlCheckingFlag;
    config.CrlCheckingFlag = 0x40000000;
    auto saved1 = config.CrlTestEnabled;
    config.CrlTestEnabled = true;
    auto saved2 = config.CrlOfflineProbability;
    config.CrlOfflineProbability = 1.0;
    auto saved4 = config.CrlDisablePeriod;
    config.CrlDisablePeriod = TimeSpan::FromSeconds(3);
    KFinally([&]
    {
        config.CrlCheckingFlag = saved0;
        config.CrlTestEnabled = saved1;
        config.CrlOfflineProbability = saved2;
        config.CrlDisablePeriod = saved4;
    });

    auto& crlOfflineCache = CrlOfflineErrCache::GetDefault();
    crlOfflineCache.Test_Reset();

    auto offlineReported = make_shared<AutoResetEvent>(false);
    auto recoveryReported = make_shared<AutoResetEvent>(false);
    auto bothCertsAdded = make_shared<AutoResetEvent>(false);
    crlOfflineCache.SetHealthReportCallback([=](size_t cacheSize, wstring const&)
    {
        Trace.WriteInfo(TraceType, "HealthReport callback called: cacheSize = {0}", cacheSize);

        if (cacheSize == 0)
        {
            recoveryReported->Set();
            return;
        }

        if (cacheSize == 1)
        {
            offlineReported->Set();
            return;
        }

        if (cacheSize == 2)
        {
            bothCertsAdded->Set();
            return;
        }
    });

    SecurityConfig::X509NameMap senderNames;
    senderNames.Add(senderCN);
    auto receiverSettings = TTestUtil::CreateX509Settings(receiverCN, senderNames);
    receiverSettings.SetCrlOfflineIgnoreCallback([=](bool) { return ignoreCrlOffline; });

    auto receiver = TcpDatagramTransport::Create(TTestUtil::GetListenAddress());
    auto error = receiver->SetSecurity(receiverSettings);
    VERIFY_IS_TRUE(error.IsSuccess());

    AutoResetEvent messageReceived;
    wstring testAction = TTestUtil::GetGuidAction();
    TTestUtil::SetMessageHandler(
        receiver,
        testAction,
        [&messageReceived](MessageUPtr & message, ISendTarget::SPtr const & st)
        {
            Trace.WriteInfo(TraceType, "[receiver] got message {0} from {1}", message->MessageId, st->Address());
            messageReceived.Set();
        });

    error = receiver->Start();
    VERIFY_IS_TRUE(error.IsSuccess());

    SecurityConfig::X509NameMap receiverNames;
    receiverNames.Add(receiverCN);
    auto senderSettings = TTestUtil::CreateX509Settings(senderCN, receiverNames);
    senderSettings.SetCrlOfflineIgnoreCallback([=](bool) { return ignoreCrlOffline; });

    auto sender = TcpDatagramTransport::CreateClient();
    error = sender->SetSecurity(senderSettings);
    VERIFY_IS_TRUE(error.IsSuccess());

    error = sender->Start();
    VERIFY_IS_TRUE(error.IsSuccess());

    auto target = sender->ResolveTarget(receiver->ListenAddress());
    auto message = make_unique<Message>();
    message->Headers.Add(MessageIdHeader());
    message->Headers.Add(ActionHeader(testAction));
    Trace.WriteInfo(TraceType, "send first message");
    error = sender->SendOneWay(target, move(message));
    VERIFY_IS_TRUE(error.IsSuccess());

    Trace.WriteInfo(TraceType, "wait for offline report");
    VERIFY_IS_TRUE(offlineReported->WaitOne(TimeSpan::FromSeconds(10)));
    Trace.WriteInfo(TraceType, "wait for double offline report");
    VERIFY_IS_TRUE(bothCertsAdded->WaitOne(TimeSpan::FromSeconds(10)));

    if (ignoreCrlOffline)
    {
        VERIFY_IS_TRUE(messageReceived.WaitOne(TimeSpan::FromSeconds(10)));
    }
    else
    {
        VERIFY_IS_FALSE(messageReceived.WaitOne(TimeSpan::FromSeconds(1)));
    }

    Trace.WriteInfo(TraceType, "close connections before sending again");
    target->Reset();

    message = make_unique<Message>();
    message->Headers.Add(MessageIdHeader());
    message->Headers.Add(ActionHeader(testAction));
    Trace.WriteInfo(TraceType, "send second message");
    error = sender->SendOneWay(target, move(message));
    VERIFY_IS_TRUE(error.IsSuccess());

    if (ignoreCrlOffline)
    {
        VERIFY_IS_TRUE(messageReceived.WaitOne(TimeSpan::FromSeconds(10)));
    }
    else
    {
        VERIFY_IS_FALSE(messageReceived.WaitOne(TimeSpan::FromSeconds(1)));
    }

    Trace.WriteInfo(TraceType, "crl recovery");
    config.CrlOfflineProbability = 0.0;

    Trace.WriteInfo(TraceType, "wait for recovery report");
    VERIFY_IS_TRUE(recoveryReported->WaitOne(config.CrlDisablePeriod + TimeSpan::FromSeconds(3)));

    Trace.WriteInfo(TraceType, "close connections before sending again");
    target->Reset();

    message = make_unique<Message>();
    message->Headers.Add(MessageIdHeader());
    message->Headers.Add(ActionHeader(testAction));
    Trace.WriteInfo(TraceType, "send after crl recovery");
    error = sender->SendOneWay(target, move(message));
    VERIFY_IS_TRUE(error.IsSuccess());

    VERIFY_IS_TRUE(messageReceived.WaitOne(TimeSpan::FromSeconds(10)));

    Trace.WriteInfo(TraceType, "there should be no more reports");
    VERIFY_IS_FALSE(offlineReported->WaitOne(TimeSpan::FromSeconds(1)));
    VERIFY_IS_FALSE(bothCertsAdded->WaitOne(TimeSpan::FromSeconds(1)));
    VERIFY_IS_FALSE(recoveryReported->WaitOne(TimeSpan::FromSeconds(1)));
}

void SecurityTest::X509CertChecking(SecuritySettings const & senderSecuritySettings, bool localChecking, bool failureExpected)
{
    SecuritySettings receiverSecuritySettings = TTestUtil::CreateX509Settings(
        L"CN=WinFabric-Test-SAN2-Charlie",
        L"WinFabric-Test-SAN1-Bob,WinFabric-Test-Expired,WinFabric-Test-SAN9-Revoked,WiNfAbRiC-tEsT-uSeR@mIcRoSoFt.CoM");

    auto receiver = TcpDatagramTransport::Create(TTestUtil::GetListenAddress());
    VERIFY_IS_TRUE(receiver != nullptr);

    auto error = receiver->SetSecurity(receiverSecuritySettings);
    VERIFY_IS_TRUE(error.IsSuccess());

    AutoResetEvent messageReceived;
    wstring testAction = TTestUtil::GetGuidAction();
    TTestUtil::SetMessageHandler(
        receiver,
        testAction,
        [&messageReceived](MessageUPtr & message, ISendTarget::SPtr const & st)
        {
            Trace.WriteInfo(TraceType, "[receiver] got message {0} from {1}", message->MessageId, st->Address());
            messageReceived.Set();
        });

    error = receiver->Start();
    VERIFY_IS_TRUE(error.IsSuccess());

    auto sender = TcpDatagramTransport::CreateClient();
    VERIFY_IS_TRUE(sender != nullptr);

    error = sender->SetSecurity(senderSecuritySettings);
    if (!error.IsSuccess())
    {
        VERIFY_IS_TRUE(localChecking && failureExpected);
        return;
    }

    error = sender->Start();
    VERIFY_IS_TRUE(error.IsSuccess());

    auto sendTarget = sender->ResolveTarget(receiver->ListenAddress());
    VERIFY_IS_TRUE(sendTarget != nullptr);

    auto message = make_unique<Message>();
    message->Headers.Add(ActionHeader(testAction));
    message->Headers.Add(MessageIdHeader());

    error = sender->SendOneWay(sendTarget, move(message));
    VERIFY_IS_TRUE(error.IsSuccess() == !(failureExpected && localChecking));

    if (failureExpected)
        VERIFY_IS_TRUE(!messageReceived.WaitOne(TimeSpan::FromSeconds(1)))
    else
        VERIFY_IS_TRUE(messageReceived.WaitOne(TimeSpan::FromSeconds(1)))
}

void SecurityTest::FindValueSecondaryTestWithThumbprint(wstring const & primary, wstring const & secondary, int expected)
{
    X509FindValue::SPtr thumbprint;
    auto error = X509FindValue::Create(X509FindType::FindByThumbprint, primary, secondary, thumbprint);
    VERIFY_IS_TRUE(error.IsSuccess());

    Thumbprint primaryThumbprint = *((Thumbprint*)thumbprint.get());
    Thumbprint secondaryThumbprint = *((Thumbprint*)thumbprint->Secondary().get());

    vector<SecurityCredentialsSPtr> cred, svrCred;
    error = SecurityCredentials::AcquireSsl(
        X509Default::StoreLocation(), 
        X509Default::StoreName(),
        thumbprint,
        &cred,
        &svrCred);

    if (expected < 0)
    {
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::CertificateNotFound));
        return;
    }

    VERIFY_IS_TRUE(error.IsSuccess());

    Thumbprint expectedThumbprint = (expected == 0) ? primaryThumbprint : secondaryThumbprint;

    Trace.WriteInfo(TraceType, "expected thumbprint = {0}, loaded thumbprint = {1}", expectedThumbprint, svrCred.front()->X509CertThumbprint());
    VERIFY_ARE_EQUAL2(expectedThumbprint, svrCred.front()->X509CertThumbprint());
}

void SecurityTest::FindValueSecondaryTestWithCommonName(wstring const & primary, wstring const & secondary, int expected)
{
    X509FindValue::SPtr subjectNameFindValue;
    auto error = X509FindValue::Create(
        X509FindType::FindBySubjectName,
        L"CN=" + primary,
        L"CN=" + secondary,
        subjectNameFindValue);

    VERIFY_IS_TRUE(error.IsSuccess());

    vector<SecurityCredentialsSPtr> cred;
    error = SecurityCredentials::AcquireSsl(
        X509Default::StoreLocation(), 
        X509Default::StoreName(),
        subjectNameFindValue,
        &cred,
        nullptr);

    if (expected < 0)
    {
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::CertificateNotFound));
        return;
    }

    VERIFY_IS_TRUE(error.IsSuccess());

    wstring nameLoaded;
    error = CryptoUtility::GetCertificateCommonName(
        X509Default::StoreLocation(), 
        X509Default::StoreName(),
        wformatString(X509FindType::FindBySubjectName),
        (expected == 0) ? primary : secondary,
        nameLoaded);

    VERIFY_IS_TRUE(error.IsSuccess());

    wstring nameExpected = (expected == 0) ? primary : secondary;
    Trace.WriteInfo(TraceType, "expected name = {0}, loaded name = {1}", nameExpected, nameLoaded);
    VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(nameLoaded, nameExpected));
}

#ifndef PLATFORM_UNIX

void SecurityTest::FullCertChainValidationWithThumbprintAuth_SelfSignedCert(
    wstring const & certStoreName,
    bool expectFailureOnFullCertChainValidation)
{
    auto storeLocation = X509Default::StoreLocation();

    bool allTestPassed = false;
    for(int i = 0; i < 3; ++i)
    {
        InstallTestCertInScope selfSignedCert(
            L"CN=SelfSigned@FullCertChainValidationWithThumbprintAuth",
            nullptr,
            InstallTestCertInScope::DefaultCertExpiry(),
            certStoreName,
            L"",
            storeLocation); 

        X509FindValue::SPtr findValue;
        auto error = X509FindValue::Create(X509FindType::FindByThumbprint, wformatString(*(selfSignedCert.Thumbprint())), findValue);
        VERIFY_IS_TRUE(error.IsSuccess());
        CertContextUPtr certificate;
        error = CryptoUtility::GetCertificate(
            storeLocation, 
            certStoreName,
            findValue,
            certificate);

        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(certificate.get() != nullptr);

        {
            ThumbprintSet thumbprintSetToMatch;
            error = thumbprintSetToMatch.Add(wformatString(*(selfSignedCert.Thumbprint())) + L"?false");
            VERIFY_IS_TRUE(error.IsSuccess());

            SECURITY_STATUS status = SecurityContextSsl::VerifyCertificate(
                L"FullCertChainValidationWithThumbprintAuth_SelfSignedCert_false",
                certificate.get(),
                SecurityConfig::GetConfig().CrlCheckingFlag,
                false,
                false,
                thumbprintSetToMatch,
                SecurityConfig::X509NameMap(),
                true);

            VERIFY_IS_TRUE(status == SEC_E_OK);
        }

        {
            auto saved = SecurityConfig::GetConfig().CrlCheckingFlag;
            KFinally([=] { SecurityConfig::GetConfig().CrlCheckingFlag = saved; });
            if (!expectFailureOnFullCertChainValidation)
            {
                SecurityConfig::GetConfig().CrlCheckingFlag |= CERT_CHAIN_ENABLE_PEER_TRUST;
            }

            ThumbprintSet thumbprintSetToMatch;
            error = thumbprintSetToMatch.Add(wformatString(*(selfSignedCert.Thumbprint())) + L"?true");
            VERIFY_IS_TRUE(error.IsSuccess());

            SECURITY_STATUS status = SecurityContextSsl::VerifyCertificate(
                L"FullCertChainValidationWithThumbprintAuth_SelfSignedCert_true",
                certificate.get(),
                SecurityConfig::GetConfig().CrlCheckingFlag,
                false,
                false,
                thumbprintSetToMatch,
                SecurityConfig::X509NameMap(),
                true);

            if (expectFailureOnFullCertChainValidation)
                VERIFY_IS_TRUE(status == ErrorCodeValue::CertificateNotMatched)
            else if (!StringUtility::AreEqualCaseInsensitive(certStoreName, X509Default::StoreName())) // root or trusted people
            {
                if (status != SEC_E_OK)
                {
                    CertContextUPtr cert;
                    error = CryptoUtility::GetCertificate(
                        storeLocation, 
                        certStoreName,
                        findValue,
                        certificate);

                    Trace.WriteInfo(TraceType, "GetCertificate returned {0}", error);
                    VERIFY_IS_TRUE(!error.IsSuccess());
                    Trace.WriteWarning(TraceType, "test cert was missing for unknown reason");
                    continue;
                }
            }
        }

        allTestPassed = true;
        break;
    }

    VERIFY_IS_TRUE(allTestPassed);
}

#endif

void SecurityTest::SendTestMsg(
    IDatagramTransportSPtr const & sender,
    ISendTarget::SPtr const & target,
    wstring const & msgAction)
{
    auto msg = make_unique<Message>();
    msg->Headers.Add(MessageIdHeader());
    msg->Headers.Add(ActionHeader(msgAction));

    target->Reset(); // close existing connections if any
    auto error = sender->SendOneWay(target, move(msg));
    VERIFY_IS_TRUE(error.IsSuccess());
}
