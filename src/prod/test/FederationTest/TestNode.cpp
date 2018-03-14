// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace FederationTest;
using namespace Federation;
using namespace std;
using namespace Transport;
using namespace Common;
using namespace TestCommon;

const StringLiteral TraceSource = "TestNode"; 

Actor::Enum const TestActor = Actor::GenericTestActor;
wstring const P2PActionName = L"P2P";
wstring const RoutedActionName = L"Routed";
wstring const BroadcastedActionName = L"Broadcasted";
wstring const MulticastedActionName = L"Multicast";

size_t const TestNode::MaxDuplicateListSize = 150;
ExclusiveLock TestNode::DuplicateCheckListLock;
list<MessageId> TestNode::DuplicateCheckList;
int const TestNode::MaxRetryCount = 3; 

struct TestMessageBody : public Serialization::FabricSerializable
{
    TestMessageBody()
    {
    }

    TestMessageBody(NodeInstance const & instance, wstring const & ringName)
        : instance_(instance), ringName_(ringName)
    {
    }

    NodeInstance get_Instance() const { return this->instance_; }
    wstring GetRingName() const { return ringName_; }

    FABRIC_FIELDS_02(instance_, ringName_);

private:
    NodeInstance instance_;
    wstring ringName_;
};

wstring GetNodeIdWithRing(wstring const & id, wstring const & ringName)
{
    if (ringName.empty())
    {
        return id;
    }

    return id + L"@" + ringName;
}

wstring GetNodeIdWithRing(PartnerNodeSPtr const & node)
{
    return GetNodeIdWithRing(node->IdString, node->RingName);
}

TestNode::TestNode(FederationTestDispatcher & dispatcher, TestFederation & federation, NodeConfig const & nodeConfig, FabricCodeVersion const & codeVersion, wstring const & fd, Store::IStoreFactorySPtr const & storeFactory, bool checkForLeak, bool expectJoinError)
    : siteNodePtr_(),
    dispatcher_(dispatcher),
    federation_(federation),
    nodeConfig_(nodeConfig),
    retryCount_(0),
    isAborted_(false),
    checkForLeak_(checkForLeak),
    expectJoinError_(expectJoinError)
{
    if (FederationTestDispatcher::SecurityProvider == SecurityProvider::Ssl)
    {
#ifdef PLATFORM_UNIX
        extern Global<InstallTestCertInScope> testCert;
        auto certThumbPrint = testCert->Thumbprint()->PrimaryToString(); 

        auto error = SecuritySettings::FromConfiguration(
            L"X509",
            X509Default::StoreName(),
            wformatString(X509Default::StoreLocation()),
            wformatString(X509FindType::FindByThumbprint),
            certThumbPrint,
            L"",
            SecurityConfig::GetConfig().ClusterProtectionLevel,
            certThumbPrint,
            SecurityConfig::X509NameMap(),
            SecurityConfig::IssuerStoreKeyValueMap(),
            L"",
            L"",
            L"",
            L"",
            securitySettings_);
#else
        SecurityConfig::X509NameMap clusterX509Names;
        auto error = clusterX509Names.Add(
            L"WinFabricRing.Rings.WinFabricTestDomain.com",
            L"b3 44 9b 01 8d 0f 68 39 a2 c5 d6 2b 5b 6c 6a c8 22 b6 f6 62");

        KInvariant(error.IsSuccess());

        error = SecuritySettings::FromConfiguration(
            L"X509",
            X509Default::StoreName(),
            wformatString(X509Default::StoreLocation()),
            wformatString(X509FindType::FindBySubjectName),
            L"CN=NoSuchNameShouldExistOrWeCannotTestSecondary",
            L"CN=WinFabric-Test-SAN1-Alice",
            SecurityConfig::GetConfig().ClusterProtectionLevel,
            L"",
            clusterX509Names,
            SecurityConfig::IssuerStoreKeyValueMap(),
            L"",
            L"",
            L"",
            L"",
            securitySettings_);
#endif

        TestSession::FailTestIfNot(error.IsSuccess(), "Invalid security settings: {0}", error);
    }
    else if (SecurityProvider::IsWindowsProvider(FederationTestDispatcher::SecurityProvider))
    {
        ErrorCode error = SecuritySettings::FromConfiguration(
            L"Windows",
            L"",
            L"",
            L"",
            L"",
            L"",
            SecurityConfig::GetConfig().ClusterProtectionLevel,
            L"",
            SecurityConfig::X509NameMap(),
            SecurityConfig::IssuerStoreKeyValueMap(),
            L"",
            L"",
            SecurityConfig::GetConfig().ClusterSpn,
            SecurityConfig::GetConfig().ClusterIdentities,
            securitySettings_);

        TestSession::FailTestIfNot(error.IsSuccess(), "Invalid security settings: {0}", error);
    }

    Uri fdUri;
    Uri::TryParse(L"fd:/" + fd, fdUri);
    siteNodePtr_ = SiteNode::Create(nodeConfig, codeVersion, storeFactory, fdUri, this->securitySettings_);
    TestSession::WriteInfo(TraceSource, "creating test node {0}", nodeConfig);
}

