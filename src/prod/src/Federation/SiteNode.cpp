// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Federation;
using namespace std;
using namespace Transport;
using namespace Common;
using namespace LeaseWrapper;

StringLiteral const TraceState("State");
StringLiteral const TraceChannel("Channel");
StringLiteral const TraceLivenessQuery("LivenessQuery");

class SiteNode::OpenSiteNodeAsyncOperation : public TimedAsyncOperation
{
    DENY_COPY(OpenSiteNodeAsyncOperation)

public:
    OpenSiteNodeAsyncOperation(
        SiteNode & siteNode,
        TimeSpan timeout, 
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & asyncOperationParent)
        : 
    siteNode_(siteNode),
        timeoutHelper_(timeout),
        TimedAsyncOperation(timeout, callback, asyncOperationParent)
    {
    }

    static void StartJoining(AsyncOperationSPtr const& operation)
    {
        auto thisPtr = AsyncOperation::Get<OpenSiteNodeAsyncOperation>(operation);
        thisPtr->siteNode_.joinManagerUPtr_->Start(thisPtr->timeoutHelper_.GetRemainingTime());
    }

    static ErrorCode End(AsyncOperationSPtr const& operation)
    {
        auto thisPtr = AsyncOperation::End<OpenSiteNodeAsyncOperation>(operation);
        ErrorCode result = thisPtr->Error;

        WriteInfo(TraceState, thisPtr->siteNode_.IdString,
            "Open Completed for {0}: {1}",
            thisPtr->siteNode_.Instance, result);

        return result;
    }

protected:

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        SiteNodeSPtr siteNode = this->siteNode_.GetSiteNodeSPtr();
        
        auto error = this->siteNode_.Initialize(siteNode);
        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, error);
            return;
        }

        TimedAsyncOperation::OnStart(thisSPtr);

        // all messages now pass through unreliable transport.
        // Which will be bypassed in case there is no unreliable transport behavior specified.
        this->siteNode_.channelSPtr_ = DatagramTransportFactory::CreateUnreliable(this->siteNode_, this->siteNode_.channelSPtr_);
        this->siteNode_.channelSPtr_->SetConnectionOpenTimeout(FederationConfig::GetConfig().ConnectionOpenTimeout);
        this->siteNode_.channelSPtr_->SetMessageErrorChecking(FederationConfig::GetConfig().MessageErrorCheckingEnabled);
		
        WriteInfo(TraceChannel, "Node {0} created channel at {1}", this->siteNode_.Id, this->siteNode_.Address);

        TryOpenLeaseAgent(thisSPtr);

    }

    void TryOpenLeaseAgent(AsyncOperationSPtr const & thisSPtr)
    {
        SiteNodeSPtr siteNode = this->siteNode_.GetSiteNodeSPtr();
        FederationConfig & config = FederationConfig::GetConfig();

        {
            AcquireExclusiveLock lock(this->siteNode_.leaseAgentOpenLock_);

            if (false == this->siteNode_.isAborted  && !this->siteNode_.leaseAgentUPtr_)
            {
                this->siteNode_.leaseAgentUPtr_ = make_unique<LeaseAgent>(
                    this->siteNode_,
                    this->siteNode_,
                    LeaseAgentConfiguration(
                        this->siteNode_.Instance.ToString(),
                        this->siteNode_.LeaseAgentAddress,
                        true),
                    this->siteNode_.securitySettings_
                    );
            }

        }

        Common::ErrorCode errorCode = Common::ErrorCodeValue::RegisterWithLeaseDriverFailed;
        if (this->siteNode_.leaseAgentUPtr_ && false == this->siteNode_.isAborted)
        {
            errorCode = this->siteNode_.leaseAgentUPtr_->Open();
        }

        if (!errorCode.IsSuccess())
        {
            if (errorCode.ToHResult() == HRESULT_FROM_WIN32(ERROR_RETRY) && this->timeoutHelper_.GetRemainingTime() > config.LeaseDuration)
            {
                WriteWarning(TraceState, this->siteNode_.IdString, "Open SiteNode {0} with error code {1}, retry after {2}s, remaining time {3}s ",
                    this->siteNode_.Instance, errorCode.ReadValue(),
                    config.LeaseDuration,
                    this->timeoutHelper_.GetRemainingTime());

                {
                    AcquireExclusiveLock lock(this->siteNode_.leaseAgentOpenLock_);
                    this->siteNode_.leaseAgentUPtr_ = nullptr;
                }

                Threadpool::Post(
                    [this, siteNode, thisSPtr] () { this->TryOpenLeaseAgent(thisSPtr); },
                    config.LeaseDuration);
            }
            else
            {
                //return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(ErrorCodeValue::RegisterWithLeaseDriverFailed, callback, parent);;
                this->TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::RegisterWithLeaseDriverFailed));
            }

            return;
        }

        this->siteNode_.SetLeaseAgentInstanceId(this->siteNode_.leaseAgentUPtr_->InstanceId);

        siteNode_.SetTarget(siteNode_.ResolveTarget(siteNode_.Address, siteNode_.Id.ToString(), siteNode_.Instance.InstanceId));

        auto tokenOrNeighborhoodChangedEvent = [siteNode](EventArgs const &)
        {
            // TODO: If SiteNode must do any other processing on these events consider making a general method to move ProcessRoutedMessageHoldingList to
            if (siteNode)
            {
                siteNode->routingManager_->ProcessRoutedMessageHoldingList();
            }
        };

        this->siteNode_.RegisterNeighborhoodChangedEvent(tokenOrNeighborhoodChangedEvent);
        this->siteNode_.RegisterRoutingTokenChangedEvent(tokenOrNeighborhoodChangedEvent);

        if (!siteNode_.GetVoteManager().Initialize())
        {
            thisSPtr->TryComplete(thisSPtr, ErrorCodeValue::VoteStoreAccessError);
        }

        this->siteNode_.multicastChannelUPtr_ = make_unique<MulticastDatagramSender>(this->siteNode_.channelSPtr_);

        this->siteNode_.channelSPtr_->SetMessageHandler([siteNode](MessageUPtr & message, ISendTarget::SPtr const & sender) -> void
        {
            // Note: The transport now has a shared_ptr to the SiteNode, this means that SiteNode MUST remove this handler from the transport before closing
            siteNode->pointToPointManager_->ProcessIncomingTransportMessages(message, sender);
        }
        );

        this->siteNode_.RegisterMessageHandler(FederationMessage::Actor,
            [siteNode](MessageUPtr & message, OneWayReceiverContextUPtr & oneWayReceiverContext){ siteNode->FederationOneWayMessageHandler(*message, *oneWayReceiverContext); },
            [siteNode](MessageUPtr & message, RequestReceiverContextUPtr & requestReceiverContext){ siteNode->FederationRequestMessageHandler(*message, requestReceiverContext); },
            true /*dispatchOnTransportThread*/);

        errorCode = siteNode_.channelSPtr_->CompleteStart();
        if (!errorCode.IsSuccess())
        {
            thisSPtr->TryComplete(thisSPtr, errorCode);
        }

        this->siteNode_.channelSPtr_->SetConnectionFaultHandler([siteNode](ISendTarget const & target, ErrorCode lastError)
        {
            siteNode->OnConnectionFault(target, lastError);
        });

        siteNode_.GetVoteManager().Start();
    }

