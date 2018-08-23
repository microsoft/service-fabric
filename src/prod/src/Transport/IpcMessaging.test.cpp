// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TestCommon.h"

namespace Transport
{
    using namespace std;
    using namespace Common;

    typedef std::shared_ptr<IpcServer> IpcServerSPtr;
    typedef std::shared_ptr<IpcClient> IpcClientSPtr;

    class IpcTestBase
    {
    protected:
        IpcTestBase()
            : serverSideActor_(Actor::IpcTestActor1)
            , clientSideActor_(Actor::IpcTestActor2)
        {
            BOOST_REQUIRE(Setup());
        }

        TEST_CLASS_SETUP(Setup)

        Actor::Enum serverSideActor_;
        Actor::Enum clientSideActor_;

        void OnewayTest(bool secureMode);
        void RequestReplyTest(bool secureMode);
        void ReconnectTest(bool secureMode);

        IpcServerSPtr OpenServer(std::wstring const & serverAddress, bool secureMode);
        IpcServerSPtr OpenServer(
            std::wstring const & serverAddress,
            std::wstring const & serverAddress2,
            bool secureMode);

        IpcClientSPtr OpenClient(
            std::wstring const & clientId,
            std::wstring const & serverAddress,
            bool secureMode);

        void CloseServer(IpcServerSPtr server);
        void CloseClient(IpcClientSPtr client);
        void Cleanup();

        MessageUPtr CreateServerMessage(Actor::Enum actor);
        MessageUPtr CreateClientMessage(Actor::Enum actor);
        MessageUPtr CreateClientMessageLong(Actor::Enum actor);

        class TestRoot : public Common::ComponentRoot
        {
        public:
            ~TestRoot() {}

            IpcServerSPtr server_;
            IpcClientSPtr client_;
            vector<IpcClientSPtr> clients_;
        };

        std::shared_ptr<TestRoot> root_;
    };


    BOOST_FIXTURE_TEST_SUITE2(IpcTests,IpcTestBase)

    BOOST_AUTO_TEST_CASE(OnewayTestNonSecure)
    {
        ENTER;
        OnewayTest(false);
        LEAVE;
    }

#ifndef PLATFORM_UNIX
    BOOST_AUTO_TEST_CASE(RequestReplyTestNonSecure)
    {
        ENTER;
        RequestReplyTest(false);
        LEAVE;
    }
#endif