TestNode::~TestNode()
{
    TestSession::WriteInfo(TraceSource, "Destructing test node {0}", siteNodePtr_->Id);
    TestSession::FailTestIf(
        checkForLeak_ && siteNodePtr_.use_count() != 1,
        "SiteNode shared pointer still has {0} references.",
        siteNodePtr_.use_count());
}

int TestNode::GetLeaseAgentPort() const
{
    size_t index = nodeConfig_.LeaseAgentAddress.find_last_of(L':');
    return Int32_Parse<wstring>(nodeConfig_.LeaseAgentAddress.substr(index + 1));
}

void TestNode::Open(TimeSpan openTimeout, bool retryOpen, Common::EventHandler handler)
{
    FEDERATIONSESSION.Expect("Node {0} successfully opened", GetNodeIdWithRing(siteNodePtr_));

    ManualResetEvent openEvent(false);

    SiteNodeSPtr localSiteNodePtr = siteNodePtr_;

    siteNodePtr_->RegisterNodeFailedEvent(handler);

    TestNodeSPtr testNode = shared_from_this();
    siteNodePtr_->RegisterMessageHandler(TestActor,
        [this, testNode](MessageUPtr & message, OneWayReceiverContextUPtr & oneWayReceiverContext){ OneWayHandler(*message, *oneWayReceiverContext); },
        [this, testNode](MessageUPtr & message, RequestReceiverContextUPtr & requestReceiverContext){ RequestReplyHandler(*message, *requestReceiverContext); },
        true/*dispatchOnTransportThread*/);

    siteNodePtr_->BeginOpen(
        openTimeout,
        [this, &openEvent, retryOpen, testNode](AsyncOperationSPtr const& asyncOperation) -> void
    {
        OnOpenComplete(asyncOperation, retryOpen);
    },
        AsyncOperationRoot<SiteNodeSPtr>::Create(siteNodePtr_));
}

void TestNode::OnOpenComplete(AsyncOperationSPtr const & asyncOperation, bool retryOpen)
{
    ErrorCode error = siteNodePtr_->EndOpen(asyncOperation); 
    TestNodeSPtr testNode = shared_from_this();
    if (error.IsSuccess())
    {
        if (expectJoinError_)
        {
            TestSession::FailTest("Unexpected join success for node {0}", siteNodePtr_->Id);
        }
        else
        {
            FEDERATIONSESSION.Validate("Node {0} successfully opened", GetNodeIdWithRing(siteNodePtr_));
        }
    }
    else if(error.IsError(ErrorCodeValue::Timeout) && retryOpen && retryCount_ < TestNode::MaxRetryCount)
    {
        TestSession::WriteError(TraceSource, "Open for node {0} completed with {1}, so retrying", siteNodePtr_->Id, error);

        FabricCodeVersion codeVersion = siteNodePtr_->CodeVersion;
        siteNodePtr_->Abort();
        siteNodePtr_.reset();
        siteNodePtr_ = SiteNode::Create(nodeConfig_, codeVersion, nullptr, Common::Uri(), this->securitySettings_);

        siteNodePtr_->RegisterNodeFailedEvent( [this, testNode](EventArgs const &)
        {
            TestSession::FailTest("Neighborhood lost for node {0}", siteNodePtr_->Id);
        });

        retryCount_++;
        siteNodePtr_->BeginOpen(
            TimeSpan::FromSeconds(15), 
            [this, testNode](AsyncOperationSPtr const& asyncOperation) -> void
        {
            OnOpenComplete(asyncOperation, true);
        },
            AsyncOperationRoot<SiteNodeSPtr>::Create(siteNodePtr_));
    }
    else
    {
        if (expectJoinError_)
        {
            dispatcher_.Federation.RemoveNode(siteNodePtr_->Id);
        }

        if (!isAborted_ && !expectJoinError_)
        {
            TestSession::WriteError(TraceSource, "Open for node {0} completed with  ErrorCodeValue = {1}", 
                siteNodePtr_->Id, 
                error.ReadValue());
        }
        else
        {
            FEDERATIONSESSION.Validate("Node {0} successfully opened", GetNodeIdWithRing(siteNodePtr_));
            TestSession::WriteInfo(TraceSource, "Ignoring open completion for aborted node {0}", siteNodePtr_->Instance); 
        }
    }
}

