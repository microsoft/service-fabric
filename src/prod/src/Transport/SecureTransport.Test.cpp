// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <thread>
#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TestCommon.h"
#include "Common/CryptoTest.h"
#include "Common/Types.h"

using namespace Common;
using namespace std;

namespace Transport
{
    class SecureTransportTests
    {
    protected:
        void X509Test_SelfSigned(
            std::wstring const & senderAddress,
            std::wstring const & receiverAddress);

        void X509ManyMessage_SelfSigned(
            std::wstring const & senderAddress,
            std::wstring const & receiverAddress);

        void X509ManySmallMessage_SelfSigned(
            std::wstring const & senderAddress,
            std::wstring const & receiverAddress);

        void BasicTestWin(
            SecurityProvider::Enum provider,
            std::wstring const & senderAddress,
            std::wstring const & receiverAddress,
            ProtectionLevel::Enum protectionLevel);

        void SecurityTest(
            std::wstring const & senderAddress,
            SecuritySettings const & senderSecSettings,
            std::wstring const & receiverAddress,
            SecuritySettings const & receiverSecSettings,
            bool failureExpected = false);

        void ManyMessageTest(
            std::wstring const & senderAddress,
            SecuritySettings const & senderSecSettings,
            std::wstring const & receiverAddress,
            SecuritySettings const & receiverSecSettings);

        void SignTestWithDeletedHeaders(size_t outgoingHeaderChunkSize, bool deleteHeader = true);
        // Added for SQL Server: Defect 1169842: Transport security: 
        // signature only protection is broken when there are deleted message headers

        void ManySmallMessageTest(
            std::wstring const & senderAddress,
            SecuritySettings const & senderSecSettings,
            std::wstring const & receiverAddress,
            SecuritySettings const & receiverSecSettings);

        void EncryptionTestX509(ULONG outgoingHeaderChunkSize);

        void ClientServerMessageSizeLimitTest_WithoutClientRole(
            bool disableIncomingMessageSizeLimitOnClient,
            bool setOutgoingMessageSizeLimitOnServer);

        void NegotiationTimeoutTest(SecurityProvider::Enum securityProvider);

        void AdminRoleTest(SecuritySettings & securitySettings, bool enable, bool enableCorrectClientId);

        static void ClaimAuthTestClientSend(
            std::wstring const & receiverAddress,
            std::wstring const & clientClaims,
            bool expectFailure = false,
            bool clientIsAdmin = false,
            std::wstring const clientCertFindValue = L"",
            Common::SecurityConfig::IssuerStoreKeyValueMap clientIssuerStore = Common::SecurityConfig::IssuerStoreKeyValueMap());

        static void ClaimAuthTestClientSend2(
            std::wstring const & receiverAddress,
            std::wstring const & svrCertThumbprint,
            std::wstring const & clientClaims,
            bool expectFailure = false,
            bool clientIsAdmin = false,
            std::wstring const clientCertFindValue = L"");

        static void X509CertIssuerMatchTest(std::wstring const & issuers, bool shouldPass);

        void FramingProtectionEnabled_Negotiate_ByConfig(ProtectionLevel::Enum protectionLevel);

        SecurityTestSetup securityTestSetup_;
    };

    struct DummyServerResponse : public Serialization::FabricSerializable
    {
        DummyServerResponse()
        {
        }

        DummyServerResponse(wstring const &response) : response_(response)
        {
        }

        FABRIC_FIELDS_01(response_);

        wstring response_;
    };

    struct ClientIsAdmin: public Serialization::FabricSerializable
    {
        ClientIsAdmin() : IsAdmin(FALSE)
        {
        }

        ClientIsAdmin(bool isAdmin) : IsAdmin(isAdmin)
        {
        }

        FABRIC_FIELDS_01(IsAdmin);

        bool IsAdmin;
    };

    BOOST_FIXTURE_TEST_SUITE2(SecSuite, SecureTransportTests)

