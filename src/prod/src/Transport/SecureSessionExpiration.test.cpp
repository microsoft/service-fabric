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

using namespace Common;
using namespace std;

namespace Transport
{
    struct RequestReplyTestRoot : public ComponentRoot
    {
        IDatagramTransportSPtr client;
        shared_ptr<RequestReply> requestReply;
    };

    class SessionExpirationTestBase
    {
    protected:
        void ReliabilityTest_ClientServer(SecuritySettings const & securitySettings);

        void ReliabilityTest_NodeToNode(
            SecuritySettings const & securitySettings,
            bool leaseMode = false);

        void ReliabilityTest_PeerToPeer_Send(
            IDatagramTransportSPtr const & node,
            ISendTarget::SPtr const & target,
            TimeSpan sendDuration,
            int & sentCount);

        void RequestReplyAtSessionExpiration(SecuritySettings const & securitySettings);

        void ConnectionSurvivesRefreshFailure(SecuritySettings const & securitySettings);

        SecurityTestSetup securityTestSetup_;
    };

    BOOST_FIXTURE_TEST_SUITE2(SessionExpiration, SessionExpirationTestBase)

    BOOST_AUTO_TEST_CASE(X509_ClientServer)
    {
        ENTER;

        SecurityConfig & securityConfig = SecurityConfig::GetConfig();
        auto savedSessionExpiration = securityConfig.SessionExpiration;
        securityConfig.SessionExpiration = TimeSpan::FromSeconds(5);
        auto savedDefaultCloseDelay = SecurityConfig::GetConfig().SessionExpirationCloseDelay;
        SecurityConfig::GetConfig().SessionExpirationCloseDelay = TimeSpan::FromSeconds(1);
        auto saved3 = TransportConfig::GetConfig().CloseDrainTimeout;
        TransportConfig::GetConfig().CloseDrainTimeout = TimeSpan::FromMilliseconds(100);
        KFinally([&]
        {
            securityConfig.SessionExpiration = savedSessionExpiration;
            securityConfig.SessionExpirationCloseDelay = savedDefaultCloseDelay;
            TransportConfig::GetConfig().CloseDrainTimeout = saved3;
        });

        auto client = DatagramTransportFactory::CreateTcpClient(L"client");
        auto disconnected = make_shared<AutoResetEvent>(false);
        auto disconnectCount = make_shared<atomic_uint64>(0);
        auto cde = client->RegisterDisconnectEvent(
            [disconnected, disconnectCount](const IDatagramTransport::DisconnectEventArgs& e)
        {
            Trace.WriteInfo(TraceType, "client: disconnected: target={0}, fault={1}, count = {2}", e.Target->TraceId(), e.Fault, ++(*disconnectCount));
            VERIFY_IS_TRUE(e.Fault.IsError(SESSION_EXPIRATION_FAULT));
            disconnected->Set();
        });

        InstallTestCertInScope testCert;
        SecuritySettings securitySettings = TTestUtil::CreateX509SettingsTp(
            testCert.Thumbprint()->PrimaryToString(),
            L"",
            testCert.Thumbprint()->PrimaryToString(),
            L"");

        VERIFY_IS_TRUE(client->SetSecurity(securitySettings).IsSuccess());
        VERIFY_IS_TRUE(client->Start().IsSuccess());

        auto server = DatagramTransportFactory::CreateTcp(TTestUtil::GetListenAddress(), L"", L"server");
        VERIFY_IS_TRUE(server->SetSecurity(securitySettings).IsSuccess());

        AutoResetEvent messageReceived;
        wstring testAction = TTestUtil::GetGuidAction();
        TTestUtil::SetMessageHandler(
            server,
            testAction,
            [&server, &client, &messageReceived](MessageUPtr &, ISendTarget::SPtr const & st) -> void
        {
            Trace.WriteInfo(TraceType, "receiver got a message from {0}", st->Address());
            messageReceived.Set();
        });

        auto disconnectedServer = make_shared<AutoResetEvent>(false);
        auto disconnectCountServer = make_shared<atomic_uint64>(0);
        auto sde = server->RegisterDisconnectEvent([disconnectedServer, disconnectCountServer](const IDatagramTransport::DisconnectEventArgs& e)
        {
            Trace.WriteInfo(TraceType, "server: disconnected: target={0}, fault={1}, count = {2}", e.Target->TraceId(), e.Fault, ++(*disconnectCountServer));
            VERIFY_IS_TRUE(e.Fault.IsError(ErrorCodeValue::SecuritySessionExpiredByRemoteEnd));
            disconnectedServer->Set();
        });

        VERIFY_IS_TRUE(server->Start().IsSuccess());

        ISendTarget::SPtr target = client->ResolveTarget(server->ListenAddress());
        VERIFY_IS_TRUE(target);

        auto message1 = make_unique<Message>();
        message1->Headers.Add(ActionHeader(testAction));
        message1->Headers.Add(MessageIdHeader());

        Trace.WriteInfo(TraceType, "sending first message");
        client->SendOneWay(target, std::move(message1));
        VERIFY_IS_TRUE(messageReceived.WaitOne(TimeSpan::FromSeconds(30)));

        VERIFY_IS_TRUE(TTestUtil::WaitUntil(
            [disconnected] { return disconnected->IsSet(); },
            securityConfig.SessionExpiration + TimeSpan::FromSeconds(5)));

        VERIFY_IS_TRUE(TTestUtil::WaitUntil(
            [disconnectedServer] { return disconnectedServer->IsSet(); },
            TimeSpan::FromSeconds(5)));

        // Now the old session should have been replaced by a new one
        VERIFY_IS_TRUE(
            TTestUtil::WaitUntil(
                [&]
        {
            size_t connectionCount = target->ConnectionCount();
            Trace.WriteInfo(TraceType, "client connection count = {0}, 1 is expected", connectionCount);
            return connectionCount == 1;
        },
            TimeSpan::FromSeconds(3)));

        auto message2 = make_unique<Message>();
        message2->Headers.Add(ActionHeader(testAction));
        message2->Headers.Add(MessageIdHeader());

        Trace.WriteInfo(TraceType, "sending second message");
        client->SendOneWay(target, std::move(message2));
        VERIFY_IS_TRUE(messageReceived.WaitOne(TimeSpan::FromSeconds(30)));

        Trace.WriteInfo(TraceType, "the second expiration is not due yet");
        auto dcount = disconnectCount->load();
        Trace.WriteInfo(TraceType, "disconnect count = {0}", disconnectCount->load());
        VERIFY_IS_TRUE(dcount == 1);

        auto dcountServer = disconnectCountServer->load();
        Trace.WriteInfo(TraceType, "server disconnect count = {0}", disconnectCountServer->load());
        VERIFY_IS_TRUE(dcountServer == 1);

        auto connectionCount = target->ConnectionCount();
        Trace.WriteInfo(TraceType, "client connection count = {0}", connectionCount);
        VERIFY_IS_TRUE(connectionCount == 1);

        client->UnregisterDisconnectEvent(cde);
        server->UnregisterDisconnectEvent(sde);

        client->Stop();
        server->Stop();

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509_ServerOnly)
    {
        ENTER;

        SecurityConfig & securityConfig = SecurityConfig::GetConfig();
        auto savedDefaultCloseDelay = SecurityConfig::GetConfig().SessionExpirationCloseDelay;
        SecurityConfig::GetConfig().SessionExpirationCloseDelay = TimeSpan::Zero;
        auto saved2 = SecurityConfig::GetConfig().SessionRefreshTimeout;
        SecurityConfig::GetConfig().SessionRefreshTimeout = TimeSpan::FromSeconds(1);
        KFinally([&]
        {
            securityConfig.SessionExpirationCloseDelay = savedDefaultCloseDelay;
            SecurityConfig::GetConfig().SessionRefreshTimeout = saved2;
        });

        auto client = DatagramTransportFactory::CreateTcpClient(L"client");
        auto disconnected = make_shared<AutoResetEvent>(false);
        auto disconnectCount = make_shared<atomic_uint64>(0);
        auto cde = client->RegisterDisconnectEvent([disconnected, disconnectCount](const IDatagramTransport::DisconnectEventArgs& e)
        {
            Trace.WriteInfo(TraceType, "client: disconnected: target={0}, fault={1}, count = {2}", e.Target->TraceId(), e.Fault, ++(*disconnectCount));
            VERIFY_IS_TRUE(e.Fault.IsError(SESSION_EXPIRATION_FAULT));
            disconnected->Set();
        });

        InstallTestCertInScope testCert;
        SecuritySettings clientSecuritySettings = TTestUtil::CreateX509SettingsTp(
            testCert.Thumbprint()->PrimaryToString(),
            L"",
            testCert.Thumbprint()->PrimaryToString(),
            L"");

        VERIFY_IS_TRUE(client->SetSecurity(clientSecuritySettings).IsSuccess());
        Trace.WriteInfo(TraceType, "disable session expiration on client to simulate a misbehaving client");
        client->DisableSecureSessionExpiration();
        VERIFY_IS_TRUE(client->Start().IsSuccess());

        auto server = DatagramTransportFactory::CreateTcp(TTestUtil::GetListenAddress(), L"", L"server");
        SecuritySettings serverSecuritySettings = TTestUtil::CreateX509SettingsTp(
            testCert.Thumbprint()->PrimaryToString(),
            L"",
            testCert.Thumbprint()->PrimaryToString(),
            L"");

        serverSecuritySettings.SetSessionDurationCallback([] { return TimeSpan::FromSeconds(3); });

        VERIFY_IS_TRUE(server->SetSecurity(serverSecuritySettings).IsSuccess());

        AutoResetEvent messageReceived;
        wstring testAction = TTestUtil::GetGuidAction();
        TTestUtil::SetMessageHandler(server, testAction, [&server, &client, &messageReceived](MessageUPtr &, ISendTarget::SPtr const & st) -> void
        {
            Trace.WriteInfo(TraceType, "receiver got a message from {0}", st->Address());
            messageReceived.Set();
        });

        auto disconnectedServer = make_shared<AutoResetEvent>(false);
        auto disconnectCountServer = make_shared<atomic_uint64>(0);
        auto sde = server->RegisterDisconnectEvent([disconnectedServer, disconnectCountServer](const IDatagramTransport::DisconnectEventArgs& e)
        {
            Trace.WriteInfo(TraceType, "server: disconnected: target={0}, fault={1}, count = {2}", e.Target->TraceId(), e.Fault, ++(*disconnectCountServer));
            VERIFY_IS_TRUE(e.Fault.IsError(ErrorCodeValue::SecuritySessionExpiredByRemoteEnd));
            disconnectedServer->Set();
        });

        VERIFY_IS_TRUE(server->Start().IsSuccess());

        ISendTarget::SPtr target = client->ResolveTarget(server->ListenAddress());
        VERIFY_IS_TRUE(target);

        auto message1 = make_unique<Message>();
        message1->Headers.Add(ActionHeader(testAction));
        message1->Headers.Add(MessageIdHeader());

        Trace.WriteInfo(TraceType, "sending first message");
        client->SendOneWay(target, std::move(message1));
        VERIFY_IS_TRUE(messageReceived.WaitOne(TimeSpan::FromSeconds(20)));

        VERIFY_IS_TRUE(TTestUtil::WaitUntil(
            [disconnected] { return disconnected->IsSet(); },
            securityConfig.SessionExpiration + TimeSpan::FromSeconds(5)));

        VERIFY_IS_TRUE(TTestUtil::WaitUntil(
            [disconnectedServer] { return disconnectedServer->IsSet(); },
            TimeSpan::FromSeconds(5)));

        // Now the old session should have been replaced by a new one
        VERIFY_IS_TRUE(
            TTestUtil::WaitUntil([&]
        {
            size_t connectionCount = target->ConnectionCount();
            Trace.WriteInfo(TraceType, "client connection count = {0}, 1 is expected", connectionCount);
            return connectionCount == 1;
        },
                TimeSpan::FromSeconds(3)));

        auto message2 = make_unique<Message>();
        message2->Headers.Add(ActionHeader(testAction));
        message2->Headers.Add(MessageIdHeader());
        Trace.WriteInfo(TraceType, "sending second message");
        client->SendOneWay(target, std::move(message2));
        VERIFY_IS_TRUE(messageReceived.WaitOne(TimeSpan::FromSeconds(30)));

        client->UnregisterDisconnectEvent(cde);
        server->UnregisterDisconnectEvent(sde);

        client->Stop();
        server->Stop();

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509_PeerToPeer)
    {
        ENTER;

        SecurityConfig & securityConfig = SecurityConfig::GetConfig();
        auto savedSessionExpiration = securityConfig.SessionExpiration;
        securityConfig.SessionExpiration = TimeSpan::FromSeconds(6);
        auto savedDefaultCloseDelay = SecurityConfig::GetConfig().SessionExpirationCloseDelay;
        SecurityConfig::GetConfig().SessionExpirationCloseDelay = TimeSpan::Zero;
        KFinally([&]
        {
            securityConfig.SessionExpiration = savedSessionExpiration;
            securityConfig.SessionExpirationCloseDelay = savedDefaultCloseDelay;
        });

        auto node1 = DatagramTransportFactory::CreateTcp(TTestUtil::GetListenAddress(), L"", L"node1");
        auto disconnected1 = make_shared<AutoResetEvent>(false);
        auto disconnectCount1 = make_shared<atomic_uint64>(0);
        auto de1 = node1->RegisterDisconnectEvent([disconnected1, disconnectCount1](const IDatagramTransport::DisconnectEventArgs& e)
        {
            Trace.WriteInfo(TraceType, "client: disconnected: target={0}, fault={1}, count = {2}", e.Target->TraceId(), e.Fault, ++(*disconnectCount1));
            VERIFY_IS_TRUE(e.Fault.IsError(SESSION_EXPIRATION_FAULT));
            disconnected1->Set();
        });

        InstallTestCertInScope testCert;
        SecuritySettings securitySettings = TTestUtil::CreateX509SettingsTp(
            testCert.Thumbprint()->PrimaryToString(),
            L"",
            testCert.Thumbprint()->PrimaryToString(),
            L"");

        securitySettings.EnablePeerToPeerMode();

        VERIFY_IS_TRUE(node1->SetSecurity(securitySettings).IsSuccess());
        VERIFY_IS_TRUE(node1->Start().IsSuccess());

        auto node2 = DatagramTransportFactory::CreateTcp(TTestUtil::GetListenAddress(), L"", L"node2");
        auto disconnected2 = make_shared<AutoResetEvent>(false);
        auto disconnectCount2 = make_shared<atomic_uint64>(0);
        auto de2 = node2->RegisterDisconnectEvent([disconnected2, disconnectCount2](const IDatagramTransport::DisconnectEventArgs& e)
        {
            Trace.WriteInfo(TraceType, "client: disconnected: target={0}, fault={1}, count = {2}", e.Target->TraceId(), e.Fault, ++(*disconnectCount2));
            // node1 sends message first, so node1 is the connecting side, thus node1 should initiate session expiration
            VERIFY_IS_TRUE(e.Fault.IsError(ErrorCodeValue::SecuritySessionExpiredByRemoteEnd));
            disconnected2->Set();
        });

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

        Trace.WriteInfo(TraceType, "wait for first disconnect");
        VERIFY_IS_TRUE(TTestUtil::WaitUntil(
            [disconnected1] { return disconnected1->IsSet(); },
            securityConfig.SessionExpiration + TimeSpan::FromSeconds(5)));

        VERIFY_IS_TRUE(TTestUtil::WaitUntil(
            [disconnected2] { return disconnected2->IsSet(); },
            TimeSpan::FromSeconds(5)));

        // Now the old session should have been replaced by a new one
        VERIFY_IS_TRUE(
            TTestUtil::WaitUntil(
                [&]
        {
            size_t connectionCount = target->ConnectionCount();
            Trace.WriteInfo(TraceType, "node1 connection count = {0}, 1 is expected", connectionCount);
            return connectionCount == 1;
        },
            TimeSpan::FromSeconds(3)));

        Trace.WriteInfo(TraceType, "Only one disconnect event one each side is expected, as second session expiration is not due yet, expiration = {0}", securityConfig.SessionExpiration);
        auto dcount = disconnectCount1->load();
        Trace.WriteInfo(TraceType, "disconnect count = {0}", disconnectCount1->load());
        VERIFY_IS_TRUE(dcount == 1);

        auto dcount2 = disconnectCount2->load();
        Trace.WriteInfo(TraceType, "disconnect count = {0}", disconnectCount2->load());
        VERIFY_IS_TRUE(dcount2 == 1);

        Trace.WriteInfo(TraceType, "wait for second disconnect");
        VERIFY_IS_TRUE(TTestUtil::WaitUntil(
            [disconnected1] { return disconnected1->IsSet(); },
            securityConfig.SessionExpiration + TimeSpan::FromSeconds(5)));

        VERIFY_IS_TRUE(TTestUtil::WaitUntil(
            [disconnected2] { return disconnected2->IsSet(); },
            TimeSpan::FromSeconds(5)));

        VERIFY_IS_TRUE(
            TTestUtil::WaitUntil(
                [&]
        {
            size_t connectionCount = target->ConnectionCount();
            Trace.WriteInfo(TraceType, "node1 connection count = {0}, 1 is expected", connectionCount);
            return connectionCount == 1;
        },
            TimeSpan::FromSeconds(3)));

        node1->UnregisterDisconnectEvent(de1);
        node2->UnregisterDisconnectEvent(de2);

        node1->Stop();
        node2->Stop();

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509_ClientSecUpdate)
    {
        ENTER;

        SecurityConfig & securityConfig = SecurityConfig::GetConfig();
        auto savedMaxSessionRefreshDelay = securityConfig.MaxSessionRefreshDelay;
        securityConfig.MaxSessionRefreshDelay = TimeSpan::FromSeconds(3);
        auto savedDefaultCloseDelay = SecurityConfig::GetConfig().SessionExpirationCloseDelay;
        SecurityConfig::GetConfig().SessionExpirationCloseDelay = TimeSpan::FromSeconds(1);
        auto savedRefreshTimeout = SecurityConfig::GetConfig().SessionRefreshTimeout;
        SecurityConfig::GetConfig().SessionRefreshTimeout = TimeSpan::FromSeconds(1);
        auto saved4 = TransportConfig::GetConfig().CloseDrainTimeout;
        TransportConfig::GetConfig().CloseDrainTimeout = TimeSpan::Zero;
        KFinally([=]
        {
            SecurityConfig::GetConfig().MaxSessionRefreshDelay = savedMaxSessionRefreshDelay;
            SecurityConfig::GetConfig().SessionExpirationCloseDelay = savedDefaultCloseDelay;
            SecurityConfig::GetConfig().SessionRefreshTimeout = savedRefreshTimeout;
            TransportConfig::GetConfig().CloseDrainTimeout = saved4;
        });

        InstallTestCertInScope testCert;
        SecuritySettings securitySettings = TTestUtil::CreateX509SettingsTp(
            testCert.Thumbprint()->PrimaryToString(),
            L"",
            testCert.Thumbprint()->PrimaryToString(),
            L"");

        auto client = DatagramTransportFactory::CreateTcpClient(L"client");
        auto disconnected = make_shared<AutoResetEvent>(false);
        auto disconnectCount = make_shared<atomic_uint64>(0);
        auto cde = client->RegisterDisconnectEvent([disconnected, disconnectCount](const IDatagramTransport::DisconnectEventArgs& e)
        {
            Trace.WriteInfo(TraceType, "client: disconnected: target={0}, fault={1}, count = {2}", e.Target->TraceId(), e.Fault, ++(*disconnectCount));
            VERIFY_IS_TRUE(e.Fault.IsError(SESSION_EXPIRATION_FAULT));
            disconnected->Set();
        });

        VERIFY_IS_TRUE(client->SetSecurity(securitySettings).IsSuccess());
        VERIFY_IS_TRUE(client->Start().IsSuccess());

        auto server = DatagramTransportFactory::CreateTcp(TTestUtil::GetListenAddress(), L"", L"server");
        VERIFY_IS_TRUE(server->SetSecurity(securitySettings).IsSuccess());

        AutoResetEvent messageReceived;
        server->SetMessageHandler([&server, &client, &messageReceived](MessageUPtr &, ISendTarget::SPtr const & st) -> void
        {
            Trace.WriteInfo(TraceType, "server got a message from {0}", st->Address());
            messageReceived.Set();
        });

        auto disconnectedServer = make_shared<AutoResetEvent>(false);
        auto disconnectCountServer = make_shared<atomic_uint64>(0);
        auto sde = server->RegisterDisconnectEvent([disconnectedServer, disconnectCountServer](const IDatagramTransport::DisconnectEventArgs& e)
        {
            Trace.WriteInfo(TraceType, "server: disconnected: target={0}, fault={1}, count = {2}", e.Target->TraceId(), e.Fault, ++(*disconnectCountServer));
            VERIFY_IS_TRUE(e.Fault.IsError(ErrorCodeValue::SecuritySessionExpiredByRemoteEnd));
            disconnectedServer->Set();
        });

        VERIFY_IS_TRUE(server->Start().IsSuccess());

        ISendTarget::SPtr target = client->ResolveTarget(server->ListenAddress());
        VERIFY_IS_TRUE(target);

        auto message1 = make_unique<Message>();
        message1->Headers.Add(MessageIdHeader());

        Trace.WriteInfo(TraceType, "client sending first message");
        client->SendOneWay(target, std::move(message1));
        VERIFY_IS_TRUE(messageReceived.WaitOne(TimeSpan::FromSeconds(20)));

        // Update security setting to trigger the current session to be replaced
        auto error = client->SetSecurity(securitySettings);
        VERIFY_IS_TRUE(error.IsSuccess());

        Trace.WriteInfo(TraceType, "connection created for previous send will be replaced");
        VERIFY_IS_TRUE(disconnected->WaitOne(securityConfig.SessionExpiration + TimeSpan::FromSeconds(5)));
        VERIFY_IS_TRUE(disconnectedServer->WaitOne(TimeSpan::FromSeconds(5)));

        auto dcount = disconnectCount->load();
        Trace.WriteInfo(TraceType, "client: disconnect count = {0}", dcount);
        VERIFY_IS_TRUE(dcount == 1);

        VERIFY_IS_TRUE(
            TTestUtil::WaitUntil(
                [&]
        {
            size_t connectionCount = target->ConnectionCount();
            Trace.WriteInfo(TraceType, "client connection count = {0}, 1 is expected", connectionCount);
            return connectionCount == 1;
        },
            TimeSpan::FromSeconds(3)));

        auto message2 = make_unique<Message>();
        message2->Headers.Add(MessageIdHeader());
        Trace.WriteInfo(TraceType, "client sending second message");
        client->SendOneWay(target, std::move(message2));
        VERIFY_IS_TRUE(messageReceived.WaitOne(TimeSpan::FromSeconds(20)));

        dcount = disconnectCount->load();
        Trace.WriteInfo(TraceType, "client: disconnect count = {0}", dcount);
        VERIFY_IS_TRUE(dcount == 1);
        dcount = disconnectCountServer->load();
        Trace.WriteInfo(TraceType, "server: disconnect count = {0}", dcount);
        VERIFY_IS_TRUE(dcount == 1);

        VERIFY_IS_TRUE(
            TTestUtil::WaitUntil(
                [&]
        {
            size_t connectionCount = target->ConnectionCount();
            Trace.WriteInfo(TraceType, "client connection count = {0}, 1 is expected", connectionCount);
            return connectionCount == 1;
        },
            TimeSpan::FromSeconds(3)));

        client->UnregisterDisconnectEvent(cde);
        server->UnregisterDisconnectEvent(sde);

        client->Stop();
        server->Stop();

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509_ServerSecUpdate)
    {
        ENTER;

        SecurityConfig & securityConfig = SecurityConfig::GetConfig();
        auto savedMaxSessionRefreshDelay = securityConfig.MaxSessionRefreshDelay;
        securityConfig.MaxSessionRefreshDelay = TimeSpan::FromSeconds(3);
        auto savedDefaultCloseDelay = SecurityConfig::GetConfig().SessionExpirationCloseDelay;
        SecurityConfig::GetConfig().SessionExpirationCloseDelay = TimeSpan::FromSeconds(3);
        auto savedRefreshTimeout = SecurityConfig::GetConfig().SessionRefreshTimeout;
        SecurityConfig::GetConfig().SessionRefreshTimeout = TimeSpan::FromSeconds(1);
        auto saved3 = TransportConfig::GetConfig().CloseDrainTimeout;
        TransportConfig::GetConfig().CloseDrainTimeout = TimeSpan::FromSeconds(1);
        KFinally([&]
        {
            SecurityConfig::GetConfig().MaxSessionRefreshDelay = savedMaxSessionRefreshDelay;
            SecurityConfig::GetConfig().SessionExpirationCloseDelay = savedDefaultCloseDelay;
            SecurityConfig::GetConfig().SessionRefreshTimeout = savedRefreshTimeout;
            TransportConfig::GetConfig().CloseDrainTimeout = saved3;
        });

        InstallTestCertInScope testCert;
        SecuritySettings securitySettings = TTestUtil::CreateX509SettingsTp(
            testCert.Thumbprint()->PrimaryToString(),
            L"",
            testCert.Thumbprint()->PrimaryToString(),
            L"");

        auto client = DatagramTransportFactory::CreateTcpClient(L"client");
        auto clientConnectionFaults = make_shared<vector<ErrorCode>>();
        auto clientLock = make_shared<RwLock>();
        auto cde = client->RegisterDisconnectEvent([clientConnectionFaults, clientLock](const IDatagramTransport::DisconnectEventArgs& e)
        {
            Trace.WriteInfo(TraceType, "client: disconnected: target={0}, fault={1}", e.Target->TraceId(), e.Fault);
            AcquireWriteLock grab(*clientLock);
            clientConnectionFaults->emplace_back(e.Fault);
        });

        VERIFY_IS_TRUE(client->SetSecurity(securitySettings).IsSuccess());
        VERIFY_IS_TRUE(client->Start().IsSuccess());

        auto server = DatagramTransportFactory::CreateTcp(TTestUtil::GetListenAddress());
        VERIFY_IS_TRUE(server->SetSecurity(securitySettings).IsSuccess());

        AutoResetEvent messageReceived;
        server->SetMessageHandler([&server, &client, &messageReceived](MessageUPtr &, ISendTarget::SPtr const & st) -> void
        {
            Trace.WriteInfo(TraceType, "server got a message from {0}", st->Address());
            messageReceived.Set();
        });

        auto serverConnectionFaults = make_shared<vector<ErrorCode>>();
        auto serverLock = make_shared<RwLock>();
        auto sde = server->RegisterDisconnectEvent([serverConnectionFaults, serverLock](const IDatagramTransport::DisconnectEventArgs& e)
        {
            Trace.WriteInfo(TraceType, "server: disconnected: target={0}, fault={1}", e.Target->TraceId(), e.Fault);
            AcquireWriteLock grab(*serverLock);
            serverConnectionFaults->emplace_back(e.Fault);
        });

        VERIFY_IS_TRUE(server->Start().IsSuccess());

        ISendTarget::SPtr target = client->ResolveTarget(server->ListenAddress());
        VERIFY_IS_TRUE(target);

        auto message1 = make_unique<Message>();
        message1->Headers.Add(MessageIdHeader());

        Trace.WriteInfo(TraceType, "client sending first message");
        client->SendOneWay(target, std::move(message1));
        VERIFY_IS_TRUE(messageReceived.WaitOne(TimeSpan::FromSeconds(20)));

        // To further prove that the old connection is gone, set server security setting so that
        // server will reject new session from client and cause message sending to fail
        SecuritySettings noOneIsAllowedSecuritySettings = TTestUtil::CreateX509SettingsTp(
            testCert.Thumbprint()->PrimaryToString(),
            L"",
            L"0000000000000000000000000000000000000000", //Assuming this will match any certificate thumbprints
            L"");

        VERIFY_IS_TRUE(server->SetSecurity(noOneIsAllowedSecuritySettings).IsSuccess());

        Trace.WriteInfo(TraceType, "wait for all connections to go down due to noOneIsAllowedSecuritySettings");
        VERIFY_IS_TRUE(TTestUtil::WaitUntil(
            [&] { return target->ConnectionCount() == 0; },
            securityConfig.SessionExpiration + TimeSpan::FromSeconds(5)));

        auto message2 = make_unique<Message>();
        message2->Headers.Add(MessageIdHeader());
        Trace.WriteInfo(TraceType, "client sending second message, will not reach server");
        client->SendOneWay(target, std::move(message2));
        VERIFY_IS_FALSE(messageReceived.WaitOne(TimeSpan::FromSeconds(3)));

        // Now set server security settings back to proper value, things should come back to normal again
        VERIFY_IS_TRUE(server->SetSecurity(securitySettings).IsSuccess());

        auto message3 = make_unique<Message>();
        message3->Headers.Add(MessageIdHeader());
        Trace.WriteInfo(TraceType, "client sending third message");
        client->SendOneWay(target, std::move(message3));
        VERIFY_IS_TRUE(messageReceived.WaitOne(TimeSpan::FromSeconds(20)));

        {
            AcquireReadLock grab(*clientLock);

            Trace.WriteInfo(TraceType, "client connection faults: {0}", *clientConnectionFaults);
            VERIFY_IS_TRUE(clientConnectionFaults->size() == 3);
            VERIFY_IS_TRUE(clientConnectionFaults->at(0).IsError(ErrorCodeValue::ConnectionDenied));
            // session expiration should be forced by server side, as there is no client side security update
            VERIFY_IS_TRUE(clientConnectionFaults->at(1).IsError(ErrorCodeValue::SecuritySessionExpiredByRemoteEnd));
            VERIFY_IS_TRUE(clientConnectionFaults->at(2).IsError(ErrorCodeValue::ConnectionDenied));
        }

        {
            AcquireReadLock grab(*serverLock);

            Trace.WriteInfo(TraceType, "server connection faults: {0}", *serverConnectionFaults);
            VERIFY_IS_TRUE(serverConnectionFaults->size() == 3);
            VERIFY_IS_TRUE(serverConnectionFaults->at(0).IsError(ErrorCodeValue::ConnectionDenied));
            VERIFY_IS_TRUE(serverConnectionFaults->at(1).IsError(SESSION_EXPIRATION_FAULT));
            VERIFY_IS_TRUE(serverConnectionFaults->at(2).IsError(ErrorCodeValue::ConnectionDenied));
        }

        client->UnregisterDisconnectEvent(cde);
        server->UnregisterDisconnectEvent(sde);

        client->Stop();
        server->Stop();

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509_SecUpdateOnBothSides)
    {
        ENTER;

        SecurityConfig & securityConfig = SecurityConfig::GetConfig();
        auto savedMaxSessionRefreshDelay = securityConfig.MaxSessionRefreshDelay;
        securityConfig.MaxSessionRefreshDelay = TimeSpan::FromSeconds(3);
        auto savedDefaultCloseDelay = SecurityConfig::GetConfig().SessionExpirationCloseDelay;
        SecurityConfig::GetConfig().SessionExpirationCloseDelay = TimeSpan::Zero;
        auto savedRefreshTimeout = SecurityConfig::GetConfig().SessionRefreshTimeout;
        SecurityConfig::GetConfig().SessionRefreshTimeout = TimeSpan::FromSeconds(1);
        auto saved4 = TransportConfig::GetConfig().CloseDrainTimeout;
        TransportConfig::GetConfig().CloseDrainTimeout = TimeSpan::Zero;
        KFinally([=]
        {
            SecurityConfig::GetConfig().MaxSessionRefreshDelay = savedMaxSessionRefreshDelay;
            SecurityConfig::GetConfig().SessionExpirationCloseDelay = savedDefaultCloseDelay;
            SecurityConfig::GetConfig().SessionRefreshTimeout = savedRefreshTimeout;
            TransportConfig::GetConfig().CloseDrainTimeout = saved4;
        });

        InstallTestCertInScope testCert;
        SecuritySettings securitySettings = TTestUtil::CreateX509SettingsTp(
            testCert.Thumbprint()->PrimaryToString(),
            L"",
            testCert.Thumbprint()->PrimaryToString(),
            L"");

        auto client = DatagramTransportFactory::CreateTcpClient(L"client");
        auto disconnected = make_shared<AutoResetEvent>(false);
        auto disconnectCount = make_shared<atomic_uint64>(0);
        auto cde = client->RegisterDisconnectEvent([disconnected, disconnectCount](const IDatagramTransport::DisconnectEventArgs& e)
        {
            Trace.WriteInfo(TraceType, "client: disconnected: target={0}, fault={1}, count = {2}", e.Target->TraceId(), e.Fault, ++(*disconnectCount));
            VERIFY_IS_TRUE(e.Fault.IsError(SESSION_EXPIRATION_FAULT));
            disconnected->Set();
        });

        VERIFY_IS_TRUE(client->SetSecurity(securitySettings).IsSuccess());
        VERIFY_IS_TRUE(client->Start().IsSuccess());

        auto server = DatagramTransportFactory::CreateTcp(TTestUtil::GetListenAddress(), L"", L"server");
        VERIFY_IS_TRUE(server->SetSecurity(securitySettings).IsSuccess());

        AutoResetEvent messageReceived;
        server->SetMessageHandler([&server, &client, &messageReceived](MessageUPtr &, ISendTarget::SPtr const & st) -> void
        {
            Trace.WriteInfo(TraceType, "server got a message from {0}", st->Address());
            messageReceived.Set();
        });

        auto disconnectedServer = make_shared<AutoResetEvent>(false);
        auto disconnectCountServer = make_shared<atomic_uint64>(0);
        auto sde = server->RegisterDisconnectEvent([disconnectedServer, disconnectCountServer](const IDatagramTransport::DisconnectEventArgs& e)
        {
            Trace.WriteInfo(TraceType, "server: disconnected: target={0}, fault={1}, count = {2}", e.Target->TraceId(), e.Fault, ++(*disconnectCountServer));
            VERIFY_IS_TRUE(e.Fault.IsError(ErrorCodeValue::SecuritySessionExpiredByRemoteEnd));
            disconnectedServer->Set();
        });

        VERIFY_IS_TRUE(server->Start().IsSuccess());

        ISendTarget::SPtr target = client->ResolveTarget(server->ListenAddress());
        VERIFY_IS_TRUE(target);

        auto message1 = make_unique<Message>();
        message1->Headers.Add(MessageIdHeader());

        Trace.WriteInfo(TraceType, "client sending first message");
        client->SendOneWay(target, std::move(message1));
        VERIFY_IS_TRUE(messageReceived.WaitOne(TimeSpan::FromSeconds(20)));

        // Update security setting to trigger session fresh from both sides
        auto error = client->SetSecurity(securitySettings);
        VERIFY_IS_TRUE(error.IsSuccess());

        error = server->SetSecurity(securitySettings);
        VERIFY_IS_TRUE(error.IsSuccess());

        Trace.WriteInfo(TraceType, "connection created for previous send will be replaced");
        VERIFY_IS_TRUE(disconnected->WaitOne(securityConfig.SessionExpiration + TimeSpan::FromSeconds(5)));
        VERIFY_IS_TRUE(disconnectedServer->WaitOne(TimeSpan::FromSeconds(5)));

        auto dcount = disconnectCount->load();
        Trace.WriteInfo(TraceType, "client: disconnect count = {0}", dcount);

        VERIFY_IS_TRUE(
            TTestUtil::WaitUntil([&]
        {
            size_t connectionCount = target->ConnectionCount();
            Trace.WriteInfo(TraceType, "client connection count = {0}, 1 is expected", connectionCount);
            return connectionCount == 1;
        },
                TimeSpan::FromSeconds(3)));

        auto message2 = make_unique<Message>();
        message2->Headers.Add(MessageIdHeader());
        Trace.WriteInfo(TraceType, "client sending second message");
        client->SendOneWay(target, std::move(message2));
        VERIFY_IS_TRUE(messageReceived.WaitOne(TimeSpan::FromSeconds(20)));

        VERIFY_IS_TRUE(
            TTestUtil::WaitUntil([&]
        {
            size_t connectionCount = target->ConnectionCount();
            Trace.WriteInfo(TraceType, "client connection count = {0}, 1 is expected", connectionCount);
            return connectionCount == 1;
        },
                TimeSpan::FromSeconds(3)));

        dcount = disconnectCount->load();
        Trace.WriteInfo(TraceType, "client: disconnect count = {0}", dcount);
        VERIFY_IS_TRUE(dcount == 1);

        dcount = disconnectCountServer->load();
        Trace.WriteInfo(TraceType, "server: disconnect count = {0}", dcount);
        VERIFY_IS_TRUE(dcount == 1);

        client->UnregisterDisconnectEvent(cde);
        server->UnregisterDisconnectEvent(sde);

        client->Stop();
        server->Stop();

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509_RequestReplyAtSessionExpiration)
    {
        ENTER;

        InstallTestCertInScope testCert;
        SecuritySettings securitySettings = TTestUtil::CreateX509SettingsTp(
            testCert.Thumbprint()->PrimaryToString(),
            L"",
            testCert.Thumbprint()->PrimaryToString(),
            L"");

        RequestReplyAtSessionExpiration(securitySettings);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509_RequestReplyAtSessionExpiration_RefreshDisabled)
    {
        ENTER;

        InstallTestCertInScope testCert;
        SecuritySettings securitySettings = TTestUtil::CreateX509SettingsTp(
            testCert.Thumbprint()->PrimaryToString(),
            L"",
            testCert.Thumbprint()->PrimaryToString(),
            L"");

        securitySettings.SetReadyNewSessionBeforeExpirationCallback([] { return false; });
        RequestReplyAtSessionExpiration(securitySettings);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509_Reliability_PeerToPeer)
    {
        ENTER;

        InstallTestCertInScope testCert;
        SecuritySettings securitySettings = TTestUtil::CreateX509SettingsTp(
            testCert.Thumbprint()->PrimaryToString(),
            L"",
            testCert.Thumbprint()->PrimaryToString(),
            L"");

        securitySettings.EnablePeerToPeerMode();
        ReliabilityTest_NodeToNode(securitySettings);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509_Reliability_ClientServer)
    {
        ENTER;

        InstallTestCertInScope testCert;
        SecuritySettings securitySettings = TTestUtil::CreateX509SettingsTp(
            testCert.Thumbprint()->PrimaryToString(),
            L"",
            testCert.Thumbprint()->PrimaryToString(),
            L"");

        ReliabilityTest_ClientServer(securitySettings);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509_ConnectionSurvivesRefreshFailure)
    {
        ENTER;

        InstallTestCertInScope testCert;
        SecuritySettings securitySettings = TTestUtil::CreateX509SettingsTp(
            testCert.Thumbprint()->PrimaryToString(),
            L"",
            testCert.Thumbprint()->PrimaryToString(),
            L"");

        ConnectionSurvivesRefreshFailure(securitySettings);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509_ConnectionSurvivesRefreshFailure_PeerToPeer)
    {
        ENTER;

        InstallTestCertInScope testCert;
        SecuritySettings securitySettings = TTestUtil::CreateX509SettingsTp(
            testCert.Thumbprint()->PrimaryToString(),
            L"",
            testCert.Thumbprint()->PrimaryToString(),
            L"");

        securitySettings.EnablePeerToPeerMode();
        ConnectionSurvivesRefreshFailure(securitySettings);

        LEAVE;
    }

#ifdef PLATFORM_UNIX

