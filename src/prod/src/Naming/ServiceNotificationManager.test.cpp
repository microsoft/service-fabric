// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

#include "MockServiceTableEntryCache.h"
#include "client/Client.Internal.h"
#include "Transport/TransportConfig.h"
#include "Transport/UnreliableTransportSpecification.h"
#include "Transport/UnreliableTransportConfig.h"

using namespace std;

using namespace Common;
using namespace Transport;
using namespace Reliability;
using namespace ServiceModel;
using namespace Federation;
using namespace Client;
using namespace ClientServerTransport;
using namespace Naming;

#define VERIFY(condition, ...) if (!(condition)) { DebugBreak(); } VERIFY_IS_TRUE(condition, wformatString(__VA_ARGS__).c_str());

StringLiteral const TraceComponent("ServiceNotificationManagerTest");

const int ReOpenDelayInMilliseconds = 2000;

class ServiceNotificationManagerTest
{
protected:
    ServiceNotificationManagerTest() { BOOST_REQUIRE(TestcaseSetup()); }
    TEST_METHOD_SETUP(TestcaseSetup)

    /*
    This test needs to be enabled after functionality for counting pending notifications at
    both entreeservice and proxy is added
    */
    void DeadlockedClientTest();

};

//
// Helper Classes (mock gateway and client)
//

// This class maintains the transport level datastructures for the gateway
//
class MockTransportRoot : public ComponentRoot
{
public:

    MockTransportRoot(
        uint64 nodeId,
        USHORT entreeServiceProxyListenAddress)
        : entreeServiceProxyListenAddress_(entreeServiceProxyListenAddress)
        , entreeServiceListenAddress_(entreeServiceProxyListenAddress + 1)
        , nodeInstance_(LargeInteger(0, nodeId), DateTime::Now().Ticks)
    {
    }

    __declspec(property(get=get_Id)) Federation::NodeId Id;
    Federation::NodeId get_Id() const { return nodeInstance_.Id; };

    __declspec(property(get=get_Instance)) Federation::NodeInstance Instance;
    Federation::NodeInstance get_Instance() const { return nodeInstance_; };

    wstring const & GetListenAddress() const { return receiver_->ListenAddress(); }
    USHORT GetEntreeServiceProxyPort() const { return entreeServiceProxyListenAddress_; }
    USHORT GetEntreeServicePort() const { return entreeServiceListenAddress_; }

    IDatagramTransportSPtr const & GetReceiver() const { return receiver_; }
    DemuxerUPtr const & GetDemuxer() const { return demuxer_; }
    RequestReplySPtr const & GetRequestReply() const { return requestReply_; }
    unique_ptr<ProxyToEntreeServiceIpcChannel> const & GetProxyIpcClient() const { return proxyIpcClient_; }
    EntreeServiceTransportSPtr const & GetEntreeServiceTransport() const { return entreeServiceTransport_; }

    void Open()
    {
        nodeInstance_ = NodeInstance(nodeInstance_.Id, nodeInstance_.InstanceId + 1);

        receiver_ = DatagramTransportFactory::CreateTcp(
            wformatString("127.0.0.1:{0}", entreeServiceProxyListenAddress_), 
            nodeInstance_.ToString(), 
            L"MockTransportRootToClient");

        receiver_->SetConnectionIdleTimeout(TimeSpan::MaxValue);
        receiver_->SetKeepAliveTimeout(TimeSpan::FromSeconds(2));
        receiver_->DisableSecureSessionExpiration();

        demuxer_ = make_unique<Demuxer>(*this, receiver_);

        auto error = demuxer_->Open();
        VERIFY(error.IsSuccess(), "MockTransportRoot demuxer open: {0}", error);

        error = receiver_->Start();
        VERIFY(error.IsSuccess(), "MockTransportRoot receiver open: {0}", error);

        requestReply_ = make_shared<RequestReply>(*this, receiver_);
        requestReply_->Open();
        demuxer_->SetReplyHandler(*requestReply_);

        entreeServiceTransport_ = make_shared<EntreeServiceTransport>(
            *this, 
            wformatString("127.0.0.1:{0}", entreeServiceListenAddress_), 
            nodeInstance_,
            L"MockTransportRoot",
            eventSource_,
            true); // use unreliable transport

        error = entreeServiceTransport_->Open();
        VERIFY(error.IsSuccess(), "EntreeServiceTransport open: {0}", error);

        proxyIpcClient_ = make_unique<ProxyToEntreeServiceIpcChannel>(
            *this, 
            nodeInstance_.ToString(), 
            entreeServiceTransport_->TransportListenAddress);

        auto waiter = make_shared<AsyncOperationWaiter>();
        auto op = proxyIpcClient_->BeginOpen(
            TimeSpan::FromSeconds(30),
            [this, waiter](AsyncOperationSPtr const &)
            {
                waiter->Set();
            },
            this->CreateAsyncOperationRoot());

        waiter->WaitOne();
        error = proxyIpcClient_->EndOpen(op);
        VERIFY(error.IsSuccess(), "ProxyIpcClient open : {0}", error);
    }

    virtual void Close()
    {
        auto waiter = make_shared<AsyncOperationWaiter>();
        auto op = proxyIpcClient_->BeginClose(
            TimeSpan::FromSeconds(30),
            [this, waiter](AsyncOperationSPtr const &)
            {
                waiter->Set();
            },
            this->CreateAsyncOperationRoot());

        waiter->WaitOne();
        auto error = proxyIpcClient_->EndClose(op);
        VERIFY(error.IsSuccess(), "ProxyIpcClient close : {0}", error);

        error = entreeServiceTransport_->Close();
        VERIFY(error.IsSuccess(), "EntreeServiceTransport close : {0}", error);

        receiver_->Stop();

        requestReply_->Close();

        error = demuxer_->Close();
        VERIFY(error.IsSuccess(), "MockTransportRoot transport close: {0}", error);
    }

private:

    USHORT entreeServiceProxyListenAddress_;
    USHORT entreeServiceListenAddress_;
    NodeInstance nodeInstance_;

    IDatagramTransportSPtr receiver_;
    DemuxerUPtr demuxer_;
    RequestReplySPtr requestReply_;
    unique_ptr<ProxyToEntreeServiceIpcChannel> proxyIpcClient_;
    EntreeServiceTransportSPtr entreeServiceTransport_;
    GatewayEventSource eventSource_;
};

// Wrapper around transport components to support re-open
//
class MockGatewayBase : public ComponentRoot
{
private:

    static GlobalWString DelayNotificationsSpecName;

public:
    MockGatewayBase(uint64 nodeId)
        : transportRoot_()
    {
        USHORT port = 0;
        TestPortHelper::GetPorts(2, port);

        transportRoot_ = make_shared<MockTransportRoot>(nodeId, port);
    }

    __declspec(property(get=get_Id)) Federation::NodeId Id;
    Federation::NodeId get_Id() const { return transportRoot_->Id; };

    __declspec(property(get=get_Instance)) Federation::NodeInstance Instance;
    Federation::NodeInstance get_Instance() const { return transportRoot_->Instance; };

    wstring const & GetListenAddress() const { return transportRoot_->GetListenAddress(); }
    USHORT GetEntreeServiceProxyPort() const { return transportRoot_->GetEntreeServiceProxyPort(); }
    USHORT GetEntreeServicePort() const { return transportRoot_->GetEntreeServicePort(); }

    shared_ptr<MockTransportRoot> const & GetTransportRoot() const { return transportRoot_; }

    void CloseAndReOpen()
    {
        this->Close();
        this->ReOpen(TimeSpan::Zero);
    }

    void ReOpen()
    {
        this->ReOpen(TimeSpan::Zero);
    }

    void ReOpenWithDelayedNotifications()
    {
        this->ReOpen(TimeSpan::FromMilliseconds(ReOpenDelayInMilliseconds));
    }

    void ReOpen(TimeSpan const & delay)
    {
        if (delay > TimeSpan::Zero)
        {
            // Delay notifications on re-open when we want filter re-registration
            // and synchronization to complete before any notifications are
            // actually received by the client. This is needed to avoid
            // synchronization buffering so that some testcases can accurately count
            // the expected number of notifications.
            //
            wstring delayNotificationsSpec = wformatString("* * ServiceNotificationRequest 1.0 {0}", (double)delay.TotalMilliseconds() / 1000);
            UnreliableTransportConfig::GetConfig().AddSpecification(DelayNotificationsSpecName, delayNotificationsSpec);
        }

        // All transport components defer lifetime responsibilities to
        // their component root
        //
        auto recreatedTransportRoot = make_shared<MockTransportRoot>(
            this->Id.IdValue.Low,
            this->GetTransportRoot()->GetEntreeServiceProxyPort());

        transportRoot_ = move(recreatedTransportRoot);

        this->Open();
    }

    void ResetDelayedNotifications()
    {
        UnreliableTransportConfig::GetConfig().RemoveSpecification(DelayNotificationsSpecName);
    }

protected:

    virtual void Open()
    {
        transportRoot_->Open();
    }

    virtual void Close()
    {
        transportRoot_->Close();
    }

private:

    shared_ptr<MockTransportRoot> transportRoot_;
};

GlobalWString MockGatewayBase::DelayNotificationsSpecName = make_global<wstring>(L"DelayNotifications");

// This class wraps the duplex request reply client connection
//
class MockClientBase : public ComponentRoot
{
public:
    MockClientBase(MockGatewayBase const & mockGateway)
        : sender_()
        , requestReply_()
        , demuxer_()
        , gatewayAddresses_()
    {
        gatewayAddresses_.push_back(mockGateway.GetListenAddress());
    }

    MockClientBase(vector<shared_ptr<MockGatewayBase>> const & mockGateways)
        : sender_()
        , requestReply_()
        , demuxer_()
        , gatewayAddresses_()
    {
        for (auto const & it : mockGateways)
        {
            gatewayAddresses_.push_back(it->GetListenAddress());
        }
    }

protected:

    IDatagramTransportSPtr const & GetClientTransport() { return sender_; }
    shared_ptr<DuplexRequestReply> const & GetDuplexRequestReply() { return requestReply_; }
    vector<wstring> const & GetGatewayAddresses() { return gatewayAddresses_; }

    virtual void Open()
    {
        auto transport = DatagramTransportFactory::CreateTcpClient();
        sender_ = DatagramTransportFactory::CreateUnreliable(*this, transport);
        requestReply_ = make_shared<DuplexRequestReply>(*this, sender_);
        requestReply_->Open();
        demuxer_ = make_shared<Demuxer>(*this, sender_);
        demuxer_->SetReplyHandler(*requestReply_);

        auto error = demuxer_->Open();
        VERIFY(error.IsSuccess(), "MockClient demuxer open: {0}", error);

        error = sender_->Start();
        VERIFY(error.IsSuccess(), "MockClient sender open: {0}", error);
    }

    virtual void Close()
    {
        sender_->Stop();

        auto error = demuxer_->Close();
        VERIFY(error.IsSuccess(), "MockClient demuxer close: {0}", error);

        requestReply_->Close();
    }

private:
    IDatagramTransportSPtr sender_;
    DuplexRequestReplySPtr requestReply_;
    DemuxerSPtr demuxer_;

    vector<wstring> gatewayAddresses_;
};

const GatewayEventSource Events;

// Maintains service notification manager components
//
class MockNotificationManagerRoot : public ComponentRoot
{
public:
    MockNotificationManagerRoot(
        shared_ptr<MockTransportRoot> const & transportRoot,
        shared_ptr<MockServiceTableEntryCache> const & cache)
        : transportRoot_(transportRoot)
        , cache_(cache)
        , notificationManager_()
        , notificationManagerProxy_()
        , notificationManagerLock_()
    {
    }

    GatewayDescription GetGatewayDescription() const 
    { 
        return this->GetNotificationManagerUnderLock()->CreateGatewayDescription(); 
    }

    size_t GetTotalPendingTransportMessageCount(wstring const & clientId) const
    {
        return notificationManagerProxy_->GetTotalPendingTransportMessageCount(clientId);
    }

    void Open()
    {
        this->CreateNotificationManagers();

        auto error = notificationManager_->Open(transportRoot_->GetEntreeServiceTransport());
        VERIFY(error.IsSuccess(), "ServiceNotificationManager::Open {0}", error);
        
        error = notificationManagerProxy_->Open();
        VERIFY(error.IsSuccess(), "ServiceNotificationManagerProxy::Open {0}", error);

        auto selfRoot = this->CreateAsyncOperationRoot();
        transportRoot_->GetEntreeServiceTransport()->RegisterMessageHandler(
            Actor::NamingGateway,
            [this, selfRoot] (MessageUPtr &message, TimeSpan const timeout, AsyncCallback const &callback, AsyncOperationSPtr const &parent)
            {
                return AsyncOperation::CreateAndStart<ProcessIpcRequestAsyncOperation>(
                    *this,
                    move(message),
                    timeout,
                    callback,
                    parent);
            },
            [this, selfRoot] (AsyncOperationSPtr const &operation, __out MessageUPtr &reply)
            {
                return ProcessIpcRequestAsyncOperation::End(operation, reply);
            });

        transportRoot_->GetDemuxer()->RegisterMessageHandler(
            Actor::NamingGateway,
            [this, selfRoot](MessageUPtr & msg, ReceiverContextUPtr & receiverContext)
            {
                this->ProcessMessage(msg, move(receiverContext));
            },
            false);
    }

    void Close()
    {
        AcquireWriteLock lock(notificationManagerLock_);

        auto error = notificationManagerProxy_->Close();
        VERIFY(error.IsSuccess(), "ServiceNotificationManagerProxy::Close {0}", error);
        transportRoot_->GetDemuxer()->UnregisterMessageHandler(Actor::NamingGateway);

        error = notificationManager_->Close();
        VERIFY(error.IsSuccess(), "ServiceNotificationManager::Close {0}", error);
        transportRoot_->GetEntreeServiceTransport()->UnregisterMessageHandler(Actor::NamingGateway);
    }

private:

    class ProcessIpcRequestAsyncOperation : public TimedAsyncOperation
    {
    public:
        ProcessIpcRequestAsyncOperation(
            MockNotificationManagerRoot &owner,
            MessageUPtr &&message,
            TimeSpan const timeout,
            AsyncCallback const &callback,
            AsyncOperationSPtr const &parent)
            : TimedAsyncOperation(timeout, callback, parent)
            , owner_(owner)
            , request_(move(message))
            , reply_()
        {
        }

        static ErrorCode End(AsyncOperationSPtr const &operation, __out MessageUPtr &reply)
        {
            auto casted = AsyncOperation::End<ProcessIpcRequestAsyncOperation>(operation);
            if (casted->Error.IsSuccess())
            {
                reply = move(casted->reply_);
            }
            return casted->Error;
        }

    protected:
        void OnStart(AsyncOperationSPtr const &thisSPtr)
        {
            auto error = owner_.GetNotificationManagerUnderLock()->ProcessRequest(request_, reply_);

            this->TryComplete(thisSPtr, error);
        }

        MockNotificationManagerRoot &owner_;
        MessageUPtr request_;
        MessageUPtr reply_;
    };

private:

    class ProcessRequestAsyncOperation : public AsyncOperation
    {
    public:
        ProcessRequestAsyncOperation(
            MockNotificationManagerRoot &owner,
            MessageUPtr &&message,
            ISendTarget::SPtr const&sender,
            TimeSpan const &timeout,
            AsyncCallback const &callback,
            AsyncOperationSPtr const &parent)
            : AsyncOperation(callback, parent)
            , owner_(owner)
            , message_(move(message))
            , sender_(sender)
            , timeout_(timeout)
        {
        }

        static ErrorCode End(AsyncOperationSPtr const &operation, __out MessageUPtr &reply)
        {
            auto casted = AsyncOperation::End<ProcessRequestAsyncOperation>(operation);
            if (casted->Error.IsSuccess())
            {
                reply = move(casted->reply_);
            }

            return casted->Error;
        }

    protected:
        void OnStart(AsyncOperationSPtr const &thisSPtr)
        {
            if (message_->Action == NamingTcpMessage::PingRequestAction)
            {
                reply_ = NamingTcpMessage::GetGatewayPingReply(
                    Common::make_unique<PingReplyMessageBody>(owner_.GetGatewayDescription()))->GetTcpMessage();

                reply_->Headers.Add(*ClientProtocolVersionHeader::CurrentVersionHeader);
                this->TryComplete(thisSPtr, ErrorCodeValue::Success);
            }
            else
            {
                auto operation = owner_.notificationManagerProxy_->BeginProcessRequest(
                    move(message_), 
                    sender_, 
                    timeout_, 
                    [this](AsyncOperationSPtr const &operation) { this->OnProcessRequestComplete(operation, false); },
                    thisSPtr);
                this->OnProcessRequestComplete(operation, true);
            }
        }

    private:

        void OnProcessRequestComplete(
            AsyncOperationSPtr const & operation, 
            bool expectedCompletedSynchronously)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

            auto error = owner_.notificationManagerProxy_->EndProcessRequest(operation, reply_);

            this->TryComplete(operation->Parent, error);
        }

    private:
        MockNotificationManagerRoot & owner_;
        ISendTarget::SPtr sender_;
        MessageUPtr message_;
        TimeSpan timeout_;
        MessageUPtr reply_;
    };

    AsyncOperationSPtr BeginProcessRequest(
        MessageUPtr && message,
        ISendTarget::SPtr const &sender,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        return AsyncOperation::CreateAndStart<ProcessRequestAsyncOperation>(
            *this,
            move(message),
            sender,
            timeout,
            callback,
            parent);
    }

    ErrorCode EndProcessRequest(
        AsyncOperationSPtr const &operation,
        __out MessageUPtr &reply)
    {
        return ProcessRequestAsyncOperation::End(operation, reply);
    }

    void ProcessMessage(MessageUPtr & msg, ReceiverContextUPtr &&context)
    {
        auto id = msg->MessageId;
        ManualResetEvent waitForCompletion;
        auto operation = this->BeginProcessRequest(
            move(msg), 
            context->ReplyTarget, 
            TimeSpan::MaxValue,
            [&waitForCompletion](AsyncOperationSPtr const&)
            {
                waitForCompletion.Set();
            },
            this->CreateAsyncOperationRoot());

        waitForCompletion.WaitOne();

        this->OnProcessMessageComplete(operation, id, move(context));
    }

    void OnProcessMessageComplete(AsyncOperationSPtr const &thisSPtr, MessageId id, ReceiverContextUPtr &&context)
    {
        MessageUPtr reply;
        auto error = this->EndProcessRequest(thisSPtr, reply);
        if (!error.IsSuccess())
        {
            reply = NamingMessage::GetClientOperationFailure(id, move(error));
        }
        context->Reply(move(reply));
    }

private:

    shared_ptr<ServiceNotificationManager> GetNotificationManagerUnderLock() const
    {
        AcquireReadLock lock(notificationManagerLock_);

        return notificationManager_;
    }

    void CreateNotificationManagers()
    {
        notificationManager_ = make_shared<ServiceNotificationManager>(
            transportRoot_->Instance,
            wformatString("node:{0}", transportRoot_->Instance),
            *cache_, 
            Events,
            *this);

        notificationManagerProxy_ = make_shared<ServiceNotificationManagerProxy>(
            *this,
            transportRoot_->Instance,
            transportRoot_->GetRequestReply(),
            *transportRoot_->GetProxyIpcClient(),
            transportRoot_->GetReceiver());
    }

    shared_ptr<MockTransportRoot> transportRoot_;
    shared_ptr<MockServiceTableEntryCache> cache_;

    shared_ptr<ServiceNotificationManager> notificationManager_;
    shared_ptr<ServiceNotificationManagerProxy> notificationManagerProxy_;
    mutable RwLock notificationManagerLock_;
};

