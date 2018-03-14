// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Transport;
using namespace Common;
using namespace Federation;

FederationSubsystem::FederationSubsystem(
    NodeConfig const& config,
    FabricCodeVersion const & codeVersion,
    Store::IStoreFactorySPtr const & storeFactory,
    Common::Uri const & nodeFaultDomainId,
    Transport::SecuritySettings const& securitySettings,
    ComponentRoot const & root)
    : Common::RootedObject(root), siteNode_(SiteNode::Create(config, codeVersion, storeFactory, nodeFaultDomainId, securitySettings))
{
}

FederationSubsystem::~FederationSubsystem()
{
}

NodeId FederationSubsystem::get_NodeId() const
{
    return siteNode_->Instance.Id;
}

wstring const & FederationSubsystem::getIdString() const
{ 
    return siteNode_->IdString; 
}

NodeInstance const & FederationSubsystem::get_NodeInstance() const
{
    return siteNode_->Instance;
}

NodePhase::Enum FederationSubsystem::get_Phase() const 
{ 
    return siteNode_->Phase;
}

bool FederationSubsystem::get_IsRouting() const
{
    return siteNode_->IsRouting;
}

bool FederationSubsystem::get_IsShutdown() const
{
    return siteNode_->IsShutdown;
}

RoutingToken FederationSubsystem::get_RoutingToken() const 
{
    return siteNode_->Table.GetRoutingToken(); 
}

RoutingTable & FederationSubsystem::get_RoutingTable() const
{ 
    return siteNode_->Table; 
}

shared_ptr<IDatagramTransport> const & FederationSubsystem::get_Channel() 
{
    return siteNode_->Channel; 
}

LeaseWrapper::LeaseAgent const & FederationSubsystem::get_LeaseAgent() const
{
    return siteNode_->GetLeaseAgent();
}

wstring const & FederationSubsystem::getRingName() const 
{ 
    return siteNode_->RingName; 
}

void FederationSubsystem::SetHealthClient(Client::HealthReportingComponentSPtr const & value) 
{ 
    siteNode_->SetHealthClient(value);
}

Client::HealthReportingComponentSPtr FederationSubsystem::GetHealthClient() const
{
    return siteNode_->GetHealthClient();
}

ErrorCode FederationSubsystem::SetSecurity(SecuritySettings const & securitySettings)
{
    return siteNode_->SetSecurity(securitySettings);
}

AsyncOperationSPtr FederationSubsystem::OnBeginOpen(
    TimeSpan timeout,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    return siteNode_->BeginOpen(timeout, callback, parent);
}

ErrorCode FederationSubsystem::OnEndOpen(AsyncOperationSPtr const & operation)
{
    return siteNode_->EndOpen(operation);
}

AsyncOperationSPtr FederationSubsystem::OnBeginClose(
    TimeSpan timeout,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    return siteNode_->BeginClose(timeout, callback, parent);
}

ErrorCode FederationSubsystem::OnEndClose(AsyncOperationSPtr const & operation)
{
    return siteNode_->EndClose(operation);
}

void FederationSubsystem::OnAbort()
{
    siteNode_->Abort();
}

AsyncOperationSPtr FederationSubsystem::OnBeginRoute(
    Transport::MessageUPtr && message,
    NodeId nodeId,
    uint64 instance,
    wstring const & toRing,
    bool useExactRouting,
    TimeSpan retryTimeout,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return siteNode_->BeginRoute(
        std::move(message),
        nodeId,
        instance,
        toRing,
        useExactRouting,
        retryTimeout,
        timeout,
        callback,
        parent);
}

ErrorCode FederationSubsystem::EndRoute(Common::AsyncOperationSPtr const & operation)
{
    return siteNode_->EndRoute(operation);
}

AsyncOperationSPtr FederationSubsystem::OnBeginRouteRequest(
    Transport::MessageUPtr && request,
    NodeId nodeId,
    uint64 instance,
    wstring const & toRing,
    bool useExactRouting,
    TimeSpan retryTimeout,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return siteNode_->BeginRouteRequest(
        std::move(request),
        nodeId,
        instance,
        toRing,
        useExactRouting,
        retryTimeout,
        timeout,
        callback,
        parent);
}

ErrorCode FederationSubsystem::EndRouteRequest(Common::AsyncOperationSPtr const & operation, __out Transport::MessageUPtr & reply) const
{
    return siteNode_->EndRouteRequest(operation, reply);
}

void FederationSubsystem::Broadcast(Transport::MessageUPtr && message)
{
    siteNode_->Broadcast(std::move(message));
}