private:

    SiteNode & siteNode_;
    TimeoutHelper timeoutHelper_;
};

AsyncOperationSPtr SiteNode::OnBeginOpen(
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    WriteInfo(TraceState, IdString, "Opening {0} with timeout {1}", Instance, timeout);

    openOperation_ = make_shared<OpenSiteNodeAsyncOperation>(*this, timeout, callback, parent);
    openOperation_->Start(openOperation_);

    return openOperation_;
}

// There is a potential static initialization issue here.  FromSeconds looks at a const MaxValue...  Use this when we figure out a safe pattern to do so
//TimeSpan DefaultRouteTimeout = TimeSpan::FromSeconds(5);
int const MaxRoutingRetryCount = 3;

SiteNodeSPtr SiteNode::Create(
    NodeConfig const & config, 
    FabricCodeVersion const & codeVersion,
    Store::IStoreFactorySPtr const & storeFactory,
    Common::Uri const & nodeFaultDomainId,
    SecuritySettings const& securitySettings)
{
    SiteNodeSPtr siteNodeSPtr(new SiteNode(config, codeVersion, storeFactory, nodeFaultDomainId, securitySettings));
    siteNodeSPtr->InitializeInternal();

    auto votes = FederationConfig::GetConfig().Votes;
    auto vote = votes.find(siteNodeSPtr->Id, config.RingName);
    if (vote!= votes.end() && vote->Type == Constants::SeedNodeVoteType)
    {
        ASSERT_IF(
            vote->ConnectionString != siteNodeSPtr->Address, 
            "Seed node '{0}' with address '{1}' mismatches configured address '{2}'",
            siteNodeSPtr->Id, 
            siteNodeSPtr->Address, 
            vote->ConnectionString);
    }

    return siteNodeSPtr;
}

SiteNode::SiteNode(
    NodeConfig const& config,
    FabricCodeVersion const & codeVersion,
    Store::IStoreFactorySPtr const & storeFactory,
    Common::Uri const & nodeFaultDomainId,
    SecuritySettings const& securitySettings)
    : 
    PartnerNode(config.Id, RetrieveInstanceID(config.WorkingDir, config.Id), config.Address, config.LeaseAgentAddress, nodeFaultDomainId, config.RingName),
    rwJoinLockObject_(),
    statusLock_(),
    AsyncFabricComponent(),
    ILeasingApplication(true),
    workingDir_(config.WorkingDir),
    codeVersion_(codeVersion),
    securitySettings_(securitySettings),
    globalStoreUPtr_(make_unique<GlobalStore>()),
    isAborted(false),
    arbitrationFailure_(0),
    storeFactory_(storeFactory)
{
    // register all Federation message header types here for dumping.
    REGISTER_MESSAGE_HEADER(BroadcastHeader);
    REGISTER_MESSAGE_HEADER(BroadcastRangeHeader);
    REGISTER_MESSAGE_HEADER(BroadcastRelatesToHeader);
    REGISTER_MESSAGE_HEADER(BroadcastStepHeader);
    REGISTER_MESSAGE_HEADER(FabricCodeVersionHeader);
    REGISTER_MESSAGE_HEADER(FederationNeighborhoodRangeHeader);
    REGISTER_MESSAGE_HEADER(FederationNeighborhoodVersionHeader);
    REGISTER_MESSAGE_HEADER(FederationPartnerNodeHeader);
    REGISTER_MESSAGE_HEADER(FederationRoutingTokenHeader);
    REGISTER_MESSAGE_HEADER(FederationTraceProbeHeader);
    REGISTER_MESSAGE_HEADER(FederationTokenEchoHeader);
    REGISTER_MESSAGE_HEADER(PToPHeader);
    REGISTER_MESSAGE_HEADER(RoutingHeader);
    REGISTER_MESSAGE_HEADER(MulticastHeader);
    REGISTER_MESSAGE_HEADER(MulticastTargetsHeader);
    REGISTER_MESSAGE_HEADER(GlobalTimeExchangeHeader);
    REGISTER_MESSAGE_HEADER(VoterStoreHeader);
    REGISTER_MESSAGE_HEADER(JoinThrottleHeader);

    WriteInfo(TraceState, IdString, "SiteNode constructor called for {0}", Instance);

    securitySettings_.EnablePeerToPeerMode();

    leaseAgentUPtr_ = make_unique<LeaseAgent>(
        *this,
        *this,
        LeaseAgentConfiguration(
            Instance.ToString(),
            config.LeaseAgentAddress,
            true),
        securitySettings_
        );

    this->routingManager_ = unique_ptr<RoutingManager>(new RoutingManager(*this));
}

SiteNode::~SiteNode()
{
    WriteInfo(TraceState, IdString, "SiteNode destructor called for {0}", Instance);
}

void SiteNode::InitializeInternal()
{
    this->pointToPointManager_ = unique_ptr<PointToPointManager>(new PointToPointManager(*this));
}

ErrorCode SiteNode::SetSecurity(Transport::SecuritySettings const & securitySettings)
{
    auto newSettings = securitySettings;
    newSettings.EnablePeerToPeerMode();
    {
        AcquireWriteLock lock(healthClientLock_);
        securitySettings_ = newSettings; 
    }

    ErrorCode error = leaseAgentUPtr_->SetSecurity(newSettings);
    if (!error.IsSuccess())
    {
        return error;
    }

    return this->channelSPtr_->SetSecurity(newSettings);
}

