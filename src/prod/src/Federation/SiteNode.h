// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class PointToPointManager;
    class BroadcastManager;
    class MulticastManager;
    class RoutingManager;
    class RoutingTable;
    class PingManager;
    class UpdateManager;
    class JoinManager;
    class JoinLockManager;
    class VoteManager;
    class VoterStore;
    class GlobalStore;

    // Structure represents information about a particular instance of a node.
    class SiteNode :
        public PartnerNode,
        public IRouter,
        public Common::AsyncFabricComponent,
        public Common::ComponentRoot,
        public LeaseWrapper::ILeasingApplication,
        public Common::TextTraceComponent<Common::TraceTaskCodes::SiteNode>
    {
        DENY_COPY(SiteNode);
        friend class VoteManagerStateMachineTests;
        friend class RoutingTableTests;
    public:
        //Default security settings means no security
        static std::shared_ptr<SiteNode> Create(
            NodeConfig const& config,
            Common::FabricCodeVersion const & codeVersion,
            Store::IStoreFactorySPtr const & storeFactory,
            Common::Uri const & nodeFaultDomainId = Common::Uri(),
            Transport::SecuritySettings const& securitySettings = Transport::SecuritySettings());

        ~SiteNode();

        // -------------------------------------------------------------------
        // Initializer for the tests.
        // 
        // Description:
        //      It is only for the CITs so that they dont need to call open.
        Common::ErrorCode Initialize(SiteNodeSPtr & siteNode);

        // -------------------------------------------------------------------
        // DeInitializer for the tests.
        // 
        // Description:
        //      It is only for the CITs so that they dont need to call close.
        void DeInitialize();

        std::shared_ptr<SiteNode> GetSiteNodeSPtr()
        {
            return std::static_pointer_cast<SiteNode>(shared_from_this());
        }

        __declspec (property(get=getWorkingDir)) std::wstring const & WorkingDir;
        __declspec (property(get=getRoutingTable)) RoutingTable & Table;
        __declspec (property(get=getCodeVersion)) Common::FabricCodeVersion const & CodeVersion;
        __declspec (property(get=getChannel)) std::shared_ptr<Transport::IDatagramTransport> const & Channel;
        __declspec (property(get=getStoreFactory)) Store::IStoreFactorySPtr const & StoreFactory;

        Client::HealthReportingComponentSPtr GetHealthClient();
        void SetHealthClient(Client::HealthReportingComponentSPtr const & value);

        Common::ErrorCode SetSecurity(Transport::SecuritySettings const &);

        void RegisterMessageHandler(
            Transport::Actor::Enum actor,
            OneWayMessageHandler const & oneWayMessageHandler,
            RequestMessageHandler const & requestMessageHandler,
            bool dispatchOnTransportThread,
            IMessageFilterSPtr && filter = IMessageFilterSPtr());

        bool UnRegisterMessageHandler(Transport::Actor::Enum actor, IMessageFilterSPtr const & filter = IMessageFilterSPtr());

        void Send(Transport::MessageUPtr && message, PartnerNodeSPtr const & to) const;

        Common::AsyncOperationSPtr BeginSendRequest(
            Transport::MessageUPtr && request,
            PartnerNodeSPtr const & to,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) const;

        Common::ErrorCode EndSendRequest(Common::AsyncOperationSPtr const & operation, __out Transport::MessageUPtr & reply) const;

        void Route(
            Transport::MessageUPtr && message,
            NodeId nodeId,
            uint64 instance,
            bool useExactRouting,
            Common::TimeSpan retryTimeout = Common::TimeSpan::FromSeconds(5),
            Common::TimeSpan timeout = Common::TimeSpan::FromSeconds(15));

        Common::ErrorCode EndRoute(Common::AsyncOperationSPtr const & operation);

        Common::ErrorCode EndRouteRequest(Common::AsyncOperationSPtr const & operation, __out Transport::MessageUPtr & reply) const;

        void Broadcast(Transport::MessageUPtr && message);

        Common::AsyncOperationSPtr BeginBroadcast(Transport::MessageUPtr && message, bool toAllRings, Common::AsyncCallback const & callback, Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndBroadcast(Common::AsyncOperationSPtr const & operation);

        IMultipleReplyContextSPtr BroadcastRequest(Transport::MessageUPtr && message, Common::TimeSpan retryInterval = Common::TimeSpan::FromSeconds(5));

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

        std::wstring const & getWorkingDir() const { return workingDir_; }

        Common::FabricCodeVersion const & getCodeVersion() const { return codeVersion_; }

        RoutingTable & getRoutingTable() const { return *routingTableUPtr_; }

        VoteManager & GetVoteManager() const { return *voteManagerUPtr_; }

        LeaseWrapper::LeaseAgent & GetLeaseAgent() const {return *leaseAgentUPtr_; }

        JoinManager & GetJoinManager() const {return *joinManagerUPtr_; }

        JoinLockManager & GetJoinLockManager() const {return *joinLockManagerUPtr_; }

        PointToPointManager & GetPointToPointManager() const {return *pointToPointManager_; }

        RoutingManager & GetRoutingManager() const {return *routingManager_; }

        VoterStore & GetVoterStore() const { return *voterStoreUPtr_; }

        GlobalStore & GetGlobalStore() const { return *globalStoreUPtr_; }

        std::shared_ptr<Transport::IDatagramTransport> const & getChannel() { return channelSPtr_; }

        Store::IStoreFactorySPtr const & getStoreFactory() const { return storeFactory_; }

        Common::HHandler RegisterRoutingTokenChangedEvent(Common::EventHandler handler);
        bool UnRegisterRoutingTokenChangeEvent(Common::HHandler hHandler);

        Common::HHandler RegisterNeighborhoodChangedEvent(Common::EventHandler handler);
        bool UnRegisterNeighborhoodChangedEvent(Common::HHandler hHandler);

        Common::HHandler RegisterNodeFailedEvent(Common::EventHandler handler);
        bool UnRegisterNodeFailedEvent(Common::HHandler hHandler);

        RoutingToken GetRoutingTokenAndShutdownNodes(std::vector<NodeInstance>& shutdownNodes);

        void OnLeaseFailed();
        void OnGlobalLeaseQuorumLost();

        bool IsLeaseExpired();
        int CountValidGlobalLeases() const;

        void ChangePhase(NodePhase::Enum phase);

        Common::ErrorCode RestartInstance();

        void OnLocalLeasingApplicationFailed();
        void OnRemoteLeasingApplicationFailed(std::wstring const & remoteId);
        void Arbitrate(
            LeaseWrapper::LeaseAgentInstance const & local, 
            Common::TimeSpan localTTL, 
            LeaseWrapper::LeaseAgentInstance const & remote, 
            Common::TimeSpan remoteTTL,
            USHORT remoteVersion, 
            LONGLONG monitorLeaseInstance,
            LONGLONG subjectLeaseInstance,
            LONG remoteArbitrationDurationUpperBound);

        bool LivenessQuery(FederationPartnerNodeHeader const & target);

        Transport::ISendTarget::SPtr ResolveTarget(
            std::wstring const & address,
            std::wstring const & targetId,
            uint64 instance);

        Common::TimeSpan GetLeaseDuration(PartnerNode const & remotePartner, LEASE_DURATION_TYPE & durationType);

        void ReportArbitrationFailure(LeaseWrapper::LeaseAgentInstance const & local, LeaseWrapper::LeaseAgentInstance const & remote, int64 monitorLeaseInstance, int64 subjectLeaseInstance, std::string const & arbitrationType);

        bool IsRingNameMatched(std::wstring const & ringName) const
        {
            return (ringName.empty() || ringName == RingName);
        }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const& options) const
        {
            PartnerNode::WriteTo(w, options);
        }

    protected:
        // TODO: these can be const & but this must start in comm object

        //AsyncFabricComponent members
        Common::AsyncOperationSPtr OnBeginOpen(
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode OnEndOpen(Common::AsyncOperationSPtr const & asyncOperation);

        Common::AsyncOperationSPtr OnBeginClose(
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode OnEndClose(Common::AsyncOperationSPtr const & asyncOperation);

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
        class OpenSiteNodeAsyncOperation;
        class CloseSiteNodeCompletedAsyncOperation;

        //---------------------------------------------------------------------
        // Constructor.
        //
        // Description:
        //      Takes the node configuration object and creates a new site node.
        //
        // Arguments:
        //      nodeInfo - the object containing the node configuration information.
        //
        SiteNode(
            NodeConfig const & nodeInfo,
            Common::FabricCodeVersion const & codeVersion,
            Store::IStoreFactorySPtr const & storeFactory,
            Common::Uri const & nodeFaultDomainId,
            Transport::SecuritySettings const& securitySettings);

        // Initializes internal objects such as the PointToPointManager
        // This must be done after the constructor has completed executing
        // and the first shared_ptr for the site node has been constructed
        void InitializeInternal();

        std::shared_ptr<const SiteNode> GetSiteNodeSPtr() const;

        void RouteCallback(Common::AsyncOperationSPtr const & operation, std::wstring const & action);

        void OpenRoutingTable(PartnerNodeSPtr const siteNode, size_t routingCapacity, int desiredHoodPerSide);
        
        void FederationOneWayMessageHandler(__in Transport::Message & message, OneWayReceiverContext & oneWayReceiverContext);
        
        void FederationRequestMessageHandler(__in Transport::Message & message, RequestReceiverContextUPtr & requestReceiverContext);

        void ProcessFederationFaultHandler(__in Transport::Message & message, OneWayReceiverContext & oneWayReceiverContext);

        void DepartOneWayMessageHandler(__in Transport::Message & message, OneWayReceiverContext & oneWayReceiverContext);

        void SimpleRequestMessageHandler(__in Transport::Message & message, RequestReceiverContext & requestReceiverContext);

        void NeighborhoodQueryRequestHandler(__in Transport::Message & message, RequestReceiverContext & requestReceiverContext);

        bool CompleteOpen(Common::ErrorCode error);
        void CompleteOpenAsync(Common::ErrorCode error);

        void SetPhase(NodePhase::Enum phase);

        void OnNeighborhoodChanged();

        void OnRoutingTokenChanged();

        void OnNeighborhoodLost();

        void OnConnectionFault(Transport::ISendTarget const & target, Common::ErrorCode fault);

        bool IsNotOpened();

        void SendDepartMessages();

        void AbortSiteNodeOnFailure(Common::ErrorCodeValue::Enum errorCodeValue);

        void ProcessLivenessQueryResponse(NodeInstance const & targetInstance, Common::AsyncOperationSPtr const & operation);

        std::shared_ptr<Transport::IDatagramTransport> channelSPtr_;
        std::unique_ptr<Transport::MulticastDatagramSender> multicastChannelUPtr_;

        std::unique_ptr<RoutingTable> routingTableUPtr_;
        std::unique_ptr<VoteManager> voteManagerUPtr_;
        std::unique_ptr<JoinManager> joinManagerUPtr_;
        std::unique_ptr<JoinLockManager> joinLockManagerUPtr_;

        std::unique_ptr<LeaseWrapper::LeaseAgent> leaseAgentUPtr_;
        std::unique_ptr<BroadcastManager> broadcastManagerUPtr_;
        std::unique_ptr<MulticastManager> multicastManagerUPtr_;

        std::unique_ptr<PingManager> pingManagerUPtr_;
        std::unique_ptr<UpdateManager> updateManagerUPtr_;

        std::unique_ptr<Federation::PointToPointManager> pointToPointManager_;
        std::unique_ptr<Federation::RoutingManager> routingManager_;

        std::unique_ptr<VoterStore> voterStoreUPtr_;
        std::unique_ptr<GlobalStore> globalStoreUPtr_;

		RWLOCK(Federation.JoinLock, rwJoinLockObject_);
		RWLOCK(Federation.Status, statusLock_);
        std::shared_ptr<OpenSiteNodeAsyncOperation> openOperation_;

        Common::Event neighborhoodChangedEvent_;
        Common::Event routingTokenChangedEvent_;
        Common::Event nodeFailedEvent_;

        std::map<NodeId, Common::StopwatchTime> livenesQueryTable_;
		RWLOCK(Federation.LivenessQueryTable, livenesQueryTableLock_);

        std::wstring workingDir_;
        Common::FabricCodeVersion codeVersion_;
        Transport::SecuritySettings securitySettings_;

		RWLOCK(Federation.LeaseAgentOpen, leaseAgentOpenLock_);
        bool isAborted;

        Client::HealthReportingComponentSPtr healthClient_;
        Common::RwLock healthClientLock_;

        volatile LONG arbitrationFailure_;

        Store::IStoreFactorySPtr storeFactory_;

        void PreSendDepartMessageCleanup();
        void PostSendDepartMessageCleanup();

        static uint64 RetrieveInstanceID(std::wstring const & dir, Federation::NodeId nodeId);

        friend class RoutingTable;
        friend class JoinManager;
        friend class JoinLockManager;
        friend class RouteAsyncOperation;
        friend class PointToPointManager;
        friend class RoutingManager;
    };
}