void TestNode::Close()
{
    ManualResetEvent closeCompleteEvent;
    ErrorCode error;
    FEDERATIONSESSION.Expect("Node {0} successfully closed", GetNodeIdWithRing(siteNodePtr_));

    siteNodePtr_->UnRegisterMessageHandler(TestActor);

    //Passing closeCompleteEvent & error by reference since Close is sync for now. Once it is Async this will be changed
    TestSession::WriteNoise(
        TraceSource,
        "Close async op for node {0} is starting...",
        siteNodePtr_->Id);

    AsyncOperationSPtr ptr = siteNodePtr_->BeginClose(TimeSpan::FromSeconds(5), 
        [this, &closeCompleteEvent, &error](AsyncOperationSPtr const& operation) -> void
    {
        TestSession::WriteNoise(
            TraceSource,
            "Close async op for node {0} is ending...",
            siteNodePtr_->Id);
        error = siteNodePtr_->EndClose(operation);
        if(error.IsSuccess())
        {
            FEDERATIONSESSION.Validate("Node {0} successfully closed", GetNodeIdWithRing(siteNodePtr_));
        }

        closeCompleteEvent.Set();
        TestSession::WriteNoise(
            TraceSource,
            "Close async op for node {0} has ended.",
            siteNodePtr_->Id);
    }, AsyncOperationSPtr());

    closeCompleteEvent.WaitOne();

    TestSession::WriteNoise(
        TraceSource,
        "Close for node {0} has ended.",
        siteNodePtr_->Id);    

    if(!error.IsSuccess())
    {
        TestSession::FailTest("Close for node {0} failed with  ErrorCodeValue = {1}", siteNodePtr_->Id, error.ReadValue());
    }

    CheckForSiteNodeLeak();
}

void TestNode::Abort()
{
    isAborted_ = true;
    siteNodePtr_->UnRegisterMessageHandler(TestActor);
    siteNodePtr_->Abort();

    CheckForSiteNodeLeak();
}

void TestNode::CheckForSiteNodeLeak()
{
    if(checkForLeak_ && siteNodePtr_.use_count() != 1)
    {
        TestSession::WriteWarning(TraceSource, "SiteNode {0} ref count after close = {1}",siteNodePtr_->Id, siteNodePtr_.use_count());
        //Sleep for 15 seconds for pending work to complete before checking again
        Sleep(20000);

        TestSession::FailTestIf(
            siteNodePtr_.use_count() != 1,
            "SiteNode shared pointer still has {0} references.",
            siteNodePtr_.use_count());
    }
}

void TestNode::Consider(FederationPartnerNodeHeader const& partnerNodeHeader, bool isInserting)
{
    siteNodePtr_->Table.Consider(partnerNodeHeader, isInserting);
}

void TestNode::GetGlobalTimeState(int64 & epoch, TimeSpan & interval, bool & isAuthority)
{
    int64 lowerLimit, upperLimit;
    epoch = siteNodePtr_->Table.GetGlobalTimeInfo(lowerLimit, upperLimit, isAuthority);
    interval = (upperLimit == DateTime::MaxValue.Ticks || lowerLimit == 0 ?  TimeSpan::MaxValue : TimeSpan::FromTicks(upperLimit - lowerLimit));
}

void TestNode::SendOneWayMessage(NodeId const& nodeIdTo, wstring const & ringName)
{
    PartnerNodeSPtr partnerNodeSPtr = siteNodePtr_->Table.Get(nodeIdTo, ringName);
    ASSERT_IF(!partnerNodeSPtr, "Node for {0}@{1} not found", nodeIdTo, ringName);

    auto oneWayMessage = make_unique<Message>();
    ActorHeader actor(TestActor);
    oneWayMessage->Headers.Add(actor);
    ActionHeader action(P2PActionName);
    oneWayMessage->Headers.Add(action);
    oneWayMessage->Idempotent = true;

    oneWayMessage->Headers.Add(MessageIdHeader(MessageId()));

    FEDERATIONSESSION.Expect("{0} one way send from {1} to {2} with Id {3}", P2PActionName, GetNodeIdWithRing(siteNodePtr_), GetNodeIdWithRing(nodeIdTo.ToString(), ringName), oneWayMessage->MessageId);

    siteNodePtr_->Send(std::move(oneWayMessage), partnerNodeSPtr);
}

void TestNode::SendRequestReplyMessage(NodeId const& nodeIdTo, TimeSpan sendTimeout, wstring const & ringName)
{
    //TODO : figure out the default timespan
    PartnerNodeSPtr partnerNodeSPtr = siteNodePtr_->Table.Get(nodeIdTo, ringName);
    ASSERT_IF(!partnerNodeSPtr, "Node for {0}@{1} not found", nodeIdTo, ringName);
    auto requestMessage = make_unique<Message>();

    ActorHeader actor(TestActor);
    requestMessage->Headers.Add(actor);
    ActionHeader action(P2PActionName);
    requestMessage->Headers.Add(action);
    requestMessage->Idempotent = true;
    MessageId messageId = MessageId();
    requestMessage->Headers.Add(MessageIdHeader(messageId));

    FEDERATIONSESSION.Expect("{0} request send from {1} to {2} with Id {3}", P2PActionName, GetNodeIdWithRing(siteNodePtr_), GetNodeIdWithRing(nodeIdTo.ToString(), ringName), requestMessage->MessageId);
    FEDERATIONSESSION.Expect("Reply sent from {0} to {1} for message with Id {2}", GetNodeIdWithRing(nodeIdTo.ToString(), ringName), GetNodeIdWithRing(siteNodePtr_), requestMessage->MessageId);


    TestNodeSPtr testNode = shared_from_this();
    auto result = siteNodePtr_->BeginSendRequest(std::move(requestMessage), partnerNodeSPtr, sendTimeout,
        [this, nodeIdTo, ringName, messageId, testNode](AsyncOperationSPtr contextSPtr) -> void
    {
        MessageUPtr reply;
        ErrorCode error = siteNodePtr_->EndSendRequest(contextSPtr, reply);

        TestSession::WriteInfo(TraceSource, "Received Reply at {0} with Success = {1}", siteNodePtr_->Id, error.IsSuccess());

        if(error.IsSuccess())
        {
            FEDERATIONSESSION.Validate("Reply sent from {0} to {1} for message with Id {2}", GetNodeIdWithRing(nodeIdTo.ToString(), ringName), GetNodeIdWithRing(siteNodePtr_), messageId);
        }
        else
        {
            TestSession::WriteError(TraceSource, "Send Reply was not a success: ErrorCodeValue = {0}", error.ReadValue());
        }

    }, AsyncOperationSPtr());
}