    BOOST_AUTO_TEST_CASE(X509_Reliability_PeerToPeer_Lease)
    {
        ENTER;

        InstallTestCertInScope testCert;
        SecuritySettings securitySettings = TTestUtil::CreateX509SettingsTp(
            testCert.Thumbprint()->PrimaryToString(),
            L"",
            testCert.Thumbprint()->PrimaryToString(),
            L"");

        securitySettings.EnablePeerToPeerMode();
        ReliabilityTest_NodeToNode(securitySettings, true);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509_Reliability_NodeToNode_NonPeerToPeerMode)
    {
        ENTER;

        InstallTestCertInScope testCert;
        SecuritySettings securitySettings = TTestUtil::CreateX509SettingsTp(
            testCert.Thumbprint()->PrimaryToString(),
            L"",
            testCert.Thumbprint()->PrimaryToString(),
            L"");

        ReliabilityTest_NodeToNode(securitySettings);

        LEAVE;
    }

#else

    BOOST_AUTO_TEST_CASE(Win_Reliability_PeerToPeer)
    {
        ENTER;

        SecuritySettings securitySettings = TTestUtil::CreateTestSecuritySettings(
            SecurityProvider::Negotiate,
            ProtectionLevel::EncryptAndSign);

        securitySettings.EnablePeerToPeerMode();
        ReliabilityTest_NodeToNode(securitySettings);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(Win_Reliability_ClientServer)
    {
        ENTER;

        SecuritySettings securitySettings = TTestUtil::CreateTestSecuritySettings(
            SecurityProvider::Negotiate,
            ProtectionLevel::EncryptAndSign);

        ReliabilityTest_ClientServer(securitySettings);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(Win_RequestReplyAtSessionExpiration)
    {
        ENTER;

        SecuritySettings securitySettings = TTestUtil::CreateTestSecuritySettings(
            SecurityProvider::Negotiate,
            ProtectionLevel::EncryptAndSign);

        RequestReplyAtSessionExpiration(securitySettings);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(Win_ConnectionSurvivesRefreshFailure)
    {
        ENTER;

        SecuritySettings securitySettings = TTestUtil::CreateTestSecuritySettings(
            SecurityProvider::Negotiate,
            ProtectionLevel::EncryptAndSign);

        ConnectionSurvivesRefreshFailure(securitySettings);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(Win_ConnectionSurvivesRefreshFailure_PeerToPeer)
    {
        ENTER;

        SecuritySettings securitySettings = TTestUtil::CreateTestSecuritySettings(
            SecurityProvider::Negotiate,
            ProtectionLevel::EncryptAndSign);

        securitySettings.EnablePeerToPeerMode();
        ConnectionSurvivesRefreshFailure(securitySettings);

        LEAVE;
    }

#endif

    BOOST_AUTO_TEST_CASE(X509_DisableAutoRefresh)
    {
        ENTER;

        SecurityConfig & securityConfig = SecurityConfig::GetConfig();
        auto savedSessionExpiration = securityConfig.SessionExpiration;
        securityConfig.SessionExpiration = TimeSpan::FromSeconds(5);
        auto saved2 = securityConfig.SessionExpirationCloseDelay;
        securityConfig.SessionExpirationCloseDelay = TimeSpan::Zero;
        KFinally([&]
        {
            securityConfig.SessionExpiration = savedSessionExpiration;
            securityConfig.SessionExpirationCloseDelay = saved2;
        });

        auto client = DatagramTransportFactory::CreateTcpClient(L"client");
        auto disconnected = make_shared<AutoResetEvent>(false);
        auto disconnectCount = make_shared<atomic_uint64>(0);
        auto cde = client->RegisterDisconnectEvent(
            [disconnected, disconnectCount](const IDatagramTransport::DisconnectEventArgs& e)
            {
                auto dc = ++(*disconnectCount);
                Trace.WriteInfo(TraceType, "client: disconnected: target={0}, fault={1}, count = {2}", e.Target->TraceId(), e.Fault, dc);
                VERIFY_IS_TRUE(e.Fault.IsError(ErrorCodeValue::SecuritySessionExpiredByRemoteEnd) || e.Fault.IsError(SESSION_EXPIRATION_FAULT));
                if (dc == 2)
                {
                    disconnected->Set();
                }
            });

        InstallTestCertInScope testCert;
        SecuritySettings securitySettings = TTestUtil::CreateX509SettingsTp(
            testCert.Thumbprint()->PrimaryToString(),
            L"",
            testCert.Thumbprint()->PrimaryToString(),
            L"");

        securitySettings.SetReadyNewSessionBeforeExpirationCallback([] { return false; });
        VERIFY_IS_TRUE(client->SetSecurity(securitySettings).IsSuccess());
        VERIFY_IS_TRUE(client->Start().IsSuccess());

        auto server = DatagramTransportFactory::CreateTcp(TTestUtil::GetListenAddress(), L"", L"server");
        VERIFY_IS_TRUE(server->SetSecurity(securitySettings).IsSuccess());

        AutoResetEvent messageReceived;
        wstring testAction = TTestUtil::GetGuidAction();
        TTestUtil::SetMessageHandler(
            server,
            testAction,
            [&server, &client, &messageReceived](MessageUPtr &, ISendTarget::SPtr const & st) -> void
            {
                Trace.WriteInfo(TraceType, "receiver got a message from {0}", st->Address());
                messageReceived.Set();
            });

        auto disconnectedServer = make_shared<AutoResetEvent>(false);
        auto disconnectCountServer = make_shared<atomic_uint64>(0);
        auto sde = server->RegisterDisconnectEvent([disconnectedServer, disconnectCountServer](const IDatagramTransport::DisconnectEventArgs& e)
        {
            auto dc = ++(*disconnectCountServer);
            Trace.WriteInfo(TraceType, "server: disconnected: target={0}, fault={1}, count = {2}", e.Target->TraceId(), e.Fault, dc);
            if (dc == 2)
            {
                disconnectedServer->Set();
            }
        });

        VERIFY_IS_TRUE(server->Start().IsSuccess());

        ISendTarget::SPtr target = client->ResolveTarget(server->ListenAddress());
        VERIFY_IS_TRUE(target);

        auto message1 = make_unique<Message>();
        message1->Headers.Add(ActionHeader(testAction));
        message1->Headers.Add(MessageIdHeader());

        Trace.WriteInfo(TraceType, "sending at normal priority");
        client->SendOneWay(target, std::move(message1), TimeSpan::MaxValue, TransportPriority::Normal);
        VERIFY_IS_TRUE(messageReceived.WaitOne(TimeSpan::FromSeconds(30)));
        auto connectionCount1 = target->ConnectionCount();
        Trace.WriteInfo(TraceType, "connection count = {0}", connectionCount1);
        VERIFY_ARE_EQUAL2(connectionCount1, 1);

        Trace.WriteInfo(TraceType, "sending at high priority");
        auto message2 = make_unique<Message>();
        message2->Headers.Add(ActionHeader(testAction));
        message2->Headers.Add(MessageIdHeader());
        client->SendOneWay(target, std::move(message2), TimeSpan::MaxValue, TransportPriority::High);
        VERIFY_IS_TRUE(messageReceived.WaitOne(TimeSpan::FromSeconds(30)));
        auto connectionCount2 = target->ConnectionCount();
        Trace.WriteInfo(TraceType, "connection count = {0}", connectionCount2);
        VERIFY_ARE_EQUAL2(connectionCount2, 2);

        VERIFY_IS_TRUE(TTestUtil::WaitUntil(
            [disconnected] { return disconnected->IsSet(); },
            securityConfig.SessionExpiration + TimeSpan::FromSeconds(5)));

        VERIFY_IS_TRUE(TTestUtil::WaitUntil(
            [disconnectedServer] { return disconnectedServer->IsSet(); },
            TimeSpan::FromSeconds(5)));

        // Now the old session should have been retried, and no new session is crreated as this is high priority connection
        VERIFY_IS_TRUE(
            TTestUtil::WaitUntil(
                [&]
                {
                    size_t connectionCount = target->ConnectionCount();
                    Trace.WriteInfo(TraceType, "client connection count = {0}, 0 is expected", connectionCount);
                    return connectionCount == 0;
                },
                TimeSpan::FromSeconds(3)));

        Trace.WriteInfo(TraceType, "the second expiration is not due yet");
        auto dcount = disconnectCount->load();
        Trace.WriteInfo(TraceType, "disconnect count = {0}", disconnectCount->load());
        VERIFY_IS_TRUE(dcount == 2);

        auto dcountServer = disconnectCountServer->load();
        Trace.WriteInfo(TraceType, "server disconnect count = {0}", disconnectCountServer->load());
        VERIFY_IS_TRUE(dcountServer == 2);

        auto connectionCount = target->ConnectionCount();
        Trace.WriteInfo(TraceType, "client connection count = {0}", connectionCount);
        VERIFY_IS_TRUE(connectionCount == 0);

        client->UnregisterDisconnectEvent(cde);
        server->UnregisterDisconnectEvent(sde);

        client->Stop();
        server->Stop();

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509_DisableAutoRefreshByConfig)
    {
        ENTER;

        SecurityConfig & securityConfig = SecurityConfig::GetConfig();
        auto savedSessionExpiration = securityConfig.SessionExpiration;
        securityConfig.SessionExpiration = TimeSpan::FromSeconds(5);
        auto saved2 = securityConfig.ReadyNewSessionBeforeExpiration;
        securityConfig.ReadyNewSessionBeforeExpiration = false;
        auto saved3 = securityConfig.SessionExpirationCloseDelay;
        securityConfig.SessionExpirationCloseDelay = TimeSpan::Zero;
        KFinally([&]
        {
            securityConfig.SessionExpiration = savedSessionExpiration;
            securityConfig.ReadyNewSessionBeforeExpiration = saved2;
            securityConfig.SessionExpirationCloseDelay = saved3;
        });

        auto client = DatagramTransportFactory::CreateTcpClient(L"client");
        auto disconnected = make_shared<AutoResetEvent>(false);
        auto disconnectCount = make_shared<atomic_uint64>(0);
        auto cde = client->RegisterDisconnectEvent(
            [disconnected, disconnectCount](const IDatagramTransport::DisconnectEventArgs& e)
        {
            auto dc = ++(*disconnectCount);
            Trace.WriteInfo(TraceType, "client: disconnected: target={0}, fault={1}, count = {2}", e.Target->TraceId(), e.Fault, dc);
            VERIFY_IS_TRUE(e.Fault.IsError(ErrorCodeValue::SecuritySessionExpiredByRemoteEnd) || e.Fault.IsError(SESSION_EXPIRATION_FAULT));
            if (dc == 2)
            {
                disconnected->Set();
            }
        });

        InstallTestCertInScope testCert;
        SecuritySettings securitySettings = TTestUtil::CreateX509SettingsTp(
            testCert.Thumbprint()->PrimaryToString(),
            L"",
            testCert.Thumbprint()->PrimaryToString(),
            L"");

        VERIFY_IS_TRUE(client->SetSecurity(securitySettings).IsSuccess());
        VERIFY_IS_TRUE(client->Start().IsSuccess());

        auto server = DatagramTransportFactory::CreateTcp(TTestUtil::GetListenAddress(), L"", L"server");
        VERIFY_IS_TRUE(server->SetSecurity(securitySettings).IsSuccess());

        AutoResetEvent messageReceived;
        wstring testAction = TTestUtil::GetGuidAction();
        TTestUtil::SetMessageHandler(
            server,
            testAction,
            [&server, &client, &messageReceived](MessageUPtr &, ISendTarget::SPtr const & st) -> void
        {
            Trace.WriteInfo(TraceType, "receiver got a message from {0}", st->Address());
            messageReceived.Set();
        });

        auto disconnectedServer = make_shared<AutoResetEvent>(false);
        auto disconnectCountServer = make_shared<atomic_uint64>(0);
        auto sde = server->RegisterDisconnectEvent([disconnectedServer, disconnectCountServer](const IDatagramTransport::DisconnectEventArgs& e)
        {
            auto dc = ++(*disconnectCountServer);
            Trace.WriteInfo(TraceType, "server: disconnected: target={0}, fault={1}, count = {2}", e.Target->TraceId(), e.Fault, dc);
            if (dc == 2)
            {
                disconnectedServer->Set();
            }
        });

        VERIFY_IS_TRUE(server->Start().IsSuccess());

        ISendTarget::SPtr target = client->ResolveTarget(server->ListenAddress());
        VERIFY_IS_TRUE(target);

        auto message1 = make_unique<Message>();
        message1->Headers.Add(ActionHeader(testAction));
        message1->Headers.Add(MessageIdHeader());

        Trace.WriteInfo(TraceType, "sending at normal priority");
        client->SendOneWay(target, std::move(message1), TimeSpan::MaxValue, TransportPriority::Normal);
        VERIFY_IS_TRUE(messageReceived.WaitOne(TimeSpan::FromSeconds(30)));
        auto connectionCount1 = target->ConnectionCount();
        Trace.WriteInfo(TraceType, "connection count = {0}", connectionCount1);
        VERIFY_ARE_EQUAL2(connectionCount1, 1);

        Trace.WriteInfo(TraceType, "sending at high priority");
        auto message2 = make_unique<Message>();
        message2->Headers.Add(ActionHeader(testAction));
        message2->Headers.Add(MessageIdHeader());
        client->SendOneWay(target, std::move(message2), TimeSpan::MaxValue, TransportPriority::High);
        VERIFY_IS_TRUE(messageReceived.WaitOne(TimeSpan::FromSeconds(30)));
        auto connectionCount2 = target->ConnectionCount();
        Trace.WriteInfo(TraceType, "connection count = {0}", connectionCount2);
        VERIFY_ARE_EQUAL2(connectionCount2, 2);

        VERIFY_IS_TRUE(TTestUtil::WaitUntil(
            [disconnected] { return disconnected->IsSet(); },
            securityConfig.SessionExpiration + TimeSpan::FromSeconds(5)));

        VERIFY_IS_TRUE(TTestUtil::WaitUntil(
            [disconnectedServer] { return disconnectedServer->IsSet(); },
            TimeSpan::FromSeconds(5)));

        // Now the old session should have been retried, and no new session is crreated as this is high priority connection
        VERIFY_IS_TRUE(
            TTestUtil::WaitUntil(
                [&]
        {
            size_t connectionCount = target->ConnectionCount();
            Trace.WriteInfo(TraceType, "client connection count = {0}, 0 is expected", connectionCount);
            return connectionCount == 0;
        },
            TimeSpan::FromSeconds(3)));

        Trace.WriteInfo(TraceType, "the second expiration is not due yet");
        auto dcount = disconnectCount->load();
        Trace.WriteInfo(TraceType, "disconnect count = {0}", disconnectCount->load());
        VERIFY_IS_TRUE(dcount == 2);

        auto dcountServer = disconnectCountServer->load();
        Trace.WriteInfo(TraceType, "server disconnect count = {0}", disconnectCountServer->load());
        VERIFY_IS_TRUE(dcountServer == 2);

        auto connectionCount = target->ConnectionCount();
        Trace.WriteInfo(TraceType, "client connection count = {0}", connectionCount);
        VERIFY_IS_TRUE(connectionCount == 0);

        client->UnregisterDisconnectEvent(cde);
        server->UnregisterDisconnectEvent(sde);

        client->Stop();
        server->Stop();

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509_DisableAutoRefresh_ReplyAtExpiration)
    {
        ENTER;

        SecurityConfig & securityConfig = SecurityConfig::GetConfig();
        auto savedSessionExpiration = securityConfig.SessionExpiration;
        securityConfig.SessionExpiration = TimeSpan::FromSeconds(3);
        auto saved2 = securityConfig.SessionExpirationCloseDelay;
        securityConfig.SessionExpirationCloseDelay = TimeSpan::FromSeconds(3);
        auto saved3 = TransportConfig::GetConfig().CloseDrainTimeout;
        TransportConfig::GetConfig().CloseDrainTimeout = TimeSpan::Zero;
        KFinally([&]
        {
            securityConfig.SessionExpiration = savedSessionExpiration;
            securityConfig.SessionExpirationCloseDelay = saved2;
            TransportConfig::GetConfig().CloseDrainTimeout = saved3;
        });

        auto client = DatagramTransportFactory::CreateTcpClient(L"client");
        auto disconnected = make_shared<AutoResetEvent>(false);
        auto disconnectCount = make_shared<atomic_uint64>(0);
        auto cde = client->RegisterDisconnectEvent([disconnected, disconnectCount](const IDatagramTransport::DisconnectEventArgs& e)
        {
            auto dc = ++(*disconnectCount);
            Trace.WriteInfo(TraceType, "client: disconnected: target={0}, fault={1}, count = {2}", e.Target->TraceId(), e.Fault, dc);
            VERIFY_IS_TRUE(e.Fault.IsError(ErrorCodeValue::SecuritySessionExpiredByRemoteEnd) || e.Fault.IsError(SESSION_EXPIRATION_FAULT));
            disconnected->Set();
        });

        InstallTestCertInScope testCert;
        SecuritySettings securitySettings = TTestUtil::CreateX509SettingsTp(
            testCert.Thumbprint()->PrimaryToString(),
            L"",
            testCert.Thumbprint()->PrimaryToString(),
            L"");

        securitySettings.SetReadyNewSessionBeforeExpirationCallback([] { return false; });
        VERIFY_IS_TRUE(client->SetSecurity(securitySettings).IsSuccess());

        AutoResetEvent replyReceived;
        wstring testAction = TTestUtil::GetGuidAction();
        TTestUtil::SetMessageHandler(
            client,
            testAction,
            [&replyReceived](MessageUPtr &, ISendTarget::SPtr const &) -> void { replyReceived.Set(); });

        VERIFY_IS_TRUE(client->Start().IsSuccess());

        auto server = DatagramTransportFactory::CreateTcp(TTestUtil::GetListenAddress(), L"", L"server");
        VERIFY_IS_TRUE(server->SetSecurity(securitySettings).IsSuccess());

        TTestUtil::SetMessageHandler(
            server,
            testAction,
            [server, testAction](MessageUPtr &, ISendTarget::SPtr const & st) -> void
            {
                Trace.WriteInfo(TraceType, "server got a message from {0}", st->Address());
                Threadpool::Post(
                    [server, st, testAction]
                    {
                        auto reply = make_unique<Message>();
                        reply->Headers.Add(ActionHeader(testAction));
                        reply->Headers.Add(MessageIdHeader());
                        Trace.WriteInfo(TraceType, "server: sending reply after session expiration");
                        server->SendOneWay(st, move(reply));
                    },
                    SecurityConfig::GetConfig().SessionExpiration + TimeSpan::FromMilliseconds(100));
            });

        auto disconnectedServer = make_shared<AutoResetEvent>(false);
        auto disconnectCountServer = make_shared<atomic_uint64>(0);
        auto sde = server->RegisterDisconnectEvent([disconnectedServer, disconnectCountServer](const IDatagramTransport::DisconnectEventArgs& e)
        {
            auto dc = ++(*disconnectCountServer);
            Trace.WriteInfo(TraceType, "server: disconnected: target={0}, fault={1}, count = {2}", e.Target->TraceId(), e.Fault, dc);
            disconnectedServer->Set();
        });

        VERIFY_IS_TRUE(server->Start().IsSuccess());

        ISendTarget::SPtr target = client->ResolveTarget(server->ListenAddress());
        VERIFY_IS_TRUE(target);

        auto message1 = make_unique<Message>();
        message1->Headers.Add(ActionHeader(testAction));
        message1->Headers.Add(MessageIdHeader());

        Trace.WriteInfo(TraceType, "sending request");
        client->SendOneWay(target, std::move(message1));
        Trace.WriteInfo(TraceType, "waiting for reply to come back");
        VERIFY_IS_TRUE(replyReceived.WaitOne(TimeSpan::FromSeconds(10)));

        VERIFY_IS_TRUE(disconnected->IsSet());
        auto connectionCount = target->ConnectionCount();
        Trace.WriteInfo(TraceType, "client connection count = {0}", connectionCount);
        VERIFY_IS_TRUE(connectionCount == 0);

        Trace.WriteInfo(TraceType, "server side disconnect should happen by SessionExpirationCloseDelay");
        VERIFY_IS_TRUE(
            TTestUtil::WaitUntil(
                [disconnectedServer] { return disconnectedServer->IsSet(); },
                securityConfig.SessionExpirationCloseDelay + TimeSpan::FromSeconds(2)));

        client->UnregisterDisconnectEvent(cde);
        server->UnregisterDisconnectEvent(sde);

        client->Stop();
        server->Stop();

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509_DisableAutoRefresh_ReplyAroundSecurityUpdate)
    {
        ENTER;

        SecurityConfig & securityConfig = SecurityConfig::GetConfig();
        auto saved1 = securityConfig.MaxSessionRefreshDelay;
        securityConfig.MaxSessionRefreshDelay = TimeSpan::FromSeconds(3);
        auto saved2 = securityConfig.SessionExpirationCloseDelay;
        securityConfig.SessionExpirationCloseDelay = TimeSpan::FromSeconds(3);
        auto saved3 = TransportConfig::GetConfig().CloseDrainTimeout;
        TransportConfig::GetConfig().CloseDrainTimeout = TimeSpan::Zero;
        KFinally([&]
        {
            securityConfig.MaxSessionRefreshDelay = saved1;
            securityConfig.SessionExpirationCloseDelay = saved2;
            TransportConfig::GetConfig().CloseDrainTimeout = saved3;
        });

        auto client = DatagramTransportFactory::CreateTcpClient(L"client");
        auto disconnected = make_shared<AutoResetEvent>(false);
        auto disconnectCount = make_shared<atomic_uint64>(0);
        auto cde = client->RegisterDisconnectEvent([disconnected, disconnectCount](const IDatagramTransport::DisconnectEventArgs& e)
        {
            auto dc = ++(*disconnectCount);
            Trace.WriteInfo(TraceType, "client: disconnected: target={0}, fault={1}, count = {2}", e.Target->TraceId(), e.Fault, dc);
            VERIFY_IS_TRUE(e.Fault.IsError(ErrorCodeValue::SecuritySessionExpiredByRemoteEnd) || e.Fault.IsError(SESSION_EXPIRATION_FAULT));
            disconnected->Set();
        });

        InstallTestCertInScope testCert;
        SecuritySettings securitySettings = TTestUtil::CreateX509SettingsTp(
            testCert.Thumbprint()->PrimaryToString(),
            L"",
            testCert.Thumbprint()->PrimaryToString(),
            L"");

        securitySettings.SetReadyNewSessionBeforeExpirationCallback([] { return false; });
        VERIFY_IS_TRUE(client->SetSecurity(securitySettings).IsSuccess());

        AutoResetEvent replyReceived;
        wstring testAction = TTestUtil::GetGuidAction();
        TTestUtil::SetMessageHandler(
            client,
            testAction,
            [&replyReceived](MessageUPtr &, ISendTarget::SPtr const &) -> void { replyReceived.Set(); });

        VERIFY_IS_TRUE(client->Start().IsSuccess());

        auto server = DatagramTransportFactory::CreateTcp(TTestUtil::GetListenAddress(), L"", L"server");
        VERIFY_IS_TRUE(server->SetSecurity(securitySettings).IsSuccess());
        Trace.WriteInfo(TraceType, "server side session expiration: {0}", securitySettings.SessionDuration());

        TTestUtil::SetMessageHandler(
            server,
            testAction,
            [server, testAction](MessageUPtr &, ISendTarget::SPtr const & st) -> void
            {
                Trace.WriteInfo(TraceType, "server got a message from {0}", st->Address());
                Threadpool::Post(
                    [server, st, testAction]
                    {
                        auto reply = make_unique<Message>();
                        reply->Headers.Add(ActionHeader(testAction));
                        reply->Headers.Add(MessageIdHeader());
                        Trace.WriteInfo(TraceType, "server: sending reply after session expiration");
                        server->SendOneWay(st, move(reply));
                    },
                    SecurityConfig::GetConfig().MaxSessionRefreshDelay /*wait for session expiration caused by security configuration update*/);
            });

        auto disconnectedServer = make_shared<AutoResetEvent>(false);
        auto disconnectCountServer = make_shared<atomic_uint64>(0);
        auto sde = server->RegisterDisconnectEvent([disconnectedServer, disconnectCountServer](const IDatagramTransport::DisconnectEventArgs& e)
        {
            auto dc = ++(*disconnectCountServer);
            Trace.WriteInfo(TraceType, "server: disconnected: target={0}, fault={1}, count = {2}", e.Target->TraceId(), e.Fault, dc);
            disconnectedServer->Set();
        });

        VERIFY_IS_TRUE(server->Start().IsSuccess());

        ISendTarget::SPtr target = client->ResolveTarget(server->ListenAddress());
        VERIFY_IS_TRUE(target);

        auto message1 = make_unique<Message>();
        message1->Headers.Add(ActionHeader(testAction));
        message1->Headers.Add(MessageIdHeader());

        Trace.WriteInfo(TraceType, "sending request");
        client->SendOneWay(target, std::move(message1));

        // wait for connection to be established and request to arrive at server
        // also, server side session expiration caused by security configuration update
        // is in a ragne [0, MaxSessionRefreshDelay], we need some safety margin so 
        // that server side does not delay reply too much
        Sleep(500);

        Trace.WriteInfo(TraceType, "simulate server side security configuration update to trigger session expiration");
        VERIFY_IS_TRUE(server->SetSecurity(securitySettings).IsSuccess());

        Trace.WriteInfo(TraceType, "waiting for reply to come back");
        VERIFY_IS_TRUE(replyReceived.WaitOne(TimeSpan::FromSeconds(10)));

        Trace.WriteInfo(TraceType, "server side disconnect should happen by SessionExpirationCloseDelay");
        VERIFY_IS_TRUE(
            TTestUtil::WaitUntil(
                [disconnectedServer] { return disconnectedServer->IsSet(); },
                securityConfig.SessionExpirationCloseDelay + TimeSpan::FromSeconds(2)));

        //client side has longer session expiration, thus its disconnect should happen after server side closes the connection
        VERIFY_IS_TRUE(
            TTestUtil::WaitUntil(
                [disconnected] { return disconnected->IsSet(); },
                securityConfig.SessionExpirationCloseDelay));

        client->UnregisterDisconnectEvent(cde);
        server->UnregisterDisconnectEvent(sde);

        client->Stop();
        server->Stop();

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509_DisableExpiration)
    {
        ENTER;

        auto client = DatagramTransportFactory::CreateTcpClient(L"client");
        auto disconnected = make_shared<AutoResetEvent>(false);
        auto cde = client->RegisterDisconnectEvent(
            [disconnected](const IDatagramTransport::DisconnectEventArgs& e)
            {
                Trace.WriteInfo(TraceType, "client: disconnected: target={0}, fault={1}", e.Target->TraceId(), e.Fault);
                disconnected->Set();
            });

        InstallTestCertInScope testCert;
        SecuritySettings cltSecuritySettings = TTestUtil::CreateX509SettingsTp(
            testCert.Thumbprint()->PrimaryToString(),
            L"",
            testCert.Thumbprint()->PrimaryToString(),
            L"");

        auto svrSecuritySettings = cltSecuritySettings;

        cltSecuritySettings.SetSessionDurationCallback([] { return TimeSpan::MaxValue; });
        VERIFY_IS_TRUE(client->SetSecurity(cltSecuritySettings).IsSuccess());
        VERIFY_IS_TRUE(client->Start().IsSuccess());

        svrSecuritySettings.SetSessionDurationCallback([] { return TimeSpan::Zero; });
        auto server = DatagramTransportFactory::CreateTcp(TTestUtil::GetListenAddress(), L"", L"server");
        VERIFY_IS_TRUE(server->SetSecurity(svrSecuritySettings).IsSuccess());

        AutoResetEvent messageReceived;
        wstring testAction = TTestUtil::GetGuidAction();
        TTestUtil::SetMessageHandler(
            server,
            testAction,
            [&server, &client, &messageReceived](MessageUPtr &, ISendTarget::SPtr const & st) -> void
            {
                Trace.WriteInfo(TraceType, "receiver got a message from {0}", st->Address());
                messageReceived.Set();
            });

        VERIFY_IS_TRUE(server->Start().IsSuccess());

        ISendTarget::SPtr target = client->ResolveTarget(server->ListenAddress());
        VERIFY_IS_TRUE(target);

        auto message1 = make_unique<Message>();
        message1->Headers.Add(ActionHeader(testAction));
        message1->Headers.Add(MessageIdHeader());

        Trace.WriteInfo(TraceType, "sending a message");
        client->SendOneWay(target, std::move(message1), TimeSpan::MaxValue, TransportPriority::Normal);
        VERIFY_IS_TRUE(messageReceived.WaitOne(TimeSpan::FromSeconds(30)));

        VERIFY_IS_FALSE(
            TTestUtil::WaitUntil([disconnected] { return disconnected->IsSet(); },
            TimeSpan::FromSeconds(3)));

        client->UnregisterDisconnectEvent(cde);

        client->Stop();
        server->Stop();

        LEAVE;
    }

    BOOST_AUTO_TEST_SUITE_END()

    void SessionExpirationTestBase::RequestReplyAtSessionExpiration(SecuritySettings const & securitySettings)
    {
        TimeSpan savedSessionExpiration = SecurityConfig::GetConfig().SessionExpiration;
        SecurityConfig::GetConfig().SessionExpiration =
            SecurityProvider::IsWindowsProvider(securitySettings.SecurityProvider()) ?
            TimeSpan::FromSeconds(30) //talking to AD may take long
            : TimeSpan::FromSeconds(6);
        TimeSpan savedDefaultCloseDelay = SecurityConfig::GetConfig().SessionExpirationCloseDelay;
        SecurityConfig::GetConfig().SessionExpirationCloseDelay = TimeSpan::FromSeconds(6);
        auto saved3 = TransportConfig::GetConfig().CloseDrainTimeout;
        TransportConfig::GetConfig().CloseDrainTimeout = TimeSpan::FromSeconds(1);
        KFinally([=]
        {
            SecurityConfig::GetConfig().SessionExpiration = savedSessionExpiration;
            SecurityConfig::GetConfig().SessionExpirationCloseDelay = savedDefaultCloseDelay;
            TransportConfig::GetConfig().CloseDrainTimeout = saved3;
        });

        auto testRoot = make_shared<RequestReplyTestRoot>();
        testRoot->client = DatagramTransportFactory::CreateTcpClient();
        testRoot->requestReply = make_shared<RequestReply>(*testRoot, testRoot->client);

        auto client = testRoot->client;
        VERIFY_IS_TRUE(client->SetSecurity(securitySettings).IsSuccess());

        client->SetMessageHandler([testRoot](MessageUPtr & message, ISendTarget::SPtr const & sender)
        {
            Trace.WriteInfo(TraceType, "client: received message {0} from {1}", message->TraceId(), sender->Address());
            testRoot->requestReply->OnReplyMessage(*message, sender);
        });

        auto sessionExpired = make_shared<AutoResetEvent>();
        client->SetConnectionFaultHandler([sessionExpired](ISendTarget const &, ErrorCode fault)
        {
            Trace.WriteInfo(TraceType, "client: connection faulted: {0}", fault);
            VERIFY_IS_TRUE(fault.IsError(SESSION_EXPIRATION_FAULT));
            sessionExpired->Set();
        });

        testRoot->requestReply->Open();
        VERIFY_IS_TRUE(client->Start().IsSuccess());

        auto server = DatagramTransportFactory::CreateTcp(TTestUtil::GetListenAddress());
        VERIFY_IS_TRUE(server->SetSecurity(securitySettings).IsSuccess());

        // Client side refreshes connection at SecurityConfig::GetConfig().SessionExpiration, and then the old
        // connection is "removed" for sending after the new connection is ready. We want the reply to be sent
        // out after client side has "removed" the connection.
        auto replyDelay = SecurityConfig::GetConfig().SessionExpiration + TimeSpan::FromSeconds(2);
        wstring testAction = TTestUtil::GetGuidAction();
        TTestUtil::SetMessageHandler(
            server,
            testAction,
            [&server, replyDelay, testAction](MessageUPtr & request, ISendTarget::SPtr const & st) -> void
            {
                Trace.WriteInfo(TraceType, "server received request {0}, wait {1} before sending reply", request->TraceId(), replyDelay);
                auto requestId = request->MessageId;
                Threadpool::Post([requestId, st, testAction, &server]
                    {
                        Trace.WriteInfo(TraceType, "server sending reply");
                        auto reply = make_unique<Message>();
                        reply->Headers.Add(RelatesToHeader(requestId));
                        reply->Headers.Add(ActionHeader(testAction));
                        auto error = server->SendOneWay(st, move(reply));
                        VERIFY_IS_TRUE(error.IsSuccess());
                    },
                    replyDelay);
            });

        VERIFY_IS_TRUE(server->Start().IsSuccess());

        ISendTarget::SPtr serverTarget = client->ResolveTarget(
            server->ListenAddress(),
            L"",
            (securitySettings.SecurityProvider() == SecurityProvider::Negotiate) ? TransportSecurity().LocalWindowsIdentity() : L"");

        VERIFY_IS_TRUE(serverTarget);

        Trace.WriteInfo(TraceType, "client sending request");
        auto msg = make_unique<Message>();
        msg->Headers.Add(ActionHeader(testAction));
        msg->Headers.Add(MessageIdHeader());

        auto requestCompleted = make_shared<AutoResetEvent>(false);
        auto requestOp = testRoot->requestReply->BeginRequest(
            move(msg),
            serverTarget,
            replyDelay + TimeSpan::FromSeconds(10),
            [=](AsyncOperationSPtr const &)
            {
                Trace.WriteInfo(TraceType, "request operation completed");
                requestCompleted->Set();
            },
            testRoot->CreateAsyncOperationRoot());

        auto verifyWait = SecurityConfig::GetConfig().SessionExpiration +
            SecurityConfig::GetConfig().SessionExpirationCloseDelay +
            TransportConfig::GetConfig().CloseDrainTimeout +
            TimeSpan::FromSeconds(30);

        Trace.WriteInfo(TraceType, "waiting for session expiration");
        VERIFY_IS_TRUE(sessionExpired->WaitOne(verifyWait));
        Trace.WriteInfo(TraceType, "session expiration verified");

        Trace.WriteInfo(TraceType, "client waiting for reply");
        VERIFY_IS_TRUE(requestCompleted->WaitOne());

        MessageUPtr reply;
        ErrorCode endRequestErrorCode = testRoot->requestReply->EndRequest(requestOp, reply);
        VERIFY_IS_TRUE(endRequestErrorCode.IsSuccess());
        VERIFY_IS_TRUE(reply);

        client->Stop();
        server->Stop();
    }

    void SessionExpirationTestBase::ReliabilityTest_ClientServer(SecuritySettings const & securitySettings)
    {
        auto& sconfig = SecurityConfig::GetConfig();
        auto savedSessionExpiration = sconfig.SessionExpiration;
        sconfig.SessionExpiration =
            (securitySettings.SecurityProvider() == SecurityProvider::Negotiate) ?
            TimeSpan::FromSeconds(30) : //talking to AD may take long
            TimeSpan::FromSeconds(3);

        auto saved2 = sconfig.SessionExpirationCloseDelay;
        sconfig.SessionExpirationCloseDelay = TimeSpan::FromSeconds(10);

        KFinally([&]
        {
            sconfig.SessionExpiration = savedSessionExpiration;
            sconfig.SessionExpirationCloseDelay = saved2;
        });

        TimeSpan sendDuration =
            SecurityProvider::IsWindowsProvider(securitySettings.SecurityProvider()) ? TimeSpan::FromSeconds(190) : TimeSpan::FromSeconds(160);
        VERIFY_IS_TRUE(sendDuration.TotalMilliseconds() > 3.0*(sconfig.SessionExpiration.TotalMilliseconds() + sconfig.SessionExpirationCloseDelay.TotalMilliseconds()));
        auto& tconfig = TransportConfig::GetConfig();
        VERIFY_IS_TRUE(sendDuration > (sconfig.SessionExpiration + sconfig.SessionExpirationCloseDelay + tconfig.CloseDrainTimeout));

        auto client = DatagramTransportFactory::CreateTcpClient(TTestUtil::GetListenAddress(), L"client");
        VERIFY_IS_TRUE(client->SetSecurity(securitySettings).IsSuccess());
        Common::atomic_long clientReceiveCount(0);
        wstring testAction = TTestUtil::GetGuidAction();
        TTestUtil::SetMessageHandler(
            client,
            testAction,
            [&clientReceiveCount](MessageUPtr &, ISendTarget::SPtr const &) -> void { ++clientReceiveCount; });

        VERIFY_IS_TRUE(client->Start().IsSuccess());

        auto server = DatagramTransportFactory::CreateTcp(TTestUtil::GetListenAddress(), L"", L"server");
        server->SetPerTargetSendQueueLimit(0); // disable send queue limit
        VERIFY_IS_TRUE(server->SetSecurity(securitySettings).IsSuccess());

        Common::atomic_long serverReceiveCount(0);
        TTestUtil::SetMessageHandler(
            server,
            testAction,
            [testAction, &serverReceiveCount, &server](MessageUPtr &, ISendTarget::SPtr const & st) -> void
            {
                ++serverReceiveCount;
                auto reply = make_unique<Message>();
                reply->Headers.Add(ActionHeader(testAction));
                reply->Headers.Add(MessageIdHeader());

                //a large number of messages are sent and received in this test,
                //use Invariant instead of boost test macros to avoid excessive output 
                //at boost log level 'all', which slows down the test too much.
                Invariant(server->SendOneWay(st, move(reply)).IsSuccess());
            });

        VERIFY_IS_TRUE(server->Start().IsSuccess());

        ISendTarget::SPtr receiverTarget = client->ResolveTarget(server->ListenAddress(), L"", TransportSecurity().LocalWindowsIdentity());
        VERIFY_IS_TRUE(receiverTarget);

        LONG sentCount = 0;
        Stopwatch stopwatch;
        stopwatch.Start();
        MessageId lastMessageSent;

        Trace.WriteInfo(TraceType, "client starts sending");
        constexpr size_t pendingLimit = 10000;
        while (stopwatch.Elapsed < sendDuration)
        {
            if (receiverTarget->MessagesPendingForSend() < pendingLimit)
            {
                auto msg = make_unique<Message>();
                msg->Headers.Add(ActionHeader(testAction));
                msg->Headers.Add(MessageIdHeader());
                lastMessageSent = msg->MessageId;

                //a large number of messages are sent and received in this test,
                //use Invariant instead of boost test macros to avoid excessive output 
                //at boost log level 'all', which slows down the test too much.
                Invariant(client->SendOneWay(receiverTarget, move(msg)).IsSuccess());
                ++sentCount;
            }
            this_thread::yield();
        }
        Trace.WriteInfo(TraceType, "client sentCount = {0}, last sent by client: {1}", sentCount, lastMessageSent);

        for (int i = 0; i < 200; ++i)
        {
            if ((clientReceiveCount.load() == sentCount) && (serverReceiveCount.load() == sentCount))
            {
                break;
            }

            ::Sleep(300);
        }

        Trace.WriteInfo(TraceType, "clientReceiveCount = {0}, serverReceiveCount = {1}", clientReceiveCount.load(), serverReceiveCount.load());

        VERIFY_ARE_EQUAL2(sentCount, clientReceiveCount.load());
        VERIFY_ARE_EQUAL2(sentCount, serverReceiveCount.load());

        client->Stop();
        server->Stop();
    }

    void SessionExpirationTestBase::ReliabilityTest_NodeToNode(SecuritySettings const & securitySettings, bool leaseMode)
    {
        auto& sconfig = SecurityConfig::GetConfig();
        auto savedSessionExpiration = sconfig.SessionExpiration;
        sconfig.SessionExpiration =
            (securitySettings.SecurityProvider() == SecurityProvider::Negotiate) ?
            TimeSpan::FromSeconds(30) : //talking to AD may take long
            TimeSpan::FromSeconds(3);

        auto saved2 = sconfig.SessionExpirationCloseDelay;
        sconfig.SessionExpirationCloseDelay = TimeSpan::FromSeconds(10);

        KFinally([&]
        {
            sconfig.SessionExpiration = savedSessionExpiration;
            sconfig.SessionExpirationCloseDelay = saved2;
        });

        TimeSpan sendDuration =
            SecurityProvider::IsWindowsProvider(securitySettings.SecurityProvider()) ? TimeSpan::FromSeconds(190) : TimeSpan::FromSeconds(160);
        VERIFY_IS_TRUE(sendDuration.TotalMilliseconds() > 3.0*(sconfig.SessionExpiration.TotalMilliseconds() + sconfig.SessionExpirationCloseDelay.TotalMilliseconds()));
        auto& tconfig = TransportConfig::GetConfig();
        VERIFY_IS_TRUE(sendDuration > (sconfig.SessionExpiration + sconfig.SessionExpirationCloseDelay + tconfig.CloseDrainTimeout));

        auto node1 = DatagramTransportFactory::CreateTcp(TTestUtil::GetListenAddress(), L"", L"node1");
        VERIFY_IS_TRUE(node1->SetSecurity(securitySettings).IsSuccess());
        Common::atomic_long node1ReceiveCount(0);
        node1->SetMessageHandler([&node1ReceiveCount](MessageUPtr & msg, ISendTarget::SPtr const &) -> void
        {
            vector<const_buffer> buffers;

            //a large number of messages are sent and received in this test,
            //use Invariant instead of boost test macros to avoid excessive output 
            //at boost log level 'all', which slows down the test too much.
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
            Trace.WriteInfo(TraceType, "node1 receive message {0}", testLeaseMessage->MessageIdentifier.QuadPart);

            //a large number of messages are sent and received in this test,
            //use Invariant instead of boost test macros to avoid excessive output 
            //at boost log level 'all', which slows down the test too much.
            Invariant(testLeaseMessage->Verify());

            ++node1ReceiveCount;
        });

        if (leaseMode)
        {
#ifdef PLATFORM_UNIX
            node1->DisableListenInstanceMessage();
            node1->SetBufferFactory(make_unique<LTBufferFactory>());
#else
            Assert::CodingError("not implemented");
#endif
        }

        VERIFY_IS_TRUE(node1->Start().IsSuccess());

        auto node2 = DatagramTransportFactory::CreateTcp(TTestUtil::GetListenAddress(), L"", L"node2");
        node2->SetPerTargetSendQueueLimit(0); // disable send queue limit
        VERIFY_IS_TRUE(node2->SetSecurity(securitySettings).IsSuccess());

        Common::atomic_long node2ReceiveCount(0);
        node2->SetMessageHandler([&node2ReceiveCount](MessageUPtr & msg, ISendTarget::SPtr const &) -> void
        {
            vector<const_buffer> buffers;

            //a large number of messages are sent and received in this test,
            //use Invariant instead of boost test macros to avoid excessive output 
            //at boost log level 'all', which slows down the test too much.
            Invariant(msg->GetBody(buffers));

            size_t totalBytes = 0;
            for(auto const & buffer : buffers)
            {
                totalBytes += buffer.size();
            }

            auto buf = new BYTE[totalBytes];
            KFinally([buf] { delete buf; });

            auto ptr = buf;
            for(auto const & buffer : buffers)
            {
                memcpy(ptr, buffer.buf, buffer.len);
                ptr += buffer.len;
            }

            auto testLeaseMessage = (TestLeaseMessage*)buf;
            Trace.WriteInfo(TraceType, "node2 receive message {0}", testLeaseMessage->MessageIdentifier.QuadPart);

            //a large number of messages are sent and received in this test,
            //use Invariant instead of boost test macros to avoid excessive output 
            //at boost log level 'all', which slows down the test too much.
            Invariant(testLeaseMessage->Verify());

            ++node2ReceiveCount;
        });

        if (leaseMode)
        {
#ifdef PLATFORM_UNIX
            node2->DisableListenInstanceMessage();
            node2->SetBufferFactory(make_unique<LTBufferFactory>());
#else
            Assert::CodingError("not implemented");
#endif
        }

        VERIFY_IS_TRUE(node2->Start().IsSuccess());

        auto target1 = node2->ResolveTarget(node1->ListenAddress(), L"", TransportSecurity().LocalWindowsIdentity());
        auto target2 = node1->ResolveTarget(node2->ListenAddress(), L"", TransportSecurity().LocalWindowsIdentity());

        int sentCount1 = 0;
        int sentCount2 = 0;

        std::thread t1([&] { ReliabilityTest_PeerToPeer_Send(node1, target2, sendDuration, sentCount1); });
        std::thread t2([&] { ReliabilityTest_PeerToPeer_Send(node2, target1, sendDuration, sentCount2); });

        t1.join();
        t2.join();

        for (int i = 0; i < 200; ++i)
        {
            if ((node1ReceiveCount.load() == sentCount2) && (node2ReceiveCount.load() == sentCount1))
            {
                break;
            }

            ::Sleep(300);
        }

        Trace.WriteInfo(TraceType, "node1ReceiveCount = {0}, node2ReceiveCount = {1}", node1ReceiveCount.load(), node2ReceiveCount.load());

        VERIFY_ARE_EQUAL2(sentCount2, node1ReceiveCount.load());
        VERIFY_ARE_EQUAL2(sentCount1, node2ReceiveCount.load());

        node1->Stop();
        node2->Stop();
    }

    void SessionExpirationTestBase::ReliabilityTest_PeerToPeer_Send(
        IDatagramTransportSPtr const & node,
        ISendTarget::SPtr const & target,
        TimeSpan sendDuration,
        int & sentCount)
    {
        Trace.WriteInfo(TraceType, "{0} start sending to {1}", node->TraceId(), target->TraceId());

        Stopwatch stopwatch;
        stopwatch.Start();

        sentCount = 0;
        constexpr size_t pendingLimit = 1000;
        uint64 msgId = 1;
        uint64 lastMessageSent = msgId;
        while (stopwatch.Elapsed < sendDuration)
        {
            if (target->MessagesPendingForSend() < pendingLimit)
            {
                auto testLeaseMessage = make_shared<TestLeaseMessage>(msgId);
                vector<const_buffer> bufferList(1, const_buffer(testLeaseMessage.get(), sizeof(*testLeaseMessage)));
                auto msg = make_unique<Message>(
                    bufferList,
                    [testMsg = move(testLeaseMessage)] (std::vector<Common::const_buffer> const &, void*) {},
                    nullptr);

                Trace.WriteInfo(TraceType, "node {0} sending {1}", node->TraceId(), msgId); 
                lastMessageSent = msgId++;
                auto error = node->SendOneWay(target, move(msg));

                //a large number of messages are sent and received in this test,
                //use Invariant instead of boost test macros to avoid excessive output 
                //at boost log level 'all', which slows down the test too much.
                Invariant(error.IsSuccess());
                ++sentCount;
            }
            this_thread::yield();
        }

        Trace.WriteInfo(TraceType, "sentCount = {0}, last sent by sender: {1}", sentCount, lastMessageSent);
    }

    void SessionExpirationTestBase::ConnectionSurvivesRefreshFailure(SecuritySettings const & securitySettings)
    {
        SecurityConfig & securityConfig = SecurityConfig::GetConfig();
        auto saved1 = securityConfig.SessionExpiration;
        securityConfig.SessionExpiration = TimeSpan::FromSeconds(3);
        auto saved2 = securityConfig.SessionRefreshRetryDelay;
        SecurityConfig::GetConfig().SessionRefreshRetryDelay = TimeSpan::FromSeconds(1);
        auto saved3 = securityConfig.SessionRefreshTimeout;
        SecurityConfig::GetConfig().SessionRefreshTimeout = TimeSpan::FromSeconds(1);

        KFinally([&]
        {
            securityConfig.SessionExpiration = saved1;
            securityConfig.SessionRefreshRetryDelay = saved2;
            securityConfig.SessionRefreshTimeout = saved3;
        });

        auto node1 = DatagramTransportFactory::CreateTcp(TTestUtil::GetListenAddress(), L"", L"node1");
        IConnection const * firstConnection = nullptr;
        auto de1 = node1->RegisterDisconnectEvent([&firstConnection](const IDatagramTransport::DisconnectEventArgs& e)
        {
            Trace.WriteInfo(TraceType, "node1: disconnected: target={0}, fault={1}", e.Target->TraceId(), e.Fault);
            VERIFY_IS_FALSE(e.Fault.IsError(SESSION_EXPIRATION_FAULT));
            VERIFY_IS_FALSE(e.Fault.IsError(ErrorCodeValue::SecuritySessionExpiredByRemoteEnd));
            auto currentConnection = ((TcpSendTarget&)(*e.Target)).Test_ActiveConnection();
            Trace.WriteInfo(TraceType, "firstConnection = {0}, currentConnection = {1}", TextTracePtr(firstConnection), TextTracePtr(currentConnection));
            VERIFY_IS_TRUE(currentConnection == firstConnection);
        });

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
                Trace.WriteInfo(TraceType, "receiver got a message from {0}", st->Address());
                messageReceived.Set();
            });

        auto disconnected = make_shared<ManualResetEvent>(false);
        auto de2 = node2->RegisterDisconnectEvent([disconnected](const IDatagramTransport::DisconnectEventArgs& e)
        {
            Trace.WriteInfo(TraceType, "node2: disconnected: target={0}, fault={1}", e.Target->TraceId(), e.Fault);
            VERIFY_IS_FALSE(e.Fault.IsError(SESSION_EXPIRATION_FAULT));
            VERIFY_IS_FALSE(e.Fault.IsError(ErrorCodeValue::SecuritySessionExpiredByRemoteEnd));
        });

        VERIFY_IS_TRUE(node2->Start().IsSuccess());

        ISendTarget::SPtr target = node1->ResolveTarget(node2->ListenAddress());
        VERIFY_IS_TRUE(target);

        auto message1 = make_unique<Message>();
        message1->Headers.Add(ActionHeader(testAction));
        message1->Headers.Add(MessageIdHeader());

        Trace.WriteInfo(TraceType, "sending first message");
        node1->SendOneWay(target, std::move(message1));
        VERIFY_IS_TRUE(messageReceived.WaitOne(TimeSpan::FromSeconds(10)));

        firstConnection = ((TcpSendTarget&)(*target)).Test_ActiveConnection();
        Trace.WriteInfo(TraceType, "firstConnection: {0}", TextTracePtr(firstConnection));

        ((TcpDatagramTransport&)(*node2)).Test_DisableAccept(); // fail connection refresh

        Trace.WriteInfo(TraceType, "sleep for multiple session expiration");
        Sleep((DWORD)securityConfig.SessionExpiration.TotalMilliseconds() * 8);

        auto activeConnection2 = ((TcpSendTarget&)(*target)).Test_ActiveConnection();
        Trace.WriteInfo(TraceType, "activeConnection2: {0}", TextTracePtr(activeConnection2));
        VERIFY_IS_TRUE(firstConnection == activeConnection2); // acti

        auto message2 = make_unique<Message>();
        message2->Headers.Add(ActionHeader(testAction));
        message2->Headers.Add(MessageIdHeader());

        Trace.WriteInfo(TraceType, "sending second message");
        node1->SendOneWay(target, std::move(message2));
        VERIFY_IS_TRUE(messageReceived.WaitOne(TimeSpan::FromSeconds(10)));

        auto activeConnection3 = ((TcpSendTarget&)(*target)).Test_ActiveConnection();
        Trace.WriteInfo(TraceType, "activeConnection3: {0}", TextTracePtr(activeConnection3));
        VERIFY_IS_TRUE(firstConnection == activeConnection3);

        node1->UnregisterDisconnectEvent(de1);
        node2->UnregisterDisconnectEvent(de2);

        node2->Stop();
        node1->Stop();
    }
}
