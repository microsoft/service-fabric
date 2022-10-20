// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class ClusterManagerReplica;

        using OpenReplicaCallback = std::function<void(std::shared_ptr<ClusterManagerReplica> const &)>;
        using ChangeRoleCallback = std::function<void(SystemServices::SystemServiceLocation const &, std::shared_ptr<ClusterManagerReplica> const &)>;
        using CloseReplicaCallback = std::function<void(SystemServices::SystemServiceLocation const &)>;

        using NamingJobCallback = std::function<void(Common::AsyncOperationSPtr const &)>;
        typedef std::function<void()> ProcessingCallback;

        template <class R>
        class AppTypeCleanupJobQueue : public Common::DefaultJobQueue<R>
        {
            DENY_COPY(AppTypeCleanupJobQueue)
        public:
            AppTypeCleanupJobQueue(std::wstring const & name, R & root, bool forceEnqueue, int maxThreads = 0, uint64 queueSize = UINT64_MAX)
                : Common::DefaultJobQueue<R>(
                    name,
                    root,
                    forceEnqueue,
                    maxThreads,
                    nullptr,
                    queueSize),
                onFinishEvent_()
            {
            }

            __declspec(property(get = get_OperationsFinishedEvent)) Common::ManualResetEvent & OperationsFinishedAsyncEvent;
            Common::ManualResetEvent & get_OperationsFinishedEvent() { return onFinishEvent_; }

        protected:
            void OnFinishItems() override { onFinishEvent_.Set(); }

        private:
            Common::ManualResetEvent onFinishEvent_;
        };

        class ClusterManagerReplica :
            public Store::KeyValueStoreReplica,
            protected Common::TextTraceComponent<Common::TraceTaskCodes::ClusterManager>,
            private Common::RootedObject
        {
            using Common::TextTraceComponent<Common::TraceTaskCodes::ClusterManager>::WriteNoise;
            using Common::TextTraceComponent<Common::TraceTaskCodes::ClusterManager>::WriteInfo;
            using Common::TextTraceComponent<Common::TraceTaskCodes::ClusterManager>::WriteWarning;
            using Common::TextTraceComponent<Common::TraceTaskCodes::ClusterManager>::WriteError;

            DENY_COPY(ClusterManagerReplica);

        public:
            ClusterManagerReplica(
                Common::Guid const &,
                FABRIC_REPLICA_ID,
                __in Reliability::FederationWrapper &,
                __in Reliability::ServiceResolver &,
                __in IImageBuilder &,
                Api::IClientFactoryPtr const &,
                std::wstring const & workingDir,
                std::wstring const & nodeName,
                __in Common::ComponentRoot const &);

            virtual ~ClusterManagerReplica();

            __declspec(property(get=get_NodeInstance)) Federation::NodeInstance const & NodeInstance;
            Federation::NodeInstance const & get_NodeInstance() const { return federation_.Instance; }

            __declspec(property(get=get_NodeName)) std::wstring const & NodeName;
            std::wstring const & get_NodeName() const { return nodeName_; }

            __declspec(property(get=get_Federation)) Reliability::FederationWrapper & Router;
            Reliability::FederationWrapper & get_Federation() const { return federation_; }

            __declspec(property(get=get_ImageBuilder)) IImageBuilder & ImageBuilder;
            IImageBuilder & get_ImageBuilder() const { return imageBuilder_; }

            __declspec(property(get=get_Client)) FabricClientProxy const & Client;
            FabricClientProxy const & get_Client() const { return clientProxy_; }

            __declspec(property(get=get_ApplicationTypeRequestTracker)) ApplicationTypeRequestTracker & AppTypeRequestTracker;
            ApplicationTypeRequestTracker & get_ApplicationTypeRequestTracker() const { return *applicationTypeTrackerUPtr_; }

            __declspec(property(get=get_FabricRoot)) Common::ComponentRootSPtr const & FabricRoot;
            Common::ComponentRootSPtr const & get_FabricRoot() const { return fabricRoot_; }

            __declspec(property(get=get_WorkingDir)) std::wstring const & WorkingDirectory;
            std::wstring const & get_WorkingDir() const { return workingDir_; }

            __declspec(property(get=get_ContextProcessingDelay)) Common::TimeSpan const & ContextProcessingDelay;
            Common::TimeSpan const & get_ContextProcessingDelay() const { return contextProcessingDelay_; }

            __declspec(property(get=get_VolumeManager)) std::unique_ptr<VolumeManager> & VolMgr;
            std::unique_ptr<VolumeManager> & get_VolumeManager() { return volumeManagerUPtr_; }

            OpenReplicaCallback OnOpenReplicaCallback;
            ChangeRoleCallback OnChangeRoleCallback;
            CloseReplicaCallback OnCloseReplicaCallback;

            Common::ErrorCode GetHealthAggregator(__inout HealthManager::IHealthAggregatorSPtr & healthAggregator) const;

            // *********************
            // Management operations
            // *********************

            bool TryAcceptRequestByString(std::wstring const &, __in ClientRequestSPtr &);
            bool TryAcceptRequestByName(Common::NamingUri const &, __in ClientRequestSPtr &);
            bool TryAcceptClusterOperationRequest(__in ClientRequestSPtr &);

            Common::ErrorCode CheckAndDeleteUnusedApplicationTypes();
            Common::ErrorCode CheckAndDeleteUnusedApplicationTypes(std::wstring const &, Common::ActivityId const &);
            void MarkUsedAppTypeVersions(
                std::vector<ApplicationTypeContext> const &,
                std::wstring const &,
                __out std::vector<bool> &);

            Common::ErrorCode FindUsedAppTypeVersionVersion(
                std::vector<ApplicationTypeContext> const &,
                std::vector<ApplicationContext> const &,
                std::wstring const &,
                __out std::vector<bool> &);

            void OnUnprovisionAcceptComplete(
                std::shared_ptr<UnprovisionApplicationTypeDescription> const &,
                Common::ActivityId const &,
                Common::AsyncOperationSPtr const&,
                bool);

            Common::AsyncOperationSPtr BeginAcceptProvisionApplicationType(
                ProvisionApplicationTypeDescription const & body,
                ClientRequestSPtr &&,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            Common::ErrorCode EndAcceptProvisionApplicationType(Common::AsyncOperationSPtr const & op) { return EndAcceptRequest(op); }

            Common::AsyncOperationSPtr BeginAcceptCreateApplication(
                Common::NamingUri const &,
                ServiceModelTypeName const &,
                ServiceModelVersion const &,
                std::map<std::wstring, std::wstring> const &,
                Reliability::ApplicationCapacityDescription const &,
                ClientRequestSPtr &&,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            Common::ErrorCode EndAcceptCreateApplication(Common::AsyncOperationSPtr const & op) { return EndAcceptRequest(op); }

            Common::AsyncOperationSPtr BeginAcceptCreateComposeDeployment(
                std::wstring const &,
                std::wstring const &,
                std::vector<Common::ByteBuffer> &&,
                std::vector<Common::ByteBuffer> &&,
                std::wstring const & repositoryUserName,
                std::wstring const & repositoryPassword,
                bool isPasswordEncrypted,
                ClientRequestSPtr && clientRequest,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & root);
            Common::ErrorCode EndAcceptCreateComposeDeployment(Common::AsyncOperationSPtr const & operation) { return EndAcceptRequest(operation); }

            Common::AsyncOperationSPtr BeginAcceptCreateSingleInstanceApplication(
                ServiceModel::ModelV2::ApplicationDescription &&,
                ClientRequestSPtr && clientRequest,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & root);
            Common::ErrorCode EndAcceptCreateSingleInstanceApplication(Common::AsyncOperationSPtr const & operation) { return EndAcceptRequest(operation); }

            Common::AsyncOperationSPtr BeginAcceptDeleteComposeDeployment(
                std::wstring const &,
                Common::NamingUri const &,
                ClientRequestSPtr &&,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            Common::ErrorCode EndAcceptDeleteComposeDeployment(Common::AsyncOperationSPtr const & operation) { return EndAcceptRequest(operation); }

            Common::AsyncOperationSPtr BeginAcceptDeleteSingleInstanceDeployment(
                ServiceModel::DeleteSingleInstanceDeploymentDescription const &,
                ClientRequestSPtr &&,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            Common::ErrorCode EndAcceptDeleteSingleInstanceDeployment(Common::AsyncOperationSPtr const & operation) { return EndAcceptRequest(operation); }

            Common::AsyncOperationSPtr BeginAcceptUpdateApplication(
                ServiceModel::ApplicationUpdateDescriptionWrapper const & updateDescription,
                ClientRequestSPtr &&,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            Common::ErrorCode EndAcceptUpdateApplication(Common::AsyncOperationSPtr const & op) { return EndAcceptRequest(op); }

            Common::AsyncOperationSPtr BeginAcceptCreateService(
                __in Naming::PartitionedServiceDescriptor &,
                ClientRequestSPtr &&,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            Common::ErrorCode EndAcceptCreateService(Common::AsyncOperationSPtr const & op) { return EndAcceptRequest(op); }

            Common::AsyncOperationSPtr BeginAcceptCreateServiceFromTemplate(
                Common::NamingUri const & applicationName,
                Common::NamingUri const & absoluteServiceName,
                std::wstring const & serviceDnsName,
                ServiceModelTypeName const &,
                ServiceModel::ServicePackageActivationMode::Enum servicePackageActivationMode,
                std::vector<byte> &&,
                ClientRequestSPtr &&,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            Common::ErrorCode EndAcceptCreateServiceFromTemplate(Common::AsyncOperationSPtr const & op) { return EndAcceptRequest(op); }

            Common::AsyncOperationSPtr BeginAcceptUnprovisionApplicationType(
                UnprovisionApplicationTypeDescription const & description,
                ClientRequestSPtr &&,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            Common::ErrorCode EndAcceptUnprovisionApplicationType(Common::AsyncOperationSPtr const & op) { return EndAcceptRequest(op); }

            Common::AsyncOperationSPtr BeginAcceptDeleteApplication(
                DeleteApplicationMessageBody const & body,
                ClientRequestSPtr &&,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            Common::ErrorCode EndAcceptDeleteApplication(Common::AsyncOperationSPtr const & op) { return EndAcceptRequest(op); }

            Common::AsyncOperationSPtr BeginAcceptDeleteService(
                Common::NamingUri const & appName,
                Common::NamingUri const & absoluteServiceName,
                bool const isForce,
                ClientRequestSPtr &&,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            Common::ErrorCode EndAcceptDeleteService(Common::AsyncOperationSPtr const & op) { return EndAcceptRequest(op); }

            Common::AsyncOperationSPtr BeginAcceptUpgradeComposeDeployment(
                ServiceModel::ComposeDeploymentUpgradeDescription const &,
                ClientRequestSPtr &&,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            Common::ErrorCode EndAcceptUpgradeComposeDeployment(Common::AsyncOperationSPtr const & op) { return EndAcceptRequest(op); }

            Common::AsyncOperationSPtr BeginAcceptRollbackComposeDeployment(
                std::wstring const &,
                ClientRequestSPtr &&,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            Common::ErrorCode EndAcceptRollbackComposeDeployment(Common::AsyncOperationSPtr const & op) { return EndAcceptRequest(op); }

            Common::AsyncOperationSPtr BeginAcceptUpgradeApplication(
                ApplicationUpgradeDescription const &,
                ClientRequestSPtr &&,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            Common::ErrorCode EndAcceptUpgradeApplication(Common::AsyncOperationSPtr const & op) { return EndAcceptRequest(op); }

            Common::AsyncOperationSPtr BeginAcceptRollbackApplicationUpgrade(
                Common::NamingUri const & appName,
                ClientRequestSPtr &&,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            Common::ErrorCode EndAcceptRollbackApplicationUpgrade(Common::AsyncOperationSPtr const & op) { return EndAcceptRequest(op); }

            Common::AsyncOperationSPtr BeginAcceptUpdateApplicationUpgrade(
                ApplicationUpgradeUpdateDescription const &,
                ClientRequestSPtr &&,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            Common::ErrorCode EndAcceptUpdateApplicationUpgrade(Common::AsyncOperationSPtr const & op) { return EndAcceptRequest(op); }

            Common::ErrorCode GetApplicationUpgradeStatus(
                Common::NamingUri const & appName,
                ClientRequestSPtr const &,
                __out ApplicationUpgradeStatusDescription &);

            Common::AsyncOperationSPtr BeginAcceptReportUpgradeHealth(
                Common::NamingUri const & appName,
                std::vector<std::wstring> && upgradeDomains,
                uint64 upgradeInstance,
                ClientRequestSPtr &&,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            Common::ErrorCode EndAcceptReportUpgradeHealth(Common::AsyncOperationSPtr const & op) { return EndAcceptRequest(op); }

            Common::AsyncOperationSPtr BeginAcceptMoveNextUpgradeDomain(
                Common::NamingUri const & appName,
                std::wstring const & nextUpgradeDomain,
                ClientRequestSPtr &&,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            Common::ErrorCode EndAcceptMoveNextUpgradeDomain(Common::AsyncOperationSPtr const & op) { return EndAcceptRequest(op); }

            Common::AsyncOperationSPtr BeginAcceptProvisionFabric(
                std::wstring const & codeFilepath,
                std::wstring const & clusterManifestFilepath,
                ClientRequestSPtr &&,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            Common::ErrorCode EndAcceptProvisionFabric(Common::AsyncOperationSPtr const & op) { return EndAcceptRequest(op); }

            Common::AsyncOperationSPtr BeginAcceptUpgradeFabric(
                FabricUpgradeDescription const &,
                ClientRequestSPtr &&,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            Common::ErrorCode EndAcceptUpgradeFabric(Common::AsyncOperationSPtr const & op) { return EndAcceptRequest(op); }

            Common::AsyncOperationSPtr BeginAcceptRollbackFabricUpgrade(
                ClientRequestSPtr &&,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            Common::ErrorCode EndAcceptRollbackFabricUpgrade(Common::AsyncOperationSPtr const & op) { return EndAcceptRequest(op); }

            Common::AsyncOperationSPtr BeginAcceptUpdateFabricUpgrade(
                FabricUpgradeUpdateDescription const &,
                ClientRequestSPtr &&,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            Common::ErrorCode EndAcceptUpdateFabricUpgrade(Common::AsyncOperationSPtr const & op) { return EndAcceptRequest(op); }

            Common::ErrorCode GetFabricUpgradeStatus(
                ClientRequestSPtr const &,
                __out FabricUpgradeStatusDescription &);

            Common::AsyncOperationSPtr BeginAcceptReportFabricUpgradeHealth(
                std::vector<std::wstring> && upgradeDomains,
                uint64 upgradeInstance,
                ClientRequestSPtr &&,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            Common::ErrorCode EndAcceptReportFabricUpgradeHealth(Common::AsyncOperationSPtr const & op) { return EndAcceptRequest(op); }

            Common::AsyncOperationSPtr BeginAcceptMoveNextFabricUpgradeDomain(
                std::wstring const & nextUpgradeDomain,
                ClientRequestSPtr &&,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            Common::ErrorCode EndAcceptMoveNextFabricUpgradeDomain(Common::AsyncOperationSPtr const & op) { return EndAcceptRequest(op); }

            Common::AsyncOperationSPtr BeginAcceptUnprovisionFabric(
                Common::FabricVersion const &,
                ClientRequestSPtr &&,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            Common::ErrorCode EndAcceptUnprovisionFabric(Common::AsyncOperationSPtr const & op) { return EndAcceptRequest(op); }

            Common::AsyncOperationSPtr BeginAcceptStartInfrastructureTask(
                InfrastructureTaskDescription const &,
                ClientRequestSPtr &&,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            Common::ErrorCode EndAcceptStartInfrastructureTask(Common::AsyncOperationSPtr const & op) { return EndAcceptRequest(op); }

            Common::AsyncOperationSPtr BeginAcceptFinishInfrastructureTask(
                TaskInstance const &,
                ClientRequestSPtr &&,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            Common::ErrorCode EndAcceptFinishInfrastructureTask(Common::AsyncOperationSPtr const & op) { return EndAcceptRequest(op); }

            Common::AsyncOperationSPtr RejectInvalidMessage(
                ClientRequestSPtr &&,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            // *****************
            // Utility functions
            // *****************
            Common::ErrorCode CheckApplicationTypeNameAndVersion(
                ServiceModelTypeName const & typeName,
                ServiceModelVersion const & typeVersion) const;

            Common::ErrorCode ScheduleNamingAsyncWork(
                Common::BeginAsyncWorkCallback const & beginCallback,
                Common::EndAsyncWorkCallback const & endCallback) const;

            Common::ErrorCode GetApplicationContexts(__out std::vector<ApplicationContext> &) const;

            Common::ErrorCode GetCompletedOrUpgradingApplicationContexts(__out std::vector<ApplicationContext> &, bool isQuery) const;

            Common::ErrorCode GetComposeDeploymentContexts(__out std::vector<ComposeDeploymentContext> &) const;

            Common::ErrorCode GetApplicationManifestContexts(
                __in std::wstring const &,
                bool excludeManifest,
                __out std::vector<std::pair<ApplicationTypeContext, std::shared_ptr<StoreDataApplicationManifest>>> &) const;

            Common::ErrorCode GetApplicationResourceQueryResult(
                Store::StoreTransaction const &,
                SingleInstanceDeploymentContext const &,
                Store::ReplicaActivityId const &,
                __out ServiceModel::ModelV2::ApplicationDescriptionQueryResult &);

            static Common::TimeSpan GetRandomizedOperationRetryDelay(Common::ErrorCode const &);
            Common::ErrorCode GetCompletedApplicationContext(
                Store::StoreTransaction const &,
                __out ApplicationContext &) const;
            Common::ErrorCode GetCompletedOrUpgradingApplicationContext(
                Store::StoreTransaction const &,
                __inout ApplicationContext &) const;
            Common::ErrorCode GetValidApplicationContext(
                Store::StoreTransaction const &,
                __inout ApplicationContext &) const;
            Common::ErrorCode GetApplicationContextForServiceProcessing(
                Store::StoreTransaction const &,
                __out ApplicationContext &) const;
            Common::ErrorCode ValidateServiceTypeDuringUpgrade(Store::StoreTransaction const &, ApplicationContext const &, ServiceModelTypeName const &) const;

            Common::ErrorCode GetStoreData(
                Store::StoreTransaction const &,
                DeploymentType::Enum deploymentType,
                std::wstring const & deploymentName,
                ServiceModelVersion const & version,
                std::shared_ptr<Store::StoreData> & storeData) const;

            template <class TStoreData>
            Common::ErrorCode ClearVerifiedUpgradeDomains(
                Store::StoreTransaction const &,
                __inout TStoreData &) const;

            Common::TimeSpan GetRollbackReplicaSetCheckTimeout(Common::TimeSpan const) const;

            Common::ErrorCode TraceAndGetErrorDetails(
                Common::ErrorCodeValue::Enum,
                std::wstring && msg,
                std::wstring const & level = L"Warning") const;

            Common::ErrorCode TraceAndGetErrorDetails(
                Common::StringLiteral const & traceComponent,
                Common::ErrorCodeValue::Enum,
                std::wstring && msg,
                std::wstring const & level = L"Warning") const;

            void CreateApplicationHealthReport(
                Common::NamingUri const & applicationName,
                Common::SystemHealthReportCode::Enum reportCode,
                std::wstring && applicationPolicyString,
                std::wstring const & applicationTypeName,
                ApplicationDefinitionKind::Enum const applicationDefinitionKind,
                uint64 instanceId,
                uint64 sequenceNumber,
                __inout std::vector<ServiceModel::HealthReport> & reports) const;

            void CreateApplicationHealthReport(
                ApplicationContext const & appContext,
                Common::SystemHealthReportCode::Enum reportCode,
                std::wstring && applicationPolicyString,
                __inout std::vector<ServiceModel::HealthReport> & reports) const;

            void CreateDeleteApplicationHealthReport(
                std::wstring const & applicationName,
                uint64 instanceId,
                uint64 sequenceNumber,
                __inout std::vector<ServiceModel::HealthReport> & reports) const;

            Common::ErrorCode ReportApplicationsType(
                Common::StringLiteral const & traceComponent,
                Common::ActivityId const & activityId,
                Common::AsyncOperationSPtr const &,
                std::vector<std::wstring> const &,
                Common::TimeSpan const timeout) const;

            static bool IsDnsServiceEnabled();
            static bool IsPartitionedDnsQueryFeatureEnabled();
            static Common::ErrorCode ValidateServiceDnsName(std::wstring const &);
            static Common::ErrorCode ValidateServiceDnsNameForPartitionedQueryCompliance(std::wstring const &);
            static std::wstring GetDeploymentNameFromAppName(std::wstring const &);

            bool QueueAutomaticCleanupApplicationType(std::wstring const &, Common::ActivityId const & activityId) const;
            Common::ErrorCode CloseAutomaticCleanupApplicationType();

            // ************
            // Test Helpers
            // ************

            void Test_SetProcessingDelay (Common::TimeSpan const &);
            void Test_SetRetryDelay (Common::TimeSpan const &);
            Common::ErrorCode Test_GetApplicationId(Common::NamingUri const &, __out ServiceModelApplicationId &) const;
            Common::ErrorCode Test_GetComposeDeployment(std::wstring const &, __out ServiceModelApplicationId &, __out std::wstring & typeName, __out std::wstring & typeVersion) const;
            Common::ErrorCode Test_GetComposeDeploymentContexts(__out std::vector<ComposeDeploymentContext> &, __out std::vector<StoreDataComposeDeploymentInstance> &) const;
            Common::ErrorCode Test_GetComposeDeploymentInstanceData(std::wstring const & ,__out std::vector<StoreDataComposeDeploymentInstance> &) const;
            Common::ErrorCode Test_GetComposeDeploymentUpgradeState(std::wstring const &, __out ServiceModel::ComposeDeploymentUpgradeState::Enum &) const;
            Common::ErrorCode Test_GetServiceContexts(__out std::vector<ServiceContext> &) const;
            Common::ErrorCode Test_GetServiceTemplates(__out std::vector<StoreDataServiceTemplate> &) const;
            Common::ErrorCode Test_GetServicePackages(__out std::vector<StoreDataServicePackage> &) const;
            HealthManager::HealthManagerReplicaSPtr Test_GetHealthManagerReplica() const;

            virtual Common::ErrorCode ProcessQuery(
                Query::QueryNames::Enum queryName,
                ServiceModel::QueryArgumentMap const & queryArgs,
                Common::ActivityId const & activityId,
                Common::TimeSpan const timeout,
                __out Transport::MessageUPtr & reply);

            bool SetMockImageBuilderParameter(
                std::wstring const & appName,
                std::wstring && mockBuildPath,
                std::wstring && mockTypeName,
                std::wstring && mockTypeVersion);

            void EraseMockImageBuilderParameter(std::wstring const & appName);

        protected:
            // ********************
            // KeyValueStoreReplica
            // ********************

            virtual Common::ErrorCode OnOpen(Common::ComPointer<IFabricStatefulServicePartition> const & servicePartition) override;
            virtual Common::ErrorCode OnChangeRole(::FABRIC_REPLICA_ROLE newRole, __out std::wstring & serviceLocation) override;
            virtual Common::ErrorCode OnClose() override;
            virtual void OnAbort() override;

        protected:

            virtual Common::AsyncOperationSPtr BeginProcessQuery(
                Query::QueryNames::Enum queryName,
                ServiceModel::QueryArgumentMap const & queryArgs,
                Common::ActivityId const & activityId,
                Common::TimeSpan timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);
            virtual Common::ErrorCode EndProcessQuery(
                Common::AsyncOperationSPtr const & operation,
                _Out_ Transport::MessageUPtr & reply);

            virtual Common::AsyncOperationSPtr BeginForwardMessage(
                std::wstring const & childAddressSegment,
                std::wstring const & childAddressSegmentMetadata,
                Transport::MessageUPtr & message,
                Common::TimeSpan timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);
            virtual Common::ErrorCode EndForwardMessage(
                Common::AsyncOperationSPtr const & asyncOperation,
                __out Transport::MessageUPtr & reply);

        private:
            Common::AsyncOperationSPtr BeginProcessUpgradeSingleInstanceApplication(
                std::shared_ptr<SingleInstanceDeploymentContext> &&,
                ServiceModel::SingleInstanceApplicationUpgradeDescription const & upgradeDescription,
                Store::StoreTransaction &&,
                ClientRequestSPtr && clientRequest,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & root);

            Common::AsyncOperationSPtr BeginReplaceSingleInstanceApplication(
                std::shared_ptr<SingleInstanceDeploymentContext> &&,
                ServiceModel::ModelV2::ApplicationDescription const &,
                Store::StoreTransaction &&,
                ClientRequestSPtr && clientRequest,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & root);

            class FinishAcceptRequestAsyncOperation;

            Common::ErrorCode OpenHealthManagerReplica();
            Common::ErrorCode CloseHealthManagerReplica();
            HealthManager::HealthManagerReplicaSPtr GetHealthManagerReplica() const;
            void OnReportApplicationsTypeComplete(
                Common::AsyncOperationSPtr const &,
                bool expectedCompletedSynchronously) const;

            // ReadPrefix
            template <class T>
            Common::ErrorCode ReadPrefix(std::wstring const & storeType, __out std::vector<T> & contexts) const;

            template <class T>
            Common::ErrorCode ReadPrefix(std::wstring const & storeType, std::wstring const & keyPrefix, __out std::vector<T> & contexts) const;

            template <class T>
            Common::ErrorCode ReadExact(__inout T & context) const;

            void CheckAddUniquePackages(
                std::wstring const & serviceManifestName,
                std::map<std::wstring, std::wstring> const & packages,
                __inout std::set<std::wstring> & duplicateDetector,
                __inout std::vector<std::wstring> & uniquePackages);

            // *********************
            // Management operations
            // *********************

            Common::ErrorCode UpdateOperationTimeout(
                Store::StoreTransaction const &,
                __in RolloutContext &,
                Common::TimeSpan const,
                __out bool & shouldCommit) const;

            Common::ErrorCode EndAcceptRequest(Common::AsyncOperationSPtr const &);

            Common::AsyncOperationSPtr FinishAcceptDeleteContext(
                Store::StoreTransaction &&,
                ClientRequestSPtr &&,
                std::shared_ptr<RolloutContext> &&,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            Common::AsyncOperationSPtr FinishAcceptRequest(
                Store::StoreTransaction &&,
                std::shared_ptr<RolloutContext> &&,
                Common::ErrorCode &&,
                bool shouldUpdateContext,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            Common::AsyncOperationSPtr FinishAcceptRequest(
                Store::StoreTransaction &&,
                std::shared_ptr<RolloutContext> &&,
                Common::ErrorCode &&,
                bool shouldUpdateContext,
                bool shouldCompleteClient,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            bool ShouldEnqueue(Common::ErrorCode const &);

            Common::AsyncOperationSPtr RejectRequest(
                ClientRequestSPtr &&,
                Common::ErrorCode &&,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            Common::ErrorCode RejectRequest(
                ClientRequestSPtr &&,
                Common::ErrorCode &&);

            // ****************
            // Message Handling
            // ****************

            static Common::GlobalWString RejectReasonMissingActivityHeader;
            static Common::GlobalWString RejectReasonMissingTimeoutHeader;
            static Common::GlobalWString RejectReasonMissingGatewayRetryHeader;
            static Common::GlobalWString RejectReasonIncorrectActor;
            static Common::GlobalWString RejectReasonInvalidAction;

            // r-values not compiling
            typedef std::function<Common::AsyncOperationSPtr(
                __in ClusterManagerReplica &,
                __in Transport::MessageUPtr &,
                __in Federation::RequestReceiverContextUPtr &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &)> ProcessRequestHandler;

            void Initialize();
            void InitializeRequestHandlers();
            void AddHandler(
                std::map<std::wstring, ProcessRequestHandler> & temp,
                std::wstring const &,
                ProcessRequestHandler const &);

            template <class TAsyncOperation>
            static Common::AsyncOperationSPtr CreateHandler(
                __in ClusterManagerReplica &,
                __in Transport::MessageUPtr &,
                __in Federation::RequestReceiverContextUPtr &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            bool ValidateClientMessage(__in Transport::MessageUPtr &, __out std::wstring & rejectReason);
            void InitializeMessageHandler();
            void RequestMessageHandler(Transport::MessageUPtr &&, Federation::RequestReceiverContextUPtr &&);
            void OnProcessRequestComplete(Common::ActivityId const &, Transport::MessageId const &, Common::AsyncOperationSPtr const &);
            Common::ErrorCode ValidateAndGetComposeDeploymentName(std::wstring const &applicationName, __out Common::NamingUri &);

            Common::AsyncOperationSPtr BeginProcessRequest(
                Transport::MessageUPtr &&,
                Federation::RequestReceiverContextUPtr &&,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            Common::AsyncOperationSPtr BeginProcessRequest(
                Transport::MessageUPtr &&,
                Federation::RequestReceiverContextUPtr &&,
                Common::ActivityId const &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            Common::ErrorCode EndProcessRequest(
                Common::AsyncOperationSPtr const &,
                __out Transport::MessageUPtr &,
                __out Federation::RequestReceiverContextUPtr &);

            Common::ErrorCode ToNamingGatewayReply(Common::ErrorCode const &);

            Common::ErrorCode EnqueueRolloutContext(std::shared_ptr<RolloutContext> &&);

            // ****************
            // Helper functions
            // ****************

            Common::ErrorCode ProcessServiceGroupFromTemplate(
                Management::ClusterManager::StoreDataServiceTemplate &,
                Common::NamingUri const &,
                std::vector<byte> const & initializationData);

            Common::ErrorCode CheckCompletedApplicationContext(ApplicationContext const &) const;
            Common::ErrorCode CheckDeletableApplicationContext(ApplicationContext const &) const;
            Common::ErrorCode CheckCompletedOrUpgradingApplicationContext(ApplicationContext const &) const;
            Common::ErrorCode CheckValidApplicationContext(ApplicationContext const &) const;
            Common::ErrorCode CheckApplicationContextForServiceProcessing(ApplicationContext const &) const;

            Common::ErrorCode GetDeletableApplicationContext(
                Store::StoreTransaction const &,
                __out ApplicationContext &) const;

            Common::ErrorCode GetCompletedOrUpgradingComposeDeploymentContext(
                Store::StoreTransaction const &,
                __inout ComposeDeploymentContext &) const;

            Common::ErrorCode GetCompletedComposeDeploymentContext(
                Store::StoreTransaction const &,
                __inout ComposeDeploymentContext &) const;

            Common::ErrorCode GetDeletableComposeDeploymentContext(
                Store::StoreTransaction const &,
                __inout ComposeDeploymentContext &) const;

            Common::ErrorCode GetDeletableSingleInstanceDeploymentContext(
                Store::StoreTransaction const &,
                __inout SingleInstanceDeploymentContext &) const;

            Common::ErrorCode GetCompletedApplicationTypeContext(Store::StoreTransaction const &, __inout ApplicationTypeContext &);

        public:
            Common::ErrorCode GetSingleInstanceDeploymentContext(
                Store::StoreTransaction const &,
                __inout SingleInstanceDeploymentContext &) const;

            Common::ErrorCode GetComposeDeploymentContext(
                Store::StoreTransaction const &,
                __inout ComposeDeploymentContext &) const;

            Common::ErrorCode GetApplicationContext(
                Store::StoreTransaction const &,
                __out ApplicationContext &) const;

            Common::ErrorCode GetApplicationTypeContext(
                Store::StoreTransaction const &,
                __inout ApplicationTypeContext &) const;

            Common::ErrorCode GetFabricVersionInfo(
                std::wstring const & codeFilepath,
                std::wstring const & clusterManifestFilepath,
                Common::TimeSpan const timeout,
                __out Common::FabricVersion &) const;

            Common::ErrorCode GetProvisionedFabricContext(
                Store::StoreTransaction const &,
                Common::FabricVersion const &,
                __inout FabricProvisionContext &,
                __out bool & containsVersion) const;

        private:

            Common::ErrorCode GetApplicationContextForQuery(
                std::wstring const & applicationName,
                Store::ReplicaActivityId const & replicaActivityId,
                __out std::unique_ptr<ApplicationContext> & applicationContext);

            Common::ErrorCode GetComposeDeploymentContextForQuery(
                std::wstring const & deploymentName,
                Store::ReplicaActivityId const & replicaActivityId,
                __out std::unique_ptr<ComposeDeploymentContext> & dockerComposeDeploymentContext);

            Common::ErrorCode GetServiceTypesFromStore(
                std::wstring applicationTypeName,
                std::wstring applicationTypeVersion,
                Store::ReplicaActivityId const & replicaActivityId,
                __out std::vector<ServiceModelServiceManifestDescription> & serviceManifests);
            ServiceModel::QueryResult GetApplications(ServiceModel::QueryArgumentMap const & queryArgs, Store::ReplicaActivityId const & replicaActivityId);
            ServiceModel::QueryResult GetApplicationTypes(ServiceModel::QueryArgumentMap const & queryArgs, Store::ReplicaActivityId const & replicaActivityId);
            ServiceModel::QueryResult GetApplicationTypesPaged(ServiceModel::QueryArgumentMap const & queryArgs, Store::ReplicaActivityId const & replicaActivityId);
            ServiceModel::QueryResult GetComposeDeploymentStatus(ServiceModel::QueryArgumentMap const & queryArgs, Store::ReplicaActivityId const & replicaActivityId);
            ServiceModel::QueryResult GetComposeDeploymentUpgradeProgress(ServiceModel::QueryArgumentMap const & queryArgs, Store::ReplicaActivityId const & replicaActivityId);
            ServiceModel::QueryResult GetServices(ServiceModel::QueryArgumentMap const & queryArgs, Store::ReplicaActivityId const & replicaActivityId);
            ServiceModel::QueryResult GetServiceTypes(ServiceModel::QueryArgumentMap const & queryArgs, Store::ReplicaActivityId const & replicaActivityId);
            ServiceModel::QueryResult GetServiceGroupMemberTypes(ServiceModel::QueryArgumentMap const & queryArgs, Store::ReplicaActivityId const & replicaActivityId);
            ServiceModel::QueryResult GetApplicationManifest(ServiceModel::QueryArgumentMap const & queryArgs, Store::ReplicaActivityId const & replicaActivityId);
            ServiceModel::QueryResult GetNodes(ServiceModel::QueryArgumentMap const & queryArgs, Store::ReplicaActivityId const & replicaActivityId);
            ServiceModel::QueryResult GetClusterManifest(ServiceModel::QueryArgumentMap const & queryArgs, Store::ReplicaActivityId const & replicaActivityId, Common::TimeSpan const timeout);
            ServiceModel::QueryResult GetClusterVersion();
            ServiceModel::QueryResult GetServiceManifest(ServiceModel::QueryArgumentMap const & queryArgs, Store::ReplicaActivityId const & replicaActivityId);
            ServiceModel::QueryResult GetInfrastructureTask(ServiceModel::QueryArgumentMap const & queryArgs, Store::ReplicaActivityId const & replicaActivityId);
            ServiceModel::QueryResult GetProvisionedFabricCodeVersions(ServiceModel::QueryArgumentMap const & queryArgs, Store::ReplicaActivityId const & replicaActivityId);
            ServiceModel::QueryResult GetProvisionedFabricConfigVersions(ServiceModel::QueryArgumentMap const & queryArgs, Store::ReplicaActivityId const & replicaActivityId);
            ServiceModel::QueryResult GetDeletedApplicationsList(ServiceModel::QueryArgumentMap const & queryArgs, Store::ReplicaActivityId const & ReplicaId);
            ServiceModel::QueryResult GetProvisionedApplicationTypePackages(ServiceModel::QueryArgumentMap const & queryArgs, Store::ReplicaActivityId const & ReplicaId);
            ServiceModel::QueryResult GetApplicationResources(ServiceModel::QueryArgumentMap const & queryArgs, Store::ReplicaActivityId const & replicaActivityId);
            ServiceModel::QueryResult GetServiceResources(ServiceModel::QueryArgumentMap const & queryArgs, Store::ReplicaActivityId const & replicaActivityId);
            ServiceModel::QueryResult GetVolumeResources(ServiceModel::QueryArgumentMap const & queryArgs, Store::ReplicaActivityId const & replicaActivityId);

            void InitializeCachedFilePtrs();

            static std::wstring ToWString(FABRIC_REPLICA_ROLE);
            static bool IsExclusiveFilterHelper(bool const isValid, bool & hasFilterSet);

            static bool IsStatusFilterDefaultOrAll(DWORD replicaStatusFilter);

            std::wstring workingDir_;
            Reliability::FederationWrapper & federation_;
            Reliability::ServiceResolver & resolver_;
            IImageBuilder & imageBuilder_;
            FabricClientProxy clientProxy_;
            std::unique_ptr<Common::AsyncOperationWorkJobQueue> namingJobQueue_;

            std::wstring nodeName_;
            std::unique_ptr<RolloutManager> rolloutManagerUPtr_;

            std::map<std::wstring, ProcessRequestHandler> requestHandlers_;

            Naming::StringRequestInstanceTrackerUPtr stringRequestTrackerUPtr_;
            Naming::NameRequestInstanceTrackerUPtr nameRequestTrackerUPtr_;

            ApplicationTypeRequestTrackerUPtr applicationTypeTrackerUPtr_;

            Common::atomic_bool isGettingApplicationTypeInfo_;
            mutable Common::atomic_bool isFabricImageBuilderOperationActive_;

            // This replica is kept alive by the runtime through a ComPointer, which
            // the runtime may release at any time. Therefore, this replica must
            // keep the FabricNode alive since references to its FederationSubsystem
            // and ReliabilitySubsystem are needed for resolution+routing.
            // Furthermore, this replica must be a ComponentRoot and keep itself alive
            // as needed.
            //
            Common::ComponentRootSPtr fabricRoot_;

            SystemServices::SystemServiceLocation serviceLocation_;
            SystemServices::SystemServiceMessageFilterSPtr messageFilterSPtr_;

            Common::TimeSpan contextProcessingDelay_;

            Query::QueryMessageHandlerUPtr queryMessageHandler_;

            // ClusterManager holds an instance of HealthManager to simulate co-located services.
            // CM will maintain the state on the HM by calling appropriate methods on Open/ChangeRole/Close.
            HealthManager::HealthManagerReplicaSPtr healthManagerReplica_;
            MUTABLE_RWLOCK(CM, lock_);

            // Create/Delete service requests will not perform any updates to the application context
            // in order to improve performance by avoiding write conflicts at the database record level.
            // This means that the processing of Create/Delete service requests can race with a
            // Delete application request. For example, a Delete application request must check the
            // state of all services within the application after committing the pending delete state.
            // Furthermore, the Create/Delete service requests must check the persisted state of the application
            // again after committing their own respective states.
            //
            Common::CachedFileSPtr currentClusterManifest_;

            std::shared_ptr<IApplicationDeploymentTypeNameVersionGenerator> dockerComposeAppTypeVersionGenerator_;
            std::shared_ptr<IApplicationDeploymentTypeNameVersionGenerator> containerGroupAppTypeVersionGenerator_;

            std::unique_ptr<VolumeManager> volumeManagerUPtr_;

            // For automatic removal of unused application types
            Common::ExclusiveLock cleanupApplicationTypeTimerLock_;
            Common::TimerSPtr cleanupApplicationTypeTimer_;
            Common::ExclusiveLock callbackLock_;

            class CleanupAppTypeJobItem;
            std::unique_ptr<AppTypeCleanupJobQueue<ClusterManagerReplica>> cleanupAppTypejobQueue_;
        };
    }
}