void TestNode::RouteOneWayMessage(NodeId const& nodeIdTo, uint64 instance, wstring const & ringName, NodeId const& closestSiteNode, TimeSpan routeTimeout, TimeSpan routeRetryTimeout, bool expectReceived, bool useExactRouting, bool idempotent, ErrorCodeValue::Enum expectedErrorCode)
{
    //TODO : figure out the default timespan
    auto requestMessage = make_unique<Message>();
    ActorHeader actor(TestActor);
    requestMessage->Headers.Add(actor);
    ActionHeader action(RoutedActionName);
    requestMessage->Headers.Add(action);
    MessageId messageId = MessageId();
    requestMessage->Headers.Add(MessageIdHeader(messageId));

    if (expectReceived)
    {
        FEDERATIONSESSION.Expect("{0} one way send from {1} to {2} with Id {3}", RoutedActionName, GetNodeIdWithRing(siteNodePtr_), GetNodeIdWithRing(closestSiteNode.ToString(), ringName), requestMessage->MessageId);
    }

    FEDERATIONSESSION.Expect("Routeone {0} completed with error of {1}", requestMessage->MessageId, expectedErrorCode);

    if (idempotent)
    {
        requestMessage->Idempotent = true;
    }

    TestNodeSPtr testNode = shared_from_this();
    auto result = siteNodePtr_->BeginRoute(std::move(requestMessage), nodeIdTo, instance, ringName, useExactRouting, routeRetryTimeout, routeTimeout,
        [this, testNode, messageId](AsyncOperationSPtr contextSPtr) -> void
    {
        ErrorCode error = siteNodePtr_->EndRoute(contextSPtr);

        TestSession::WriteInfo(TraceSource, "Route One Way completed at {0} with Success = {1}", siteNodePtr_->Id, error.IsSuccess());

        FEDERATIONSESSION.Validate("Routeone {0} completed with error of {1}", messageId, error.ReadValue());
    }, AsyncOperationSPtr());
}

void TestNode::RouteRequestReplyMessage(NodeId const& nodeIdTo, uint64 instance, wstring const & ringName, NodeId const& closestSiteNode, TimeSpan routeTimeout, TimeSpan routeRetryTimeout, bool expectReceived, bool useExactRouting, bool idempotent, ErrorCodeValue::Enum expectedErrorCode)
{
    //TODO : figure out the default timespan
    auto requestMessage = make_unique<Message>();
    ActorHeader actor(TestActor);
    requestMessage->Headers.Add(actor);
    ActionHeader action(RoutedActionName);
    requestMessage->Headers.Add(action);
    MessageId messageId = MessageId();
    requestMessage->Headers.Add(MessageIdHeader(messageId));

    if (expectReceived)
    {
        FEDERATIONSESSION.Expect("{0} request send from {1} to {2} with Id {3}", RoutedActionName, GetNodeIdWithRing(siteNodePtr_), GetNodeIdWithRing(closestSiteNode.ToString(), ringName), requestMessage->MessageId);
        FEDERATIONSESSION.Expect("Reply sent from {0} to {1} for message with Id {2}", GetNodeIdWithRing(closestSiteNode.ToString(), ringName), GetNodeIdWithRing(siteNodePtr_), requestMessage->MessageId);
    }

    FEDERATIONSESSION.Expect("Request {0} completed with error of {1}", requestMessage->MessageId, expectedErrorCode);

    SiteNodeSPtr localSiteNodePtr = siteNodePtr_;

    if (idempotent)
    {
        requestMessage->Idempotent = true;
    }

    TestNodeSPtr testNode = shared_from_this();
    auto result = siteNodePtr_->BeginRouteRequest(std::move(requestMessage), nodeIdTo, instance, ringName, useExactRouting, routeRetryTimeout, routeTimeout,
        [this, closestSiteNode, ringName, messageId, expectReceived, expectedErrorCode, testNode](AsyncOperationSPtr contextSPtr) -> void
    {
        MessageUPtr reply;
        ErrorCode error = siteNodePtr_->EndRouteRequest(contextSPtr, reply);

        TestSession::WriteInfo(TraceSource, "Received Route Reply at {0} with Success = {1}", siteNodePtr_->Id, error.IsSuccess());

        if(error.IsSuccess())
        {
            FEDERATIONSESSION.Validate("Reply sent from {0} to {1} for message with Id {2}", GetNodeIdWithRing(closestSiteNode.ToString(), ringName), GetNodeIdWithRing(siteNodePtr_), messageId);
        }
        else if(!expectReceived)
        {
            TestSession::WriteInfo(TraceSource, "Route Reply failed, as expected: ErrorCodeValue = {0}", error.ReadValue());
        }
        else
        {
            TestSession::WriteError(TraceSource, "Route Reply was not received: ErrorCodeValue = {0}", error.ReadValue());
        }

        FEDERATIONSESSION.Validate("Request {0} completed with error of {1}", messageId, error.ReadValue());

    }, AsyncOperationSPtr());
}