IMultipleReplyContextSPtr FederationSubsystem::BroadcastRequest(Transport::MessageUPtr && message, Common::TimeSpan retryInterval)
{
    return siteNode_->BroadcastRequest(std::move(message), retryInterval);
}

AsyncOperationSPtr FederationSubsystem::BeginMulticast(
    MessageUPtr && message,
    vector<NodeInstance> const & targets,
    TimeSpan retryTimeout,
    TimeSpan timeout,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    return siteNode_->BeginMulticast(move(message), targets, retryTimeout, timeout, callback, parent);
}

ErrorCode FederationSubsystem::EndMulticast(
    AsyncOperationSPtr const & operation,
    __out vector<NodeInstance> & failedTargets,
    __out std::vector<NodeInstance> & pendingTargets)
{
    return siteNode_->EndMulticast(operation, failedTargets, pendingTargets);
}

IMultipleReplyContextSPtr FederationSubsystem::MulticastRequest(Transport::MessageUPtr && message, std::vector<NodeInstance> && destinations)
{
    return siteNode_->MulticastRequest(move(message), move(destinations));
}

void FederationSubsystem::PToPSend(Transport::MessageUPtr && message, PartnerNodeSPtr const & to) const
{
    return siteNode_->Send(move(message), to);
}

void FederationSubsystem::RegisterMessageHandler(
    Actor::Enum actor,
    OneWayMessageHandler const & oneWayMessageHandler,
    RequestMessageHandler const & requestMessageHandler,
    bool dispatchOnTransportThread,
    IMessageFilterSPtr && filter)
{
    siteNode_->RegisterMessageHandler(actor, oneWayMessageHandler, requestMessageHandler, dispatchOnTransportThread, std::move(filter));
}

bool FederationSubsystem::UnRegisterMessageHandler(Actor::Enum actor, IMessageFilterSPtr const & filter)
{
    return siteNode_->UnRegisterMessageHandler(actor, filter);
}

Common::HHandler FederationSubsystem::RegisterRoutingTokenChangedEvent(Common::EventHandler handler)
{
    return siteNode_->RegisterRoutingTokenChangedEvent(handler);
}

bool FederationSubsystem::UnRegisterRoutingTokenChangeEvent(Common::HHandler hHandler)
{
    return siteNode_->UnRegisterRoutingTokenChangeEvent(hHandler);
}

Common::HHandler FederationSubsystem::RegisterNodeFailedEvent(Common::EventHandler handler)
{
    return siteNode_->RegisterNodeFailedEvent(handler);
}

bool FederationSubsystem::UnRegisterNodeFailedEvent(Common::HHandler hHandler)
{
    return siteNode_->UnRegisterNodeFailedEvent(hHandler);
}

RoutingToken FederationSubsystem::GetRoutingTokenAndShutdownNodes(std::vector<NodeInstance>& shutdownNodes) const
{
    return siteNode_->GetRoutingTokenAndShutdownNodes(shutdownNodes);
}

void FederationSubsystem::SetShutdown(NodeInstance const & nodeInstance, wstring const & ringName)
{
    siteNode_->Table.SetShutdown(nodeInstance, ringName);
}

int FederationSubsystem::GetSeedNodes(std::vector<NodeId> & seedNodes) const
{
    return siteNode_->GetVoteManager().GetSeedNodes(seedNodes);
}

void FederationSubsystem::WriteTo(TextWriter& w, FormatOptions const&) const
{
    w << *siteNode_;
}

bool FederationSubsystem::IsSeedNode(NodeId nodeId) const
{
    return VoteManager::IsSeedNode(nodeId, siteNode_->RingName); 
}

void FederationSubsystem::Register(GlobalStoreProviderSPtr const & provider)
{
    siteNode_->GetGlobalStore().Register(provider);
}

int64 FederationSubsystem::GetGlobalTimeInfo(int64 & lowerBound, int64 & upperBound, bool isAuthority)
{
    if (!siteNode_->IsRouting)
    {
        return 0;
    }

    return siteNode_->Table.GetGlobalTimeInfo(lowerBound, upperBound, isAuthority);
}

PartnerNodeSPtr FederationSubsystem::GetNode(NodeInstance const & instance)
{
    {
        AcquireReadLock grab(nodeCacheLock_);
        auto it = nodeCache_.find(instance.Id);
        if (it != nodeCache_.end() && it->second->Instance.InstanceId >= instance.InstanceId)
        {
            return it->second;
        }
    }

    PartnerNodeSPtr result = siteNode_->Table.Get(instance.Id);
    if (!result || result->Instance.InstanceId < instance.InstanceId)
    {
        return nullptr;
    }

    AcquireWriteLock grab(nodeCacheLock_);
    nodeCache_[instance.Id] = result;

    return result;
}