Client::HealthReportingComponentSPtr SiteNode::GetHealthClient()
{
    AcquireReadLock lock(healthClientLock_);
    return healthClient_;
}

void SiteNode::SetHealthClient(Client::HealthReportingComponentSPtr const & value)
{
    AcquireWriteLock lock(healthClientLock_);
    healthClient_ = value;
}

ErrorCode SiteNode::Initialize(SiteNodeSPtr & siteNodeSPtr)
{
    channelSPtr_ = DatagramTransportFactory::CreateTcp(PartnerNode::Address, siteNodeSPtr->IdString, L"SiteNode");
    channelSPtr_->SetConnectionIdleTimeout(FederationConfig::GetConfig().ConnectionIdleTimeout);
    channelSPtr_->SetInstance(Instance.InstanceId);
    channelSPtr_->SetPerTargetSendQueueLimit(FederationConfig::GetConfig().SendQueueSizeLimit);

    auto error = channelSPtr_->SetSecurity(this->securitySettings_);
    if (!error.IsSuccess())
    {
        WriteError(TraceState, IdString, "Failed to set security on transport: {0}", error);
        return error;
    }

    auto errorCode = channelSPtr_->Start(false);

    auto listenAddress = channelSPtr_->ListenAddress();
    if (PartnerNode::Address != listenAddress)
    {
        // The transport must have chosen a new port, update our address

        WriteInfo(TraceState, IdString, 
            "Updating listen address from {0} to {1} for {2}",
            PartnerNode::Address,
            listenAddress,
            Instance);

        this->Address = listenAddress;
    }

    if (errorCode.IsSuccess())
    {
        siteNodeSPtr->routingTableUPtr_ = make_unique<RoutingTable>(siteNodeSPtr, FederationConfig::GetConfig().NeighborhoodSize);
        siteNodeSPtr->voteManagerUPtr_ = make_unique<VoteManager>(*this);
        siteNodeSPtr->joinManagerUPtr_ = make_unique<JoinManager>(*this);
        siteNodeSPtr->joinLockManagerUPtr_ = make_unique<JoinLockManager>(*this);

        siteNodeSPtr->pingManagerUPtr_ = make_unique<PingManager>(*this);
        siteNodeSPtr->updateManagerUPtr_ = make_unique<UpdateManager>(*this);
        siteNodeSPtr->broadcastManagerUPtr_ = unique_ptr<BroadcastManager>(new BroadcastManager(*this));
        siteNodeSPtr->multicastManagerUPtr_ = unique_ptr<MulticastManager>(new MulticastManager(*this));
        siteNodeSPtr->voterStoreUPtr_ = make_unique<VoterStore>(*this);
    }

    return errorCode;
}

void SiteNode::DeInitialize()
{
    if (routingTableUPtr_)
    {
        routingTableUPtr_->CloseSiteNode();
    }
}

void SiteNode::Send(MessageUPtr && message, PartnerNodeSPtr const & to) const
{
    CODING_ERROR_ASSERT(message);
    CODING_ERROR_ASSERT(to);

    this->pointToPointManager_->PToPSend(std::move(message), to, true, PToPActor::Direct);
}

shared_ptr<AsyncOperation> SiteNode::BeginSendRequest(
    MessageUPtr && request,
    PartnerNodeSPtr const & to,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent) const
{
    CODING_ERROR_ASSERT(request);
    CODING_ERROR_ASSERT(to);
    CODING_ERROR_ASSERT(timeout > TimeSpan::Zero);

    return this->pointToPointManager_->BeginPToPSendRequest(std::move(request), to, true, PToPActor::Direct, timeout, callback, parent);
}

ErrorCode SiteNode::EndSendRequest(AsyncOperationSPtr const & operation, __out MessageUPtr & message) const
{
    return this->pointToPointManager_->EndPToPSendRequest(operation, message);
}

void SiteNode::Route(
    MessageUPtr && message,
    NodeId nodeId,
    uint64 instance,
    bool useExactRouting,
    TimeSpan retryTimeout,
    TimeSpan timeout)
{
    wstring const & action = message->Action;
    BeginRoute(
        move(message), 
        nodeId, 
        instance, 
        useExactRouting, 
        retryTimeout, 
        timeout,
        [this, action] (AsyncOperationSPtr const & operation) { this->RouteCallback(operation, action); }, 
        this->CreateAsyncOperationRoot());
}

void SiteNode::RouteCallback(AsyncOperationSPtr const & operation, wstring const & action)
{
    ErrorCode error = EndRoute(operation);
    if (!error.IsSuccess())
    {
        RoutingManager::WriteWarning("Fault",
            "{0} failed to route message {1} error {2}",
            Id, action, error);
    }
}

AsyncOperationSPtr SiteNode::OnBeginRoute(
    MessageUPtr && message,
    NodeId nodeId,
    uint64 instance,
    wstring const & toRing,
    bool useExactRouting,
    TimeSpan retryTimeout,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return this->routingManager_->BeginRoute(std::move(message), nodeId, instance, toRing, useExactRouting, retryTimeout, timeout, callback, parent);
}

ErrorCode SiteNode::EndRoute(AsyncOperationSPtr const & operation)
{
    return this->routingManager_->EndRoute(operation);
}

AsyncOperationSPtr SiteNode::OnBeginRouteRequest(
    MessageUPtr && request,
    NodeId nodeId,
    uint64 instance,
    wstring const & toRing,
    bool useExactRouting,
    TimeSpan retryTimeout,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return this->routingManager_->BeginRouteRequest(std::move(request), nodeId, instance, toRing, useExactRouting, retryTimeout, timeout, callback, parent);
}

ErrorCode SiteNode::EndRouteRequest(AsyncOperationSPtr const & operation, __out MessageUPtr & reply) const
{
    return this->routingManager_->EndRouteRequest(operation, reply);
}

void SiteNode::Broadcast(MessageUPtr && message)
{
    CODING_ERROR_ASSERT(message);

    this->broadcastManagerUPtr_->Broadcast(std::move(message));
}

AsyncOperationSPtr SiteNode::BeginBroadcast(MessageUPtr && message, bool toAllRings, AsyncCallback const & callback, AsyncOperationSPtr const & parent)
{
    CODING_ERROR_ASSERT(message);

    return this->broadcastManagerUPtr_->BeginBroadcast(move(message), toAllRings, callback, parent);
}

ErrorCode SiteNode::EndBroadcast(AsyncOperationSPtr const & operation)
{
    return this->broadcastManagerUPtr_->EndBroadcast(operation);
}