void TestNode::Broadcast(set<wstring> const & nodes, bool isReliable, bool toAllRings)
{
    auto oneWayMessage = make_unique<Message>();
    ActorHeader actor(TestActor);
    oneWayMessage->Headers.Add(actor);
    ActionHeader action(BroadcastedActionName);
    oneWayMessage->Headers.Add(action);

    oneWayMessage->Headers.Add(MessageIdHeader());

    MessageId id = oneWayMessage->MessageId;
    for (auto & nodeIdTo : nodes)
    {
        FEDERATIONSESSION.Expect("{0} one way send from {1} to {2} with Id {3}", BroadcastedActionName, GetNodeIdWithRing(siteNodePtr_), nodeIdTo, id);
    }

    if (isReliable)
    {
        FEDERATIONSESSION.Expect("Reliable broadcast one way from {0} with Id {1} completed", GetNodeIdWithRing(siteNodePtr_), id);

        siteNodePtr_->BeginBroadcast(std::move(oneWayMessage), toAllRings,
            [this, id](AsyncOperationSPtr operation)
            {
                ErrorCode error = this->siteNodePtr_->EndBroadcast(operation);
                if(error.IsSuccess())
                {
                    FEDERATIONSESSION.Validate("Reliable broadcast one way from {0} with Id {1} completed", GetNodeIdWithRing(siteNodePtr_), id);
                }
                else
                {
                    TestSession::WriteError(TraceSource, "Reliable broadcast one way from {0} with Id {1} failed: {2}", this->siteNodePtr_->Id, id, error);
                }
            },
            AsyncOperationSPtr());
    }
    else
    {
        siteNodePtr_->Broadcast(std::move(oneWayMessage));
    }
}

void TestNode::Multicast(
    vector<NodeInstance> const & targets,
    vector<NodeInstance> const & expectedFailedTargets,
    vector<NodeInstance> const & pendingTargets,
    TimeSpan retryTimeout, 
    TimeSpan timeout)
{
    auto oneWayMessage = make_unique<Message>();
    ActorHeader actor(TestActor);
    oneWayMessage->Headers.Add(actor);
    ActionHeader action(MulticastedActionName);
    oneWayMessage->Headers.Add(action);

    oneWayMessage->Headers.Add(MessageIdHeader());

    MessageId id = oneWayMessage->MessageId;
    for (auto & target : targets)
    {
        if (find(expectedFailedTargets.begin(), expectedFailedTargets.end(), target) == expectedFailedTargets.end() &&
            find(pendingTargets.begin(), pendingTargets.end(), target) == pendingTargets.end())
        {
			FEDERATIONSESSION.Expect("{0} one way send from {1} to {2} with Id {3}", MulticastedActionName, GetNodeIdWithRing(siteNodePtr_), GetNodeIdWithRing(target.Id.ToString(), siteNodePtr_->RingName), id);
        }
    }

    FEDERATIONSESSION.Expect("Multicast from {0} with Id {1} completed with failed nodes {2}, pending {3}", siteNodePtr_->Id, id, expectedFailedTargets, pendingTargets);

    bool expectTimeout = (pendingTargets.size() > 0);
    siteNodePtr_->BeginMulticast(std::move(oneWayMessage),
        targets,
        retryTimeout,
        timeout,
        [this, id, expectTimeout](AsyncOperationSPtr operation)
        {
            vector<NodeInstance> failed;
            vector<NodeInstance> pending;
            ErrorCode error = this->siteNodePtr_->EndMulticast(operation, failed, pending);
            if (expectTimeout ? error.IsError(ErrorCodeValue::Timeout) : error.IsSuccess())
            {
                sort(failed.begin(), failed.end());
                sort(pending.begin(), pending.end());
                FEDERATIONSESSION.Validate("Multicast from {0} with Id {1} completed with failed nodes {2}, pending {3}", this->siteNodePtr_->Id, id, failed, pending);
            }
            else
            {
                TestSession::WriteError(TraceSource, "Multicast from {0} with Id {1} failed: {2}", this->siteNodePtr_->Id, id, error);
            }
        },
        AsyncOperationSPtr());
}

