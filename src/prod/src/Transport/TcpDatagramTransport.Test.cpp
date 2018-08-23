// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TestCommon.h"
#include <thread>

using namespace Common;
using namespace std;

namespace Transport
{
    class TcpTransportTests
    {
    protected:
        void SimpleTcpTest(std::wstring const & senderAddress, std::wstring const & receiverAddress);
        void DrainTest(int messageSize, bool successExpected, Common::TimeSpan drainTimeout);
        void ClientTcpTest(wstring const & serverAddress);
        void IdleTimeoutTest(bool idleTimeoutExpected, bool keepExternalSendTargetReference);
        void SendQueueExpirationTest(SecurityProvider::Enum provider);

        void ConnectionFaultTest_SenderClose(bool abortConnection);
        void ConnectionFaultTest_ReceiverClose(bool abortConnection);

        void ReceiveThreadDeadlockDetectionTest(SecurityProvider::Enum securityProvider);

        static void TargetInstanceTestSendRequest(
            IDatagramTransportSPtr const & srcNode,
            ISendTarget::SPtr const & target,
            wstring const & action);

        static void TargetInstanceTestCheckConnecitonCount(
            ISendTarget::SPtr const & target,
            wstring const & connectionType,
            size_t expected);
    };

    BOOST_FIXTURE_TEST_SUITE2(TcpSuite,TcpTransportTests)