IMultipleReplyContextSPtr SiteNode::BroadcastRequest(MessageUPtr && message, TimeSpan retryInterval)
{
    CODING_ERROR_ASSERT(message);
    CODING_ERROR_ASSERT(retryInterval > TimeSpan::Zero);

    return this->broadcastManagerUPtr_->BroadcastRequest(std::move(message), retryInterval);
}

AsyncOperationSPtr SiteNode::BeginMulticast(
    MessageUPtr && message,
    vector<NodeInstance> const & targets,
    TimeSpan retryTimeout,
    TimeSpan timeout,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    return this->multicastManagerUPtr_->BeginMulticast(move(message), targets, retryTimeout, timeout, callback, parent);
}

ErrorCode SiteNode::EndMulticast(
    AsyncOperationSPtr const & operation,
    __out vector<NodeInstance> & failedTargets,
    __out std::vector<NodeInstance> & pendingTargets)
{
    return this->multicastManagerUPtr_->EndMulticast(operation, failedTargets, pendingTargets);
}

IMultipleReplyContextSPtr SiteNode::MulticastRequest(Transport::MessageUPtr && message, std::vector<NodeInstance> && destinations)
{
    return MulticastReplyContext::CreateAndStart(move(message), move(destinations), *this);
}

HHandler SiteNode::RegisterRoutingTokenChangedEvent(EventHandler handler)
{
    return routingTokenChangedEvent_.Add(handler);
}

bool SiteNode::UnRegisterRoutingTokenChangeEvent(HHandler hHandler)
{
    return routingTokenChangedEvent_.Remove(hHandler);
}

HHandler SiteNode::RegisterNeighborhoodChangedEvent(EventHandler handler)
{
    return neighborhoodChangedEvent_.Add(handler);
}

bool SiteNode::UnRegisterNeighborhoodChangedEvent(HHandler hHandler)
{
    return neighborhoodChangedEvent_.Remove(hHandler);
}

HHandler SiteNode::RegisterNodeFailedEvent(EventHandler handler)
{
    return nodeFailedEvent_.Add(handler);
}

bool SiteNode::UnRegisterNodeFailedEvent(HHandler hHandler)
{
    return nodeFailedEvent_.Remove(hHandler);
}

void SiteNode::RegisterMessageHandler(
    Actor::Enum actor,
    OneWayMessageHandler const & oneWayMessageHandler,
    RequestMessageHandler const & requestMessageHandler,
    bool dispatchOnTransportThread,
    IMessageFilterSPtr && filter)
{
    this->pointToPointManager_->RegisterMessageHandler(actor, oneWayMessageHandler, requestMessageHandler, dispatchOnTransportThread, std::move(filter));
}

bool SiteNode::UnRegisterMessageHandler(Actor::Enum actor, IMessageFilterSPtr const & filter)
{
    return this->pointToPointManager_->UnRegisterMessageHandler(actor, filter);
}

bool SiteNode::CompleteOpen(ErrorCode error)
{
    bool result = false;

    if (openOperation_)
    {
        AcquireExclusiveLock grab(statusLock_);
        if (openOperation_)
        {
            shared_ptr<OpenSiteNodeAsyncOperation> operation = move(openOperation_);
            result = operation->TryStartComplete();
            if (result)
            {
                Threadpool::Post([operation, error] { operation->FinishComplete(operation, error); });
            }
        }
    }

    if (!error.IsSuccess() && result)
    {
        ChangePhase(NodePhase::Shutdown);
    }

    return result;
}

void SiteNode::CompleteOpenAsync(ErrorCode error)
{
    auto root = GetSiteNodeSPtr();
    Threadpool::Post([root, error] { root->CompleteOpen(error); });
}

void SiteNode::OnNeighborhoodChanged()
{
    SiteNodeEventSource::Events->NeighborhoodChanged(IdString);

    neighborhoodChangedEvent_.Fire(EventArgs(), true);
}

void SiteNode::OnRoutingTokenChanged()
{
    SiteNodeEventSource::Events->RoutingTokenChanged(IdString);

    if (CompleteOpen(ErrorCodeValue::Success))
    {
        pingManagerUPtr_->Start();
        updateManagerUPtr_->Start();
        voterStoreUPtr_->Start();
    }

    voteManagerUPtr_->OnRoutingTokenChanged();
    routingTokenChangedEvent_.Fire(EventArgs(), false);
}

void SiteNode::OnNeighborhoodLost()
{
    SiteNodeEventSource::Events->NeighborhoodLost(IdString, Instance.getInstanceId());

    // TODO: add node failure reason
    nodeFailedEvent_.Fire(EventArgs(), false);
    AbortSiteNodeOnFailure(ErrorCodeValue::NeighborhoodLost);
}

void SiteNode::OnLeaseFailed()
{
    PartnerNode::Phase = NodePhase::Shutdown;
    SendDepartMessages();

    SiteNodeSPtr root = GetSiteNodeSPtr();
    Threadpool::Post([root] 
    {
        SiteNodeEventSource::Events->LeaseFailed(root->IdString, root->Instance.getInstanceId());
        root->nodeFailedEvent_.Fire(EventArgs(), true);

        root->AbortSiteNodeOnFailure(ErrorCodeValue::LeaseFailed);
    }, TimeSpan::FromSeconds(1));
}

void SiteNode::AbortSiteNodeOnFailure(ErrorCodeValue::Enum errorCodeValue)
{
    ASSERT_IF(State.Value == FabricComponentState::Created, "SiteNode cannot be in created state when {0}", errorCodeValue);
    shared_ptr<OpenSiteNodeAsyncOperation> localOperation = openOperation_;
    if (localOperation)
    {
        localOperation->TryComplete(openOperation_, ErrorCode(errorCodeValue));
    }

    Abort();
}

void SiteNode::OnRemoteLeasingApplicationFailed(std::wstring const & remoteId)
{
    NodeInstance nodeInstance;
    bool isValid = NodeInstance::TryParse(remoteId, nodeInstance);
    ASSERT_IF(!isValid, "Invalid remoteId {0}", remoteId);
    Table.SetShutdown(nodeInstance, RingName);
}