void TestNode::OneWayHandler(Message & message, OneWayReceiverContext & oneWayReceiverContext)
{
    TestSession::WriteNoise(TraceSource, "Received One way message {0} id {1} at {2}",
        message.Action, message.MessageId, siteNodePtr_->Id);

    if (!messageHandlerRejectError_.IsSuccess())
    {
        TestSession::WriteInfo(TraceSource, "Rejecting One way message {0} id {1} at {2} with {3}",
            message.Action, message.MessageId, siteNodePtr_->Id, messageHandlerRejectError_);
        if (messageHandlerRejectError_.IsError(ErrorCodeValue::NotReady))
        {
            oneWayReceiverContext.Ignore();
        }
        else
        {
            oneWayReceiverContext.Reject(messageHandlerRejectError_);
        }
        return;
    }

    std::wstring finalMessage;
    Common::StringWriter sw(finalMessage);
    sw.Write("{0} one way send from {1} to {2} with Id {3}", 
        message.Action,
        GetNodeIdWithRing(oneWayReceiverContext.From->Id.ToString(), oneWayReceiverContext.From->RingName),
        GetNodeIdWithRing(siteNodePtr_),
        message.MessageId);

    bool isFirstBroadcastMessage = (message.Action == BroadcastedActionName || message.Action == MulticastedActionName) && !IsLocalDuplicate(message.MessageId);

    //If the message is a duplicate then validation of the destination has already been done and we dont 
    //need to redo
    if(!IsDuplicate(message.MessageId) || isFirstBroadcastMessage)
    {
        FEDERATIONSESSION.Validate(finalMessage, isFirstBroadcastMessage);
    }
    else
    {
        TestSession::WriteInfo(
            TraceSource,
            "Received duplicate at {0} from {1}, message Id {2}", 
            siteNodePtr_->Id,
            oneWayReceiverContext.From->Id, 
            message.MessageId);
    }

    oneWayReceiverContext.Accept();
}

void TestNode::RequestReplyHandler(Message & message, RequestReceiverContext & requestReceiverContext)
{
    TestSession::WriteNoise(TraceSource, "Received Request at {0}", siteNodePtr_->Id);

    if (!messageHandlerRejectError_.IsSuccess())
    {
        TestSession::WriteInfo(TraceSource, "Rejecting request message {0} id {1} at {2} with {3}",
            message.Action, message.MessageId, siteNodePtr_->Id, messageHandlerRejectError_);
        if (messageHandlerRejectError_.IsError(ErrorCodeValue::NotReady))
        {
            requestReceiverContext.Ignore();
        }
        else
        {
            requestReceiverContext.Reject(messageHandlerRejectError_);
        }
        return;
    }

    std::wstring finalMessage;
    Common::StringWriter sw(finalMessage);
    sw.Write("{0} request send from {1} to {2} with Id {3}", 
        message.Action,
        GetNodeIdWithRing(requestReceiverContext.From->Id.ToString(), requestReceiverContext.From->RingName),
        GetNodeIdWithRing(siteNodePtr_),
        message.MessageId);

    //If the message is a duplicate then validation of the destination has already been done and we dont 
    //need to redo. Also since only one of the replies is received at the sender we dont need to expect the
    //duplicate message's reply.  Broadcast is an exception and a 'duplicate' can happen on many nodes.
    bool isFirstBroadcastMessage = message.Action == BroadcastedActionName && !IsLocalDuplicate(message.MessageId);

    if(!IsDuplicate(message.MessageId) || isFirstBroadcastMessage)
    {
        FEDERATIONSESSION.Validate(finalMessage, isFirstBroadcastMessage);
    }
    else
    {
        TestSession::WriteInfo(
            TraceSource,
            "Received duplicate at {0} from {1}, message Id {2}", 
            siteNodePtr_->Id,
            requestReceiverContext.From->Id, 
            message.MessageId);
    }

    // reply with the instance as the body

    TestMessageBody body(siteNodePtr_->Instance, siteNodePtr_->RingName);
    auto replyMessage = make_unique<Message>(body);
    replyMessage->Headers.Add(MessageIdHeader(MessageId()));
    replyMessage->Headers.Add(ActionHeader(message.Action + L"Reply"));
    requestReceiverContext.Reply(std::move(replyMessage));
}

bool TestNode::IsDuplicate(MessageId receivedMessageId)
{
    AcquireExclusiveLock lock(DuplicateCheckListLock);
    for (MessageId const & messageId : TestNode::DuplicateCheckList)
    {
        if(receivedMessageId == messageId)
        {
            return true;
        }
    }

    TestNode::DuplicateCheckList.push_back(receivedMessageId);
    if(TestNode::DuplicateCheckList.size() > TestNode::MaxDuplicateListSize)
    {
        TestNode::DuplicateCheckList.pop_front();
    }

    return false;
}

bool TestNode::IsLocalDuplicate(MessageId receivedMessageId)
{
    AcquireExclusiveLock lock(duplicateCheckListLock_);
    for (MessageId const & messageId : duplicateCheckList_)
    {
        if(receivedMessageId == messageId)
        {
            return true;
        }
    }

    duplicateCheckList_.push_back(receivedMessageId);
    if(duplicateCheckList_.size() > TestNode::MaxDuplicateListSize)
    {
        duplicateCheckList_.pop_front();
    }

    return false;
}