    BOOST_AUTO_TEST_CASE(InvalidAddressTest)
    {
        ENTER;

        wstring serverName;
        auto error = TcpTransportUtility::GetLocalFqdn(serverName);
        VERIFY_IS_TRUE(error.IsSuccess());

        wstring localhost = L"localhost";
        VERIFY_IS_FALSE(StringUtility::AreEqualCaseInsensitive(serverName, localhost));

        wstring serverAddress = serverName + L":1234";
        Trace.WriteInfo(TraceType, "test with invalid IPC address {0}", serverAddress);
        {
            auto server = std::make_shared<IpcServer>(
                *root_, serverAddress, L"IpcServer", false /* disallow use of unreliable transport */, L"IpcTestBase::InvalidAddressTest");
            VERIFY_IS_TRUE(server->Open().IsError(ErrorCodeValue::InvalidAddress));
        }

#ifndef PLATFORM_UNIX
        vector<Endpoint> addresses;
        VERIFY_IS_TRUE(TcpTransportUtility::TryResolveHostNameAddress(serverAddress, ResolveOptions::Unspecified, addresses).IsSuccess());
        serverAddress = addresses[0].ToString();
        Trace.WriteInfo(TraceType, "test with invalid IPC address {0}", serverAddress);
        {
            auto server = std::make_shared<IpcServer>(
                *root_, serverAddress, L"IpcServer", false /* disallow use of unreliable transport */, L"IpcTestBase::InvalidAddressTest");
            VERIFY_IS_TRUE(server->Open().IsError(ErrorCodeValue::InvalidAddress));
        }
#endif

        LEAVE;
    }

#ifndef PLATFORM_UNIX
    BOOST_AUTO_TEST_CASE(SecuredOnewayTest)
    {
        ENTER;
        OnewayTest(true);
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(SecuredRequestReplyTest)
    {
        ENTER;
        RequestReplyTest(true);
        LEAVE;
    }
#endif

    BOOST_AUTO_TEST_CASE(ClientNotFoundTest)
    {
        ENTER;

        KFinally([this] { Cleanup(); });

        // set up IpcServer
        auto& server = root_->server_;
        std::wstring serverListenAddress = TTestUtil::GetListenAddress();
        server = OpenServer(serverListenAddress, false);

        ErrorCode errorCode = server->SendOneWay(L"NoSuchClient", CreateServerMessage(clientSideActor_));
        VERIFY_IS_TRUE(errorCode.IsError(ErrorCodeValue::NotFound));

        Common::AutoResetEvent requestCompleted(false);
        server->BeginRequest(
            CreateServerMessage(clientSideActor_),
            L"NotSuchClient",
            Common::TimeSpan::MaxValue,
            [&] (AsyncOperationSPtr const & operation)
            {
                MessageUPtr reply;
                ErrorCode endRequestErrorCode = server->EndRequest(operation, reply);
                VERIFY_IS_TRUE(endRequestErrorCode.IsError(ErrorCodeValue::NotFound));
                requestCompleted.Set();
            },
            Common::AsyncOperationSPtr());

        requestCompleted.WaitOne(TimeSpan::FromSeconds(30));

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(ReconnectTestNonSecure)
    {
        ENTER;
        ReconnectTest(false);
        LEAVE;
    }

#ifndef PLATFORM_UNIX
    BOOST_AUTO_TEST_CASE(ReconnectTestSecure)
    {
        ENTER;
        ReconnectTest(true);
        LEAVE;
    }

    //message size checking only works in secure mode, but Windwos security (default security provider for IPC)
    //is not supported on Linux, thus OpenServer(secureMode=true) is not supproted on Linux.
    BOOST_AUTO_TEST_CASE(RequestReplyMessageTooLongTest_WithWindowsSecurity)
    {
        ENTER;

        KFinally([this] { Cleanup(); });

        Assert::DisableDebugBreakInThisScope disableDebugBreakInThisScope;
        Assert::DisableTestAssertInThisScope disableTestAssertInThisScope;

        std::wstring clientStr = L"client";        

        auto& server = root_->server_;
        auto& client = root_->client_;

        std::wstring serverListenAddress = TTestUtil::GetListenAddress();
        server = OpenServer(serverListenAddress, true);
        auto isMessageHandled = make_shared<AutoResetEvent>(false);
        server->RegisterMessageHandler(
            serverSideActor_,
            [=] (MessageUPtr & message, IpcReceiverContextUPtr & context)
            {
                if (message->Headers.ExpectsReply)
                {
                    context->Reply(CreateServerMessage(clientSideActor_));
                }
            },
            true/*dispatchOnTransportThread*/);
        
        client = OpenClient(clientStr, serverListenAddress, true);

        Trace.WriteInfo(TraceType, "ping first to make sure security negotiation is completed before testing request dropping on sending side");
        auto ping = make_unique<Message>();
        ping->Headers.Add(ActorHeader(serverSideActor_));
        ping->Headers.Add(MessageIdHeader());
        ping->Headers.Add(ExpectsReplyHeader(true));
        client->BeginRequest(
            move(ping),
            Common::TimeSpan::MaxValue,
            [=](AsyncOperationSPtr const & operation)
            {
                MessageUPtr reply;
                auto endRequestErrorCode = client->EndRequest(operation, reply);
                VERIFY_IS_TRUE(endRequestErrorCode.IsSuccess());
                isMessageHandled->Set();
            },
            Common::AsyncOperationSPtr());

        VERIFY_IS_TRUE(isMessageHandled->WaitOne(TimeSpan::FromSeconds(10)));

        Common::Stopwatch timer;
        timer.Start();

        Trace.WriteInfo(TraceType, "testing request size checking on client side");
        client->BeginRequest(
            CreateClientMessageLong(serverSideActor_),
            Common::TimeSpan::MaxValue,
            [=] (AsyncOperationSPtr const & operation)
            {
                MessageUPtr reply;
                ErrorCode endRequestErrorCode = client->EndRequest(operation, reply);
                VERIFY_IS_TRUE( endRequestErrorCode.IsError(Common::ErrorCodeValue::MessageTooLarge) );
                isMessageHandled->Set();
            },
            Common::AsyncOperationSPtr());

        isMessageHandled->WaitOne( TimeSpan::FromSeconds(10) );
        timer.Stop();
        VERIFY_IS_TRUE( timer.Elapsed < TimeSpan::FromSeconds(10) );  //verify that Timeout didn't occur.

        LEAVE;
    }

#endif

    BOOST_AUTO_TEST_CASE(DualListener)
    {
        ENTER;

        KFinally([this] { Cleanup(); });

        Endpoint ep;
        auto err = TcpTransportUtility::GetFirstLocalAddress(ep, ResolveOptions::IPv4);
        VERIFY_IS_TRUE(err.IsSuccess());
        vector<wstring> addresses = { TTestUtil::GetListenAddress(), wformatString("{0}", ep) };

        auto& server = root_->server_;
        auto& clients = root_->clients_;

        std::wstring clientIdArray[] = { L"client0", L"client1" };
        LONG const numberOfClients = sizeof(clientIdArray) / sizeof(std::wstring);

        server = OpenServer(addresses[0], addresses[1], false);
        vector<wstring> serverListenAddresses = { server->TransportListenAddress, server->TransportListenAddressTls};

        Common::atomic_long serverRequestCounter;
        ManualResetEvent serverReceivedAllRequests(false);
        server->RegisterMessageHandler(
            serverSideActor_,
            [&](MessageUPtr & message, IpcReceiverContextUPtr & context)
            {
                bool foundClientId = false;
                for (LONG i = 0; i < numberOfClients; ++i)
                {
                    if (context->From == clientIdArray[i])
                    {
                        foundClientId = true;
                    }
                }

                VERIFY_IS_TRUE(foundClientId);

                if (message->Headers.ExpectsReply)
                {
                    if (++serverRequestCounter == numberOfClients)
                    {
                        serverReceivedAllRequests.Set();
                    }

                    context->Reply(CreateServerMessage(clientSideActor_));
                }
            },
            true/*dispatchOnTransportThread*/);

        Common::atomic_long clientRequestCounter;
        Common::atomic_long clientReplyCounter;
        ManualResetEvent clientReceivedAllReplies(false);
        for (size_t i = 0; i < numberOfClients; ++i)
        {
            auto client = OpenClient(clientIdArray[i], serverListenAddresses[i], false);
            clients.push_back(client);
            client->RegisterMessageHandler(
                clientSideActor_,
                [&](MessageUPtr & message, IpcReceiverContextUPtr & context)
                {
                    Trace.WriteInfo(TraceType, "client got a message {0} {1}", message->TraceId(), message->IsReply());
                    VERIFY_IS_TRUE(context->From.empty());

                    if (message->Headers.ExpectsReply)
                    {
                        Trace.WriteInfo(TraceType, "This is a request from server");
                        ++clientRequestCounter;
                        context->Reply(CreateClientMessage(serverSideActor_));
                    }
                },
                true/*dispatchOnTransportThread*/);

            // Send request from client to server
            client->BeginRequest(
                CreateClientMessage(serverSideActor_),
                Common::TimeSpan::MaxValue,
                [&](AsyncOperationSPtr const & operation)
                {
                    MessageUPtr reply;
                    ErrorCode endRequestErrorCode = client->EndRequest(operation, reply);
                    VERIFY_IS_TRUE(endRequestErrorCode.IsSuccess(), L"Client got a reply");
                    Trace.WriteInfo(TraceType, "client got a reply {0}", reply->TraceId());
                    if (++clientReplyCounter == numberOfClients)
                    {
                        clientReceivedAllReplies.Set();
                    }
                },
                Common::AsyncOperationSPtr());
        }

        serverReceivedAllRequests.WaitOne(TimeSpan::FromMinutes(1));

        // Send request from server to client
        Trace.WriteInfo(TraceType, "Sending requests from server to clients...");
        Common::atomic_long serverReplyCounter;
        Common::ManualResetEvent serverReceivedAllReplies(false);
        for (std::wstring const & clientId : clientIdArray)
        {
            server->BeginRequest(
                CreateServerMessage(clientSideActor_),
                clientId,
                Common::TimeSpan::MaxValue,
                [&](AsyncOperationSPtr const & operation)
                {
                    MessageUPtr reply;
                    ErrorCode endRequestErrorCode = server->EndRequest(operation, reply);
                    VERIFY_IS_TRUE(endRequestErrorCode.IsSuccess(), L"server got a reply");
                    Trace.WriteInfo(TraceType, "server got a reply {0}", reply->TraceId());
                    if (++serverReplyCounter == numberOfClients)
                    {
                        serverReceivedAllReplies.Set();
                    }
                },
                Common::AsyncOperationSPtr());
        }

        clientReceivedAllReplies.WaitOne(TimeSpan::FromMinutes(2));
        serverReceivedAllReplies.WaitOne(TimeSpan::FromMinutes(2));

        VERIFY_ARE_EQUAL2(numberOfClients, serverRequestCounter.load());
        VERIFY_ARE_EQUAL2(numberOfClients, serverReplyCounter.load());
        VERIFY_ARE_EQUAL2(numberOfClients, clientRequestCounter.load());
        VERIFY_ARE_EQUAL2(numberOfClients, clientReplyCounter.load());

        LEAVE;
    }

    BOOST_AUTO_TEST_SUITE_END()

    bool IpcTestBase::Setup()
    {
        root_ = std::make_shared<TestRoot>();
        return true;
    }

    void IpcTestBase::OnewayTest(bool secureMode)
    {
        KFinally([this] { Cleanup(); });

        std::wstring clientIdArray[] = {L"client0", L"client1"};
        LONG const numberOfClients = sizeof(clientIdArray)/sizeof(std::wstring);

        auto& server = root_->server_;
        auto& clients = root_->clients_;

        // set up IpcServer
        std::wstring serverListenAddress = TTestUtil::GetListenAddress();
        server = OpenServer(serverListenAddress, secureMode);
        Common::atomic_long serverRequestCounter;
        Common::ManualResetEvent serverReceiveDone(false);

        server->RegisterMessageHandler(
            serverSideActor_,
            [&] (MessageUPtr & msg, IpcReceiverContextUPtr & context)
            {
                Trace.WriteInfo(TraceType, "server received {0} from {1}", msg->TraceId(), context->From);

                bool foundClientId = false;
                for (LONG i = 0; i < numberOfClients; ++ i)
                {
                    if (context->From == clientIdArray[i])
                    {
                        foundClientId = true;
                    }
                }

                VERIFY_IS_TRUE(foundClientId);

                Trace.WriteInfo(TraceType, "server sending response");
                server->SendOneWay(context->From, CreateServerMessage(clientSideActor_));
                if (++ serverRequestCounter == numberOfClients)
                {
                    Trace.WriteInfo(TraceType, "set serverReceiveDone");
                    serverReceiveDone.Set();
                }
            },
            true/*dispatchOnTransportThread*/);

        Common::atomic_long clientMessageCounterArray[numberOfClients];

        Common::atomic_long clientMessageTotal;
        Common::ManualResetEvent clientReceivedAllRequests(false);

        for(int i = 0; i < numberOfClients; i++)
        {
            auto client = OpenClient(clientIdArray[i], serverListenAddress, secureMode);
            clients.push_back(client);
            Common::atomic_long & messageCounter = clientMessageCounterArray[i];

            client->RegisterMessageHandler(
                clientSideActor_,
                [&] (MessageUPtr &, IpcReceiverContextUPtr & context)
                {
                    VERIFY_IS_TRUE(context->From.empty());
                    ++ messageCounter;
                    if (++ clientMessageTotal == numberOfClients * 2)
                    {
                        clientReceivedAllRequests.Set();
                    }
                },
                true/*dispatchOnTransportThread*/);
        }

        for (auto const & client : clients)
        {
            Trace.WriteInfo(TraceType, "{0} sending bad message", client->ClientId);
            client->SendOneWay(CreateClientMessage(Actor::GenericTestActor)); // message will be dropped
            Trace.WriteInfo(TraceType, "{0} sending good message", client->ClientId);
            client->SendOneWay(CreateClientMessage(serverSideActor_));
        }

        VERIFY_IS_TRUE(serverReceiveDone.WaitOne(TimeSpan::FromSeconds(30)));
        Trace.WriteInfo(TraceType, "serverReceiveDone.WaitOne returned, serverRequestCounter = {0}", serverRequestCounter.load());

        // Send from server to clients
        for (std::wstring const & clientId : clientIdArray)
        {
            Trace.WriteInfo(TraceType, "server sending bad message");
            server->SendOneWay(clientId, CreateServerMessage(Actor::GenericTestActor2)); // message will be dropped
            Trace.WriteInfo(TraceType, "server sending good message");
            server->SendOneWay(clientId, CreateServerMessage(clientSideActor_));
        }

        clientReceivedAllRequests.WaitOne(TimeSpan::FromMinutes(2));
        Trace.WriteInfo(
            TraceType,
            "clientReceivedAllRequests.WaitOne returned, serverRequestCounter = {0}, clientMessageTotal = {1}",
            serverRequestCounter.load(), clientMessageTotal.load());

        VERIFY_ARE_EQUAL2(2, serverRequestCounter.load());
        VERIFY_ARE_EQUAL2(numberOfClients * 2, clientMessageTotal.load());
        for (Common::atomic_long const & counter : clientMessageCounterArray)
        {
            auto value = counter.load();
            Trace.WriteInfo(TraceType, "counter = {0}", value);
            VERIFY_ARE_EQUAL2(2, value);
        }
    }

    void IpcTestBase::ReconnectTest(bool secureMode)
    {
        KFinally([this] { Cleanup(); });

        auto saved = TransportConfig::GetConfig().IpcReconnectDelay;
        KFinally([=] { TransportConfig::GetConfig().IpcReconnectDelay = saved; });

        std::wstring clientId = L"clientX";

        auto& server = root_->server_;
        auto& client = root_->client_;

        // set up IpcServer
        std::wstring serverListenAddress = TTestUtil::GetListenAddress();
        server = OpenServer(serverListenAddress, secureMode);
        Common::atomic_long serverRequestCounter;
        auto serverReceivedClientRequest = make_shared<AutoResetEvent>(false);

        server->RegisterMessageHandler(
            serverSideActor_,
            [serverReceivedClientRequest] (MessageUPtr & message, IpcReceiverContextUPtr & context)
            {
                Trace.WriteInfo(
                    TraceType, "ipcServer: got message {0} from {1}",
                    message->TraceId(), context->From);

                Trace.WriteInfo(TraceType, "ipcServer: kill client connection to trigger reconnect");
                context->ReplyTarget->Reset();

                serverReceivedClientRequest->Set();
            },
            true/*dispatchOnTransportThread*/);

        auto clientReceivedServerMessage = make_shared<AutoResetEvent>(false);
        client = OpenClient(clientId, serverListenAddress, secureMode);
        client->RegisterMessageHandler(
            clientSideActor_,
            [clientReceivedServerMessage] (MessageUPtr & message, IpcReceiverContextUPtr & context)
            {
                Trace.WriteInfo(TraceType, "client got message {0} from {1}", message->TraceId(), context->From);
                VERIFY_IS_TRUE(context->From.empty());
                clientReceivedServerMessage->Set();
            },
            false/*dispatchOnTransportThread*/);

        Trace.WriteInfo(TraceType, "First disable reconnect timer to test reconnect when IpcClient sends messages");
        TimeSpan savedIpcReconnectDelay = TransportConfig::GetConfig().IpcReconnectDelay;
        TransportConfig::GetConfig().IpcReconnectDelay = TimeSpan::MaxValue;
        KFinally([=] { TransportConfig::GetConfig().IpcReconnectDelay = savedIpcReconnectDelay; });

        Trace.WriteInfo(TraceType, "IpcClient sending first message");
        client->SendOneWay(CreateClientMessage(serverSideActor_));
        VERIFY_IS_TRUE(serverReceivedClientRequest->WaitOne(TimeSpan::FromSeconds(30)));

        Trace.WriteInfo(TraceType, "IpcServer must have started resetting the connection, now send more messages to prove that IpcClient can send after connection reset");
        bool serverReceivedMessage = false;
        for (int i = 0; i < 300; ++i)
        {
            Trace.WriteInfo(TraceType, "client sending a message");
            client->SendOneWay(CreateClientMessage(serverSideActor_));
            if (serverReceivedClientRequest->WaitOne(TimeSpan::FromMilliseconds(100)))
            {
                serverReceivedMessage = true;
                break;
            }
        }

        VERIFY_IS_TRUE(serverReceivedMessage);

        Trace.WriteInfo(TraceType, "Enable reconnect timer to test reconnect without outgoing messages from IpcClient");
        TransportConfig::GetConfig().IpcReconnectDelay = TimeSpan::FromMilliseconds(100);

        Trace.WriteInfo(TraceType, "IpcClient sends messages to IpcServer, who closes connection on incoming message, which triggers reconnect timer in IpcClient");
        bool serverClosedConnection = false;
        for (int i = 0; i < 300; ++i)
        {
            Trace.WriteInfo(TraceType, "client sending a message to trigger reconnect timer");
            client->SendOneWay(CreateClientMessage(serverSideActor_));
            if (serverReceivedClientRequest->WaitOne(TimeSpan::FromMilliseconds(100)))
            {
                // IpcServer must have closed or started closing the connection
                serverClosedConnection = true;
                break;
            }
        }

        VERIFY_IS_TRUE(serverClosedConnection);

        // sending from IpcServer to IpcClient will not succeed until IpcClient reconnect
        bool received = false;
        for (int i = 0; i < 300; ++i)
        {
            Trace.WriteInfo(TraceType, "ipcServer sending a message");
            server->SendOneWay(clientId, CreateServerMessage(clientSideActor_));
            if ((received = clientReceivedServerMessage->WaitOne(30)) == true)
            {
                break;
            }
        }
        VERIFY_IS_TRUE(received);
    }

    void IpcTestBase::RequestReplyTest(bool secureMode)
    {
        KFinally([this] { Cleanup(); });

        std::wstring clientIdArray[] = {L"client0", L"client1"};
        LONG const numberOfClients = sizeof(clientIdArray)/sizeof(std::wstring);

        auto& server = root_->server_;
        auto& clients = root_->clients_;

        // set up IpcServer
        std::wstring serverListenAddress = TTestUtil::GetListenAddress();
        server = OpenServer(serverListenAddress, secureMode);
        Common::atomic_long serverRequestCounter;
        Common::ManualResetEvent serverReceivedAllRequests(false);

        server->RegisterMessageHandler(
            serverSideActor_,
            [&] (MessageUPtr & message, IpcReceiverContextUPtr & context)
            {
                bool foundClientId = false;
                for (LONG i = 0; i < numberOfClients; ++ i)
                {
                    if (context->From == clientIdArray[i])
                    {
                        foundClientId = true;
                    }
                }

                VERIFY_IS_TRUE(foundClientId);

                if (message->Headers.ExpectsReply)
                {
                    if (++ serverRequestCounter == numberOfClients)
                    {
                        serverReceivedAllRequests.Set();
                    }

                    context->Reply(CreateServerMessage(clientSideActor_));
                }
            },
            true/*dispatchOnTransportThread*/);

        Common::atomic_long clientRequestCounter;
        Common::atomic_long clientReplyCounter;
        Common::ManualResetEvent clientReceivedAllReplies(false);
        for (size_t i = 0; i < numberOfClients; ++ i)
        {
            auto client = OpenClient(clientIdArray[i], serverListenAddress, secureMode);
            clients.push_back(client);
            client->RegisterMessageHandler(
                clientSideActor_,
                [&] (MessageUPtr & message, IpcReceiverContextUPtr & context)
                {
                    Trace.WriteInfo(TraceType, "client got a message {0} {1}", message->TraceId(), message->IsReply());
                    VERIFY_IS_TRUE(context->From.empty());

                    if (message->Headers.ExpectsReply)
                    {
                        Trace.WriteInfo(TraceType, "This is a request from server");
                        ++ clientRequestCounter;
                        context->Reply(CreateClientMessage(serverSideActor_));
                    }
                },
                true/*dispatchOnTransportThread*/);

            // Send request from client to server
            client->BeginRequest(
                CreateClientMessage(serverSideActor_),
                Common::TimeSpan::MaxValue,
                [&] (AsyncOperationSPtr const & operation)
                {
                    MessageUPtr reply;
                    ErrorCode endRequestErrorCode = client->EndRequest(operation, reply);
                    VERIFY_IS_TRUE(endRequestErrorCode.IsSuccess(), L"Client got a reply");
                    Trace.WriteInfo(TraceType, "client got a reply {0}", reply->TraceId());
                    if (++ clientReplyCounter == numberOfClients)
                    {
                        clientReceivedAllReplies.Set();
                    }
                },
                Common::AsyncOperationSPtr());
        }

        serverReceivedAllRequests.WaitOne(TimeSpan::FromMinutes(2));

        // Send request from server to client
        Trace.WriteInfo(TraceType, "Sending requests from server to clients...");
        Common::atomic_long serverReplyCounter;
        Common::ManualResetEvent serverReceivedAllReplies(false);
        for (std::wstring const & clientId : clientIdArray)
        {
            server->BeginRequest(
                CreateServerMessage(clientSideActor_),
                clientId,
                Common::TimeSpan::MaxValue,
                [&] (AsyncOperationSPtr const & operation)
                {
                    MessageUPtr reply;
                    ErrorCode endRequestErrorCode = server->EndRequest(operation, reply);
                    VERIFY_IS_TRUE(endRequestErrorCode.IsSuccess(), L"server got a reply");
                    Trace.WriteInfo(TraceType, "server got a reply {0}", reply->TraceId());
                    if (++ serverReplyCounter == numberOfClients)
                    {
                        serverReceivedAllReplies.Set();
                    }
                },
                Common::AsyncOperationSPtr());
        }

        clientReceivedAllReplies.WaitOne(TimeSpan::FromMinutes(2));
        serverReceivedAllReplies.WaitOne(TimeSpan::FromMinutes(2));

        VERIFY_ARE_EQUAL2(numberOfClients, serverRequestCounter.load());
        VERIFY_ARE_EQUAL2(numberOfClients, serverReplyCounter.load());
        VERIFY_ARE_EQUAL2(numberOfClients, clientRequestCounter.load());
        VERIFY_ARE_EQUAL2(numberOfClients, clientReplyCounter.load());
    }

    void IpcTestBase::Cleanup()
    {
        for(auto & client : root_->clients_)
        {
            CloseClient(client);
        }

        if (root_->client_) CloseClient(root_->client_);
        if (root_->server_) CloseServer(root_->server_);

        root_.reset();
    }

    IpcServerSPtr IpcTestBase::OpenServer(std::wstring const & serverAddress, bool secureMode)
    {
        return OpenServer(serverAddress, L"", secureMode);
    }

    IpcServerSPtr IpcTestBase::OpenServer(wstring const & serverAddress, wstring const & serverAddress2, bool secureMode)
    {
        IpcServerSPtr server = std::make_shared<IpcServer>(
            *root_,
            serverAddress,
            serverAddress2,
            L"IpcTestBase",
            false /* disallow use of unreliable transport */,
            L"IpcTestBase");

        if (secureMode)
        {
            SecuritySettings securitySettings;
            auto error = SecuritySettings::CreateNegotiateServer(L"", securitySettings);
            VERIFY_IS_TRUE(error.IsSuccess());

            VERIFY_IS_TRUE(server->SetSecurity(securitySettings).IsSuccess());
        }

        auto errorCode = server->Open();
        VERIFY_IS_TRUE(errorCode.IsSuccess(), L"IpcServer opened.");
        
        return server;
    }

    IpcClientSPtr IpcTestBase::OpenClient(
        std::wstring const & clientId,
        std::wstring const & serverAddress,
        bool secureMode)
    {
        IpcClientSPtr client = std::make_shared<IpcClient>(
            *root_, clientId, serverAddress, false /* disallow use of unreliable transport */, L"IpcTestBase");

        if (secureMode)
        {
            SecuritySettings securitySettings;
            auto error = SecuritySettings::CreateNegotiateClient(
                TransportSecurity().LocalWindowsIdentity(),
                securitySettings);

            VERIFY_IS_TRUE(error.IsSuccess());
            client->SecuritySettings = securitySettings;
        }

        auto errorCode = client->Open();
        VERIFY_IS_TRUE(errorCode.IsSuccess(), L"IpcClient opened.");
      
        return client;
    }

    void IpcTestBase::CloseClient(IpcClientSPtr client)
    {
        ErrorCode error = client->Close();
        VERIFY_IS_TRUE(error.IsSuccess());
    }

    void IpcTestBase::CloseServer(IpcServerSPtr server)
    {
        ErrorCode error = server->Close();
        VERIFY_IS_TRUE(error.IsSuccess());
    }

    MessageUPtr IpcTestBase::CreateServerMessage(Actor::Enum actor)
    {
        MessageUPtr message = Common::make_unique<Message>();

        message->Headers.Add(MessageIdHeader());
        message->Headers.Add(ActorHeader(actor));

        return message;
    }

    MessageUPtr IpcTestBase::CreateClientMessage(Actor::Enum actor)
    {
        MessageUPtr message = Common::make_unique<Message>();

        message->Headers.Add(MessageIdHeader());
        message->Headers.Add(ActorHeader(actor));

        return message;
    }

    struct IpcTestMessage : public Serialization::FabricSerializable
    {
        IpcTestMessage()
        {
        }

        IpcTestMessage(std::wstring const & message)
            : message_(message)
        {
        }

        FABRIC_FIELDS_01(message_);

        std::wstring message_;
    };

    MessageUPtr IpcTestBase::CreateClientMessageLong(Actor::Enum actor)
    {
        std::wstring requestBodyString;
        while(requestBodyString.size() * sizeof(wchar_t) < TransportSecurity::GetInternalMaxFrameSize(TransportConfig::GetConfig().IpcMaxMessageSize))
        {
            requestBodyString += L"Message ";
        }
        requestBodyString += L"oh yeah now message too long";
        IpcTestMessage requestBody(requestBodyString); 
        MessageUPtr message = make_unique<Message>(requestBody);
        
        //MessageUPtr message = Common::make_unique<Message>();

        message->Headers.Add(ActorHeader(actor));

        return message;
    }
}
