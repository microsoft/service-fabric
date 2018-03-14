// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace LeaseWrapper
{
    class LeaseAgent;
}

namespace Federation
{
    class FederationSubsystem : public Common::AsyncFabricComponent, public std::enable_shared_from_this<FederationSubsystem>, public IRouter, public Common::RootedObject
    {
    public:
        FederationSubsystem(
            NodeConfig const& config,
            Common::FabricCodeVersion const & codeVersion,
            Store::IStoreFactorySPtr const & storeFactory,
            Common::Uri const & nodeFaultDomainId,
            Transport::SecuritySettings const& securitySettings,
            Common::ComponentRoot const & root);

        ~FederationSubsystem();

        __declspec (property(get=get_NodeId)) NodeId Id;
        NodeId get_NodeId() const;
        __declspec (property(get=getIdString)) std::wstring const & IdString;
        std::wstring const & getIdString() const;
        __declspec (property(get=get_NodeInstance)) NodeInstance const & Instance;
        NodeInstance const & get_NodeInstance() const;
        __declspec (property(get=getRingName)) std::wstring const & RingName;
        std::wstring const & getRingName() const;
        __declspec (property(get=get_Phase)) NodePhase::Enum Phase;
        NodePhase::Enum get_Phase() const;
        __declspec (property(get=get_IsRouting)) bool IsRouting;
        bool get_IsRouting() const;
        __declspec (property(get=get_IsShutdown)) bool IsShutdown;
        bool get_IsShutdown() const;
        __declspec (property(get=get_RoutingToken)) RoutingToken Token;
        RoutingToken get_RoutingToken() const;
        __declspec (property(get=get_RoutingTable)) RoutingTable & Table;
        RoutingTable & get_RoutingTable() const;
        __declspec (property(get=get_Channel)) std::shared_ptr<Transport::IDatagramTransport> const & Channel;
        std::shared_ptr<Transport::IDatagramTransport> const & get_Channel();
        __declspec (property(get=get_LeaseAgent)) LeaseWrapper::LeaseAgent const & LeaseAgent;
        LeaseWrapper::LeaseAgent const & get_LeaseAgent() const;

        void SetHealthClient(Client::HealthReportingComponentSPtr const & value);
        Client::HealthReportingComponentSPtr GetHealthClient() const;

        Common::ErrorCode SetSecurity(Transport::SecuritySettings const &);

        Common::ErrorCode EndRoute(Common::AsyncOperationSPtr const & operation);

        Common::ErrorCode EndRouteRequest(Common::AsyncOperationSPtr const & operation, __out Transport::MessageUPtr & reply) const;

        void Broadcast(Transport::MessageUPtr && message);

        IMultipleReplyContextSPtr BroadcastRequest(Transport::MessageUPtr && message, Common::TimeSpan retryInterval);

        Common::AsyncOperationSPtr BeginMulticast(
            Transport::MessageUPtr && message,
            std::vector<NodeInstance> const & targets,
            Common::TimeSpan retryTimeout,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndMulticast(
            Common::AsyncOperationSPtr const & operation,
            __out std::vector<NodeInstance> & failedTargets,
            __out std::vector<NodeInstance> & pendingTargets);

        IMultipleReplyContextSPtr MulticastRequest(Transport::MessageUPtr && message, std::vector<NodeInstance> && destinations);

        void PToPSend(Transport::MessageUPtr && message, PartnerNodeSPtr const & to) const;

        void RegisterMessageHandler(
            Transport::Actor::Enum actor,
            OneWayMessageHandler const & oneWayMessageHandler,
            RequestMessageHandler const & requestMessageHandler,
            bool dispatchOnTransportThread,
            IMessageFilterSPtr && filter = IMessageFilterSPtr());

        bool UnRegisterMessageHandler(Transport::Actor::Enum actor, IMessageFilterSPtr const & filter = IMessageFilterSPtr());

        Common::HHandler RegisterRoutingTokenChangedEvent(Common::EventHandler handler);

        bool UnRegisterRoutingTokenChangeEvent(Common::HHandler hHandler);

        Common::HHandler RegisterNodeFailedEvent(Common::EventHandler handler);

        bool UnRegisterNodeFailedEvent(Common::HHandler hHandler);

        RoutingToken GetRoutingTokenAndShutdownNodes(std::vector<NodeInstance>& shutdownNodes) const;

        void SetShutdown(NodeInstance const & nodeInstance, std::wstring const & ringName);

        int GetSeedNodes(std::vector<NodeId> & seedNodes) const;

        PartnerNodeSPtr GetNode(NodeInstance const & instance);

        void Register(GlobalStoreProviderSPtr const & provider);

        bool IsSeedNode(NodeId nodeId) const;

        int64 GetGlobalTimeInfo(int64 & lowerBound, int64 & upperBound, bool isAuthority);

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;

    protected:
        Common::AsyncOperationSPtr OnBeginOpen(
            Common::TimeSpan,
            Common::AsyncCallback const &, 
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode OnEndOpen(Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr OnBeginClose(
            Common::TimeSpan,
            Common::AsyncCallback const & , 
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode OnEndClose(Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr OnBeginRoute(
            Transport::MessageUPtr && message,
            NodeId nodeId,
            uint64 instance,
            std::wstring const & toRing,
            bool useExactRouting,
            Common::TimeSpan retryTimeout,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::AsyncOperationSPtr OnBeginRouteRequest(
            Transport::MessageUPtr && request,
            NodeId nodeId,
            uint64 instance,
            std::wstring const & toRing,
            bool useExactRouting,
            Common::TimeSpan retryTimeout,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        void OnAbort();

    private:
        // contains SideNodeSPtr for now, but it would change to not being the shared pointer, it does not need to.
        SiteNodeSPtr siteNode_;

        MUTABLE_RWLOCK(NodeCache, nodeCacheLock_);
        std::unordered_map<NodeId, PartnerNodeSPtr, NodeId::Hasher> nodeCache_;
    };
}