void SiteNode::FederationOneWayMessageHandler(__in Message & message, OneWayReceiverContext & oneWayReceiverContext)
{
    wstring const & action = message.Action;

    if (action == FederationMessage::GetLockRequest().Action)
    {
        joinLockManagerUPtr_->LockRequestHandler(message, oneWayReceiverContext);
    }
    else if (action == FederationMessage::GetLockRenew().Action)
    {
        joinLockManagerUPtr_->LockRenewHandler(message, oneWayReceiverContext);
    }
    else if (action == FederationMessage::GetLockRelease().Action)
    {
        joinLockManagerUPtr_->LockReleaseHandler(message, oneWayReceiverContext);
    }
    else if (action == FederationMessage::GetUnlockRequest().Action)
    {
        joinLockManagerUPtr_->UnlockHandler(message, oneWayReceiverContext);
    }
    else if (action == FederationMessage::GetLockDeny().Action)
    {
        joinManagerUPtr_->LockDenyHandler(message, oneWayReceiverContext);
    }
    else if (action == FederationMessage::GetLockGrant().Action)
    {
        joinManagerUPtr_->LockGrantHandler(message, oneWayReceiverContext);
    }
    else if (action == FederationMessage::GetUnlockGrant().Action)
    {
        joinManagerUPtr_->UnLockGrantHandler(message, oneWayReceiverContext);
    }
    else if (action == FederationMessage::GetUnlockDeny().Action)
    {
        joinManagerUPtr_->UnLockDenyHandler(message, oneWayReceiverContext);
    }
    else if (action == FederationMessage::GetLockTransferRequest().Action)
    {
        joinManagerUPtr_->LockTransferRequestHandler(message, oneWayReceiverContext);
    }
    else if (action == FederationMessage::GetLockTransferReply().Action)
    {
        joinManagerUPtr_->LockTransferReplyHandler(message, oneWayReceiverContext);
    }
    else if (action == FederationMessage::GetResumeJoin().Action)
    {
        joinManagerUPtr_->ResumeJoinHandler(message, oneWayReceiverContext);
    }
    else if (action == FederationMessage::GetDepart().Action)
    {
        DepartOneWayMessageHandler(message, oneWayReceiverContext);
    }
    else if (action == FederationMessage::GetNodeDoesNotMatchFault().Action)
    {
        ProcessFederationFaultHandler(message, oneWayReceiverContext);
    }
    else if(action == FederationMessage::GetTokenRequest().Action)
    {
        routingTableUPtr_->TokenTransferRequestMessageHandler(message, oneWayReceiverContext);
    }
    else if (action == FederationMessage::GetTokenTransferAccept().Action ||
        action == FederationMessage::GetTokenTransferReject().Action)
    {
        routingTableUPtr_->TokenAcceptRejectMessageHandler(message, oneWayReceiverContext);
    }
    else if(action == FederationMessage::GetTokenTransfer().Action)
    {
        routingTableUPtr_->TokenTransferMessageHandler(message, oneWayReceiverContext);
    }
    else if(action == FederationMessage::GetTokenEcho().Action)
    {
        routingTableUPtr_->TokenEchoMessageHandler(message, oneWayReceiverContext);
    }
    else if(action == FederationMessage::GetTokenProbe().Action)
    {
        routingTableUPtr_->TokenProbeMessageHandler(message, oneWayReceiverContext);
    }
    else if (action == FederationMessage::GetEdgeProbe().Action)
    {
        routingTableUPtr_->EdgeProbeOneWayMessageHandler(message, oneWayReceiverContext);
    }
    else if (action == FederationMessage::GetRingAdjust().Action)
    {
        routingTableUPtr_->RingAdjustOneWayMessageHandler(message, oneWayReceiverContext);
    }
    else if (action == FederationMessage::GetPing().Action)
    {
        pingManagerUPtr_->MessageHandler(message, oneWayReceiverContext);
    }
    else if (action == FederationMessage::GetVotePing().Action)
    {
        voteManagerUPtr_->VotePingHandler(message, oneWayReceiverContext);
    }
    else if (action == FederationMessage::GetVotePingReply().Action)
    {
        voteManagerUPtr_->VotePingReplyHandler(message, oneWayReceiverContext);
    }
    else if (action == FederationMessage::GetVoteTransferRequest().Action)
    {
        voteManagerUPtr_->VoteTransferRequestHandler(message, oneWayReceiverContext);
    }
    else if (action == FederationMessage::GetVoteTransferReply().Action)
    {
        voteManagerUPtr_->VoteTransferReplyHandler(message, oneWayReceiverContext);
    }
	else if (action == FederationMessage::GetVoteRenewRequest().Action)
	{
		voteManagerUPtr_->VoteRenewRequestHandler(message, oneWayReceiverContext);
	}
	else if (action == FederationMessage::GetVoteRenewReply().Action)
	{
		voteManagerUPtr_->VoteRenewReplyHandler(message, oneWayReceiverContext);
	}
    else if (action == FederationMessage::GetLivenessUpdate().Action)
    {
        routingTableUPtr_->LivenessUpdateMessageHandler(message, oneWayReceiverContext);
    }
    else if (action == FederationMessage::GetExternalRingPing().Action)
    {
        Send(FederationMessage::GetExternalRingPingResponse().CreateMessage(), oneWayReceiverContext.From);
        oneWayReceiverContext.Accept();
    }
    else if (action == FederationMessage::GetExternalRingPingResponse().Action)
    {
        oneWayReceiverContext.Accept();
    }
    else if (action == FederationMessage::GetVoterStoreJoin().Action)
    {
        voterStoreUPtr_->ProcessOneWayMessage(message, oneWayReceiverContext);
    }
    else if (action == FederationMessage::GetExtendedArbitrateRequest().Action || action == FederationMessage::GetArbitrateKeepAlive().Action)
    {
        voteManagerUPtr_->ArbitrateOneWayMessageHandler(message, oneWayReceiverContext);
    }
    else if (action == FederationMessage::GetDelayedArbitrateReply().Action)
    {
        voteManagerUPtr_->DelayedArbitrateReplyHandler(message, oneWayReceiverContext);
    }
    else
    {
        WriteWarning(TraceChannel, IdString, "unknown action {0}, dropping message {1}", action, message);
    }
}