    BOOST_AUTO_TEST_CASE(IncomingConnectionThrottle_SingleListener)
    {
        ENTER;

        AcceptThrottle::GetThrottle()->Test_SetThrottle(17);
        KFinally([] { AcceptThrottle::GetThrottle()->Test_SetThrottle(); });

        auto server = TcpDatagramTransport::Create(TTestUtil::GetListenAddress());

        wstring testAction = TTestUtil::GetGuidAction();
        auto messageReceived = make_shared<AutoResetEvent>(false);
        TTestUtil::SetMessageHandler(
            server,
            testAction,
            [testAction, messageReceived](MessageUPtr & msg, ISendTarget::SPtr const & st) -> void
            {
                Trace.WriteInfo(TraceType, "server received message {0} from {1}", msg->TraceId(), st->Address());
                messageReceived->Set();
            });

        VERIFY_IS_TRUE(server->Start().IsSuccess());

        vector<IDatagramTransportSPtr> clients;
        vector<ISendTarget::SPtr> targets;
        while (targets.size() < AcceptThrottle::GetThrottle()->Test_GetThrottle() * 3)
        {
            auto client = TcpDatagramTransport::CreateClient();
            VERIFY_IS_TRUE(client->Start().IsSuccess());

            ISendTarget::SPtr target = client->ResolveTarget(server->ListenAddress());
            VERIFY_IS_TRUE(target);

            auto oneway(make_unique<Message>());
            oneway->Headers.Add(ActionHeader(testAction));
            oneway->Headers.Add(MessageIdHeader());
            Trace.WriteInfo(TraceType, "client sending {0}", oneway->MessageId);
            client->SendOneWay(target, std::move(oneway));

            clients.push_back(move(client));
            targets.push_back(move(target));

            if (targets.size() < (AcceptThrottle::GetThrottle()->Test_GetThrottle() -1))
            {
                VERIFY_ARE_EQUAL2(AcceptThrottle::GetThrottle()->ConnectionDropCount(), 0);
                VERIFY_ARE_EQUAL2(TcpConnection::ConnectionFailureCount(), 0);
            }
        }

        VERIFY_IS_TRUE(messageReceived->WaitOne(TimeSpan::FromSeconds(3)));

        Trace.WriteInfo(TraceType, "AcceptThrottle dump: {0}", *AcceptThrottle::GetThrottle());
        auto connectionGroups = AcceptThrottle::GetThrottle()->Test_ConnectionGroups();
        VERIFY_IS_TRUE(connectionGroups.size() == 1);
        VERIFY_IS_TRUE(connectionGroups.cbegin()->second.size() < AcceptThrottle::GetThrottle()->Test_GetThrottle());

        Trace.WriteInfo(
            TraceType, "client count = {0}, incoming connection count = {1}, IncomingConnectionThrottle = {2}", 
            clients.size(), AcceptThrottle::GetThrottle()->IncomingConnectionCount(), AcceptThrottle::GetThrottle()->Test_GetThrottle());

        VERIFY_IS_TRUE(AcceptThrottle::GetThrottle()->IncomingConnectionCount() < AcceptThrottle::Test_IncomingConnectionUpperBound());

        auto expectedConnectionDropCountMin = targets.size() - AcceptThrottle::GetThrottle()->Test_GetThrottle() + 1;
        Trace.WriteInfo(
            TraceType,
            "expected connection drop count min = {0}, explicitDrop = {1}, ConnectionFailureCount= {2}",
            expectedConnectionDropCountMin,
            AcceptThrottle::GetThrottle()->ConnectionDropCount(),
            TcpConnection::ConnectionFailureCount());

        VERIFY_IS_TRUE(TTestUtil::WaitUntil(
            [=] { return (AcceptThrottle::GetThrottle()->ConnectionDropCount() + TcpConnection::ConnectionFailureCount()) >= expectedConnectionDropCountMin; },
            TimeSpan::FromSeconds(3)));

        Trace.WriteInfo(TraceType, "waiting for incoming connection count to drop below throttle");
        VERIFY_IS_TRUE(TTestUtil::WaitUntil(
            [] { return AcceptThrottle::GetThrottle()->IncomingConnectionCount() < AcceptThrottle::GetThrottle()->Test_GetThrottle(); },
            TimeSpan::FromSeconds(3)));

        VERIFY_IS_TRUE(
            TTestUtil::WaitUntil(
                [=] { return ((TcpDatagramTransport const&)(*server)).Test_AcceptSuspended() == false; },
                TimeSpan::FromSeconds(30)));

        server->Stop();

        for (auto const & client : clients)
        {
            client->Stop();
        }

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(IncomingConnectionThrottle_3Listeners)
    {
        ENTER;

        AcceptThrottle::GetThrottle()->Test_SetThrottle(21);
        KFinally([] { AcceptThrottle::GetThrottle()->Test_SetThrottle(); });

        vector<IDatagramTransportSPtr> servers;
        Common::atomic_bool connectionFaultOnFirstListener(false);
        AutoResetEvent connectionFaulted;
        wstring testAction = TTestUtil::GetGuidAction();
        Common::atomic_long counter(0);
        AutoResetEvent receivedAllTracked;
        Common::atomic_long outgoingMsgTracked(0);
        for (int i = 0; i < 3; ++i)
        {
            auto server = TcpDatagramTransport::Create(TTestUtil::GetListenAddress());
            TTestUtil::SetMessageHandler(
                server,
                testAction,
                [&counter, &outgoingMsgTracked, &receivedAllTracked](MessageUPtr &, ISendTarget::SPtr const &) -> void
            {
                if (++counter == outgoingMsgTracked.load())
                    receivedAllTracked.Set();
            });

            server->SetConnectionFaultHandler([i, &connectionFaulted, &connectionFaultOnFirstListener](ISendTarget const &, ErrorCode) -> void
            {
                if (i == 0)
                {
                    connectionFaultOnFirstListener.store(true);
                }
                connectionFaulted.Set();
            });

            VERIFY_IS_TRUE(server->Start().IsSuccess());
            servers.push_back(server);
        }

        vector<IDatagramTransportSPtr> clients;
        vector<ISendTarget::SPtr> targets;

        auto client = TcpDatagramTransport::CreateClient();
        VERIFY_IS_TRUE(client->Start().IsSuccess());
        for (auto const & server : servers)
        {
            ISendTarget::SPtr target = client->ResolveTarget(server->ListenAddress());
            VERIFY_IS_TRUE(target);

            auto oneway(make_unique<Message>());
            oneway->Headers.Add(ActionHeader(testAction));
            oneway->Headers.Add(MessageIdHeader());
            Trace.WriteInfo(TraceType, "client sending {0}", oneway->MessageId);
            client->SendOneWay(target, std::move(oneway));
            ++outgoingMsgTracked;

            targets.push_back(move(target));
        }

        // There should be no throttle so far
        VERIFY_ARE_EQUAL2(AcceptThrottle::GetThrottle()->ConnectionDropCount(), 0);
        VERIFY_ARE_EQUAL2(TcpConnection::ConnectionFailureCount(), 0);

        // Add more connections to the first listeners
        while (targets.size() < (AcceptThrottle::GetThrottle()->Test_GetThrottle() - 1))
        {
            auto client1 = TcpDatagramTransport::CreateClient();
            VERIFY_IS_TRUE(client1->Start().IsSuccess());

            ISendTarget::SPtr target = client1->ResolveTarget(servers.front()->ListenAddress());
            VERIFY_IS_TRUE(target);

            auto oneway(make_unique<Message>());
            oneway->Headers.Add(ActionHeader(testAction));
            oneway->Headers.Add(MessageIdHeader());
            Trace.WriteInfo(TraceType, "client sending {0}", oneway->MessageId);
            client1->SendOneWay(target, std::move(oneway));
            ++outgoingMsgTracked;

            targets.push_back(move(target));
            clients.push_back(move(client1));
        }

        // There should be no throttle so far
        VERIFY_ARE_EQUAL2(AcceptThrottle::GetThrottle()->ConnectionDropCount(), 0);
        VERIFY_ARE_EQUAL2(TcpConnection::ConnectionFailureCount(), 0);

        Trace.WriteInfo(TraceType, "messages sent previously should all be received as throttle will not be hit");
        VERIFY_IS_TRUE(receivedAllTracked.WaitOne(TimeSpan::FromSeconds(3)));

        Trace.WriteInfo(TraceType, "creating more cliens to the first listener to trigger throttle");
        for (uint i = 0; i < AcceptThrottle::GetThrottle()->Test_GetThrottle() * 2; ++i)
        {
            auto client1 = TcpDatagramTransport::CreateClient();
            VERIFY_IS_TRUE(client1->Start().IsSuccess());

            ISendTarget::SPtr target = client1->ResolveTarget(servers.front()->ListenAddress());
            VERIFY_IS_TRUE(target);

            auto oneway(make_unique<Message>());
            oneway->Headers.Add(ActionHeader(testAction));
            oneway->Headers.Add(MessageIdHeader());
            Trace.WriteInfo(TraceType, "client1 sending {0}", oneway->MessageId);
            client1->SendOneWay(target, std::move(oneway));

            targets.push_back(move(target));
            clients.push_back(move(client1));
        }

        Trace.WriteInfo(TraceType, "waiting for throttle to abort connections on the first listener");
        VERIFY_IS_TRUE(connectionFaulted.WaitOne(TimeSpan::FromSeconds(1)));

        Trace.WriteInfo(TraceType, "connection faults should happen to the first listener at least");
        VERIFY_IS_TRUE(connectionFaultOnFirstListener.load());

        Trace.WriteInfo(TraceType, "AcceptThrottle dump: {0}", *AcceptThrottle::GetThrottle());
        auto connectionGroups = AcceptThrottle::GetThrottle()->Test_ConnectionGroups();
        VERIFY_IS_TRUE(connectionGroups.size() == servers.size());
        uint totalConnectionInAllGroups = 0;
        for (auto const & connectionGroup : connectionGroups)
        {
            totalConnectionInAllGroups += static_cast<uint>(connectionGroup.second.size());
        }

        VERIFY_IS_TRUE(totalConnectionInAllGroups < AcceptThrottle::GetThrottle()->Test_GetThrottle());

        auto incomingCurrent = AcceptThrottle::GetThrottle()->IncomingConnectionCount();
        Trace.WriteInfo(TraceType, "incoming connection count = {0}", incomingCurrent);
        VERIFY_IS_TRUE(incomingCurrent < (AcceptThrottle::GetThrottle()->Test_GetThrottle() + servers.size()));

        auto expectedConnectionDropCountMin = targets.size() - AcceptThrottle::GetThrottle()->Test_GetThrottle() + 1;
        Trace.WriteInfo(
            TraceType,
            "expected connection drop count min = {0}, explicitDrop = {1}, implicitDropByAcceptSuspend= {2}",
            expectedConnectionDropCountMin,
            AcceptThrottle::GetThrottle()->ConnectionDropCount(),
            TcpConnection::ConnectionFailureCount());

        VERIFY_IS_TRUE(TTestUtil::WaitUntil(
            [=] { return (AcceptThrottle::GetThrottle()->ConnectionDropCount() + TcpConnection::ConnectionFailureCount()) >= expectedConnectionDropCountMin; },
            TimeSpan::FromSeconds(3)));

        Trace.WriteInfo(TraceType, "waiting for incoming connection count to drop below throttle");
        VERIFY_IS_TRUE(TTestUtil::WaitUntil(
            [] { return AcceptThrottle::GetThrottle()->IncomingConnectionCount() < AcceptThrottle::GetThrottle()->Test_GetThrottle(); },
            TimeSpan::FromSeconds(3)));

        for (auto const & server : servers)
        {
            VERIFY_IS_TRUE(
                TTestUtil::WaitUntil(
                    [=] { return ((TcpDatagramTransport const&)(*server)).Test_AcceptSuspended() == false; },
                    TimeSpan::FromSeconds(30)));

            server->Stop();
        }

        for (auto const & c : clients)
        {
            c->Stop();
        }

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(IncomingConnectionThrottle_ConcurrentClients)
    {
        ENTER;

        AcceptThrottle::GetThrottle()->Test_SetThrottle(16);
        auto saved2 = TransportConfig::GetConfig().TcpFastLoopbackEnabled;
        TransportConfig::GetConfig().TcpFastLoopbackEnabled = false;
        auto saved3 = TransportConfig::GetConfig().TcpListenBacklog;
        TransportConfig::GetConfig().TcpListenBacklog = 1;
        KFinally([=]
        {
            AcceptThrottle::GetThrottle()->Test_SetThrottle();
            TransportConfig::GetConfig().TcpFastLoopbackEnabled = saved2;
            TransportConfig::GetConfig().TcpListenBacklog = saved3;
        });

        vector<IDatagramTransportSPtr> servers;
        AutoResetEvent connectionFaulted;
        wstring testAction = TTestUtil::GetGuidAction();
        for (int i = 0; i < 5; ++i)
        {
            auto server = TcpDatagramTransport::Create(TTestUtil::GetListenAddress());
            TTestUtil::SetMessageHandler(
                server,
                testAction,
                [](MessageUPtr &, ISendTarget::SPtr const &) -> void {});

            VERIFY_IS_TRUE(server->Start().IsSuccess());
            servers.push_back(server);
        }

        ManualResetEvent shouldStart;
        Common::atomic_long completed(0);
        RwLock lock;
        vector<IDatagramTransportSPtr> clients;
        vector<ISendTarget::SPtr> targets;

        int processorCount = Environment::GetNumberOfProcessors();
        int clientThreadCount = (processorCount > 2) ? processorCount : 3;
        Trace.WriteInfo(TraceType, "client thread count = {0}", clientThreadCount);

#ifdef PLATFORM_UNIX
        const int round = 2; //LINUXTODO, increase this
#else
        const int round = 20;
#endif

        for (int i = 0; i < clientThreadCount; ++i)
        {
            Threadpool::Post([&]
            {
                Trace.WriteInfo(TraceType, "client thread waiting to start");
                shouldStart.WaitOne();

                for (int i = 0; i < round ; ++i)
                {
                    auto client = TcpDatagramTransport::CreateClient();
                    client->SetKeepAliveTimeout(TimeSpan::FromSeconds(1));
                    VERIFY_IS_TRUE(client->Start().IsSuccess());

                    for (auto const & server : servers)
                    {
                        ISendTarget::SPtr target = client->ResolveTarget(server->ListenAddress());
                        VERIFY_IS_TRUE(target);

                        auto oneway(make_unique<Message>());
                        oneway->Headers.Add(ActionHeader(testAction));
                        oneway->Headers.Add(MessageIdHeader());
                        client->SendOneWay(target, std::move(oneway));

                        {
                            AcquireWriteLock grab(lock);
                            targets.push_back(move(target));
                        }
                    }

                    {
                        AcquireWriteLock grab(lock);
                        clients.push_back(move(client));
                    }

                    this_thread::yield();
                }

                ++completed;
            });
        }

        Trace.WriteInfo(TraceType, "start clients");
        shouldStart.Set();

        Trace.WriteInfo(TraceType, "wait for all clients to complete");
        VERIFY_IS_TRUE(TTestUtil::WaitUntil(
            [&] { return completed.load() == clientThreadCount; },
            TimeSpan::FromMinutes(1)));

        Trace.WriteInfo(TraceType, "AcceptThrottle dump: {0}", *AcceptThrottle::GetThrottle());
        auto connectionGroups = AcceptThrottle::GetThrottle()->Test_ConnectionGroups();
        VERIFY_IS_TRUE(connectionGroups.size() <= servers.size());
        uint totalConnectionInAllGroups = 0;
        for (auto const & connectionGroup : connectionGroups)
        {
            totalConnectionInAllGroups += static_cast<uint>(connectionGroup.second.size());
        }

        VERIFY_IS_TRUE(totalConnectionInAllGroups < AcceptThrottle::GetThrottle()->Test_GetThrottle());

        auto incomingCurrent = AcceptThrottle::GetThrottle()->IncomingConnectionCount();
        Trace.WriteInfo(TraceType, "incoming connection count = {0}", incomingCurrent);
        VERIFY_IS_TRUE(incomingCurrent < (AcceptThrottle::GetThrottle()->Test_GetThrottle() + servers.size()));

        auto expectedConnectionDropCountMin = targets.size() - AcceptThrottle::GetThrottle()->Test_GetThrottle() + 1;
        Trace.WriteInfo(
            TraceType,
            "expected connection drop count min = {0}, explicitDrop = {1}, ConnectionFailureCount = {2}",
            expectedConnectionDropCountMin,
            AcceptThrottle::GetThrottle()->ConnectionDropCount(),
            TcpConnection::ConnectionFailureCount());

        VERIFY_IS_TRUE(
            TTestUtil::WaitUntil([=]
                {
                    Trace.WriteInfo(
                        TraceType,
                        "ConnectionDropCount = {0}, ConnectionFailureCount = {1}",
                        AcceptThrottle::GetThrottle()->ConnectionDropCount(),
                        TcpConnection::ConnectionFailureCount());

                    return (AcceptThrottle::GetThrottle()->ConnectionDropCount() + TcpConnection::ConnectionFailureCount()) >= expectedConnectionDropCountMin;
                },
                TimeSpan::FromSeconds(60)));

        Trace.WriteInfo(TraceType, "waiting for incoming connection count to drop below throttle");
        VERIFY_IS_TRUE(TTestUtil::WaitUntil(
            [] { return AcceptThrottle::GetThrottle()->IncomingConnectionCount() < AcceptThrottle::GetThrottle()->Test_GetThrottle(); },
            TimeSpan::FromSeconds(60)));

        Trace.WriteInfo(TraceType, "clean up");
        for (auto const & server : servers)
        {
            VERIFY_IS_TRUE(
                TTestUtil::WaitUntil(
                    [=] { return ((TcpDatagramTransport const&)(*server)).Test_AcceptSuspended() == false; },
                    TimeSpan::FromSeconds(30)));

            server->Stop();
        }

        for (auto const & client : clients)
        {
            client->Stop();
        }

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(IncomingConnectionThrottle_RandomClients)
    {
        ENTER;

        AcceptThrottle::GetThrottle()->Test_SetThrottle(16);
        auto saved2 = TransportConfig::GetConfig().TcpFastLoopbackEnabled;
        TransportConfig::GetConfig().TcpFastLoopbackEnabled = false;
        KFinally([=]
        {
            AcceptThrottle::GetThrottle()->Test_SetThrottle();
            TransportConfig::GetConfig().TcpFastLoopbackEnabled = saved2;
        });

        vector<IDatagramTransportSPtr> servers;
        AutoResetEvent connectionFaulted;
        wstring testAction = TTestUtil::GetGuidAction();
        for (int i = 0; i < 5; ++i)
        {
            auto server = TcpDatagramTransport::Create(TTestUtil::GetListenAddress());
            TTestUtil::SetMessageHandler(
                server,
                testAction,
                [](MessageUPtr &, ISendTarget::SPtr const &) -> void {});

            VERIFY_IS_TRUE(server->Start().IsSuccess());
            servers.push_back(server);
        }

        ManualResetEvent shouldStart;
        Common::atomic_long completed(0);
        RwLock lock;
        vector<IDatagramTransportSPtr> clients;
        vector<ISendTarget::SPtr> targets;

        int processorCount = Environment::GetNumberOfProcessors();
        int clientThreadCount = (processorCount > 2) ? processorCount : 3;
        Trace.WriteInfo(TraceType, "client thread count = {0}", clientThreadCount);

        for (int i = 0; i < clientThreadCount; ++i)
        {
            Threadpool::Post([&]
            {
                Trace.WriteInfo(TraceType, "client thread waiting to start");
                shouldStart.WaitOne();

                Random r;
                for (int i = 0; i < 100; ++i)
                {
                    auto client = TcpDatagramTransport::CreateClient();
                    client->SetKeepAliveTimeout(TimeSpan::FromSeconds(1));
                    VERIFY_IS_TRUE(client->Start().IsSuccess());

                    ISendTarget::SPtr target = client->ResolveTarget(servers[r.Next(0, static_cast<int>(servers.size()))]->ListenAddress());
                    VERIFY_IS_TRUE(target);

                    auto oneway(make_unique<Message>());
                    oneway->Headers.Add(ActionHeader(testAction));
                    oneway->Headers.Add(MessageIdHeader());
                    client->SendOneWay(target, std::move(oneway));

                    {
                        AcquireWriteLock grab(lock);
                        targets.push_back(move(target));
                        clients.push_back(move(client));
                    }

                    this_thread::yield();
                }

                ++completed;
            });
        }

        Trace.WriteInfo(TraceType, "start clients");
        shouldStart.Set();

        Trace.WriteInfo(TraceType, "wait for all clients to complete");
        auto w = TTestUtil::WaitUntil(
            [&] { return completed.load() == clientThreadCount; },
            TimeSpan::FromMinutes(1));

        VERIFY_IS_TRUE(w);

        Trace.WriteInfo(TraceType, "AcceptThrottle dump: {0}", *AcceptThrottle::GetThrottle());
        auto connectionGroups = AcceptThrottle::GetThrottle()->Test_ConnectionGroups();
        VERIFY_IS_TRUE(connectionGroups.size() <= servers.size());
        uint totalConnectionInAllGroups = 0;
        for (auto const & connectionGroup : connectionGroups)
        {
            totalConnectionInAllGroups += static_cast<uint>(connectionGroup.second.size());
        }

        VERIFY_IS_TRUE(totalConnectionInAllGroups < AcceptThrottle::GetThrottle()->Test_GetThrottle());

        auto incomingCurrent = AcceptThrottle::GetThrottle()->IncomingConnectionCount();
        Trace.WriteInfo(TraceType, "incoming connection count = {0}", incomingCurrent);
        VERIFY_IS_TRUE(incomingCurrent < (AcceptThrottle::GetThrottle()->Test_GetThrottle() + servers.size()));

        auto expectedConnectionDropCountMin = targets.size() - AcceptThrottle::GetThrottle()->Test_GetThrottle() + 1;
        Trace.WriteInfo(
            TraceType,
            "expected connection drop count min = {0}, explicitDrop = {1}, implicitDropByAcceptSuspend= {2}",
            expectedConnectionDropCountMin,
            AcceptThrottle::GetThrottle()->ConnectionDropCount(),
            TcpConnection::ConnectionFailureCount());

        VERIFY_IS_TRUE(
            TTestUtil::WaitUntil([=]
                {
                    for (auto const & client : clients)
                    {
                        ((TcpDatagramTransport&)(*client)).ValidateConnections();
                    }

                    Trace.WriteInfo(
                        TraceType,
                        "ConnectionDropCount = {0}, ConnectionFailureCount = {1}",
                        AcceptThrottle::GetThrottle()->ConnectionDropCount(),
                        TcpConnection::ConnectionFailureCount());

                    return (AcceptThrottle::GetThrottle()->ConnectionDropCount() + TcpConnection::ConnectionFailureCount()) >= expectedConnectionDropCountMin;
                },
                TimeSpan::FromSeconds(60)));

        Trace.WriteInfo(TraceType, "waiting for incoming connection count to drop below throttle");
        VERIFY_IS_TRUE(TTestUtil::WaitUntil(
            [] { return AcceptThrottle::GetThrottle()->IncomingConnectionCount() < AcceptThrottle::GetThrottle()->Test_GetThrottle(); },
            TimeSpan::FromSeconds(60)));

        Trace.WriteInfo(TraceType, "clean up");
        targets.clear();

        for (auto const & server : servers)
        {
            VERIFY_IS_TRUE(
                TTestUtil::WaitUntil(
                    [=] { return ((TcpDatagramTransport const&)(*server)).Test_AcceptSuspended() == false; },
                    TimeSpan::FromSeconds(30)));
            server->Stop();
        }

        for (auto const & client : clients)
        {
            client->Stop();
        }

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(TargetInstanceTest)
    {
        ENTER;

        Trace.WriteInfo(TraceType, "test TargetDown on inbound send target");
        {
            auto node1 = TcpDatagramTransport::Create(TTestUtil::GetListenAddress());
            node1->SetInstance(10000);

            auto node2 = TcpDatagramTransport::Create(TTestUtil::GetListenAddress());
            node2->SetInstance(20000);

            ISendTarget::SPtr inboundSendTarget;
            wstring testAction = TTestUtil::GetGuidAction();
            TTestUtil::SetMessageHandler(
                node2,
                testAction,
                [testAction, &inboundSendTarget, &node1, &node2] (MessageUPtr & message, ISendTarget::SPtr const & target)
                {
                    Trace.WriteInfo(
                        TraceType,
                        "node2 received {0} from {1} at {2}",
                        message->TraceId(), TextTracePtrAs(target.get(), ISendTarget), target->Address());

                    VERIFY_IS_TRUE(target->Address() == node1->ListenAddress());
                    inboundSendTarget = target;

                    // reply
                    auto reply = make_unique<Message>();
                    reply->Headers.Add(ActionHeader(testAction));
                    reply->Headers.Add(RelatesToHeader(message->MessageId));
                    Trace.WriteInfo(
                        TraceType,
                        "node2 sending reply {0} to {1} at {2}",
                        reply->TraceId(), TextTracePtrAs(target.get(), ISendTarget), target->Address());
                    node2->SendOneWay(target, move(reply));
                });

            VERIFY_IS_TRUE(node2->Start().IsSuccess());

            AutoResetEvent replyReceived(false);
            TTestUtil::SetMessageHandler(
                node1,
                testAction,
                [&replyReceived, &node2] (MessageUPtr & reply, ISendTarget::SPtr const & target)
                {
                    Trace.WriteInfo(
                        TraceType,
                        "node1 received {0} from {1} at {2}",
                        reply->TraceId(), TextTracePtrAs(target.get(), ISendTarget), target->Address());

                    VERIFY_IS_TRUE(target->Address() == node2->ListenAddress());
                    replyReceived.Set();
                });

            VERIFY_IS_TRUE(node1->Start().IsSuccess());

            auto outboundSendTarget = node1->ResolveTarget(node2->ListenAddress());
            TargetInstanceTestSendRequest(node1, outboundSendTarget, testAction);

            Trace.WriteInfo(TraceType, "wait for node1 getting ListenInstance from node2, which comes before reply");
            VERIFY_IS_TRUE(replyReceived.WaitOne(TimeSpan::FromSeconds(30)));

            TargetInstanceTestCheckConnecitonCount(outboundSendTarget, L"outbound", 1);
            TargetInstanceTestCheckConnecitonCount(inboundSendTarget, L"inbound", 1);

            Trace.WriteInfo(TraceType, "TargetDown with older instance will not work");
            inboundSendTarget->TargetDown(node1->Instance() - 1);
            TargetInstanceTestCheckConnecitonCount(outboundSendTarget, L"outbound", 1);
            TargetInstanceTestCheckConnecitonCount(inboundSendTarget, L"inbound", 1);

            Trace.WriteInfo(TraceType, "TargetDown with current instance should close the connection");
            inboundSendTarget->TargetDown(node1->Instance());
            TargetInstanceTestCheckConnecitonCount(inboundSendTarget, L"inbound", 0);

            Trace.WriteInfo(TraceType, "Request after incrementing node1 instance");
            node1->SetInstance(node1->Instance() + 1);
            TargetInstanceTestCheckConnecitonCount(outboundSendTarget, L"outbound", 0);
            TargetInstanceTestSendRequest(node1, outboundSendTarget, testAction);
            VERIFY_IS_TRUE(replyReceived.WaitOne(TimeSpan::FromSeconds(30)));
        }

        Trace.WriteInfo(TraceType, "test TargetDown on outbound send target");
        {
            auto node1 = TcpDatagramTransport::Create(TTestUtil::GetListenAddress());
            node1->SetInstance(10000);

            auto node2 = TcpDatagramTransport::Create(TTestUtil::GetListenAddress());
            node2->SetInstance(20000);

            ISendTarget::SPtr inboundSendTarget;
            wstring testAction = TTestUtil::GetGuidAction();
            TTestUtil::SetMessageHandler(
                node2,
                testAction,
                [testAction, &inboundSendTarget, &node1, &node2] (MessageUPtr & message, ISendTarget::SPtr const & target)
                {
                    Trace.WriteInfo(
                        TraceType,
                        "node2 received {0} from {1} at {2}",
                        message->TraceId(), TextTracePtrAs(target.get(), ISendTarget), target->Address());

                    VERIFY_IS_TRUE(target->Address() == node1->ListenAddress());
                    inboundSendTarget = target;

                    // reply
                    auto reply = make_unique<Message>();
                    reply->Headers.Add(ActionHeader(testAction));
                    reply->Headers.Add(RelatesToHeader(message->MessageId));
                    Trace.WriteInfo(
                        TraceType,
                        "node2 sending reply {0} to {1} at {2}",
                        reply->TraceId(), TextTracePtrAs(target.get(), ISendTarget), target->Address());
                    node2->SendOneWay(target, move(reply));
                });

            VERIFY_IS_TRUE(node2->Start().IsSuccess());

            AutoResetEvent replyReceived(false);
            TTestUtil::SetMessageHandler(
                node1,
                testAction,
                [&replyReceived, &node2] (MessageUPtr & reply, ISendTarget::SPtr const & target)
                {
                    Trace.WriteInfo(
                        TraceType,
                        "node1 received {0} from {1} at {2}",
                        reply->TraceId(), TextTracePtrAs(target.get(), ISendTarget), target->Address());

                    VERIFY_IS_TRUE(target->Address() == node2->ListenAddress());
                    replyReceived.Set();
                });

            VERIFY_IS_TRUE(node1->Start().IsSuccess());

            auto outboundSendTarget = node1->ResolveTarget(node2->ListenAddress());
            TargetInstanceTestSendRequest(node1, outboundSendTarget, testAction);

            Trace.WriteInfo(TraceType, "wait for node1 getting ListenInstance from node2, which comes before reply");
            VERIFY_IS_TRUE(replyReceived.WaitOne(TimeSpan::FromSeconds(30)));

            TargetInstanceTestCheckConnecitonCount(outboundSendTarget, L"outbound", 1);
            TargetInstanceTestCheckConnecitonCount(inboundSendTarget, L"inbound", 1);

            Trace.WriteInfo(TraceType, "TargetDown with older instance will not work");
            outboundSendTarget->TargetDown(node2->Instance() - 1);
            TargetInstanceTestCheckConnecitonCount(outboundSendTarget, L"outbound", 1);
            TargetInstanceTestCheckConnecitonCount(inboundSendTarget, L"inbound", 1);

            Trace.WriteInfo(TraceType, "TargetDown with newer instance should close the connection");
            outboundSendTarget->TargetDown(node2->Instance() + 1);
            TargetInstanceTestCheckConnecitonCount(outboundSendTarget, L"outbound", 0);

            Trace.WriteInfo(TraceType, "Request after updating node2 instance to an even larger value");
            node2->SetInstance(node2->Instance() + 2);
            TargetInstanceTestCheckConnecitonCount(inboundSendTarget, L"inbound", 0);
            TargetInstanceTestSendRequest(node1, outboundSendTarget, testAction);
            VERIFY_IS_TRUE(replyReceived.WaitOne(TimeSpan::FromSeconds(30)));
        }

        Trace.WriteInfo(TraceType, "test ResolveTarget with different instances on outboundTarget");
        {
            auto node1 = TcpDatagramTransport::Create(TTestUtil::GetListenAddress());
            node1->SetInstance(10000);

            auto node2 = TcpDatagramTransport::Create(TTestUtil::GetListenAddress());
            node2->SetInstance(20000);

            ISendTarget::SPtr inboundSendTarget;
            wstring testAction = TTestUtil::GetGuidAction();
            TTestUtil::SetMessageHandler(
                node2,
                testAction,
                [testAction, &inboundSendTarget, &node1, &node2] (MessageUPtr & message, ISendTarget::SPtr const & target)
                {
                    Trace.WriteInfo(
                        TraceType,
                        "node2 received {0} from {1} at {2}",
                        message->TraceId(), TextTracePtrAs(target.get(), ISendTarget), target->Address());

                    VERIFY_IS_TRUE(target->Address() == node1->ListenAddress());
                    inboundSendTarget = target;

                    // reply
                    auto reply = make_unique<Message>();
                    reply->Headers.Add(ActionHeader(testAction));
                    reply->Headers.Add(RelatesToHeader(message->MessageId));
                    Trace.WriteInfo(
                        TraceType,
                        "node2 sending reply {0} to {1} at {2}",
                        reply->TraceId(), TextTracePtrAs(target.get(), ISendTarget), target->Address());
                    node2->SendOneWay(target, move(reply));
                });

            VERIFY_IS_TRUE(node2->Start().IsSuccess());

            AutoResetEvent replyReceived(false);
            TTestUtil::SetMessageHandler(
                node1,
                testAction,
                [&replyReceived, &node2] (MessageUPtr & reply, ISendTarget::SPtr const & target)
                {
                    Trace.WriteInfo(
                        TraceType,
                        "node1 received {0} from {1} at {2}",
                        reply->TraceId(), TextTracePtrAs(target.get(), ISendTarget), target->Address());

                    VERIFY_IS_TRUE(target->Address() == node2->ListenAddress());
                    replyReceived.Set();
                });

            VERIFY_IS_TRUE(node1->Start().IsSuccess());

            auto outboundSendTarget = node1->ResolveTarget(node2->ListenAddress());
            TargetInstanceTestSendRequest(node1, outboundSendTarget, testAction);

            Trace.WriteInfo(TraceType, "wait for node1 getting ListenInstance from node2, which comes before reply");
            VERIFY_IS_TRUE(replyReceived.WaitOne(TimeSpan::FromSeconds(30)));

            TargetInstanceTestCheckConnecitonCount(outboundSendTarget, L"outbound", 1);
            TargetInstanceTestCheckConnecitonCount(inboundSendTarget, L"inbound", 1);

            Trace.WriteInfo(TraceType, "resolve with older instance, the connections should be still up");
            auto resolveWithOlderInstance = node1->ResolveTarget(node2->ListenAddress(), L"", node2->Instance() - 1);
            TargetInstanceTestCheckConnecitonCount(inboundSendTarget, L"inbound", 1);
            TargetInstanceTestCheckConnecitonCount(outboundSendTarget, L"outbound", 1);

            Trace.WriteInfo(TraceType, "resolve with current instance, the connections should be still up");
            auto resolveWithCurrentInstance = node1->ResolveTarget(node2->ListenAddress(), L"", node2->Instance());

            TargetInstanceTestCheckConnecitonCount(inboundSendTarget, L"inbound", 1);
            TargetInstanceTestCheckConnecitonCount(outboundSendTarget, L"outbound", 1);

            Trace.WriteInfo(TraceType, "resolve with newer instance, the connections should be down");
            auto resolveWithNewerInstance = node1->ResolveTarget(node2->ListenAddress(), L"", node2->Instance() + 1);
            TargetInstanceTestCheckConnecitonCount(outboundSendTarget, L"outbound", 0);

            Trace.WriteInfo(TraceType, "New request after incrementing node2 instance");
            node2->SetInstance(node2->Instance() + 1);
            TargetInstanceTestCheckConnecitonCount(inboundSendTarget, L"inbound", 0);
            TargetInstanceTestSendRequest(node1, outboundSendTarget, testAction);
            VERIFY_IS_TRUE(replyReceived.WaitOne(TimeSpan::FromSeconds(30)));
        }

        Trace.WriteInfo(TraceType, "test ResolveTarget with different instances on inboundTarget");
        {
            auto node1 = TcpDatagramTransport::Create(TTestUtil::GetListenAddress());
            node1->SetInstance(10000);
            auto node2 = TcpDatagramTransport::Create(TTestUtil::GetListenAddress());
            node2->SetInstance(20000);

            ISendTarget::SPtr inboundSendTarget;
            wstring testAction = TTestUtil::GetGuidAction();
            TTestUtil::SetMessageHandler(
                node2,
                testAction,
                [testAction, &inboundSendTarget, &node1, &node2] (MessageUPtr & message, ISendTarget::SPtr const & target)
                {
                    Trace.WriteInfo(
                        TraceType,
                        "node2 received {0} from {1} at {2}",
                        message->TraceId(), TextTracePtrAs(target.get(), ISendTarget), target->Address());

                    VERIFY_IS_TRUE(target->Address() == node1->ListenAddress());
                    inboundSendTarget = target;

                    // reply
                    auto reply = make_unique<Message>();
                    reply->Headers.Add(ActionHeader(testAction));
                    reply->Headers.Add(RelatesToHeader(message->MessageId));
                    Trace.WriteInfo(
                        TraceType,
                        "node2 sending reply {0} to {1} at {2}",
                        reply->TraceId(), TextTracePtrAs(target.get(), ISendTarget), target->Address());
                    node2->SendOneWay(target, move(reply));
                });

            AutoResetEvent replyReceived(false);
            TTestUtil::SetMessageHandler(
                node1,
                testAction,
                [&replyReceived, &node2] (MessageUPtr & reply, ISendTarget::SPtr const & target)
                {
                    Trace.WriteInfo(
                        TraceType,
                        "node1 received {0} from {1} at {2}",
                        reply->TraceId(), TextTracePtrAs(target.get(), ISendTarget), target->Address());

                    VERIFY_IS_TRUE(target->Address() == node2->ListenAddress());
                    replyReceived.Set();
                });

            VERIFY_IS_TRUE(node1->Start().IsSuccess());
            VERIFY_IS_TRUE(node2->Start().IsSuccess());

            auto outboundSendTarget = node1->ResolveTarget(node2->ListenAddress());

            TargetInstanceTestSendRequest(node1, outboundSendTarget, testAction);
            Trace.WriteInfo(TraceType, "wait for node1 getting ListenInstance from node2, which comes before reply");
            VERIFY_IS_TRUE(replyReceived.WaitOne(TimeSpan::FromSeconds(30)));

            TargetInstanceTestCheckConnecitonCount(outboundSendTarget, L"outbound", 1);
            TargetInstanceTestCheckConnecitonCount(inboundSendTarget, L"inbound", 1);

            Trace.WriteInfo(TraceType, "resolve with older instance, the connections should be still up");
            auto resolveWithOlderInstance = node2->ResolveTarget(node1->ListenAddress(), L"", node1->Instance() - 1);

            TargetInstanceTestCheckConnecitonCount(inboundSendTarget, L"inbound", 1);
            TargetInstanceTestCheckConnecitonCount(outboundSendTarget, L"outbound", 1);

            Trace.WriteInfo(TraceType, "resolve with current instance, the connections should be still up");
            auto resolveWithCurrentInstance = node2->ResolveTarget(
                node1->ListenAddress(), L"", node1->Instance());

            TargetInstanceTestCheckConnecitonCount(inboundSendTarget, L"inbound", 1);
            TargetInstanceTestCheckConnecitonCount(outboundSendTarget, L"outbound", 1);

            Trace.WriteInfo(TraceType, "resolve with newer instance, the connections should be down");
            auto resolveWithNewerInstance = node2->ResolveTarget(node1->ListenAddress(), L"", node1->Instance() + 1);
            TargetInstanceTestCheckConnecitonCount(inboundSendTarget, L"inbound", 0);

            Trace.WriteInfo(TraceType, "Request after updating node1 instance to an even larger value");
            node1->SetInstance(node1->Instance() + 1);
            TargetInstanceTestCheckConnecitonCount(outboundSendTarget, L"outbound", 0);
            TargetInstanceTestSendRequest(node1, outboundSendTarget, testAction);
            VERIFY_IS_TRUE(replyReceived.WaitOne(TimeSpan::FromSeconds(30)));
        }

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(ReceiveThreadDeadlockDetectionTest1)
    {
        ENTER;
        ReceiveThreadDeadlockDetectionTest(SecurityProvider::None);
        LEAVE;
    }

#ifndef PLATFORM_UNIX
    BOOST_AUTO_TEST_CASE(ReceiveThreadDeadlockDetectionTest2)
    {
        ENTER;
        ReceiveThreadDeadlockDetectionTest(SecurityProvider::Ssl);
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(ReceiveThreadDeadlockDetectionTest3)
    {
        ENTER;
        ReceiveThreadDeadlockDetectionTest(SecurityProvider::Negotiate);
        LEAVE;
    }
#endif

    BOOST_AUTO_TEST_CASE(ListenEndpointTest)
    {
        ENTER;

        TransportConfig & config = TransportConfig::GetConfig();

        wstring resolveOptionSaved = config.ResolveOption;
        config.ResolveOption = L"unspecified";
        Trace.WriteInfo(TraceType, "test with resolve type {0}", config.ResolveOption);

        {
            auto transport = TcpDatagramTransport::Create(L"127.0.0.1:0", L"");
            VERIFY_IS_TRUE(transport->Start().IsSuccess());
            vector<Endpoint> listenEndpoints = transport->Test_ListenEndpoints();

            VERIFY_IS_TRUE(listenEndpoints.size() == 1);
            VERIFY_IS_TRUE(listenEndpoints[0].IsLoopback());
        }

        {
            auto transport = TcpDatagramTransport::Create(L"[::1]:0", L"");
            VERIFY_IS_TRUE(transport->Start().IsSuccess());
            vector<Endpoint> listenEndpoints = transport->Test_ListenEndpoints();

            VERIFY_IS_TRUE(listenEndpoints.size() == 1);
            VERIFY_IS_TRUE(listenEndpoints[0].IsLoopback());
        }

        {
            auto transport = TcpDatagramTransport::Create(L"localhost:0", L"");
            VERIFY_IS_TRUE(transport->Start().IsSuccess());
            vector<Endpoint> listenEndpoints = transport->Test_ListenEndpoints();

            VERIFY_IS_FALSE(listenEndpoints.empty());
            for (auto iter = listenEndpoints.cbegin(); iter != listenEndpoints.cend(); ++ iter)
            {
                VERIFY_IS_FALSE(listenEndpoints.empty());
                for (auto iter1 = listenEndpoints.cbegin(); iter1 != listenEndpoints.cend(); ++ iter1)
                {
                    VERIFY_IS_TRUE(iter1->IsLoopback());
                }
            }
        }

        wstring hostname;
        VERIFY_IS_TRUE(TcpTransportUtility::GetLocalFqdn(hostname).IsSuccess());
        wstring hostnameAddress = hostname + L":0";
        {
            auto transport = TcpDatagramTransport::Create(hostnameAddress, L"");
            VERIFY_IS_TRUE(transport->Start().IsSuccess());
            vector<Endpoint> listenEndpoints = transport->Test_ListenEndpoints();

            VERIFY_IS_FALSE(listenEndpoints.empty());
            for (auto iter = listenEndpoints.cbegin(); iter != listenEndpoints.cend(); ++ iter)
            {
                if (iter->IsIPv4())
                {
                    VERIFY_IS_TRUE(memcmp(
                        &iter->as_sockaddr_in()->sin_addr,
                        &in4addr_any,
                        sizeof(in4addr_any)) == 0);
                }
                else if (iter->IsIPv6())
                {
                    VERIFY_IS_TRUE(memcmp(
                        &iter->as_sockaddr_in6()->sin6_addr,
                        &in6addr_any,
                        sizeof(in6addr_any)) == 0);
                }
            }
        }

        config.ResolveOption = L"ipv4";
        Trace.WriteInfo(TraceType, "test with resolve type {0}", config.ResolveOption);

        {
            auto transport = TcpDatagramTransport::Create(L"127.0.0.1:0", L"");
            VERIFY_IS_TRUE(transport->Start().IsSuccess());
            vector<Endpoint> listenEndpoints = transport->Test_ListenEndpoints();

            VERIFY_IS_TRUE(listenEndpoints.size() == 1);
            VERIFY_IS_TRUE(listenEndpoints[0].IsIPv4());
            VERIFY_IS_TRUE(listenEndpoints[0].IsLoopback());
        }

        {
            auto transport = TcpDatagramTransport::Create(L"localhost:0", L"");
            VERIFY_IS_TRUE(transport->Start().IsSuccess());
            vector<Endpoint> listenEndpoints = transport->Test_ListenEndpoints();

            VERIFY_IS_FALSE(listenEndpoints.empty());
            for (auto iter = listenEndpoints.cbegin(); iter != listenEndpoints.cend(); ++ iter)
            {
                VERIFY_IS_FALSE(listenEndpoints.empty());
                for (auto iter1 = listenEndpoints.cbegin(); iter1 != listenEndpoints.cend(); ++ iter1)
                {
                    VERIFY_IS_TRUE(iter1->IsIPv4());
                    VERIFY_IS_TRUE(iter1->IsLoopback());
                }
            }
        }

        {
            auto transport = TcpDatagramTransport::Create(hostnameAddress, L"");
            VERIFY_IS_TRUE(transport->Start().IsSuccess());
            vector<Endpoint> listenEndpoints = transport->Test_ListenEndpoints();

            VERIFY_IS_FALSE(listenEndpoints.empty());
            for (auto iter = listenEndpoints.cbegin(); iter != listenEndpoints.cend(); ++ iter)
            {
                VERIFY_IS_TRUE(iter->IsIPv4());

                VERIFY_IS_TRUE(memcmp(
                    &iter->as_sockaddr_in()->sin_addr,
                    &in4addr_any,
                    sizeof(in4addr_any)) == 0);
            }
        }

        config.ResolveOption = L"ipv6";
        Trace.WriteInfo(TraceType, "test with resolve type {0}", config.ResolveOption);

        {
            auto transport = TcpDatagramTransport::Create(L"[::1]:0", L"");
            VERIFY_IS_TRUE(transport->Start().IsSuccess());
            vector<Endpoint> listenEndpoints = transport->Test_ListenEndpoints();

            VERIFY_IS_TRUE(listenEndpoints.size() == 1);
            VERIFY_IS_TRUE(listenEndpoints[0].IsIPv6());
            VERIFY_IS_TRUE(listenEndpoints[0].IsLoopback());
        }

#ifndef PLATFORM_UNIX //LINUXTODO Xubuntu maps ::1 to ipv6-localhost, need to check if this is true for other distros 
        {
            auto transport = TcpDatagramTransport::Create(L"localhost:0", L"");
            VERIFY_IS_TRUE(transport->Start().IsSuccess());
            vector<Endpoint> listenEndpoints = transport->Test_ListenEndpoints();

            VERIFY_IS_FALSE(listenEndpoints.empty());
            for (auto iter = listenEndpoints.cbegin(); iter != listenEndpoints.cend(); ++ iter)
            {
                VERIFY_IS_FALSE(listenEndpoints.empty());
                for (auto iter1 = listenEndpoints.cbegin(); iter1 != listenEndpoints.cend(); ++ iter1)
                {
                    Trace.WriteInfo(TraceType, "localhost endpoint: {0}", *iter1); 
                    VERIFY_IS_TRUE(iter1->IsIPv6());
                    VERIFY_IS_TRUE(iter1->IsLoopback());
                }
            }
        }

        {
            auto transport = TcpDatagramTransport::Create(hostnameAddress, L"");
            VERIFY_IS_TRUE(transport->Start().IsSuccess());
            vector<Endpoint> listenEndpoints = transport->Test_ListenEndpoints();

            VERIFY_IS_FALSE(listenEndpoints.empty());
            for (auto iter = listenEndpoints.cbegin(); iter != listenEndpoints.cend(); ++ iter)
            {
                VERIFY_IS_TRUE(iter->IsIPv6());
                VERIFY_IS_TRUE(memcmp(
                    &iter->as_sockaddr_in6()->sin6_addr,
                    &in6addr_any,
                    sizeof(in6addr_any)) == 0);
            }
        }
#endif

        config.ResolveOption = resolveOptionSaved;
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(ConcurrentClientStart)
    {
        ENTER;

        auto client = DatagramTransportFactory::CreateTcpClient();

        atomic_uint64 count(32);
        AutoResetEvent completed(false);
        for(auto startCount = count.load(); startCount > 0; --startCount)
        {
            Threadpool::Post([&]
            {
                auto error = client->Start();
                Trace.WriteInfo(TraceType, "start returned {0}", error);
                VERIFY_IS_TRUE(error.IsSuccess() || error.IsError(ErrorCodeValue::InvalidState));
                if (--count == 0)
                {
                    completed.Set();
                }
            });
        }

        VERIFY_IS_TRUE(completed.WaitOne(TimeSpan::FromSeconds(10)));

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(TargetIdentityTest)
    {
        auto transport = TcpDatagramTransport::CreateClient();
        VERIFY_IS_TRUE(transport->Start().IsSuccess());

        auto target1 = transport->ResolveTarget(L"localhost:20000", L"");
        VERIFY_IS_TRUE(target1);

        auto target2 = transport->ResolveTarget(L"localhost:20000", L"");
        VERIFY_IS_TRUE(target2);
        VERIFY_IS_TRUE(target2 == target1);

        auto target3 = transport->ResolveTarget(L"localhost:20000/blahblah", L"");
        VERIFY_IS_TRUE(target3);
        VERIFY_IS_TRUE(target3 == target1);

        auto target4 = transport->ResolveTarget(L"localhost:20001", L"");
        VERIFY_IS_TRUE(target4);
        VERIFY_IS_FALSE(target4 == target1);

        auto target5 = transport->ResolveTarget(L"10.0.0.1:20000", L"");
        VERIFY_IS_TRUE(target5);
        VERIFY_IS_FALSE(target5 == target1);

        auto target6 = transport->ResolveTarget(L"www.bing.com:80", L"");
        VERIFY_IS_TRUE(target6);

        auto target7 = transport->ResolveTarget(L"WWW.Bing.com:80", L"");
        VERIFY_IS_TRUE(target7);
        VERIFY_IS_TRUE(target6 == target7);

        VERIFY_IS_TRUE(static_cast<TcpDatagramTransport*>(transport.get())->SendTargetCount() == 4);

        transport->Stop();
    }

    BOOST_AUTO_TEST_CASE(IdleTimeoutTestBasic)
    {
        ENTER;
        IdleTimeoutTest(true, false);
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(IdleTimeoutTestWithExternalRefCount)
    {
        ENTER;
        IdleTimeoutTest(true, true);
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(TcpGracefulShutdownByClient)
    {
        ENTER;

#ifdef DBG
        auto sendShutdownBefore = TcpConnection::socketSendShutdownCount_.load();
        auto receiveDrainedBefore = TcpConnection::socketReceiveDrainedCount_.load();
#endif

        auto client = TcpDatagramTransport::CreateClient();
        VERIFY_IS_TRUE(client->Start().IsSuccess());

        auto server = TcpDatagramTransport::Create(TTestUtil::GetListenAddress());

        wstring testAction = TTestUtil::GetGuidAction();
        AutoResetEvent messageReceived;
        TTestUtil::SetMessageHandler(
            server,
            testAction,
            [testAction, &messageReceived](MessageUPtr & msg, ISendTarget::SPtr const & st) -> void
        {
            Trace.WriteInfo(TraceType, "server received message {0} from {1}", msg->TraceId(), st->Address());
            messageReceived.Set();
        });

        VERIFY_IS_TRUE(server->Start().IsSuccess());

        ISendTarget::SPtr target = client->ResolveTarget(server->ListenAddress());
        VERIFY_IS_TRUE(target);

        auto oneway(make_unique<Message>());
        oneway->Headers.Add(ActionHeader(testAction));
        oneway->Headers.Add(MessageIdHeader());
        Trace.WriteInfo(TraceType, "client sending {0}", oneway->MessageId);
        client->SendOneWay(target, std::move(oneway));

        Trace.WriteInfo(TraceType, "waiting for messageReceived");
        bool receivedMessageOne = messageReceived.WaitOne(TimeSpan::FromSeconds(5));
        VERIFY_IS_TRUE(receivedMessageOne);

        Trace.WriteInfo(TraceType, "closing the connection from client side");
        target->Reset();

        Trace.WriteInfo(TraceType, "waiting for connection object destruction");
        VERIFY_IS_TRUE(TTestUtil::WaitUntil([] { return TcpConnection::GetObjCount() == 0; }, TimeSpan::FromSeconds(3)));

#ifdef DBG
        auto sendShutdownAfter = TcpConnection::socketSendShutdownCount_.load();
        auto receiveDrainedAfter = TcpConnection::socketReceiveDrainedCount_.load();

        VERIFY_ARE_EQUAL2(sendShutdownAfter - sendShutdownBefore, 2);
        VERIFY_ARE_EQUAL2(receiveDrainedAfter - receiveDrainedBefore, 2);
#endif

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(TcpGracefulShutdownByServer)
    {
        ENTER;

#ifdef DBG
        auto sendShutdownBefore = TcpConnection::socketSendShutdownCount_.load();
        auto receiveDrainedBefore = TcpConnection::socketReceiveDrainedCount_.load();
#endif

        auto client = TcpDatagramTransport::CreateClient();
        VERIFY_IS_TRUE(client->Start().IsSuccess());

        auto server = TcpDatagramTransport::Create(TTestUtil::GetListenAddress());

        wstring testAction = TTestUtil::GetGuidAction();
        AutoResetEvent messageReceived;
        TTestUtil::SetMessageHandler(
            server,
            testAction,
            [testAction, &messageReceived](MessageUPtr & msg, ISendTarget::SPtr const & st) -> void
        {
            Trace.WriteInfo(TraceType, "server received message {0} from {1}", msg->TraceId(), st->Address());
            messageReceived.Set();

            Trace.WriteInfo(TraceType, "closing the connection from server side");
            st->Reset();
        });

        VERIFY_IS_TRUE(server->Start().IsSuccess());

        ISendTarget::SPtr target = client->ResolveTarget(server->ListenAddress());
        VERIFY_IS_TRUE(target);

        auto oneway(make_unique<Message>());
        oneway->Headers.Add(ActionHeader(testAction));
        oneway->Headers.Add(MessageIdHeader());
        Trace.WriteInfo(TraceType, "client sending {0}", oneway->MessageId);
        client->SendOneWay(target, std::move(oneway));

        Trace.WriteInfo(TraceType, "waiting for messageReceived");
        bool receivedMessageOne = messageReceived.WaitOne(TimeSpan::FromSeconds(5));
        VERIFY_IS_TRUE(receivedMessageOne);

        Trace.WriteInfo(TraceType, "waiting for connection object destruction");
        VERIFY_IS_TRUE(TTestUtil::WaitUntil([] { return TcpConnection::GetObjCount() == 0; }, TimeSpan::FromSeconds(3)));

#ifdef DBG
        auto sendShutdownAfter = TcpConnection::socketSendShutdownCount_.load();
        auto receiveDrainedAfter = TcpConnection::socketReceiveDrainedCount_.load();

        VERIFY_ARE_EQUAL2(sendShutdownAfter - sendShutdownBefore, 2);
        VERIFY_ARE_EQUAL2(receiveDrainedAfter - receiveDrainedBefore, 2);
#endif

        LEAVE;
    }


    BOOST_AUTO_TEST_CASE(SimpleIPv4TcpTest)
    {
        ENTER;
        SimpleTcpTest(TTestUtil::GetListenAddress(), TTestUtil::GetListenAddress());
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(SimpleIPv6TcpTest)
    {
        ENTER;
        SimpleTcpTest(
            TTestUtil::GetListenAddress(ListenAddressType::LoopbackIpv6),
            TTestUtil::GetListenAddress(L"[0:0:0:0:0:0:0:1]"));
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(SimpleTcpTestWithLocalhost)
    {
        ENTER;
        SimpleTcpTest(L"localhost:0", L"localhost:0"); // Test dynamic listen port
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(SimpleTcpTestWithLocalhost1)
    {
        ENTER;
        SimpleTcpTest(TTestUtil::GetListenAddress(ListenAddressType::Localhost), TTestUtil::GetListenAddress(ListenAddressType::Localhost));
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(SimpleTcpTestWithLocalhost2)
    {
        ENTER;
        SimpleTcpTest(L"localhost:0", L"127.0.0.1:0"); // Test dynamic listen port
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(SimpleTcpTestWithLocalhost3)
    {
        ENTER;
        SimpleTcpTest(TTestUtil::GetListenAddress(ListenAddressType::LoopbackIpv4), TTestUtil::GetListenAddress(ListenAddressType::Localhost));
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(SimpleTcpTestWithLocalFqdn)
    {
        ENTER;
        wstring thisHost;
        VERIFY_IS_TRUE(TcpTransportUtility::GetLocalFqdn(thisHost).IsSuccess());
        thisHost += L":0";
        SimpleTcpTest(thisHost, thisHost);
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(SimpleTcpTestWithLocalFqdnResolvedToIPv4Only)
    {
        ENTER;
        TransportConfig & config = TransportConfig::GetConfig();
        wstring savedResolveOption = config.ResolveOption;
        config.ResolveOption = L"ipv4";
        KFinally([&] { config.ResolveOption = savedResolveOption; });

        SimpleTcpTest(TTestUtil::GetListenAddress(ListenAddressType::Fqdn), TTestUtil::GetListenAddress(ListenAddressType::Fqdn));

        LEAVE;
    }

//LINUXTODO
#ifndef PLATFORM_UNIX
    BOOST_AUTO_TEST_CASE(SimpleTcpTestWithLocalFqdnResolvedToIPv6Only)
    {
        ENTER;
        TransportConfig & config = TransportConfig::GetConfig();
        wstring savedResolveOption = config.ResolveOption;
        config.ResolveOption = L"ipv6";
        KFinally([&] { config.ResolveOption = savedResolveOption; });

        SimpleTcpTest(TTestUtil::GetListenAddress(ListenAddressType::Fqdn), TTestUtil::GetListenAddress(ListenAddressType::Fqdn));

        LEAVE;
    }
#endif

    BOOST_AUTO_TEST_CASE(MixedAddressTestWithIPv4Receiver)
    {
        ENTER;
        SimpleTcpTest(L"[::1]:0", L"127.0.0.1:0");
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(MixedAddressTestWithIPv6Receiver)
    {
        ENTER;
        SimpleTcpTest(L"127.0.0.1:0", L"[::1]:0");
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(TestListenPort0WithIPv6)
    {
        ENTER;
        SimpleTcpTest(L"[::1]:0", L"[::1]:0");
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(TestListenPort0WithIPv4)
    {
        ENTER;
        SimpleTcpTest(L"127.0.0.1:0", L"127.0.0.1:0");
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(DrainCompleteTest)
    {
        ENTER;
        this->DrainTest(1024*1024, true, TimeSpan::FromMinutes(5));
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(DrainFailTest)
    {
        ENTER;
        this->DrainTest(10*1024*1024, false, TimeSpan::FromSeconds(0));
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(DrainFailTest2)
    {
        ENTER;
        this->DrainTest(1024 * 1024, false, TimeSpan::FromMilliseconds(1000));
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(BindToSameAddress)
    {
        ENTER;

        std::wstring ipv4Address = TTestUtil::GetListenAddress(ListenAddressType::LoopbackIpv4);
        std::wstring ipv6Address = TTestUtil::GetListenAddress(ListenAddressType::LoopbackIpv6);

        auto ipv4Sender1 = TcpDatagramTransport::Create(ipv4Address);
        VERIFY_IS_TRUE(ipv4Sender1->Start().IsSuccess());

        auto ipv4Sender2 = TcpDatagramTransport::Create(ipv4Address);
        VERIFY_IS_FALSE(ipv4Sender2->Start().IsSuccess());

        auto ipv6Sender1 = TcpDatagramTransport::Create(ipv6Address);
        VERIFY_IS_TRUE(ipv6Sender1->Start().IsSuccess());

        auto ipv6Sender2 = TcpDatagramTransport::Create(ipv6Address);
        VERIFY_IS_FALSE(ipv6Sender2->Start().IsSuccess());

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(SimpleTcpTestWithAbortInCallback)
    {
        ENTER;

        auto sender = TcpDatagramTransport::Create(TTestUtil::GetListenAddress());
        auto receiver = TcpDatagramTransport::Create(TTestUtil::GetListenAddress());

        AutoResetEvent replyReceived;
        sender->SetMessageHandler([&replyReceived, &sender](MessageUPtr &, ISendTarget::SPtr const &) -> void
        {
            Trace.WriteInfo(TraceType, "[sender] got a message");

            sender->Stop();

            replyReceived.Set();
        });

        AutoResetEvent messageReceived;
        receiver->SetMessageHandler([&receiver, &sender, &messageReceived](MessageUPtr &, ISendTarget::SPtr const & target) -> void
        {
            Trace.WriteInfo(TraceType, "[receiver] got a message");

            receiver->SendOneWay(target, make_unique<Message>());

            Sleep(1000);

            receiver->Stop();

            messageReceived.Set();
        });

        VERIFY_IS_TRUE(sender->Start().IsSuccess());
        VERIFY_IS_TRUE(receiver->Start().IsSuccess());

        auto target = sender->ResolveTarget(receiver->ListenAddress());

        TcpTestMessage body(L"Message message message message oh yeah");
        auto oneway(make_unique<Message>(body));
        sender->SendOneWay(target, std::move(oneway));

        bool receivedMessageOne = messageReceived.WaitOne(TimeSpan::FromSeconds(30));
        VERIFY_IS_TRUE(receivedMessageOne);

        bool receivedReplyOne = replyReceived.WaitOne(TimeSpan::FromSeconds(30));
        VERIFY_IS_TRUE(receivedReplyOne);

        Sleep(3000); // wait for the Receiver to react to the connection break

        target.reset();
        VERIFY_IS_TRUE(sender->SendTargetCount() == 0);
        VERIFY_IS_TRUE(receiver->SendTargetCount() == 0);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(SimpleTcpTestWithUserBuffer)
    {
        ENTER;

        auto sender = TcpDatagramTransport::Create(TTestUtil::GetListenAddress());
        auto receiver = TcpDatagramTransport::Create(TTestUtil::GetListenAddress());
        auto myString = std::wstring(L"Message message message message oh yeah");

        AutoResetEvent replyReceived;
        sender->SetMessageHandler([&replyReceived](MessageUPtr &, ISendTarget::SPtr const &) -> void
        {
            Trace.WriteInfo(TraceType, "[sender] got a message");

            replyReceived.Set();
        });

        AutoResetEvent messageReceived;
        wstring testAction = TTestUtil::GetGuidAction();
        TTestUtil::SetMessageHandler(
            receiver,
            testAction,
            [&receiver, &sender, &messageReceived, &myString](MessageUPtr & message, ISendTarget::SPtr const & st) -> void
            {
                Trace.WriteInfo(TraceType, "[receiver] got a message {0} {1}", sender->ListenAddress(), st->Address());

                VERIFY_IS_TRUE(sender->ListenAddress() == st->Address());

                vector<const_buffer> buffers;
                message->GetBody(buffers);

                size_t bufferSizeTotal = buffers[0].len;
                for (int i = 1; i < buffers.size(); ++ i)
                {
                    bufferSizeTotal += buffers[i].len;
                }

                VERIFY_IS_TRUE(message->SerializedBodySize() == bufferSizeTotal);

                size_t offset = 0;
                byte* stringBuffer = (byte*)myString.c_str();
                for (int i = 0; i < buffers.size(); ++ i)
                {
                    VERIFY_IS_TRUE(memcmp(stringBuffer + offset, buffers[i].buf, buffers[i].len) == 0);
                    offset += buffers[i].len;
                }

                messageReceived.Set();

                // ping back
                receiver->SendOneWay(st, make_unique<Message>());
            });

        VERIFY_IS_TRUE(sender->Start().IsSuccess());
        VERIFY_IS_TRUE(receiver->Start().IsSuccess());

        ISendTarget::SPtr target = sender->ResolveTarget(receiver->ListenAddress());
        VERIFY_IS_TRUE(target);

        bool receivedReplyOne = replyReceived.WaitOne(TimeSpan::FromSeconds(1));
        VERIFY_IS_FALSE(receivedReplyOne);

        AutoResetEvent messageSent(false);

        std::vector<const_buffer> buffers;
        const_buffer myBuffer(myString.c_str(), myString.size() * 2 + 2);
        buffers.push_back(myBuffer);

        auto oneway = make_unique<Message>(buffers,
            [&myString, &messageSent](std::vector<const_buffer> const & buffers, void * state)
            {
                VERIFY_IS_TRUE(state == &myString);
                VERIFY_IS_TRUE(buffers.size() == 1);
                VERIFY_IS_TRUE(buffers[0].buf == (char *)myString.c_str());

                std::wstring receiverString((wchar_t*)buffers[0].buf);

                VERIFY_IS_TRUE(receiverString == myString);
                VERIFY_IS_TRUE(receiverString.c_str() != myString.c_str());

                messageSent.Set();
            },
            &myString);

        oneway->Headers.Add(ActionHeader(testAction));
        oneway->Headers.Add(MessageIdHeader());
        sender->SendOneWay(target, std::move(oneway));

        VERIFY_IS_TRUE(messageSent.WaitOne(4000));

        bool receivedMessageOne = messageReceived.WaitOne(TimeSpan::FromSeconds(30));
        VERIFY_IS_TRUE(receivedMessageOne);

        receivedReplyOne = replyReceived.WaitOne(TimeSpan::FromSeconds(30));
        VERIFY_IS_TRUE(receivedReplyOne);

        sender->Stop();
        receiver->Stop();

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(ClientModeTestWithIPv4Server)
    {
        ENTER;
        ClientTcpTest(TTestUtil::GetListenAddress());
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(ClientModeTestWithIPv6Server)
    {
        ENTER;
        ClientTcpTest(TTestUtil::GetListenAddress(ListenAddressType::LoopbackIpv6));
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(RequestSendFailureTest)
    {
        ENTER;

        auto sender = TcpDatagramTransport::CreateClient();
        VERIFY_IS_TRUE(sender->Start().IsSuccess());

        auto noneListening = sender->ResolveTarget(TTestUtil::GetListenAddress());
        VERIFY_IS_TRUE(noneListening != nullptr);

        auto rrw = RequestReplyWrapper::Create(sender);
        auto requestReply = rrw->InitRequestReply();

        auto request = make_unique<Message>();
        request->Headers.Add(MessageIdHeader());

        ManualResetEvent requestCompleted(false);
        ErrorCode error;
        TimeSpan timeout = TimeSpan::FromSeconds(20);
        requestReply->BeginRequest(
            move(request),
            noneListening,
            timeout,
            [requestReply, &requestCompleted, &error](AsyncOperationSPtr const & operation)
            {
                MessageUPtr reply;
                error = requestReply->EndRequest(operation, reply);
                requestCompleted.Set();
            },
            rrw->CreateAsyncOperationRoot());

        VERIFY_IS_TRUE(requestCompleted.WaitOne(timeout));
        Trace.WriteInfo(TraceType, "request&reply result: {0}", error);
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::CannotConnect));

        sender->Stop();
        requestReply->Close();

        LEAVE;
    }


    BOOST_AUTO_TEST_CASE(RequestReplyQueueFullTest)
    {
        ENTER;
        auto receiver = TcpDatagramTransport::Create(TTestUtil::GetListenAddress());

        auto sender = TcpDatagramTransport::CreateClient();
        VERIFY_IS_TRUE(sender->Start().IsSuccess());
        const ULONG sendQueueLimit = 1024;
        sender->SetPerTargetSendQueueLimit(sendQueueLimit);
        ULONG messageBodySize = sendQueueLimit * 2;
        auto request = (make_unique<Message>(TestMessageBody(messageBodySize)));
        request->Headers.Add(MessageIdHeader());
        VERIFY_IS_TRUE(receiver->Start().IsSuccess());
        auto address = sender->ResolveTarget(receiver->ListenAddress());
        VERIFY_IS_TRUE(address != nullptr);

        auto rrw = RequestReplyWrapper::Create(sender);
        auto requestReply = rrw->InitRequestReply();

        ManualResetEvent requestCompleted(false);
        ErrorCode error;
        TimeSpan timeout = TimeSpan::FromSeconds(20);
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
        Trace.WriteInfo(TraceType, "request&reply result: {0}", error);
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::TransportSendQueueFull));

        sender->Stop();
        requestReply->Close();
        receiver->Stop();

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(RequestReplyTimeoutWithSendFailed)
    {
        ENTER;

        auto saved = TransportConfig::GetConfig().EventLoopCleanupDelay;
        TransportConfig::GetConfig().EventLoopCleanupDelay = TimeSpan::FromSeconds(3);
        KFinally([=] { TransportConfig::GetConfig().EventLoopCleanupDelay = saved; }); 

        auto sender = TcpDatagramTransport::CreateClient();
        VERIFY_IS_TRUE(sender->Start().IsSuccess());

        auto request = (make_unique<Message>(TestMessageBody()));
        request->Headers.Add(MessageIdHeader());
        auto address = sender->ResolveTarget(L"www.bing.com:23", L"");

        VERIFY_IS_TRUE(address != nullptr);

        auto rrw = RequestReplyWrapper::Create(sender);
        auto requestReply = rrw->InitRequestReply();

        requestReply->EnableDifferentTimeoutError();
        ManualResetEvent requestCompleted(false);
        ErrorCode error;
        TimeSpan timeout = TimeSpan::FromSeconds(1);
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

        VERIFY_IS_TRUE(requestCompleted.WaitOne(TimeSpan::FromSeconds(10)));
        Trace.WriteInfo(TraceType, "request&reply result: {0}", error);
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::SendFailed) || error.IsError(ErrorCodeValue::CannotConnect));

        sender->Stop();
        requestReply->Close();
        
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(RequestReplyTimeoutTestWhenReplyNotRecieved)
    {
        ENTER;
        auto receiver = TcpDatagramTransport::Create(TTestUtil::GetListenAddress());

        receiver->SetMessageHandler([](MessageUPtr &, ISendTarget::SPtr const &) -> void
        {
            Sleep(30000);
        });
        auto sender = TcpDatagramTransport::CreateClient();
        VERIFY_IS_TRUE(sender->Start().IsSuccess());
        auto request = (make_unique<Message>());
        request->Headers.Add(MessageIdHeader());
        VERIFY_IS_TRUE(receiver->Start().IsSuccess());
        auto address = sender->ResolveTarget(receiver->ListenAddress());
        VERIFY_IS_TRUE(address != nullptr);

        auto rrw = RequestReplyWrapper::Create(sender);
        auto requestReply = rrw->InitRequestReply();

        ManualResetEvent requestCompleted(false);
        ErrorCode error;
        TimeSpan timeout = TimeSpan::FromSeconds(20);
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

        VERIFY_IS_TRUE(requestCompleted.WaitOne(TimeSpan::FromSeconds(50)));
        Trace.WriteInfo(TraceType, "request&reply result: {0}", error);
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::Timeout));

        sender->Stop();
        requestReply->Close();

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(RequestReply_ConnectionClosedByRemoteAfterRequestSent)
    {
        ENTER;

        auto faultyReceiver = TcpDatagramTransport::Create(TTestUtil::GetListenAddress());
        wstring testAction = TTestUtil::GetGuidAction();
        TTestUtil::SetMessageHandler(
            faultyReceiver,
            testAction,
            [testAction](MessageUPtr & msg, ISendTarget::SPtr const & st) -> void
            {
                Trace.WriteInfo(TraceType, "faulty receiver received reqeust {0} from {1}, will reset connection", msg->TraceId(), st->Address());
                st->Reset();
            });

        VERIFY_IS_TRUE(faultyReceiver->Start().IsSuccess());

        auto goodReceiver = TcpDatagramTransport::Create(TTestUtil::GetListenAddress());
        TTestUtil::SetMessageHandler(
            goodReceiver,
            testAction,
            [goodReceiver, testAction](MessageUPtr & msg, ISendTarget::SPtr const & st) -> void
            {
                Trace.WriteInfo(TraceType, "good receiver received reqeust {0} from {1}, will reply after delay", msg->TraceId(), st->Address());
                auto reply = make_unique<Message>();
                reply->Headers.Add(RelatesToHeader(msg->MessageId));
                ::Sleep(100);
                goodReceiver->SendOneWay(st, move(reply));
            });

        VERIFY_IS_TRUE(goodReceiver->Start().IsSuccess());

        auto sender = TcpDatagramTransport::CreateClient();
        auto senderRootSPtr = make_shared<ComponentRoot>();

        auto rrw = RequestReplyWrapper::Create(sender);
        auto requestReply = rrw->InitRequestReply();

        auto senderDemuxer = make_shared<Demuxer>(*senderRootSPtr, sender);
        senderDemuxer->SetReplyHandler(*requestReply);

        VERIFY_IS_TRUE(senderDemuxer->Open().IsSuccess());
        VERIFY_IS_TRUE(sender->Start().IsSuccess());

        auto goodTarget = sender->ResolveTarget(goodReceiver->ListenAddress());
        VERIFY_IS_TRUE(goodTarget != nullptr);

        ManualResetEvent requestSucceeded(false);
        ErrorCode requestResult;
        TimeSpan testWait = TimeSpan::FromSeconds(10);
        TimeSpan timeout = testWait + TimeSpan::FromSeconds(5);

        auto request = make_unique<Message>();
        request->Headers.Add(MessageIdHeader());
        request->Headers.Add(ActionHeader(testAction));

        requestReply->BeginRequest(
            move(request),
            goodTarget,
            timeout,
            [requestReply, &requestSucceeded, &requestResult](AsyncOperationSPtr const & operation)
            {
                MessageUPtr reply;
                requestResult = requestReply->EndRequest(operation, reply);
                requestSucceeded.Set();
            },
            rrw->CreateAsyncOperationRoot());

        auto faultyTarget = sender->ResolveTarget(faultyReceiver->ListenAddress());
        VERIFY_IS_TRUE(faultyTarget != nullptr);

        auto requestWillFail = make_unique<Message>();
        requestWillFail->Headers.Add(MessageIdHeader());
        requestWillFail->Headers.Add(ActionHeader(testAction));

        ManualResetEvent requestFailed(false);
        ErrorCode error;
        requestReply->BeginRequest(
            move(requestWillFail),
            faultyTarget,
            timeout,
            [requestReply, &requestFailed, &error](AsyncOperationSPtr const & operation)
            {
                MessageUPtr reply;
                error = requestReply->EndRequest(operation, reply);
                requestFailed.Set();
            },
            rrw->CreateAsyncOperationRoot());

        VERIFY_IS_TRUE(requestFailed.WaitOne(testWait));
        Trace.WriteInfo(TraceType, "request to faulty receiver: result: {0}", error);
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::ConnectionClosedByRemoteEnd));

        VERIFY_IS_TRUE(requestSucceeded.WaitOne(testWait));

        goodReceiver->Stop();
        faultyReceiver->Stop();

        VERIFY_IS_TRUE(senderDemuxer->Close().IsSuccess());
        requestReply->Close();
        sender->Stop();

        // connection fault with faulty receiver should not affect request-reply with the good receiver
        Trace.WriteInfo(TraceType, "request to good receiver: result: {0}", requestResult);
        VERIFY_IS_TRUE(requestResult.IsSuccess());

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(DuplexRequestReplyTest)
    {
        LONG notificationCount = 100;

        auto sender = TcpDatagramTransport::CreateClient();
        auto senderRootSPtr = make_shared<ComponentRoot>();

        auto rrw = DuplexRequestReplyWrapper::Create(sender);
        auto senderRequestReply = rrw->InitRequestReply();

        auto senderDemuxer = make_shared<Demuxer>(*senderRootSPtr, sender);
        senderDemuxer->SetReplyHandler(*senderRequestReply);

        VERIFY_IS_TRUE(senderDemuxer->Open().IsSuccess());
        VERIFY_IS_TRUE(sender->Start().IsSuccess());

        auto receiver = TcpDatagramTransport::Create(TTestUtil::GetListenAddress());
        auto receiverRootSPtr = make_shared<ComponentRoot>();

        auto rrw2 = DuplexRequestReplyWrapper::Create(receiver);
        auto receiverRequestReply = rrw2->InitRequestReply();

        auto receiverDemuxer = make_shared<Demuxer>(*receiverRootSPtr, receiver);
        receiverDemuxer->SetReplyHandler(*receiverRequestReply);

        ManualResetEvent notificationReplyEvent(false);
        Common::atomic_long pendingNotificationReplies(notificationCount);

        receiverDemuxer->RegisterMessageHandler(
            Actor::NamingGateway,
            [&](MessageUPtr & msg, ReceiverContextUPtr & receiverContext)
            {
                Trace.WriteInfo(TraceType, "[receiver] got a message {0}", *msg);

                // Normal reply
                receiverContext->Reply(make_unique<Message>());

                // Notifications (uncorrelated replies)
                auto replyTarget = receiverContext->ReplyTarget;
                for (auto ix=0; ix<notificationCount; ++ix)
                {
                    auto notification = make_unique<Message>();

                    ManualResetEvent receiverEvent(false);

                    auto operation = receiverRequestReply->BeginNotification(
                        move(notification),
                        replyTarget,
                        TimeSpan::FromSeconds(10),
                        [&](AsyncOperationSPtr const &) { receiverEvent.Set(); },
                        AsyncOperationSPtr());

                    receiverEvent.WaitOne();

                    MessageUPtr receiverReply;
                    auto error = receiverRequestReply->EndNotification(operation, receiverReply);

                    VERIFY_IS_TRUE(error.IsSuccess());
                    VERIFY_IS_TRUE(receiverReply);

                    Trace.WriteInfo(TraceType, "[receiver] got a reply {0}", *receiverReply);

                    if (--pendingNotificationReplies == 0)
                    {
                        notificationReplyEvent.Set();
                    }
                }
            },
            false);

        VERIFY_IS_TRUE(receiverDemuxer->Open().IsSuccess());
        VERIFY_IS_TRUE(receiver->Start().IsSuccess());

        ManualResetEvent notificationEvent(false);
        Common::atomic_long pendingNotifications(notificationCount);
        senderRequestReply->SetNotificationHandler([&](Message & msg, ReceiverContextUPtr & receiverContext)
        {
            Trace.WriteInfo(TraceType, "[sender] got a notification {0}", msg);

            // Notification reply
            receiverContext->Reply(make_unique<Message>());

            if (--pendingNotifications == 0)
            {
                notificationEvent.Set();
            }
        },
        true);

        auto sendTarget = sender->ResolveTarget(receiver->ListenAddress());
        VERIFY_IS_TRUE(sendTarget);

        ManualResetEvent requestReplyEvent(false);

        // Send initial request, which will trigger notifications
        auto msg = make_unique<Message>();
        msg->Headers.Add(ActorHeader(Actor::NamingGateway));

        auto operation = senderRequestReply->BeginRequest(
            move(msg),
            sendTarget,
            TimeSpan::FromSeconds(10),
            [&](AsyncOperationSPtr const &) { requestReplyEvent.Set(); },
            AsyncOperationSPtr());

        requestReplyEvent.WaitOne();

        MessageUPtr reply;
        auto error = senderRequestReply->EndRequest(operation, reply);

        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(reply);

        Trace.WriteInfo(TraceType, "[sender] got a reply {0}", *reply);

        notificationEvent.WaitOne();

        Trace.WriteInfo(TraceType, "[sender] got {0} notifications", notificationCount);

        senderRequestReply->RemoveNotificationHandler();

        notificationReplyEvent.WaitOne();

        Trace.WriteInfo(TraceType, "[receiver] got {0} notification replies", notificationCount);

        receiver->Stop();
        VERIFY_IS_TRUE(receiverDemuxer->Close().IsSuccess());
        receiverRequestReply->Close();

        sender->Stop();
        VERIFY_IS_TRUE(senderDemuxer->Close().IsSuccess());
        senderRequestReply->Close();
    }


    BOOST_AUTO_TEST_CASE(ConnectionFaultTest_ConnectionRefused)
    {
        ENTER;

        auto sender = TcpDatagramTransport::Create(TTestUtil::GetListenAddress());

        ManualResetEvent waitHandle(false);
        std::wstring invalidAddress = TTestUtil::GetListenAddress();
        sender->SetConnectionFaultHandler(
            [&waitHandle, &invalidAddress](ISendTarget const & target, ErrorCode lastError) -> void
            {
                // ERROR_CONNECTION_REFUSED = 1225 = The remote computer refused the network connection.
                VERIFY_IS_TRUE(lastError.IsError(ErrorCodeValue::CannotConnect));
                VERIFY_IS_TRUE(invalidAddress == target.Address());
                waitHandle.Set();
            });

        VERIFY_IS_TRUE(sender->Start().IsSuccess());

        auto target = sender->ResolveTarget(invalidAddress);
        VERIFY_IS_TRUE(target);

        TcpTestMessage body(L"Message message message message oh yeah");
        auto oneway(make_unique<Message>(body));
        sender->SendOneWay(target, std::move(oneway));

        VERIFY_IS_TRUE(waitHandle.WaitOne(TimeSpan::FromSeconds(30)));

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(ConnectionFaultTest_SenderCloseConnection)
    {
        ENTER;
        ConnectionFaultTest_SenderClose(false);
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(ConnectionFaultTest_SenderAbortConnection)
    {
        ENTER;
        ConnectionFaultTest_SenderClose(true);
        LEAVE;
    }


    BOOST_AUTO_TEST_CASE(ConnectionFaultTest_ReceiverCloseConnection)
    {
        ENTER;
        ConnectionFaultTest_ReceiverClose(false);
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(ConnectionFaultTest_ReceiverAbortConnection)
    {
        ENTER;
        ConnectionFaultTest_ReceiverClose(true);
        LEAVE;
    }


    BOOST_AUTO_TEST_CASE(ManyMessagesTest)
    {
        ENTER;

        LONG TotalMessageCount = 100;
        LONG messageCount = 0;

        auto sender = TcpDatagramTransport::Create(TTestUtil::GetListenAddress());
        auto receiver = TcpDatagramTransport::Create(TTestUtil::GetListenAddress());

        AutoResetEvent messagesReceived;

        receiver->SetMessageHandler([&messageCount, &messagesReceived, TotalMessageCount](MessageUPtr &, ISendTarget::SPtr const &) -> void
        {
            auto count = InterlockedIncrement((volatile LONG *)&messageCount);

            if (count == TotalMessageCount)
            {
                messagesReceived.Set();
            }
        });

        VERIFY_IS_TRUE(receiver->Start().IsSuccess());
        VERIFY_IS_TRUE(sender->Start().IsSuccess());

        ISendTarget::SPtr target = sender->ResolveTarget(receiver->ListenAddress());
        VERIFY_IS_TRUE(target);

        for (LONG i = 0; i < TotalMessageCount; ++i)
        {
            TcpTestMessage body(L"Message message message message oh yeah");
            auto oneway(make_unique<Message>(body));
            sender->SendOneWay(target, std::move(oneway));
        }

        bool receivedMessages = messagesReceived.WaitOne(TimeSpan::FromSeconds(30));
        VERIFY_IS_TRUE(receivedMessages);
            
        VERIFY_IS_TRUE(messageCount == TotalMessageCount);

        sender->Stop();
        receiver->Stop();

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(AbortReceiver)
    {
        ENTER;

        const int TotalMessageCount = 50;

        auto sender = TcpDatagramTransport::Create(TTestUtil::GetListenAddress());
        auto receiver = TcpDatagramTransport::Create(TTestUtil::GetListenAddress());

        AutoResetEvent messagesSentWaitHandle;
        AutoResetEvent readyToAbortWaitHandle;

        receiver->SetMessageHandler([](MessageUPtr &, ISendTarget::SPtr const &) -> void
        {
            // No Need to do anything
        });

        VERIFY_IS_TRUE(sender->Start().IsSuccess());
        VERIFY_IS_TRUE(receiver->Start().IsSuccess());

        ISendTarget::SPtr target = sender->ResolveTarget(receiver->ListenAddress());
        VERIFY_IS_TRUE(target);
        Common::Threadpool::Post(
            [sender, target, &readyToAbortWaitHandle, &messagesSentWaitHandle, TotalMessageCount]() mutable
            {
                for (int i = 0; i < TotalMessageCount; ++i)
                {
                    TcpTestMessage body(L"Message message message message oh yeah");
                    auto oneway(make_unique<Message>(body));
                    sender->SendOneWay(target, std::move(oneway));
                    Sleep(100);

                    if (i == TotalMessageCount / 2)
                    {
                        readyToAbortWaitHandle.Set();
                    }
                }

                target = nullptr;
                sender = nullptr;

                messagesSentWaitHandle.Set();
            });

        bool readyToAbort = readyToAbortWaitHandle.WaitOne(TimeSpan::FromSeconds(30));
        VERIFY_IS_TRUE(readyToAbort);

        receiver->Stop();

        bool sentMessages = messagesSentWaitHandle.WaitOne(TimeSpan::FromSeconds(30));
        VERIFY_IS_TRUE(sentMessages);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(AbortReceiverWithManyClients)
    {
        ENTER;

        const int TotalClientCount = 50;
        const int TotalMessageCount = 50;

        auto receiver = TcpDatagramTransport::Create(TTestUtil::GetListenAddress());

        std::vector<TcpDatagramTransportSPtr> clients;
        clients.reserve(TotalClientCount);
        std::vector<ISendTarget::SPtr> targets;
        targets.reserve(TotalClientCount);

        for (int i = 0; i < TotalClientCount; ++i)
        {
            auto sender = TcpDatagramTransport::CreateClient();
            VERIFY_IS_TRUE(sender->Start().IsSuccess());

            clients.push_back(sender);

            auto target = sender->ResolveTarget(receiver->ListenAddress());
            targets.push_back(target);
        }

        AutoResetEvent messagesSentWaitHandle;
        AutoResetEvent readyToAbortWaitHandle;

        receiver->SetMessageHandler([](MessageUPtr &, ISendTarget::SPtr const &) -> void
        {
            // No Need to do anything
        });

        VERIFY_IS_TRUE(receiver->Start().IsSuccess());

        Common::Threadpool::Post(
            [&clients, &targets, &readyToAbortWaitHandle, &messagesSentWaitHandle, TotalMessageCount, TotalClientCount]() mutable
            {
                for (int i = 0; i < TotalMessageCount; ++i)
                {
                    for (int j = 0; j < TotalClientCount; ++j)
                    {
                        TcpTestMessage body(L"Message message message message oh yeah");
                        auto oneway(make_unique<Message>(body));
                        clients[j]->SendOneWay(targets[j], std::move(oneway));
                    }

                    Sleep(100);

                    if (i == TotalMessageCount / 2)
                    {
                        readyToAbortWaitHandle.Set();
                    }
                }

                targets.clear();
                clients.clear();

                messagesSentWaitHandle.Set();
            });

        bool readyToAbort = readyToAbortWaitHandle.WaitOne(TimeSpan::FromSeconds(30));
        VERIFY_IS_TRUE(readyToAbort);

        receiver->Stop();

        bool sentMessages = messagesSentWaitHandle.WaitOne(TimeSpan::FromSeconds(30));
        VERIFY_IS_TRUE(sentMessages);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(ConnectionRecoveryTest)
    {
        ENTER;

        auto sender = TcpDatagramTransport::Create(TTestUtil::GetListenAddress());
        auto receiver = TcpDatagramTransport::Create(TTestUtil::GetListenAddress());

        ManualResetEvent messageReceived(false);
        ManualResetEvent replyReceived(false);

        sender->SetMessageHandler([&replyReceived](MessageUPtr &, ISendTarget::SPtr const &) -> void
        {
            Trace.WriteInfo(TraceType, "[sender] got a reply");
            replyReceived.Set();
        });

        receiver->SetMessageHandler([&sender, &receiver,&messageReceived](MessageUPtr &, ISendTarget::SPtr const & replyTarget) -> void
        {
            // break the connection before sending the reply
            Trace.WriteInfo(TraceType, "[receiver] got a message");

            replyTarget->TargetDown(sender->Instance());

            // Wait for connection clean up to complete before sending reply
            Threadpool::Post([replyTarget, &sender, &receiver, &messageReceived]
            {
                bool allConnectionsClosed = TTestUtil::WaitUntil(
                    [&]
                    {
                        size_t connectionCount = replyTarget->ConnectionCount();
                        Trace.WriteInfo(TraceType, "[receiver] replyTarget connection count: {0}", connectionCount);
                        return connectionCount == 0; 
                    },
                    TimeSpan::FromSeconds(30));

                VERIFY_IS_TRUE(allConnectionsClosed);

                // Need to increment instance to allow new connections to be established
                sender->SetInstance(sender->Instance() + 1);

                Trace.WriteInfo(TraceType, "[receiver] sending reply");
                TcpTestMessage body(L"Message message message message oh yeah");
                auto replyMessage(make_unique<Message>(body));
                receiver->SendOneWay(replyTarget, std::move(replyMessage));
                messageReceived.Set();
            });
        });

        VERIFY_IS_TRUE(sender->Start().IsSuccess());
        VERIFY_IS_TRUE(receiver->Start().IsSuccess());

        ISendTarget::SPtr target = sender->ResolveTarget(receiver->ListenAddress());
        VERIFY_IS_TRUE(target);

        Trace.WriteInfo(TraceType, "[sender] sending request 1");
        TcpTestMessage body(L"Message message message message oh yeah");
        auto testMessage(make_unique<Message>(body));
        sender->SendOneWay(target, std::move(testMessage));

        bool receivedMessages = messageReceived.WaitOne(TimeSpan::FromSeconds(60));
        VERIFY_IS_TRUE(receivedMessages);

        bool receivedReply = replyReceived.WaitOne(TimeSpan::FromSeconds(30));
        VERIFY_IS_TRUE(receivedReply);

        target->TargetDown(receiver->Instance());

        // Wait for connection clean up to complete before sending reply
        for(;;)
        {
            size_t connectionCount = target->ConnectionCount();
            Trace.WriteInfo(TraceType, "[sender] target connection count: {0}", connectionCount);
            if (connectionCount == 0)
            {
                break;
            }

            ::Sleep(30);
        }

        messageReceived.Reset();
        replyReceived.Reset();

        receivedReply = replyReceived.WaitOne(TimeSpan::FromSeconds(1));
        VERIFY_IS_FALSE(receivedReply);

        // Need to increment instance to allow new connections to be established
        receiver->SetInstance(receiver->Instance() + 1);

        Trace.WriteInfo(TraceType, "[sender] sending request 2");
        TcpTestMessage body2(L"Message message message message oh yeah");
        auto testMessage2 = make_unique<Message>(body2);
        sender->SendOneWay(target, std::move(testMessage2));

        receivedMessages = messageReceived.WaitOne(TimeSpan::FromSeconds(30));
        VERIFY_IS_TRUE(receivedMessages);
            
        receivedReply = replyReceived.WaitOne(TimeSpan::FromSeconds(30));
        VERIFY_IS_TRUE(receivedReply);

        sender->Stop();
        receiver->Stop();

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(TargetResetTest)
    {
        ENTER;

        auto receiver = TcpDatagramTransport::Create(TTestUtil::GetListenAddress());

        AutoResetEvent messageReceived;
        receiver->SetMessageHandler([&messageReceived](MessageUPtr & message, ISendTarget::SPtr const & st)
        {
            Trace.WriteInfo(TraceType, "[receiver] got message {0} from {1}", message->MessageId, st->Address());
            messageReceived.Set();
        });

        auto error = receiver->Start();
        VERIFY_IS_TRUE(error.IsSuccess());

        auto sender = TcpDatagramTransport::CreateClient();
        error = sender->Start();
        VERIFY_IS_TRUE(error.IsSuccess());

        auto sendTarget = sender->ResolveTarget(receiver->ListenAddress());
        VERIFY_IS_TRUE(sendTarget != nullptr);

        auto message = make_unique<Message>();
        message->Headers.Add(MessageIdHeader());

        error = sender->SendOneWay(sendTarget, move(message));
        VERIFY_IS_TRUE(error.IsSuccess());

        VERIFY_IS_TRUE(messageReceived.WaitOne(TimeSpan::FromSeconds(30)));

        sendTarget->Reset();

        message = make_unique<Message>();
        message->Headers.Add(MessageIdHeader());

        error = sender->SendOneWay(sendTarget, move(message));
        VERIFY_IS_TRUE(error.IsSuccess());

        VERIFY_IS_TRUE(messageReceived.WaitOne(TimeSpan::FromSeconds(30)));

        sender.reset();
        receiver.reset();

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(TargetCloseTest)
    {
        ENTER;

        auto receiver = TcpDatagramTransport::Create(TTestUtil::GetListenAddress());

        AutoResetEvent messageReceived;
        receiver->SetMessageHandler([&messageReceived](MessageUPtr & message, ISendTarget::SPtr const & st)
        {
            Trace.WriteInfo(TraceType, "[receiver] got message {0} from {1}", message->MessageId, st->Address());
            messageReceived.Set();
        });

        auto error = receiver->Start();
        VERIFY_IS_TRUE(error.IsSuccess());

        auto sender = TcpDatagramTransport::CreateClient();
        error = sender->Start();
        VERIFY_IS_TRUE(error.IsSuccess());

        auto sendTarget = sender->ResolveTarget(receiver->ListenAddress());
        VERIFY_IS_TRUE(sendTarget != nullptr);

        auto message = make_unique<Message>();
        message->Headers.Add(MessageIdHeader());

        error = sender->SendOneWay(sendTarget, move(message));
        VERIFY_IS_TRUE(error.IsSuccess());

        VERIFY_IS_TRUE(messageReceived.WaitOne(TimeSpan::FromSeconds(30)));

        ((TcpSendTarget&)(*sendTarget)).Close();

        message = make_unique<Message>();
        message->Headers.Add(MessageIdHeader());

        error = sender->SendOneWay(sendTarget, move(message));
        VERIFY_IS_FALSE(error.IsSuccess());

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(SendTimeoutTest)
    {
        ENTER;

        auto saved1 = TransportConfig::GetConfig().SendTimeout;
        TransportConfig::GetConfig().SendTimeout = TimeSpan::FromSeconds(1);
        auto saved2 = TransportConfig::GetConfig().ConnectionIdleTimeout;
        TransportConfig::GetConfig().ConnectionIdleTimeout = TimeSpan::FromSeconds(1);
        KFinally([=]
        {
            TransportConfig::GetConfig().SendTimeout = saved1;
            TransportConfig::GetConfig().ConnectionIdleTimeout = saved2;
        });

        auto node1 = TcpDatagramTransport::Create(TTestUtil::GetListenAddress());
        node1->SetMessageHandler([=](MessageUPtr &, ISendTarget::SPtr const &) {});
        auto connectionFaulted1 = make_shared<ManualResetEvent>(false);
        node1->RegisterDisconnectEvent([=](const IDatagramTransport::DisconnectEventArgs& e)
        {
            Trace.WriteInfo(TraceType, "node1: disconnected: target={0}, fault={1}", e.Target->TraceId(), e.Fault);
            if (!connectionFaulted1->IsSet())
            {
                VERIFY_IS_TRUE(e.Fault.IsError(ErrorCodeValue::OperationCanceled));
                connectionFaulted1->Set();
            }
        });


        VERIFY_IS_TRUE(node1->Start().IsSuccess());

        auto node2 = TcpDatagramTransport::Create(TTestUtil::GetListenAddress());
        auto node2Received = make_shared<ManualResetEvent>(false);
        auto completeTest = make_shared<ManualResetEvent>(false);
        node2->SetMessageHandler([=](MessageUPtr & message, ISendTarget::SPtr const &)
        {
            Trace.WriteInfo(TraceType, "node2 received {0}, blocking receive thread, node1 send will time out", message->TraceId());
            node2Received->Set();
            completeTest->WaitOne(); //block to apply TCP back-pressure to sending side
            Trace.WriteInfo(TraceType, "node2 receive thread unblocked");
        });

        auto connectionFaulted2 = make_shared<ManualResetEvent>(false);
        node2->RegisterDisconnectEvent([=](const IDatagramTransport::DisconnectEventArgs& e)
        {
            Trace.WriteInfo(TraceType, "node2: disconnected: target={0}, fault={1}", e.Target->TraceId(), e.Fault);
            connectionFaulted2->Set();
        });

        VERIFY_IS_TRUE(node2->Start().IsSuccess());

        auto target1 = node2->ResolveTarget(node1->ListenAddress());
        VERIFY_IS_TRUE(target1);

        auto target2 = node1->ResolveTarget(node2->ListenAddress());
        VERIFY_IS_TRUE(target2);

        TestMessageBody body(64 * 1024);
        auto firstMsg = make_unique<Message>(body);
        firstMsg->Headers.Add(MessageIdHeader());
        Trace.WriteInfo(TraceType, "node1 sending first message {0} to node2", firstMsg->TraceId());
        node1->SendOneWay(target2, move(firstMsg));

        //Ensure a connection is set up before node2 starts sending to avoid duplicate connections
        VERIFY_IS_TRUE(node2Received->WaitOne(TimeSpan::FromSeconds(5)));

        auto startTime = Stopwatch::Now();
        while ((Stopwatch::Now() - startTime) < TimeSpan::FromMinutes(1))
        {
            Trace.WriteInfo(TraceType, "connectionFaulted1->IsSet() = {0}", connectionFaulted1->IsSet());
            Trace.WriteInfo(TraceType, "connectionFaulted2->IsSet() = {0}", connectionFaulted2->IsSet());
            if (connectionFaulted1->IsSet())
            {
                completeTest->Set();
                if (connectionFaulted2->IsSet()) break;

                Sleep(10);
                continue;
            }

            if (target2->MessagesPendingForSend() == 0) //Check to avoid send queue full
            {
                auto testMsg = make_unique<Message>(body);
                testMsg->Headers.Add(MessageIdHeader());
                Trace.WriteInfo(TraceType, "node1 sending {0} to node2", testMsg->TraceId());
                node1->SendOneWay(target2, move(testMsg));
            }

            if (target1->MessagesPendingForSend() == 0) //Check to avoid send queue full
            {
                auto testMsg = make_unique<Message>(body);
                testMsg->Headers.Add(MessageIdHeader());
                Trace.WriteInfo(TraceType, "node2 sending {0} to node1", testMsg->TraceId());
                node2->SendOneWay(target1, move(testMsg));
            }

            Sleep(10);
        }

        Trace.WriteInfo(TraceType, "connectionFaulted1->IsSet() = {0}", connectionFaulted1->IsSet());
        VERIFY_IS_TRUE(connectionFaulted1->IsSet());
        Trace.WriteInfo(TraceType, "connectionFaulted2->IsSet() = {0}", connectionFaulted2->IsSet());
        VERIFY_IS_TRUE(connectionFaulted2->IsSet());

        node1->Stop(TimeSpan::FromSeconds(10));
        node2->Stop(TimeSpan::FromSeconds(10));

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(SendQueueFullTest)
    {
        ENTER;

        auto receiver = TcpDatagramTransport::Create(TTestUtil::GetListenAddress());

        AutoResetEvent messageReceived;
        receiver->SetMessageHandler([&messageReceived](MessageUPtr & message, ISendTarget::SPtr const & st) -> void
        {
            Trace.WriteInfo(TraceType, "[receiver] got a message {0} {1}", message->MessageId, st->Address());
            messageReceived.Set();
        });

        VERIFY_IS_TRUE(receiver->Start().IsSuccess());

        auto sender = TcpDatagramTransport::CreateClient();
        const ULONG sendQueueLimit = 1024;
        sender->SetPerTargetSendQueueLimit(sendQueueLimit);
        VERIFY_IS_TRUE(sender->Start().IsSuccess());

        ISendTarget::SPtr target = sender->ResolveTarget(receiver->ListenAddress());
        VERIFY_IS_TRUE(target);

        ULONG messageBodySize = sendQueueLimit * 2;
        auto testMsg(make_unique<Message>(TestMessageBody(messageBodySize)));
        testMsg->Headers.Add(MessageIdHeader());
        ULONG newSendQueueSizeLimit = testMsg->SerializedSize() * 2;
        VERIFY_IS_TRUE(sender->SendOneWay(target, std::move(testMsg)).IsError(ErrorCodeValue::TransportSendQueueFull));

        VERIFY_IS_FALSE(messageReceived.WaitOne(TimeSpan::FromSeconds(0.1)));

        sender->SetPerTargetSendQueueLimit(newSendQueueSizeLimit);

        auto testMsg2(make_unique<Message>(TestMessageBody(messageBodySize)));
        testMsg2->Headers.Add(MessageIdHeader());
        auto err = sender->SendOneWay(target, std::move(testMsg2));
        VERIFY_IS_TRUE(err.IsSuccess());
        VERIFY_IS_TRUE(messageReceived.WaitOne(TimeSpan::FromSeconds(30)));

        // Sending completes on a different thread than receiving, thus we might 
        // need to wait a bit before verifying that send queue becomes empty
        for (int i = 0; i < 100; ++i)
        {
            auto pendingBytesForSend = target->BytesPendingForSend();
            Trace.WriteInfo(TraceType, "target->BytesPendingForSend = {0}", pendingBytesForSend);
            if (pendingBytesForSend == 0)
            {
                break;
            }

            ::Sleep(100);
        }

        VERIFY_IS_TRUE(target->BytesPendingForSend() == 0);

        sender->Stop();
        receiver->Stop();

        LEAVE;
    }


#ifdef PLATFORM_UNIX

    BOOST_AUTO_TEST_CASE(LeaseTransportMode)
    {
        ENTER;

        auto sender = TcpDatagramTransport::Create(L"127.0.0.1:0");
        sender->DisableListenInstanceMessage();
        sender->SetBufferFactory(make_unique<LTBufferFactory>());
        auto receiver = TcpDatagramTransport::Create(L"127.0.0.1:0");
        receiver->DisableListenInstanceMessage();
        receiver->SetBufferFactory(make_unique<LTBufferFactory>());

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

    BOOST_AUTO_TEST_CASE(SendBeyond_IOV_MAX)
    {
        ENTER;

        auto sender = TcpDatagramTransport::CreateClient();
        auto receiver = TcpDatagramTransport::Create(L"127.0.0.1:0");

        size_t toSend = 3;
        wstring testAction = TTestUtil::GetGuidAction();
        AutoResetEvent messageReceived;
        atomic_uint64 receiveCount(0);
        TTestUtil::SetMessageHandler(
            receiver,
            testAction,
            [&, toSend](MessageUPtr &, ISendTarget::SPtr const & st) -> void
            {
                Trace.WriteInfo(TraceType, "[receiver] got a message {0} {1}", sender->ListenAddress(), st->Address());
                if (++receiveCount == toSend)
                {
                    messageReceived.Set();
                }
            });

        VERIFY_IS_TRUE(receiver->Start().IsSuccess());
        VERIFY_IS_TRUE(sender->Start().IsSuccess());

        ISendTarget::SPtr target = sender->ResolveTarget(receiver->ListenAddress());
        VERIFY_IS_TRUE(target);

        for(size_t bodySize = 1024*1024; ; bodySize += bodySize)
        {
            TestMessageBody body(bodySize);
            auto msg = make_unique<Message>(body);
            msg->Headers.Add(ActionHeader(testAction));
            msg->Headers.Add(MessageIdHeader());

            size_t chunkCount = 0;
            for (BiqueChunkIterator chunk = msg->BeginHeaderChunks(); chunk != msg->EndHeaderChunks(); ++chunk)
            {
                ++chunkCount;
            }

            for (BufferIterator chunk = msg->BeginBodyChunks(); chunk != msg->EndBodyChunks(); ++chunk)
            {
                ++chunkCount;
            }

            Trace.WriteInfo(TraceType, "bodySize = {0}, chunkCount = {1}", bodySize, chunkCount);
            if (chunkCount <= IOV_MAX*2) continue;

            Trace.WriteInfo(TraceType, "[sender] sending {0}", msg->MessageId);
            sender->SendOneWay(target, std::move(msg));
            --toSend;

            for (; toSend > 0; --toSend)
            {
                auto msg = make_unique<Message>(body);
                msg->Headers.Add(ActionHeader(testAction));
                msg->Headers.Add(MessageIdHeader());
                Trace.WriteInfo(TraceType, "[sender] sending {0}", msg->MessageId);
                sender->SendOneWay(target, std::move(msg));
            }

            break;
        }

        bool receivedMessageOne = messageReceived.WaitOne(TimeSpan::FromSeconds(30));
        VERIFY_IS_TRUE(receivedMessageOne);

        sender->Stop();
        receiver->Stop();

        LEAVE;
    }

#else //LINUXTODO: enable the following tests for Linux

    BOOST_AUTO_TEST_CASE(SendQueueExpirationTest_ZeroExpiration)
    {
        ENTER;

        auto saved1 = TransportConfig::GetConfig().OutgoingMessageExpirationCheckInterval;
        TransportConfig::GetConfig().OutgoingMessageExpirationCheckInterval = TimeSpan::FromMilliseconds(1);
        KFinally([=] {
            TransportConfig::GetConfig().OutgoingMessageExpirationCheckInterval = saved1;
        });

        auto receiver = TcpDatagramTransport::Create(TTestUtil::GetListenAddress());

        wstring testAction = TTestUtil::GetGuidAction();
        TTestUtil::SetMessageHandler(
            receiver,
            testAction,
            [](MessageUPtr & message, ISendTarget::SPtr const & st) -> void
            {
                Trace.WriteInfo(TraceType, "receiver: got a message {0} {1}", message->MessageId, st->Address());
                auto target = static_pointer_cast<TcpSendTarget>(st);
                target->Test_SuspendReceive();
                Threadpool::Post([target] { target->Test_ResumeReceive(); }, TimeSpan::FromSeconds(30));
            });

        VERIFY_IS_TRUE(receiver->Start().IsSuccess());

        auto sender = TcpDatagramTransport::CreateClient();
        sender->SetPerTargetSendQueueLimit(4096 * 1024);

        ManualResetEvent connectionFault(false);
        sender->SetConnectionFaultHandler([&connectionFault](ISendTarget const & target, ErrorCode fault) {
            Trace.WriteError(TraceType, "sender: unexpected fault {0} on target {1}", fault, target.Address());
            connectionFault.Set();
        });

        VERIFY_IS_TRUE(sender->Start().IsSuccess());

        ISendTarget::SPtr target = sender->ResolveTarget(receiver->ListenAddress());
        VERIFY_IS_TRUE(target);

        ULONG messageBodySize = 4096;
        shared_ptr<ManualResetEvent> messageExpired = make_shared<ManualResetEvent>(false);

        Stopwatch stopwatch;
        stopwatch.Start();

        while (!messageExpired->WaitOne(TimeSpan::Zero) && (stopwatch.Elapsed < TimeSpan::FromSeconds(6)))
        {
            auto testMsg(make_unique<Message>(TestMessageBody(messageBodySize)));
            testMsg->Headers.Add(MessageIdHeader());
            testMsg->Headers.Add(ActionHeader(testAction));
            testMsg->SetSendStatusCallback([messageExpired](ErrorCode const & failure, MessageUPtr&&)
            {
                Trace.WriteInfo(TraceType, "sender: send failed: {0}", failure);
                if (failure.IsError(ErrorCodeValue::MessageExpired))
                {
                    messageExpired->Set();
                }
            });

            sender->SendOneWay(target, std::move(testMsg), TimeSpan::Zero);
            this_thread::yield();
        }

        VERIFY_IS_TRUE(messageExpired->WaitOne(TimeSpan::Zero));

        stopwatch.Stop();

        VERIFY_IS_FALSE(connectionFault.WaitOne(TimeSpan::FromMilliseconds(300)));
        sender->RemoveConnectionFaultHandler();
        sender->Stop();
        receiver->Stop();

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(SendQueueExpirationTest1)
    {
        ENTER;
        SendQueueExpirationTest(SecurityProvider::None);
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(SendQueueExpirationTest2)
    {
        ENTER;
        SendQueueExpirationTest(SecurityProvider::Ssl);
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(SendQueueExpirationTest3)
    {
        ENTER;
        SendQueueExpirationTest(SecurityProvider::Kerberos);
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(SendQueueExpirationTest4)
    {
        ENTER;
        SendQueueExpirationTest(SecurityProvider::Negotiate);
        LEAVE;
    }
#endif

    BOOST_AUTO_TEST_SUITE_END()

    void TcpTransportTests::ReceiveThreadDeadlockDetectionTest(SecurityProvider::Enum securityProvider)
    {
        TransportConfig& config = TransportConfig::GetConfig();
        auto savedReceiveMissingThreshold = config.ReceiveMissingThreshold;
        config.ReceiveMissingThreshold = TimeSpan::FromSeconds(3);
        TcpDatagramTransport::Test_ReceiveMissingAssertDisabled = true;

        auto sender = TcpDatagramTransport::CreateClient();
        auto receiver = TcpDatagramTransport::Create(TTestUtil::GetListenAddress());

        if (securityProvider != SecurityProvider::None)
        {
            SecuritySettings securitySettings = TTestUtil::CreateTestSecuritySettings(
                securityProvider,
                ProtectionLevel::EncryptAndSign);

            VERIFY_IS_TRUE(sender->SetSecurity(securitySettings).IsSuccess());
            VERIFY_IS_TRUE(receiver->SetSecurity(securitySettings).IsSuccess());
        }

        wstring testAction = L"BlockMe";
        AutoResetEvent blocker(false);
        TTestUtil::SetMessageHandler(
            receiver,
            testAction,
            [&blocker](MessageUPtr &, ISendTarget::SPtr const &) -> void
            {
                Trace.WriteInfo(TraceType, "[receiver] blocking receive thread");
                blocker.WaitOne(TimeSpan::MaxValue);
                Trace.WriteInfo(TraceType, "[receiver] receive thread unblocked");
        });

        VERIFY_IS_TRUE(receiver->Start().IsSuccess());
        VERIFY_IS_TRUE(sender->Start().IsSuccess());

        ISendTarget::SPtr target = sender->ResolveTarget(receiver->ListenAddress());
        VERIFY_IS_TRUE(target);

        TcpTestMessage body(L"test message for receive thread deadlock detection");
        auto oneway(make_unique<Message>(body));
        oneway->Headers.Add(MessageIdHeader());
        oneway->Headers.Add(ActorHeader(Actor::GenericTestActor));
        oneway->Headers.Add(ActionHeader(testAction));
        sender->SendOneWay(target, std::move(oneway));

        TimeSpan checkInterval = TimeSpan::FromSeconds(1);
        TimeSpan totalWaitTime = TimeSpan::Zero;
        for(;;)
        {
            Sleep((DWORD)checkInterval.TotalMilliseconds());
            totalWaitTime = totalWaitTime + checkInterval;

            if (TcpDatagramTransport::Test_ReceiveMissingDetected)
            {
                Trace.WriteInfo(TraceType, "Receive thread deadlock detected");
                break;
            }

            if (totalWaitTime > TimeSpan::FromSeconds(60))
            {
                VERIFY_IS_TRUE(false, L"Receive thread deadlock detection failed");
                break;
            }
        }

        TcpDatagramTransport::Test_ReceiveMissingAssertDisabled = false;
        TcpDatagramTransport::Test_ReceiveMissingDetected = false;
        config.ReceiveMissingThreshold = savedReceiveMissingThreshold;

        Trace.WriteInfo(TraceType, "unblock receive thread to let the connection clean up");
        blocker.Set();
    }

    void TcpTransportTests::IdleTimeoutTest(bool idleTimeoutExpected, bool keepExternalSendTargetReference)
    {
        TcpConnection::Test_IdleTimeoutHappened = false;

        auto sender = TcpDatagramTransport::CreateClient();
        auto idleTimeout = TimeSpan::FromSeconds(6);
        sender->SetConnectionIdleTimeout(idleTimeout);

        VERIFY_IS_TRUE(sender->Start().IsSuccess());

        auto receiver = TcpDatagramTransport::Create(TTestUtil::GetListenAddress());

        AutoResetEvent messageReceived;
        receiver->SetMessageHandler([&messageReceived](MessageUPtr &, ISendTarget::SPtr const & st) -> void
        {
            Trace.WriteInfo(TraceType, "[receiver] got a message from {0}", st->Address());
            messageReceived.Set();
        });

        VERIFY_IS_TRUE(receiver->Start().IsSuccess());

        auto target = sender->ResolveTarget(receiver->ListenAddress());
        auto msg(make_unique<Message>());
        sender->SendOneWay(target, std::move(msg));

        if (!keepExternalSendTargetReference)
        {
            target.reset(); // Remove external refcount on the send target
        }

        VERIFY_IS_TRUE(messageReceived.WaitOne(TimeSpan::FromSeconds(30)));

        TTestUtil::WaitUntil(
            [&]
            {            
                Trace.WriteInfo(TraceType, "target refcount = {0}", target.use_count());
                return TcpConnection::Test_IdleTimeoutHappened == idleTimeoutExpected;
            },
            idleTimeout + TimeSpan::FromSeconds(3));
    }

    void TcpTransportTests::SimpleTcpTest(std::wstring const & senderAddress, std::wstring const & receiverAddress)
    {
        Trace.WriteInfo(TraceType, "senderAddress ={0},receiverAddress={1}", senderAddress, receiverAddress);
        auto sender = TcpDatagramTransport::Create(senderAddress);
        auto receiver = TcpDatagramTransport::Create(receiverAddress);

        AutoResetEvent replyReceived;
        wstring testAction = TTestUtil::GetGuidAction();
        TTestUtil::SetMessageHandler(
            sender,
            testAction,
            [&replyReceived](MessageUPtr & reply, ISendTarget::SPtr const &) -> void
        {
            Trace.WriteInfo(TraceType, "sender got a reply, which is supposed to be set idempotent");
            VERIFY_IS_TRUE(reply->Idempotent);
            replyReceived.Set();
        });

        AutoResetEvent messageReceived;
        TTestUtil::SetMessageHandler(
            receiver,
            testAction,
            [testAction, &receiver, &sender, &messageReceived](MessageUPtr & msg, ISendTarget::SPtr const & st) -> void
        {
            Trace.WriteInfo(TraceType, "receiver got a message {0} {1}, it should be non-idempotent", sender->ListenAddress(), st->Address());
            VERIFY_IS_TRUE(sender->ListenAddress() == st->Address());
            VERIFY_IS_FALSE(msg->Idempotent);
            messageReceived.Set();

            // ping back
            MessageUPtr reply = make_unique<Message>();
            reply->Headers.Add(ActionHeader(testAction));
            reply->Headers.Add(MessageIdHeader());
            reply->Idempotent = true;
            Trace.WriteInfo(TraceType, "[receiver] sending {0}", reply->MessageId);
            receiver->SendOneWay(st, move(reply));
        });

        VERIFY_IS_TRUE(receiver->Start().IsSuccess());
        VERIFY_IS_TRUE(sender->Start().IsSuccess());

        ISendTarget::SPtr target = sender->ResolveTarget(receiver->ListenAddress());
        VERIFY_IS_TRUE(target);

        bool receivedReplyOne = replyReceived.WaitOne(TimeSpan::FromSeconds(1));
        VERIFY_IS_FALSE(receivedReplyOne);

        TcpTestMessage body(L"Message message message message oh yeah");
        auto request(make_unique<Message>(body));
        request->Headers.Add(ActionHeader(testAction));
        request->Headers.Add(MessageIdHeader());
        Random r;
        if (r.Next() % 2)
        {
            Trace.WriteInfo(TraceType, "sender: changing idempotency more than once");
            request->Idempotent = true;
            request->Idempotent = false;
        }

        Trace.WriteInfo(TraceType, "[sender] sending {0}", request->MessageId);
        sender->SendOneWay(target, std::move(request));

        bool receivedMessageOne = messageReceived.WaitOne(TimeSpan::FromSeconds(30));
        VERIFY_IS_TRUE(receivedMessageOne);

        receivedReplyOne = replyReceived.WaitOne(TimeSpan::FromSeconds(30));
        VERIFY_IS_TRUE(receivedReplyOne);

        VERIFY_IS_TRUE(sender->SendTargetCount() > 0);
        VERIFY_IS_TRUE(receiver->SendTargetCount() > 0);
        // Verify SendTarget cleanup

        target->TargetDown(); // break connection for receiver
        target = nullptr; // release our reference

        size_t targetCountInSender = 0;
        size_t targetCountInReceiver = 0;
        for (int i = 0; i < 600; ++i)
        {
            targetCountInSender = sender->SendTargetCount();
            targetCountInReceiver = receiver->SendTargetCount();
            if (targetCountInSender || targetCountInReceiver)
            {
                Sleep(100);
            }
            else
            {
                break;
            }
        }

        Trace.WriteInfo(TraceType, "sender->SendTargetCount(): {0}", targetCountInSender);
        VERIFY_IS_TRUE(targetCountInSender == 0);
        Trace.WriteInfo(TraceType, "receiver->SendTargetCount(): {0}", targetCountInReceiver);
        VERIFY_IS_TRUE(targetCountInReceiver == 0);

        VERIFY_ARE_EQUAL2(sender->Start().ReadValue(), ErrorCodeValue::InvalidState);

        sender->Stop(TimeSpan::FromSeconds(10));
        receiver->Stop(TimeSpan::FromSeconds(10));
    }

    void TcpTransportTests::DrainTest(int messageSize, bool successExpected, Common::TimeSpan drainTimeout)
    {
        auto & transportConfig = TransportConfig::GetConfig();
        auto savedSizeLimit = transportConfig.DefaultSendQueueSizeLimit;
        transportConfig.DefaultSendQueueSizeLimit = 0;
        auto savedExpiration = transportConfig.DefaultOutgoingMessageExpiration;
        transportConfig.DefaultOutgoingMessageExpiration = TimeSpan::Zero;
        auto savedCloseDrainTimeout = transportConfig.CloseDrainTimeout;
        transportConfig.CloseDrainTimeout = drainTimeout;
        KFinally([&] {
            transportConfig.DefaultSendQueueSizeLimit = savedSizeLimit;
            transportConfig.DefaultOutgoingMessageExpiration = savedExpiration;
            transportConfig.CloseDrainTimeout = savedCloseDrainTimeout;
        });

        auto sender = TcpDatagramTransport::CreateClient();
        auto receiver = TcpDatagramTransport::Create(L"127.0.0.1:0");

        AutoResetEvent receivedEnoughMessages;

        const LONG sendTotal = 400;
        shared_ptr<Common::atomic_long> receiveCount = make_shared<Common::atomic_long>(0);

        auto bodyByteBlock = make_shared<vector<BYTE>>(messageSize);
        for (int i = 0; i < messageSize; ++i)
        {
            (*bodyByteBlock)[i] = (i & 0xff); // set up pattern for receiving side to verify
        }

        wstring testAction = TTestUtil::GetGuidAction();
        TTestUtil::SetMessageHandler(
            receiver,
            testAction,
            [&receivedEnoughMessages, sendTotal, receiveCount, successExpected, bodyByteBlock](MessageUPtr & msg, ISendTarget::SPtr const &)
            {
                ++(*receiveCount);

                LONG receivedSoFar = receiveCount->load();
                Trace.WriteInfo(
                    TraceType, "DrainTest: received {0}, header = {1}, body = {2}, total message received = {3}",
                    msg->TraceId(), msg->SerializedHeaderSize(), msg->SerializedBodySize(), receivedSoFar);

                //verify message body
                vector<const_buffer> buffers;
                BOOST_REQUIRE(msg->GetBody(buffers));
                int byteCount = 0;
                for(auto const & buffer : buffers)
                {
                    if(memcmp(bodyByteBlock->data() + byteCount, buffer.buf, buffer.size()) != 0)
                    {
                        BOOST_FAIL("body verification failed");
                    }
                    
                    byteCount += static_cast<int>(buffer.size());
                }

                if (receivedSoFar == sendTotal)
                {
                    Trace.WriteInfo(TraceType, "Received enough messages.");
                    receivedEnoughMessages.Set();
                }

                // Use TCP back pressure to slow down sending side
                if (successExpected)
                {
                    Sleep(1);
                }
                else
                {
                    Sleep(1000);
                }
            });

        VERIFY_IS_TRUE(receiver->Start().IsSuccess());
        VERIFY_IS_TRUE(sender->Start().IsSuccess());

        auto target = sender->ResolveTarget(receiver->ListenAddress());

        std::vector<const_buffer> buffers;
        const_buffer myBuffer(bodyByteBlock->data(), messageSize);
        buffers.push_back(myBuffer);

        shared_ptr<Common::atomic_long> callbackCounter = make_shared<Common::atomic_long>(0);

        Transport::Message::DeleteCallback callback = [callbackCounter, sender](std::vector<Common::const_buffer> const &, void *)
        {
            ++(*callbackCounter);

            if (callbackCounter->load() % 100 == 0)
            {
                Trace.WriteInfo(TraceType, "Deleted {0} messages", callbackCounter->load());
            }
        };

        for (int i = 0; i < sendTotal; ++i)
        {
            auto oneway(make_unique<Message>(buffers, callback, bodyByteBlock->data()));
            oneway->Headers.Add(MessageIdHeader());
            oneway->Headers.Add(ActionHeader(testAction));
            Trace.WriteInfo(TraceType, "sending {0}: header = {1}, body = {2}", oneway->TraceId(), oneway->SerializedHeaderSize(), oneway->SerializedBodySize());
            sender->SendOneWay(target, std::move(oneway));
        }

        Trace.WriteInfo(TraceType, "All messages have been queued");

        // Wait for the first message to be received by stopping sender, otherwise draining may not be triggered
        TTestUtil::WaitUntil([&] { return receiveCount->load() > 0; }, TimeSpan::FromSeconds(10));
        Trace.WriteInfo(TraceType, "The first message has reached receiver");

        sender->Stop();
        Trace.WriteInfo(TraceType, "Sender has been stopped, draining in progress ...");

        bool waitSucceeded = receivedEnoughMessages.WaitOne(TimeSpan::FromSeconds(5) + drainTimeout);
        VERIFY_IS_TRUE(waitSucceeded == successExpected);

        if (successExpected)
        {
            for (int i = 0; i < 100 && (receiveCount->load() < sendTotal); ++ i)
            {
                Sleep(100);
            }
        }

        Trace.WriteInfo(TraceType, "Received {0} messages, send total = {1}", receiveCount->load(), sendTotal);

        receiver->Stop();
    }

    void TcpTransportTests::ClientTcpTest(wstring const & serverAddress)
    {
        auto sender = TcpDatagramTransport::CreateClient();
        auto receiver = TcpDatagramTransport::Create(serverAddress);

        AutoResetEvent replyReceived;
        sender->SetMessageHandler([&replyReceived](MessageUPtr &, ISendTarget::SPtr const &) -> void
        {
            Trace.WriteInfo(TraceType, "[sender] got a message");

            replyReceived.Set();
        });

        AutoResetEvent messageReceived;
        receiver->SetMessageHandler([&receiver, &sender, &messageReceived](MessageUPtr &, ISendTarget::SPtr const & st) -> void
        {
            Trace.WriteInfo(TraceType, "[receiver] got a message");

            messageReceived.Set();

            // ping back
            receiver->SendOneWay(st, make_unique<Message>());
        });

        VERIFY_IS_TRUE(receiver->Start().IsSuccess());
        VERIFY_IS_TRUE(sender->Start().IsSuccess());
        ISendTarget::SPtr target = sender->ResolveTarget(receiver->ListenAddress());
        VERIFY_IS_TRUE(target);

        bool receivedReplyOne = replyReceived.WaitOne(TimeSpan::FromSeconds(1));
        VERIFY_IS_FALSE(receivedReplyOne);
            
        TcpTestMessage body(L"Message message message message oh yeah");
        sender->SendOneWay(target, make_unique<Message>(body));

        bool receivedMessageOne = messageReceived.WaitOne(TimeSpan::FromSeconds(30));
        VERIFY_IS_TRUE(receivedMessageOne);

        receivedReplyOne = replyReceived.WaitOne(TimeSpan::FromSeconds(30));
        VERIFY_IS_TRUE(receivedReplyOne);

        sender->Stop();
        receiver->Stop();
    }

    void TcpTransportTests::ConnectionFaultTest_SenderClose(bool abortConnection)
    {
        auto receiver = TcpDatagramTransport::Create(TTestUtil::GetListenAddress());

        AutoResetEvent receiverGotConnectionFault(false);
        receiver->SetConnectionFaultHandler(
            [&receiverGotConnectionFault, abortConnection](ISendTarget const &, ErrorCode fault) -> void
        {
            Trace.WriteInfo(TraceType, "[receiver] got connetion fault {0}", fault);
            receiverGotConnectionFault.Set();
        });

        AutoResetEvent receiverGotMessage(false);
        receiver->SetMessageHandler([&receiver, &receiverGotMessage](MessageUPtr & msg, ISendTarget::SPtr const & st) -> void
        {
            Trace.WriteInfo(TraceType, "[receiver] got message {0}", msg->TraceId());
            receiverGotMessage.Set();

            auto reply(make_unique<Message>());
            reply->Headers.Add(MessageIdHeader());
            Trace.WriteInfo(TraceType, "[receiver] sending reply {0}", reply->TraceId());
            receiver->SendOneWay(st, std::move(msg));
        });

        VERIFY_IS_TRUE(receiver->Start().IsSuccess());

        auto sender = TcpDatagramTransport::CreateClient();

        AutoResetEvent senderGotConnectionFault(false);
        ErrorCode senderConnectionFault;
        sender->SetConnectionFaultHandler(
            [&senderConnectionFault, &senderGotConnectionFault](ISendTarget const &, ErrorCode fault) -> void
            {
                Trace.WriteInfo(TraceType, "[sender] got connetion fault {0}", fault);
                senderConnectionFault = fault;
                senderGotConnectionFault.Set();
            });

        AutoResetEvent senderGotMessage(false);
        sender->SetMessageHandler([&senderGotMessage](MessageUPtr &, ISendTarget::SPtr const &) -> void
        {
            Trace.WriteInfo(TraceType, "[receiver] got message");
            senderGotMessage.Set();
        });

        VERIFY_IS_TRUE(sender->Start().IsSuccess());

        auto target = sender->ResolveTarget(receiver->ListenAddress());
        VERIFY_IS_TRUE(target);

        auto msg(make_unique<Message>());
        msg->Headers.Add(MessageIdHeader());
        Trace.WriteInfo(TraceType, "[sender] sending message {0}", msg->TraceId());
        sender->SendOneWay(target, std::move(msg));

        VERIFY_IS_TRUE(receiverGotMessage.WaitOne(TimeSpan::FromSeconds(15)));
        VERIFY_IS_TRUE(senderGotMessage.WaitOne(TimeSpan::FromSeconds(15)));

        if (abortConnection)
        {
            Trace.WriteInfo(TraceType, "[sender] abort target to trigger connection fault");
            ((TcpSendTarget&)(*target)).Abort();
        }
        else
        {
            Trace.WriteInfo(TraceType, "[sender] close target to trigger connection fault");
            target->Reset();
        }

        VERIFY_IS_TRUE(receiverGotConnectionFault.WaitOne(TimeSpan::FromSeconds(15)));
        VERIFY_IS_TRUE(senderGotConnectionFault.WaitOne(TimeSpan::FromSeconds(15)));

#ifndef PLATFORM_UNIX
        VERIFY_IS_TRUE(senderConnectionFault.ReadValue() == (abortConnection? ErrorCodeValue::OperationCanceled : ErrorCodeValue::Success));
#endif

        receiver->Stop();
        sender->Stop();
    }

    void TcpTransportTests::ConnectionFaultTest_ReceiverClose(bool abortConnection)
    {
        auto receiver = TcpDatagramTransport::Create(TTestUtil::GetListenAddress());

        AutoResetEvent receiverGotConnectionFault(false);
        receiver->SetConnectionFaultHandler(
            [&receiverGotConnectionFault](ISendTarget const &, ErrorCode fault) -> void
        {
            Trace.WriteInfo(TraceType, "[receiver] got connetion fault {0}", fault);
            receiverGotConnectionFault.Set();
        });

        receiver->SetMessageHandler([&receiver, abortConnection](MessageUPtr & msg, ISendTarget::SPtr const & st) -> void
        {
            Trace.WriteInfo(TraceType, "[receiver] got message {0}", msg->TraceId());

            if (abortConnection)
            {
                Trace.WriteInfo(TraceType, "[receiver] abort target to trigger connection fault");
                ((TcpSendTarget&)(*st)).Abort();
            }
            else
            {
                Trace.WriteInfo(TraceType, "[receiver] close target to trigger connection fault");
                st->Reset();
            }
        });

        VERIFY_IS_TRUE(receiver->Start().IsSuccess());

        auto sender = TcpDatagramTransport::CreateClient();

        AutoResetEvent senderGotConnectionFault(false);
        ErrorCode senderConnectionFault;
        sender->SetConnectionFaultHandler(
            [&senderConnectionFault, &senderGotConnectionFault](ISendTarget const &, ErrorCode fault) -> void
        {
            Trace.WriteInfo(TraceType, "[sender] got connetion fault {0}", fault);
            senderConnectionFault = fault;
            senderGotConnectionFault.Set();
        });

        VERIFY_IS_TRUE(sender->Start().IsSuccess());

        auto target = sender->ResolveTarget(receiver->ListenAddress());
        VERIFY_IS_TRUE(target);

        auto msg(make_unique<Message>());
        msg->Headers.Add(MessageIdHeader());
        Trace.WriteInfo(TraceType, "[sender] sending message {0}", msg->TraceId());
        sender->SendOneWay(target, std::move(msg));

        VERIFY_IS_TRUE(receiverGotConnectionFault.WaitOne(TimeSpan::FromSeconds(15)));
        VERIFY_IS_TRUE(senderGotConnectionFault.WaitOne(TimeSpan::FromSeconds(15)));
        VERIFY_ARE_EQUAL2(senderConnectionFault.ReadValue(), ErrorCodeValue::ConnectionClosedByRemoteEnd);
    }


    void TcpTransportTests::SendQueueExpirationTest(SecurityProvider::Enum provider)
    {
        TimeSpan outgoingMessageExpiration = TimeSpan::FromSeconds(10);

        // Disable receiving message in this test
        auto savedCheckInterval = TransportConfig::GetConfig().OutgoingMessageExpirationCheckInterval;
        TransportConfig::GetConfig().OutgoingMessageExpirationCheckInterval =
            TimeSpan::FromMilliseconds(outgoingMessageExpiration.TotalMilliseconds() / 2.0);

        KFinally([=] {
            TransportConfig::GetConfig().OutgoingMessageExpirationCheckInterval = savedCheckInterval;
        });

        auto sender = TcpDatagramTransport::CreateClient(L"Sender", L"SendQueueExpirationTest");
        sender->SetOutgoingMessageExpiration(outgoingMessageExpiration);

        auto receiver = TcpDatagramTransport::Create(
            TTestUtil::GetListenAddress(),
            L"Receiver",
            L"SendQueueExpirationTest");

        TimeSpan receiveBlockingTime = TimeSpan::FromMilliseconds(outgoingMessageExpiration.TotalMilliseconds() * 2.0);
        AutoResetEvent messageReceived(false);
        Common::atomic_bool firstReceiveCallback(true);
        DWORD lastReceivedMessageIndex;

        wstring testAction = TTestUtil::GetGuidAction();
        TTestUtil::SetMessageHandler(
            receiver,
            testAction,
            [receiveBlockingTime, &messageReceived, &firstReceiveCallback, &lastReceivedMessageIndex]
        (MessageUPtr & message, ISendTarget::SPtr const & st) -> void
        {
            Trace.WriteInfo(TraceType, "[receiver] got a message {0} {1}", message->MessageId, st->Address());
            lastReceivedMessageIndex = message->MessageId.Index;
            messageReceived.Set();

            if (firstReceiveCallback.exchange(false))
            {
                Trace.WriteInfo(TraceType, "[receiver] suspend receiving for {0} to trigger message backup on sender", receiveBlockingTime);
                auto target = static_pointer_cast<TcpSendTarget>(st);
                target->Test_SuspendReceive();
                Threadpool::Post([target] { target->Test_ResumeReceive(); }, receiveBlockingTime);
            }
        });

        SecuritySettings securitySettings = TTestUtil::CreateTestSecuritySettings(provider);
        VERIFY_IS_TRUE(receiver->SetSecurity(securitySettings).IsSuccess());
        VERIFY_IS_TRUE(sender->SetSecurity(securitySettings).IsSuccess());

        ManualResetEvent connectionFaulted(false);
        sender->SetConnectionFaultHandler([&connectionFaulted](ISendTarget const &, ErrorCode fault)
        {
            Trace.WriteError(TraceType, "[sender] unexpected connection fault: {0}", fault);
            connectionFaulted.Set();
        });

        VERIFY_IS_TRUE(receiver->Start().IsSuccess());
        VERIFY_IS_TRUE(sender->Start().IsSuccess());

        ISendTarget::SPtr target = sender->ResolveTarget(receiver->ListenAddress(), L"", TransportSecurity().LocalWindowsIdentity());
        VERIFY_IS_TRUE(target);

        auto firstMsg = make_unique<Message>();
        firstMsg->Headers.Add(ActionHeader(testAction));
        firstMsg->Headers.Add(MessageIdHeader());
        sender->SendOneWay(target, std::move(firstMsg));
        VERIFY_IS_TRUE(messageReceived.WaitOne(outgoingMessageExpiration + TimeSpan::FromSeconds(30)));

        shared_ptr<ManualResetEvent> messageExpired = make_shared<ManualResetEvent>(false);
        Stopwatch stopwatch;
        stopwatch.Start();

        while (!messageExpired->WaitOne(TimeSpan::Zero) && (stopwatch.Elapsed <= receiveBlockingTime))
        {
            TestMessageBody body(8192);
            auto testMsg(make_unique<Message>(body));
            testMsg->Headers.Add(ActionHeader(testAction));
            testMsg->Headers.Add(MessageIdHeader());
            testMsg->SetSendStatusCallback([messageExpired](ErrorCode const & failure, MessageUPtr&&)
            {
                if (failure.IsError(ErrorCodeValue::MessageExpired))
                {
                    Trace.WriteInfo(TraceType, "[sender] message expired");
                    messageExpired->Set();
                }
            });

            sender->SendOneWay(target, std::move(testMsg));
            ::Sleep(10);
        }

        stopwatch.Stop();

        VERIFY_IS_TRUE(messageExpired->WaitOne(TimeSpan::Zero));

        // verify message can reach destination after message expiration happens, without having connection fault
        for (int i = 0; i < 2000; ++ i)
        {
            auto newMsg = make_unique<Message>();
            newMsg->Headers.Add(ActionHeader(testAction));
            newMsg->Headers.Add(MessageIdHeader());
            DWORD sendingIndex = newMsg->MessageId.Index;
            sender->SendOneWay(target, std::move(newMsg));

            if (messageReceived.WaitOne(TimeSpan::FromMilliseconds(10)) && (sendingIndex == lastReceivedMessageIndex))
            {
                break;
            }
        }

        VERIFY_IS_FALSE(connectionFaulted.WaitOne(TimeSpan::FromSeconds(1)));
        sender->RemoveConnectionFaultHandler();

        sender->Stop();
        receiver->Stop();
    }

    void TcpTransportTests::TargetInstanceTestSendRequest(
        IDatagramTransportSPtr const & srcNode,
        ISendTarget::SPtr const & target,
        wstring const & action)
    {
        auto request = make_unique<Message>();
        request->Headers.Add(ActionHeader(action));
        request->Headers.Add(MessageIdHeader());
        Trace.WriteInfo(
            TraceType,
            "node1 sending request {0} to {1} at {2}",
            request->TraceId(), TextTracePtr(target.get()), target->Address());

        srcNode->SendOneWay(target, move(request));
    }

    void TcpTransportTests::TargetInstanceTestCheckConnecitonCount(
        ISendTarget::SPtr const & target,
        wstring const & connectionType,
        size_t expected)
    {
        size_t connectionCount = target->ConnectionCount();
        Trace.WriteInfo(
            TraceType,
            "{0} connection count = {1}, expected = {2}",
            connectionType, connectionCount, expected);

        VERIFY_IS_TRUE(connectionCount == expected);
    }
}