// Wraps notification manager components to support re-open
//
class MockNotificationGateway : public MockGatewayBase
{
public:
    MockNotificationGateway(uint64 nodeId)
        : MockGatewayBase(nodeId) 
        , cache_(make_shared<MockServiceTableEntryCache>())
        , notificationManagerRoot_()
    {
    }

    MockNotificationGateway(uint64 nodeId, uint64 generation)
        : MockGatewayBase(nodeId) 
        , cache_(make_shared<MockServiceTableEntryCache>(generation))
        , notificationManagerRoot_()
    {
    }

    MockServiceTableEntryCache & GetMockCache() { return *cache_; }

    GatewayDescription GetGatewayDescription() const 
    { 
        return notificationManagerRoot_->GetGatewayDescription(); 
    }

    size_t GetTotalPendingTransportMessageCount(wstring const & clientId) const
    {
        return notificationManagerRoot_->GetTotalPendingTransportMessageCount(clientId);
    }

    virtual void Open()
    {
        MockGatewayBase::Open();

        // The cache is not cleared on re-open in order to simulate
        // client disconnections
        //
        notificationManagerRoot_ = make_shared<MockNotificationManagerRoot>(
            this->GetTransportRoot(),
            cache_);

        notificationManagerRoot_->Open();
    }

    virtual void Close()
    {
        notificationManagerRoot_->Close();

        MockGatewayBase::Close();
    }

    void VerifyVersions(VersionRangeCollection const & expectedVersions)
    {
        auto const & cacheVersions = cache_->GetVersions();

        VERIFY(
            cacheVersions.VersionRanges.size() == expectedVersions.VersionRanges.size(),
            "version ranges size mismatch: expected={0} actual={1}",
            expectedVersions,
            cacheVersions);

        auto expectedIt = expectedVersions.VersionRanges.begin();
        auto actualIt = cacheVersions.VersionRanges.begin();

        while (expectedIt != expectedVersions.VersionRanges.end())
        {
            VERIFY(
                expectedIt->StartVersion == actualIt->StartVersion,
                "start version mismatch: expected={0} actual={1}",
                *expectedIt,
                *actualIt);

            VERIFY(
                expectedIt->EndVersion == actualIt->EndVersion,
                "end version mismatch: expected={0} actual={1}",
                *expectedIt,
                *actualIt);

            ++expectedIt;
            ++actualIt;
        }
    }

    void VerifyEmptyPartitionsCount(size_t expected)
    {
        auto actual = cache_->GetEmptyPartitionsCount();
        VERIFY(expected == actual, "Empty partitions count: expected={0} actual={1}", expected, actual);
    }

    void VerifyAllPartitionsCount(size_t expected)
    {
        auto actual = cache_->GetAllPartitionsCount();
        VERIFY(expected == actual, "All partitions count: expected={0} actual={1}", expected, actual);
    }

private:

    shared_ptr<MockServiceTableEntryCache> cache_;
    shared_ptr<MockNotificationManagerRoot> notificationManagerRoot_;
};

class MockClientCache : public INotificationClientCache
{
public:
    MockClientCache() 
        : deadLockEvent_(false)
        , enableDeadlock_(false)
    { 
    }

    AsyncOperationSPtr BeginUpdateCacheEntries(
        __in ServiceNotification & notification,
        TimeSpan const,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        if (enableDeadlock_)
        {
            deadLockEvent_.WaitOne();
        }

        return AsyncOperation::CreateAndStart<MockUpdateCacheEntriesAsyncOperation>(notification, callback, parent);
    }

    ErrorCode EndUpdateCacheEntries(
        AsyncOperationSPtr const & operation,
        __out vector<ServiceNotificationResultSPtr> & results)
    {
        return MockUpdateCacheEntriesAsyncOperation::End(operation, results);
    }

public:
    class MockUpdateCacheEntriesAsyncOperation : public AsyncOperation
    {
    public:
        MockUpdateCacheEntriesAsyncOperation(
            __in ServiceNotification & notification,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
            : AsyncOperation(callback, parent)
            , generation_(notification.GetGenerationNumber())
            , stesNotifications_(notification.TakePartitions())
            , results_()
        {
        }

        void OnStart(AsyncOperationSPtr const & thisSPtr)
        {
            for (auto const & steNotification : stesNotifications_)
            {
                results_.push_back(make_shared<ServiceNotificationResult>(
                    steNotification->GetPartition(), 
                    generation_));
            }

            this->TryComplete(thisSPtr, ErrorCodeValue::Success);
        }

        static ErrorCode End(
            AsyncOperationSPtr const & operation,
            __out vector<ServiceNotificationResultSPtr> & results)
        {
            auto casted = AsyncOperation::End<MockUpdateCacheEntriesAsyncOperation>(operation);

            if (casted->Error.IsSuccess())
            {
                results = move(casted->results_);
            }

            return casted->Error;
        }

    private:
        GenerationNumber generation_;
        vector<ServiceTableEntryNotificationSPtr> stesNotifications_;
        vector<ServiceNotificationResultSPtr> results_;
    };

    void EnableDeadlock() 
    { 
        deadLockEvent_.Reset();
        enableDeadlock_ = true; 
    }

    void DisableDeadlock() 
    { 
        deadLockEvent_.Set();
        enableDeadlock_ = false; 
    }

private:
    ManualResetEvent deadLockEvent_;
    bool enableDeadlock_;
};

class MockClientSettings : public INotificationClientSettings
{
public:
    MockClientSettings(unique_ptr<FabricClientInternalSettings> && settings) : settings_(move(settings)) { }

    virtual FabricClientInternalSettings * operator -> () const
    {
        return settings_.get();
    }

private:
    unique_ptr<FabricClientInternalSettings> settings_;
};

class MockNotificationClient : public MockClientBase
{
private:

    static const int WaitOneTimeoutInMilliseconds = 30000;

public:
    MockNotificationClient(
        wstring const & clientId,
        MockGatewayBase const & gateway,
        INamingMessageProcessorSPtr const &namingMessageProcessor = INamingMessageProcessorSPtr())
        : MockClientBase(gateway) 
        , clientId_(clientId)
        , namingMessageProcessor_(namingMessageProcessor)
        , clientConnection_()
        , clientCache_()
        , innerClient_()
        , notificationEvent_(false)
        , notificationCount_(0)
        , expectedCountsSet_(false)
        , lastNotifications_()
        , lastNotificationsLock_()
        , connectionHandlersId_(0)
        , connectionEvent_(false)
        , disconnectionEvent_(false)
        , currentTarget_()
        , currentGateway_()
    { 
    }

    MockNotificationClient(
        wstring const & clientId,
        vector<shared_ptr<MockGatewayBase>> const & gateways) 
        : MockClientBase(gateways)
        , clientId_(clientId)
        , namingMessageProcessor_()
        , clientConnection_()
        , clientCache_()
        , innerClient_()
        , notificationEvent_(false)
        , notificationCount_(0)
        , expectedCountsSet_(false)
        , lastNotifications_()
        , lastNotificationsLock_()
        , connectionHandlersId_(0)
        , connectionEvent_(false)
        , disconnectionEvent_(false)
        , currentTarget_()
        , currentGateway_()
    { 
    }

    wstring const & GetClientId() const { return clientId_; }

    virtual void Open()
    {
        MockClientBase::Open();

        vector<wstring> addresses = this->GetGatewayAddresses();

        // Only ConnectionInitializationTimeoutInSeconds  is relevant
        FabricClientSettings settings1(
            0, // partitionLocationCacheLimit
            0, // keepAliveIntervalInSeconds 
            0, // serviceChangePollIntervalInSeconds
            2, // connectionInitializationTimeoutInSeconds
            0, // healthOperationTimeoutInSeconds
            0, // healthReportSendIntervalInSeconds
            0);// connectionIdleTimeoutInSeconds
        settings1.SetNotificationGatewayConnectionTimeoutInSeconds(10);
        settings1.SetNotificationCacheUpdateTimeoutInSeconds(10);

        // if namingmessageprocessor is not null, this ignores the addresses and 
        // creates the passthrough transport.
        clientConnection_ = make_unique<ClientConnectionManager>(
            clientId_,
            make_unique<MockClientSettings>(make_unique<FabricClientInternalSettings>(clientId_, move(settings1))),
            move(addresses),
            namingMessageProcessor_,
            *this);

        auto selfRoot = this->CreateAsyncOperationRoot();

        connectionHandlersId_ = clientConnection_->RegisterConnectionHandlers(
            [this, selfRoot](ClientConnectionManager::HandlerId id, ISendTarget::SPtr const & st, GatewayDescription const & g)
            {
                this->OnConnected(id, st, g);
            },
            [this, selfRoot](ClientConnectionManager::HandlerId id, ISendTarget::SPtr const & st, GatewayDescription const & g, ErrorCode const & err)
            {
                this->OnDisconnected(id, st, g, err);
            },
            [this, selfRoot](ClientConnectionManager::HandlerId, shared_ptr<ClaimsRetrievalMetadata> const &, __out wstring &)
            {
                return ErrorCodeValue::NotImplemented;
            });

        FabricClientSettings settings2(
            0, // partitionLocationCacheLimit
            0, // keepAliveIntervalInSeconds 
            0, // serviceChangePollIntervalInSeconds
            2, // connectionInitializationTimeoutInSeconds
            0, // healthOperationTimeoutInSeconds
            0, // healthReportSendIntervalInSeconds
            0);// connectionIdleTimeoutInSeconds

        settings2.SetNotificationGatewayConnectionTimeoutInSeconds(10);
        settings2.SetNotificationCacheUpdateTimeoutInSeconds(10);

        innerClient_ = make_unique<ServiceNotificationClient>(
            clientId_, 
            make_unique<MockClientSettings>(make_unique<FabricClientInternalSettings>(clientId_, move(settings2))),
            *clientConnection_,
            clientCache_,
            [this](vector<ServiceNotificationResultSPtr> const & notificationResults) -> ErrorCode
            {
                return this->OnNotificationReceived(notificationResults);
            },
            *this);

        auto error = innerClient_->Open();
        VERIFY(error.IsSuccess(), "{0}: ServiceNotificationClient::Open {1}", clientId_, error);

        this->WaitForConnection();
    }

    virtual void Close()
    {
        auto error = innerClient_->Close();
        VERIFY(error.IsSuccess(), "{0}: ServiceNotificationClient::Close {1}", clientId_, error);

        clientConnection_->UnregisterConnectionHandlers(connectionHandlersId_);

        MockClientBase::Close();
    }

public: // Test Functions

    void EnableReceiveDeadlock()
    {
        clientCache_.EnableDeadlock();
    }

    void DisableReceiveDeadlock()
    {
        clientCache_.DisableDeadlock();
    }

    void WaitForConnection()
    {
        auto result = connectionEvent_.WaitOne(WaitOneTimeoutInMilliseconds);
        VERIFY(result, "WaitForConnection timeout: {0}ms", WaitOneTimeoutInMilliseconds);

        connectionEvent_.Reset();
    }

    void WaitForDisconnection(bool expectedResult = true)
    {
        auto result = disconnectionEvent_.WaitOne(WaitOneTimeoutInMilliseconds);
        VERIFY(result == expectedResult, "WaitForDisconnection timeout: {0}ms", WaitOneTimeoutInMilliseconds);

        disconnectionEvent_.Reset();
    }

    void WaitForReconnection()
    {
        this->WaitForDisconnection();
        this->WaitForConnection();
    }

    static void WaitForDisconnection(vector<shared_ptr<MockNotificationClient>> const & clients)
    {
        for (auto const & client : clients)
        {
            client->WaitForDisconnection();
        }
    }

    void VerifyConnection(shared_ptr<MockNotificationGateway> const & gateway)
    {
        VERIFY(
            innerClient_->CurrentGateway == gateway->GetGatewayDescription(),
            "Gateway connection: client={0} gateway={1}",
            innerClient_->CurrentGateway,
            gateway->GetGatewayDescription());
    }

    void ReOpenCurrentGateway(vector<shared_ptr<MockNotificationGateway>> const & gateways)
    {
        for (auto const & gateway : gateways)
        {
            GatewayDescription currentGateway;
            if (clientConnection_->IsCurrentGateway(gateway->GetGatewayDescription(), currentGateway))
            {
                Trace.WriteInfo(
                    TraceComponent, 
                    clientConnection_->TraceId,
                    "Re-Opening current gateway {0}",
                    currentGateway_);

                gateway->CloseAndReOpen();

                return;
            }
            else
            {
                Trace.WriteInfo(
                    TraceComponent, 
                    clientConnection_->TraceId,
                    "Skipping non-current gateway: target={0} current={1}",
                    currentGateway_,
                    currentGateway);
            }
        }
    }

    void RegisterExactFilter(wstring const & name)
    {
        this->RegisterFilter(name, ServiceNotificationFilterFlags::None);
    }

    void RegisterPrefixFilter(wstring const & name)
    {
        this->RegisterFilter(name, ServiceNotificationFilterFlags::NamePrefix);
    }

    void RegisterPrimaryOnlyExactFilter(wstring const & name)
    {
        this->RegisterFilter(name, ServiceNotificationFilterFlags::PrimaryOnly);
    }

    void RegisterPrimaryOnlyPrefixFilter(wstring const & name)
    {
        this->RegisterFilter(name, ServiceNotificationFilterFlags::NamePrefix | ServiceNotificationFilterFlags::PrimaryOnly);
    }

    void RegisterFilter(wstring const & name, ServiceNotificationFilterFlags const & flags)
    {
        auto filter = make_shared<ServiceNotificationFilter>(
            NamingUri(name), 
            flags);

        ManualResetEvent event(false);
        auto timeout = TimeSpan::FromMilliseconds(WaitOneTimeoutInMilliseconds / 3);

        auto operation = innerClient_->BeginRegisterFilter(
            ActivityId(),
            filter,
            timeout,
            [&](AsyncOperationSPtr const &) { event.Set(); },
            AsyncOperationSPtr());

        auto result = event.WaitOne(static_cast<int>(timeout.TotalMilliseconds()));
        VERIFY(result, "RegisterFilter timeout: {0}", timeout);

        uint64 filterId;
        auto error = innerClient_->EndRegisterFilter(operation, filterId);

        VERIFY(error.IsSuccess(), "{0}: End-RegisterFilter: filterId={1} error={2}", clientId_, filterId, error);
    }

    void UnregisterFilter(wstring const & name)
    {
        bool filterFound = false;
        uint64 filterId = 0;

        for (auto const & it : innerClient_->RegisteredFilters)
        {
            if (it.second->Name.ToString() == wformatString("fabric:/{0}", name))
            {
                filterId = it.first;
                filterFound = true;
                break;
            }
        }

        VERIFY(filterFound, "filter: {0}", name);

        ManualResetEvent event(false);
        auto timeout = TimeSpan::FromMilliseconds(WaitOneTimeoutInMilliseconds / 3);

        auto operation = innerClient_->BeginUnregisterFilter(
            ActivityId(),
            filterId,
            timeout,
            [&](AsyncOperationSPtr const &) { event.Set(); },
            AsyncOperationSPtr());

        auto result = event.WaitOne(static_cast<int>(timeout.TotalMilliseconds()));
        VERIFY(result, "UnregisterFilter timeout: {0}", timeout);

        auto error = innerClient_->EndUnregisterFilter(operation);

        VERIFY(error.IsSuccess(), "{0}: End-UnregisterFilter: filterId={1} error={2}", clientId_, filterId, error);
    }

    void SetExpectedNotificationCount(size_t count)
    {
        if (!expectedCountsSet_)
        {
            notificationEvent_.Reset();
            notificationCount_.store(static_cast<LONG>(count));
            expectedCountsSet_ = true;

            AcquireWriteLock lock(lastNotificationsLock_);

            lastNotifications_.clear();
        }
    }

    void VerifyNotifications(vector<ServiceTableEntrySPtr> const & stesToVerify)
    {
        if (notificationCount_.load() > 0)
        {
            auto result = notificationEvent_.WaitOne(WaitOneTimeoutInMilliseconds);
            VERIFY(result, "VerifyNotifications timeout: {0}ms", WaitOneTimeoutInMilliseconds);
        }
        else
        {
            Sleep(1000);
        }

        map<int64, ServiceTableEntrySPtr> expectedNotifications;
        for (auto const & it : stesToVerify)
        {
            expectedNotifications[it->Version] = it;
        }

        map<int64, ServiceTableEntrySPtr> actualNotifications;
        {
            AcquireReadLock lock(lastNotificationsLock_);

            for (auto & partition : lastNotifications_)
            {
                auto findIt = actualNotifications.find(partition->Version);

                if (findIt != actualNotifications.end())
                {
                    VERIFY(
                        findIt == actualNotifications.end(),
                        "{0}: Duplicate notification: existing={1} duplicate={2}",
                        clientId_,
                        *findIt->second,
                        *partition);
                }

                actualNotifications[partition->Version] = partition;
            }
        }

        VERIFY(
            expectedNotifications.size() == actualNotifications.size(),
            "{0}: size mismatch: expected={1} actual={2}",
            clientId_,
            expectedNotifications.size(),
            actualNotifications.size());

        for (auto const & it : expectedNotifications)
        {
            auto findIt = actualNotifications.find(it.first);

            VERIFY(
                findIt != actualNotifications.end(),
                "{0}: Missing expected notification: {1}",
                clientId_,
                *it.second);

            VERIFY(
                *it.second == *findIt->second,
                "{0}: Notification mismatch: expected={1} actual={2}",
                clientId_,
                *it.second,
                *findIt->second);
        }

        expectedCountsSet_ = false;
    }

private:

    void OnConnected(
        ClientConnectionManager::HandlerId handlerId,
        ISendTarget::SPtr const & target,
        GatewayDescription const & gateway)
    {
        Trace.WriteInfo(
            TraceComponent, 
            clientId_,
            "OnConnected({0}, {1}, {2})",
            handlerId,
            target->Id(),
            gateway);

        currentTarget_ = target;
        currentGateway_ = gateway;

        connectionEvent_.Set();
    }

    void OnDisconnected(
        ClientConnectionManager::HandlerId handlerId,
        ISendTarget::SPtr const & target,
        GatewayDescription const & gateway,
        ErrorCode const & error)
    {
        Trace.WriteInfo(
            TraceComponent, 
            clientId_,
            "OnDisconnected({0}, {1}, {2}, {3})",
            handlerId,
            target->Id(),
            gateway,
            error);

        currentTarget_.reset();
        currentGateway_.Clear();

        disconnectionEvent_.Set();
    }