class TestNode::BroadcastReplyReceiver : public enable_shared_from_this<BroadcastReplyReceiver>
{
public:

    explicit BroadcastReplyReceiver(
        IMultipleReplyContextSPtr const & context, 
        TimeSpan timeout, 
        MessageId broadcastId, 
        TestNodeSPtr const& testNode)
        : context_(context), timeoutHelper_(timeout), broadcastId_(broadcastId), testNode_(testNode)
    {
    }

    void Start()
    {
        auto parent = AsyncOperationRoot<shared_ptr<BroadcastReplyReceiver>>::Create(this->shared_from_this());
        this->InitializeReceive(parent);
    }

private:

    static void OnMessageReceived(AsyncOperationSPtr contextSPtr)
    {
        auto parent = AsyncOperationRoot<std::shared_ptr<BroadcastReplyReceiver> >::Get(contextSPtr->Parent);
        MessageUPtr reply;
        NodeInstance from;
        auto errorCode = parent->context_->EndReceive(contextSPtr, reply, from);

        if (errorCode.IsSuccess())
        {
            if (reply == nullptr)
            {
                TestSession::WriteNoise(TraceSource, "Received broadcast empty reply at {0}, broadcast complete", parent->testNode_->siteNodePtr_->Id);

                parent->context_->Close();

                FEDERATIONSESSION.Validate("Broadcast at {0} completed with Id {1}", GetNodeIdWithRing(parent->testNode_->siteNodePtr_), parent->broadcastId_);
            }
            else
            {
                TestMessageBody body;

                reply->GetBody(body);

                NodeInstance fromInstance = body.get_Instance();

                TestSession::WriteNoise(TraceSource, "Received Broadcast Reply at {0} from {1}", parent->testNode_->siteNodePtr_->Id, fromInstance.Id);

                bool validate = false;
                {
                    AcquireExclusiveLock lock(parent->setLock_);
                    auto findIter = parent->duplicateSet_.find(fromInstance.Id);

                    if (findIter == parent->duplicateSet_.end())
                    {
                        parent->duplicateSet_.insert(fromInstance.Id);
                        validate = true;
                    }
                }

                if (validate)
                {
                    FEDERATIONSESSION.Validate("Reply sent from {0} to {1} for message with Id {2}", GetNodeIdWithRing(fromInstance.Id.ToString(), body.GetRingName()), GetNodeIdWithRing(parent->testNode_->siteNodePtr_), parent->broadcastId_);
                }

                parent->InitializeReceive(contextSPtr->Parent);
            }
        }
        else
        {
            parent->context_->Close();

            TestSession::WriteError(
                TraceSource,
                "Broadcast receive for {0} failed at {1} with {2}.  Broadcast closed",
                parent->broadcastId_,
                parent->testNode_->siteNodePtr_->Id,
                errorCode.ReadValue());
        }
    }

    void InitializeReceive(AsyncOperationSPtr const & parent)
    {
        this->context_->BeginReceive(
            this->timeoutHelper_.GetRemainingTime(),
            BroadcastReplyReceiver::OnMessageReceived,
            parent);
    }

    IMultipleReplyContextSPtr context_;
    TimeoutHelper timeoutHelper_;
    TestNodeSPtr testNode_;
    MessageId broadcastId_;
    Common::ExclusiveLock setLock_;
    set<NodeId> duplicateSet_;
};

void TestNode::BroadcastRequest(set<wstring> const & nodes, Common::TimeSpan replyTimeout)
{
    auto requestMessage = make_unique<Message>();
    ActorHeader actor(TestActor);
    requestMessage->Headers.Add(actor);
    ActionHeader action(BroadcastedActionName);
    requestMessage->Headers.Add(action);

    requestMessage->Headers.Add(MessageIdHeader());

    auto broadcastId = requestMessage->MessageId;

    for (auto & nodeIdTo : nodes)
    {
        FEDERATIONSESSION.Expect("{0} request send from {1} to {2} with Id {3}", BroadcastedActionName, GetNodeIdWithRing(siteNodePtr_), nodeIdTo, requestMessage->MessageId);
        FEDERATIONSESSION.Expect("Reply sent from {0} to {1} for message with Id {2}", nodeIdTo, GetNodeIdWithRing(siteNodePtr_), requestMessage->MessageId);
    }

    FEDERATIONSESSION.Expect("Broadcast at {0} completed with Id {1}", GetNodeIdWithRing(siteNodePtr_), requestMessage->MessageId);

    auto context = siteNodePtr_->BroadcastRequest(std::move(requestMessage), replyTimeout);

    auto receiver = make_shared<BroadcastReplyReceiver>(context, TimeSpan::FromSeconds(600), broadcastId, shared_from_this());
    receiver->Start();
}