void SiteNode::FederationRequestMessageHandler(__in Message & message, RequestReceiverContextUPtr & requestReceiverContext)
{
    auto const & action = message.Action;
    if (action == FederationMessage::GetNeighborhoodQueryRequest().Action)
    {
        NeighborhoodQueryRequestHandler(message, *requestReceiverContext);
    }
    else if (action == FederationMessage::GetUpdateRequest().Action)
    {
        updateManagerUPtr_->ProcessUpdateRequestMessage(message, *requestReceiverContext);
    }
    else if (action == FederationMessage::GetArbitrateRequest().Action)
    {
        voteManagerUPtr_->ArbitrateRequestHandler(message, *requestReceiverContext);
    }
    else if (action == FederationMessage::GetExtendedArbitrateRequest().Action)
    {
        voteManagerUPtr_->ArbitrateRequestHandler(message, *requestReceiverContext);
    }
    else if (action == FederationMessage::GetTicketGapRequest().Action)
    {
        voteManagerUPtr_->TicketGapRequestHandler(message, *requestReceiverContext);
    }
    else if (action == FederationMessage::GetLivenessQueryRequest().Action)
    {
        routingTableUPtr_->LivenessQueryRequestHandler(message, *requestReceiverContext);
    }
    else if (action == FederationMessage::GetVoterStoreReadRequest().Action ||
             action == FederationMessage::GetVoterStoreWriteRequest().Action ||
             action == FederationMessage::GetVoterStoreConfigQueryRequest().Action ||
             action == FederationMessage::GetVoterStoreIntroduceRequest().Action ||
             action == FederationMessage::GetVoterStoreBootstrapRequest().Action ||
             action == FederationMessage::GetVoterStoreLeaderProbeRequest().Action ||
             action == FederationMessage::GetVoterStoreProgressRequest().Action ||
             action == FederationMessage::GetVoterStoreSyncRequest().Action)
    {
        voterStoreUPtr_->ProcessRequestMessage(message, requestReceiverContext);
    }
    else if (action == FederationMessage::GetEmptyRequest().Action)
    {
        voteManagerUPtr_->UpdateGlobalTickets(message, requestReceiverContext->FromInstance);
        requestReceiverContext->Reply(FederationMessage::GetEmptyReply().CreateMessage(voteManagerUPtr_->GenerateGlobalLease(requestReceiverContext->FromInstance), true));
    }
    else
    {
        WriteWarning(TraceChannel, IdString, "unknown action {0}, dropping message {1}", action, message);
    }
}

void SiteNode::ProcessFederationFaultHandler(__in Message & message, OneWayReceiverContext & oneWayReceiverContext)
{
    oneWayReceiverContext.Accept();

    ASSERT_IF(message.FaultErrorCodeValue == ErrorCodeValue::Success, "message was not a fault");

    if (message.FaultErrorCodeValue == ErrorCodeValue::P2PNodeDoesNotMatchFault)
    {
        NodeDoesNotMatchFaultBody body;
        if (message.GetBody<NodeDoesNotMatchFaultBody>(body))
        {
            WriteInfo(
                "Fault",
                "[{0}] Received Fault message from {1} with ErrorCodeValue of {2}. Original message was targeted to {3}.",
                this->Instance,
                oneWayReceiverContext.From->Instance,
                ErrorCodeValue::P2PNodeDoesNotMatchFault,
                body.Instance);

            routingTableUPtr_->SetShutdown(body.Instance, body.RingName);
        }
    }

    // TODO: handle other Federation faults
}

void SiteNode::NeighborhoodQueryRequestHandler(__in Message & message, RequestReceiverContext & requestReceiverContext)
{
    FabricCodeVersion version;
    FabricCodeVersionHeader::ReadFromMessage(message, version);

    if (version < *FabricCodeVersion::MinCompatible)
    {
        Trace.WriteWarning(Constants::VersionTraceType, "Rejected version {0} from {1}, min acceptable {2}",
            version, requestReceiverContext.From->Instance, *FabricCodeVersion::MinCompatible);
        requestReceiverContext.InternalReject(ErrorCodeValue::IncompatibleVersion);
        return;
    }

    NeighborhoodQueryRequestBody body;

    if (message.GetBody<NeighborhoodQueryRequestBody>(body))
    {
        TimeSpan delta = Stopwatch::Now() - body.get_Time();
        vector<VoteTicket> tickets;
        voteManagerUPtr_->GenerateGlobalLease(tickets);

        Table.ProcessGlobalTimeExchangeHeader(message, requestReceiverContext.FromWithSameInstance);

        MessageUPtr reply = FederationMessage::GetNeighborhoodQueryReply().CreateMessage(GlobalLease(move(tickets), delta));
        reply->Headers.Add(FabricCodeVersionHeader(CodeVersion));
        Table.AddGlobalTimeExchangeHeader(*reply, requestReceiverContext.From);
        joinLockManagerUPtr_->AddThrottleHeaderIfNeeded(*reply, requestReceiverContext.From->Id);

        requestReceiverContext.Reply(move(reply));
    }
    else
    {
        requestReceiverContext.InternalReject(message.FaultErrorCodeValue);
    }
}

void SiteNode::SetPhase(NodePhase::Enum phase)
{
    if (Phase != phase)
    {
        WriteInfo(TraceState, IdString,
            "{0} phase changed from {1} to {2}",
            Id, Phase, phase);
        Phase = phase;
    }
}

void SiteNode::ChangePhase(NodePhase::Enum phase)
{
    NodePhase::Enum oldPhase = Phase;

    if (!routingTableUPtr_->ChangePhase(phase))
    {
        return;
    }

    WriteInfo(TraceState, IdString,
        "{0} phase changed from {1} to {2}",
        Id, oldPhase, phase);

    AcquireExclusiveLock grab(statusLock_);
    if (phase == NodePhase::Joining && openOperation_ != nullptr)
    {
        openOperation_->StartJoining(openOperation_);
    }
    else if (Phase == NodePhase::Routing)
    {
        routingTableUPtr_->StartRouting();
	}
}

ErrorCode SiteNode::OnEndOpen(AsyncOperationSPtr const & asyncOperation)
{
    return OpenSiteNodeAsyncOperation::End(asyncOperation);
}

void SiteNode::SendDepartMessages()
{
    vector<MessageUPtr> departMessages;
    vector<PartnerNodeSPtr> neighbors;

    Table.CreateDepartMessages(departMessages, neighbors);

    for (size_t i = 0; i < departMessages.size(); i++)
    {
        this->Send(std::move(departMessages[i]), neighbors[i]);
    }
}