    ErrorCode OnNotificationReceived(vector<ServiceNotificationResultSPtr> const & notificationResults)
    {
        {
            AcquireWriteLock lock(lastNotificationsLock_);

            for (auto const & notification : notificationResults)
            {
                lastNotifications_.push_back(make_shared<ServiceTableEntry>(notification->Partition));
            }
        }

        auto count = --notificationCount_;

        Trace.WriteInfo(
            TraceComponent, 
            clientId_,
            "OnNotificationReceived(remaining={0})",
            count);

        if (count == 0)
        {
            notificationEvent_.Set();
        }

        return ErrorCodeValue::Success;
    }

    wstring clientId_;
    unique_ptr<ClientConnectionManager> clientConnection_;
    MockClientCache clientCache_;
    unique_ptr<ServiceNotificationClient> innerClient_;

    ManualResetEvent notificationEvent_;
    Common::atomic_long notificationCount_;
    bool expectedCountsSet_;
    vector<ServiceTableEntrySPtr> lastNotifications_;
    RwLock lastNotificationsLock_;

    ClientConnectionManager::HandlerId connectionHandlersId_;
    ManualResetEvent connectionEvent_;
    ManualResetEvent disconnectionEvent_;
    ISendTarget::SPtr currentTarget_;
    GatewayDescription currentGateway_;
    INamingMessageProcessorSPtr namingMessageProcessor_;
};

class UpdateGenerator
{
public:
    UpdateGenerator() 
        : updates_()
        , matchesByClientName_() 
        , nextVersion_(0)
        , iteration_(0)
        , versions_()
        , statefulServiceNames_()
        , primaryLocationsByName_()
        , secondaryLocationPrefixesByName_()
        , defaultEndpoint_()
    { }

    UpdateGenerator(wstring const & defaultEndpoint) 
        : updates_()
        , matchesByClientName_() 
        , nextVersion_(0)
        , iteration_(0)
        , versions_()
        , statefulServiceNames_()
        , primaryLocationsByName_()
        , secondaryLocationPrefixesByName_()
        , defaultEndpoint_(defaultEndpoint)
    { }


    void SetPrimaryLocation(wstring const & name, wstring const & location)
    {
        this->SetIsStateful(name);

        primaryLocationsByName_[name] = location;
    }
    
    void SetSecondaryLocationPrefix(wstring const & name, wstring const & location)
    {
        secondaryLocationPrefixesByName_[name] = location;
    }

    int64 AddUpdateMatch(wstring const & name, shared_ptr<MockNotificationClient> const & client)
    {
        vector<shared_ptr<MockNotificationClient>> clients;
        clients.push_back(client);
        return this->AddUpdateMatch(name, clients);
    }

    int64 AddUpdateMatch(wstring const & name, vector<shared_ptr<MockNotificationClient>> const & clients)
    {
        auto nextVersion = nextVersion_;
        
        this->AddUpdateMatch(
            this->CreateServiceTableEntry(wformatString("fabric:/{0}", name)),
            clients);

        return nextVersion;
    }

    void AddUpdateMatch(
        wstring const & name, 
        shared_ptr<MockNotificationClient> const & client,
        int64 version)
    {
        vector<shared_ptr<MockNotificationClient>> clients;
        clients.push_back(client);
        this->AddUpdateMatch(this->CreateServiceTableEntry(wformatString("fabric:/{0}", name), version), clients);
    }

    void AddUpdateMatchForMissedDelete(
        wstring const & name, 
        shared_ptr<MockNotificationClient> const & client,
        int64 versionToMatch)
    {
        vector<shared_ptr<MockNotificationClient>> clients;
        clients.push_back(client);
        this->AddUpdateMatchForMissedDelete(
            name, 
            clients, 
            versionToMatch);
    }

    void AddUpdateMatchForMissedDelete(
        wstring const & name, 
        vector<shared_ptr<MockNotificationClient>> const & clients,
        int64 versionToMatch)
    {
        this->AddUpdateMatch(
            this->CreateServiceTableEntry(wformatString("fabric:/{0}", name)),
            clients,
            versionToMatch);
    }

    void AddUpdateNoMatch(wstring const & name)
    {
        this->AddUpdateNoMatch(name, -1);
    }
    
    void AddUpdateNoMatch(wstring const & name, int64 version)
    {
        this->AddUpdateNoMatch(this->CreateServiceTableEntry(wformatString("fabric:/{0}", name), version));
    }

    void AddInvalid(wstring const & name)
    {
        this->AddUpdateNoMatch(this->CreateServiceTableEntry(name));
    }

    void DispatchUpdatesAndVerify(
        vector<shared_ptr<MockNotificationGateway>> const & gateways,
        vector<shared_ptr<MockNotificationClient>> const & clients)
    {
        this->SetExpectedNotificationCounts(clients);

        for (auto const & it : gateways)
        {
            this->DispatchUpdates(it);
        }

        this->VerifyClientNotifications(clients);

        for (auto const & it : gateways)
        {
            it->VerifyVersions(versions_);
        }

        this->Clear();
    }

    void DispatchUpdatesAndVerify(
        shared_ptr<MockNotificationGateway> const & gateway,
        shared_ptr<MockNotificationClient> const & client)
    {
        vector<shared_ptr<MockNotificationClient>> clients;
        clients.push_back(client);

        this->DispatchUpdatesAndVerify(gateway, clients);
    }

    void DispatchUpdatesAndVerify(
        shared_ptr<MockNotificationGateway> const & gateway,
        vector<shared_ptr<MockNotificationClient>> const & clients)
    {
        this->SetExpectedNotificationCounts(clients);

        this->DispatchUpdates(gateway);

        this->Verify(gateway, clients);
    }

    void DispatchUpdatesNoVerify(
        shared_ptr<MockNotificationGateway> const & gateway)
    {
        this->DispatchUpdates(gateway);

        this->Clear();
    }

    void DispatchUpdatesNoClear(
        shared_ptr<MockNotificationGateway> const & gateway)
    {
        this->DispatchUpdates(gateway);
    }

    void DispatchUpdatesReOpenAndVerify(
        shared_ptr<MockNotificationGateway> const & gateway,
        shared_ptr<MockNotificationClient> const & client)
    {
        vector<shared_ptr<MockNotificationClient>> clients;
        clients.push_back(client);

        this->DispatchUpdatesReOpenAndVerify(gateway, clients);
    }

    void DispatchUpdatesReOpenAndVerify(
        shared_ptr<MockNotificationGateway> const & gateway,
        vector<shared_ptr<MockNotificationClient>> const & clients)
    {
        this->SetExpectedNotificationCounts(clients);

        this->DispatchUpdatesNoClear(gateway);

        gateway->ReOpen();

        this->Verify(gateway, clients);
    }

    void DispatchUpdatesReOpenWithDelayAndVerify(
        shared_ptr<MockNotificationGateway> const & gateway,
        shared_ptr<MockNotificationClient> const & client)
    {
        vector<shared_ptr<MockNotificationClient>> clients;
        clients.push_back(client);

        this->DispatchUpdatesReOpenWithDelayAndVerify(gateway, clients);
    }

    void DispatchUpdatesReOpenWithDelayAndVerify(
        shared_ptr<MockNotificationGateway> const & gateway,
        vector<shared_ptr<MockNotificationClient>> const & clients)
    {
        this->SetExpectedNotificationCounts(clients);

        this->DispatchUpdatesNoClear(gateway);

        gateway->ReOpenWithDelayedNotifications();

        this->Verify(gateway, clients);
    }

    void Verify(
        shared_ptr<MockNotificationGateway> const & gateway,
        shared_ptr<MockNotificationClient> const & client)
    {
        vector<shared_ptr<MockNotificationClient>> clients;
        clients.push_back(client);

        this->Verify(gateway, clients);
    }

    void Verify(
        shared_ptr<MockNotificationGateway> const & gateway,
        vector<shared_ptr<MockNotificationClient>> const & clients)
    {
        this->VerifyClientNotifications(clients);

        gateway->VerifyVersions(versions_);

        this->Clear();
    }

    void VerifyClientNotifications(vector<shared_ptr<MockNotificationClient>> const & clients)
    {
        Trace.WriteInfo(
            TraceComponent, 
            "Verifying: iteration={0}",
            iteration_);

        for (auto const & client : clients)
        {
            client->VerifyNotifications(*(this->GetMatchingUpdates(client->GetClientId())));
        }
    }

    vector<ServiceTableEntrySPtr> const & GetAllUpdates() const
    {
        return updates_;
    }

    shared_ptr<vector<ServiceTableEntrySPtr>> GetMatchingUpdates(wstring const & clientName) const
    {
        auto findIt = matchesByClientName_.find(clientName);
        if (findIt != matchesByClientName_.end())
        {
            return findIt->second;
        }

        return make_shared<vector<ServiceTableEntrySPtr>>();
    }

    ConsistencyUnitId GetCuid(wstring const & name)
    {
        auto findIt = cuidsByName_.find(name);
        if (findIt != cuidsByName_.end())
        {
            return findIt->second;
        }
        else
        {
            ConsistencyUnitId cuid;
            cuidsByName_.insert(make_pair(name, cuid));
            return cuid;
        }
    }

    ConsistencyUnitId SetNewCuid(wstring const & name)
    {
        ConsistencyUnitId cuid;
        this->SetCuid(name, cuid);

        return cuid;
    }

    void SetCuid(wstring const & name, ConsistencyUnitId const & cuid)
    {
        auto findIt = cuidsByName_.find(name);
        if (findIt != cuidsByName_.end())
        {
            cuidsByName_[name] = cuid;
        }
        else
        {
            cuidsByName_.insert(make_pair(name, cuid));
        }
    }

private:
    void SetExpectedNotificationCounts(vector<shared_ptr<MockNotificationClient>> const & clients)
    {
        for (auto const & client : clients)
        {
            if (!this->GetMatchingUpdates(client->GetClientId())->empty())
            {
                client->SetExpectedNotificationCount(1);
            }
            else
            {
                client->SetExpectedNotificationCount(0);
            }
        }
    }

    void DispatchUpdates(
        shared_ptr<MockNotificationGateway> const & gateway)
    {
        Trace.WriteInfo(
            TraceComponent, 
            "DispatchUpdates: iteration={0}",
            ++iteration_);

        gateway->GetMockCache().UpdateCacheEntries(this->GetAllUpdates());
    }

    void Clear()
    {
        updates_.clear();
        matchesByClientName_.clear();
    }

    void AddUpdateMatch(
        ServiceTableEntrySPtr const & ste, 
        vector<shared_ptr<MockNotificationClient>> const & clients,
        int64 versionToMatch = -1)
    {
        updates_.push_back(ste);

        for (auto const & client : clients)
        {
            auto const & clientName = client->GetClientId();

            shared_ptr<vector<ServiceTableEntrySPtr>> matches;

            auto findIt = matchesByClientName_.find(clientName);
            if (findIt == matchesByClientName_.end())
            {
                matches = make_shared<vector<ServiceTableEntrySPtr>>();
                matchesByClientName_[clientName] = matches;
            }
            else
            {
                matches = findIt->second;
            }

            auto steToMatch = ste;

            if (versionToMatch >= 0)
            {
                // Deleted empty partition synchronization protocol will not know
                // details about the deleted partition. Only the CUID and
                // service name will be known.
                //
                steToMatch = make_shared<ServiceTableEntry>(
                    ste->ConsistencyUnitId,
                    ste->ServiceName,
                    ServiceReplicaSet(
                        false, // isStateful
                        false, // isPrimaryLocationValid
                        L"", // primaryLocation
                        vector<wstring>(), // locations
                        versionToMatch));
            }

            matches->push_back(steToMatch);
        }
    }

    void AddUpdateNoMatch(ServiceTableEntrySPtr const & ste)
    {
        updates_.push_back(ste);
    }

    ServiceTableEntrySPtr CreateServiceTableEntry(wstring const & uri, int64 version = -1)
    {
        if (version < 0)
        {
            version = nextVersion_++;
        }

        auto name = uri;
        StringUtility::Replace<wstring>(name, L"fabric:/", L"");

        auto isStateful = this->GetIsStateful(name);
        auto primaryLocation = this->GetPrimaryLocation(name);
        auto secondaryPrefix = this->GetSecondaryLocationPrefix(name);
        auto secondaryLocation = secondaryPrefix.empty() ? L"" : wformatString(
            "{0}_{1}", 
            secondaryPrefix,
            version);

        vector<wstring> locations;
        if (isStateful && !secondaryLocation.empty())
        {
            locations.push_back(secondaryLocation);
        }

        if(primaryLocation.empty() && locations.empty() && !defaultEndpoint_.empty())
        {
            locations.push_back(defaultEndpoint_);
        }

        ServiceReplicaSet replicaSet(
            isStateful,
            !primaryLocation.empty(), // is primary location valid
            move(primaryLocation),
            move(locations),
            version);

        versions_.Add(VersionRange(version, version + 1));

        return make_shared<ServiceTableEntry>(
            this->GetCuid(name),
            uri,
            move(replicaSet));
    }

    bool GetIsStateful(wstring const & name)
    {
        return (statefulServiceNames_.find(name) != statefulServiceNames_.end());
    }

    void SetIsStateful(wstring const & name)
    {
        statefulServiceNames_.insert(name);
    }

    wstring GetPrimaryLocation(wstring const & name)
    {
        auto findIt = primaryLocationsByName_.find(name);
        if (findIt != primaryLocationsByName_.end())
        {
            return findIt->second;
        }

        return L"";
    }

    wstring GetSecondaryLocationPrefix(wstring const & name)
    {
        auto findIt = secondaryLocationPrefixesByName_.find(name);
        if (findIt != secondaryLocationPrefixesByName_.end())
        {
            return findIt->second;
        }

        return L"";
    }

    vector<ServiceTableEntrySPtr> updates_;
    map<wstring, shared_ptr<vector<ServiceTableEntrySPtr>>> matchesByClientName_;
    int64 nextVersion_;
    int iteration_;
    VersionRangeCollection versions_;

    set<wstring> statefulServiceNames_;
    map<wstring, wstring> primaryLocationsByName_;
    map<wstring, wstring> secondaryLocationPrefixesByName_;
    map<wstring, ConsistencyUnitId> cuidsByName_;