void TestNode::ReadVoterStore(wstring const & key, int64 expectedValue, int64 expectedSequence, bool expectFailure)
{
    FEDERATIONSESSION.Expect("VoterStore read on {0} key {1}", siteNodePtr_->Id, key);

    siteNodePtr_->GetVoterStore().BeginRead(
        key,
        TimeSpan::FromSeconds(60),
        [this, key, expectedValue, expectedSequence, expectFailure](AsyncOperationSPtr const& operation) -> void 
        {
            this->ReadVoterStoreCallback(operation, key, expectedValue, expectedSequence, expectFailure);
        },
        AsyncOperationRoot<SiteNodeSPtr>::Create(siteNodePtr_));
}

void TestNode::WriteVoterStore(wstring const & key, int64 value, int64 checkSequence, bool allowConflict, bool isStale, bool expectFailure)
{
    FEDERATIONSESSION.Expect("VoterStore write on {0} key {1}={2}", siteNodePtr_->Id, key, value);

    siteNodePtr_->GetVoterStore().BeginWrite(
        key,
        checkSequence,
        make_unique<VoterStoreSequenceEntry>(value),
        true,
        TimeSpan::FromSeconds(60),
        [this, key, value, checkSequence, allowConflict, isStale, expectFailure](AsyncOperationSPtr const& operation) -> void
        {
        this->WriteVoterStoreCallback(operation, key, value, checkSequence, allowConflict, isStale, expectFailure);
        },
        AsyncOperationRoot<SiteNodeSPtr>::Create(siteNodePtr_));
}

void TestNode::ReadVoterStoreCallback(AsyncOperationSPtr const & operation, wstring const & key, int64 expectedValue, int64 expectedSequence, bool expectFailure)
{
    SerializedVoterStoreEntryUPtr entry;
    int64 sequence;
    ErrorCode error = siteNodePtr_->GetVoterStore().EndRead(operation, entry, sequence);

    TestSession::FailTestIf(error.IsSuccess() == expectFailure,
        "Read from {0} with {1} unexpected result {2}",
        siteNodePtr_->Id,
        key,
        error);

    VoterStoreSequenceEntry const* value = (entry ? VoterStoreSequenceEntry::Convert(*entry) : nullptr);
    int64 entryValue = (value ? value->Value : 0);
    
    if (error.IsSuccess())
    {
        TestSession::FailTestIf(
            (expectedValue != entryValue) || (expectedSequence >= 0 && expectedSequence != sequence),
            "Read from {0} with {1} expected value {2} actual {3}, expected sequence {4} actual {5}",
            siteNodePtr_->Id,
            key,
            expectedValue,
            entryValue,
            expectedSequence,
            sequence);
    }

    TestSession::WriteInfo(
        TraceSource,
        "Read from {0} {1}={2} sequence {3} result {4}",
        siteNodePtr_->Id,
        key,
        entryValue,
        sequence,
        error);

    FEDERATIONSESSION.Validate("VoterStore read on {0} key {1}", siteNodePtr_->Id, key);
}

void TestNode::WriteVoterStoreCallback(AsyncOperationSPtr const & operation, wstring const & key, int64 value, int64 checkSequence, bool allowConflict, bool isStale, bool expectFailure)
{
    SerializedVoterStoreEntryUPtr entry;
    int64 sequence;
    ErrorCode error = siteNodePtr_->GetVoterStore().EndWrite(operation, entry, sequence);

    TestSession::WriteInfo(
        TraceSource,
        "Write from {0} {1}={2} check seq {3} result {4} sequence {5}",
        siteNodePtr_->Id,
        key,
        value,
        checkSequence,
        error,
        sequence);

    bool result;
    if (error.IsError(ErrorCodeValue::StoreWriteConflict) && VoterStoreSequenceEntry::Convert(*entry)->Value == value)
    {
        error = ErrorCodeValue::Success;
    }

    if (error.IsSuccess() || error.IsError(ErrorCodeValue::AlreadyExists))
    {
        federation_.OnStoreWriteCompleted(key, value, sequence);
        result = (!isStale && !expectFailure);
    }
    else if (error.IsError(ErrorCodeValue::StaleRequest))
    {
        result = (isStale || checkSequence < 0);
    }
    else if (error.IsError(ErrorCodeValue::StoreWriteConflict))
    {
        result = (allowConflict && checkSequence >= 0 && sequence > checkSequence);
    }
    else
    {
        result = expectFailure;
    }

    TestSession::FailTestIf(!result,
        "Unexpected result {0} isStale {1}",
        error, isStale);

    FEDERATIONSESSION.Validate("VoterStore write on {0} key {1}={2}", siteNodePtr_->Id, key, value);
}

wstring TestNode::GetState(vector<wstring> const & params)
{
	if (params[0] == L"Phase")
	{
		return wformatString(SiteNodePtr->Phase);
	}
	else if(params[0] == L"gt")
	{
		int64 epoch;
		TimeSpan interval;
		bool isAuthority;
		GetGlobalTimeState(epoch, interval, isAuthority);

		return wformatString(epoch);
	}
	else if (params[0] == L"store")
	{
        wstring result = wformatString(SiteNodePtr->GetVoterStore());
        if (params.size() > 1 && params[1] == L"phase")
        {
            return result.substr(0, 1);
        }

        return result;
    }

	return wstring();
}
