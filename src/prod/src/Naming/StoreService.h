// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class StoreService;

    typedef std::function<void(std::shared_ptr<StoreService> const &)> OpenReplicaCallback;
    typedef std::function<void(SystemServices::SystemServiceLocation const &, Common::Guid const & partitionId, ::FABRIC_REPLICA_ID, ::FABRIC_REPLICA_ROLE)> ChangeRoleCallback;
    typedef std::function<void(SystemServices::SystemServiceLocation const &, std::shared_ptr<StoreService> const &)> CloseReplicaCallback;

    class StoreService : 
        public Store::KeyValueStoreReplica,
        public INamingStoreServiceRouter,
        protected Common::TextTraceComponent<Common::TraceTaskCodes::NamingStoreService>
    {
        using Common::TextTraceComponent<Common::TraceTaskCodes::NamingStoreService>::WriteNoise;
        using Common::TextTraceComponent<Common::TraceTaskCodes::NamingStoreService>::WriteInfo;
        using Common::TextTraceComponent<Common::TraceTaskCodes::NamingStoreService>::WriteWarning;
        using Common::TextTraceComponent<Common::TraceTaskCodes::NamingStoreService>::WriteError;

        DENY_COPY( StoreService );

    public:
        StoreService(
            Common::Guid const &,
            FABRIC_REPLICA_ID,
            Federation::NodeInstance const &, 
            __in Reliability::FederationWrapper &, 
            __in Reliability::ServiceAdminClient &,
            __in Reliability::ServiceResolver &,
            Common::ComponentRoot const &);

        ~StoreService();

        __declspec(property(get=get_Id)) Federation::NodeId Id;
        __declspec(property(get=get_NodeInstance)) Federation::NodeInstance Instance;
        __declspec(property(get=get_NamingServiceCuids)) std::vector<Reliability::ConsistencyUnitId> const & NamingServiceCuids;
        __declspec(property(get=get_PendingRequestCount)) LONG PendingRequestCount;
        
        Federation::NodeId get_Id() const { return propertiesUPtr_->Instance.Id; }
        Federation::NodeInstance get_NodeInstance() const { return propertiesUPtr_->Instance; }
        std::vector<Reliability::ConsistencyUnitId> const & get_NamingServiceCuids() const { return propertiesUPtr_->NamingCuidCollection.NamingServiceCuids; }
        LONG get_PendingRequestCount() const { return pendingRequestCount_.load(); }
        
        OpenReplicaCallback OnOpenReplicaCallback;
        ChangeRoleCallback OnChangeRoleCallback;
        CloseReplicaCallback OnCloseReplicaCallback;

        //
        // INamingStoreServiceRouter
        //

        Common::AsyncOperationSPtr BeginProcessRequest(
            Transport::MessageUPtr &&,
            Common::TimeSpan const,
            Common::AsyncCallback const &, 
            Common::AsyncOperationSPtr const & = Common::AsyncOperationSPtr()) override;
        Common::ErrorCode EndProcessRequest(
            Common::AsyncOperationSPtr const &,
            __out Transport::MessageUPtr &) override;

        Common::AsyncOperationSPtr BeginRequestReplyToPeer(
            Transport::MessageUPtr &&,
            Federation::NodeInstance const &,
            Transport::FabricActivityHeader const & activityHeader,
            Common::TimeSpan timeout,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) override;
        Common::ErrorCode EndRequestReplyToPeer(
            Common::AsyncOperationSPtr const &,
            __out Transport::MessageUPtr &) override;

        bool ShouldAbortProcessing() override;

        void RecoverPrimaryStartedOrCancelled(Common::AsyncOperationSPtr const &);
        bool IsPendingPrimaryRecovery();

        void OnProcessRequestFailed(Common::ErrorCode &&, Federation::RequestReceiverContext &);

    protected:

        // 
        // KeyValueStoreReplica
        //

        virtual Common::ErrorCode OnOpen(Common::ComPointer<IFabricStatefulServicePartition> const & servicePartition) override;
        virtual Common::ErrorCode OnChangeRole(::FABRIC_REPLICA_ROLE newRole, __out std::wstring & serviceLocation) override;
        virtual Common::ErrorCode OnClose() override;
        virtual void OnAbort() override;

    private:
        class ProcessRequestAsyncOperation;
        class ProcessCreateNameRequestAsyncOperation;
        class ProcessDeleteNameRequestAsyncOperation;
        class ProcessNameExistsRequestAsyncOperation;
        class ProcessInnerCreateNameRequestAsyncOperation;
        class ProcessInnerDeleteNameRequestAsyncOperation;
        class ProcessCreateServiceRequestAsyncOperation;
        class ProcessUpdateServiceRequestAsyncOperation;
        class ProcessDeleteServiceRequestAsyncOperation;
        class ProcessInnerCreateServiceRequestAsyncOperation;
        class ProcessInnerUpdateServiceRequestAsyncOperation;
        class ProcessInnerDeleteServiceRequestAsyncOperation;
        class ProcessEnumerateSubNamesRequestAsyncOperation;
        class ProcessPropertyBatchRequestAsyncOperation;
        class ProcessEnumeratePropertiesRequestAsyncOperation;
        class ProcessServiceExistsRequestAsyncOperation;
        class ProcessPrefixResolveRequestAsyncOperation;
        class ProcessInvalidRequestAsyncOperation;
        
        typedef std::function<Common::AsyncOperationSPtr(
            __in Transport::MessageUPtr &,
            __in NamingStore &,
            __in StoreServiceProperties &,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &)> ProcessRequestHandler;

        void InitializeHandlers();

        void AddHandler(
            std::map<std::wstring, ProcessRequestHandler> & temp,
            std::wstring const &, 
            ProcessRequestHandler const &);

        template <class TAsyncOperation>
        static Common::AsyncOperationSPtr CreateHandler(
            __in Transport::MessageUPtr &,
            __in NamingStore &,
            __in StoreServiceProperties &,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);

        class StoreServiceRequestJobItem;

        void InitializeMessageHandler();

        bool ValidateMessage(__in Transport::MessageUPtr &);

        Common::ErrorCode TryAcceptMessage(
            __in Transport::MessageUPtr &);

        void StartRecoveringPrimary();
        void FinishRecoveringPrimary(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        // StoreService belongs to the FabricRuntime object hierarchy so its lifetime
        // is independent of FabricNode. However, since StoreService uses FederationSubsystem, 
        // we need to take a shared pointer to the root (FabricNode) for FederationSubSystem.
        //
        Common::ComponentRootSPtr federationRoot_;

        NamingConfig const & namingConfig_;

        NamingStoreUPtr namingStoreUPtr_;
        RequestInstanceUPtr requestInstanceUPtr_;
        NameRequestInstanceTrackerUPtr authorityOwnerRequestTrackerUPtr_;
        NameRequestInstanceTrackerUPtr nameOwnerRequestTrackerUPtr_;
        NamePropertyRequestInstanceTrackerUPtr propertyRequestTrackerUPtr_;
        PerformanceCountersSPtr perfCounters_;
        PsdCache psdCache_;

        // Component used to report health on naming operations
        StoreServiceHealthMonitorUPtr healthMonitorUPtr_;

        StoreServicePropertiesUPtr propertiesUPtr_;

        Reliability::FederationWrapper & federation_;
        SystemServices::SystemServiceLocation serviceLocation_;
        SystemServices::SystemServiceMessageFilterSPtr messageFilterSPtr_;
        std::map<std::wstring, ProcessRequestHandler> requestHandlers_;

        // We need to ensure that if a name has tentative state associated with it,
        // then there is either a pending request or repair on that name. This greatly simplifies
        // the implementation of request handling since we can then be sure that the state of a name is _not_
        // tentative if the corresponding named lock is acquired.
        //
        // To ensure this, we do not accept any requests unless this replica is in the primary state
        // and all primary recovery operations have taken the necessary locks to start repair. 
        // Note that we cannot wait for recovery to finish before accepting operations since this
        // can lead to distributed deadlock.
        //
        // Conversely, we also do not start a new recovery until all pending requests have
        // completed since we look for tentative state outside the named locks when starting recovery.
        // This avoids having to check tentative twice, once outside the lock to find the state
        // and then again under the appropriate named lock.
        //
        // There can be multiple pending primary recoveries in the case of change role churn. This is
        // necessary because we may have accepted more requests during the role churn.
        //
        Common::atomic_long pendingRequestCount_;
        std::vector<RecoverPrimaryAsyncOperationSPtr> pendingPrimaryRecoveryOperations_;
        Common::RwLock recoverPrimaryLock_;

        class StoreServiceRequestJobItem;
        using NamingAsyncJobQueue = Common::AsyncWorkJobQueue<StoreServiceRequestJobItem>;
        using NamingAsyncJobQueueUPtr = std::unique_ptr<NamingAsyncJobQueue>;

        // Queue for processing AO incoming requests.
        NamingAsyncJobQueueUPtr namingAOJobQueue_;
        // Queue for processing NO incoming requests. The NO must be processed in a different queue
        // to prevent live locks when the AO work starts NO work.
        // If the same queue is used and the queue is full,
        // the NO work can't be enqueued and the AO work is blocked.
        NamingAsyncJobQueueUPtr namingNOJobQueue_;
    };
}