AsyncOperationSPtr SiteNode::OnBeginClose(
    TimeSpan,
    Common::AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    WriteInfo(TraceState, IdString, "Closing {0}", Instance);

    //TODO: Wait for pending requests to complete
    PreSendDepartMessageCleanup();

    SendDepartMessages();

    PostSendDepartMessageCleanup();

    ErrorCode error = leaseAgentUPtr_->Close();
	WriteInfo(TraceState, "Close Completed for {0}", Instance);

    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(error, callback, parent);
}

ErrorCode SiteNode::OnEndClose(AsyncOperationSPtr const & asyncOperation)
{
    return CompletedAsyncOperation::End(asyncOperation);
}

void SiteNode::OnAbort()
{
    WriteInfo(TraceState, IdString, "Aborting {0}", Instance);
    PreSendDepartMessageCleanup();
    //We do not send Depart message in Abort.
    PostSendDepartMessageCleanup();

    {
        AcquireExclusiveLock lock(leaseAgentOpenLock_);
        isAborted = true;

        if (leaseAgentUPtr_)
        {
            leaseAgentUPtr_->Abort();
        }
    }

    WriteInfo(TraceState, IdString, "Abort Completed for {0}", Instance);
}

void SiteNode::PreSendDepartMessageCleanup()
{
    SetPhase(NodePhase::Shutdown);
    //Stop receiving messages

    CompleteOpen(ErrorCodeValue::OperationCanceled);

    if (channelSPtr_)
    {
        channelSPtr_->RemoveConnectionFaultHandler();
    }

    if (voteManagerUPtr_)
    {
        voteManagerUPtr_->Stop();
    }

    if (pingManagerUPtr_)
    {
        pingManagerUPtr_->Stop();
    }

    if (updateManagerUPtr_)
    {
        updateManagerUPtr_->Stop();
    }

    if (broadcastManagerUPtr_)
    {
        broadcastManagerUPtr_->Stop();
    }

    if (multicastManagerUPtr_)
    {
        multicastManagerUPtr_->Stop();
    }

    if (joinManagerUPtr_)
    {
        joinManagerUPtr_->Stop();
    }

    if (voterStoreUPtr_)
    {
        voterStoreUPtr_->Stop();
    }

    if (globalStoreUPtr_)
    {
        globalStoreUPtr_->Close();
    }
}

void SiteNode::PostSendDepartMessageCleanup()
{
    UnRegisterMessageHandler(FederationMessage::Actor);

    DeInitialize();
    routingTokenChangedEvent_.Close();
    neighborhoodChangedEvent_.Close();
    nodeFailedEvent_.Close();

    //Cancel pending requests
    this->pointToPointManager_->Stop();
    this->routingManager_->Stop();

    // SiteNode open can fail before the transport is created.
    if(channelSPtr_)
    {
        this->channelSPtr_->Stop();
    }
}

ErrorCode SiteNode::RestartInstance()
{
    if (Phase == NodePhase::Shutdown)
    {
        return ErrorCodeValue::ObjectClosed;
    }

    NodeInstance newInstance(Id, Instance.InstanceId + 1);
    SiteNodeEventSource::Events->RestartInstance(IdString, Instance.InstanceId);

    ErrorCode error = leaseAgentUPtr_->Restart(newInstance.ToString());
    if (error.IsSuccess())
    {
        routingTableUPtr_->RestartInstance();

        channelSPtr_->SetInstance(Instance.InstanceId);
    }
    else
    {
        WriteError(TraceState, IdString,
            "{0} restart lease agent failed: {1}",
            Instance, error);
    }

    return error;
}

void SiteNode::DepartOneWayMessageHandler(__in Message & message, OneWayReceiverContext & oneWayReceiverContext)
{
    message;

    oneWayReceiverContext.Accept();
}

bool SiteNode::IsNotOpened()
{
    return this->State.Value != FabricComponentState::Opened;
}

void SiteNode::OnConnectionFault(ISendTarget const & target, ErrorCode fault)
{
    // This handler will listen for hints from the tranports about unreachable and lost addresses

    WriteInfo("ConnectionStatus", IdString, "ConnectionStatus: {0} from {1}", fault, target.Address());

    if (fault.IsError(ErrorCodeValue::CannotConnect))
    {
        if (target.Address() != this->Address)
        {
            this->routingTableUPtr_->SetUnknown(target.Address());
        }
    }
}

RoutingToken SiteNode::GetRoutingTokenAndShutdownNodes(std::vector<NodeInstance> & shutdownNodes)
{
    return Table.GetRoutingTokenAndShutdownNodes(shutdownNodes);
}

void SiteNode::OnGlobalLeaseQuorumLost()
{
    SiteNodeEventSource::Events->GlobalLeaseQuorumLost(IdString, Instance.getInstanceId());
    nodeFailedEvent_.Fire(EventArgs(), false);
    AbortSiteNodeOnFailure(ErrorCodeValue::GlobalLeaseLost);
}

bool SiteNode::IsLeaseExpired()
{
    return leaseAgentUPtr_->IsLeaseExpired;
}

int SiteNode::CountValidGlobalLeases() const
{
    vector<VoteTicket> tickets;
    voteManagerUPtr_->GenerateGlobalLease(tickets);

    return static_cast<int>(tickets.size());
}

shared_ptr<const SiteNode> SiteNode::GetSiteNodeSPtr() const
{
    shared_ptr<const ComponentRoot> comm = shared_from_this();
    return static_pointer_cast<const SiteNode>(comm);
}

void SiteNode::Arbitrate(
    LeaseAgentInstance const & local,
    TimeSpan localTTL,
    LeaseAgentInstance const & remote,
    TimeSpan remoteTTL,
    USHORT remoteVersion,
    LONGLONG monitorLeaseInstance,
    LONGLONG subjectLeaseInstance,
    LONG remoteArbitrationDurationUpperBound)
{
    UNREFERENCED_PARAMETER(localTTL);

    TimeSpan historyNeeded = TimeSpan::FromMilliseconds(remoteArbitrationDurationUpperBound) + FederationConfig::GetConfig().MaxArbitrationTimeout;
    
    ArbitrationType::Enum type;
    if (remote.InstanceId == 0)
    {
        type = ArbitrationType::KeepAlive;
    }
    else if (monitorLeaseInstance == 0)
    {
        type = ArbitrationType::OneWay;
    }
    else if ((remoteVersion >> 8) == 1)
    {
        type = ArbitrationType::TwoWaySimple;
    }
    else
    {
        type = ArbitrationType::TwoWayExtended;
    }
    
    voteManagerUPtr_->Arbitrate(
        local,
        remote,
        remoteTTL,
        historyNeeded,
        monitorLeaseInstance,
        subjectLeaseInstance,
        0,
        type);
}