    wstring defaultEndpoint_;
};

//
// Helper Functions
//

BOOST_FIXTURE_TEST_SUITE(ServiceNotificationManagerTestSuite,ServiceNotificationManagerTest)

BOOST_AUTO_TEST_CASE(ExactFiltersBasicTest)
{
    auto gateway = make_shared<MockNotificationGateway>(11);
    gateway->Open();

    auto client = make_shared<MockNotificationClient>(L"ExactFiltersBasicTest-Client", *gateway);
    client->Open();

    UpdateGenerator generator(wformatString("default-endpoint-{0}", DateTime::Now().Ticks));

    Trace.WriteInfo(TraceComponent, "Testcase: Basic exact match. Single filter.");

    client->RegisterExactFilter(L"a");

    generator.AddUpdateMatch(L"a", client);
    generator.AddUpdateNoMatch(L"b");
    generator.DispatchUpdatesAndVerify(gateway, client);

    Trace.WriteInfo(TraceComponent, "Testcase: Basic exact match. Two filters.");

    client->RegisterExactFilter(L"b");

    generator.AddUpdateMatch(L"a", client);
    generator.AddUpdateMatch(L"b", client);
    generator.DispatchUpdatesAndVerify(gateway, client);

    generator.AddUpdateMatch(L"a", client);
    generator.AddUpdateMatch(L"b", client);
    generator.AddUpdateNoMatch(L"a/b");
    generator.AddUpdateNoMatch(L"b/a");
    generator.DispatchUpdatesAndVerify(gateway, client);

    generator.AddUpdateMatch(L"a", client);
    generator.AddUpdateNoMatch(L"aa");
    generator.AddUpdateNoMatch(L" a");
    generator.DispatchUpdatesAndVerify(gateway, client);

    Trace.WriteInfo(TraceComponent, "Testcase: Basic exact match. Multiple filters. Multiple segments.");

    client->RegisterExactFilter(L"b/c");

    generator.AddUpdateMatch(L"b/c", client);
    generator.AddUpdateNoMatch(L"b/c/d");
    generator.DispatchUpdatesAndVerify(gateway, client);

    client->RegisterExactFilter(L"b/c/d");

    generator.AddUpdateMatch(L"b/c", client);
    generator.AddUpdateMatch(L"b/c/d", client);
    generator.DispatchUpdatesAndVerify(gateway, client);

    Trace.WriteInfo(TraceComponent, "Testcase: Reconnecting client should get updates retro-actively");

    gateway->Close();
    client->WaitForDisconnection();

    generator.AddUpdateMatch(L"a", client);
    generator.AddUpdateMatch(L"b/c", client);
    generator.AddUpdateMatch(L"b/c/d", client);
    generator.AddUpdateNoMatch(L"a/e");
    generator.AddUpdateNoMatch(L"b/e");
    generator.AddUpdateNoMatch(L"a/b");
    generator.DispatchUpdatesReOpenAndVerify(gateway, client);
    
    generator.AddUpdateMatch(L"a", client);
    generator.AddUpdateMatch(L"b/c", client);
    generator.AddUpdateMatch(L"b/c/d", client);
    generator.AddUpdateNoMatch(L"a/e");
    generator.AddUpdateNoMatch(L"b/e");
    generator.AddUpdateNoMatch(L"a/b");
    generator.DispatchUpdatesAndVerify(gateway, client);
    
    Trace.WriteInfo(TraceComponent, "Testcase: Reconnecting client should re-register all filters");

    gateway->Close();
    client->WaitForDisconnection();

    Sleep(ReOpenDelayInMilliseconds);

    generator.AddUpdateMatch(L"a", client);
    generator.AddUpdateMatch(L"b", client);
    generator.AddUpdateNoMatch(L"c");
    generator.AddUpdateNoMatch(L"d");
    generator.DispatchUpdatesReOpenAndVerify(gateway, client);
    
    client->Close();

    Trace.WriteInfo(TraceComponent, "Testcase: Gateway can still process updates with no clients connected");

    generator.AddUpdateMatch(L"a", client);
    generator.AddUpdateMatch(L"b", client);
    generator.AddUpdateMatch(L"b/c", client);
    generator.AddUpdateMatch(L"b/c/d", client);
    generator.DispatchUpdatesNoVerify(gateway);

    Sleep(ReOpenDelayInMilliseconds);

    gateway->Close();
}

BOOST_AUTO_TEST_CASE(PrefixFiltersBasicTest)
{
    auto gateway = make_shared<MockNotificationGateway>(22);
    gateway->Open();

    auto client = make_shared<MockNotificationClient>(L"PrefixFiltersBasicTest-Client", *gateway);
    client->Open();

    UpdateGenerator generator(wformatString("default-endpoint-{0}", DateTime::Now().Ticks));

    Trace.WriteInfo(TraceComponent, "Testcase: Basic prefix match. Single filter.");

    client->RegisterPrefixFilter(L"a");

    generator.AddUpdateMatch(L"a", client);
    generator.AddUpdateMatch(L"a/b", client);
    generator.AddUpdateNoMatch(L"aa");
    generator.AddUpdateNoMatch(L" a");
    generator.AddUpdateNoMatch(L"b");
    generator.DispatchUpdatesAndVerify(gateway, client);

    Trace.WriteInfo(TraceComponent, "Testcase: Basic prefix match. Multiple filters. Multiple segments.");

    client->RegisterPrefixFilter(L"b/c");

    generator.AddUpdateNoMatch(L"b");
    generator.AddUpdateNoMatch(L"bc");
    generator.AddUpdateNoMatch(L"b.c");
    generator.AddUpdateNoMatch(L"bb/c");
    generator.AddUpdateNoMatch(L" b/c");
    generator.AddUpdateMatch(L"b/c", client);
    generator.AddUpdateMatch(L"b/c/d", client);
    generator.DispatchUpdatesAndVerify(gateway, client);

    generator.AddUpdateMatch(L"a", client);
    generator.AddUpdateMatch(L"a/b", client);
    generator.AddUpdateMatch(L"a/b/c", client);
    generator.AddUpdateMatch(L"a/b/c/d", client);
    generator.DispatchUpdatesAndVerify(gateway, client);

    generator.AddUpdateMatch(L"a/b", client);
    generator.AddUpdateMatch(L"b/c/d", client);
    generator.AddUpdateNoMatch(L"ab");
    generator.DispatchUpdatesAndVerify(gateway, client);

    client->RegisterPrefixFilter(L"b/c/d");

    generator.AddUpdateNoMatch(L"b");
    generator.AddUpdateMatch(L"b/c", client);
    generator.AddUpdateMatch(L"b/c/d", client);
    generator.DispatchUpdatesAndVerify(gateway, client);

    Trace.WriteInfo(TraceComponent, "Testcase: Reconnecting client should get updates retro-actively");

    gateway->Close();
    client->WaitForDisconnection();

    generator.AddUpdateMatch(L"a", client);
    generator.AddUpdateMatch(L"b/c", client);
    generator.AddUpdateMatch(L"b/c/d", client);
    generator.AddUpdateNoMatch(L"b/c.d");
    generator.AddUpdateMatch(L"a/e", client);
    generator.AddUpdateMatch(L"a/b", client);
    generator.AddUpdateNoMatch(L"b/a");
    generator.AddUpdateNoMatch(L"b/e");
    generator.DispatchUpdatesReOpenAndVerify(gateway, client);
    
    generator.AddUpdateMatch(L"a", client);
    generator.AddUpdateMatch(L"b/c", client);
    generator.AddUpdateMatch(L"b/c/d", client);
    generator.AddUpdateNoMatch(L"b/c.d");
    generator.AddUpdateMatch(L"a/e", client);
    generator.AddUpdateMatch(L"a/b", client);
    generator.AddUpdateNoMatch(L"b/a");
    generator.AddUpdateNoMatch(L"b/e");
    generator.DispatchUpdatesAndVerify(gateway, client);
    
    Trace.WriteInfo(TraceComponent, "Testcase: Reconnecting client should re-register all filters");

    gateway->CloseAndReOpen();

    Sleep(ReOpenDelayInMilliseconds);

    generator.AddUpdateMatch(L"a/e", client);
    generator.AddUpdateMatch(L"b/c/d", client);
    generator.AddUpdateNoMatch(L"b");
    generator.AddUpdateNoMatch(L"c");
    generator.AddUpdateNoMatch(L"d");
    generator.DispatchUpdatesAndVerify(gateway, client);
    
    Trace.WriteInfo(TraceComponent, "Testcase: Gateway can still process updates with no clients connected");

    client->Close();

    generator.AddUpdateMatch(L"a", client);
    generator.AddUpdateMatch(L"b", client);
    generator.AddUpdateMatch(L"b/c", client);
    generator.AddUpdateMatch(L"b/c/d", client);
    generator.DispatchUpdatesNoVerify(gateway);

    Sleep(ReOpenDelayInMilliseconds);

    gateway->Close();
}

BOOST_AUTO_TEST_CASE(MixedFiltersBasicTest)
{
    auto gateway = make_shared<MockNotificationGateway>(33);
    gateway->Open();

    auto client = make_shared<MockNotificationClient>(L"MixedFiltersBasicTest-Client", *gateway);
    client->Open();

    UpdateGenerator generator(wformatString("default-endpoint-{0}", DateTime::Now().Ticks));

    Trace.WriteInfo(TraceComponent, "Testcase: Updates against filters");

    client->RegisterExactFilter(L"a");
    client->RegisterPrefixFilter(L"b");

    generator.AddUpdateMatch(L"a", client);
    generator.AddUpdateNoMatch(L"a/b");
    generator.AddUpdateMatch(L"b", client);
    generator.AddUpdateMatch(L"b/c", client);
    generator.AddUpdateNoMatch(L"c");
    generator.DispatchUpdatesAndVerify(gateway, client);

    client->RegisterExactFilter(L"a/b");
    client->RegisterPrefixFilter(L"a/b/c");

    generator.AddUpdateMatch(L"a", client);
    generator.AddUpdateMatch(L"a/b", client);
    generator.AddUpdateMatch(L"a/b/c", client);
    generator.AddUpdateMatch(L"a/b/c/d", client);
    generator.AddUpdateNoMatch(L"a/b/d");
    generator.AddUpdateNoMatch(L"a/c");
    generator.AddUpdateNoMatch(L"a/b/c.d");
    generator.AddUpdateMatch(L"a/b/c/f", client);
    generator.AddUpdateMatch(L"b", client);
    generator.AddUpdateMatch(L"b/c", client);
    generator.AddUpdateMatch(L"b/c/d", client);
    generator.AddUpdateNoMatch(L"c");
    generator.DispatchUpdatesAndVerify(gateway, client);

    Trace.WriteInfo(TraceComponent, "Testcase: Reconnections");

    gateway->Close();
    client->WaitForDisconnection();

    generator.AddUpdateMatch(L"a", client);
    generator.AddUpdateMatch(L"a/b", client);
    generator.AddUpdateMatch(L"a/b/c", client);
    generator.AddUpdateMatch(L"a/b/c/e", client);
    generator.AddUpdateNoMatch(L"a/c");
    generator.AddUpdateNoMatch(L"a/b/d/e");
    generator.AddUpdateMatch(L"b", client);
    generator.AddUpdateMatch(L"b/a", client);
    generator.AddUpdateNoMatch(L"c");
    generator.AddUpdateNoMatch(L"d");
    generator.DispatchUpdatesReOpenAndVerify(gateway, client);

    generator.AddUpdateMatch(L"a", client);
    generator.AddUpdateMatch(L"a/b", client);
    generator.AddUpdateMatch(L"a/b/c", client);
    generator.AddUpdateMatch(L"a/b/c/e", client);
    generator.AddUpdateNoMatch(L"a/c");
    generator.AddUpdateNoMatch(L"a/b/d/e");
    generator.AddUpdateMatch(L"b", client);
    generator.AddUpdateMatch(L"b/a", client);
    generator.AddUpdateNoMatch(L"c");
    generator.AddUpdateNoMatch(L"d");
    generator.DispatchUpdatesAndVerify(gateway, client);
    
    client->Close();
    gateway->Close();
}

BOOST_AUTO_TEST_CASE(RootUriTest)
{
    auto gateway = make_shared<MockNotificationGateway>(44);
    gateway->Open();

    auto client = make_shared<MockNotificationClient>(L"RootUriTest-Client", *gateway);
    client->Open();

    UpdateGenerator generator(wformatString("default-endpoint-{0}", DateTime::Now().Ticks));

    client->RegisterPrefixFilter(L"");

    generator.AddInvalid(L"SystemService");
    generator.AddInvalid(L"b.c");
    generator.AddUpdateMatch(L"a", client);
    generator.AddUpdateMatch(L"b", client);
    generator.AddUpdateMatch(L"b/c", client);
    generator.AddUpdateMatch(L"c/d/e", client);
    generator.AddUpdateMatch(L" a", client);
    generator.DispatchUpdatesAndVerify(gateway, client);

    gateway->Close();
    client->WaitForDisconnection();

    generator.AddInvalid(L"SystemService");
    generator.AddInvalid(L"b.c");
    generator.AddUpdateMatch(L"a", client);
    generator.AddUpdateMatch(L"b", client);
    generator.AddUpdateMatch(L"b/c", client);
    generator.AddUpdateMatch(L"c/d/e", client);
    generator.AddUpdateMatch(L" a", client);
    generator.DispatchUpdatesReOpenAndVerify(gateway, client);

    generator.AddInvalid(L"SystemService");
    generator.AddInvalid(L"b.c");
    generator.AddUpdateMatch(L"a", client);
    generator.AddUpdateMatch(L"b", client);
    generator.AddUpdateMatch(L"b/c", client);
    generator.AddUpdateMatch(L"c/d/e", client);
    generator.AddUpdateMatch(L" a", client);
    generator.DispatchUpdatesAndVerify(gateway, client);

    gateway->Close();
    client->Close();
}

BOOST_AUTO_TEST_CASE(UnregisterFiltersTest)
{
    auto gateway = make_shared<MockNotificationGateway>(45);
    gateway->Open();

    auto client1 = make_shared<MockNotificationClient>(L"UnregisterFiltersTest-Client1", *gateway);
    client1->Open();

    UpdateGenerator generator(wformatString("default-endpoint-{0}", DateTime::Now().Ticks));

    client1->RegisterExactFilter(L"a1");
    client1->RegisterPrefixFilter(L"b1");

    generator.AddUpdateMatch(L"a1", client1);
    generator.AddUpdateMatch(L"b1/x", client1);
    generator.DispatchUpdatesAndVerify(gateway, client1);

    auto client2 = make_shared<MockNotificationClient>(L"UnregisterFiltersTest-Client2", *gateway);
    client2->Open();

    vector<shared_ptr<MockNotificationClient>> allClients;
    allClients.push_back(client1);
    allClients.push_back(client2);

    client2->RegisterExactFilter(L"a2");
    client2->RegisterPrefixFilter(L"b2");
    client2->RegisterExactFilter(L"x");
    client2->RegisterPrefixFilter(L"y");
    
    client1->RegisterExactFilter(L"x");
    client1->RegisterPrefixFilter(L"y");

    generator.AddUpdateMatch(L"a1", client1);
    generator.AddUpdateMatch(L"b1/x", client1);
    generator.AddUpdateMatch(L"a2", client2);
    generator.AddUpdateMatch(L"b2/x", client2);
    generator.AddUpdateMatch(L"x", allClients);
    generator.AddUpdateMatch(L"y/z", allClients);
    generator.DispatchUpdatesAndVerify(gateway, allClients);

    client1->UnregisterFilter(L"a1");

    generator.AddUpdateNoMatch(L"a1");
    generator.AddUpdateMatch(L"b1/x", client1);
    generator.AddUpdateMatch(L"a2", client2);
    generator.AddUpdateMatch(L"b2/x", client2);
    generator.AddUpdateMatch(L"x", allClients);
    generator.AddUpdateMatch(L"y/z", allClients);
    generator.DispatchUpdatesAndVerify(gateway, allClients);

    client2->UnregisterFilter(L"a2");

    generator.AddUpdateNoMatch(L"a1");
    generator.AddUpdateMatch(L"b1/x", client1);
    generator.AddUpdateNoMatch(L"a2");
    generator.AddUpdateMatch(L"b2/x", client2);
    generator.AddUpdateMatch(L"x", allClients);
    generator.AddUpdateMatch(L"y/z", allClients);
    generator.DispatchUpdatesAndVerify(gateway, allClients);

    client1->UnregisterFilter(L"b1");

    generator.AddUpdateNoMatch(L"a1");
    generator.AddUpdateNoMatch(L"b1/x");
    generator.AddUpdateNoMatch(L"a2");
    generator.AddUpdateMatch(L"b2/x", client2);
    generator.AddUpdateMatch(L"x", allClients);
    generator.AddUpdateMatch(L"y/z", allClients);
    generator.DispatchUpdatesAndVerify(gateway, allClients);

    client2->UnregisterFilter(L"b2");

    generator.AddUpdateNoMatch(L"a1");
    generator.AddUpdateNoMatch(L"b1/x");
    generator.AddUpdateNoMatch(L"a2");
    generator.AddUpdateNoMatch(L"b2/x");
    generator.AddUpdateMatch(L"x", allClients);
    generator.AddUpdateMatch(L"y/z", allClients);
    generator.DispatchUpdatesAndVerify(gateway, allClients);

    client1->UnregisterFilter(L"x");

    generator.AddUpdateNoMatch(L"a1");
    generator.AddUpdateNoMatch(L"b1/x");
    generator.AddUpdateNoMatch(L"a2");
    generator.AddUpdateNoMatch(L"b2/x");
    generator.AddUpdateMatch(L"x", client2);
    generator.AddUpdateMatch(L"y/z", allClients);
    generator.DispatchUpdatesAndVerify(gateway, allClients);

    client2->UnregisterFilter(L"y");

    generator.AddUpdateNoMatch(L"a1");
    generator.AddUpdateNoMatch(L"b1/x");
    generator.AddUpdateNoMatch(L"a2");
    generator.AddUpdateNoMatch(L"b2/x");
    generator.AddUpdateMatch(L"x", client2);
    generator.AddUpdateMatch(L"y/z", client1);
    generator.DispatchUpdatesAndVerify(gateway, allClients);

    client1->UnregisterFilter(L"y");

    generator.AddUpdateNoMatch(L"a1");
    generator.AddUpdateNoMatch(L"b1/x");
    generator.AddUpdateNoMatch(L"a2");
    generator.AddUpdateNoMatch(L"b2/x");
    generator.AddUpdateMatch(L"x", client2);
    generator.AddUpdateNoMatch(L"y/z");
    generator.DispatchUpdatesAndVerify(gateway, allClients);

    client2->UnregisterFilter(L"x");

    generator.AddUpdateNoMatch(L"a1");
    generator.AddUpdateNoMatch(L"b1/x");
    generator.AddUpdateNoMatch(L"a2");
    generator.AddUpdateNoMatch(L"b2/x");
    generator.AddUpdateNoMatch(L"x");
    generator.AddUpdateNoMatch(L"y/z");
    generator.DispatchUpdatesAndVerify(gateway, allClients);

    client1->Close();
    gateway->Close();
    client2->Close();
}

BOOST_AUTO_TEST_CASE(MultiClientTest)
{
    auto gateway = make_shared<MockNotificationGateway>(55);
    gateway->Open();

    auto client1 = make_shared<MockNotificationClient>(L"MultiClientTest-Client1", *gateway);
    client1->Open();

    auto client2 = make_shared<MockNotificationClient>(L"MultiClientTest-Client2", *gateway);
    client2->Open();

    vector<shared_ptr<MockNotificationClient>> allClients;
    allClients.push_back(client1);
    allClients.push_back(client2);

    UpdateGenerator generator(wformatString("default-endpoint-{0}", DateTime::Now().Ticks));

    client1->RegisterExactFilter(L"c1");
    client1->RegisterPrefixFilter(L"c1/a");

    client2->RegisterExactFilter(L"c2");
    client2->RegisterPrefixFilter(L"c2/b");

    generator.AddUpdateMatch(L"c1", client1);
    generator.AddUpdateMatch(L"c2", client2);
    generator.AddUpdateNoMatch(L"c");
    generator.AddUpdateMatch(L"c1/a", client1);
    generator.AddUpdateMatch(L"c1/a/c", client1);
    generator.AddUpdateNoMatch(L"c1/c");
    generator.AddUpdateMatch(L"c2/b", client2);
    generator.AddUpdateMatch(L"c2/b/c", client2);
    generator.AddUpdateNoMatch(L"c2/c");
    generator.DispatchUpdatesAndVerify(gateway, allClients);

    client1->RegisterExactFilter(L"cx");
    client1->RegisterPrefixFilter(L"cy/y");
    client1->RegisterPrefixFilter(L"cy/a");

    client2->RegisterExactFilter(L"cx");
    client2->RegisterPrefixFilter(L"cy/y");
    client2->RegisterPrefixFilter(L"cy/b");

    generator.AddUpdateMatch(L"c1", client1);
    generator.AddUpdateMatch(L"c2", client2);
    generator.AddUpdateNoMatch(L"c");
    generator.AddUpdateMatch(L"c1/a", client1);
    generator.AddUpdateMatch(L"c1/a/c", client1);
    generator.AddUpdateNoMatch(L"c1/c");
    generator.AddUpdateMatch(L"c2/b", client2);
    generator.AddUpdateMatch(L"c2/b/c", client2);
    generator.AddUpdateNoMatch(L"c2/c");

    generator.AddUpdateMatch(L"cx", allClients);
    generator.AddUpdateNoMatch(L"cx/a");
    generator.AddUpdateNoMatch(L"cx/b");
    generator.AddUpdateNoMatch(L"cy");
    generator.AddUpdateMatch(L"cy/y", allClients);
    generator.AddUpdateMatch(L"cy/y/a", allClients);
    generator.AddUpdateMatch(L"cy/y/b", allClients);
    generator.AddUpdateMatch(L"cy/a", client1);
    generator.AddUpdateMatch(L"cy/b", client2);
    generator.AddUpdateNoMatch(L"cy/x");
    generator.DispatchUpdatesAndVerify(gateway, allClients);

    auto client3 = make_shared<MockNotificationClient>(L"MultiClientTest-Client3", *gateway);
    client3->Open();

    allClients.push_back(client3);

    client3->RegisterExactFilter(L"c3");
    client3->RegisterPrefixFilter(L"c3/c");
    client3->RegisterExactFilter(L"cx");
    client3->RegisterPrefixFilter(L"cy/y");
    client3->RegisterPrefixFilter(L"cy/c");

    gateway->Close();
    MockNotificationClient::WaitForDisconnection(allClients);

    generator.AddUpdateMatch(L"c1", client1);
    generator.AddUpdateMatch(L"c2", client2);
    generator.AddUpdateMatch(L"c3", client3);
    generator.AddUpdateMatch(L"cx", allClients);
    generator.AddUpdateMatch(L"c1/a/d", client1);
    generator.AddUpdateMatch(L"c2/b/d", client2);
    generator.AddUpdateMatch(L"c3/c/d", client3);
    generator.AddUpdateMatch(L"cy/a/d", client1);
    generator.AddUpdateMatch(L"cy/b/d", client2);
    generator.AddUpdateMatch(L"cy/c/d", client3);
    generator.AddUpdateMatch(L"cy/y/d", allClients);
    generator.AddUpdateNoMatch(L"cx/a");
    generator.AddUpdateNoMatch(L"cx/b");
    generator.AddUpdateNoMatch(L"cy");
    generator.AddUpdateNoMatch(L"cy/x");
    generator.DispatchUpdatesReOpenAndVerify(gateway, allClients);

    generator.AddUpdateMatch(L"c1", client1);
    generator.AddUpdateMatch(L"c2", client2);
    generator.AddUpdateMatch(L"c3", client3);
    generator.AddUpdateMatch(L"cx", allClients);
    generator.AddUpdateMatch(L"c1/a/d", client1);
    generator.AddUpdateMatch(L"c2/b/d", client2);
    generator.AddUpdateMatch(L"c3/c/d", client3);
    generator.AddUpdateMatch(L"cy/a/d", client1);
    generator.AddUpdateMatch(L"cy/b/d", client2);
    generator.AddUpdateMatch(L"cy/c/d", client3);
    generator.AddUpdateMatch(L"cy/y/d", allClients);
    generator.AddUpdateNoMatch(L"cx/a");
    generator.AddUpdateNoMatch(L"cx/b");
    generator.AddUpdateNoMatch(L"cy");
    generator.AddUpdateNoMatch(L"cy/x");
    generator.DispatchUpdatesAndVerify(gateway, allClients);

    vector<shared_ptr<MockNotificationClient>> clients12;
    vector<shared_ptr<MockNotificationClient>> clients13;
    vector<shared_ptr<MockNotificationClient>> clients23;

    clients12.push_back(client1);
    clients12.push_back(client2);

    clients13.push_back(client1);
    clients13.push_back(client3);

    clients23.push_back(client2);
    clients23.push_back(client3);

    client1->RegisterExactFilter(L"c12");
    client1->RegisterPrefixFilter(L"c12/a");
    client1->RegisterPrefixFilter(L"c12/x");
    client1->RegisterExactFilter(L"c13");
    client1->RegisterPrefixFilter(L"c13/a");
    client1->RegisterPrefixFilter(L"c13/x");

    client2->RegisterExactFilter(L"c12");
    client2->RegisterPrefixFilter(L"c12/b");
    client2->RegisterPrefixFilter(L"c12/x");
    client2->RegisterExactFilter(L"c23");
    client2->RegisterPrefixFilter(L"c23/b");
    client2->RegisterPrefixFilter(L"c23/x");

    client3->RegisterExactFilter(L"c13");
    client3->RegisterPrefixFilter(L"c13/c");
    client3->RegisterPrefixFilter(L"c13/x");
    client3->RegisterExactFilter(L"c23");
    client3->RegisterPrefixFilter(L"c23/c");
    client3->RegisterPrefixFilter(L"c23/x");

    gateway->Close();
    MockNotificationClient::WaitForDisconnection(allClients);

    generator.AddUpdateMatch(L"c12", clients12);
    generator.AddUpdateMatch(L"c13", clients13);
    generator.AddUpdateMatch(L"c23", clients23);
    generator.AddUpdateMatch(L"c12/a/x", client1);
    generator.AddUpdateMatch(L"c12/b/x", client2);
    generator.AddUpdateMatch(L"c12/x/x", clients12);
    generator.AddUpdateMatch(L"c13/a/x", client1);
    generator.AddUpdateMatch(L"c13/c/x", client3);
    generator.AddUpdateMatch(L"c13/x/x", clients13);
    generator.AddUpdateMatch(L"c23/b/x", client2);
    generator.AddUpdateMatch(L"c23/c/x", client3);
    generator.AddUpdateMatch(L"c23/x/x", clients23);
    generator.AddUpdateNoMatch(L"c.1");
    generator.AddUpdateNoMatch(L"c.2");
    generator.AddUpdateNoMatch(L"c.3");
    generator.AddUpdateNoMatch(L"c12/.x");
    generator.AddUpdateNoMatch(L"c13/.x");
    generator.AddUpdateNoMatch(L"c23/.x");
    generator.DispatchUpdatesReOpenAndVerify(gateway, allClients);

    generator.AddUpdateMatch(L"c12", clients12);
    generator.AddUpdateMatch(L"c13", clients13);
    generator.AddUpdateMatch(L"c23", clients23);
    generator.AddUpdateMatch(L"c12/a/x", client1);
    generator.AddUpdateMatch(L"c12/b/x", client2);
    generator.AddUpdateMatch(L"c12/x/x", clients12);
    generator.AddUpdateMatch(L"c13/a/x", client1);
    generator.AddUpdateMatch(L"c13/c/x", client3);
    generator.AddUpdateMatch(L"c13/x/x", clients13);
    generator.AddUpdateMatch(L"c23/b/x", client2);
    generator.AddUpdateMatch(L"c23/c/x", client3);
    generator.AddUpdateMatch(L"c23/x/x", clients23);
    generator.AddUpdateNoMatch(L"c.1");
    generator.AddUpdateNoMatch(L"c.2");
    generator.AddUpdateNoMatch(L"c.3");
    generator.AddUpdateNoMatch(L"c12/.x");
    generator.AddUpdateNoMatch(L"c13/.x");
    generator.AddUpdateNoMatch(L"c23/.x");
    generator.DispatchUpdatesAndVerify(gateway, allClients);

    for (auto const & client : allClients)
    {
        client->Close();
    }
    gateway->Close();
}

BOOST_AUTO_TEST_CASE(PrimaryOnlyFiltersTest)
{
    auto gateway = make_shared<MockNotificationGateway>(66);
    gateway->Open();

    auto client = make_shared<MockNotificationClient>(L"PrimaryOnlyFiltersTest-Client", *gateway);
    client->Open();

    UpdateGenerator generator;

    client->RegisterPrimaryOnlyExactFilter(L"a");
    client->RegisterExactFilter(L"b");

    generator.SetPrimaryLocation(L"a", L"primary_address_0");
    generator.SetSecondaryLocationPrefix(L"a", L"secondary_address");
    
    generator.SetPrimaryLocation(L"b", L"primary_address_0");
    
    generator.AddUpdateMatch(L"a", client);
    generator.AddUpdateMatch(L"b", client);
    generator.DispatchUpdatesAndVerify(gateway, client);

    generator.SetPrimaryLocation(L"b", L"primary_address_1");

    generator.AddUpdateNoMatch(L"a");
    generator.AddUpdateMatch(L"b", client);
    generator.DispatchUpdatesAndVerify(gateway, client);

    generator.SetPrimaryLocation(L"a", L"primary_address_1");
    generator.SetPrimaryLocation(L"b", L"primary_address_2");

    generator.AddUpdateMatch(L"a", client);
    generator.AddUpdateMatch(L"b", client);
    generator.DispatchUpdatesAndVerify(gateway, client);

    generator.SetPrimaryLocation(L"b", L"primary_address_3");

    generator.AddUpdateNoMatch(L"a");
    generator.AddUpdateMatch(L"b", client);
    generator.DispatchUpdatesAndVerify(gateway, client);

    generator.SetPrimaryLocation(L"a", L"");
    generator.SetPrimaryLocation(L"b", L"primary_address_4");

    generator.AddUpdateMatch(L"a", client);
    generator.AddUpdateMatch(L"b", client);
    generator.DispatchUpdatesAndVerify(gateway, client);

    generator.SetPrimaryLocation(L"b", L"");

    generator.AddUpdateNoMatch(L"a");
    generator.AddUpdateMatch(L"b", client);
    generator.DispatchUpdatesAndVerify(gateway, client);

    client->RegisterPrimaryOnlyPrefixFilter(L"c");
    client->RegisterPrefixFilter(L"d");

    generator.SetPrimaryLocation(L"b", L"primary_address_0");
    generator.SetPrimaryLocation(L"c/x", L"primary_address_0");
    generator.SetSecondaryLocationPrefix(L"c/x", L"secondary_address");
    generator.SetPrimaryLocation(L"c/y", L"primary_address_0");
    generator.SetPrimaryLocation(L"d/x", L"primary_address_0");
    
    generator.AddUpdateNoMatch(L"a");
    generator.AddUpdateMatch(L"b", client);
    generator.AddUpdateMatch(L"c/x", client);
    generator.AddUpdateMatch(L"c/y", client);
    generator.AddUpdateMatch(L"d/x", client);
    generator.DispatchUpdatesAndVerify(gateway, client);

    generator.SetPrimaryLocation(L"b", L"primary_address_1");
    generator.SetPrimaryLocation(L"d/x", L"primary_address_1");

    generator.AddUpdateNoMatch(L"a");
    generator.AddUpdateMatch(L"b", client);
    generator.AddUpdateNoMatch(L"c/x");
    generator.AddUpdateNoMatch(L"c/y");
    generator.AddUpdateMatch(L"d/x", client);
    generator.DispatchUpdatesAndVerify(gateway, client);

    generator.SetPrimaryLocation(L"b", L"primary_address_2");
    generator.SetPrimaryLocation(L"d/x", L"primary_address_2");
    generator.SetPrimaryLocation(L"c/x", L"primary_address_1");

    generator.AddUpdateNoMatch(L"a");
    generator.AddUpdateMatch(L"b", client);
    generator.AddUpdateMatch(L"c/x", client);
    generator.AddUpdateNoMatch(L"c/y");
    generator.AddUpdateMatch(L"d/x", client);
    generator.DispatchUpdatesAndVerify(gateway, client);

    generator.SetPrimaryLocation(L"b", L"primary_address_3");
    generator.SetPrimaryLocation(L"d/x", L"primary_address_3");

    generator.AddUpdateNoMatch(L"a");
    generator.AddUpdateMatch(L"b", client);
    generator.AddUpdateNoMatch(L"c/x");
    generator.AddUpdateNoMatch(L"c/y");
    generator.AddUpdateMatch(L"d/x", client);
    generator.DispatchUpdatesAndVerify(gateway, client);

    generator.SetPrimaryLocation(L"b", L"primary_address_4");
    generator.SetPrimaryLocation(L"d/x", L"primary_address_4");
    generator.SetPrimaryLocation(L"c/y", L"primary_address_1");

    generator.AddUpdateNoMatch(L"a");
    generator.AddUpdateMatch(L"b", client);
    generator.AddUpdateNoMatch(L"c/x");
    generator.AddUpdateMatch(L"c/y", client);
    generator.AddUpdateMatch(L"d/x", client);
    generator.DispatchUpdatesAndVerify(gateway, client);

    generator.SetPrimaryLocation(L"b", L"primary_address_5");
    generator.SetPrimaryLocation(L"d/x", L"primary_address_5");

    generator.AddUpdateNoMatch(L"a");
    generator.AddUpdateMatch(L"b", client);
    generator.AddUpdateNoMatch(L"c/x");
    generator.AddUpdateNoMatch(L"c/y");
    generator.AddUpdateMatch(L"d/x", client);
    generator.DispatchUpdatesAndVerify(gateway, client);

    generator.SetPrimaryLocation(L"b", L"primary_address_6");
    generator.SetPrimaryLocation(L"d/x", L"primary_address_6");
    generator.SetPrimaryLocation(L"c/x", L"");

    generator.AddUpdateNoMatch(L"a");
    generator.AddUpdateMatch(L"b", client);
    generator.AddUpdateMatch(L"c/x", client);
    generator.AddUpdateNoMatch(L"c/y");
    generator.AddUpdateMatch(L"d/x", client);
    generator.DispatchUpdatesAndVerify(gateway, client);

    generator.SetPrimaryLocation(L"b", L"primary_address_7");
    generator.SetPrimaryLocation(L"d/x", L"primary_address_7");

    generator.AddUpdateNoMatch(L"a");
    generator.AddUpdateMatch(L"b", client);
    generator.AddUpdateNoMatch(L"c/x");
    generator.AddUpdateNoMatch(L"c/y");
    generator.AddUpdateMatch(L"d/x", client);
    generator.DispatchUpdatesAndVerify(gateway, client);

    generator.SetPrimaryLocation(L"b", L"primary_address_8");
    generator.SetPrimaryLocation(L"d/x", L"primary_address_8");
    generator.SetPrimaryLocation(L"c/y", L"");

    generator.AddUpdateNoMatch(L"a");
    generator.AddUpdateMatch(L"b", client);
    generator.AddUpdateNoMatch(L"c/x");
    generator.AddUpdateMatch(L"c/y", client);
    generator.AddUpdateMatch(L"d/x", client);
    generator.DispatchUpdatesAndVerify(gateway, client);

    generator.SetPrimaryLocation(L"b", L"primary_address_9");
    generator.SetPrimaryLocation(L"d/x", L"primary_address_9");

    generator.AddUpdateNoMatch(L"a");
    generator.AddUpdateMatch(L"b", client);
    generator.AddUpdateNoMatch(L"c/x");
    generator.AddUpdateNoMatch(L"c/y");
    generator.AddUpdateMatch(L"d/x", client);
    generator.DispatchUpdatesAndVerify(gateway, client);

    generator.SetPrimaryLocation(L"b", L"primary_address_10");
    generator.SetPrimaryLocation(L"d/x", L"primary_address_10");

    gateway->Close();
    client->WaitForDisconnection();

    generator.AddUpdateNoMatch(L"a");
    generator.AddUpdateMatch(L"b", client);
    generator.AddUpdateNoMatch(L"c/x");
    generator.AddUpdateNoMatch(L"c/y");
    generator.AddUpdateMatch(L"d/x", client);
    generator.DispatchUpdatesReOpenAndVerify(gateway, client);

    generator.SetPrimaryLocation(L"a", L"primary_address_6");
    generator.SetPrimaryLocation(L"b", L"primary_address_11");
    generator.SetPrimaryLocation(L"d/x", L"primary_address_11");

    gateway->Close();
    client->WaitForDisconnection();

    generator.AddUpdateMatch(L"a", client);
    generator.AddUpdateMatch(L"b", client);
    generator.AddUpdateNoMatch(L"c/x");
    generator.AddUpdateNoMatch(L"c/y");
    generator.AddUpdateMatch(L"d/x", client);
    generator.DispatchUpdatesReOpenAndVerify(gateway, client);

    generator.SetPrimaryLocation(L"b", L"primary_address_12");
    generator.SetPrimaryLocation(L"d/x", L"primary_address_12");
    generator.SetPrimaryLocation(L"c/x", L"primary_address_7");

    gateway->Close();
    client->WaitForDisconnection();

    generator.AddUpdateNoMatch(L"a");
    generator.AddUpdateMatch(L"b", client);
    generator.AddUpdateMatch(L"c/x", client);
    generator.AddUpdateNoMatch(L"c/y");
    generator.AddUpdateMatch(L"d/x", client);
    generator.DispatchUpdatesReOpenAndVerify(gateway, client);

    generator.SetPrimaryLocation(L"b", L"primary_address_13");
    generator.SetPrimaryLocation(L"d/x", L"primary_address_13");
    generator.SetPrimaryLocation(L"c/y", L"primary_address_8");

    gateway->Close();
    client->WaitForDisconnection();

    generator.AddUpdateNoMatch(L"a");
    generator.AddUpdateMatch(L"b", client);
    generator.AddUpdateNoMatch(L"c/x");
    generator.AddUpdateMatch(L"c/y", client);
    generator.AddUpdateMatch(L"d/x", client);
    generator.DispatchUpdatesReOpenAndVerify(gateway, client);

    generator.SetPrimaryLocation(L"a", L"primary_address_9");
    generator.SetPrimaryLocation(L"b", L"primary_address_14");
    generator.SetPrimaryLocation(L"d/x", L"primary_address_14");
    generator.SetPrimaryLocation(L"c/x", L"primary_address_10");
    generator.SetPrimaryLocation(L"c/y", L"primary_address_11");

    gateway->Close();
    client->WaitForDisconnection();

    generator.AddUpdateMatch(L"a", client);
    generator.AddUpdateMatch(L"b", client);
    generator.AddUpdateMatch(L"c/x", client);
    generator.AddUpdateMatch(L"c/y", client);
    generator.AddUpdateMatch(L"d/x", client);
    generator.DispatchUpdatesReOpenAndVerify(gateway, client);

    generator.SetPrimaryLocation(L"a", L"");
    generator.SetPrimaryLocation(L"b", L"");
    generator.SetPrimaryLocation(L"d/x", L"");

    gateway->Close();
    client->WaitForDisconnection();

    generator.AddUpdateMatch(L"a", client);
    generator.AddUpdateMatch(L"b", client);
    generator.AddUpdateNoMatch(L"c/x");
    generator.AddUpdateNoMatch(L"c/y");
    generator.AddUpdateMatch(L"d/x", client);
    generator.DispatchUpdatesReOpenAndVerify(gateway, client);

    generator.SetPrimaryLocation(L"b", L"primary_address_1");
    generator.SetPrimaryLocation(L"d/x", L"primary_address_1");
    generator.SetPrimaryLocation(L"c/x", L"");

    gateway->Close();
    client->WaitForDisconnection();

    generator.AddUpdateNoMatch(L"a");
    generator.AddUpdateMatch(L"b", client);
    generator.AddUpdateMatch(L"c/x", client);
    generator.AddUpdateNoMatch(L"c/y");
    generator.AddUpdateMatch(L"d/x", client);
    generator.DispatchUpdatesReOpenAndVerify(gateway, client);

    generator.SetPrimaryLocation(L"b", L"");
    generator.SetPrimaryLocation(L"d/x", L"p");
    generator.SetPrimaryLocation(L"c/y", L"");

    gateway->Close();
    client->WaitForDisconnection();

    generator.AddUpdateNoMatch(L"a");
    generator.AddUpdateMatch(L"b", client);
    generator.AddUpdateNoMatch(L"c/x");
    generator.AddUpdateMatch(L"c/y", client);
    generator.AddUpdateMatch(L"d/x", client);
    generator.DispatchUpdatesReOpenAndVerify(gateway, client);

    client->Close();
    gateway->Close();
}

BOOST_AUTO_TEST_CASE(PrimaryOnlyFiltersMultiClientTest)
{
    auto gateway = make_shared<MockNotificationGateway>(77);
    gateway->Open();

    auto client1 = make_shared<MockNotificationClient>(L"PrimaryOnlyFiltersMultiClientTest-Client1", *gateway);
    client1->Open();

    auto client2 = make_shared<MockNotificationClient>(L"PrimaryOnlyFiltersMultiClientTest-Client2", *gateway);
    client2->Open();

    vector<shared_ptr<MockNotificationClient>> allClients;
    allClients.push_back(client1);
    allClients.push_back(client2);

    UpdateGenerator generator;

    client1->RegisterPrimaryOnlyExactFilter(L"x");
    client2->RegisterPrimaryOnlyExactFilter(L"x");

    client1->RegisterPrimaryOnlyExactFilter(L"a1");
    client1->RegisterPrimaryOnlyExactFilter(L"a12");
    client2->RegisterPrimaryOnlyExactFilter(L"a2");
    client2->RegisterPrimaryOnlyExactFilter(L"a12");

    generator.SetPrimaryLocation(L"x", L"primary_address_0");
    generator.SetPrimaryLocation(L"a1", L"primary_address_0");
    generator.SetPrimaryLocation(L"a2", L"primary_address_0");
    generator.SetPrimaryLocation(L"a12", L"primary_address_0");

    generator.SetSecondaryLocationPrefix(L"a1", L"secondary_address");
    generator.SetSecondaryLocationPrefix(L"a2", L"secondary_address");
    generator.SetSecondaryLocationPrefix(L"a12", L"secondary_address");

    generator.AddUpdateMatch(L"x", allClients);
    generator.AddUpdateMatch(L"a1", client1);
    generator.AddUpdateMatch(L"a2", client2);
    generator.AddUpdateMatch(L"a12", allClients);
    generator.DispatchUpdatesAndVerify(gateway, allClients);

    generator.SetPrimaryLocation(L"x", L"primary_address_1");
    generator.SetPrimaryLocation(L"a1", L"primary_address_1");

    generator.AddUpdateMatch(L"x", allClients);
    generator.AddUpdateMatch(L"a1", client1);
    generator.AddUpdateNoMatch(L"a2");
    generator.AddUpdateNoMatch(L"a12");
    generator.DispatchUpdatesAndVerify(gateway, allClients);

    generator.SetPrimaryLocation(L"x", L"primary_address_2");
    generator.SetPrimaryLocation(L"a2", L"primary_address_2");

    generator.AddUpdateMatch(L"x", allClients);
    generator.AddUpdateNoMatch(L"a1");
    generator.AddUpdateMatch(L"a2", client2);
    generator.AddUpdateNoMatch(L"a12");
    generator.DispatchUpdatesAndVerify(gateway, allClients);

    generator.SetPrimaryLocation(L"x", L"primary_address_3");
    generator.SetPrimaryLocation(L"a12", L"primary_address_3");

    generator.AddUpdateMatch(L"x", allClients);
    generator.AddUpdateNoMatch(L"a1");
    generator.AddUpdateNoMatch(L"a2");
    generator.AddUpdateMatch(L"a12", allClients);
    generator.DispatchUpdatesAndVerify(gateway, allClients);

    generator.SetPrimaryLocation(L"x", L"primary_address_4");
    generator.SetPrimaryLocation(L"a1", L"");

    generator.AddUpdateMatch(L"x", allClients);
    generator.AddUpdateMatch(L"a1", client1);
    generator.AddUpdateNoMatch(L"a2");
    generator.AddUpdateNoMatch(L"a12");
    generator.DispatchUpdatesAndVerify(gateway, allClients);

    generator.SetPrimaryLocation(L"x", L"primary_address_5");
    generator.SetPrimaryLocation(L"a2", L"");

    generator.AddUpdateMatch(L"x", allClients);
    generator.AddUpdateNoMatch(L"a1");
    generator.AddUpdateMatch(L"a2", client2);
    generator.AddUpdateNoMatch(L"a12");
    generator.DispatchUpdatesAndVerify(gateway, allClients);

    generator.SetPrimaryLocation(L"x", L"");
    generator.SetPrimaryLocation(L"a12", L"");

    generator.AddUpdateMatch(L"x", allClients);
    generator.AddUpdateNoMatch(L"a1");
    generator.AddUpdateNoMatch(L"a2");
    generator.AddUpdateMatch(L"a12", allClients);
    generator.DispatchUpdatesAndVerify(gateway, allClients);

    client1->RegisterPrimaryOnlyPrefixFilter(L"b1");
    client1->RegisterPrimaryOnlyPrefixFilter(L"b12");
    client2->RegisterPrimaryOnlyPrefixFilter(L"b2");
    client2->RegisterPrimaryOnlyPrefixFilter(L"b12");

    generator.SetPrimaryLocation(L"x", L"primary_address_0");

    generator.SetPrimaryLocation(L"b1/x", L"primary_address_0");
    generator.SetPrimaryLocation(L"b2/x", L"primary_address_0");
    generator.SetPrimaryLocation(L"b12/x", L"primary_address_0");

    generator.SetSecondaryLocationPrefix(L"b1/x", L"secondary_address");
    generator.SetSecondaryLocationPrefix(L"b2/x", L"secondary_address");
    generator.SetSecondaryLocationPrefix(L"b12/x", L"secondary_address");

    generator.SetPrimaryLocation(L"b1/y", L"primary_address_0");
    generator.SetPrimaryLocation(L"b2/y", L"primary_address_0");
    generator.SetPrimaryLocation(L"b12/y", L"primary_address_0");

    generator.SetSecondaryLocationPrefix(L"b1/y", L"secondary_address");
    generator.SetSecondaryLocationPrefix(L"b2/y", L"secondary_address");
    generator.SetSecondaryLocationPrefix(L"b12/y", L"secondary_address");

    generator.AddUpdateMatch(L"x", allClients);
    generator.AddUpdateMatch(L"b1/x", client1);
    generator.AddUpdateMatch(L"b1/y", client1);
    generator.AddUpdateMatch(L"b2/x", client2);
    generator.AddUpdateMatch(L"b2/y", client2);
    generator.AddUpdateMatch(L"b12/x", allClients);
    generator.AddUpdateMatch(L"b12/y", allClients);
    generator.DispatchUpdatesAndVerify(gateway, allClients);

    generator.SetPrimaryLocation(L"x", L"primary_address_1");
    generator.SetPrimaryLocation(L"b1/x", L"primary_address_1");

    generator.AddUpdateMatch(L"x", allClients);
    generator.AddUpdateMatch(L"b1/x", client1);
    generator.AddUpdateNoMatch(L"b1/y");
    generator.AddUpdateNoMatch(L"b2/x");
    generator.AddUpdateNoMatch(L"b2/y");
    generator.AddUpdateNoMatch(L"b12/x");
    generator.AddUpdateNoMatch(L"b12/y");
    generator.DispatchUpdatesAndVerify(gateway, allClients);

    generator.SetPrimaryLocation(L"x", L"primary_address_2");
    generator.SetPrimaryLocation(L"b2/x", L"primary_address_2");

    generator.AddUpdateMatch(L"x", allClients);
    generator.AddUpdateNoMatch(L"b1/x");
    generator.AddUpdateNoMatch(L"b1/y");
    generator.AddUpdateMatch(L"b2/x", client2);
    generator.AddUpdateNoMatch(L"b2/y");
    generator.AddUpdateNoMatch(L"b12/x");
    generator.AddUpdateNoMatch(L"b12/y");
    generator.DispatchUpdatesAndVerify(gateway, allClients);

    generator.SetPrimaryLocation(L"x", L"primary_address_3");
    generator.SetPrimaryLocation(L"b12/x", L"primary_address_3");

    generator.AddUpdateMatch(L"x", allClients);
    generator.AddUpdateNoMatch(L"b1/x");
    generator.AddUpdateNoMatch(L"b1/y");
    generator.AddUpdateNoMatch(L"b2/x");
    generator.AddUpdateNoMatch(L"b2/y");
    generator.AddUpdateMatch(L"b12/x", allClients);
    generator.AddUpdateNoMatch(L"b12/y");
    generator.DispatchUpdatesAndVerify(gateway, allClients);

    generator.SetPrimaryLocation(L"x", L"primary_address_4");
    generator.SetPrimaryLocation(L"b1/y", L"primary_address_4");

    generator.AddUpdateMatch(L"x", allClients);
    generator.AddUpdateNoMatch(L"b1/x");
    generator.AddUpdateMatch(L"b1/y", client1);
    generator.AddUpdateNoMatch(L"b2/x");
    generator.AddUpdateNoMatch(L"b2/y");
    generator.AddUpdateNoMatch(L"b12/x");
    generator.AddUpdateNoMatch(L"b12/y");
    generator.DispatchUpdatesAndVerify(gateway, allClients);

    generator.SetPrimaryLocation(L"x", L"primary_address_5");
    generator.SetPrimaryLocation(L"b2/y", L"primary_address_5");

    generator.AddUpdateMatch(L"x", allClients);
    generator.AddUpdateNoMatch(L"b1/x");
    generator.AddUpdateNoMatch(L"b1/y");
    generator.AddUpdateNoMatch(L"b2/x");
    generator.AddUpdateMatch(L"b2/y", client2);
    generator.AddUpdateNoMatch(L"b12/x");
    generator.AddUpdateNoMatch(L"b12/y");
    generator.DispatchUpdatesAndVerify(gateway, allClients);

    generator.SetPrimaryLocation(L"x", L"primary_address_6");
    generator.SetPrimaryLocation(L"b12/y", L"primary_address_6");

    generator.AddUpdateMatch(L"x", allClients);
    generator.AddUpdateNoMatch(L"b1/x");
    generator.AddUpdateNoMatch(L"b1/y");
    generator.AddUpdateNoMatch(L"b2/x");
    generator.AddUpdateNoMatch(L"b2/y");
    generator.AddUpdateNoMatch(L"b12/x");
    generator.AddUpdateMatch(L"b12/y", allClients);
    generator.DispatchUpdatesAndVerify(gateway, allClients);

    generator.SetPrimaryLocation(L"x", L"primary_address_7");
    generator.SetPrimaryLocation(L"b1/x", L"");

    generator.AddUpdateMatch(L"x", allClients);
    generator.AddUpdateMatch(L"b1/x", client1);
    generator.AddUpdateNoMatch(L"b1/y");
    generator.AddUpdateNoMatch(L"b2/x");
    generator.AddUpdateNoMatch(L"b2/y");
    generator.AddUpdateNoMatch(L"b12/x");
    generator.AddUpdateNoMatch(L"b12/y");
    generator.DispatchUpdatesAndVerify(gateway, allClients);

    generator.SetPrimaryLocation(L"x", L"primary_address_8");
    generator.SetPrimaryLocation(L"b2/x", L"");

    generator.AddUpdateMatch(L"x", allClients);
    generator.AddUpdateNoMatch(L"b1/x");
    generator.AddUpdateNoMatch(L"b1/y");
    generator.AddUpdateMatch(L"b2/x", client2);
    generator.AddUpdateNoMatch(L"b2/y");
    generator.AddUpdateNoMatch(L"b12/x");
    generator.AddUpdateNoMatch(L"b12/y");
    generator.DispatchUpdatesAndVerify(gateway, allClients);

    generator.SetPrimaryLocation(L"x", L"primary_address_9");
    generator.SetPrimaryLocation(L"b12/x", L"");

    generator.AddUpdateMatch(L"x", allClients);
    generator.AddUpdateNoMatch(L"b1/x");
    generator.AddUpdateNoMatch(L"b1/y");
    generator.AddUpdateNoMatch(L"b2/x");
    generator.AddUpdateNoMatch(L"b2/y");
    generator.AddUpdateMatch(L"b12/x", allClients);
    generator.AddUpdateNoMatch(L"b12/y");
    generator.DispatchUpdatesAndVerify(gateway, allClients);

    generator.SetPrimaryLocation(L"x", L"primary_address_10");
    generator.SetPrimaryLocation(L"b1/y", L"");

    generator.AddUpdateMatch(L"x", allClients);
    generator.AddUpdateNoMatch(L"b1/x");
    generator.AddUpdateMatch(L"b1/y", client1);
    generator.AddUpdateNoMatch(L"b2/x");
    generator.AddUpdateNoMatch(L"b2/y");
    generator.AddUpdateNoMatch(L"b12/x");
    generator.AddUpdateNoMatch(L"b12/y");
    generator.DispatchUpdatesAndVerify(gateway, allClients);

    generator.SetPrimaryLocation(L"x", L"primary_address_11");
    generator.SetPrimaryLocation(L"b2/y", L"");

    generator.AddUpdateMatch(L"x", allClients);
    generator.AddUpdateNoMatch(L"b1/x");
    generator.AddUpdateNoMatch(L"b1/y");
    generator.AddUpdateNoMatch(L"b2/x");
    generator.AddUpdateMatch(L"b2/y", client2);
    generator.AddUpdateNoMatch(L"b12/x");
    generator.AddUpdateNoMatch(L"b12/y");
    generator.DispatchUpdatesAndVerify(gateway, allClients);

    generator.SetPrimaryLocation(L"x", L"");
    generator.SetPrimaryLocation(L"b12/y", L"");

    generator.AddUpdateMatch(L"x", allClients);
    generator.AddUpdateNoMatch(L"b1/x");
    generator.AddUpdateNoMatch(L"b1/y");
    generator.AddUpdateNoMatch(L"b2/x");
    generator.AddUpdateNoMatch(L"b2/y");
    generator.AddUpdateNoMatch(L"b12/x");
    generator.AddUpdateMatch(L"b12/y", allClients);
    generator.DispatchUpdatesAndVerify(gateway, allClients);

    generator.SetPrimaryLocation(L"x", L"primary_address_0");
    generator.SetPrimaryLocation(L"a1", L"primary_address_1");

    gateway->Close();
    MockNotificationClient::WaitForDisconnection(allClients);

    generator.AddUpdateMatch(L"x", allClients);
    generator.AddUpdateMatch(L"a1", client1);
    generator.AddUpdateNoMatch(L"a2");
    generator.AddUpdateNoMatch(L"a12");
    generator.DispatchUpdatesReOpenAndVerify(gateway, allClients);

    generator.SetPrimaryLocation(L"x", L"primary_address_1");
    generator.SetPrimaryLocation(L"a2", L"primary_address_2");

    gateway->Close();
    MockNotificationClient::WaitForDisconnection(allClients);

    generator.AddUpdateMatch(L"x", allClients);
    generator.AddUpdateNoMatch(L"a1");
    generator.AddUpdateMatch(L"a2", client2);
    generator.AddUpdateNoMatch(L"a12");
    generator.DispatchUpdatesReOpenAndVerify(gateway, allClients);

    generator.SetPrimaryLocation(L"x", L"primary_address_2");
    generator.SetPrimaryLocation(L"a12", L"primary_address_3");

    gateway->Close();
    MockNotificationClient::WaitForDisconnection(allClients);

    generator.AddUpdateMatch(L"x", allClients);
    generator.AddUpdateNoMatch(L"a1");
    generator.AddUpdateNoMatch(L"a2");
    generator.AddUpdateMatch(L"a12", allClients);
    generator.DispatchUpdatesReOpenAndVerify(gateway, allClients);

    generator.SetPrimaryLocation(L"x", L"primary_address_3");
    generator.SetPrimaryLocation(L"a1", L"");

    gateway->Close();
    MockNotificationClient::WaitForDisconnection(allClients);

    generator.AddUpdateMatch(L"x", allClients);
    generator.AddUpdateMatch(L"a1", client1);
    generator.AddUpdateNoMatch(L"a2");
    generator.AddUpdateNoMatch(L"a12");
    generator.DispatchUpdatesReOpenAndVerify(gateway, allClients);

    generator.SetPrimaryLocation(L"x", L"primary_address_4");
    generator.SetPrimaryLocation(L"a2", L"");

    gateway->Close();
    MockNotificationClient::WaitForDisconnection(allClients);

    generator.AddUpdateMatch(L"x", allClients);
    generator.AddUpdateNoMatch(L"a1");
    generator.AddUpdateMatch(L"a2", client2);
    generator.AddUpdateNoMatch(L"a12");
    generator.DispatchUpdatesReOpenAndVerify(gateway, allClients);

    generator.SetPrimaryLocation(L"x", L"primary_address_5");
    generator.SetPrimaryLocation(L"a12", L"");

    gateway->Close();
    MockNotificationClient::WaitForDisconnection(allClients);

    generator.AddUpdateMatch(L"x", allClients);
    generator.AddUpdateNoMatch(L"a1");
    generator.AddUpdateNoMatch(L"a2");
    generator.AddUpdateMatch(L"a12", allClients);
    generator.DispatchUpdatesReOpenAndVerify(gateway, allClients);

    generator.SetPrimaryLocation(L"x", L"primary_address_6");
    generator.SetPrimaryLocation(L"b1/x", L"primary_address_0");
    generator.SetPrimaryLocation(L"b2/x", L"primary_address_0");
    generator.SetPrimaryLocation(L"b12/x", L"primary_address_0");
    generator.SetPrimaryLocation(L"b1/y", L"primary_address_0");
    generator.SetPrimaryLocation(L"b2/y", L"primary_address_0");
    generator.SetPrimaryLocation(L"b12/y", L"primary_address_0");

    gateway->Close();
    MockNotificationClient::WaitForDisconnection(allClients);

    generator.AddUpdateMatch(L"x", allClients);
    generator.AddUpdateMatch(L"b1/x", client1);
    generator.AddUpdateMatch(L"b1/y", client1);
    generator.AddUpdateMatch(L"b2/x", client2);
    generator.AddUpdateMatch(L"b2/y", client2);
    generator.AddUpdateMatch(L"b12/x", allClients);
    generator.AddUpdateMatch(L"b12/y", allClients);
    generator.DispatchUpdatesReOpenAndVerify(gateway, allClients);

    generator.SetPrimaryLocation(L"x", L"primary_address_7");
    generator.SetPrimaryLocation(L"b1/x", L"primary_address_1");

    gateway->Close();
    MockNotificationClient::WaitForDisconnection(allClients);

    generator.AddUpdateMatch(L"x", allClients);
    generator.AddUpdateMatch(L"b1/x", client1);
    generator.AddUpdateNoMatch(L"b1/y");
    generator.AddUpdateNoMatch(L"b2/x");
    generator.AddUpdateNoMatch(L"b2/y");
    generator.AddUpdateNoMatch(L"b12/x");
    generator.AddUpdateNoMatch(L"b12/y");
    generator.DispatchUpdatesReOpenAndVerify(gateway, allClients);

    generator.SetPrimaryLocation(L"x", L"primary_address_8");
    generator.SetPrimaryLocation(L"b2/x", L"primary_address_2");

    gateway->Close();
    MockNotificationClient::WaitForDisconnection(allClients);

    generator.AddUpdateMatch(L"x", allClients);
    generator.AddUpdateNoMatch(L"b1/x");
    generator.AddUpdateNoMatch(L"b1/y");
    generator.AddUpdateMatch(L"b2/x", client2);
    generator.AddUpdateNoMatch(L"b2/y");
    generator.AddUpdateNoMatch(L"b12/x");
    generator.AddUpdateNoMatch(L"b12/y");
    generator.DispatchUpdatesReOpenAndVerify(gateway, allClients);

    generator.SetPrimaryLocation(L"x", L"primary_address_9");
    generator.SetPrimaryLocation(L"b12/x", L"primary_address_3");

    gateway->Close();
    MockNotificationClient::WaitForDisconnection(allClients);

    generator.AddUpdateMatch(L"x", allClients);
    generator.AddUpdateNoMatch(L"b1/x");
    generator.AddUpdateNoMatch(L"b1/y");
    generator.AddUpdateNoMatch(L"b2/x");
    generator.AddUpdateNoMatch(L"b2/y");
    generator.AddUpdateMatch(L"b12/x", allClients);
    generator.AddUpdateNoMatch(L"b12/y");
    generator.DispatchUpdatesReOpenAndVerify(gateway, allClients);

    generator.SetPrimaryLocation(L"x", L"primary_address_10");
    generator.SetPrimaryLocation(L"b1/y", L"primary_address_4");

    gateway->Close();
    MockNotificationClient::WaitForDisconnection(allClients);

    generator.AddUpdateMatch(L"x", allClients);
    generator.AddUpdateNoMatch(L"b1/x");
    generator.AddUpdateMatch(L"b1/y", client1);
    generator.AddUpdateNoMatch(L"b2/x");
    generator.AddUpdateNoMatch(L"b2/y");
    generator.AddUpdateNoMatch(L"b12/x");
    generator.AddUpdateNoMatch(L"b12/y");
    generator.DispatchUpdatesReOpenAndVerify(gateway, allClients);

    generator.SetPrimaryLocation(L"x", L"primary_address_11");
    generator.SetPrimaryLocation(L"b2/y", L"primary_address_5");

    gateway->Close();
    MockNotificationClient::WaitForDisconnection(allClients);

    generator.AddUpdateMatch(L"x", allClients);
    generator.AddUpdateNoMatch(L"b1/x");
    generator.AddUpdateNoMatch(L"b1/y");
    generator.AddUpdateNoMatch(L"b2/x");
    generator.AddUpdateMatch(L"b2/y", client2);
    generator.AddUpdateNoMatch(L"b12/x");
    generator.AddUpdateNoMatch(L"b12/y");
    generator.DispatchUpdatesReOpenAndVerify(gateway, allClients);

    generator.SetPrimaryLocation(L"x", L"primary_address_12");
    generator.SetPrimaryLocation(L"b12/y", L"primary_address_6");

    gateway->Close();
    MockNotificationClient::WaitForDisconnection(allClients);

    generator.AddUpdateMatch(L"x", allClients);
    generator.AddUpdateNoMatch(L"b1/x");
    generator.AddUpdateNoMatch(L"b1/y");
    generator.AddUpdateNoMatch(L"b2/x");
    generator.AddUpdateNoMatch(L"b2/y");
    generator.AddUpdateNoMatch(L"b12/x");
    generator.AddUpdateMatch(L"b12/y", allClients);
    generator.DispatchUpdatesReOpenAndVerify(gateway, allClients);

    generator.SetPrimaryLocation(L"x", L"primary_address_13");
    generator.SetPrimaryLocation(L"b1/x", L"");

    gateway->Close();
    MockNotificationClient::WaitForDisconnection(allClients);

    generator.AddUpdateMatch(L"x", allClients);
    generator.AddUpdateMatch(L"b1/x", client1);
    generator.AddUpdateNoMatch(L"b1/y");
    generator.AddUpdateNoMatch(L"b2/x");
    generator.AddUpdateNoMatch(L"b2/y");
    generator.AddUpdateNoMatch(L"b12/x");
    generator.AddUpdateNoMatch(L"b12/y");
    generator.DispatchUpdatesReOpenAndVerify(gateway, allClients);

    generator.SetPrimaryLocation(L"x", L"primary_address_14");
    generator.SetPrimaryLocation(L"b2/x", L"");

    gateway->Close();
    MockNotificationClient::WaitForDisconnection(allClients);

    generator.AddUpdateMatch(L"x", allClients);
    generator.AddUpdateNoMatch(L"b1/x");
    generator.AddUpdateNoMatch(L"b1/y");
    generator.AddUpdateMatch(L"b2/x", client2);
    generator.AddUpdateNoMatch(L"b2/y");
    generator.AddUpdateNoMatch(L"b12/x");
    generator.AddUpdateNoMatch(L"b12/y");
    generator.DispatchUpdatesReOpenAndVerify(gateway, allClients);

    generator.SetPrimaryLocation(L"x", L"primary_address_15");
    generator.SetPrimaryLocation(L"b12/x", L"");

    gateway->Close();
    MockNotificationClient::WaitForDisconnection(allClients);

    generator.AddUpdateMatch(L"x", allClients);
    generator.AddUpdateNoMatch(L"b1/x");
    generator.AddUpdateNoMatch(L"b1/y");
    generator.AddUpdateNoMatch(L"b2/x");
    generator.AddUpdateNoMatch(L"b2/y");
    generator.AddUpdateMatch(L"b12/x", allClients);
    generator.AddUpdateNoMatch(L"b12/y");
    generator.DispatchUpdatesReOpenAndVerify(gateway, allClients);

    generator.SetPrimaryLocation(L"x", L"primary_address_16");
    generator.SetPrimaryLocation(L"b1/y", L"");

    gateway->Close();
    MockNotificationClient::WaitForDisconnection(allClients);

    generator.AddUpdateMatch(L"x", allClients);
    generator.AddUpdateNoMatch(L"b1/x");
    generator.AddUpdateMatch(L"b1/y", client1);
    generator.AddUpdateNoMatch(L"b2/x");
    generator.AddUpdateNoMatch(L"b2/y");
    generator.AddUpdateNoMatch(L"b12/x");
    generator.AddUpdateNoMatch(L"b12/y");
    generator.DispatchUpdatesReOpenAndVerify(gateway, allClients);

    generator.SetPrimaryLocation(L"x", L"primary_address_17");
    generator.SetPrimaryLocation(L"b2/y", L"");

    gateway->Close();
    MockNotificationClient::WaitForDisconnection(allClients);

    generator.AddUpdateMatch(L"x", allClients);
    generator.AddUpdateNoMatch(L"b1/x");
    generator.AddUpdateNoMatch(L"b1/y");
    generator.AddUpdateNoMatch(L"b2/x");
    generator.AddUpdateMatch(L"b2/y", client2);
    generator.AddUpdateNoMatch(L"b12/x");
    generator.AddUpdateNoMatch(L"b12/y");
    generator.DispatchUpdatesReOpenAndVerify(gateway, allClients);

    generator.SetPrimaryLocation(L"x", L"");
    generator.SetPrimaryLocation(L"b12/y", L"");

    gateway->Close();
    MockNotificationClient::WaitForDisconnection(allClients);

    generator.AddUpdateMatch(L"x", allClients);
    generator.AddUpdateNoMatch(L"b1/x");
    generator.AddUpdateNoMatch(L"b1/y");
    generator.AddUpdateNoMatch(L"b2/x");
    generator.AddUpdateNoMatch(L"b2/y");
    generator.AddUpdateNoMatch(L"b12/x");
    generator.AddUpdateMatch(L"b12/y", allClients);
    generator.DispatchUpdatesReOpenAndVerify(gateway, allClients);

    gateway->Close();
    client1->Close();
    client2->Close();
}

BOOST_AUTO_TEST_CASE(PagingTest)
{
    ServiceModelConfig::GetConfig().MaxMessageSize = 400;

    auto gateway = make_shared<MockNotificationGateway>(88);
    gateway->Open();

    auto client = make_shared<MockNotificationClient>(L"PagingTest-Client", *gateway);
    client->Open();

    UpdateGenerator generator(wformatString("default-endpoint-{0}", DateTime::Now().Ticks));

    client->RegisterPrefixFilter(L"a");

    generator.SetPrimaryLocation(L"a/b", L"primary_address_0");
    generator.SetPrimaryLocation(L"a/c", L"primary_address_0");
    generator.SetPrimaryLocation(L"a/d", L"primary_address_0");
    generator.SetPrimaryLocation(L"a/e", L"primary_address_0");
    generator.SetPrimaryLocation(L"a/f", L"primary_address_0");

    generator.SetSecondaryLocationPrefix(L"a/b", L"secondary_address");
    generator.SetSecondaryLocationPrefix(L"a/c", L"secondary_address");
    generator.SetSecondaryLocationPrefix(L"a/d", L"secondary_address");
    generator.SetSecondaryLocationPrefix(L"a/e", L"secondary_address");
    generator.SetSecondaryLocationPrefix(L"a/f", L"secondary_address");
    
    generator.AddUpdateMatch(L"a/b", client);
    generator.AddUpdateMatch(L"a/c", client);
    generator.AddUpdateMatch(L"a/d", client);
    generator.AddUpdateMatch(L"a/e", client);
    generator.AddUpdateMatch(L"a/f", client);
    client->SetExpectedNotificationCount(5);
    generator.DispatchUpdatesAndVerify(gateway, client);

    ServiceModelConfig::GetConfig().MaxMessageSize = 740;

    generator.AddUpdateMatch(L"a/b", client);
    generator.AddUpdateMatch(L"a/c", client);
    generator.AddUpdateMatch(L"a/d", client);
    generator.AddUpdateMatch(L"a/e", client);
    generator.AddUpdateMatch(L"a/f", client);
    client->SetExpectedNotificationCount(3);
    generator.DispatchUpdatesAndVerify(gateway, client);

    ServiceModelConfig::GetConfig().MaxMessageSize = 1160;

    generator.AddUpdateMatch(L"a/b", client);
    generator.AddUpdateMatch(L"a/c", client);
    generator.AddUpdateMatch(L"a/d", client);
    generator.AddUpdateMatch(L"a/e", client);
    generator.AddUpdateMatch(L"a/f", client);
    client->SetExpectedNotificationCount(2);
    generator.DispatchUpdatesAndVerify(gateway, client);

    ServiceModelConfig::GetConfig().MaxMessageSize = 400;

    gateway->Close();
    client->WaitForDisconnection();

    generator.AddUpdateMatch(L"a/b", client);
    generator.AddUpdateMatch(L"a/c", client);
    generator.AddUpdateMatch(L"a/d", client);
    generator.AddUpdateMatch(L"a/e", client);
    generator.AddUpdateMatch(L"a/f", client);
    client->SetExpectedNotificationCount(5);
    generator.DispatchUpdatesReOpenWithDelayAndVerify(gateway, client);

    ServiceModelConfig::GetConfig().MaxMessageSize = 800;

    gateway->Close();
    client->WaitForDisconnection();

    generator.AddUpdateMatch(L"a/b", client);
    generator.AddUpdateMatch(L"a/c", client);
    generator.AddUpdateMatch(L"a/d", client);
    generator.AddUpdateMatch(L"a/e", client);
    generator.AddUpdateMatch(L"a/f", client);
    client->SetExpectedNotificationCount(3);
    generator.DispatchUpdatesReOpenWithDelayAndVerify(gateway, client);

    ServiceModelConfig::GetConfig().MaxMessageSize = 1200;

    gateway->Close();
    client->WaitForDisconnection();

    generator.AddUpdateMatch(L"a/b", client);
    generator.AddUpdateMatch(L"a/c", client);
    generator.AddUpdateMatch(L"a/d", client);
    generator.AddUpdateMatch(L"a/e", client);
    generator.AddUpdateMatch(L"a/f", client);
    client->SetExpectedNotificationCount(2);
    generator.DispatchUpdatesReOpenWithDelayAndVerify(gateway, client);

    gateway->Close();
    client->Close();
}

BOOST_AUTO_TEST_CASE(ForceDisconnectClientTest)
{
    auto gateway = make_shared<MockNotificationGateway>(99);
    gateway->Open();

    auto client = make_shared<MockNotificationClient>(L"ForceDisconnectClientTest-Client", *gateway);
    client->Open();

    UpdateGenerator generator(wformatString("default-endpoint-{0}", DateTime::Now().Ticks));

    wstring dropNotificationsSpecName(L"DropNotifications");
    wstring dropNotificationsSpec(L"* * ServiceNotificationRequest");
    UnreliableTransportConfig::GetConfig().AddSpecification(dropNotificationsSpecName, dropNotificationsSpec);

    ServiceModelConfig::GetConfig().ServiceNotificationTimeout = TimeSpan::FromSeconds(2);
    ServiceModelConfig::GetConfig().MaxOutstandingNotificationsPerClient = 10;

    client->RegisterPrefixFilter(L"a");

    for (auto ix=0; ix<ServiceModelConfig::GetConfig().MaxOutstandingNotificationsPerClient + 1; ++ix)
    {
        generator.AddUpdateMatch(wformatString("a/child-{0}", ix), client);
        generator.DispatchUpdatesNoClear(gateway);
    }

    client->VerifyConnection(gateway);

    client->WaitForReconnection();

    client->VerifyConnection(gateway);

    client->SetExpectedNotificationCount(1);

    UnreliableTransportConfig::GetConfig().RemoveSpecification(dropNotificationsSpecName);

    generator.Verify(gateway, client);

    gateway->Close();

    client->WaitForDisconnection();

    client->Close();
}


BOOST_AUTO_TEST_CASE(MultiGatewayTest)
{
    auto generation = DateTime::Now().Ticks;

    auto gateway1 = make_shared<MockNotificationGateway>(1010, generation);
    gateway1->Open();

    auto gateway2 = make_shared<MockNotificationGateway>(1011, generation);
    gateway2->Open();

    auto gateway3 = make_shared<MockNotificationGateway>(1012, generation);
    gateway3->Open();

    vector<shared_ptr<MockGatewayBase>> allGateways;
    allGateways.push_back(gateway1);
    allGateways.push_back(gateway2);
    allGateways.push_back(gateway3);

    vector<shared_ptr<MockNotificationGateway>> allNotificationGateways;
    allNotificationGateways.push_back(gateway1);
    allNotificationGateways.push_back(gateway2);
    allNotificationGateways.push_back(gateway3);

    auto client1 = make_shared<MockNotificationClient>(L"MultiGatewayTest-Client1", allGateways);
    client1->Open();

    vector<shared_ptr<MockNotificationClient>> allClients;
    allClients.push_back(client1);

    UpdateGenerator generator(wformatString("default-endpoint-{0}", DateTime::Now().Ticks));

    client1->RegisterPrefixFilter(L"a");

    for (auto ix=0; ix<allNotificationGateways.size(); ++ix)
    {
        client1->ReOpenCurrentGateway(allNotificationGateways);
        client1->WaitForReconnection();

        generator.AddUpdateMatch(L"a/b", client1);
        generator.DispatchUpdatesAndVerify(allNotificationGateways, allClients);
    }

    auto client2 = make_shared<MockNotificationClient>(L"MultiGatewayTest-Client2", allGateways);
    auto client3 = make_shared<MockNotificationClient>(L"MultiGatewayTest-Client3", allGateways);

    client2->Open();
    client3->Open();

    client2->RegisterPrefixFilter(L"b");
    client3->RegisterPrefixFilter(L"c");

    allClients.push_back(client2);
    allClients.push_back(client3);

    for (auto ix=0; ix<allNotificationGateways.size(); ++ix)
    {
        for (auto const & gateway : allGateways)
        {
            gateway->CloseAndReOpen();
        }

        for (auto const & client : allClients)
        {
            client->WaitForReconnection();
        }

        generator.AddUpdateMatch(L"a/x", client1);
        generator.AddUpdateMatch(L"b/y", client2);
        generator.AddUpdateMatch(L"c/z", client3);
        generator.DispatchUpdatesAndVerify(allNotificationGateways, allClients);
    }

    for (auto const & gateway : allNotificationGateways)
    {
        gateway->Close();
    }

    for (auto const & client : allClients)
    {
        client->WaitForDisconnection();
        client->Close();
    }
}

BOOST_AUTO_TEST_CASE(DelayedNotificationsTest)
{
    auto gateway = make_shared<MockNotificationGateway>(1111);
    gateway->Open();

    auto client = make_shared<MockNotificationClient>(L"DelayedNotificationsTest-Client", *gateway);
    client->Open();

    UpdateGenerator generator(wformatString("default-endpoint-{0}", DateTime::Now().Ticks));

    wstring dropNotificationsSpecName(L"DropNotifications");
    wstring dropNotificationsSpec(L"* * ServiceNotificationRequest 1.0 1 4");
    UnreliableTransportConfig::GetConfig().AddSpecification(dropNotificationsSpecName, dropNotificationsSpec);

    ServiceModelConfig::GetConfig().ServiceNotificationTimeout = TimeSpan::FromMilliseconds(2500);

    // Force one partition per page in order to get a predictable notification count
    // in the face of retries and duplicates - only one notification for each
    // partition should be received.
    //
    ServiceModelConfig::GetConfig().MaxMessageSize = 400;

    client->RegisterPrimaryOnlyPrefixFilter(L"a");

    int notificationCount = 10;

    client->SetExpectedNotificationCount(notificationCount);

    for (auto ix=0; ix<notificationCount; ++ix)
    {
        wstring name = wformatString("a/child-{0}", ix);

        generator.SetPrimaryLocation(name, L"primary_address_0");
        generator.SetSecondaryLocationPrefix(name, L"secondary_address");

        generator.AddUpdateMatch(name, client);
        generator.DispatchUpdatesNoClear(gateway);
    }

    generator.Verify(gateway, client);

    gateway->Close();
    client->WaitForDisconnection();

    notificationCount = 40;

    client->SetExpectedNotificationCount(notificationCount);

    for (auto ix=0; ix<notificationCount; ++ix)
    {
        wstring name = wformatString("a/child-{0}", ix);

        generator.SetPrimaryLocation(name, L"primary_address_1");
        generator.SetSecondaryLocationPrefix(name, L"secondary_address");

        generator.AddUpdateMatch(name, client);
        generator.DispatchUpdatesNoClear(gateway);
    }

    gateway->ReOpen();
    generator.Verify(gateway, client);

    client->Close();
    gateway->Close();
}

BOOST_AUTO_TEST_CASE(MultiPartitionTest)
{
    auto gateway = make_shared<MockNotificationGateway>(1212);
    gateway->Open();

    auto client = make_shared<MockNotificationClient>(L"MultiPartitionTest-Client", *gateway);
    client->Open();

    UpdateGenerator generator(wformatString("default-endpoint-{0}", DateTime::Now().Ticks));

    generator.SetPrimaryLocation(L"a/b", L"primary_address");
    generator.SetPrimaryLocation(L"a/c", L"primary_address");

    client->RegisterPrefixFilter(L"a");

    generator.AddUpdateMatch(L"a/b", client);
    generator.AddUpdateMatch(L"a/c", client);
    generator.DispatchUpdatesAndVerify(gateway, client);
    gateway->VerifyAllPartitionsCount(2);

    auto abCuid1 = generator.GetCuid(L"a/b");
    auto acCuid1 = generator.GetCuid(L"a/c");
    auto abCuid2 = generator.SetNewCuid(L"a/b");
    auto acCuid2 = generator.SetNewCuid(L"a/c");

    generator.AddUpdateMatch(L"a/b", client);
    generator.AddUpdateMatch(L"a/c", client);
    generator.DispatchUpdatesAndVerify(gateway, client);
    gateway->VerifyAllPartitionsCount(4);

    generator.SetCuid(L"a/b", abCuid1);
    generator.SetCuid(L"a/c", acCuid1);

    generator.AddUpdateMatch(L"a/b", client);
    generator.AddUpdateMatch(L"a/c", client);
    generator.DispatchUpdatesAndVerify(gateway, client);
    gateway->VerifyAllPartitionsCount(4);

    generator.SetPrimaryLocation(L"b", L"primary_address");

    client->RegisterExactFilter(L"b");

    generator.AddUpdateMatch(L"b", client);
    generator.DispatchUpdatesAndVerify(gateway, client);
    gateway->VerifyAllPartitionsCount(5);

    auto bCuid1 = generator.GetCuid(L"b");
    auto bCuid2 = generator.SetNewCuid(L"b");

    generator.AddUpdateMatch(L"b", client);
    generator.DispatchUpdatesAndVerify(gateway, client);
    gateway->VerifyAllPartitionsCount(6);

    generator.SetCuid(L"b", bCuid1);

    generator.AddUpdateMatch(L"b", client);
    generator.DispatchUpdatesAndVerify(gateway, client);
    gateway->VerifyAllPartitionsCount(6);

    generator.SetPrimaryLocation(L"a-p/x", L"primary_address_0");
    generator.SetPrimaryLocation(L"a-p/y", L"primary_address_0");

    client->RegisterPrimaryOnlyPrefixFilter(L"a-p");

    generator.AddUpdateMatch(L"a-p/x", client);
    generator.AddUpdateMatch(L"a-p/y", client);
    generator.DispatchUpdatesAndVerify(gateway, client);
    gateway->VerifyAllPartitionsCount(8);

    auto apxCuid1 = generator.GetCuid(L"a-p/x");
    auto apyCuid1 = generator.GetCuid(L"a-p/x");
    auto apxCuid2 = generator.SetNewCuid(L"a-p/x");
    auto apyCuid2 = generator.SetNewCuid(L"a-p/y");

    generator.AddUpdateMatch(L"a-p/x", client);
    generator.AddUpdateMatch(L"a-p/y", client);
    generator.DispatchUpdatesAndVerify(gateway, client);
    gateway->VerifyAllPartitionsCount(10);

    generator.SetPrimaryLocation(L"a-p/x", L"primary_address_1");

    generator.AddUpdateMatch(L"a-p/x", client);
    generator.AddUpdateNoMatch(L"a-p/y");
    generator.DispatchUpdatesAndVerify(gateway, client);
    gateway->VerifyAllPartitionsCount(10);

    generator.SetCuid(L"a-p/x", apxCuid1);

    generator.AddUpdateMatch(L"a-p/x", client);
    generator.AddUpdateNoMatch(L"a-p/y");
    generator.DispatchUpdatesAndVerify(gateway, client);
    gateway->VerifyAllPartitionsCount(10);

    generator.SetPrimaryLocation(L"a-p/y", L"primary_address_1");

    generator.AddUpdateNoMatch(L"a-p/x");
    generator.AddUpdateMatch(L"a-p/y", client);
    generator.DispatchUpdatesAndVerify(gateway, client);
    gateway->VerifyAllPartitionsCount(10);

    generator.SetCuid(L"a-p/y", apyCuid1);

    generator.AddUpdateNoMatch(L"a-p/x");
    generator.AddUpdateMatch(L"a-p/y", client);
    generator.DispatchUpdatesAndVerify(gateway, client);
    gateway->VerifyAllPartitionsCount(10);

    generator.SetPrimaryLocation(L"b-p", L"primary_address_0");

    client->RegisterPrimaryOnlyExactFilter(L"b-p");

    generator.AddUpdateMatch(L"b-p", client);
    generator.DispatchUpdatesAndVerify(gateway, client);
    gateway->VerifyAllPartitionsCount(11);

    auto bpCuid1 = generator.GetCuid(L"b-p");
    auto bpCuid2 = generator.SetNewCuid(L"b-p");

    generator.AddUpdateMatch(L"b-p", client);
    generator.DispatchUpdatesAndVerify(gateway, client);
    gateway->VerifyAllPartitionsCount(12);

    generator.SetPrimaryLocation(L"b-p", L"primary_address_1");

    generator.SetCuid(L"b-p", bpCuid1);

    generator.AddUpdateMatch(L"b-p", client);
    generator.DispatchUpdatesAndVerify(gateway, client);
    gateway->VerifyAllPartitionsCount(12);

    generator.SetPrimaryLocation(L"a/b", L"primary_address_2");
    generator.SetPrimaryLocation(L"a/c", L"primary_address_2");
    generator.SetPrimaryLocation(L"b", L"primary_address_2");
    generator.SetPrimaryLocation(L"a-p/x", L"primary_address_2");
    generator.SetPrimaryLocation(L"a-p/y", L"primary_address_2");
    generator.SetPrimaryLocation(L"b-p", L"primary_address_2");

    gateway->Close();
    client->WaitForDisconnection();

    generator.AddUpdateMatch(L"a/b", client);
    generator.AddUpdateMatch(L"a/c", client);
    generator.AddUpdateMatch(L"b", client);
    generator.AddUpdateMatch(L"a-p/x", client);
    generator.AddUpdateMatch(L"a-p/y", client);
    generator.AddUpdateMatch(L"b-p", client);
    generator.SetCuid(L"a/b", abCuid2);
    generator.SetCuid(L"a/c", acCuid2);
    generator.SetCuid(L"b", bCuid2);
    generator.SetCuid(L"a-p/x", apxCuid2);
    generator.SetCuid(L"a-p/y", apyCuid2);
    generator.SetCuid(L"b-p", bpCuid2);
    generator.AddUpdateMatch(L"a/b", client);
    generator.AddUpdateMatch(L"a/c", client);
    generator.AddUpdateMatch(L"b", client);
    generator.AddUpdateMatch(L"a-p/x", client);
    generator.AddUpdateMatch(L"a-p/y", client);
    generator.AddUpdateMatch(L"b-p", client);
    generator.DispatchUpdatesReOpenAndVerify(gateway, client);

    gateway->VerifyAllPartitionsCount(12);

    gateway->Close();
    client->Close();
}

BOOST_AUTO_TEST_CASE(EmptyPartitionTrimTest)
{
    ServiceModelConfig::GetConfig().MaxIndexedEmptyPartitions = 5;

    auto gateway = make_shared<MockNotificationGateway>(1313);
    gateway->Open();

    auto client = make_shared<MockNotificationClient>(L"EmptyPartitionTrimTest-Client", *gateway);
    client->Open();

    UpdateGenerator generator;

    client->RegisterPrefixFilter(L"a");

    generator.SetPrimaryLocation(L"a/b", L"primary_address_0");
    generator.SetPrimaryLocation(L"a/c", L"primary_address_0");
    generator.SetPrimaryLocation(L"a/d", L"primary_address_0");
    generator.SetPrimaryLocation(L"a/e", L"primary_address_0");
    generator.SetPrimaryLocation(L"a/f", L"primary_address_0");
    
    generator.AddUpdateMatch(L"a/b", client);
    generator.AddUpdateMatch(L"a/c", client);
    generator.AddUpdateMatch(L"a/d", client);
    generator.AddUpdateMatch(L"a/e", client);
    generator.AddUpdateMatch(L"a/f", client);
    generator.DispatchUpdatesAndVerify(gateway, client);
    gateway->VerifyEmptyPartitionsCount(0);

    generator.SetPrimaryLocation(L"a/b", L"");
    generator.SetPrimaryLocation(L"a/c", L"");
    generator.SetPrimaryLocation(L"a/d", L"");
    generator.SetPrimaryLocation(L"a/e", L"");
    generator.SetPrimaryLocation(L"a/f", L"");

    generator.AddUpdateMatch(L"a/b", client);
    auto vC = generator.AddUpdateMatch(L"a/c", client);
    generator.AddUpdateMatch(L"a/d", client);
    generator.AddUpdateMatch(L"a/e", client);
    generator.AddUpdateMatch(L"a/f", client);
    generator.DispatchUpdatesAndVerify(gateway, client);
    gateway->VerifyEmptyPartitionsCount(5);

    generator.SetPrimaryLocation(L"a/b", L"primary_address_0");

    generator.AddUpdateNoMatch(L"a/c", vC);
    generator.AddUpdateMatch(L"a/b", client);
    generator.DispatchUpdatesAndVerify(gateway, client);
    gateway->VerifyEmptyPartitionsCount(4);

    generator.SetPrimaryLocation(L"a/g", L"primary_address_0");

    generator.AddUpdateNoMatch(L"a/c", vC);
    generator.AddUpdateMatch(L"a/g", client);
    generator.DispatchUpdatesAndVerify(gateway, client);
    gateway->VerifyEmptyPartitionsCount(4);

    generator.SetPrimaryLocation(L"a/g", L"primary_address_1");
    generator.SetPrimaryLocation(L"a/h", L"primary_address_1");

    generator.AddUpdateMatch(L"a/g", client);
    generator.AddUpdateMatch(L"a/h", client);
    generator.AddUpdateMatch(L"a/d", client);
    generator.AddUpdateMatch(L"a/e", client);
    generator.AddUpdateMatch(L"a/f", client);
    generator.DispatchUpdatesAndVerify(gateway, client);
    gateway->VerifyEmptyPartitionsCount(4);

    generator.SetPrimaryLocation(L"a/h", L"");
    generator.SetPrimaryLocation(L"a/i", L"primary_address_1");

    generator.AddUpdateMatch(L"a/g", client);
    generator.AddUpdateMatch(L"a/h", client);
    generator.AddUpdateMatch(L"a/i", client);
    generator.DispatchUpdatesAndVerify(gateway, client);
    gateway->VerifyEmptyPartitionsCount(5);

    generator.SetPrimaryLocation(L"a/i", L"");

    generator.AddUpdateMatch(L"a/i", client);
    generator.DispatchUpdatesAndVerify(gateway, client);
    gateway->VerifyEmptyPartitionsCount(5);

    ServiceModelConfig::GetConfig().MaxIndexedEmptyPartitions = 1;

    generator.SetPrimaryLocation(L"a/j", L"primary_address_1");
    generator.SetPrimaryLocation(L"a/k", L"primary_address_1");

    generator.AddUpdateMatch(L"a/j", client);
    generator.AddUpdateMatch(L"a/k", client);
    generator.DispatchUpdatesAndVerify(gateway, client);
    gateway->VerifyEmptyPartitionsCount(5);

    generator.SetPrimaryLocation(L"a/k", L"");

    generator.AddUpdateMatch(L"a/k", client);
    generator.DispatchUpdatesAndVerify(gateway, client);
    gateway->VerifyEmptyPartitionsCount(1);

    generator.SetPrimaryLocation(L"a/d", L"primary_address_0");
    generator.SetPrimaryLocation(L"a/e", L"primary_address_0");

    auto dVersion = generator.AddUpdateMatch(L"a/d", client);
    auto eVersion = generator.AddUpdateMatch(L"a/e", client);
    generator.DispatchUpdatesAndVerify(gateway, client);
    gateway->VerifyEmptyPartitionsCount(1);

    generator.SetPrimaryLocation(L"a/d", L"");
    generator.SetPrimaryLocation(L"a/e", L"");
    generator.SetPrimaryLocation(L"a/j", L"");

    client->SetExpectedNotificationCount(2);

    gateway->Close();
    client->WaitForDisconnection();

    generator.AddUpdateMatchForMissedDelete(L"a/d", client, dVersion);
    generator.AddUpdateMatchForMissedDelete(L"a/e", client, eVersion);
    generator.AddUpdateMatch(L"a/j", client);
    generator.DispatchUpdatesReOpenWithDelayAndVerify(gateway, client);

    gateway->VerifyEmptyPartitionsCount(1);
    gateway->ResetDelayedNotifications();

    ServiceModelConfig::GetConfig().MaxMessageSize = 20;
    ServiceModelConfig::GetConfig().MaxIndexedEmptyPartitions = 2;

    generator.SetPrimaryLocation(L"a/b", L"primary_address_0");
    generator.SetPrimaryLocation(L"a/c", L"primary_address_0");
    generator.SetPrimaryLocation(L"a/d", L"primary_address_0");
    generator.SetPrimaryLocation(L"a/e", L"primary_address_0");
    generator.SetPrimaryLocation(L"a/f", L"primary_address_0");
    generator.SetPrimaryLocation(L"a/h", L"primary_address_0");
    generator.SetPrimaryLocation(L"a/i", L"primary_address_0");
    generator.SetPrimaryLocation(L"a/j", L"primary_address_0");
    generator.SetPrimaryLocation(L"a/k", L"primary_address_0");
    
    generator.SetSecondaryLocationPrefix(L"a/b", L"secondary_address");
    generator.SetSecondaryLocationPrefix(L"a/c", L"secondary_address");
    generator.SetSecondaryLocationPrefix(L"a/d", L"secondary_address");
    generator.SetSecondaryLocationPrefix(L"a/e", L"secondary_address");
    generator.SetSecondaryLocationPrefix(L"a/f", L"secondary_address");
    
    client->SetExpectedNotificationCount(9);

    auto bVersion = generator.AddUpdateMatch(L"a/b", client);
    auto cVersion = generator.AddUpdateMatch(L"a/c", client);
    dVersion = generator.AddUpdateMatch(L"a/d", client);
    generator.AddUpdateMatch(L"a/e", client);
    generator.AddUpdateMatch(L"a/f", client);
    generator.AddUpdateMatch(L"a/h", client);
    generator.AddUpdateMatch(L"a/i", client);
    generator.AddUpdateMatch(L"a/j", client);
    generator.AddUpdateMatch(L"a/k", client);
    generator.DispatchUpdatesAndVerify(gateway, client);
    gateway->VerifyEmptyPartitionsCount(0);

    generator.SetPrimaryLocation(L"a/b", L"");
    generator.SetPrimaryLocation(L"a/c", L"");
    generator.SetPrimaryLocation(L"a/d", L"");
    generator.SetPrimaryLocation(L"a/e", L"");
    generator.SetPrimaryLocation(L"a/f", L"");
    
    generator.SetSecondaryLocationPrefix(L"a/b", L"");
    generator.SetSecondaryLocationPrefix(L"a/c", L"");
    generator.SetSecondaryLocationPrefix(L"a/d", L"");
    generator.SetSecondaryLocationPrefix(L"a/e", L"");
    generator.SetSecondaryLocationPrefix(L"a/f", L"");
    
    client->SetExpectedNotificationCount(5);

    gateway->Close();
    client->WaitForDisconnection();

    generator.AddUpdateMatchForMissedDelete(L"a/b", client, bVersion);
    generator.AddUpdateMatchForMissedDelete(L"a/c", client, cVersion);
    generator.AddUpdateMatchForMissedDelete(L"a/d", client, dVersion);
    generator.AddUpdateMatch(L"a/e", client);
    generator.AddUpdateMatch(L"a/f", client);
    generator.DispatchUpdatesReOpenWithDelayAndVerify(gateway, client);

    gateway->VerifyEmptyPartitionsCount(2);
    gateway->ResetDelayedNotifications();

    ServiceModelConfig::GetConfig().MaxIndexedEmptyPartitions = 4;

    client->SetExpectedNotificationCount(6);

    auto vK = generator.AddUpdateMatch(L"a/k", client);
    generator.AddUpdateMatch(L"a/b", client);
    generator.AddUpdateMatch(L"a/c", client);
    generator.AddUpdateMatch(L"a/d", client);
    generator.AddUpdateMatch(L"a/e", client);
    generator.AddUpdateMatch(L"a/f", client);
    generator.DispatchUpdatesAndVerify(gateway, client);
    gateway->VerifyEmptyPartitionsCount(4);

    client->SetExpectedNotificationCount(5);

    generator.SetPrimaryLocation(L"a/k", L"");

    gateway->Close();
    client->WaitForDisconnection();

    generator.AddUpdateMatchForMissedDelete(L"a/k", client, vK);
    generator.AddUpdateMatch(L"a/c", client);
    generator.AddUpdateMatch(L"a/d", client);
    generator.AddUpdateMatch(L"a/e", client);
    generator.AddUpdateMatch(L"a/f", client);
    generator.DispatchUpdatesReOpenWithDelayAndVerify(gateway, client);

    gateway->VerifyEmptyPartitionsCount(4);

    client->Close();
    gateway->Close();
}

BOOST_AUTO_TEST_CASE(DeleteNotificationTest)
{
    ServiceModelConfig::GetConfig().MaxIndexedEmptyPartitions = 5;

    auto gateway = make_shared<MockNotificationGateway>(1414);
    gateway->Open();

    auto client = make_shared<MockNotificationClient>(L"DeleteNotificationTest-Client", *gateway);
    client->Open();

    UpdateGenerator generator;

    client->RegisterPrefixFilter(L"a");

    generator.SetPrimaryLocation(L"a/b", L"");
    generator.SetPrimaryLocation(L"a/c", L"");
    generator.SetPrimaryLocation(L"a/d", L"");
    generator.SetPrimaryLocation(L"a/e", L"");
    generator.SetPrimaryLocation(L"a/f", L"");
    
    // Delete notifications should be delivered even if no live notifications
    // were ever delivered.
    //
    auto vB = generator.AddUpdateMatch(L"a/b", client);
    auto vC = generator.AddUpdateMatch(L"a/c", client);
    generator.DispatchUpdatesAndVerify(gateway, client);
    gateway->VerifyEmptyPartitionsCount(2);

    // Duplicates are suppressed by client's version ranges
    //
    generator.AddUpdateNoMatch(L"a/b", vB);
    generator.AddUpdateNoMatch(L"a/c", vC);
    generator.AddUpdateMatch(L"a/d", client);
    generator.AddUpdateMatch(L"a/e", client);
    generator.AddUpdateMatch(L"a/f", client);
    generator.DispatchUpdatesAndVerify(gateway, client);
    gateway->VerifyEmptyPartitionsCount(5);

    // Re-create and delete
    //
    generator.SetPrimaryLocation(L"a/d", L"d-location");

    generator.AddUpdateMatch(L"a/d", client);
    generator.DispatchUpdatesAndVerify(gateway, client);
    gateway->VerifyEmptyPartitionsCount(4);

    // Duplicates continue to be suppressed
    //
    generator.SetPrimaryLocation(L"a/d", L"");

    generator.AddUpdateNoMatch(L"a/b", vB);
    generator.AddUpdateNoMatch(L"a/c", vC);
    generator.AddUpdateMatch(L"a/d", client);
    generator.DispatchUpdatesAndVerify(gateway, client);
    gateway->VerifyEmptyPartitionsCount(5);

    client->Close();
    gateway->Close();
}

BOOST_AUTO_TEST_SUITE_END()

void ServiceNotificationManagerTest::DeadlockedClientTest()
{
    wstring clientId(L"DeadlockedClientTest-Client");

    // Flood the underlying transport with retries - this should not overwhelm outgoing
    // transport buffers with semantically duplicate messages. Once the client's
    // receive buffer fills, messages will stop dispatching from the gateway's
    // send buffer.
    //
    TransportConfig::GetConfig().OutgoingMessageExpirationCheckInterval = TimeSpan::MaxValue;
    ServiceModelConfig::GetConfig().ServiceNotificationTimeout = TimeSpan::FromMilliseconds(3);
    ServiceModelConfig::GetConfig().MaxOutstandingNotificationsPerClient = 5;

    auto gateway = make_shared<MockNotificationGateway>(999);
    gateway->Open();

    auto client = make_shared<MockNotificationClient>(clientId, *gateway);
    client->Open();

    UpdateGenerator generator(wformatString("default-endpoint-{0}", DateTime::Now().Ticks));

    client->RegisterPrefixFilter(L"a");

    // TODO: A queue needs to be implemented at the client to prevent the client side from exhausting threads because of large number of notifications
    // This test doesnt work correctly since the client's receive buffer doesnt fillup, since we are dispatching the messages on a different thread 
    // each time.
    client->EnableReceiveDeadlock();

    for (auto ix=0; ix<ServiceModelConfig::GetConfig().MaxOutstandingNotificationsPerClient; ++ix)
    {
        generator.AddUpdateMatch(wformatString("a/child-{0}", ix), client);
        generator.DispatchUpdatesNoClear(gateway);

        for (auto jx=0; jx<10; ++jx)
        {
            Sleep(1000);

            auto count = gateway->GetTotalPendingTransportMessageCount(clientId);
            VERIFY(count == ix || count == (ix + 1), "Pending count = {0}", count);
        }
    }

    generator.AddUpdateMatch(L"a/limit", client);
    generator.DispatchUpdatesNoClear(gateway);

    // Client is deadlocked but gateway should still
    // release registration resources
    //
    client->WaitForDisconnection(false);

    for (auto jx=0; jx<10; ++jx)
    {
        Sleep(1000);

        auto count = gateway->GetTotalPendingTransportMessageCount(clientId);
        VERIFY(count == 0, "Pending count = {0}", count);
    }

    ServiceModelConfig::GetConfig().ServiceNotificationTimeout = TimeSpan::FromSeconds(10);
    
    // Upon unblocking the client, only the first deadlocked notification will go through.
    // Any notifications in the receive buffer will be stale and dropped.
    // The remaining notifications will be picked up during reconnection.
    //
    client->SetExpectedNotificationCount(2);

    client->DisableReceiveDeadlock();

    client->WaitForReconnection();

    generator.Verify(gateway, client);

    gateway->Close();

    client->WaitForDisconnection();
}


bool ServiceNotificationManagerTest::TestcaseSetup()
{
    Config cfg;
    ServiceModelConfig::GetConfig().Test_Reset();
    UnreliableTransportConfig::GetConfig().Test_Reset();
    TransportConfig::GetConfig().Test_Reset();
    return true;
}