    BOOST_AUTO_TEST_CASE(X509Test_SelfSigned_Ipv4)
    {
        ENTER;
        X509Test_SelfSigned(L"127.0.0.1:0", L"127.0.0.1:0");
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509Test_SelfSigned_Ipv6)
    {
        ENTER;
        X509Test_SelfSigned(L"[::1]:0", L"[0:0:0:0:0:0:0:1]:0");
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509Test_SelfSigned_ChainValidation1)
    {
        ENTER;

        cout << "test with self-signed certs, with chain validation enabled, failure expected" << endl;

        wstring server = L"server"+Guid::NewGuid().ToString();
        InstallTestCertInScope serverCert(L"CN="+server); 
        auto serverCertThumbprint = serverCert.Thumbprint()->PrimaryToString(); 
        wstring client = L"client"+Guid::NewGuid().ToString();
        InstallTestCertInScope clientCert(L"CN="+client); 
        auto clientCertThumbprint = clientCert.Thumbprint()->PrimaryToString(); 

        SecuritySettings senderSecSettings = TTestUtil::CreateX509SettingsTp(
            clientCertThumbprint,
            L"", 
            serverCertThumbprint+L"?true",
            L"");

        SecuritySettings receiverSecSettings = TTestUtil::CreateX509SettingsTp(
            serverCertThumbprint,
            L"",
            clientCertThumbprint+L"?true",
            L"");

        SecurityTest(L"127.0.0.1:0", senderSecSettings, L"127.0.0.1:0", receiverSecSettings, true);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509Test_SelfSigned_ChainValidation2)
    {
        ENTER;

        cout << "test with self-signed certs, chain validation required, failure expected" << endl;

        wstring server = L"server"+Guid::NewGuid().ToString();
        InstallTestCertInScope serverCert(L"CN="+server); 
        wstring client = L"client"+Guid::NewGuid().ToString();
        InstallTestCertInScope clientCert(L"CN="+client); 

        SecuritySettings senderSecSettings = TTestUtil::CreateX509Settings(
            client,
            L"", 
            server,
            L"");

        SecuritySettings receiverSecSettings = TTestUtil::CreateX509Settings(
            server,
            L"",
            client,
            L"");

        SecurityTest(L"127.0.0.1:0", senderSecSettings, L"127.0.0.1:0", receiverSecSettings, true);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509Test_Expired)
    {
        ENTER;

        cout << "test with expired certs, failure expected" << endl;

        auto saved = SecurityConfig::GetConfig().SkipX509CredentialExpirationChecking;
        SecurityConfig::GetConfig().SkipX509CredentialExpirationChecking = true;
        KFinally([&] { SecurityConfig::GetConfig().SkipX509CredentialExpirationChecking = saved; });

        InstallTestCertInScope serverCert(L"CN=server.TestWithExpiredSelfSignedCert", nullptr, TimeSpan::Zero);
        auto serverCertThumbprint = serverCert.Thumbprint()->PrimaryToString(); 
        InstallTestCertInScope clientCert(L"CN=client@TestWithExpiredSelfSignedCert", nullptr, TimeSpan::Zero);
        auto clientCertThumbprint = clientCert.Thumbprint()->PrimaryToString(); 

        SecuritySettings senderSecSettings = TTestUtil::CreateX509SettingsTp(
            clientCertThumbprint,
            L"", 
            serverCertThumbprint,
            L"");

        SecuritySettings receiverSecSettings = TTestUtil::CreateX509SettingsTp(
            serverCertThumbprint,
            L"",
            clientCertThumbprint,
            L"");

        SecurityTest(L"127.0.0.1:0", senderSecSettings, L"127.0.0.1:0", receiverSecSettings, true);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509Test_CertRefresh_LoadByName)
    {
        ENTER;

        auto savedRefreshInterval = SecurityConfig::GetConfig().CertificateMonitorInterval;
        SecurityConfig::GetConfig().CertificateMonitorInterval = TimeSpan::FromSeconds(3);
        KFinally([&] { SecurityConfig::GetConfig().CertificateMonitorInterval = savedRefreshInterval; });

        auto cn = wformatString("CertRefresh.{0}", Guid::NewGuid());
        auto sn = wformatString("CN={0}", cn);
        InstallTestCertInScope c1(sn);

        // Create SecuritySettings with:
        // 1. loading by name
        // 2. auth by certificate thumbprint
        // In practice, if loading is done by name, auth will most probably be done by name too. In this test, auth is done
        // by certificate thumbprint matching to avoid the complexity of also installing the certificate to trusted root.
        auto secSettings = TTestUtil::CreateX509Settings_LoadByName_AuthByThumbprint(cn, c1.Thumbprint()->PrimaryToString());

        auto sender = DatagramTransportFactory::CreateTcp(L"127.0.0.1:0");
        auto receiver = DatagramTransportFactory::CreateTcp(L"127.0.0.1:0");

        VERIFY_IS_TRUE(sender->SetSecurity(secSettings).IsSuccess());
        VERIFY_IS_TRUE(receiver->SetSecurity(secSettings).IsSuccess());

        VERIFY_IS_TRUE(sender->Start().IsSuccess());

        auto testAction = TTestUtil::GetGuidAction();


        AutoResetEvent messageReceived;
        TTestUtil::SetMessageHandler(
            receiver,
            testAction,
            [&messageReceived, testAction](MessageUPtr & message, ISendTarget::SPtr const & st) -> void
            {
                Trace.WriteInfo(TraceType, "receiver got a message {0} from {1}", message->TraceId(), st->Address());
                messageReceived.Set();
            });

        VERIFY_IS_TRUE(receiver->Start().IsSuccess());

        wstring localListenerIdentity = TransportSecurity().LocalWindowsIdentity();
        ISendTarget::SPtr target = sender->ResolveTarget(receiver->ListenAddress(), L"", localListenerIdentity);
        VERIFY_IS_TRUE(target);

        auto oneway = make_unique<Message>();
        oneway->Headers.Add(ActionHeader(testAction));
        oneway->Headers.Add(MessageIdHeader());

        Trace.WriteInfo(TraceType, "sending message ...");
        sender->SendOneWay(target, std::move(oneway));

        auto waitTime = TimeSpan::FromSeconds(30);
        Trace.WriteInfo(TraceType, "wait for receiver to get the first message");
        VERIFY_ARE_EQUAL2(messageReceived.WaitOne(waitTime), true);

        InstallTestCertInScope c2(sn, nullptr, InstallTestCertInScope::DefaultCertExpiry() + TimeSpan::FromHours(48));

        Trace.WriteInfo(TraceType, "wait for new c2 to be loaded");
        ManualResetEvent().WaitOne(SecurityConfig::GetConfig().CertificateMonitorInterval + TimeSpan::FromSeconds(3));

        Trace.WriteInfo(TraceType, "close existing connection");
        target->Reset();

        oneway = make_unique<Message>();
        oneway->Headers.Add(ActionHeader(testAction));
        oneway->Headers.Add(MessageIdHeader());

        Trace.WriteInfo(TraceType, "sending a second message ...");
        sender->SendOneWay(target, std::move(oneway));

        waitTime = TimeSpan::FromSeconds(0.3);
        Trace.WriteInfo(TraceType, "the second message should not arrive as c2 has a different thumbprint");
        VERIFY_ARE_EQUAL2(messageReceived.WaitOne(waitTime), false);

        Trace.WriteInfo(TraceType, "add new certificate thumbprint to white list");
        auto secSettings2 = TTestUtil::CreateX509Settings_LoadByName_AuthByThumbprint(cn, c2.Thumbprint()->PrimaryToString());
        VERIFY_IS_TRUE(sender->SetSecurity(secSettings2).IsSuccess());
        VERIFY_IS_TRUE(receiver->SetSecurity(secSettings2).IsSuccess());

        oneway = make_unique<Message>();
        oneway->Headers.Add(ActionHeader(testAction));
        oneway->Headers.Add(MessageIdHeader());

        Trace.WriteInfo(TraceType, "sending a third message ...");
        sender->SendOneWay(target, std::move(oneway));

        waitTime = TimeSpan::FromSeconds(30);
        Trace.WriteInfo(TraceType, "the third message should arrive as new SecuritySettings has c2 in allow list");
        VERIFY_ARE_EQUAL2(messageReceived.WaitOne(waitTime), true);

        sender->Stop();
        receiver->Stop();

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(ClaimsAuthWithServerThumbprint)
    {
        ENTER;

        InstallTestCertInScope serverCert;
        auto serverCertThumbprint = *(serverCert.Thumbprint());

        auto serverSecurity = TTestUtil::CreateSvrX509Settings_ClaimsTest(serverCertThumbprint.PrimaryToString());
        auto error = serverSecurity.EnableClaimBasedAuthOnClients(L"t1=v1 && t2=v2 || t3=v3", L"t4=v4 && t5=v5");
        VERIFY_IS_TRUE(error.IsSuccess());
        Trace.WriteInfo(TraceType, "server security settings = {0}", serverSecurity);

        auto receiver = DatagramTransportFactory::CreateTcp(TTestUtil::GetListenAddress());
        auto testAction = TTestUtil::GetGuidAction();
        AutoResetEvent receiverGotMessage(false);
        TTestUtil::SetMessageHandler(
            receiver,
            testAction,
            [&receiverGotMessage](MessageUPtr & message, ISendTarget::SPtr const & st) -> void
            {
                Trace.WriteInfo(TraceType, "[receiver] got a message {0} {1}", message->TraceId(), st->Address());
                receiverGotMessage.Set();
            });

        VERIFY_IS_TRUE(receiver->SetSecurity(serverSecurity).IsSuccess());
        receiver->SetClaimsHandler(
            [](wstring const & claims, SecurityContextSPtr const & context)
        {
            SecuritySettings::RoleClaims clientClaims;
            auto error = SecuritySettings::StringToRoleClaims(claims, clientClaims);
            VERIFY_IS_TRUE(error.IsSuccess());

            Trace.WriteInfo(TraceType, "completing client auth with '{0}'", clientClaims);
            context->CompleteClientAuth(error, clientClaims, TimeSpan::MaxValue);
        });

        VERIFY_IS_TRUE(receiver->Start().IsSuccess());

        SecuritySettings clientSecurity;
        error = SecuritySettings::CreateClaimTokenClient(L"t3 = v3", serverCertThumbprint.PrimaryToString(), L"", L"", wformatString(ProtectionLevel::EncryptAndSign), clientSecurity);
        VERIFY_IS_TRUE(error.IsSuccess());

        auto sender = DatagramTransportFactory::CreateTcpClient();
        VERIFY_IS_TRUE(sender->SetSecurity(clientSecurity).IsSuccess());
        VERIFY_IS_TRUE(sender->Start().IsSuccess());

        ISendTarget::SPtr target = sender->ResolveTarget(receiver->ListenAddress());
        VERIFY_IS_TRUE(target);

        auto request(make_unique<Message>());
        request->Headers.Add(MessageIdHeader());
        request->Headers.Add(ActionHeader(testAction));
        VERIFY_IS_TRUE(sender->SendOneWay(target, move(request)).IsSuccess());
        VERIFY_IS_TRUE(receiverGotMessage.WaitOne(TimeSpan::FromSeconds(30)));

        // Set an incorrect server certificate thumbprint
        BYTE* hashBuffer = (BYTE*)(serverCertThumbprint.Hash());
        hashBuffer[0] = !hashBuffer[0];
        error = clientSecurity.SetRemoteCertThumbprints(serverCertThumbprint.PrimaryToString());
        VERIFY_IS_TRUE(error.IsSuccess());
        target->Reset();
        VERIFY_IS_TRUE(sender->SetSecurity(clientSecurity).IsSuccess());

        request = make_unique<Message>();
        request->Headers.Add(MessageIdHeader());
        request->Headers.Add(ActionHeader(testAction));
        sender->SendOneWay(target, move(request));
        VERIFY_IS_FALSE(receiverGotMessage.WaitOne(TimeSpan::FromSeconds(1)));

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(ClaimsAuthWithServerThumbprint2) // Test with expired server certificate
    {
        ENTER;

        InstallTestCertInScope serverCert(L"CN=ClaimAuthWithServerThumbprint2.TestWithExpiredSelfSignedCert", nullptr, TimeSpan::Zero);
        auto serverCertThumbprint = *(serverCert.Thumbprint());

        Sleep(10); //sleep to make sure certificate expired

        auto saved = SecurityConfig::GetConfig().SkipX509CredentialExpirationChecking;
        KFinally([=] { SecurityConfig::GetConfig().SkipX509CredentialExpirationChecking = saved; });
        SecurityConfig::GetConfig().SkipX509CredentialExpirationChecking = true;

        auto serverSecurity = TTestUtil::CreateSvrX509Settings_ClaimsTest(serverCertThumbprint.PrimaryToString());
        auto error = serverSecurity.EnableClaimBasedAuthOnClients(L"t1=v1 && t2=v2 || t3=v3", L"t4=v4 && t5=v5");
        VERIFY_IS_TRUE(error.IsSuccess());
        Trace.WriteInfo(TraceType, "server security settings = {0}", serverSecurity);

        auto receiver = DatagramTransportFactory::CreateTcp(TTestUtil::GetListenAddress());
        auto testAction = TTestUtil::GetGuidAction();
        AutoResetEvent receiverGotMessage(false);
        TTestUtil::SetMessageHandler(
            receiver,
            testAction,
            [&receiverGotMessage](MessageUPtr & message, ISendTarget::SPtr const & st) -> void
        {
            Trace.WriteInfo(TraceType, "[receiver] got a message {0} {1}", message->TraceId(), st->Address());
            receiverGotMessage.Set();
        });

        VERIFY_IS_TRUE(receiver->SetSecurity(serverSecurity).IsSuccess());
        receiver->SetClaimsHandler(
            [](wstring const & claims, SecurityContextSPtr const & context)
            {
                SecuritySettings::RoleClaims clientClaims;
                auto error = SecuritySettings::StringToRoleClaims(claims, clientClaims);
                VERIFY_IS_TRUE(error.IsSuccess());

                Trace.WriteInfo(TraceType, "completing client auth with '{0}'", clientClaims);
                context->CompleteClientAuth(error, clientClaims, TimeSpan::MaxValue);
            });

        VERIFY_IS_TRUE(receiver->Start().IsSuccess());

        SecuritySettings clientSecurity;
        error = SecuritySettings::CreateClaimTokenClient(L"t3 = v3", serverCertThumbprint.PrimaryToString()+L"?true", L"", L"", wformatString(ProtectionLevel::EncryptAndSign), clientSecurity);
        VERIFY_IS_TRUE(error.IsSuccess());

        auto sender = DatagramTransportFactory::CreateTcpClient();
        VERIFY_IS_TRUE(sender->SetSecurity(clientSecurity).IsSuccess());
        VERIFY_IS_TRUE(sender->Start().IsSuccess());

        ISendTarget::SPtr target = sender->ResolveTarget(receiver->ListenAddress());
        VERIFY_IS_TRUE(target);

        auto request(make_unique<Message>());
        request->Headers.Add(MessageIdHeader());
        request->Headers.Add(ActionHeader(testAction));
        sender->SendOneWay(target, move(request));
        VERIFY_IS_FALSE(receiverGotMessage.WaitOne(TimeSpan::FromSeconds(1)));

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(ClaimsTokenErrorTest)
    {
        ENTER;

        InstallTestCertInScope serverCert;
        auto serverCertThumbprint = *(serverCert.Thumbprint());
        auto serverSecurity = TTestUtil::CreateSvrX509Settings_ClaimsTest(serverCertThumbprint.PrimaryToString());

        auto error = serverSecurity.EnableClaimBasedAuthOnClients(L"ClaimType1=ClaimValue1", L"ClaimType2=ClaimValue2");
        VERIFY_IS_TRUE(error.IsSuccess());
        Trace.WriteInfo(TraceType, "server security settings = {0}", serverSecurity);

        auto receiver = DatagramTransportFactory::CreateTcp(TTestUtil::GetListenAddress());
        VERIFY_IS_TRUE(receiver->SetSecurity(serverSecurity).IsSuccess());
        receiver->SetClaimsHandler(
            [](wstring const & claims, SecurityContextSPtr const & context)
        {
            Trace.WriteInfo(TraceType, "fail client auth '{0}'", claims);
            context->CompleteClientAuth(ErrorCodeValue::InvalidCredentials, SecuritySettings::RoleClaims(), TimeSpan::MaxValue);
        });

        VERIFY_IS_TRUE(receiver->Start().IsSuccess());

        SecuritySettings clientSecurity;
        error = SecuritySettings::CreateClaimTokenClient(L"ClaimType1=ClaimValue1", serverCertThumbprint.PrimaryToString(), L"", L"", wformatString(ProtectionLevel::EncryptAndSign), clientSecurity);
        VERIFY_IS_TRUE(error.IsSuccess());
        Trace.WriteInfo(TraceType, "client security settings = {0}", clientSecurity);

        auto sender = DatagramTransportFactory::CreateTcpClient();
        VERIFY_IS_TRUE(sender->SetSecurity(clientSecurity).IsSuccess());
        VERIFY_IS_TRUE(sender->Start().IsSuccess());

        ISendTarget::SPtr target = sender->ResolveTarget(receiver->ListenAddress());
        VERIFY_IS_TRUE(target);

        AutoResetEvent connectionFaultReported(false);
        sender->SetConnectionFaultHandler([&target, &connectionFaultReported](ISendTarget const & st, ErrorCode status)
        {
            Trace.WriteInfo(TraceType, "ConnectionFaultHandler: target {0} at {1}, error ={2}", TextTracePtr(&st), st.Address(), status);
            VERIFY_IS_TRUE(target.get() == &st);
            VERIFY_IS_TRUE(status.IsError(ErrorCodeValue::InvalidCredentials));
            connectionFaultReported.Set();
        });

        auto request(make_unique<Message>());
        request->Headers.Add(MessageIdHeader());

        sender->SendOneWay(target, move(request));

        VERIFY_IS_TRUE(connectionFaultReported.WaitOne(TimeSpan::FromSeconds(30)));

        sender->RemoveConnectionFaultHandler();
        sender->Stop();
        receiver->RemoveClaimsHandler();
        receiver->Stop();

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(ClaimsTokenExpirationTest)
    {
        ENTER;

        SecurityConfig & securityConfig = SecurityConfig::GetConfig();
        auto savedSessionExpiration = securityConfig.SessionExpiration;
        securityConfig.SessionExpiration = TimeSpan::FromSeconds(5);
        auto saved2 = securityConfig.SessionRefreshRetryDelay;
        securityConfig.SessionRefreshRetryDelay = TimeSpan::Zero;
        auto saved3 = securityConfig.SessionExpirationCloseDelay;
        securityConfig.SessionExpirationCloseDelay = TimeSpan::Zero;
        KFinally([&]
        {
            securityConfig.SessionExpiration = savedSessionExpiration;
            securityConfig.SessionRefreshRetryDelay = saved2;
            securityConfig.SessionExpirationCloseDelay = saved3;
        });

#ifdef PLATFORM_UNIX
        InstallTestCertInScope serverCert;
        auto serverCertThumbprint = *(serverCert.Thumbprint());
        auto serverSecurity = TTestUtil::CreateSvrX509Settings_ClaimsTest(serverCertThumbprint.PrimaryToString());
#else
        auto serverSecurity = TTestUtil::CreateX509Settings(
            L"CN=WinFabric-Test-SAN1-Alice",
            L"WinFabric-Test-SAN1-Bob");
#endif

        auto error = serverSecurity.EnableClaimBasedAuthOnClients(L"ClaimType1=ClaimValue1", L"ClaimType2=ClaimValue2");
        VERIFY_IS_TRUE(error.IsSuccess());
        Trace.WriteInfo(TraceType, "server security settings = {0}", serverSecurity);

        auto receiver = DatagramTransportFactory::CreateTcp(TTestUtil::GetListenAddress());
        VERIFY_IS_TRUE(receiver->SetSecurity(serverSecurity).IsSuccess());
        receiver->SetClaimsHandler(
        [](wstring const & claims, SecurityContextSPtr const & context)
        {
            static bool firstCall = true;
            if (firstCall)
            {
                firstCall = false;
                Trace.WriteInfo(TraceType, "complete initial client auth '{0}' with success", claims);
                SecuritySettings::RoleClaims clientClaims;
                VERIFY_IS_TRUE(clientClaims.AddClaim(claims));
                context->CompleteClientAuth(ErrorCodeValue::Success, clientClaims, TimeSpan::MaxValue);
                return;
            }

            Trace.WriteInfo(TraceType, "fail later client auth '{0}'", claims);
            context->CompleteClientAuth(ErrorCodeValue::InvalidCredentials, SecuritySettings::RoleClaims(), TimeSpan::MaxValue);
        });

        wstring testAction = TTestUtil::GetGuidAction();
        AutoResetEvent receiverReceivedMessage(false);
        TTestUtil::SetMessageHandler(
            receiver,
            testAction,
            [&receiverReceivedMessage](MessageUPtr & message, ISendTarget::SPtr const & target) -> void
            {
                Trace.WriteInfo(TraceType, "recevier got message {0} from {1}", message->TraceId(), target->Address());
                receiverReceivedMessage.Set();
            });

        VERIFY_IS_TRUE(receiver->Start().IsSuccess());

        SecuritySettings clientSecurity;
#ifdef PLATFORM_UNIX
        error = SecuritySettings::CreateClaimTokenClient(L"ClaimType1=ClaimValue1", serverCertThumbprint.PrimaryToString(), L"", L"", wformatString(ProtectionLevel::EncryptAndSign), clientSecurity);
#else
        error = SecuritySettings::CreateClaimTokenClient(L"ClaimType1=ClaimValue1", L"", L"WinFabric-Test-SAN1-Alice", L"", wformatString(ProtectionLevel::EncryptAndSign), clientSecurity);
#endif

        VERIFY_IS_TRUE(error.IsSuccess());
        Trace.WriteInfo(TraceType, "client security settings = {0}", clientSecurity);

        auto sender = DatagramTransportFactory::CreateTcpClient();
        VERIFY_IS_TRUE(sender->SetSecurity(clientSecurity).IsSuccess());
        VERIFY_IS_TRUE(sender->Start().IsSuccess());

        ISendTarget::SPtr target = sender->ResolveTarget(receiver->ListenAddress());
        VERIFY_IS_TRUE(target);

        AutoResetEvent sessionExpired(false);
        AutoResetEvent faultedWithInvalidCredential(false);
        sender->SetConnectionFaultHandler([&](ISendTarget const & st, ErrorCode fault)
        {
            Trace.WriteInfo(TraceType, "ConnectionFaultHandler: target {0} at {1}, error ={2}", TextTracePtr(&st), st.Address(), fault);
            VERIFY_IS_TRUE(target.get() == &st);
            if (fault.IsError(SESSION_EXPIRATION_FAULT))
            {
                sessionExpired.Set();
                return;
            }

            if (fault.IsError(ErrorCodeValue::InvalidCredentials))
            {
                faultedWithInvalidCredential.Set();
                return;
            }

            VERIFY_IS_TRUE(false);
        });

        auto request(make_unique<Message>());
        request->Headers.Add(ActionHeader(testAction));
        request->Headers.Add(MessageIdHeader());

        Trace.WriteInfo(TraceType, "sending message to trigger connection fault");
        VERIFY_IS_TRUE(sender->SendOneWay(target, move(request)).IsSuccess());
        VERIFY_IS_TRUE(receiverReceivedMessage.WaitOne(TimeSpan::FromSeconds(20)));

        // Wait for session expiration
        VERIFY_IS_TRUE(sessionExpired.WaitOne(TimeSpan::FromSeconds(30)));

        Trace.WriteInfo(TraceType, "waiting for connection cleanup to complete");
        VERIFY_IS_TRUE(TTestUtil::WaitUntil([&] { return target->ConnectionCount() == 0; }, TimeSpan::FromSeconds(30)));
        Trace.WriteInfo(TraceType, "connection cleanup completed");

        auto request2(make_unique<Message>());
        request2->Headers.Add(ActionHeader(testAction));
        request2->Headers.Add(MessageIdHeader());

        Trace.WriteInfo(TraceType, "sending another message to trigger reconnect");
        sender->SendOneWay(target, move(request2));

        // Wait for connection fault caused by invalid token
        VERIFY_IS_TRUE(faultedWithInvalidCredential.WaitOne(TimeSpan::FromSeconds(30)));

        sender->RemoveConnectionFaultHandler();
        sender->Stop();
        receiver->RemoveClaimsHandler();
        receiver->Stop();

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509_ImplicitPeerTrust_Enabled)
    {
        ENTER;

        auto saved = SecurityConfig::GetConfig().ImplicitPeerTrustEnabled;
        SecurityConfig::GetConfig().ImplicitPeerTrustEnabled = true;
        KFinally([=] {SecurityConfig::GetConfig().ImplicitPeerTrustEnabled = saved; });

        InstallTestCertInScope testCert;
        SecuritySettings securitySettings = TTestUtil::CreateX509Settings(
            testCert.SubjectName(),
            SecurityConfig::X509NameMap() /* empty whitelist to test implicit peer trust: remote is trusted if its X509 cert is the same as the local one*/);

        securitySettings.EnablePeerToPeerMode();

        auto node1 = DatagramTransportFactory::CreateTcp(TTestUtil::GetListenAddress(), L"", L"node1");
        VERIFY_IS_TRUE(node1->SetSecurity(securitySettings).IsSuccess());
        VERIFY_IS_TRUE(node1->Start().IsSuccess());

        auto node2 = DatagramTransportFactory::CreateTcp(TTestUtil::GetListenAddress(), L"", L"node2");
        VERIFY_IS_TRUE(node2->SetSecurity(securitySettings).IsSuccess());

        AutoResetEvent messageReceived;
        wstring testAction = TTestUtil::GetGuidAction();
        TTestUtil::SetMessageHandler(
            node2,
            testAction,
            [&node2, &node1, &messageReceived](MessageUPtr &, ISendTarget::SPtr const & st) -> void
            {
                Trace.WriteInfo(TraceType, "node2 got a message from {0}, node1 listen address = {1}", st->Address(), node1->ListenAddress());
                messageReceived.Set();
            });

        VERIFY_IS_TRUE(node2->Start().IsSuccess());

        ISendTarget::SPtr target = node1->ResolveTarget(node2->ListenAddress());
        VERIFY_IS_TRUE(target);

        auto message1 = make_unique<Message>();
        message1->Headers.Add(ActionHeader(testAction));
        message1->Headers.Add(MessageIdHeader());

        Trace.WriteInfo(TraceType, "sending first message");
        node1->SendOneWay(target, std::move(message1));
        VERIFY_IS_TRUE(messageReceived.WaitOne(TimeSpan::FromSeconds(30)));

        node1->Stop();
        node2->Stop();

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509_ImplicitPeerTrust_Disabled)
    {
        ENTER;

        auto saved = SecurityConfig::GetConfig().ImplicitPeerTrustEnabled;
        SecurityConfig::GetConfig().ImplicitPeerTrustEnabled = false;
        KFinally([=] {SecurityConfig::GetConfig().ImplicitPeerTrustEnabled = saved; });

        InstallTestCertInScope testCert;
        SecuritySettings securitySettings = TTestUtil::CreateX509Settings(
            testCert.SubjectName(),
            SecurityConfig::X509NameMap() /* empty whitelist to test implicit peer trust: remote is trusted if its X509 cert is the same as the local one*/);

        securitySettings.EnablePeerToPeerMode();

        auto node1 = DatagramTransportFactory::CreateTcp(TTestUtil::GetListenAddress(), L"", L"node1");
        VERIFY_IS_TRUE(node1->SetSecurity(securitySettings).IsSuccess());
        VERIFY_IS_TRUE(node1->Start().IsSuccess());

        auto node2 = DatagramTransportFactory::CreateTcp(TTestUtil::GetListenAddress(), L"", L"node2");
        VERIFY_IS_TRUE(node2->SetSecurity(securitySettings).IsSuccess());

        AutoResetEvent messageReceived;
        wstring testAction = TTestUtil::GetGuidAction();
        TTestUtil::SetMessageHandler(
            node2,
            testAction,
            [&node2, &node1, &messageReceived](MessageUPtr &, ISendTarget::SPtr const & st) -> void
        {
            Trace.WriteInfo(TraceType, "node2 got a message from {0}, node1 listen address = {1}", st->Address(), node1->ListenAddress());
            messageReceived.Set();
        });

        VERIFY_IS_TRUE(node2->Start().IsSuccess());

        ISendTarget::SPtr target = node1->ResolveTarget(node2->ListenAddress());
        VERIFY_IS_TRUE(target);

        auto message1 = make_unique<Message>();
        message1->Headers.Add(ActionHeader(testAction));
        message1->Headers.Add(MessageIdHeader());

        Trace.WriteInfo(TraceType, "sending first message");
        node1->SendOneWay(target, std::move(message1));
        VERIFY_IS_FALSE(messageReceived.WaitOne(TimeSpan::FromMilliseconds(300)));

        node1->Stop();
        node2->Stop();

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509_ImplicitPeerTrust_Disabled2)
    {
        ENTER;

        InstallTestCertInScope testCert;
        SecuritySettings securitySettings = TTestUtil::CreateX509Settings(
            testCert.SubjectName(),
            SecurityConfig::X509NameMap());

        auto node1 = DatagramTransportFactory::CreateTcp(TTestUtil::GetListenAddress(), L"", L"node1");
        VERIFY_IS_TRUE(node1->SetSecurity(securitySettings).IsSuccess());
        VERIFY_IS_TRUE(node1->Start().IsSuccess());

        auto node2 = DatagramTransportFactory::CreateTcp(TTestUtil::GetListenAddress(), L"", L"node2");
        VERIFY_IS_TRUE(node2->SetSecurity(securitySettings).IsSuccess());

        AutoResetEvent messageReceived;
        wstring testAction = TTestUtil::GetGuidAction();
        TTestUtil::SetMessageHandler(
            node2,
            testAction,
            [&node2, &node1, &messageReceived](MessageUPtr &, ISendTarget::SPtr const & st) -> void
        {
            Trace.WriteInfo(TraceType, "node2 got a message from {0}, node1 listen address = {1}", st->Address(), node1->ListenAddress());
            messageReceived.Set();
        });

        VERIFY_IS_TRUE(node2->Start().IsSuccess());

        ISendTarget::SPtr target = node1->ResolveTarget(node2->ListenAddress());
        VERIFY_IS_TRUE(target);

        auto message1 = make_unique<Message>();
        message1->Headers.Add(ActionHeader(testAction));
        message1->Headers.Add(MessageIdHeader());

        Trace.WriteInfo(TraceType, "sending first message");
        node1->SendOneWay(target, std::move(message1));

        Trace.WriteInfo(TraceType, "SecuritySettings::EnablePeerToPeerMode was not called, communication is supposed to fail");
        VERIFY_IS_FALSE(messageReceived.WaitOne(TimeSpan::FromMilliseconds(300)));

        node1->Stop();
        node2->Stop();

        LEAVE;
    }



#ifdef PLATFORM_UNIX

    BOOST_AUTO_TEST_CASE(LeaseTransportMode)
    {
        ENTER;

        wstring server = L"server"+Guid::NewGuid().ToString();
        InstallTestCertInScope serverCert(L"CN="+server); 
        auto serverCertThumbprint = serverCert.Thumbprint()->PrimaryToString(); 
        wstring client = L"client"+Guid::NewGuid().ToString();
        InstallTestCertInScope clientCert(L"CN="+client); 
        auto clientCertThumbprint = clientCert.Thumbprint()->PrimaryToString(); 

        SecuritySettings senderSecSettings = TTestUtil::CreateX509SettingsTp(
            clientCertThumbprint,
            L"", 
            serverCertThumbprint,
            L"");

        SecuritySettings receiverSecSettings = TTestUtil::CreateX509SettingsTp(
            serverCertThumbprint,
            L"",
            clientCertThumbprint,
            L"");

        auto sender = TcpDatagramTransport::Create(L"127.0.0.1:0");
        sender->DisableListenInstanceMessage();
        sender->SetBufferFactory(make_unique<LTBufferFactory>());
        auto receiver = TcpDatagramTransport::Create(L"127.0.0.1:0");
        receiver->DisableListenInstanceMessage();
        receiver->SetBufferFactory(make_unique<LTBufferFactory>());

        VERIFY_ARE_EQUAL2(sender->SetSecurity(senderSecSettings).ReadValue(), ErrorCodeValue::Success);
        VERIFY_ARE_EQUAL2(receiver->SetSecurity(receiverSecSettings).ReadValue(), ErrorCodeValue::Success);

        const uint64 replyId = numeric_limits<uint64>::max() / 11; 
        AutoResetEvent replyReceived;
        sender->SetMessageHandler(
            [&replyReceived](MessageUPtr & msg, ISendTarget::SPtr const & st) -> void
            {
                Trace.WriteInfo(TraceType, "[sender] got reply from {0}", st->Address());

                vector<const_buffer> buffers;
                Invariant(msg->GetBody(buffers));

                size_t totalBytes = 0;
                for(auto const & buffer : buffers)
                {
                    totalBytes += buffer.size();
                }

                auto bufUPtr = std::make_unique<BYTE[]>(totalBytes);
                auto buf = bufUPtr.get(); 

                auto ptr = buf;
                for(auto const & buffer : buffers)
                {
                    memcpy(ptr, buffer.buf, buffer.len);
                    ptr += buffer.len;
                }

                auto testLeaseMessage = (TestLeaseMessage*)buf;
                Trace.WriteInfo(TraceType, "[sender] reply id = {0}", testLeaseMessage->MessageIdentifier.QuadPart);
                Invariant(testLeaseMessage->MessageIdentifier.QuadPart == replyId);
                Invariant(testLeaseMessage->Verify());
                replyReceived.Set();
            });

        const uint64 requestId = numeric_limits<uint64>::max() / 7; 
        AutoResetEvent messageReceived;
        receiver->SetMessageHandler(
            [&receiver, &messageReceived, &sender](MessageUPtr & msg, ISendTarget::SPtr const & st) -> void
            {
                Trace.WriteInfo(TraceType, "[receiver] got request from {0}", st->Address());

                vector<const_buffer> buffers;
                Invariant(msg->GetBody(buffers));

                size_t totalBytes = 0;
                for(auto const & buffer : buffers)
                {
                    totalBytes += buffer.size();
                }

                auto bufUPtr = std::make_unique<BYTE[]>(totalBytes);
                auto buf = bufUPtr.get(); 

                auto ptr = buf;
                for(auto const & buffer : buffers)
                {
                    memcpy(ptr, buffer.buf, buffer.len);
                    ptr += buffer.len;
                }

                auto testLeaseMessage = (TestLeaseMessage*)buf;
                Trace.WriteInfo(TraceType, "[receiver] request id = {0}", testLeaseMessage->MessageIdentifier.QuadPart);
                Invariant(testLeaseMessage->MessageIdentifier.QuadPart == requestId);
                Invariant(testLeaseMessage->Verify());
                messageReceived.Set();

                auto replyBody = new TestLeaseMessage(replyId);
                TestLeaseMessage testReply(replyId);
                vector<const_buffer> replyBufferList(1, const_buffer(replyBody, sizeof(TestLeaseMessage)));
                auto reply = make_unique<Message>(
                    replyBufferList,
                    [] (std::vector<Common::const_buffer> const &, void* bodyBuffer) { delete bodyBuffer; },
                    replyBody);

                Trace.WriteInfo(TraceType, "[receiver] sending reply {0}", replyBody->MessageIdentifier.QuadPart); 
                ISendTarget::SPtr target = receiver->ResolveTarget(sender->ListenAddress());
                VERIFY_IS_TRUE(target);
                receiver->SendOneWay(target, std::move(reply));
            });

        VERIFY_IS_TRUE(receiver->Start().IsSuccess());
        VERIFY_IS_TRUE(sender->Start().IsSuccess());

        ISendTarget::SPtr target = sender->ResolveTarget(receiver->ListenAddress());
        VERIFY_IS_TRUE(target);

        TestLeaseMessage testLeaseMessage(requestId);
        vector<const_buffer> bufferList(1, const_buffer(&testLeaseMessage, sizeof(testLeaseMessage)));
        auto oneway = make_unique<Message>(
            bufferList,
            [] (std::vector<Common::const_buffer> const &, void*) {},
            nullptr);

        Trace.WriteInfo(TraceType, "[sender] sending {0}", testLeaseMessage.MessageIdentifier.QuadPart); 
        sender->SendOneWay(target, std::move(oneway));

        bool receivedMessageOne = messageReceived.WaitOne(TimeSpan::FromSeconds(30));
        VERIFY_IS_TRUE(receivedMessageOne);

        bool receivedReplyOne = replyReceived.WaitOne(TimeSpan::FromSeconds(30));
        VERIFY_IS_TRUE(receivedReplyOne);

        VERIFY_IS_TRUE(sender->SendTargetCount() > 0);
        VERIFY_IS_TRUE(receiver->SendTargetCount() > 0);

        sender->Stop(TimeSpan::FromSeconds(10));
        receiver->Stop(TimeSpan::FromSeconds(10));

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(ClaimsAuthTestsWithClientRoles)
    {
        ENTER;

        InstallTestCertInScope serverCert;
        auto serverCertThumbprint = serverCert.Thumbprint()->PrimaryToString();

        InstallTestCertInScope clientCert;
        InstallTestCertInScope adminClientCert;
        InstallTestCertInScope nonClientCert;

        auto serverSecurity = TTestUtil::CreateSvrX509Settings_ClaimsTest(
            serverCertThumbprint,
            clientCert.Thumbprint()->PrimaryToString());

        serverSecurity.EnableAdminRole(adminClientCert.Thumbprint()->PrimaryToString(), SecurityConfig::X509NameMap(), L"");
        auto error = serverSecurity.EnableClaimBasedAuthOnClients(L"t1=v1 && t2=v2 || t3=v3", L"t4=v4 && t5=v5");
        VERIFY_IS_TRUE(error.IsSuccess());
        Trace.WriteInfo(TraceType, "server security settings = {0}", serverSecurity);

        auto receiver = DatagramTransportFactory::CreateTcp(TTestUtil::GetListenAddress());
        receiver->SetMessageHandler([receiver](MessageUPtr & message, ISendTarget::SPtr const & st) -> void
        {
            Trace.WriteInfo(TraceType, "[receiver] got a message {0} {1}", message->TraceId(), st->Address());

            // reply with client role information so that test client can verify
            auto reply(make_unique<Message>(ClientIsAdmin(message->IsInRole(RoleMask::Admin))));
            reply->Headers.Add(RelatesToHeader(message->MessageId));
            VERIFY_IS_TRUE(receiver->SendOneWay(st, move(reply)).IsSuccess());
        });

        VERIFY_IS_TRUE(receiver->SetSecurity(serverSecurity).IsSuccess());
        receiver->SetClaimsHandler(
            [] (wstring const & claims, SecurityContextSPtr const & context)
            {
                SecuritySettings::RoleClaims clientClaims;
                auto error = SecuritySettings::StringToRoleClaims(claims, clientClaims);
                VERIFY_IS_TRUE(error.IsSuccess());

                Trace.WriteInfo(TraceType, "completing client auth with '{0}'", clientClaims);
                context->CompleteClientAuth(error, clientClaims, TimeSpan::MaxValue);
            });

        VERIFY_IS_TRUE(receiver->Start().IsSuccess());

        // test with client claims
        ClaimAuthTestClientSend2(receiver->ListenAddress(), serverCertThumbprint, L"t1=v1 && t2=v2");
        ClaimAuthTestClientSend2(receiver->ListenAddress(), serverCertThumbprint, L"t3 = v3");
        ClaimAuthTestClientSend2(receiver->ListenAddress(), serverCertThumbprint, L"T4=v4 && t5 = V5", false, true);
        ClaimAuthTestClientSend2(receiver->ListenAddress(), serverCertThumbprint, L"t1=v1", true);
        ClaimAuthTestClientSend2(receiver->ListenAddress(), serverCertThumbprint, L"t5=v5", true);

        // test with client certificates
        ClaimAuthTestClientSend2(receiver->ListenAddress(), serverCertThumbprint, L"", false, false, clientCert.Thumbprint()->PrimaryToString()); 
        ClaimAuthTestClientSend2(receiver->ListenAddress(), serverCertThumbprint, L"", false, true, adminClientCert.Thumbprint()->PrimaryToString()); 
        ClaimAuthTestClientSend2(receiver->ListenAddress(), serverCertThumbprint, L"", true, false, nonClientCert.Thumbprint()->PrimaryToString()); 

        receiver->RemoveClaimsHandler();
        receiver->Stop();

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509ManyMessage_SelfSignedTest)
    {
        ENTER;
        X509ManyMessage_SelfSigned(L"127.0.0.1:0", L"127.0.0.1:0");
        LEAVE;
    }

#else

    BOOST_AUTO_TEST_CASE(ClaimsAuthTestsWithClientRoles)
    {
        ENTER;

        SecuritySettings serverSecurity = TTestUtil::CreateX509Settings(
            L"CN=WinFabric-Test-SAN1-Alice",
            L"WinFabric-Test-SAN1-Bob");

        serverSecurity.EnableAdminRole(L"", SecurityConfig::X509NameMap(), L"WinFabric-Test-SAN3-Oscar");
        auto error = serverSecurity.EnableClaimBasedAuthOnClients(L"t1=v1 && t2=v2 || t3=v3", L"t4=v4 && t5=v5");
        VERIFY_IS_TRUE(error.IsSuccess());
        Trace.WriteInfo(TraceType, "server security settings = {0}", serverSecurity);

        auto receiver = DatagramTransportFactory::CreateTcp(TTestUtil::GetListenAddress());
        receiver->SetMessageHandler([receiver](MessageUPtr & message, ISendTarget::SPtr const & st) -> void
        {
            Trace.WriteInfo(TraceType, "[receiver] got a message {0} {1}", message->TraceId(), st->Address());

            // reply with client role information so that test client can verify
            auto reply(make_unique<Message>(ClientIsAdmin(message->IsInRole(RoleMask::Admin))));
            reply->Headers.Add(RelatesToHeader(message->MessageId));
            VERIFY_IS_TRUE(receiver->SendOneWay(st, move(reply)).IsSuccess());
        });

        VERIFY_IS_TRUE(receiver->SetSecurity(serverSecurity).IsSuccess());
        receiver->SetClaimsHandler(
            [] (wstring const & claims, SecurityContextSPtr const & context)
            {
                SecuritySettings::RoleClaims clientClaims;
                auto error = SecuritySettings::StringToRoleClaims(claims, clientClaims);
                VERIFY_IS_TRUE(error.IsSuccess());

                Trace.WriteInfo(TraceType, "completing client auth with '{0}'", clientClaims);
                context->CompleteClientAuth(error, clientClaims, TimeSpan::MaxValue);
            });

        VERIFY_IS_TRUE(receiver->Start().IsSuccess());

        // test with client claims
        ClaimAuthTestClientSend(receiver->ListenAddress(), L"t1=v1 && t2=v2");
        ClaimAuthTestClientSend(receiver->ListenAddress(), L"t3 = v3");
        ClaimAuthTestClientSend(receiver->ListenAddress(), L"T4=v4 && t5 = V5", false, true);
        ClaimAuthTestClientSend(receiver->ListenAddress(), L"t1=v1", true);
        ClaimAuthTestClientSend(receiver->ListenAddress(), L"t5=v5", true);

        // test with client certificates
        ClaimAuthTestClientSend(receiver->ListenAddress(), L"", false, false, L"CN=WinFabric-Test-SAN1-Bob");
        ClaimAuthTestClientSend(receiver->ListenAddress(), L"", false, true, L"CN=WinFabric-Test-SAN3-Oscar");
        ClaimAuthTestClientSend(receiver->ListenAddress(), L"", true, false, L"CN=WinFabric-Test-SAN2-Charlie");

        receiver->RemoveClaimsHandler();
        receiver->Stop();

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(ClaimsAuthTestsWithClientRolesAndIssuerStores)
    {
        ENTER;

        wstring certIssuerName = L"WinFabric-Test-TA-CA";
        SecurityConfig::IssuerStoreKeyValueMap issuerMap;
        issuerMap.Add(certIssuerName, L"Root");

        SecuritySettings serverSecurity = TTestUtil::CreateX509Settings(
            L"CN=WinFabric-Test-SAN1-Alice",
            L"WinFabric-Test-SAN1-Bob",
            issuerMap);

        serverSecurity.EnableAdminRole(L"", SecurityConfig::X509NameMap(), L"WinFabric-Test-SAN3-Oscar");
        Trace.WriteInfo(TraceType, "server security settings = {0}", serverSecurity);

        auto receiver = DatagramTransportFactory::CreateTcp(TTestUtil::GetListenAddress());
        receiver->SetMessageHandler([receiver](MessageUPtr & message, ISendTarget::SPtr const & st) -> void
        {
            Trace.WriteInfo(TraceType, "[receiver] got a message {0} {1}", message->TraceId(), st->Address());

            // reply with client role information so that test client can verify
            auto reply(make_unique<Message>(ClientIsAdmin(message->IsInRole(RoleMask::Admin))));
            reply->Headers.Add(RelatesToHeader(message->MessageId));
            VERIFY_IS_TRUE(receiver->SendOneWay(st, move(reply)).IsSuccess());
        });

        VERIFY_IS_TRUE(receiver->SetSecurity(serverSecurity).IsSuccess());
        VERIFY_IS_TRUE(receiver->Start().IsSuccess());

        // test with client certificates
        ClaimAuthTestClientSend(receiver->ListenAddress(), L"", false, false, L"CN=WinFabric-Test-SAN1-Bob", issuerMap);
        ClaimAuthTestClientSend(receiver->ListenAddress(), L"", false, true, L"CN=WinFabric-Test-SAN3-Oscar", issuerMap);
        ClaimAuthTestClientSend(receiver->ListenAddress(), L"", true, false, L"CN=WinFabric-Test-SAN2-Charlie", issuerMap);

        receiver->Stop();
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509Test_CertRefresh_IssuerStore)
    {
        ENTER;

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

        SecuritySettings secSettings = TTestUtil::CreateX509Settings_CertRefresh_IssuerStore(cn, remoteNames, issuerMap);

        auto sender = DatagramTransportFactory::CreateTcp(L"127.0.0.1:0");
        auto receiver = DatagramTransportFactory::CreateTcp(L"127.0.0.1:0");

        VERIFY_IS_TRUE(sender->SetSecurity(secSettings).IsSuccess());
        VERIFY_IS_TRUE(receiver->SetSecurity(secSettings).IsSuccess());

        VERIFY_IS_TRUE(sender->Start().IsSuccess());

        auto testAction = TTestUtil::GetGuidAction();


        AutoResetEvent messageReceived;
        TTestUtil::SetMessageHandler(
            receiver,
            testAction,
            [&messageReceived, testAction](MessageUPtr & message, ISendTarget::SPtr const & st) -> void
        {
            Trace.WriteInfo(TraceType, "receiver got a message {0} from {1}", message->TraceId(), st->Address());
            messageReceived.Set();
        });

        VERIFY_IS_TRUE(receiver->Start().IsSuccess());

        wstring localListenerIdentity = TransportSecurity().LocalWindowsIdentity();
        ISendTarget::SPtr target = sender->ResolveTarget(receiver->ListenAddress(), L"", localListenerIdentity);
        VERIFY_IS_TRUE(target);

        auto oneway = make_unique<Message>();
        oneway->Headers.Add(ActionHeader(testAction));
        oneway->Headers.Add(MessageIdHeader());

        Trace.WriteInfo(TraceType, "sending message ...");
        sender->SendOneWay(target, std::move(oneway));

        auto waitTime = TimeSpan::FromSeconds(30);
        Trace.WriteInfo(TraceType, "wait for receiver to get the first message");
        VERIFY_ARE_EQUAL2(messageReceived.WaitOne(waitTime), true);

        InstallTestCertInScope c2(sn, nullptr, InstallTestCertInScope::DefaultCertExpiry() + TimeSpan::FromHours(48), L"Root", L"", X509Default::StoreLocation());

        Trace.WriteInfo(TraceType, "wait for new c2 to be loaded");
        ManualResetEvent().WaitOne(SecurityConfig::GetConfig().CertificateMonitorInterval + TimeSpan::FromSeconds(3));

        Trace.WriteInfo(TraceType, "close existing connection");
        target->Reset();

        oneway = make_unique<Message>();
        oneway->Headers.Add(ActionHeader(testAction));
        oneway->Headers.Add(MessageIdHeader());

        Trace.WriteInfo(TraceType, "sending a second message ...");
        sender->SendOneWay(target, std::move(oneway));

        waitTime = TimeSpan::FromSeconds(30);
        Trace.WriteInfo(TraceType, "the second message should arrive as new SecuritySettings has c2 in allow list");
        VERIFY_ARE_EQUAL2(messageReceived.WaitOne(waitTime), true);

        sender->Stop();
        receiver->Stop();

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509TestCASignedCerts_ClientCert)
    {
        ENTER;

        wstring server = L"WinFabric-Test-SAN1-Alice";
        wstring client= L"WinFabric-Test-User@microsoft.com";

        SecuritySettings senderSecSettings = TTestUtil::CreateX509Settings(
            client,
            L"", 
            server,
            L"");

        SecuritySettings receiverSecSettings = TTestUtil::CreateX509Settings(
            server,
            L"",
            client,
            L"");

        SecurityTest(L"127.0.0.1:0", senderSecSettings, L"127.0.0.1:0", receiverSecSettings);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(FramingProtectionEnabledByConfig)
    {
        ENTER;

        SecurityConfig & securityConfig = SecurityConfig::GetConfig();
        auto saved = securityConfig.FramingProtectionEnabled;
        securityConfig.FramingProtectionEnabled = true;
        KFinally([&]
        {
            securityConfig.FramingProtectionEnabled = saved;
        });

        X509Test_SelfSigned(L"127.0.0.1:0", L"127.0.0.1:0");

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(FramingProtectionEnabled_OneSided1)
    {
        ENTER;

        const wstring senderCn = L"sender.test.com";
        const wstring receiverCn = L"receiver.test.com";
        InstallTestCertInScope senderCert(L"CN=" + senderCn);
        InstallTestCertInScope receiverCert(L"CN=" + receiverCn);

        auto senderSecSettings = TTestUtil::CreateX509SettingsTp(
            senderCert.Thumbprint()->PrimaryToString(),
            L"",
            receiverCert.Thumbprint()->PrimaryToString(),
            L"");

        auto receiverSecSettings = TTestUtil::CreateX509SettingsTp(
            receiverCert.Thumbprint()->PrimaryToString(),
            L"",
            senderCert.Thumbprint()->PrimaryToString(),
            L"");

        receiverSecSettings.SetFramingProtectionEnabledCallback([] { return true; });

        SecurityTest(L"127.0.0.1:0", senderSecSettings, L"127.0.0.1:0", receiverSecSettings);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(FramingProtectionEnabled_OneSided2)
    {
        ENTER;

        const wstring senderCn = L"sender.test.com";
        const wstring receiverCn = L"receiver.test.com";
        InstallTestCertInScope senderCert(L"CN=" + senderCn);
        InstallTestCertInScope receiverCert(L"CN=" + receiverCn);

        auto senderSecSettings = TTestUtil::CreateX509SettingsTp(
            senderCert.Thumbprint()->PrimaryToString(),
            L"",
            receiverCert.Thumbprint()->PrimaryToString(),
            L"");

        senderSecSettings.SetFramingProtectionEnabledCallback([] { return true; });

        SecuritySettings receiverSecSettings = TTestUtil::CreateX509SettingsTp(
            receiverCert.Thumbprint()->PrimaryToString(),
            L"",
            senderCert.Thumbprint()->PrimaryToString(),
            L"");

        SecurityTest(L"127.0.0.1:0", senderSecSettings, L"127.0.0.1:0", receiverSecSettings);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(FramingProtectionEnabledByConfig_ManyMessages)
    {
        ENTER;

        SecurityConfig & securityConfig = SecurityConfig::GetConfig();
        auto saved = securityConfig.FramingProtectionEnabled;
        securityConfig.FramingProtectionEnabled = true;
        KFinally([&]
        {
            securityConfig.FramingProtectionEnabled = saved;
        });

        X509ManyMessage_SelfSigned(L"127.0.0.1:0", L"127.0.0.1:0");

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(FramingProtectionEnabledByConfig_ManySmallMessages)
    {
        ENTER;

        SecurityConfig & securityConfig = SecurityConfig::GetConfig();
        auto saved = securityConfig.FramingProtectionEnabled;
        securityConfig.FramingProtectionEnabled = true;
        KFinally([&]
        {
            securityConfig.FramingProtectionEnabled = saved;
        });

        X509ManySmallMessage_SelfSigned(L"127.0.0.1:0", L"127.0.0.1:0");

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(FramingProtectionEnabled_Negotiate_ByConfig_EncryptAndSign)
    {
        ENTER;
        FramingProtectionEnabled_Negotiate_ByConfig(ProtectionLevel::EncryptAndSign);
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(FramingProtectionEnabled_Negotiate_ByConfig_Sign)
    {
        ENTER;
        FramingProtectionEnabled_Negotiate_ByConfig(ProtectionLevel::Sign);
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(FramingProtectionEnabled_Negotiate_OneSided_EncryptAndSign_1)
    {
        ENTER;

        SecuritySettings senderSecSettings;
        senderSecSettings.Test_SetRawValues(SecurityProvider::Negotiate, ProtectionLevel::EncryptAndSign);

        SecuritySettings receiverSecSettings;
        receiverSecSettings.Test_SetRawValues(SecurityProvider::Negotiate, ProtectionLevel::EncryptAndSign);
        receiverSecSettings.SetFramingProtectionEnabledCallback([] { return true; });

        SecurityTest(L"127.0.0.1:0", senderSecSettings, L"127.0.0.1:0", receiverSecSettings);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(FramingProtectionEnabled_Negotiate_OneSided_EncryptAndSign_2)
    {
        ENTER;

        SecuritySettings senderSecSettings;
        senderSecSettings.Test_SetRawValues(SecurityProvider::Negotiate, ProtectionLevel::EncryptAndSign);
        senderSecSettings.SetFramingProtectionEnabledCallback([] { return true; });

        SecuritySettings receiverSecSettings;
        receiverSecSettings.Test_SetRawValues(SecurityProvider::Negotiate, ProtectionLevel::EncryptAndSign);

        SecurityTest(L"127.0.0.1:0", senderSecSettings, L"127.0.0.1:0", receiverSecSettings);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(FramingProtectionEnabled_Negotiate_OneSided_Sign_1)
    {
        ENTER;

        SecuritySettings senderSecSettings;
        senderSecSettings.Test_SetRawValues(SecurityProvider::Negotiate, ProtectionLevel::Sign);

        SecuritySettings receiverSecSettings;
        receiverSecSettings.Test_SetRawValues(SecurityProvider::Negotiate, ProtectionLevel::Sign);
        receiverSecSettings.SetFramingProtectionEnabledCallback([] { return true; });

        SecurityTest(L"127.0.0.1:0", senderSecSettings, L"127.0.0.1:0", receiverSecSettings);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(FramingProtectionEnabled_Negotiate_OneSided_Sign_2)
    {
        ENTER;

        SecuritySettings senderSecSettings;
        senderSecSettings.Test_SetRawValues(SecurityProvider::Negotiate, ProtectionLevel::Sign);
        senderSecSettings.SetFramingProtectionEnabledCallback([] { return true; });

        SecuritySettings receiverSecSettings;
        receiverSecSettings.Test_SetRawValues(SecurityProvider::Negotiate, ProtectionLevel::Sign);

        SecurityTest(L"127.0.0.1:0", senderSecSettings, L"127.0.0.1:0", receiverSecSettings);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(FramingProtectionEnabled_Negotiate_ManyMessages_EncryptAndSign)
    {
        ENTER;

        SecuritySettings senderSecSettings;
        senderSecSettings.Test_SetRawValues(SecurityProvider::Negotiate, ProtectionLevel::EncryptAndSign);
        senderSecSettings.SetFramingProtectionEnabledCallback([] { return true; });

        SecuritySettings receiverSecSettings;
        receiverSecSettings.Test_SetRawValues(SecurityProvider::Negotiate, ProtectionLevel::EncryptAndSign);

        ManyMessageTest(L"127.0.0.1:0", senderSecSettings, L"127.0.0.1:0", receiverSecSettings);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(FramingProtectionEnabled_Negotiate_ManySmallMessages_EncryptAndSign)
    {
        ENTER;

        SecuritySettings senderSecSettings;
        senderSecSettings.Test_SetRawValues(SecurityProvider::Negotiate, ProtectionLevel::EncryptAndSign);
        senderSecSettings.SetFramingProtectionEnabledCallback([] { return true; });

        SecuritySettings receiverSecSettings;
        receiverSecSettings.Test_SetRawValues(SecurityProvider::Negotiate, ProtectionLevel::EncryptAndSign);

        ManySmallMessageTest(L"127.0.0.1:0", senderSecSettings, L"127.0.0.1:0", receiverSecSettings);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(FramingProtectionEnabled_Negotiate_ManyMessages_Sign)
    {
        ENTER;

        SecuritySettings senderSecSettings;
        senderSecSettings.Test_SetRawValues(SecurityProvider::Negotiate, ProtectionLevel::Sign);
        senderSecSettings.SetFramingProtectionEnabledCallback([] { return true; });

        SecuritySettings receiverSecSettings;
        receiverSecSettings.Test_SetRawValues(SecurityProvider::Negotiate, ProtectionLevel::Sign);

        ManyMessageTest(L"127.0.0.1:0", senderSecSettings, L"127.0.0.1:0", receiverSecSettings);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(FramingProtectionEnabled_Negotiate_ManySmallMessages_Sign)
    {
        ENTER;

        SecuritySettings senderSecSettings;
        senderSecSettings.Test_SetRawValues(SecurityProvider::Negotiate, ProtectionLevel::Sign);
        senderSecSettings.SetFramingProtectionEnabledCallback([] { return true; });

        SecuritySettings receiverSecSettings;
        receiverSecSettings.Test_SetRawValues(SecurityProvider::Negotiate, ProtectionLevel::Sign);

        ManySmallMessageTest(L"127.0.0.1:0", senderSecSettings, L"127.0.0.1:0", receiverSecSettings);

        LEAVE;
    }

#endif

    BOOST_AUTO_TEST_CASE(NonX509SenderWithX509ReceiverTest)
    {
        ENTER;

        auto sender = DatagramTransportFactory::CreateTcpClient();
        VERIFY_IS_TRUE(sender->Start().IsSuccess());

        InstallTestCertInScope testCert;
        SecuritySettings securitySettings = TTestUtil::CreateX509SettingsTp(
            testCert.Thumbprint()->PrimaryToString(),
            L"", 
            testCert.Thumbprint()->PrimaryToString(),
            L"");

        auto receiver = DatagramTransportFactory::CreateTcp(TTestUtil::GetListenAddress());
        VERIFY_IS_TRUE(receiver->SetSecurity(securitySettings).IsSuccess());

        AutoResetEvent messageReceived(false);
        wstring testAction = TTestUtil::GetGuidAction();
        TTestUtil::SetMessageHandler(
            receiver,
            testAction,
            [&receiver, &sender, &messageReceived](MessageUPtr &, ISendTarget::SPtr const & st) -> void
            {
                Trace.WriteInfo(TraceType, "[receiver] got a message from {0}, sender listen address = {1}", st->Address(), sender->ListenAddress());
                messageReceived.Set();
            });

        VERIFY_IS_TRUE(receiver->Start().IsSuccess());

        ISendTarget::SPtr target = sender->ResolveTarget(receiver->ListenAddress());
        VERIFY_IS_TRUE(target);

        for (int i = 0; i < 10; ++ i)
        {
            auto message = make_unique<Message>();
            message->Headers.Add(ActionHeader(testAction));
            message->Headers.Add(MessageIdHeader());

            sender->SendOneWay(target, move(message));
        }

        VERIFY_IS_FALSE(messageReceived.WaitOne(TimeSpan::FromSeconds(3)));

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(ConnectionDeniedTest)
    {
        ENTER;

        const wstring senderCn = L"sender.test.com";
        const wstring receiverCn = L"receiver.test.com";
        InstallTestCertInScope senderCert(L"CN=" + senderCn);
        InstallTestCertInScope receiverCert(L"CN=" + receiverCn);

        SecuritySettings noOneIsAllowedSecuritySettings = TTestUtil::CreateX509SettingsTp(
            receiverCert.Thumbprint()->PrimaryToString(),
            L"",
            L"0000000000000000000000000000000000000000", //Assuming this will match any certificate thumbprints
            L"");

        auto receiver = DatagramTransportFactory::CreateTcp(L"127.0.0.1:0");

        VERIFY_IS_TRUE(receiver->SetSecurity(noOneIsAllowedSecuritySettings).IsSuccess());
        VERIFY_IS_TRUE(receiver->Start().IsSuccess());

        auto sender = DatagramTransportFactory::CreateTcpClient();
        SecuritySettings senderSecSettings = TTestUtil::CreateX509SettingsTp(
            senderCert.Thumbprint()->PrimaryToString(),
            L"", 
            receiverCert.Thumbprint()->PrimaryToString(),
            L"");

        VERIFY_IS_TRUE(sender->SetSecurity(senderSecSettings).IsSuccess());
        VERIFY_IS_TRUE(sender->Start().IsSuccess());

        ISendTarget::SPtr target = sender->ResolveTarget(receiver->ListenAddress());
        VERIFY_IS_TRUE(target);

        AutoResetEvent connectionFaulted(false);
        sender->SetConnectionFaultHandler([&connectionFaulted, &target](ISendTarget const & faultedTarget, ErrorCode fault)
        {
            Trace.WriteInfo(TraceType, "connection to {0} faulted: {1}", faultedTarget.Address(), fault);
            VERIFY_IS_TRUE(target.get() == &faultedTarget);
            VERIFY_IS_TRUE(fault.IsError(ErrorCodeValue::ConnectionDenied));
            connectionFaulted.Set();
        });

        auto msg = make_unique<Message>();
        msg->Headers.Add(MessageIdHeader());

        AutoResetEvent sendFailed(false);
        msg->SetSendStatusCallback([&sendFailed](ErrorCode const & error, MessageUPtr&&)
        {
            Trace.WriteInfo(TraceType, "send failed: {0}", error);
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::ConnectionDenied));
            sendFailed.Set();
        });

        Trace.WriteInfo(TraceType, "sending test message {0}", msg->TraceId());
        auto error = sender->SendOneWay(target, move(msg));
        VERIFY_IS_TRUE(error.IsSuccess());

        VERIFY_IS_TRUE(sendFailed.WaitOne(TimeSpan::FromSeconds(15)));
        VERIFY_IS_TRUE(connectionFaulted.WaitOne(TimeSpan::FromSeconds(15)));

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(SecureMessageSendCallBackTest)
    {
        ENTER;

        auto receiver = DatagramTransportFactory::CreateTcp(L"127.0.0.1:0");

        InstallTestCertInScope testCert;
        SecuritySettings securitySettings = TTestUtil::CreateX509SettingsTp(
            testCert.Thumbprint()->PrimaryToString(),
            L"",
            testCert.Thumbprint()->PrimaryToString(),
            L"");

        VERIFY_IS_TRUE(receiver->SetSecurity(securitySettings).IsSuccess());
   
   
        VERIFY_IS_TRUE(receiver->Start().IsSuccess());

        auto sender = DatagramTransportFactory::CreateTcpClient();
        VERIFY_IS_TRUE(sender->SetSecurity(securitySettings).IsSuccess());
        VERIFY_IS_TRUE(sender->Start().IsSuccess());

        ISendTarget::SPtr target = sender->ResolveTarget(receiver->ListenAddress());
        VERIFY_IS_TRUE(target);

        AutoResetEvent connectionFaulted(false);
    

        auto msg = make_unique<Message>();
        msg->Headers.Add(MessageIdHeader());

        AutoResetEvent sendFailed(false);
        msg->SetSendStatusCallback([&sendFailed](ErrorCode const & error, MessageUPtr&&)
        {
            Trace.WriteInfo(TraceType, "Send Completed : {0}", error);
            VERIFY_IS_TRUE(error.IsSuccess());
            sendFailed.Set();
        });

        Trace.WriteInfo(TraceType, "sending test message {0}", msg->TraceId());
        auto error = sender->SendOneWay(target, move(msg));
        VERIFY_IS_TRUE(error.IsSuccess());

        VERIFY_IS_TRUE(sendFailed.WaitOne(TimeSpan::FromSeconds(15)));

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(ClientServerMessageSizeLimitTest_X509_WithoutClientRole1)
    {
        ENTER;
        ClientServerMessageSizeLimitTest_WithoutClientRole(true, true);
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(ClientServerMessageSizeLimitTest_X509_WithoutClientRole2)
    {
        ENTER;
        ClientServerMessageSizeLimitTest_WithoutClientRole(true, false);
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(ClientServerMessageSizeLimitTest_X509_WithoutClientRole3)
    {
        ENTER;
        ClientServerMessageSizeLimitTest_WithoutClientRole(false, true);
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(ClientServerMessageSizeLimitTest_X509_WithoutClientRole4)
    {
        ENTER;
        ClientServerMessageSizeLimitTest_WithoutClientRole(false, false);
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(ClientServerMessageSizeLimitTest_X509_ClientRoleEnabled)
    {
        ENTER;

        Assert::DisableDebugBreakInThisScope disableDebugBreakInThisScope;
        Assert::DisableTestAssertInThisScope disableTestAssertInThisScope;

        const wstring clientCn = L"client.test.com";
        const wstring adminClinetCn = L"adminclient.test.com";
        const wstring serverCn = L"server.test.com";
        InstallTestCertInScope clientCert(L"CN=" + clientCn);
        InstallTestCertInScope adminClientCert(L"CN=" + adminClinetCn);
        InstallTestCertInScope serverCert(L"CN=" + serverCn);

        auto clientSecSettings = TTestUtil::CreateX509SettingsTp(
            clientCert.Thumbprint()->PrimaryToString(),
            L"", 
            serverCert.Thumbprint()->PrimaryToString(),
            L"");

        auto adminClientSecSettings = TTestUtil::CreateX509SettingsTp(
            adminClientCert.Thumbprint()->PrimaryToString(),
            L"",
            serverCert.Thumbprint()->PrimaryToString(),
            L"");

        auto serverSecSettings = TTestUtil::CreateX509SettingsTp(
            serverCert.Thumbprint()->PrimaryToString(),
            L"",
            clientCert.Thumbprint()->PrimaryToString(),
            L"");

        serverSecSettings.EnableAdminRole(
            adminClientCert.Thumbprint()->PrimaryToString(),
            SecurityConfig::X509NameMap(),
            wstring());

        auto client = DatagramTransportFactory::CreateTcpClient();
        auto adminClient = DatagramTransportFactory::CreateTcpClient();
        auto server = DatagramTransportFactory::CreateTcp(TTestUtil::GetListenAddress());

        VERIFY_IS_TRUE(client->SetSecurity(clientSecSettings).IsSuccess());
        VERIFY_IS_TRUE(adminClient->SetSecurity(adminClientSecSettings).IsSuccess());
        VERIFY_IS_TRUE(server->SetSecurity(serverSecSettings).IsSuccess());

        ULONG lowMessageSizeLimit = 1024;
        client->SetMaxOutgoingFrameSize(lowMessageSizeLimit);
        adminClient->SetMaxOutgoingFrameSize(lowMessageSizeLimit);
        server->SetMaxIncomingFrameSize(lowMessageSizeLimit);

        // Make sure message size is over the limit
        std::wstring requestBodyString;
        while (requestBodyString.size() * sizeof(wchar_t) < TransportSecurity::Test_GetInternalMaxFrameSize(lowMessageSizeLimit))
        {
            requestBodyString += L"Message ";
        }
        requestBodyString += L"oh yeah";

        TcpTestMessage requestBody(requestBodyString);

        AutoResetEvent messageReceived;
        wstring testAction = TTestUtil::GetGuidAction();
        ISendTarget::SPtr clientTargetOnServer;
        TTestUtil::SetMessageHandler(
            server,
            testAction,
            [&messageReceived, requestBody, &clientTargetOnServer, server](MessageUPtr & message, ISendTarget::SPtr const & st) -> void
            {
                Trace.WriteInfo(TraceType, "server got a message from {0}", st->Address());
                clientTargetOnServer = st;

                if (message->Actor == Actor::GenericTestActor)
                {
                    //ping from clients
                    Trace.WriteInfo(TraceType, "server replies to ping {0}", message->TraceId());
                    server->SendOneWay(st, move(message));
                }
                else
                {
                    TcpTestMessage body;
                    VERIFY_IS_TRUE(message->GetBody(body));
                    VERIFY_IS_TRUE(requestBody.message_ == body.message_);
                }

                messageReceived.Set();
            });

        auto maxFrameSize = server->Security()->MaxIncomingFrameSize();
        VERIFY_IS_TRUE(maxFrameSize > lowMessageSizeLimit); // Due to safety margin added inside transport
        VERIFY_ARE_EQUAL2(maxFrameSize, server->Security()->MaxIncomingFrameSize()); // Verification for RDBug 3130417

        VERIFY_IS_TRUE(server->Start().IsSuccess());

        Trace.WriteInfo(TraceType, "test with admin client");
        {
            AutoResetEvent adminClientReceivedMsg(false);
            TTestUtil::SetMessageHandler(
                adminClient,
                testAction,
                [&adminClientReceivedMsg, &requestBody](MessageUPtr & message, ISendTarget::SPtr const & st) -> void
                {
                    Trace.WriteInfo(TraceType, "admin client got a message from {0}", st->Address());

                    if (message->Actor != Actor::GenericTestActor)
                    {
                        TcpTestMessage body;
                        VERIFY_IS_TRUE(message->GetBody(body));
                        VERIFY_IS_TRUE(requestBody.message_ == body.message_);
                        adminClientReceivedMsg.Set();
                    }
                    else
                    {
                        Trace.WriteInfo(TraceType, "admin client ignores ping reply");
                    }
                });

            VERIFY_IS_TRUE(adminClient->Start().IsSuccess());

            ISendTarget::SPtr target = adminClient->ResolveTarget(server->ListenAddress(), L"");
            VERIFY_IS_TRUE(target);

            auto ping = make_unique<Message>();
            ping->Headers.Add(ActionHeader(testAction));
            ping->Headers.Add(ActorHeader(Actor::GenericTestActor));
            ping->Headers.Add(MessageIdHeader());
            Trace.WriteInfo(TraceType, "ping with {0} from admin client", ping->TraceId());
            auto error = adminClient->SendOneWay(target, std::move(ping));
            VERIFY_IS_TRUE(error.IsSuccess());
            VERIFY_IS_TRUE(messageReceived.WaitOne(TimeSpan::FromSeconds(10)));
            //no need to wait for ping reply to be recevied by admin clients. unlike non-admin clients,
            //oversized outgoing messages from admin clients should not be dropped either before or after
            //the connection is confirmed

            auto oversizedFromAdminClient = make_unique<Message>(requestBody);
            oversizedFromAdminClient->Headers.Add(ActionHeader(testAction));
            oversizedFromAdminClient->Headers.Add(MessageIdHeader());
            Trace.WriteInfo(TraceType, "sending oversized message {0} from admin client", oversizedFromAdminClient->TraceId());
            error = adminClient->SendOneWay(target, std::move(oversizedFromAdminClient));
            VERIFY_IS_TRUE(error.IsSuccess());

            Trace.WriteInfo(TraceType, "Oversized message from admin client should not be dropped");
            VERIFY_IS_TRUE(messageReceived.WaitOne(TimeSpan::FromSeconds(10)));

            auto largeMessageFromServerToAdminClient = make_unique<Message>(requestBody);
            largeMessageFromServerToAdminClient->Headers.Add(ActionHeader(testAction));
            largeMessageFromServerToAdminClient->Headers.Add(MessageIdHeader());
            Trace.WriteInfo(TraceType, "server sending oversized message {0} to adminClient", largeMessageFromServerToAdminClient->TraceId());
            error = server->SendOneWay(clientTargetOnServer, move(largeMessageFromServerToAdminClient));
            Trace.WriteInfo(TraceType, "server send returned {0}, server should be able to send oversized message to client", error);
            VERIFY_IS_TRUE(error.IsSuccess());
            VERIFY_IS_TRUE(adminClientReceivedMsg.WaitOne(TimeSpan::FromSeconds(10)));
        }

        Trace.WriteInfo(TraceType, "Testing with non-admin client");
        {
            AutoResetEvent clientReceivedMsg(false);
            TTestUtil::SetMessageHandler(
                client,
                testAction,
                [&clientReceivedMsg, &requestBody](MessageUPtr & message, ISendTarget::SPtr const & st) -> void
                {
                    Trace.WriteInfo(TraceType, "client got a message from {0}", st->Address());

                    if (message->Actor != Actor::GenericTestActor)
                    {
                        TcpTestMessage body;
                        VERIFY_IS_TRUE(message->GetBody(body));
                        VERIFY_IS_TRUE(requestBody.message_ == body.message_);
                    }

                    clientReceivedMsg.Set();
                });

            VERIFY_IS_TRUE(client->Start().IsSuccess());

            ISendTarget::SPtr target = client->ResolveTarget(server->ListenAddress(), L"");
            VERIFY_IS_TRUE(target);

            auto ping = make_unique<Message>();
            ping->Headers.Add(ActionHeader(testAction));
            ping->Headers.Add(ActorHeader(Actor::GenericTestActor));
            ping->Headers.Add(MessageIdHeader());
            Trace.WriteInfo(
                TraceType,
                "sending ping {0} to trigger security negotiaion, dropping on sending side is only enabled after security negotiation is completed",
                ping->TraceId());
            auto error = client->SendOneWay(target, std::move(ping));
            VERIFY_IS_TRUE(error.IsSuccess());
            Trace.WriteInfo(TraceType, "server waiting for message from client");
            VERIFY_IS_TRUE(messageReceived.WaitOne(TimeSpan::FromSeconds(10)));
            Trace.WriteInfo(TraceType, "client waiting for ping reply to make sure security negotiation is completed on client side");
            VERIFY_IS_TRUE(clientReceivedMsg.WaitOne(TimeSpan::FromSeconds(10)));

            auto oneway = make_unique<Message>(requestBody);
            oneway->Headers.Add(ActionHeader(testAction));
            oneway->Headers.Add(MessageIdHeader());
            size_t messageSize = oneway->SerializedSize();

            error = client->SendOneWay(target, std::move(oneway));
            Trace.WriteInfo(TraceType, "Message should be dropped due to size limit on the sender");
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::MessageTooLarge));
            VERIFY_IS_FALSE(messageReceived.WaitOne(TimeSpan::FromMilliseconds(100)));

            auto largeMessageFromServerToClient = make_unique<Message>(requestBody);
            largeMessageFromServerToClient->Headers.Add(ActionHeader(testAction));
            largeMessageFromServerToClient->Headers.Add(MessageIdHeader());
            error = server->SendOneWay(clientTargetOnServer, move(largeMessageFromServerToClient));
            Trace.WriteInfo(TraceType, "server send returned {0}, server should be able to send oversized message to client", error);
            VERIFY_IS_TRUE(error.IsSuccess());
            VERIFY_IS_TRUE(clientReceivedMsg.WaitOne(TimeSpan::FromSeconds(10)));

            Trace.WriteInfo(TraceType, "Increase message size limit on sender: {0} -> {1}", lowMessageSizeLimit, messageSize);
            client->SetMaxOutgoingFrameSize(static_cast<ULONG>(messageSize));

            auto oneway1 = make_unique<Message>(requestBody);
            oneway1->Headers.Add(ActionHeader(testAction));
            oneway1->Headers.Add(MessageIdHeader());
            error = client->SendOneWay(target, std::move(oneway1));
            VERIFY_IS_TRUE(error.IsSuccess());

            Trace.WriteInfo(TraceType, "Message should be dropped due to size limit on the receiver");
            VERIFY_IS_FALSE(messageReceived.WaitOne(TimeSpan::FromSeconds(0.1)), L"Message should be dropped due to size limit on the receiver");

            Trace.WriteInfo(TraceType, "Increase message size limit on receiver: {0} -> {1}", lowMessageSizeLimit, messageSize);
            server->SetMaxIncomingFrameSize(static_cast<ULONG>(messageSize));

            Trace.WriteInfo(TraceType, "wait for old connection cleanup to complete so that new messages will not be dropped due to cleanup");
            TTestUtil::WaitUntil([&] { return target->ConnectionCount() == 0; }, TimeSpan::FromSeconds(3));

            auto oneway2 = make_unique<Message>(requestBody);
            oneway2->Headers.Add(ActionHeader(testAction));
            oneway2->Headers.Add(MessageIdHeader());
            Trace.WriteInfo(TraceType, "sending after fixing size limits");
            error = client->SendOneWay(target, std::move(oneway2));
            VERIFY_IS_TRUE(error.IsSuccess());

            Trace.WriteInfo(TraceType, "Message should be received since both limits have been raised");
            VERIFY_IS_TRUE(messageReceived.WaitOne(TimeSpan::FromSeconds(10)), L"Message should be received since both limits have been raised");

            Trace.WriteInfo(TraceType, "Reduce message size limit on receiver: {0} -> {1}", messageSize, lowMessageSizeLimit);
            server->SetMaxIncomingFrameSize(lowMessageSizeLimit);

            auto oneway3 = make_unique<Message>(requestBody);
            oneway3->Headers.Add(ActionHeader(testAction));
            oneway3->Headers.Add(MessageIdHeader());
            error = client->SendOneWay(target, std::move(oneway3));
            VERIFY_IS_TRUE(error.IsSuccess());

            Trace.WriteInfo(TraceType, "Message should be dropped due to incoming message size limit on the server");
            VERIFY_IS_FALSE(messageReceived.WaitOne(TimeSpan::FromSeconds(0.1)), L"Message should be dropped due to size limit on the receiver");

            Trace.WriteInfo(TraceType, "wait for old connection cleanup to complete so that new messages will not be dropped due to cleanup");
            TTestUtil::WaitUntil([&] { return target->ConnectionCount() == 0; }, TimeSpan::FromSeconds(3));

            Trace.WriteInfo(TraceType, "Reduce message size limit on sender: {0} -> {1}", messageSize, lowMessageSizeLimit);
            client->SetMaxOutgoingFrameSize(lowMessageSizeLimit);

            ping = make_unique<Message>();
            ping->Headers.Add(ActionHeader(testAction));
            ping->Headers.Add(ActorHeader(Actor::GenericTestActor));
            ping->Headers.Add(MessageIdHeader());
            Trace.WriteInfo(TraceType, "ping again with {0} as connection was closed by server due to oversized incoming message above", ping->TraceId());
            error = client->SendOneWay(target, std::move(ping));
            Trace.WriteInfo(TraceType, "client send returned {0}", error);
            VERIFY_IS_TRUE(error.IsSuccess());
            Trace.WriteInfo(TraceType, "server waiting for message from client");
            VERIFY_IS_TRUE(messageReceived.WaitOne(TimeSpan::FromSeconds(10)));
            Trace.WriteInfo(TraceType, "client waiting for ping reply to make sure security negotiation is completed on client side");
            VERIFY_IS_TRUE(clientReceivedMsg.WaitOne(TimeSpan::FromSeconds(10)));

            auto oneway4 = make_unique<Message>(requestBody);
            oneway4->Headers.Add(ActionHeader(testAction));
            oneway4->Headers.Add(MessageIdHeader());
            error = client->SendOneWay(target, std::move(oneway4));
            Trace.WriteInfo(TraceType, "client send returned {0}, message should be dropped due to size limit on the sender", error);
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::MessageTooLarge));
            VERIFY_IS_FALSE(messageReceived.WaitOne(TimeSpan::FromMilliseconds(100)));
        }

        client->Stop();
        adminClient->Stop();
        server->Stop();

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(ReplyBeyondClientLimit)
    {
        ENTER;

        Assert::DisableTestAssertInThisScope disableTestAssertInThisScope;

        const wstring clientCn = L"client.test.com";
        const wstring serverCn = L"server.test.com";
        InstallTestCertInScope clientCert(L"CN=" + clientCn);
        InstallTestCertInScope serverCert(L"CN=" + serverCn);

        auto clientSecSettings = TTestUtil::CreateX509SettingsTp(
            clientCert.Thumbprint()->PrimaryToString(),
            L"",
            serverCert.Thumbprint()->PrimaryToString(),
            L"");

        auto serverSecSettings = TTestUtil::CreateX509SettingsTp(
            serverCert.Thumbprint()->PrimaryToString(),
            L"",
            clientCert.Thumbprint()->PrimaryToString(),
            L"");

        auto clientRootSPtr = make_shared<ComponentRoot>();

        auto client = TcpDatagramTransport::CreateClient();
        VERIFY_IS_TRUE(client->SetSecurity(clientSecSettings).IsSuccess());
        auto size = 10000;
        client->SetMaxIncomingFrameSize(static_cast<ULONG>(size));

        auto clientDemuxer = make_shared<Demuxer>(*clientRootSPtr, client);

        auto rrw = RequestReplyWrapper::Create(client);
        auto requestReply = rrw->InitRequestReply();

        clientDemuxer->SetReplyHandler(*requestReply);

        VERIFY_IS_TRUE(clientDemuxer->Open().IsSuccess());
        VERIFY_IS_TRUE(client->Start().IsSuccess());

        auto serverRoot = make_shared<ComponentRoot>();

        auto server = TcpDatagramTransport::Create(TTestUtil::GetListenAddress());
        VERIFY_IS_TRUE(server->SetSecurity(serverSecSettings).IsSuccess());
        auto serverDemuxer = make_shared<Demuxer>(*serverRoot, server);

        serverDemuxer->RegisterMessageHandler(
            Actor::GenericTestActor,
            [&](MessageUPtr &, ReceiverContextUPtr & receiverContext)
            {
                auto reply = (make_unique<Message>(TestMessageBody(size * 100)));
                auto replyError = receiverContext->Reply(move(reply));
                Trace.WriteInfo(TraceType, "replyError : {0}", replyError);
                VERIFY_IS_TRUE(replyError.IsError(ErrorCodeValue::MessageTooLarge));
            }, false);
        VERIFY_IS_TRUE(serverDemuxer->Open().IsSuccess());

        VERIFY_IS_TRUE(server->Start().IsSuccess());
        auto address = client->ResolveTarget(server->ListenAddress());
        VERIFY_IS_TRUE(address != nullptr);
        ManualResetEvent requestCompleted(false);
        TimeSpan timeout = TimeSpan::FromSeconds(20);

        auto request = (make_unique<Message>(TestMessageBody()));
        request->Headers.Add(ActorHeader(Actor::GenericTestActor));
        request->Headers.Add(MessageIdHeader());

        ErrorCode error;
        requestReply->BeginRequest(
            move(request),
            address,
            timeout,
            [requestReply, &requestCompleted, &error](AsyncOperationSPtr const & operation)
            {
                MessageUPtr reply;
                error = requestReply->EndRequest(operation, reply);
                requestCompleted.Set();
            },
            rrw->CreateAsyncOperationRoot());

        VERIFY_IS_TRUE(requestCompleted.WaitOne(timeout));
        Trace.WriteInfo(TraceType, "request&reply error: {0}", error);
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::MessageTooLarge));

        client->Stop();
        clientDemuxer->Close();
        requestReply->Close();
        server->Stop();
        serverDemuxer->Close();

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(ReplyBeyondServerSideLimit)
    {
        ENTER;

        Assert::DisableTestAssertInThisScope disableTestAssertInThisScope;

        const wstring clientCn = L"client.test.com";
        const wstring serverCn = L"server.test.com";
        InstallTestCertInScope clientCert(L"CN=" + clientCn);
        InstallTestCertInScope serverCert(L"CN=" + serverCn);

        auto clientSecSettings = TTestUtil::CreateX509SettingsTp(
            clientCert.Thumbprint()->PrimaryToString(),
            L"",
            serverCert.Thumbprint()->PrimaryToString(),
            L"");

        auto serverSecSettings = TTestUtil::CreateX509SettingsTp(
            serverCert.Thumbprint()->PrimaryToString(),
            L"",
            clientCert.Thumbprint()->PrimaryToString(),
            L"");

        auto clientRootSPtr = make_shared<ComponentRoot>();

        auto client = TcpDatagramTransport::CreateClient();
        VERIFY_IS_TRUE(client->SetSecurity(clientSecSettings).IsSuccess());

        auto clientDemuxer = make_shared<Demuxer>(*clientRootSPtr, client);

        auto rrw = RequestReplyWrapper::Create(client);
        auto requestReply = rrw->InitRequestReply();

        clientDemuxer->SetReplyHandler(*requestReply);

        VERIFY_IS_TRUE(clientDemuxer->Open().IsSuccess());
        VERIFY_IS_TRUE(client->Start().IsSuccess());

        auto serverRoot = make_shared<ComponentRoot>();

        auto server = TcpDatagramTransport::Create(TTestUtil::GetListenAddress());
        VERIFY_IS_TRUE(server->SetSecurity(serverSecSettings).IsSuccess());
        ULONG serverOutgoingLimit = 10000;
        server->SetMaxOutgoingFrameSize(serverOutgoingLimit);

        auto serverDemuxer = make_shared<Demuxer>(*serverRoot, server);

        serverDemuxer->RegisterMessageHandler(
            Actor::GenericTestActor,
            [&](MessageUPtr &, ReceiverContextUPtr & receiverContext)
            {
                auto reply = (make_unique<Message>(TestMessageBody(serverOutgoingLimit * 100)));
                VERIFY_IS_TRUE(receiverContext->ReplyTarget->MaxOutgoingMessageSize() > TransportSecurity::Test_GetInternalMaxFrameSize(serverOutgoingLimit));
                auto replyError = receiverContext->Reply(move(reply));
                Trace.WriteInfo(TraceType, "reply error: {0}", replyError);
                VERIFY_IS_TRUE(replyError.IsSuccess());
            },
            false);
        VERIFY_IS_TRUE(serverDemuxer->Open().IsSuccess());

        VERIFY_IS_TRUE(server->Start().IsSuccess());
        auto address = client->ResolveTarget(server->ListenAddress());
        VERIFY_IS_TRUE(address != nullptr);
        ManualResetEvent requestCompleted(false);
        TimeSpan timeout = TimeSpan::FromSeconds(3);

        auto request = (make_unique<Message>(TestMessageBody()));
        request->Headers.Add(ActorHeader(Actor::GenericTestActor));
        request->Headers.Add(MessageIdHeader());

        ErrorCode error;
        requestReply->BeginRequest(
            move(request),
            address,
            timeout,
            [requestReply, &requestCompleted, &error](AsyncOperationSPtr const & operation)
            {
                MessageUPtr reply;
                error = requestReply->EndRequest(operation, reply);
                requestCompleted.Set();
            },
            rrw->CreateAsyncOperationRoot());

        VERIFY_IS_TRUE(requestCompleted.WaitOne(timeout));
        Trace.WriteInfo(TraceType, "request&reply error: {0}", error);
        VERIFY_IS_TRUE(error.IsSuccess());

        client->Stop();
        clientDemuxer->Close();
        requestReply->Close();
        server->Stop();
        serverDemuxer->Close();

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(EncryptionTestsX509)
    {
        // Test with different outgoing chunk sizes, some of them are larger than SSL record size
        EncryptionTestX509(3);
        EncryptionTestX509(8);
        EncryptionTestX509(23);
        EncryptionTestX509(512);
        EncryptionTestX509(1024);
        EncryptionTestX509(4095);
        EncryptionTestX509(1023 * 13);
        EncryptionTestX509(1024 * 17);
        EncryptionTestX509(1024 * 32);
    }

    BOOST_AUTO_TEST_CASE(AdminRoleTestX509_ClientRoleDisabled)
    {
        ENTER;

        InstallTestCertInScope testCert;
        SecuritySettings securitySettings = TTestUtil::CreateX509SettingsTp(
            testCert.Thumbprint()->PrimaryToString(),
            L"", 
            testCert.Thumbprint()->PrimaryToString(),
            L"");

        AdminRoleTest(securitySettings, false, false);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(AdminRoleTestX509_IncorrectId)
    {
        ENTER;

        InstallTestCertInScope testCert;
        SecuritySettings securitySettings = TTestUtil::CreateX509SettingsTp(
            testCert.Thumbprint()->PrimaryToString(),
            L"", 
            testCert.Thumbprint()->PrimaryToString(),
            L"");

        AdminRoleTest(securitySettings, true, false);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(NegotiationTimeoutTestSsl)
    {
        ENTER;
        NegotiationTimeoutTest(SecurityProvider::Ssl);
        LEAVE;
    }

#ifndef PLATFORM_UNIX

    BOOST_AUTO_TEST_CASE(AdminRoleTestX509_CorrectId)
    {
        ENTER;

        SecuritySettings securitySettings = TTestUtil::CreateX509Settings(
            L"CN=WinFabric-Test-SAN1-Bob",
            L"WinFabricRing.Rings.WinFabricTestDomain.com");

        // test with client cert thumbprint and common name
        AdminRoleTest(securitySettings, true, true);
        AdminRoleTest(securitySettings, true, true);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(TcpIPv4NegotiateSspiIntegrityTest)
    {
        ENTER;
        BasicTestWin(
            SecurityProvider::Negotiate,
            L"127.0.0.1:0",
            L"127.0.0.1:0",
            ProtectionLevel::Sign);
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(TcpIPv6NegotiateSspiIntegrityTest)
    {
        ENTER;
        BasicTestWin(
            SecurityProvider::Negotiate,
            L"[::1]:0",
            L"[0:0:0:0:0:0:0:1]:0",
            ProtectionLevel::Sign);
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(TcpIPv4NegotiateSspiEncryptionTest)
    {
        ENTER;
        BasicTestWin(
            SecurityProvider::Negotiate,
            L"127.0.0.1:0",
            L"127.0.0.1:0",
            ProtectionLevel::EncryptAndSign);
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(TcpIPv6NegotiateSspiEncryptionTest)
    {
        ENTER;
        BasicTestWin(
            SecurityProvider::Negotiate,
            L"[::1]:0",
            L"[0:0:0:0:0:0:0:1]:0",
            ProtectionLevel::EncryptAndSign);
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(TcpIPv4KerberosSspiIntegrityTest)
    {
        ENTER;
        BasicTestWin(
            SecurityProvider::Kerberos,
            L"127.0.0.1:0",
            L"127.0.0.1:0",
            ProtectionLevel::Sign);
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(TcpIPv6KerberosSspiIntegrityTest)
    {
        ENTER;
        BasicTestWin(
            SecurityProvider::Kerberos,
            L"[::1]:0",
            L"[0:0:0:0:0:0:0:1]:0",
            ProtectionLevel::Sign);
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(TcpIPv4KerberosSspiEncryptionTest)
    {
        ENTER;
        BasicTestWin(
            SecurityProvider::Kerberos,
            L"127.0.0.1:0",
            L"127.0.0.1:0",
            ProtectionLevel::EncryptAndSign);
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(TcpIPv6KerberosSspiEncryptionTest)
    {
        ENTER;
        BasicTestWin(
            SecurityProvider::Kerberos,
            L"[::1]:0",
            L"[0:0:0:0:0:0:0:1]:0",
            ProtectionLevel::EncryptAndSign);
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(SignTestsWithDeletedHeaders)
    {
        ENTER;

        bool deleteHeaderFlags[] = {true, false};
        for (int f = 0; f < 2; ++ f)
        {
            Trace.WriteInfo(TraceType, "delete header = {0}", deleteHeaderFlags[f]);

            for (size_t i = 1; i < 100; ++ i)
            {
                SignTestWithDeletedHeaders(i, deleteHeaderFlags[f]);
            }

            SignTestWithDeletedHeaders(127, deleteHeaderFlags[f]);
            SignTestWithDeletedHeaders(128, deleteHeaderFlags[f]);
            SignTestWithDeletedHeaders(129, deleteHeaderFlags[f]);

            SignTestWithDeletedHeaders(255, deleteHeaderFlags[f]);
            SignTestWithDeletedHeaders(256, deleteHeaderFlags[f]);
            SignTestWithDeletedHeaders(257, deleteHeaderFlags[f]);

            SignTestWithDeletedHeaders(511, deleteHeaderFlags[f]);
            SignTestWithDeletedHeaders(512, deleteHeaderFlags[f]);
            SignTestWithDeletedHeaders(513, deleteHeaderFlags[f]);

            SignTestWithDeletedHeaders(1023, deleteHeaderFlags[f]);
            SignTestWithDeletedHeaders(1024, deleteHeaderFlags[f]);
            SignTestWithDeletedHeaders(1025, deleteHeaderFlags[f]);

            SignTestWithDeletedHeaders(2047, deleteHeaderFlags[f]);
            SignTestWithDeletedHeaders(2048, deleteHeaderFlags[f]);
            SignTestWithDeletedHeaders(2049, deleteHeaderFlags[f]);

            SignTestWithDeletedHeaders(4095, deleteHeaderFlags[f]);
            SignTestWithDeletedHeaders(4096, deleteHeaderFlags[f]);
            SignTestWithDeletedHeaders(4097, deleteHeaderFlags[f]);

            SignTestWithDeletedHeaders(8 * 1024, deleteHeaderFlags[f]);
            SignTestWithDeletedHeaders(16 * 1024, deleteHeaderFlags[f]);
            SignTestWithDeletedHeaders(32 * 1024, deleteHeaderFlags[f]);
            SignTestWithDeletedHeaders(64 * 1024, deleteHeaderFlags[f]);
        }

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(AdminRoleTestKerberos_ClientRoleDisabled)
    {
        ENTER;
        SecuritySettings secSet = TTestUtil::CreateKerbSettings();
        AdminRoleTest(secSet, false, false);
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(AdminRoleTestKerberos_CorrectId)
    {
        ENTER;
        SecuritySettings secSet = TTestUtil::CreateKerbSettings();        
        AdminRoleTest(secSet, true, true);
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(AdminRoleTestKerberos_IncorrectId)
    {
        ENTER;
        SecuritySettings secSet = TTestUtil::CreateKerbSettings();        
        AdminRoleTest(secSet, true, false);
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(NegotiationTimeoutTestKerb)
    {
        ENTER;
        NegotiationTimeoutTest(SecurityProvider::Kerberos);
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509CertIssuerMatchTest1)
    {
        ENTER;
        X509CertIssuerMatchTest(L"b3 44 9b 01 8d 0f 68 39 a2 c5 d6 2b 5b 6c 6a c8 22 b6 f6 62", true);
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509CertIssuerMatchTest2)
    {
        ENTER;
        X509CertIssuerMatchTest(L"bc 21 ae 9f 0b 88 cf 6e a9 b4 d6 23 3f 97 2a 60 63 b2 25 a9", false);
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509CertIssuerMatchTest3)
    {
        ENTER;
        X509CertIssuerMatchTest(L"bc 21 ae 9f 0b 88 cf 6e a9 b4 d6 23 3f 97 2a 60 63 b2 25 a9,b3 44 9b 01 8d 0f 68 39 a2 c5 d6 2b 5b 6c 6a c8 22 b6 f6 62", true);
        LEAVE;
    }


    BOOST_AUTO_TEST_CASE(X509CertIssuerMatchTest_UntrustedRoot)
    {
        ENTER;

        cout << "test with certs validated by subject + issuer, chained to untrusted root, success expected" << endl;

        auto subjectName = L"untrusted-root.cert-issuer-match.test.servicefabric";
        auto subject = wformatString("CN={0}", subjectName);
        auto certKeyContainer = L"sf.transport.security.test";

        Common::StringMap subjectIssuerMap;

        // generate a self-signed certificate
        InstallTestCertInScope cert(
            true,                                           // do install
            subject,
            nullptr,                                        // no SANs
            InstallTestCertInScope::DefaultCertExpiry(),
            X509Default::StoreName(),
            certKeyContainer,
            X509StoreLocation::CurrentUser);

        // 1. verify a matching cert (subject + issuer) with an untrusted root is accepted
        subjectIssuerMap.clear();
        subjectIssuerMap.insert(std::pair<wstring, wstring>(subjectName, cert.Thumbprint()->PrimaryToString()));
        auto names = SecurityConfig::X509NameMap::Parse(subjectIssuerMap);
        ThumbprintSet emptyX5tSet = {};
        auto result = SecurityContextSsl::VerifyCertificate(
            L"untrusted root cert test.1 - basic",  // trace id
            cert.CertContext(),                     // cert being verified
            0,                                      // no CRL check
            true,                                   // ignore CRL offline
            false,                                  // no peer authentication
            emptyX5tSet,                            // no TP-based validation
            names,                                  // match by subject and issuer
            true);                                  // do trace cert
        VERIFY_IS_TRUE(SUCCEEDED(result));

        // 2. Verify a subject-matching cert is not accepted if the issuer is missing
        subjectIssuerMap.clear();
        subjectIssuerMap.insert(std::pair<wstring, wstring>(subjectName, L""));
        names = SecurityConfig::X509NameMap::Parse(subjectIssuerMap);
        result = SecurityContextSsl::VerifyCertificate(
            L"untrusted root cert test.2 - no issuer",  // trace id
            cert.CertContext(),                         // cert being verified
            0,                                          // no CRL check
            true,                                       // ignore CRL offline
            false,                                      // no peer authentication
            emptyX5tSet,                                // no TP-based validation
            names,                                      // match by subject and issuer
            true);                                      // do trace cert
        VERIFY_IS_TRUE(ErrorCodeValue::CertificateNotMatched == result);

        // 3. Verify a subject-matching cert is not accepted if the issuer does not match
        subjectIssuerMap.clear();
        subjectIssuerMap.insert(std::pair<wstring, wstring>(subjectName, L"deadbeef00deadbeef00deadbeef00deadbeef00"));
        names = SecurityConfig::X509NameMap::Parse(subjectIssuerMap);
        result = SecurityContextSsl::VerifyCertificate(
            L"untrusted root cert test.3 - mismatching issuer", // trace id
            cert.CertContext(),                                 // cert being verified
            0,                                                  // no CRL check
            true,                                               // ignore CRL offline
            false,                                              // no peer authentication
            emptyX5tSet,                                        // no TP-based validation
            names,                                              // match by subject and issuer
            true);                                              // do trace cert
        VERIFY_IS_TRUE(ErrorCodeValue::CertificateNotMatched == result);

        // 4. verify a non-matching cert is not accepted
        subjectIssuerMap.clear();
        subjectIssuerMap.insert(std::pair<wstring, wstring>(L"unexpected.match", cert.Thumbprint()->PrimaryToString()));
        names = SecurityConfig::X509NameMap::Parse(subjectIssuerMap);
        result = SecurityContextSsl::VerifyCertificate(
            L"untrusted root cert test.4 - mismatching subject",// trace id
            cert.CertContext(),                                 // cert being verified
            0,                                                  // no CRL check
            true,                                               // ignore CRL offline
            false,                                              // no peer authentication
            emptyX5tSet,                                        // no TP-based validation
            names,                                              // match by subject and issuer
            true);                                              // do trace cert
        VERIFY_IS_TRUE(ErrorCodeValue::CertificateNotMatched == result);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(SecurityProviderMismatchTest)
    {
        ENTER;

        auto server = DatagramTransportFactory::CreateTcp(L"127.0.0.1:0");
        SecuritySettings serverSecurity = TTestUtil::CreateKerbSettings();
        VERIFY_IS_TRUE(server->SetSecurity(serverSecurity).IsSuccess());
        VERIFY_IS_TRUE(server->Start().IsSuccess());

        InstallTestCertInScope clientCert(L"CN=client@SecurityProviderMismatchTest");
        SecuritySettings clientSecurity = TTestUtil::CreateX509SettingsTp(
            clientCert.Thumbprint()->PrimaryToString(),
            L"",
            clientCert.Thumbprint()->PrimaryToString(),
            L"");

        auto client = DatagramTransportFactory::CreateTcpClient();
        VERIFY_IS_TRUE(client->SetSecurity(clientSecurity).IsSuccess());
        VERIFY_IS_TRUE(client->Start().IsSuccess());

        auto msg(make_unique<Message>());
        msg->Headers.Add(MessageIdHeader());

        ISendTarget::SPtr target = client->ResolveTarget(server->ListenAddress());
        VERIFY_IS_TRUE(target);

        AutoResetEvent connectionFaulted(false);
        ErrorCode error;
        client->SetConnectionFaultHandler([&connectionFaulted, &error](ISendTarget const & faultedTarget, ErrorCode fault)
        {
            Trace.WriteInfo(TraceType, "connection to {0} faulted: {1}:{2}", faultedTarget.Address(), fault, fault.Message);
            error = fault;
            connectionFaulted.Set();
        });

        client->SendOneWay(target, move(msg));

        VERIFY_IS_TRUE(connectionFaulted.WaitOne(TimeSpan::FromSeconds(20)));
        Trace.WriteInfo(TraceType, "connetion fault = {0}", error);

        LEAVE;
    }

#endif

    BOOST_AUTO_TEST_CASE(X509_LoadByThumbprint_DoubleLoad)
    {
        ENTER;

        InstallTestCertInScope cert;

        vector<SecurityCredentialsSPtr> cred;
        auto err = SecurityCredentials::AcquireSsl(
            cert.StoreLocation(),
            cert.StoreName(),
            cert.Thumbprint(),
            &cred,
            nullptr);

        Trace.WriteInfo(TraceType, "reload should get the same result");
        vector<SecurityCredentialsSPtr> cred2;
        err = SecurityCredentials::AcquireSsl(
            cert.StoreLocation(),
            cert.StoreName(),
            cert.Thumbprint(),
            &cred2,
            nullptr);

        VERIFY_IS_TRUE(SecurityCredentials::SortedVectorsEqual(cred, cred2));

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509_LoadByTwoCommonNames)
    {
        ENTER;

        auto saved = SecurityConfig::GetConfig().UseSecondaryIfNewer;
        SecurityConfig::GetConfig().UseSecondaryIfNewer = true;
        KFinally([=] { SecurityConfig::GetConfig().UseSecondaryIfNewer = saved; });

        auto cn1 = wformatString("name1-{0}", Guid::NewGuid());
        auto cn2 = wformatString("name2-{0}", Guid::NewGuid());
        InstallTestCertInScope testCert1(L"CN=" + cn1);
        InstallTestCertInScope testCert1_2(L"CN=" + cn1, nullptr, InstallTestCertInScope::DefaultCertExpiry() + TimeSpan::FromHours(48));
        InstallTestCertInScope testCert2(L"CN=" + cn2, nullptr, InstallTestCertInScope::DefaultCertExpiry() + TimeSpan::FromHours(72));
        InstallTestCertInScope testCert2_2(L"CN=" + cn2, nullptr, InstallTestCertInScope::DefaultCertExpiry() + TimeSpan::FromHours(24));
        InstallTestCertInScope testCert2_3(L"CN=" + cn2, nullptr, InstallTestCertInScope::DefaultCertExpiry() + TimeSpan::FromHours(70));

        auto securitySettings  = TTestUtil::CreateX509Settings(cn1, cn2, L"DoesNotMatterForThisTest", L"");
        vector<SecurityCredentialsSPtr> credentials, svrCredentials;
        auto error = SecurityCredentials::AcquireSsl(
            securitySettings.X509StoreLocation(),
            securitySettings.X509StoreName(),
            securitySettings.X509FindValue(),
            &credentials,
            &svrCredentials);

        VERIFY_IS_TRUE(error.IsSuccess());

        VERIFY_IS_TRUE(credentials.size() == 5);
        VERIFY_IS_TRUE(svrCredentials.size() == 5);

        Trace.WriteInfo(TraceType, "testCert2 should be loaded because its expiration is later than testCert1");
        VERIFY_IS_TRUE(credentials.front()->X509CertThumbprint() == *(testCert2.Thumbprint()));
        VERIFY_IS_TRUE(svrCredentials.front()->X509CertThumbprint() == *(testCert2.Thumbprint()));

        Trace.WriteInfo(TraceType, "reload should get the same result");
        vector<SecurityCredentialsSPtr> credentials2, svrCredentials2;
        error = SecurityCredentials::AcquireSsl(
            securitySettings.X509StoreLocation(),
            securitySettings.X509StoreName(),
            securitySettings.X509FindValue(),
            &credentials2,
            &svrCredentials2);

        VERIFY_IS_TRUE(SecurityCredentials::SortedVectorsEqual(credentials, credentials2));
        VERIFY_IS_TRUE(SecurityCredentials::SortedVectorsEqual(svrCredentials, svrCredentials2));

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509_LoadByTwoCommonNames2)
    {
        ENTER;

        auto saved = SecurityConfig::GetConfig().UseSecondaryIfNewer;
        SecurityConfig::GetConfig().UseSecondaryIfNewer = true;
        KFinally([=] { SecurityConfig::GetConfig().UseSecondaryIfNewer = saved; });

        auto cn1 = wformatString("name1-{0}", Guid::NewGuid());
        auto cn2 = wformatString("name2-{0}", Guid::NewGuid());
        InstallTestCertInScope testCert1(L"CN=" + cn1);
        InstallTestCertInScope testCert1_2(L"CN=" + cn1, nullptr, InstallTestCertInScope::DefaultCertExpiry() + TimeSpan::FromHours(48));
        InstallTestCertInScope testCert2(L"CN=" + cn2, nullptr, InstallTestCertInScope::DefaultCertExpiry() + TimeSpan::FromHours(72));

        //The following two certs are specified the same expiration as the two above, to test edge case for expiration sorting,
        //the ordering of these two certs relative to the two above will be non-deterministic after sorting.
        InstallTestCertInScope testCert2_2(L"CN=" + cn2, nullptr, InstallTestCertInScope::DefaultCertExpiry() + TimeSpan::FromHours(48));
        InstallTestCertInScope testCert2_3(L"CN=" + cn2, nullptr, InstallTestCertInScope::DefaultCertExpiry() + TimeSpan::FromHours(72));

        auto securitySettings = TTestUtil::CreateX509Settings(cn2, cn1, L"DoesNotMatterForThisTest", L"");
        vector<SecurityCredentialsSPtr> credentials;
        auto error = SecurityCredentials::AcquireSsl(
            securitySettings.X509StoreLocation(),
            securitySettings.X509StoreName(),
            securitySettings.X509FindValue(),
            &credentials,
            nullptr);

        VERIFY_IS_TRUE(error.IsSuccess());

        VERIFY_IS_TRUE(credentials.size() == 5);

        Trace.WriteInfo(TraceType, "testCert2 should be loaded because its expiration is later than testCert1");
        VERIFY_IS_TRUE(
            credentials.front()->X509CertThumbprint() == *(testCert2.Thumbprint()) ||
            credentials.front()->X509CertThumbprint() == *(testCert2_3.Thumbprint()));

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509_LoadByTwoCommonNames3)
    {
        ENTER;

        auto saved = SecurityConfig::GetConfig().UseSecondaryIfNewer;
        SecurityConfig::GetConfig().UseSecondaryIfNewer = true;
        KFinally([=] { SecurityConfig::GetConfig().UseSecondaryIfNewer = saved; });

        auto cn1 = wformatString("name1-{0}", Guid::NewGuid());
        auto cn2 = wformatString("name2-{0}", Guid::NewGuid());
        InstallTestCertInScope testCert1(L"CN=" + cn1);
        InstallTestCertInScope testCert1_2(L"CN=" + cn1, nullptr, InstallTestCertInScope::DefaultCertExpiry() + TimeSpan::FromHours(24));
        InstallTestCertInScope testCert2(L"CN=" + cn2, nullptr, InstallTestCertInScope::DefaultCertExpiry() + TimeSpan::FromHours(72));
        InstallTestCertInScope testCert2_2(L"CN=" + cn2, nullptr, InstallTestCertInScope::DefaultCertExpiry() + TimeSpan::FromHours(48));
        InstallTestCertInScope testCert2_3(L"CN=" + cn2, nullptr, InstallTestCertInScope::DefaultCertExpiry() + TimeSpan::FromHours(70));

        auto securitySettings = TTestUtil::CreateX509Settings(cn1, cn2, L"DoesNotMatterForThisTest", L"");
        vector<SecurityCredentialsSPtr> svrCredentials;
        auto error = SecurityCredentials::AcquireSsl(
            securitySettings.X509StoreLocation(),
            securitySettings.X509StoreName(),
            securitySettings.X509FindValue(),
            nullptr,
            &svrCredentials);

        VERIFY_IS_TRUE(error.IsSuccess());

        VERIFY_IS_TRUE(svrCredentials.size() == 5);

        Trace.WriteInfo(TraceType, "testCert2 should be loaded because its expiration is later than testCert1");
        VERIFY_IS_TRUE(svrCredentials.front()->X509CertThumbprint() == *(testCert2.Thumbprint()));

        Trace.WriteInfo(TraceType, "reload should get the same result");
        vector<SecurityCredentialsSPtr> svrCredentials2;
        error = SecurityCredentials::AcquireSsl(
            securitySettings.X509StoreLocation(),
            securitySettings.X509StoreName(),
            securitySettings.X509FindValue(),
            nullptr,
            &svrCredentials2);

        VERIFY_IS_TRUE(SecurityCredentials::SortedVectorsEqual(svrCredentials, svrCredentials2));

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509_LoadByTwoCommonNames4)
    {
        ENTER;

        auto saved = SecurityConfig::GetConfig().UseSecondaryIfNewer;
        SecurityConfig::GetConfig().UseSecondaryIfNewer = true;
        KFinally([=] { SecurityConfig::GetConfig().UseSecondaryIfNewer = saved; });

        auto cn1 = wformatString("name1-{0}", Guid::NewGuid());
        auto cn2 = wformatString("name2-{0}", Guid::NewGuid());
        InstallTestCertInScope testCert1(L"CN=" + cn1);
        InstallTestCertInScope testCert1_2(L"CN=" + cn1, nullptr, InstallTestCertInScope::DefaultCertExpiry() + TimeSpan::FromHours(48));

        Trace.WriteInfo(TraceType, "findValue has both cn1 and cn2, but testCert2 is not installed");
        auto securitySettings = TTestUtil::CreateX509Settings(cn1, cn2, L"DoesNotMatterForThisTest", L"");
        vector<SecurityCredentialsSPtr> credentials, svrCredentials;
        auto error = SecurityCredentials::AcquireSsl(
            securitySettings.X509StoreLocation(),
            securitySettings.X509StoreName(),
            securitySettings.X509FindValue(),
            &credentials,
            &svrCredentials);

        VERIFY_IS_TRUE(error.IsSuccess());

        Trace.WriteInfo(TraceType, "credentials: {0}", SecurityCredentials::VectorsToString(credentials));
        VERIFY_IS_TRUE(credentials.size() == 2);
        Trace.WriteInfo(TraceType, "svrCredentials: {0}", SecurityCredentials::VectorsToString(svrCredentials));
        VERIFY_IS_TRUE(svrCredentials.size() == 2);

        Trace.WriteInfo(TraceType, "testCert1_2 should be loaded because its expiration is later than testCert1");
        VERIFY_IS_TRUE(credentials.front()->X509CertThumbprint() == *(testCert1_2.Thumbprint()));
        VERIFY_IS_TRUE(svrCredentials.front()->X509CertThumbprint() == *(testCert1_2.Thumbprint()));

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509_LoadByTwoCommonNames5)
    {
        ENTER;

        auto saved = SecurityConfig::GetConfig().UseSecondaryIfNewer;
        SecurityConfig::GetConfig().UseSecondaryIfNewer = true;
        KFinally([=] { SecurityConfig::GetConfig().UseSecondaryIfNewer = saved; });

        auto cn1 = wformatString("name1-{0}", Guid::NewGuid());
        auto cn2 = wformatString("name2-{0}", Guid::NewGuid());
        InstallTestCertInScope testCert2(L"CN=" + cn2, nullptr, InstallTestCertInScope::DefaultCertExpiry() + TimeSpan::FromHours(48));
        InstallTestCertInScope testCert2_2(L"CN=" + cn2, nullptr, InstallTestCertInScope::DefaultCertExpiry() + TimeSpan::FromHours(72));
        InstallTestCertInScope testCert2_3(L"CN=" + cn2, nullptr);

        Trace.WriteInfo(TraceType, "findValue has both cn1 and cn2, but testCert1 is not installed");
        auto securitySettings = TTestUtil::CreateX509Settings(cn1, cn2, L"DoesNotMatterForThisTest", L"");
        vector<SecurityCredentialsSPtr> credentials, svrCredentials;
        auto error = SecurityCredentials::AcquireSsl(
            securitySettings.X509StoreLocation(),
            securitySettings.X509StoreName(),
            securitySettings.X509FindValue(),
            &credentials,
            &svrCredentials);

        VERIFY_IS_TRUE(error.IsSuccess());

        VERIFY_IS_TRUE(credentials.size() == 3);
        VERIFY_IS_TRUE(svrCredentials.size() == 3);

        Trace.WriteInfo(TraceType, "testCert1_2 should be loaded because its expiration is later than testCert1");
        VERIFY_IS_TRUE(credentials.front()->X509CertThumbprint() == *(testCert2_2.Thumbprint()));
        VERIFY_IS_TRUE(svrCredentials.front()->X509CertThumbprint() == *(testCert2_2.Thumbprint()));

        Trace.WriteInfo(TraceType, "reload should get the same result");
        vector<SecurityCredentialsSPtr> credentials2, svrCredentials2;
        error = SecurityCredentials::AcquireSsl(
            securitySettings.X509StoreLocation(),
            securitySettings.X509StoreName(),
            securitySettings.X509FindValue(),
            &credentials2,
            &svrCredentials2);

        VERIFY_IS_TRUE(SecurityCredentials::SortedVectorsEqual(credentials, credentials2));
        VERIFY_IS_TRUE(SecurityCredentials::SortedVectorsEqual(svrCredentials, svrCredentials2));

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509_LoadByTwoThumbprints)
    {
        ENTER;

        auto saved = SecurityConfig::GetConfig().UseSecondaryIfNewer;
        SecurityConfig::GetConfig().UseSecondaryIfNewer = true;
        KFinally([=] { SecurityConfig::GetConfig().UseSecondaryIfNewer = saved; });

        InstallTestCertInScope testCert1(L"");
        InstallTestCertInScope testCert2(L"", nullptr, InstallTestCertInScope::DefaultCertExpiry() + TimeSpan::FromHours(48));

        auto securitySettings = TTestUtil::CreateX509Settings2(
            testCert1.Thumbprint()->PrimaryToString(),
            testCert2.Thumbprint()->PrimaryToString(),
            L"DoesNotMatterForThisTest",
            L"");

        vector<SecurityCredentialsSPtr> credentials, svrCredentials;
        auto error = SecurityCredentials::AcquireSsl(
            securitySettings.X509StoreLocation(),
            securitySettings.X509StoreName(),
            securitySettings.X509FindValue(),
            &credentials,
            &svrCredentials);

        VERIFY_IS_TRUE(error.IsSuccess());

        VERIFY_IS_TRUE(credentials.size() == 2);
        VERIFY_IS_TRUE(svrCredentials.size() == 2);

        Trace.WriteInfo(TraceType, "testCert1 should be loaded because its expiration is later than testCert1");
        VERIFY_IS_TRUE(credentials.front()->X509CertThumbprint() == *(testCert2.Thumbprint()));
        VERIFY_IS_TRUE(svrCredentials.front()->X509CertThumbprint() == *(testCert2.Thumbprint()));

        Trace.WriteInfo(TraceType, "reload should get the same result");
        vector<SecurityCredentialsSPtr> credentials2, svrCredentials2;
        error = SecurityCredentials::AcquireSsl(
            securitySettings.X509StoreLocation(),
            securitySettings.X509StoreName(),
            securitySettings.X509FindValue(),
            &credentials2,
            &svrCredentials2);

        VERIFY_IS_TRUE(SecurityCredentials::SortedVectorsEqual(credentials, credentials2));
        VERIFY_IS_TRUE(SecurityCredentials::SortedVectorsEqual(svrCredentials, svrCredentials2));

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509_LoadByTwoThumbprints2)
    {
        ENTER;

        auto saved = SecurityConfig::GetConfig().UseSecondaryIfNewer;
        SecurityConfig::GetConfig().UseSecondaryIfNewer = true;
        KFinally([=] { SecurityConfig::GetConfig().UseSecondaryIfNewer = saved; });

        InstallTestCertInScope testCert1(L"", nullptr, InstallTestCertInScope::DefaultCertExpiry() + TimeSpan::FromHours(48));
        InstallTestCertInScope testCert2(L"");

        auto securitySettings = TTestUtil::CreateX509Settings2(
            testCert1.Thumbprint()->PrimaryToString(),
            testCert2.Thumbprint()->PrimaryToString(),
            L"DoesNotMatterForThisTest",
            L"");

        vector<SecurityCredentialsSPtr> credentials;
        auto error = SecurityCredentials::AcquireSsl(
            securitySettings.X509StoreLocation(),
            securitySettings.X509StoreName(),
            securitySettings.X509FindValue(),
            &credentials,
            nullptr);

        VERIFY_IS_TRUE(error.IsSuccess());

        VERIFY_IS_TRUE(credentials.size() == 2);

        Trace.WriteInfo(TraceType, "testCert1 should be loaded because its expiration is later than testCert1");
        VERIFY_IS_TRUE(credentials.front()->X509CertThumbprint() == *(testCert1.Thumbprint()));

        Trace.WriteInfo(TraceType, "reload should get the same result");
        vector<SecurityCredentialsSPtr> credentials2;
        error = SecurityCredentials::AcquireSsl(
            securitySettings.X509StoreLocation(),
            securitySettings.X509StoreName(),
            securitySettings.X509FindValue(),
            &credentials2,
            nullptr);

        VERIFY_IS_TRUE(SecurityCredentials::SortedVectorsEqual(credentials, credentials2));

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509_LoadByTwoThumbprints3)
    {
        ENTER;

        auto saved = SecurityConfig::GetConfig().UseSecondaryIfNewer;
        SecurityConfig::GetConfig().UseSecondaryIfNewer = true;
        KFinally([=] { SecurityConfig::GetConfig().UseSecondaryIfNewer = saved; });

        InstallTestCertInScope testCert1(L"", nullptr, InstallTestCertInScope::DefaultCertExpiry() + TimeSpan::FromHours(48));
        InstallTestCertInScope testCert2(L"");

        auto securitySettings = TTestUtil::CreateX509Settings2(
            testCert1.Thumbprint()->PrimaryToString(),
            testCert2.Thumbprint()->PrimaryToString(),
            L"DoesNotMatterForThisTest",
            L"");

        vector<SecurityCredentialsSPtr> svrCredentials;
        auto error = SecurityCredentials::AcquireSsl(
            securitySettings.X509StoreLocation(),
            securitySettings.X509StoreName(),
            securitySettings.X509FindValue(),
            nullptr,
            &svrCredentials);

        VERIFY_IS_TRUE(error.IsSuccess());

        VERIFY_IS_TRUE(svrCredentials.size() == 2);

        Trace.WriteInfo(TraceType, "testCert1 should be loaded because its expiration is later than testCert1");
        VERIFY_IS_TRUE(svrCredentials.front()->X509CertThumbprint() == *(testCert1.Thumbprint()));

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509_LoadByTwoThumbprints4)
    {
        ENTER;

        auto saved = SecurityConfig::GetConfig().UseSecondaryIfNewer;
        SecurityConfig::GetConfig().UseSecondaryIfNewer = true;
        KFinally([=] { SecurityConfig::GetConfig().UseSecondaryIfNewer = saved; });

        InstallTestCertInScope testCert1(L"");

        auto securitySettings = TTestUtil::CreateX509Settings2(
            testCert1.Thumbprint()->PrimaryToString(),
            L"ffffffffffffffffffffffffffffffffffffffff", // secondary certificate load will fail
            L"DoesNotMatterForThisTest",
            L"");

        vector<SecurityCredentialsSPtr> credentials;
        auto error = SecurityCredentials::AcquireSsl(
            securitySettings.X509StoreLocation(),
            securitySettings.X509StoreName(),
            securitySettings.X509FindValue(),
            &credentials,
            nullptr);

        VERIFY_IS_TRUE(error.IsSuccess());

        VERIFY_IS_TRUE(credentials.size() == 1);

        Trace.WriteInfo(TraceType, "testCert1 should be loaded because secondary certificate is not installed");
        VERIFY_IS_TRUE(credentials.front()->X509CertThumbprint() == *(testCert1.Thumbprint()));

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509_LoadByTwoThumbprints5)
    {
        ENTER;

        auto saved = SecurityConfig::GetConfig().UseSecondaryIfNewer;
        SecurityConfig::GetConfig().UseSecondaryIfNewer = true;
        KFinally([=] { SecurityConfig::GetConfig().UseSecondaryIfNewer = saved; });

        InstallTestCertInScope testCert2(L"");

        auto securitySettings = TTestUtil::CreateX509Settings2(
            L"ffffffffffffffffffffffffffffffffffffffff", // primary certificate load will fail
            testCert2.Thumbprint()->PrimaryToString(),
            L"DoesNotMatterForThisTest",
            L"");

        vector<SecurityCredentialsSPtr> svrCredentials;
        auto error = SecurityCredentials::AcquireSsl(
            securitySettings.X509StoreLocation(),
            securitySettings.X509StoreName(),
            securitySettings.X509FindValue(),
            nullptr,
            &svrCredentials);

        VERIFY_IS_TRUE(error.IsSuccess());

        VERIFY_IS_TRUE(svrCredentials.size() == 1);

        Trace.WriteInfo(TraceType, "testCert2 should be loaded because primary certificate is not installed");
        VERIFY_IS_TRUE(svrCredentials.front()->X509CertThumbprint() == *(testCert2.Thumbprint()));

        LEAVE;
    }

    BOOST_AUTO_TEST_SUITE_END()

    struct TestMessageHeader : MessageHeader<MessageHeaderId::Example>, public Serialization::FabricSerializable
    {
        TestMessageHeader(size_t size)
        {
            data_.reserve(size);
            for (ULONG i = 0; i < size; ++ i)
            {
                data_.push_back((byte)i);
            }
        }

        FABRIC_FIELDS_01(data_);

    private:
        std::vector<byte> data_;
    };

    void SecureTransportTests::EncryptionTestX509(ULONG outgoingHeaderChunkSize)
    {
        ENTER;

        Trace.WriteInfo(TraceType, "outgoingHeaderChunkSize = {0}", outgoingHeaderChunkSize);
        MessageHeaders::Test_SetOutgoingChunkSize(outgoingHeaderChunkSize);
        KFinally([] { MessageHeaders::Test_SetOutgoingChunkSizeToDefault(); });

        auto receiver = DatagramTransportFactory::CreateTcp(TTestUtil::GetListenAddress());
        auto sender = DatagramTransportFactory::CreateTcpClient();

        InstallTestCertInScope testCert;
        SecuritySettings securitySettings = TTestUtil::CreateX509SettingsTp(
            testCert.Thumbprint()->PrimaryToString(),
            L"", 
            testCert.Thumbprint()->PrimaryToString(),
            L"");

        VERIFY_IS_TRUE(receiver->SetSecurity(securitySettings).IsSuccess());
        VERIFY_IS_TRUE(sender->SetSecurity(securitySettings).IsSuccess());

        ULONG headerSizes[] = {
            0,
            1,
            2,
            3,
            outgoingHeaderChunkSize / 3,
            outgoingHeaderChunkSize / 2,
            outgoingHeaderChunkSize - 1,
            outgoingHeaderChunkSize,
            outgoingHeaderChunkSize + 1,
            outgoingHeaderChunkSize + 2,
            (outgoingHeaderChunkSize < 1024) ? (outgoingHeaderChunkSize * 2) : 2048,
            (outgoingHeaderChunkSize < 1024) ? (outgoingHeaderChunkSize * 3) : 3071,
            (outgoingHeaderChunkSize < 1024) ? (outgoingHeaderChunkSize * 16) : 16*1024,
            (outgoingHeaderChunkSize < 1024) ? ((outgoingHeaderChunkSize + 1) * 23) : 23 * 1023,
            (outgoingHeaderChunkSize < 1024) ? (outgoingHeaderChunkSize * 32) : 32 * 1024
        };

        ULONG receiveChunkSize = (ULONG)TransportConfig::GetConfig().SslReceiveChunkSize;
        ULONG bodySizes[] = {
            0,
            1,
            2,
            3,
            receiveChunkSize / 3,
            receiveChunkSize / 2,
            receiveChunkSize - 1,
            receiveChunkSize,
            receiveChunkSize + 1,
            receiveChunkSize + 2,
            receiveChunkSize * 2 - 1,
            receiveChunkSize * 2,
            receiveChunkSize * 2 + receiveChunkSize / 2,
            receiveChunkSize * 3,
            receiveChunkSize * 3 + receiveChunkSize / 3
        };

        const ULONG messageExpected = sizeof(headerSizes) / sizeof(ULONG);
        ULONG messageReceived = 0;
        AutoResetEvent allMessageReceived;

        receiver->SetMessageHandler([&allMessageReceived, &receiver, messageExpected, &messageReceived](MessageUPtr & message, ISendTarget::SPtr const & st) -> void
        {
            Trace.WriteInfo(TraceType, "[receiver] got a message {0} from {1}", message->MessageId, st->Address());
            if (message->SerializedBodySize() > 0)
            {
                TestMessageBody payload;
                VERIFY_IS_TRUE(message->GetBody(payload));
                VERIFY_IS_TRUE(payload.Verify());
            }

            ++ messageReceived;
            if (messageReceived == messageExpected)
            {
                allMessageReceived.Set();
            }
        });

        VERIFY_IS_TRUE(receiver->Start().IsSuccess());
        VERIFY_IS_TRUE(sender->Start().IsSuccess());

        ISendTarget::SPtr target = sender->ResolveTarget(receiver->ListenAddress(), L"");
        VERIFY_IS_TRUE(target);

        for (ULONG i = 0; i < messageExpected; ++ i)
        {
            MessageUPtr message;
            if (bodySizes[i] == 0)
            {
                message = make_unique<Message>();
            }
            else
            {
                message = make_unique<Message>(TestMessageBody(bodySizes[i]));
            }

            if (headerSizes[i] > 0)
            {
                message->Headers.Add(MessageIdHeader());
                message->Headers.Add(TestMessageHeader(headerSizes[i]));
            }

            sender->SendOneWay(target, std::move(message));
        }

        VERIFY_IS_TRUE(allMessageReceived.WaitOne(TimeSpan::FromSeconds(60)));

        sender->Stop();
        receiver->Stop();

        LEAVE;
    }

    void SecureTransportTests::SignTestWithDeletedHeaders(size_t outgoingHeaderChunkSize, bool deleteHeader)
    {
        ENTER;

        Trace.WriteInfo(TraceType, "outgoingHeaderChunkSize = {0}", outgoingHeaderChunkSize);
        MessageHeaders::Test_SetOutgoingChunkSize(outgoingHeaderChunkSize);

        auto sender = DatagramTransportFactory::CreateTcpClient();
        auto receiver = DatagramTransportFactory::CreateTcp(TTestUtil::GetListenAddress());

        SecuritySettings securitySettings;
        securitySettings.Test_SetRawValues(SecurityProvider::Negotiate, ProtectionLevel::Sign);

        VERIFY_IS_TRUE(sender->SetSecurity(securitySettings).IsSuccess());
        VERIFY_IS_TRUE(receiver->SetSecurity(securitySettings).IsSuccess());

        AutoResetEvent replyReceived;
        sender->SetMessageHandler([&replyReceived](MessageUPtr & message, ISendTarget::SPtr const & st) -> void
        {
            Trace.WriteInfo(TraceType, "[sender] got reply {0} from {1}", message->MessageId, st->Address());
            replyReceived.Set();
        });

        AutoResetEvent messageReceived;
        wstring testAction = TTestUtil::GetGuidAction();
        TTestUtil::SetMessageHandler(
            receiver,
            testAction,
            [&messageReceived, &receiver, deleteHeader, outgoingHeaderChunkSize](MessageUPtr & message, ISendTarget::SPtr const & st) -> void
            {
                Trace.WriteInfo(TraceType, "[receiver] got a message {0} from {1}", message->MessageId, st->Address());
                messageReceived.Set();

                for (size_t i = 16; i < 48; ++i)
                {
                    message->Headers.Add(TestMessageHeader(i));
                }

                if (deleteHeader)
                {
                    // Delete some headers to verify deleted-header-skipping when generating signature
                    size_t i = 0;
                    for(auto header = message->Headers.Begin(); header != message->Headers.End(); ++i)
                    {
                        if (((outgoingHeaderChunkSize % 2) == 0) && ((i % 2) == 0))
                        {
                            message->Headers.Remove(header);
                        }
                        else if ((outgoingHeaderChunkSize % 3) == (i % 3))
                        {
                            message->Headers.Remove(header);
                        }
                        else
                        {
                            ++ header;
                        }
                    }
                }

                receiver->SendOneWay(st, std::move(message));
            });

        VERIFY_IS_TRUE(sender->Start().IsSuccess());
        VERIFY_IS_TRUE(receiver->Start().IsSuccess());

        wstring localListenerIdentity = TransportSecurity().LocalWindowsIdentity();

        ISendTarget::SPtr target = sender->ResolveTarget(receiver->ListenAddress(), L"", localListenerIdentity);
        VERIFY_IS_TRUE(target);

        auto message = make_unique<Message>();
        for (size_t i = 0; i < 32; ++i)
        {
            message->Headers.Add(TestMessageHeader(i));
        }

        if (deleteHeader)
        {
            // Delete some headers to verify deleted-header-skipping when generating signature
            size_t i = 0;
            for(auto header = message->Headers.Begin(); header != message->Headers.End(); ++i)
            {
                if (((outgoingHeaderChunkSize % 2) == 0) && ((i % 2) == 0))
                {
                    message->Headers.Remove(header);
                }
                else if ((outgoingHeaderChunkSize % 3) == (i % 3))
                {
                    message->Headers.Remove(header);
                }
                else
                {
                    ++ header;
                }
            }
        }

        message->Headers.Add(MessageIdHeader());
        message->Headers.Add(ActionHeader(testAction));

        sender->SendOneWay(target, std::move(message));

        bool receivedMessageOne = messageReceived.WaitOne(TimeSpan::FromSeconds(60));
        VERIFY_IS_TRUE(receivedMessageOne);

        bool receivedReplyOne = replyReceived.WaitOne(TimeSpan::FromSeconds(60));
        VERIFY_IS_TRUE(receivedReplyOne);

        sender->Stop();
        receiver->Stop();

        MessageHeaders::Test_SetOutgoingChunkSizeToDefault();

        LEAVE;
    }

    void SecureTransportTests::AdminRoleTest(SecuritySettings & securitySettings, bool enable, bool enableCorrectClientId)
    {
        auto client = DatagramTransportFactory::CreateTcpClient();
        VERIFY_IS_TRUE(client != nullptr);
        auto error = client->SetSecurity(securitySettings);
        VERIFY_IS_TRUE(error.IsSuccess());

        auto server = DatagramTransportFactory::CreateTcp(TTestUtil::GetListenAddress());
        VERIFY_IS_TRUE(server != nullptr);

        TransportSecurity::AssignAdminRoleToLocalUser = false;
        KFinally([&] { TransportSecurity::AssignAdminRoleToLocalUser = true; });

        auto securityProvider = securitySettings.SecurityProvider();
        if (securityProvider == SecurityProvider::Ssl)
        {
            if (enable)
            {
                if (enableCorrectClientId)
                {
                    static int counter = 0;
                    if (counter == 0)
                    {
                        ++counter;
                        Trace.WriteInfo(TraceType, "enable admin role with client cert thumbprint");
                        CertContextUPtr clientCert;
                        error = CryptoUtility::GetCertificate(
                            securitySettings.X509StoreLocation(),
                            securitySettings.X509StoreName(),
                            securitySettings.X509FindValue(),
                            clientCert);

                        VERIFY_IS_TRUE(error.IsSuccess());
                        Thumbprint clientCertThumbprint;
                        error = clientCertThumbprint.Initialize(clientCert.get());
                        VERIFY_IS_TRUE(error.IsSuccess());
                        securitySettings.EnableAdminRole(wformatString(clientCertThumbprint), SecurityConfig::X509NameMap(), L"");
                    }
                    else
                    {
                        Trace.WriteInfo(TraceType, "enable admin role with client cert commonName");
                        wstring certCommonName;
                        error = CryptoUtility::GetCertificateCommonName(
                            securitySettings.X509StoreLocation(),
                            securitySettings.X509StoreName(),
                            securitySettings.X509FindValue(),
                            certCommonName);

                        VERIFY_IS_TRUE(error.IsSuccess());

                        SecurityConfig::X509NameMap names;
                        names.Add(certCommonName);
                        securitySettings.EnableAdminRole(L"", names, L"");
                    }
                }
                else
                {
                    securitySettings.EnableAdminRole(L"", SecurityConfig::X509NameMap(), L"");
                }
            }
        }
        else
        {
            VERIFY_IS_TRUE(securityProvider == SecurityProvider::Kerberos);

            if (enable)
            {
                if (enableCorrectClientId)
                {
                    securitySettings.EnableAdminRole(TransportSecurity().LocalWindowsIdentity());
                }
                else
                {
                    securitySettings.EnableAdminRole(L"");
                }
            }
        }

        error = server->SetSecurity(securitySettings);
        VERIFY_IS_TRUE(error.IsSuccess());

        AutoResetEvent messageReceived;
        wstring testAction = TTestUtil::GetGuidAction();
        TTestUtil::SetMessageHandler(
            server,
            testAction,
            [securityProvider, enable, enableCorrectClientId, &messageReceived](MessageUPtr & message, ISendTarget::SPtr const & st) -> void
            {
                Trace.WriteInfo(TraceType, "server: got a message {0} from {1}", message->MessageId, st->Address());

                if (enable)
                {
                    if (enableCorrectClientId)
                    {
                        VERIFY_IS_TRUE(message->IsInRole(RoleMask::Admin));
                        VERIFY_IS_TRUE(message->Clone()->IsInRole(RoleMask::Admin));
                    }
                    else
                    {
                        VERIFY_IS_TRUE(message->IsInRole(RoleMask::User));
                        VERIFY_IS_TRUE(message->Clone()->IsInRole(RoleMask::User));
                    }
                }
                else
                {
                    // when disabled, RoleMask::All should have been assigned
                    VERIFY_IS_TRUE(message->IsInRole(RoleMask::All));
                    VERIFY_IS_TRUE(message->Clone()->IsInRole(RoleMask::All));
                    messageReceived.Set();
                    return;
                }

                messageReceived.Set();
            });

        error = server->Start();
        VERIFY_IS_TRUE(error.IsSuccess());

        error = client->Start();
        VERIFY_IS_TRUE(error.IsSuccess());

        auto message = make_unique<Message>();
        message->Headers.Add(ActionHeader(testAction));
        message->Headers.Add(MessageIdHeader());

        ISendTarget::SPtr target = client->ResolveTarget(server->ListenAddress(), L"", TransportSecurity().LocalWindowsIdentity());
        VERIFY_IS_TRUE(target);

        error = client->SendOneWay(target, move(message));
        VERIFY_IS_TRUE(error.IsSuccess());

        messageReceived.WaitOne(TimeSpan::FromSeconds(30));
    }

    void SecureTransportTests::NegotiationTimeoutTest(SecurityProvider::Enum securityProvider)
    {
        TimeSpan savedConnectionOpenTimeout = TransportConfig::GetConfig().ConnectionOpenTimeout;
        TransportConfig::GetConfig().ConnectionOpenTimeout = TimeSpan::FromSeconds(3);
        KFinally([savedConnectionOpenTimeout] { TransportConfig::GetConfig().ConnectionOpenTimeout = savedConnectionOpenTimeout; });

        const wstring listenerCn = L"NegotiationTimeoutTest";
        InstallTestCertInScope listenerCert(L"CN=" + listenerCn);

        SecuritySettings securitySettings;
        if (securityProvider == SecurityProvider::Ssl)
        {
            securitySettings = TTestUtil::CreateX509SettingsTp(
                listenerCert.Thumbprint()->PrimaryToString(),
                L"", 
                listenerCert.Thumbprint()->PrimaryToString(),
                L"");
        }
        else
        {
            VERIFY_IS_TRUE(securityProvider == SecurityProvider::Kerberos);
            securitySettings = TTestUtil::CreateKerbSettings();
        }

        auto listener = DatagramTransportFactory::CreateTcp(TTestUtil::GetListenAddress());

        auto error = listener->SetSecurity(securitySettings);
        VERIFY_IS_TRUE(error.IsSuccess());

        error = listener->Start();
        VERIFY_IS_TRUE(error.IsSuccess());

        Endpoint listenEndpoint;
        auto err = TcpTransportUtility::TryParseEndpointString(listener->ListenAddress(), listenEndpoint);
        VERIFY_ARE_EQUAL2(err.ReadValue(), ErrorCodeValue::Success);

        AutoResetEvent testCompleted(false);
        Threadpool::Post([&listenEndpoint, &testCompleted]
        {
#ifdef PLATFORM_UNIX
            auto sd = socket(AF_INET, SOCK_STREAM, 0);
            Invariant(sd >= 0);
            auto connected = connect(sd, listenEndpoint.AsSockAddr, listenEndpoint.AddressLength); 
            Invariant(!connected);

            char buf[1024]; 
            while(read(sd, buf, sizeof(buf)) > 0);
#else
            SOCKET socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, 0);
            VERIFY_IS_FALSE(socket == INVALID_SOCKET);

            Trace.WriteInfo(TraceType, "connecting");
            int connected = WSAConnect(socket, listenEndpoint.AsSockAddr, listenEndpoint.AddressLength, nullptr, nullptr, nullptr, nullptr);
            Trace.WriteInfo(TraceType, "connected: {0}", ::WSAGetLastError());
            VERIFY_IS_FALSE(connected == SOCKET_ERROR);

            char receiveBuffer[1];
            WSABUF wsaBuffer = {sizeof(receiveBuffer), receiveBuffer};
            ULONG bytesReceived;

            // In order to cause negotation timeout, do not send any data, just keep reading to detect connection break.
            for(;;)
            {
                DWORD flags = MSG_PARTIAL;
                Trace.WriteInfo(TraceType, "receiving");
                if (WSARecv(socket, &wsaBuffer, 1, &bytesReceived, &flags, nullptr, nullptr) != 0)
                {
                    Trace.WriteInfo(TraceType, "received failed: {0}", ::WSAGetLastError());
                    break;
                }

                Trace.WriteInfo(TraceType, "received {0} bytes", bytesReceived);
                if (bytesReceived == 0)
                {
                    Trace.WriteInfo(TraceType, "receive reached end of stream");
                    break;
                }
            }
#endif

            testCompleted.Set();
        });

        VERIFY_IS_TRUE(testCompleted.WaitOne(TransportConfig::GetConfig().ConnectionOpenTimeout + TimeSpan::FromSeconds(60)));
    }

    void SecureTransportTests::X509CertIssuerMatchTest(std::wstring const & issuers, bool shouldPass)
    {
        auto server = DatagramTransportFactory::CreateTcp(TTestUtil::GetListenAddress());
        SecuritySettings serverSecurity = TTestUtil::CreateX509Settings(
            L"CN=WinFabric-Test-SAN1-Alice",
            L"WinFabric-Test-SAN1-Bob",
            issuers);

        VERIFY_IS_TRUE(server->SetSecurity(serverSecurity).IsSuccess());

        AutoResetEvent messageReceived;
        wstring testAction = TTestUtil::GetListenAddress();
        TTestUtil::SetMessageHandler(
            server,
            testAction,
            [&messageReceived](MessageUPtr & message, ISendTarget::SPtr const & st) -> void
            {
                Trace.WriteInfo(TraceType, "[server] got a message {0} from {1}", message->MessageId, st->Address());
                messageReceived.Set();
            });

        VERIFY_IS_TRUE(server->Start().IsSuccess());

        auto client = DatagramTransportFactory::CreateTcpClient();
        SecuritySettings clientSecurity = TTestUtil::CreateX509Settings(
            L"CN=WinFabric-Test-SAN1-Bob",
            L"WinFabric-Test-SAN1-Alice",
            issuers);

        VERIFY_IS_TRUE(client->SetSecurity(clientSecurity).IsSuccess());
        VERIFY_IS_TRUE(client->Start().IsSuccess());


        auto msg(make_unique<Message>());
        msg->Headers.Add(ActionHeader(testAction));
        msg->Headers.Add(MessageIdHeader());

        ISendTarget::SPtr target = client->ResolveTarget(server->ListenAddress());
        VERIFY_IS_TRUE(target);

        client->SendOneWay(target, move(msg));
        if (shouldPass)
        {
            VERIFY_IS_TRUE(messageReceived.WaitOne(TimeSpan::FromSeconds(30)));
        }
        else
        {
            VERIFY_IS_FALSE(messageReceived.WaitOne(TimeSpan::FromSeconds(1)));
        }
    }

    void SecureTransportTests::BasicTestWin(
        SecurityProvider::Enum provider,
        std::wstring const & senderAddress,
        std::wstring const & receiverAddress,
        ProtectionLevel::Enum protectionLevel)
    {
        VERIFY_IS_TRUE((provider == SecurityProvider::Negotiate) || (provider == SecurityProvider::Kerberos));

        SecuritySettings securitySettings;
        securitySettings.Test_SetRawValues(provider, protectionLevel);

        SecurityTest(senderAddress, securitySettings, receiverAddress, securitySettings);
    }

    void SecureTransportTests::X509ManyMessage_SelfSigned(
        std::wstring const & senderAddress,
        std::wstring const & receiverAddress)
    {
        const wstring senderCn = L"sender.test.com";
        const wstring receiverCn = L"receiver.test.com";
        InstallTestCertInScope senderCert(L"CN=" + senderCn);
        InstallTestCertInScope receiverCert(L"CN=" + receiverCn);

        SecuritySettings senderSecSettings = TTestUtil::CreateX509SettingsTp(
            senderCert.Thumbprint()->PrimaryToString(),
            L"",
            receiverCert.Thumbprint()->PrimaryToString(),
            L"");

        SecuritySettings receiverSecSettings = TTestUtil::CreateX509SettingsTp(
            receiverCert.Thumbprint()->PrimaryToString(),
            L"",
            senderCert.Thumbprint()->PrimaryToString(),
            L"");

        ManyMessageTest(senderAddress, senderSecSettings, receiverAddress, receiverSecSettings);
    }

    void SecureTransportTests::X509ManySmallMessage_SelfSigned(
        std::wstring const & senderAddress,
        std::wstring const & receiverAddress)
    {
        const wstring senderCn = L"sender.test.com";
        const wstring receiverCn = L"receiver.test.com";
        InstallTestCertInScope senderCert(L"CN=" + senderCn);
        InstallTestCertInScope receiverCert(L"CN=" + receiverCn);

        SecuritySettings senderSecSettings = TTestUtil::CreateX509SettingsTp(
            senderCert.Thumbprint()->PrimaryToString(),
            L"",
            receiverCert.Thumbprint()->PrimaryToString(),
            L"");

        SecuritySettings receiverSecSettings = TTestUtil::CreateX509SettingsTp(
            receiverCert.Thumbprint()->PrimaryToString(),
            L"",
            senderCert.Thumbprint()->PrimaryToString(),
            L"");

        ManySmallMessageTest(senderAddress, senderSecSettings, receiverAddress, receiverSecSettings);
    }

    void SecureTransportTests::X509Test_SelfSigned(
        std::wstring const & senderAddress,
        std::wstring const & receiverAddress)
    {
        const wstring senderCn = L"sender.test.com";
        const wstring receiverCn = L"receiver.test.com";
        InstallTestCertInScope senderCert(L"CN=" + senderCn);
        InstallTestCertInScope receiverCert(L"CN=" + receiverCn);

        SecuritySettings senderSecSettings = TTestUtil::CreateX509SettingsTp(
            senderCert.Thumbprint()->PrimaryToString(),
            L"", 
            receiverCert.Thumbprint()->PrimaryToString(),
            L"");

        SecuritySettings receiverSecSettings = TTestUtil::CreateX509SettingsTp(
            receiverCert.Thumbprint()->PrimaryToString(),
            L"",
            senderCert.Thumbprint()->PrimaryToString(),
            L"");

        SecurityTest(senderAddress, senderSecSettings, receiverAddress, receiverSecSettings);
    }

    void SecureTransportTests::SecurityTest(
        std::wstring const & senderAddress,
        SecuritySettings const & senderSecSettings,
        std::wstring const & receiverAddress,
        SecuritySettings const & receiverSecSettings,
        bool failureExpected)
    {
        auto sender = DatagramTransportFactory::CreateTcp(senderAddress);
        auto receiver = DatagramTransportFactory::CreateTcp(receiverAddress);

        VERIFY_IS_TRUE(sender->SetSecurity(senderSecSettings).IsSuccess());
        VERIFY_IS_TRUE(receiver->SetSecurity(receiverSecSettings).IsSuccess());

        AutoResetEvent replyReceived;

        std::wstring requestAction = L"action... oh yeah!";

        // Make sure message body span more than one chunk on the incoming side
        std::wstring requestBodyString;
        size_t targetMessageBodySize = 2 * TransportConfig::GetConfig().SslReceiveChunkSize + 1;
        while((requestBodyString.size() * sizeof(WCHAR)) < targetMessageBodySize)
        {
            requestBodyString += L"Message ";
        }
        requestBodyString += L"oh yeah";

        TcpTestMessage requestBody(requestBodyString);
        MessageIdHeader requestIdHeader;

        std::wstring replyAction = L"reply action... oh yeah!";
        TcpTestMessage replyBody(L"Reply message message message message oh yeah");
        MessageIdHeader replyIdHeader;

        TTestUtil::SetMessageHandler(
            sender,
            replyAction,
            [&replyReceived, replyAction, replyBody, &replyIdHeader](MessageUPtr & message, ISendTarget::SPtr const &) -> void
            {
                Trace.WriteInfo(TraceType, "[sender] got a message");

                VERIFY_IS_TRUE(replyAction == message->Action);

                TcpTestMessage body;
                VERIFY_IS_TRUE(message->GetBody(body));

                VERIFY_IS_TRUE(replyAction == message->Action);
                VERIFY_IS_TRUE(replyBody.message_ == body.message_);
                VERIFY_IS_TRUE(replyIdHeader.MessageId == message->MessageId);

                replyReceived.Set();
            });

        VERIFY_IS_TRUE(sender->Start().IsSuccess());

        AutoResetEvent messageReceived;
        TTestUtil::SetMessageHandler(
            receiver,
            requestAction,
            [receiver, sender, &messageReceived, requestAction, requestBody, replyAction, replyBody, &requestIdHeader, &replyIdHeader](MessageUPtr & message, ISendTarget::SPtr const & st) -> void
            {
                Trace.WriteInfo(TraceType, "[receiver] got a message {0} {1}", sender->ListenAddress(), st->Address());

                VERIFY_IS_TRUE(sender->ListenAddress() == st->Address());

                TcpTestMessage body;
                VERIFY_IS_TRUE(message->GetBody(body));

                VERIFY_IS_TRUE(requestAction == message->Action);
                VERIFY_IS_TRUE(requestBody.message_ == body.message_);
                VERIFY_IS_TRUE(requestIdHeader.MessageId == message->MessageId);

                messageReceived.Set();

                auto reply = make_unique<Message>(replyBody);
                reply->Headers.Add(ActionHeader(replyAction));
                reply->Headers.Add(replyIdHeader);

                // ping back
                receiver->SendOneWay(st, std::move(reply));

                sender->Stop();
                receiver->Stop();
            });

        VERIFY_IS_TRUE(receiver->Start().IsSuccess());

        wstring localListenerIdentity = TransportSecurity().LocalWindowsIdentity();
        ISendTarget::SPtr target = sender->ResolveTarget(receiver->ListenAddress(), L"", localListenerIdentity);
        VERIFY_IS_TRUE(target);

        auto oneway = make_unique<Message>(requestBody);
        oneway->Headers.Add(ActionHeader(requestAction));
        oneway->Headers.Add(requestIdHeader);

        Trace.WriteInfo(TraceType, "sending message ...");
        sender->SendOneWay(target, std::move(oneway));

        auto waitTime = failureExpected ? TimeSpan::FromSeconds(1) : TimeSpan::FromSeconds(30);
        Trace.WriteInfo(TraceType, "wait for receiver to get the message");
        bool receivedMessageOne = messageReceived.WaitOne(waitTime);
        VERIFY_ARE_EQUAL2(receivedMessageOne, !failureExpected);

        Trace.WriteInfo(TraceType, "wait for sender to get a reply");
        bool receivedReplyOne = replyReceived.WaitOne(waitTime);
        VERIFY_ARE_EQUAL2(receivedReplyOne, !failureExpected);

        sender->Stop();
        receiver->Stop();
    }

    void SecureTransportTests::ManyMessageTest(
        std::wstring const & senderAddress,
        SecuritySettings const & senderSecSettings,
        std::wstring const & receiverAddress,
        SecuritySettings const & receiverSecSettings)
    {
        auto saved = SecurityConfig::GetConfig().WindowsSecurityMaxEncryptSize;
        SecurityConfig::GetConfig().WindowsSecurityMaxEncryptSize = TransportConfig::GetConfig().SslReceiveChunkSize;
        KFinally([=] { SecurityConfig::GetConfig().WindowsSecurityMaxEncryptSize = saved; });

        auto sender = DatagramTransportFactory::CreateTcp(senderAddress);
        auto receiver = DatagramTransportFactory::CreateTcp(receiverAddress);

        VERIFY_IS_TRUE(sender->SetSecurity(senderSecSettings).IsSuccess());
        VERIFY_IS_TRUE(receiver->SetSecurity(receiverSecSettings).IsSuccess());

        AutoResetEvent replyReceived;
        auto action = TTestUtil::GetGuidAction();
        static const int replyBodySize = 0;

        TTestUtil::SetMessageHandler(
            sender,
            action,
            [&replyReceived, action](MessageUPtr & message, ISendTarget::SPtr const &) -> void
            {
                Trace.WriteInfo(TraceType, "[sender] got a message");

                VERIFY_IS_TRUE(action == message->Action);

                TestMessageBody replyBody;
                VERIFY_IS_TRUE(message->GetBody(replyBody));

                VERIFY_IS_TRUE(replyBody.Verify());
                Trace.WriteInfo(TraceType, "replyBody.size() = {0}, replyBodySize = {1}", replyBody.size(), replyBodySize);
                VERIFY_ARE_EQUAL2(replyBody.size(), replyBodySize);

                replyReceived.Set();
            });

        VERIFY_IS_TRUE(sender->Start().IsSuccess());

        auto wsme = SecurityConfig::GetConfig().WindowsSecurityMaxEncryptSize;
        auto bb0 = make_shared<ByteBuffer>(1024);
        auto bb1 = make_shared<ByteBuffer>(wsme + 1024);
        auto bb2 = make_shared<ByteBuffer>(wsme * 3 + 1024);
        auto bb3 = make_shared<ByteBuffer>(wsme * 5 + 1024);
        auto bb4 = make_shared<ByteBuffer>(wsme * 7 + 1024);
        auto bb5 = make_shared<ByteBuffer>(wsme * 8);

        vector<const_buffer> bufferList = {
            const_buffer(bb0->data(), bb0->size()),
            const_buffer(bb1->data(), bb1->size()),
            const_buffer(bb2->data(), bb2->size()),
            const_buffer(bb3->data(), bb3->size()),
            const_buffer(bb4->data(), bb4->size()),
            const_buffer(bb5->data(), bb5->size()) };

        auto msgWithLargeBufferChunk = make_unique<Message>(
            bufferList,
            [bb0, bb1, bb2, bb3, bb4, bb5](std::vector<Common::const_buffer> const &, void*) {},
            nullptr);

        msgWithLargeBufferChunk->Headers.Add(ActionHeader(action));
        msgWithLargeBufferChunk->Headers.Add(MessageIdHeader());

        const uint largeMessageSize = msgWithLargeBufferChunk->SerializedSize();

        static const int requestTotal = 37;
        static const uint maxRequestSize = TransportConfig::GetConfig().SslReceiveChunkSize * 5;

        atomic_uint64 receiveCount(0);
        AutoResetEvent messageReceived;
        TTestUtil::SetMessageHandler(
            receiver,
            action,
            [receiver, sender, &receiveCount, &messageReceived, action, largeMessageSize](MessageUPtr & message, ISendTarget::SPtr const & st) -> void
            {
                Trace.WriteInfo(TraceType, "[receiver] got a message {0} {1}", sender->ListenAddress(), st->Address());

                VERIFY_IS_TRUE(sender->ListenAddress() == st->Address());
                VERIFY_IS_TRUE(action == message->Action);

                auto seq = ++receiveCount;
                if (seq <= requestTotal)
                {
                    TestMessageBody body;
                    VERIFY_IS_TRUE(message->GetBody(body));

                    auto requestBodySize = maxRequestSize * seq / requestTotal + seq;
                    Trace.WriteInfo(TraceType, "body.size() = {0}, requestBodySize = {1}", body.size(), requestBodySize);
                    VERIFY_ARE_EQUAL2(body.size(), requestBodySize);
                    VERIFY_IS_TRUE(body.Verify());
                }
                else
                {
                    auto incomingSize = message->SerializedSize();
                    auto expectedSize = largeMessageSize;
                    auto incomingBodySize = message->SerializedBodySize();
                    auto incomingHeaderSize = message->SerializedHeaderSize();
                    Trace.WriteInfo(
                        TraceType,
                        "incomingSize = {0}, expectedSize = {1}, incomingBodySize = {2}, incomingHeaderSize = {3}",
                        incomingSize, expectedSize, incomingBodySize, incomingHeaderSize);

                    VERIFY_IS_TRUE(incomingSize == expectedSize);
                    messageReceived.Set();

                    auto reply = make_unique<Message>(TestMessageBody(replyBodySize));
                    reply->Headers.Add(ActionHeader(action));
                    reply->Headers.Add(MessageIdHeader());

                    // ping back
                    receiver->SendOneWay(st, std::move(reply));
                }
            });

        VERIFY_IS_TRUE(receiver->Start().IsSuccess());

        wstring localListenerIdentity = TransportSecurity().LocalWindowsIdentity();
        ISendTarget::SPtr target = sender->ResolveTarget(receiver->ListenAddress(), L"", localListenerIdentity);
        VERIFY_IS_TRUE(target);

        // Make sure message body span more than one chunk on the incoming side
        for (int i = 1; i <= requestTotal; ++i)
        {
            TestMessageBody requestBody(maxRequestSize * i / requestTotal + i);

            auto oneway = make_unique<Message>(requestBody);
            oneway->Headers.Add(ActionHeader(action));
            oneway->Headers.Add(MessageIdHeader());

            Trace.WriteInfo(TraceType, "sending message {0} ...");
            sender->SendOneWay(target, std::move(oneway));
            this_thread::yield();
        }

        // send message with large chunks to test that SecurityContextWin properly breaks up large buffer chunks to fit when framing protection is enabled
        Trace.WriteInfo(TraceType, "sending msgWithLargeBufferChunk");
        sender->SendOneWay(target, std::move(msgWithLargeBufferChunk));

        auto waitTime = TimeSpan::FromSeconds(60);
        Trace.WriteInfo(TraceType, "wait for receiver to get the message");
        bool receivedMessageOne = messageReceived.WaitOne(waitTime);
        VERIFY_ARE_EQUAL2(receivedMessageOne, true);

        Trace.WriteInfo(TraceType, "wait for sender to get a reply");
        bool receivedReplyOne = replyReceived.WaitOne(waitTime);
        VERIFY_ARE_EQUAL2(receivedReplyOne, true);

        sender->Stop();
        receiver->Stop();
    }

    void SecureTransportTests::ManySmallMessageTest(
        std::wstring const & senderAddress,
        SecuritySettings const & senderSecSettings,
        std::wstring const & receiverAddress,
        SecuritySettings const & receiverSecSettings)
    {
        auto node1 = DatagramTransportFactory::CreateTcp(senderAddress);
        auto node2 = DatagramTransportFactory::CreateTcp(receiverAddress);

        VERIFY_IS_TRUE(node1->SetSecurity(senderSecSettings).IsSuccess());
        VERIFY_IS_TRUE(node2->SetSecurity(receiverSecSettings).IsSuccess());

        AutoResetEvent node1ReceivedAll(false);
        auto action = TTestUtil::GetGuidAction();
        atomic_uint64 node1ReceiveCount(0);
        static const int messageToSend = 3000;

        TTestUtil::SetMessageHandler(
            node1,
            action,
            [&node1ReceivedAll, &node1ReceiveCount, action](MessageUPtr & message, ISendTarget::SPtr const &) -> void
            {
                Trace.WriteInfo(TraceType, "[node1] got a message");
                VERIFY_IS_TRUE(action == message->Action);
                auto count = ++node1ReceiveCount;
                if (count == messageToSend)
                {
                    node1ReceivedAll.Set();
                }
            });

        VERIFY_IS_TRUE(node1->Start().IsSuccess());

        static const int requestBodySize = 128;
        static const int maxMessageSize = TransportConfig::GetConfig().SslReceiveChunkSize * 5;
        atomic_uint64 node2ReceiveCount(0);
        AutoResetEvent node2ReceivedAll;
        TTestUtil::SetMessageHandler(
            node2,
            action,
            [node2, node1, &node2ReceiveCount, &node2ReceivedAll, action](MessageUPtr & message, ISendTarget::SPtr const & st) -> void
            {
                Trace.WriteInfo(TraceType, "[node2] got a message {0} {1}", node1->ListenAddress(), st->Address());

                VERIFY_IS_TRUE(node1->ListenAddress() == st->Address());

                TestMessageBody body;
                VERIFY_IS_TRUE(message->GetBody(body));

                VERIFY_IS_TRUE(action == message->Action);
                Trace.WriteInfo(TraceType, "body.size() = {0}, requestBodySize = {1}", body.size(), requestBodySize);
                VERIFY_ARE_EQUAL2(body.size(), requestBodySize);
                VERIFY_IS_TRUE(body.Verify());

                if (++node2ReceiveCount == messageToSend)
                {
                    node2ReceivedAll.Set();
                }
            });

        VERIFY_IS_TRUE(node2->Start().IsSuccess());

        wstring localListenerIdentity = TransportSecurity().LocalWindowsIdentity();

        Threadpool::Post([&]
        {
            ISendTarget::SPtr target = node1->ResolveTarget(node2->ListenAddress(), L"", localListenerIdentity);
            VERIFY_IS_TRUE(target);

            for (int i = 1; i <= messageToSend; ++i)
            {
                TestMessageBody requestBody(requestBodySize);

                auto oneway = make_unique<Message>(requestBody);
                oneway->Headers.Add(ActionHeader(action));
                oneway->Headers.Add(MessageIdHeader());

                Trace.WriteInfo(TraceType, "node1 sending message {0} ...");
                node1->SendOneWay(target, std::move(oneway));
            }
        });

        Sleep(3000); // try to avoid duplicate connections between node1 and node2

        Threadpool::Post([&]
        {
            ISendTarget::SPtr target = node2->ResolveTarget(node1->ListenAddress(), L"", localListenerIdentity);
            VERIFY_IS_TRUE(target);

            for (int i = 1; i <= messageToSend; ++i)
            {
                auto oneway = make_unique<Message>();
                oneway->Headers.Add(ActionHeader(action));
                oneway->Headers.Add(MessageIdHeader());

                Trace.WriteInfo(TraceType, "node2 sending message {0} ...");
                node2->SendOneWay(target, std::move(oneway));
            }
        });

        auto waitTime = TimeSpan::FromSeconds(60);
        Trace.WriteInfo(TraceType, "wait for node2 to get all messages");
        VERIFY_IS_TRUE(node2ReceivedAll.WaitOne(waitTime));
        Trace.WriteInfo(TraceType, "node2 received {0} messages", node2ReceiveCount.load());

        Trace.WriteInfo(TraceType, "wait for node1 to get all messages");
        VERIFY_IS_TRUE(node1ReceivedAll.WaitOne(waitTime));
        Trace.WriteInfo(TraceType, "node1 received {0} messages", node1ReceiveCount.load());

        node1->Stop();
        node2->Stop();
    }

#ifndef PLATFORM_UNIX

    void SecureTransportTests::FramingProtectionEnabled_Negotiate_ByConfig(ProtectionLevel::Enum protectionLevel)
    {
        SecurityConfig & securityConfig = SecurityConfig::GetConfig();
        auto saved = securityConfig.FramingProtectionEnabled;
        securityConfig.FramingProtectionEnabled = true;
        KFinally([&]
        {
            securityConfig.FramingProtectionEnabled = saved;
        });

        BasicTestWin(
            SecurityProvider::Negotiate,
            L"127.0.0.1:0",
            L"127.0.0.1:0",
            protectionLevel);
    }

#endif

    void SecureTransportTests::ClaimAuthTestClientSend(
        wstring const & receiverAddress,
        wstring const & clientClaims,
        bool expectFailure,
        bool clientIsAdmin,
        std::wstring const clientCertFindValue,
        Common::SecurityConfig::IssuerStoreKeyValueMap clientIssuerStore)
    {
        Trace.WriteInfo(TraceType, "ClaimAuthTestClientSend({0})", clientClaims);

        SecuritySettings clientSecurity;
        if (!clientClaims.empty())
        {
            auto error = SecuritySettings::CreateClaimTokenClient(clientClaims, L"", L"WinFabric-Test-SAN1-Alice", L"", wformatString(ProtectionLevel::EncryptAndSign), clientSecurity);
            VERIFY_IS_TRUE(error.IsSuccess());
        }
        else
        {
            clientSecurity = TTestUtil::CreateX509Settings(
                clientCertFindValue,
                L"WinFabric-Test-SAN1-Alice",
                clientIssuerStore);
        }

        Trace.WriteInfo(TraceType, "client security settings = {0}, expectFailure = {1}, cilentIsAdmin = {2}", clientSecurity, expectFailure, clientIsAdmin);

        auto sender = DatagramTransportFactory::CreateTcpClient();
        VERIFY_IS_TRUE(sender->SetSecurity(clientSecurity).IsSuccess());

        auto request(make_unique<Message>());
        request->Headers.Add(MessageIdHeader());

        AutoResetEvent replyReceived;
        MessageId requestId = request->MessageId;
        sender->SetMessageHandler([&replyReceived, &requestId, clientIsAdmin](MessageUPtr & reply, ISendTarget::SPtr const & st) -> void
        {
            Trace.WriteInfo(TraceType, "[sender] got reply {0} from {1}", reply->TraceId(), st->Address());
            VERIFY_IS_TRUE(reply->RelatesTo == requestId);
            ClientIsAdmin clientIsAdminFromServer = false;
            VERIFY_IS_TRUE(reply->GetBody(clientIsAdminFromServer));
            VERIFY_IS_TRUE(clientIsAdminFromServer.IsAdmin == clientIsAdmin);
            replyReceived.Set();
        });

        VERIFY_IS_TRUE(sender->Start().IsSuccess());

        ISendTarget::SPtr target = sender->ResolveTarget(receiverAddress);
        VERIFY_IS_TRUE(target);

        sender->SendOneWay(target, move(request));
        if (expectFailure)
        {
            VERIFY_IS_FALSE(replyReceived.WaitOne(TimeSpan::FromSeconds(1)));
        }
        else
        {
            VERIFY_IS_TRUE(replyReceived.WaitOne(TimeSpan::FromSeconds(30)));
        }
    }

    void SecureTransportTests::ClaimAuthTestClientSend2(
        wstring const & receiverAddress,
        wstring const & svrCertThumbprint,
        wstring const & clientClaims,
        bool expectFailure,
        bool clientIsAdmin,
        std::wstring const clientCertFindValue)
    {
        Trace.WriteInfo(TraceType, "ClaimAuthTestClientSend({0})", clientClaims);

        SecuritySettings clientSecurity;
        if (!clientClaims.empty())
        {
            auto error = SecuritySettings::CreateClaimTokenClient(clientClaims, svrCertThumbprint, L"", L"", wformatString(ProtectionLevel::EncryptAndSign), clientSecurity);
            VERIFY_IS_TRUE(error.IsSuccess());
        }
        else
        {
            clientSecurity = TTestUtil::CreateX509SettingsTp(
                clientCertFindValue,
                L"",
                svrCertThumbprint,
                L"");
        }

        Trace.WriteInfo(TraceType, "client security settings = {0}, expectFailure = {1}, cilentIsAdmin = {2}", clientSecurity, expectFailure, clientIsAdmin);

        auto sender = DatagramTransportFactory::CreateTcpClient();
        VERIFY_IS_TRUE(sender->SetSecurity(clientSecurity).IsSuccess());

        auto request(make_unique<Message>());
        request->Headers.Add(MessageIdHeader());

        AutoResetEvent replyReceived;
        MessageId requestId = request->MessageId;
        sender->SetMessageHandler([&replyReceived, &requestId, clientIsAdmin](MessageUPtr & reply, ISendTarget::SPtr const & st) -> void
        {
            Trace.WriteInfo(TraceType, "[sender] got reply {0} from {1}", reply->TraceId(), st->Address());
            VERIFY_IS_TRUE(reply->RelatesTo == requestId);
            ClientIsAdmin clientIsAdminFromServer = false;
            VERIFY_IS_TRUE(reply->GetBody(clientIsAdminFromServer));
            VERIFY_IS_TRUE(clientIsAdminFromServer.IsAdmin == clientIsAdmin);
            replyReceived.Set();
        });

        VERIFY_IS_TRUE(sender->Start().IsSuccess());

        ISendTarget::SPtr target = sender->ResolveTarget(receiverAddress);
        VERIFY_IS_TRUE(target);

        sender->SendOneWay(target, move(request));
        if (expectFailure)
        {
            VERIFY_IS_FALSE(replyReceived.WaitOne(TimeSpan::FromSeconds(1)));
        }
        else
        {
            VERIFY_IS_TRUE(replyReceived.WaitOne(TimeSpan::FromSeconds(30)));
        }
    }

    namespace X509Compatibility
    {
        namespace
        {
            const auto certToLoad = L"*.eastus.cloudapp.azure.com";
            const auto server = L"server.eastus.cloudapp.azure.com";
            const auto client = L"client.eastus.cloudapp.azure.com";
            const auto svrAddressEnv = L"X509CompatibilitySvrAddress";

            const auto msgAction = L"X509CompatibilityTest";
            constexpr int replyBodySize = 0;
            constexpr int maxMessageSizeFactor = 5;
            constexpr int requestTotal = 37;

            void StartClient()
            {
                SecurityConfig::X509NameMap serverMap;
                serverMap.Add(server);

                SecuritySettings senderSecSettings = TTestUtil::CreateX509Settings(
                    certToLoad,
                    serverMap);

                auto svrAddress = Environment::GetEnvironmentVariable(svrAddressEnv);

                auto sender = DatagramTransportFactory::CreateTcpClient();

                VERIFY_IS_TRUE(sender->SetSecurity(senderSecSettings).IsSuccess());

                AutoResetEvent replyReceived;

                TTestUtil::SetMessageHandler(
                    sender,
                    msgAction,
                    [&replyReceived](MessageUPtr & message, ISendTarget::SPtr const &) -> void
                {
                    Trace.WriteInfo(TraceType, "[sender] got a message");

                    TestMessageBody replyBody;
                    VERIFY_IS_TRUE(message->GetBody(replyBody));

                    VERIFY_IS_TRUE(msgAction == message->Action);
                    VERIFY_IS_TRUE(replyBody.Verify());
                    Trace.WriteInfo(TraceType, "replyBody.size() = {0}, replyBodySize = {1}", replyBody.size(), replyBodySize);
                    VERIFY_ARE_EQUAL2(replyBody.size(), replyBodySize);

                    replyReceived.Set();
                });

                VERIFY_IS_TRUE(sender->Start().IsSuccess());

                ISendTarget::SPtr target = sender->ResolveTarget(svrAddress);
                VERIFY_IS_TRUE(target);

                // Make sure message body span more than one chunk on the incoming side
                const int maxMessageSize = TransportConfig::GetConfig().SslReceiveChunkSize * maxMessageSizeFactor;
                for (int i = 1; i <= requestTotal; ++i)
                {
                    TestMessageBody requestBody(maxMessageSize * i / requestTotal + i);

                    auto oneway = make_unique<Message>(requestBody);
                    oneway->Headers.Add(ActionHeader(msgAction));
                    oneway->Headers.Add(MessageIdHeader());

                    Trace.WriteInfo(TraceType, "sending message {0} ...");
                    sender->SendOneWay(target, std::move(oneway));
                    this_thread::yield();
                }

                auto waitTime = TimeSpan::FromSeconds(60);
                Trace.WriteInfo(TraceType, "wait for a reply");
                bool receivedReplyOne = replyReceived.WaitOne(waitTime);
                VERIFY_ARE_EQUAL2(receivedReplyOne, true);

                sender->Stop();
            }

            void StartServer()
            {
                SecurityConfig::X509NameMap clientMap;
                clientMap.Add(client);

                SecuritySettings receiverSecSettings = TTestUtil::CreateX509Settings(
                    certToLoad,
                    clientMap);

                Endpoint svrEndpoint;
                auto err = TcpTransportUtility::GetFirstLocalAddress(svrEndpoint, ResolveOptions::IPv4);
                VERIFY_IS_TRUE(err.IsSuccess());

                auto receiver = DatagramTransportFactory::CreateTcp(svrEndpoint.ToString());

                VERIFY_IS_TRUE(receiver->SetSecurity(receiverSecSettings).IsSuccess());

                atomic_uint64 receiveCount(0);
                AutoResetEvent allRequestsReceived;
                static const int maxMessageSize = TransportConfig::GetConfig().SslReceiveChunkSize * maxMessageSizeFactor;
                TTestUtil::SetMessageHandler(
                    receiver,
                    msgAction,
                    [receiver, &receiveCount, &allRequestsReceived](MessageUPtr & message, ISendTarget::SPtr const & st) -> void
                {
                    Trace.WriteInfo(TraceType, "[receiver] got a message {0}", st->Address());

                    TestMessageBody body;
                    VERIFY_IS_TRUE(message->GetBody(body));

                    VERIFY_IS_TRUE(msgAction == message->Action);
                    auto seq = receiveCount.load() + 1;
                    auto requestBodySize = maxMessageSize * seq / requestTotal + seq;
                    Trace.WriteInfo(TraceType, "body.size() = {0}, requestBodySize = {1}", body.size(), requestBodySize);
                    VERIFY_ARE_EQUAL2(body.size(), requestBodySize);
                    VERIFY_IS_TRUE(body.Verify());

                    if (++receiveCount == requestTotal)
                    {
                        auto reply = make_unique<Message>(TestMessageBody(replyBodySize));
                        reply->Headers.Add(ActionHeader(msgAction));
                        reply->Headers.Add(MessageIdHeader());

                        // ping back
                        receiver->SendOneWay(st, std::move(reply));

                        allRequestsReceived.Set();
                    }
                });

                VERIFY_IS_TRUE(receiver->Start().IsSuccess());

                cout << "server started listening on " << StringUtility::Utf16ToUtf8(receiver->ListenAddress()) << endl;

#ifdef PLATFORM_UNIX
                cout << "run the following command to start client on Windows:" << endl;
                cout << StringUtility::Utf16ToUtf8(wformatString("    set {0}={1} & Transport.Test.exe --run_test=X509Compat/Client", svrAddressEnv, receiver->ListenAddress())) << endl;
                cout << StringUtility::Utf16ToUtf8(wformatString("    set {0}={1} & Transport.Test.exe --run_test=X509Compat/Client2", svrAddressEnv, receiver->ListenAddress())) << endl;
#else
                cout << "run the following command to start client on Linux:" << endl;
                cout << StringUtility::Utf16ToUtf8(wformatString("    {0}={1} ./Transport.Test.exe --run_test=X509Compat/Client", svrAddressEnv, receiver->ListenAddress())) << endl;
#endif

                auto waitTime = TimeSpan::FromSeconds(300);
                Trace.WriteInfo(TraceType, "wait for requests");
                bool receivedMessageOne = allRequestsReceived.WaitOne(waitTime);
                VERIFY_ARE_EQUAL2(receivedMessageOne, true);

                Sleep(3000); // ensure reply is sent back

                receiver->Stop();
            }
        }
    }

    // X509Compat tests x509 security compatibility between Linux and Windows
    // To run the tests:
    // 1. copy \\sylvia\share\tools\test.with.ca.signed.certs.tgz to your Linux machine
    // 2. tar xfzv test.with.ca.signed.certs.tgz
    // 3. cd test.with.ca.signed.certs
    // 4. sudo ./install.certs.sh
    // 5. run Server* test case on Windows/Linux and copy X509CompatibilityTest environment variable value from console output
    // 6. on a different platform, set X509CompatibilityTest to value copied in the previous step and run Client* test case
    // NOTE: *FramingProtectionEnabled cases are only available on Windows, as framing protection is always enabled on Linux
    BOOST_FIXTURE_TEST_SUITE(X509Compat, SecureTransportTests)

    BOOST_AUTO_TEST_CASE(Client, *boost::unit_test::disabled())
    {
        ENTER;

        X509Compatibility::StartClient();

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(Server, *boost::unit_test::disabled())
    {
        ENTER;

        X509Compatibility::StartServer();

        LEAVE;
    }

#ifndef PLATFORM_UNIX

    BOOST_AUTO_TEST_CASE(Server2, *boost::unit_test::disabled()) //FramingProtectionEnabled
    {
        ENTER;

        SecurityConfig & securityConfig = SecurityConfig::GetConfig();
        auto saved = securityConfig.FramingProtectionEnabled;
        securityConfig.FramingProtectionEnabled = true;
        KFinally([&]
        {
            securityConfig.FramingProtectionEnabled = saved;
        });

        X509Compatibility::StartServer();

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(Client2, *boost::unit_test::disabled()) //FramingProtectionEnabled
    {
        ENTER;

        SecurityConfig & securityConfig = SecurityConfig::GetConfig();
        auto saved = securityConfig.FramingProtectionEnabled;
        securityConfig.FramingProtectionEnabled = true;
        KFinally([&]
        {
            securityConfig.FramingProtectionEnabled = saved;
        });

        X509Compatibility::StartClient();

        LEAVE;
    }

#endif

    BOOST_AUTO_TEST_SUITE_END()

    void SecureTransportTests::ClientServerMessageSizeLimitTest_WithoutClientRole(
        bool disableIncomingMessageSizeLimitOnClient,
        bool disableOutgoingMessageSizeLimitOnServer)
    {
        const wstring clientCn = L"client.test.com";
        const wstring serverCn = L"server.test.com";
        InstallTestCertInScope clientCert(L"CN=" + clientCn);
        InstallTestCertInScope serverCert(L"CN=" + serverCn);

        auto clientSecSettings = TTestUtil::CreateX509SettingsTp(
            clientCert.Thumbprint()->PrimaryToString(),
            L"",
            serverCert.Thumbprint()->PrimaryToString(),
            L"");

        auto serverSecSettings = TTestUtil::CreateX509SettingsTp(
            serverCert.Thumbprint()->PrimaryToString(),
            L"",
            clientCert.Thumbprint()->PrimaryToString(),
            L"");

        Assert::DisableDebugBreakInThisScope disableDebugBreakInThisScope;
        Assert::DisableTestAssertInThisScope disableTestAssertInThisScope;

        auto client = DatagramTransportFactory::CreateTcpClient();
        auto server = DatagramTransportFactory::CreateTcp(TTestUtil::GetListenAddress());

        VERIFY_IS_TRUE(client->SetSecurity(clientSecSettings).IsSuccess());
        VERIFY_IS_TRUE(server->SetSecurity(serverSecSettings).IsSuccess());

        ULONG lowMessageSizeLimit = 1024;
        client->SetMaxOutgoingFrameSize(lowMessageSizeLimit);
        if (!disableIncomingMessageSizeLimitOnClient)
        {
            Trace.WriteInfo(TraceType, "setting incoming message size limit on client side");
            client->SetMaxIncomingFrameSize(lowMessageSizeLimit);
        }

        server->SetMaxIncomingFrameSize(lowMessageSizeLimit);
        if (!disableOutgoingMessageSizeLimitOnServer)
        {
            Trace.WriteInfo(TraceType, "setting outgoing message size limit on server side");
            server->SetMaxOutgoingFrameSize(lowMessageSizeLimit); //Setting or not setting this should not matter, what matters is if incoming message size limit is enabled on client side
        }

        // Make sure message is larger than size limit
        std::wstring requestBodyString;
        while (requestBodyString.size() * sizeof(wchar_t) < TransportSecurity::Test_GetInternalMaxFrameSize(lowMessageSizeLimit))
        {
            requestBodyString += L"Message ";
        }
        requestBodyString += L"oh yeah";

        TcpTestMessage requestBody(requestBodyString);

        AutoResetEvent messageReceived;
        wstring testAction = TTestUtil::GetGuidAction();
        ISendTarget::SPtr clientTargetOnServer;
        TTestUtil::SetMessageHandler(
            server,
            testAction,
            [&messageReceived, requestBody, &clientTargetOnServer](MessageUPtr & message, ISendTarget::SPtr const & st) -> void
            {
                Trace.WriteInfo(TraceType, "server got a message from {0}", st->Address());
                clientTargetOnServer = st;

                if (message->Actor != Actor::GenericTestActor)
                {
                    TcpTestMessage body;
                    VERIFY_IS_TRUE(message->GetBody(body));
                    VERIFY_IS_TRUE(requestBody.message_ == body.message_);
                }

                messageReceived.Set();
            });

        AutoResetEvent clientReceivedMsg(false);
        TTestUtil::SetMessageHandler(
            client,
            testAction,
            [&clientReceivedMsg, &requestBody](MessageUPtr & message, ISendTarget::SPtr const & st) -> void
            {
                Trace.WriteInfo(TraceType, "client got a message from {0}", st->Address());

                TcpTestMessage body;
                VERIFY_IS_TRUE(message->GetBody(body));
                VERIFY_IS_TRUE(requestBody.message_ == body.message_);

                clientReceivedMsg.Set();
            });

        VERIFY_IS_TRUE(client->Start().IsSuccess());

        auto maxFrameSize = server->Security()->MaxIncomingFrameSize();
        VERIFY_IS_TRUE(maxFrameSize > lowMessageSizeLimit); // Due to safety margin added inside transport
        VERIFY_ARE_EQUAL2(maxFrameSize, server->Security()->MaxIncomingFrameSize()); // Verification for RDBug 3130417

        VERIFY_IS_TRUE(server->Start().IsSuccess());

        ISendTarget::SPtr target = client->ResolveTarget(server->ListenAddress(), L"");
        VERIFY_IS_TRUE(target);

        auto ping = make_unique<Message>();
        ping->Headers.Add(ActionHeader(testAction));
        ping->Headers.Add(ActorHeader(Actor::GenericTestActor));
        ping->Headers.Add(MessageIdHeader());
        Trace.WriteInfo(
            TraceType,
            "sending ping {0} to trigger security negotiaion, dropping on sending side is only enabled after security negotiation is completed",
            ping->TraceId());
        auto error = client->SendOneWay(target, std::move(ping));
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(messageReceived.WaitOne(TimeSpan::FromSeconds(10)));
        //no need to wait for ping reply to be recevied by clients. Without client role enabled, all clients
        //are admin clients, oversized outgoing messages from admin clients should not be dropped either before
        //or after the connection is confirmed

        auto oneway = make_unique<Message>(requestBody);
        oneway->Headers.Add(ActionHeader(testAction));
        oneway->Headers.Add(MessageIdHeader());

        error = client->SendOneWay(target, std::move(oneway));
        Trace.WriteInfo(TraceType, "Message will not be dropped. When client role is not enabled, all clients are admin clients, for which both sending and receiving side message size checkings are disabled");
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(messageReceived.WaitOne(TimeSpan::FromSeconds(10)));

        auto largeMessageFromServerToClient = make_unique<Message>(requestBody);
        largeMessageFromServerToClient->Headers.Add(ActionHeader(testAction));
        largeMessageFromServerToClient->Headers.Add(MessageIdHeader());
        error = server->SendOneWay(clientTargetOnServer, move(largeMessageFromServerToClient));
        Trace.WriteInfo(TraceType, "server send returned {0}", error);

        if (disableIncomingMessageSizeLimitOnClient)
        {
            VERIFY_IS_TRUE(error.IsSuccess());
            VERIFY_IS_TRUE(clientReceivedMsg.WaitOne(TimeSpan::FromSeconds(10)));
        }
        else
        {
            Trace.WriteInfo(TraceType, "server side outoging message size limit: {0}/0x{0:x}, should have been adjusted according to client limits");
            VERIFY_IS_TRUE(clientTargetOnServer->MaxOutgoingMessageSize() == TransportSecurity::Test_GetInternalMaxFrameSize(lowMessageSizeLimit));
            VERIFY_IS_FALSE(error.IsSuccess());
            VERIFY_IS_FALSE(clientReceivedMsg.WaitOne(TimeSpan::FromMilliseconds(100)));
        }

        client->Stop();
        server->Stop();
    }

#ifdef PLATFORM_UNIX

    // To run x509 tests with CA signed certs:
    // 1. copy \\sylvia\share\tools\test.with.ca.signed.certs.tgz to your Linux machine
    // 2. tar xfzv test.with.ca.signed.certs.tgz
    // 3. cd test.with.ca.signed.certs
    // 4. sudo ./install.certs.sh
    // 5. ${TestFolder}/Transport.Test.exe --run_test=X509_CA_Signed

    BOOST_FIXTURE_TEST_SUITE(X509_CA_Signed, SecureTransportTests, *boost::unit_test::disabled())

    BOOST_AUTO_TEST_CASE(X509TestCASignedCerts1)
    {
        ENTER;

        wstring serverCertThumbprint = L"93CDFEA6416084BF700A49DC656B2778718C65AF";
        wstring clientCertThumbprint = L"EDFF522AD07C5E1760E685A4F3AF5C67A4CA96BB";

        SecuritySettings senderSecSettings = TTestUtil::CreateX509SettingsTp(
            clientCertThumbprint,
            L"", 
            serverCertThumbprint+L"?true",
            L"");

        SecuritySettings receiverSecSettings = TTestUtil::CreateX509SettingsTp(
            serverCertThumbprint,
            L"",
            clientCertThumbprint+L"?true",
            L"");

        SecurityTest(L"127.0.0.1:0", senderSecSettings, L"127.0.0.1:0", receiverSecSettings);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509TestCASignedCerts2)
    {
        ENTER;

        wstring server = L"server";
        wstring client = L"client";

        SecuritySettings senderSecSettings = TTestUtil::CreateX509Settings(
            client,
            L"", 
            server,
            L"");

        SecuritySettings receiverSecSettings = TTestUtil::CreateX509Settings(
            server,
            L"",
            client,
            L"");

        SecurityTest(L"127.0.0.1:0", senderSecSettings, L"127.0.0.1:0", receiverSecSettings);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509TestCASignedCerts3_1)
    {
        ENTER;

        cout << "test should pass as crl checking is disabled in Transport.Test.exe.cfg" << endl;

        wstring certToLoad = L"*.eastus.cloudapp.azure.com";
        wstring server = L"server.eastus.cloudapp.azure.com";
        wstring client = L"client.eastus.cloudapp.azure.com";
        wstring issuerCertThumbprint = L"97EFF3028677894BDD4F9AC53F789BEE5DF4AD86";

        SecurityConfig::X509NameMap serverMap;
        serverMap.Add(server, issuerCertThumbprint);

        SecuritySettings senderSecSettings = TTestUtil::CreateX509Settings(
            certToLoad,
            serverMap);

        SecurityConfig::X509NameMap clientMap;
        clientMap.Add(client, issuerCertThumbprint);

        SecuritySettings receiverSecSettings = TTestUtil::CreateX509Settings(
            certToLoad,
            clientMap);

        SecurityTest(L"127.0.0.1:0", senderSecSettings, L"127.0.0.1:0", receiverSecSettings);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509TestCASignedCerts3_2)
    {
        ENTER;

        cout << "test should pass as crl retrieval failure is ignored" << endl;
        auto savedValue = SecurityConfig::GetConfig().CrlCheckingFlag;
        SecurityConfig::GetConfig().CrlCheckingFlag = 0x40000000;
        auto savedValue2 = SecurityConfig::GetConfig().IgnoreCrlOfflineError;
        SecurityConfig::GetConfig().IgnoreCrlOfflineError = true;
        auto savedValue3 = SecurityConfig::GetConfig().IgnoreSvrCrlOfflineError;
        SecurityConfig::GetConfig().IgnoreSvrCrlOfflineError = true;
        KFinally([=]
        {
            SecurityConfig::GetConfig().CrlCheckingFlag = savedValue;
            SecurityConfig::GetConfig().IgnoreCrlOfflineError = savedValue2;
            SecurityConfig::GetConfig().IgnoreSvrCrlOfflineError = savedValue3;
        });

        wstring certToLoad = L"*.eastus.cloudapp.azure.com";
        wstring server = L"server.eastus.cloudapp.azure.com";
        wstring client = L"client.eastus.cloudapp.azure.com";

        SecuritySettings senderSecSettings = TTestUtil::CreateX509Settings(
            certToLoad,
            L"", 
            server,
            L"");

        SecuritySettings receiverSecSettings = TTestUtil::CreateX509Settings(
            certToLoad,
            L"",
            client,
            L"");

        SecurityTest(L"127.0.0.1:0", senderSecSettings, L"127.0.0.1:0", receiverSecSettings);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509TestCASignedCerts3_3)
    {
        ENTER;

        cout << "failure expected as crl is not available from Linux machines" << endl;
        auto savedValue = SecurityConfig::GetConfig().CrlCheckingFlag;
        SecurityConfig::GetConfig().CrlCheckingFlag = 0x40000000;
        KFinally([=]
        {
            SecurityConfig::GetConfig().CrlCheckingFlag = savedValue;
        });

        wstring certToLoad = L"*.eastus.cloudapp.azure.com";
        wstring server = L"server.eastus.cloudapp.azure.com";
        wstring client = L"client.eastus.cloudapp.azure.com";

        SecuritySettings senderSecSettings = TTestUtil::CreateX509Settings(
            certToLoad,
            L"", 
            server,
            L"");

        SecuritySettings receiverSecSettings = TTestUtil::CreateX509Settings(
            certToLoad,
            L"",
            client,
            L"");

        SecurityTest(L"127.0.0.1:0", senderSecSettings, L"127.0.0.1:0", receiverSecSettings, true);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509TestCASignedCerts_revoked)
    {
        ENTER;

        cout << "test with revoked certs, failure expected" << endl;

        int savedValue = SecurityConfig::GetConfig().CrlCheckingFlag;
        SecurityConfig::GetConfig().CrlCheckingFlag = 0x40000000;
        KFinally([=] { SecurityConfig::GetConfig().CrlCheckingFlag = savedValue; });

        wstring server = L"revoked";
        wstring client = L"revoked";

        SecuritySettings senderSecSettings = TTestUtil::CreateX509Settings(
            client,
            L"", 
            server,
            L"");

        SecuritySettings receiverSecSettings = TTestUtil::CreateX509Settings(
            server,
            L"",
            client,
            L"");

        SecurityTest(L"127.0.0.1:0", senderSecSettings, L"127.0.0.1:0", receiverSecSettings, true);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509TestCASignedCerts_UntrustedRoot)
    {
        ENTER;

        cout << "test with certs chained to untrusted root, failure expected" << endl;

        wstring server = L"WinFabric-Test-SAN1-Alice";
        wstring client= L"WinFabric-Test-SAN1-Alice";

        SecuritySettings senderSecSettings = TTestUtil::CreateX509Settings(
            client,
            L"", 
            server,
            L"");

        SecuritySettings receiverSecSettings = TTestUtil::CreateX509Settings(
            server,
            L"",
            client,
            L"");

        SecurityTest(L"127.0.0.1:0", senderSecSettings, L"127.0.0.1:0", receiverSecSettings, true);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509TestCASignedCerts_UntrustedRoot2)
    {
        ENTER;

        cout << "test with certs chained to untrusted root, test should pass since auth is done by cert thumbprint matching" << endl;

        wstring server = L"WinFabric-Test-SAN1-Alice";
        wstring client= L"WinFabric-Test-SAN1-Alice";
        wstring serverCertThumbprint = L"7812205A39D22376DAA037F05AEDE3601A7E64BF";
        wstring clientCertThumbprint = L"7812205A39D22376DAA037F05AEDE3601A7E64BF";

        SecuritySettings senderSecSettings = TTestUtil::CreateX509SettingsTp(
            clientCertThumbprint,
            L"", 
            serverCertThumbprint,
            L"");

        SecuritySettings receiverSecSettings = TTestUtil::CreateX509SettingsTp(
            serverCertThumbprint,
            L"",
            clientCertThumbprint,
            L"");

        SecurityTest(L"127.0.0.1:0", senderSecSettings, L"127.0.0.1:0", receiverSecSettings);

        LEAVE;
    }

    BOOST_AUTO_TEST_SUITE_END()

#endif

}