void SiteNode::ReportArbitrationFailure(LeaseAgentInstance const & local, LeaseAgentInstance const & remote, int64 monitorLeaseInstance, int64 subjectLeaseInstance, std::string const & arbitrationType)
{
    if (InterlockedIncrement(&arbitrationFailure_) == 1)
    {
        ArbitrationEventSource::Events->Failure(local.Id, local.InstanceId, remote.Id, remote.InstanceId, L"LeaseFailed", monitorLeaseInstance, subjectLeaseInstance, arbitrationType);
    }
}

bool SiteNode::LivenessQuery(FederationPartnerNodeHeader const & target)
{
    bool result = routingTableUPtr_->CheckLiveness(target);
    if (result)
    {
        TimeSpan timeout = FederationConfig::GetConfig().MessageTimeout;

        AcquireExclusiveLock lock(livenesQueryTableLock_);

        StopwatchTime now = Stopwatch::Now();
        if (livenesQueryTable_.size() > 0)
        {
            for (auto it = livenesQueryTable_.begin(); it != livenesQueryTable_.end();)
            {
                if (it->second < now)
                {
                    livenesQueryTable_.erase(it++);
                }
                else
                {
                    ++it;
                }
            }
        }

        NodeId id = target.Instance.Id;
        if (livenesQueryTable_.find(id) == livenesQueryTable_.end())
        {
            NodeInstance targetInstance = target.Instance;
            WriteInfo(TraceLivenessQuery, "{0} send liveness query for {1}",
                Id, targetInstance);

            MessageUPtr message = FederationMessage::GetLivenessQueryRequest().CreateMessage(target);
            message->Idempotent = true;

            BeginRouteRequest(move(message), id, 0, false, timeout, 
                [this, targetInstance] (AsyncOperationSPtr const & operation) { this->ProcessLivenessQueryResponse(targetInstance, operation); },
                CreateAsyncOperationRoot());

            livenesQueryTable_[id] = now + timeout;
        }
    }

    return result;
}

void SiteNode::ProcessLivenessQueryResponse(NodeInstance const & targetInstance, AsyncOperationSPtr const & operation)
{
    MessageUPtr reply;
    ErrorCode error = EndRouteRequest(operation, reply);

    LivenessQueryBody body;
    if (error.IsSuccess() && reply->GetBody<LivenessQueryBody>(body))
    {
        bool result = body.get_Value();

        WriteInfo(TraceLivenessQuery, "{0} liveness query for {1} result {2}",
            Id, targetInstance, result);

        if (!result)
        {
            routingTableUPtr_->SetShutdown(targetInstance, RingName);
        }
    }
}

uint64 SiteNode::RetrieveInstanceID(std::wstring const & dir, Federation::NodeId nodeId)
{
    SiteNodeSavedInfo sni(0);          
    File file;  
    std::vector<byte> data;     
    
    std::wstring fullpath = Path::Combine(dir, nodeId.ToString() + L".sni");          
    auto error = file.TryOpen(fullpath, FileMode::OpenOrCreate, FileAccess::ReadWrite);
    if (error.IsSuccess())
    {
        int fileSize = static_cast<int>(file.size());          
        if ( fileSize>0 )
        {            
            data.resize(fileSize); 
            if (file.TryRead(reinterpret_cast<char*>(data.data()), fileSize)==fileSize)
            {
                if (!FabricSerializer::Deserialize(sni, data).IsSuccess())
                {   
                    WriteWarning(TraceState, "Failed to deserialize file content in function SiteNode::RetrieveInstanceID");
                }                
            }
            else
            {
                WriteWarning(TraceState, "Not able to read all file content in function SiteNode::RetrieveInstanceID");
            }
        }
    }
    else if ( !Directory::Exists(dir) ) 
    {
        ErrorCode ec =  Directory::Create2(dir);
        if ( ec.IsSuccess() )
        {
            file.TryOpen(fullpath, FileMode::Create, FileAccess::Write);
        }
        else
        {  
            WriteWarning(TraceState, "Return code {0} when creating folder {1} in function SiteNode::RetrieveInstanceID", ec, dir );
        }
    }
    else
    {
        WriteWarning(TraceState, "Return code {0} when OpenOrCreate file {1} in function SiteNode::RetrieveInstanceID", error, fullpath );
    }

    uint64 curTicks = DateTime::Now().Ticks;    
    sni.InstanceId = (sni.InstanceId>=curTicks) ? sni.InstanceId+1 : curTicks;
            
    if (file.IsValid())
    { 
        file.Seek(0, SeekOrigin::Begin);
        data.clear();        
        if (Common::FabricSerializer::Serialize(&sni, data).IsSuccess())
        {
            file.TryWrite(reinterpret_cast<char*>(data.data()), static_cast<ULONG>(data.size()));
        }
        else
        {
            WriteWarning(TraceState, "Failed to serialize sni in function SiteNode::RetrieveInstanceID");
        }
    }

    return sni.InstanceId;
}

ISendTarget::SPtr SiteNode::ResolveTarget(
    wstring const & address,
    wstring const & targetId,
    uint64 instance)
{
    return channelSPtr_->ResolveTarget(address, targetId, L"", instance);
}

Common::TimeSpan SiteNode::GetLeaseDuration(PartnerNode const & remotePartner, LEASE_DURATION_TYPE & durationType)
{
    FederationConfig & config = FederationConfig::GetConfig();

    if (config.LeaseDurationAcrossFaultDomain != Common::TimeSpan::Zero &&
        !NodeFaultDomainId.Segments.empty() && !remotePartner.NodeFaultDomainId.Segments.empty() &&
        NodeFaultDomainId.Segments[0].compare(remotePartner.NodeFaultDomainId.Segments[0]) != 0)
    {
        WriteInfo(TraceState, IdString, "{0} Use lease duration across fault domain {1}. Local node FD: {2}; remote node {3}, FD: {4}", 
            Instance, config.LeaseDurationAcrossFaultDomain, NodeFaultDomainId, remotePartner.IdString, remotePartner.NodeFaultDomainId);

        durationType = LEASE_DURATION_ACROSS_FAULT_DOMAIN;
        return config.LeaseDurationAcrossFaultDomain;
    }

    durationType = LEASE_DURATION;
    return config.LeaseDuration;

}

