// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    typedef std::function<bool(Common::StringCollection const &)> CommandHandler;

    class FabricTestSession;

    class FabricTestDispatcher: public FederationTestCommon::CommonDispatcher
    {
        DENY_COPY(FabricTestDispatcher)
    public:
        FabricTestDispatcher();

        virtual bool Open();
        virtual void Close();

        __declspec (property(get=getFederation)) FabricTestFederation & Federation;
        __declspec (property(get=getFabricClient)) TestFabricClient & FabricClient;
        __declspec (property(get=getFabricClientHealth)) TestFabricClientHealth & FabricClientHealth;
        __declspec (property(get=getFabricClientQuery)) TestFabricClientQuery & FabricClientQuery;
        __declspec (property(get=getQueryExecutor)) FabricTestQueryExecutor & QueryExecutor;
        __declspec (property(get=getFabricClientUpgrade)) TestFabricClientUpgrade & FabricClientUpgrade;
        __declspec (property(get=getFabricClientScenarios)) TestFabricClientScenarios & FabricClientScenarios;
        __declspec (property(get=getFabricClientSettings)) TestFabricClientSettings & FabricClientSettings;
        __declspec (property(get=getServiceMap)) FabricTestServiceMap & ServiceMap;
        __declspec (property(get=getApplicationMap)) FabricTestApplicationMap & ApplicationMap;
        __declspec (property(get=getUpgradeFabricData)) TestUpgradeFabricData & UpgradeFabricData;
        __declspec (property(get=getNamingState)) FabricTestNamingState & NamingState;
        __declspec (property(get=getFabricClientSecurity)) Transport::SecuritySettings const& FabricClientSecurity;

        __declspec (property(get=getNSDescription)) Reliability::ServiceDescription const & NSDescription;
        __declspec (property(get=getCMDescription)) Reliability::ServiceDescription const & CMDescription;
        __declspec (property(get=getFSDescription)) Reliability::ServiceDescription const & FSDescription;
        __declspec (property(get=getRMDescription)) Reliability::ServiceDescription const & RMDescription;
        __declspec (property(get=getISDescriptions)) std::vector<Reliability::ServiceDescription> const & ISDescriptions;
        __declspec (property(get=getIsNativeImageStoreEnabled)) bool IsNativeImageStoreEnabled;

        FabricTestFederation & getFederation() { return testFederation_; }
        TestFabricClient & getFabricClient() { return *fabricClient_; }
        TestFabricClientHealth & getFabricClientHealth() { return *fabricClientHealth_; }
        TestFabricClientQuery & getFabricClientQuery() { return *fabricClientQuery_; }
        FabricTestQueryExecutor & getQueryExecutor() { return *queryExecutor_; }
        TestFabricClientUpgrade & getFabricClientUpgrade() { return *fabricClientUpgrade_; }
        TestFabricClientScenarios & getFabricClientScenarios() { return *fabricClientScenarios_; }
        TestFabricClientSettings & getFabricClientSettings() { return *fabricClientSettings_; }
        FabricTestServiceMap & getServiceMap() { return serviceMap_; }
        FabricTestApplicationMap & getApplicationMap() { return applicationMap_; }
        TestUpgradeFabricData & getUpgradeFabricData() { return upgradeFabricData_; }
        FabricTestNamingState & getNamingState() { return namingState_; }

        Transport::SecuritySettings const& getFabricClientSecurity() { return clientCredentials_; }

        Reliability::ServiceDescription const& getNSDescription() { return namingServiceDescription_; }
        Reliability::ServiceDescription const& getCMDescription() { return clusterManagerServiceDescription_; }
        Reliability::ServiceDescription const& getFSDescription() { return fileStoreServiceDescription_; }
        Reliability::ServiceDescription const& getRMDescription() { return repairManagerServiceDescription_; }
        std::vector<Reliability::ServiceDescription> const& getISDescriptions() { return infrastructureServiceDescriptions_; }
        bool getIsNativeImageStoreEnabled() { return isNativeImageStoreEnabled_ && Management::ImageStore::ImageStoreServiceConfig::GetConfig().TargetReplicaSetSize > 0 ; }

        std::wstring GetState(std::wstring const & param);
        bool ShouldSkipCommand(std::wstring const & command) override;
        bool ExecuteCommand(std::wstring command) override;
        bool ExecuteApplicationBuilderCommand(std::wstring const & command, Common::StringCollection const & params);
        void PrintHelp(std::wstring const & command);

        std::wstring GetWorkingDirectory(std::wstring const & id);

        Management::HealthManager::HealthManagerReplicaSPtr GetHM();
        std::shared_ptr<Management::ClusterManager::ClusterManagerReplica> GetCM();
        std::shared_ptr<FabricNodeWrapper> GetPrimaryNodeWrapperForService(std::wstring const& serviceName);

        template <class TInterface>
        bool TryGetNamedFabricClient(std::wstring const &, REFIID riid, __out Common::ComPointer<TInterface> &);

        bool SetDefaultNodeCredentials(std::wstring const& value);
        bool SetClusterWideCredentials(std::wstring const& value);
        bool SetClientCredentials(std::wstring const& value) ;
        bool SetMockImageBuilderProperties(Common::StringCollection const & params);
        bool EraseMockImageBuilderProperties(std::wstring const & appName);
        bool NewTestCerts();
        void CleanupTestCerts();

        ReliabilityTestApi::FailoverManagerComponentTestApi::FailoverManagerTestHelperUPtr GetFM();
        ReliabilityTestApi::FailoverManagerComponentTestApi::FailoverManagerTestHelperUPtr GetFMM();

        bool IsFabricActivationManagerOpen();

        ITestStoreServiceSPtr ResolveService(std::wstring const& serviceName, __int64 key, Common::Guid const& fuId, bool useDefaultNamedClient, bool allowAllError = false);
        ITestStoreServiceSPtr ResolveServiceWithEvents(std::wstring const& serviceName, bool isCommand, HRESULT expectedError = S_OK);
        ITestStoreServiceSPtr ResolveServiceWithEvents(std::wstring const& serviceName, __int64 key, Common::Guid const& fuiId, bool isCommand, HRESULT expectedError = S_OK);
        ITestStoreServiceSPtr ResolveServiceWithEvents(std::wstring const& serviceName, std::wstring const & key, Guid const& fuId, bool isCommand, HRESULT expectedError = S_OK);
        bool PrefixResolveService(StringCollection const &);

        bool IsQuorumLostFU(Common::Guid const& fuId);
        bool IsRebuildLostFU(Common::Guid const& fuId);
        void ResetTestStoreClient(std::wstring const& serviceName);

        void RemoveFromQuorumLoss(std::wstring const& serviceName, std::set<Federation::NodeId> & downNodeIds, std::set<Federation::NodeId> & deactivatedNodeIds);

        void SetSystemServiceStorageProvider();
        void SetSystemServiceDescriptions();
        void SetSharedLogSettings();
        void SetCommandHandlers();

        bool IsRebuildLostFailoverUnit(Common::Guid partitionId);

        bool IsSeedNode(NodeId nodeId);

        void EnableNativeImageStore() { this->isNativeImageStoreEnabled_ = true; }
        void EnableTStoreSystemServices() { this->isTStoreSystemServicesEnabled_ = true; }

        DWORD GetComTimeout(Common::TimeSpan cTimeout);

        void Verify();

        static std::wstring TestDataDirectory;

        static std::wstring const ParamDelimiter;
        static std::wstring const ItemDelimiter;
        static std::wstring const KeyValueDelimiter;
        static std::wstring const ServiceFUDelimiter;
        static std::wstring const StatePropertyDelimiter;
        static std::wstring const GuidDelimiter;

        static std::wstring const ServiceTypeStateful;
        static std::wstring const ServiceTypeStateless;

        static std::wstring const PartitionKindSingleton;
        static std::wstring const PartitionKindInt64Range;
        static std::wstring const PartitionKindName;

        static std::wstring const ReplicaRolePrimary;
        static std::wstring const ReplicaRoleSecondary;
        static std::wstring const ReplicaRoleIdle;
        static std::wstring const ReplicaRoleNone;

        static std::wstring const ReplicaStateStandBy;
        static std::wstring const ReplicaStateInBuild;
        static std::wstring const ReplicaStateReady;
        static std::wstring const ReplicaStateDropped;

        static std::wstring const ReplicaStatusDown;
        static std::wstring const ReplicaStatusUp;
        static std::wstring const ReplicaStatusInvalid;

        static std::wstring const ServiceReplicaStatusInvalid;
        static std::wstring const ServiceReplicaStatusInBuild;
        static std::wstring const ServiceReplicaStatusStandBy;
        static std::wstring const ServiceReplicaStatusReady;
        static std::wstring const ServiceReplicaStatusDown;
        static std::wstring const ServiceReplicaStatusDropped;

        static std::vector<Reliability::ServiceCorrelationDescription> const DefaultCorrelations;
        static std::wstring const DefaultPlacementConstraints;
        static int const DefaultScaleoutCount;
        static std::vector<Reliability::ServiceLoadMetricDescription> const DefaultServiceMetrics;
        static uint const DefaultMoveCost;

        static bool UseBackwardsCompatibleClients;

    private:
        friend class TestFabricClient;
        friend class TestFabricClientHealth;
        friend class TestIpcClientHealth;
        friend class TestFabricClientQuery;
        friend class TestFabricClientUpgrade;
        friend class TestFabricClientSettings;
        friend class NativeImageStoreExecutor;

        class ComTestServiceNotificationEventHandler;

        bool AddDiskDriveFolders(Common::StringCollection const & params);
        bool VerifyDeletedDiskDriveFolders();

        bool AddBehavior2(Common::StringCollection const & params);
        bool RemoveBehavior2(Common::StringCollection const & params);
        bool CheckTransportBehaviorlist2(Common::StringCollection const & params);
        bool AddLeaseBehavior(Common::StringCollection const & params);
        bool RemoveLeaseBehavior(Common::StringCollection const & params);

        bool DeleteService(Common::StringCollection const & params);
        bool DeleteServiceGroup(Common::StringCollection const & params);
        bool AddNode(Common::StringCollection const & params);
        bool RemoveNode(Common::StringCollection const & params);
        bool RemoveNode(std::wstring const & nodeId, bool removeData = false);
        bool AbortNode(std::wstring const & params);
        bool CloseAllNodes(bool abortNodes);
        bool SetProperty(Common::StringCollection const & params);
        bool SetFailoverManagerServiceProperties(Common::StringCollection const & params);
        bool SetNamingServiceProperties(Common::StringCollection const & params);
        bool SetClusterManagerServiceProperties(Common::StringCollection const & params);
        bool SetRepairManagerServiceProperties(Common::StringCollection const & params);
        bool SetImageStoreServiceProperties(Common::StringCollection const & params);
        bool SetInfrastructureServiceProperties(Common::StringCollection const & params);
        bool RemoveInfrastructureServiceProperties(Common::StringCollection const & params);
        bool SetDnsServiceProperties(Common::StringCollection const & params);
        bool SetNIMServiceProperties(Common::StringCollection const & params);
        bool SetEnableUnsupportedPreviewFeatures(Common::StringCollection const & params);
        bool ClearTicket(Common::StringCollection const & params);
        bool ResetStore(Common::StringCollection const & params);
        bool CleanTest();
        bool VerifyImageStore(Common::StringCollection const & params);
        bool VerifyNodeFiles(Common::StringCollection const & params);

        bool StartTestFabricClient(Common::StringCollection const & params);
        bool StopTestFabricClient();

        bool CreateNamedFabricClient(Common::StringCollection const & params);
        bool RegisterCallback(Common::StringCollection const & params);
        bool UnregisterCallback(Common::StringCollection const & params);
        bool WaitForCallback(Common::StringCollection const & params);
        bool DeleteNamedFabricClient(Common::StringCollection const & params);
        bool VerifyCacheItem(Common::StringCollection const & params);
        bool RemoveClientCacheItem(Common::StringCollection const & params);
        bool VerifyFabricTime(Common::StringCollection const & params);

        bool InjectFailure(Common::StringCollection const & params);
        bool InjectFailureUntil(Common::StringCollection const & params);
        bool RemoveFailure(Common::StringCollection const & params);
        bool SetSignal(Common::StringCollection const & params);
        bool ResetSignal(Common::StringCollection const & params);        
        bool WaitForSignalHit(Common::StringCollection const & params);
        bool FailIfSignalHit(Common::StringCollection const & params);
        bool WaitAsync(Common::StringCollection const & params);

        bool ResolveService(
            Common::StringCollection const & params,
            ServiceLocationChangeClientSPtr const & client);
        bool ResolveServiceWithEvents(Common::StringCollection const & params);
        bool ResolveServiceUsingClient(Common::StringCollection const & params);

        bool GetServiceDescriptionUsingClient(Common::StringCollection const & params);

        bool VerifyUpgradeApp(Common::StringCollection const & params);
        bool VerifyUpgradeFabric(Common::StringCollection const & params);
        bool PrepareUpgradeFabric(Common::StringCollection const & params);

        bool FailFabricUpgradeDownload(Common::StringCollection const & params);
        bool FailFabricUpgradeValidation(Common::StringCollection const & params);
        bool FailFabricUpgrade(Common::StringCollection const & params);

        bool AddRuntime(Common::StringCollection const & params);
        bool RemoveRuntime(Common::StringCollection const & params);
        bool UnregisterRuntime(Common::StringCollection const & params);
        bool RegisterServiceType(Common::StringCollection const & params);
        bool EnableServiceType(Common::StringCollection const & params);
        bool DisableServiceType(Common::StringCollection const & params);

        bool ClientPut(Common::StringCollection const & params);
        bool ClientGet(Common::StringCollection const & params);
        bool ClientGetAll(Common::StringCollection const & params);
        bool ClientGetKeys(Common::StringCollection const & params);
        bool ClientDelete(Common::StringCollection const & params);
        bool StartClient(Common::StringCollection const & params);
        bool StopClient(Common::StringCollection const & params);
        bool ClientBackup(Common::StringCollection const & params);
        bool ClientRestore(Common::StringCollection const & params);
        bool ClientCompression(Common::StringCollection const & params);
        bool SetBackwardsCompatibleClients(Common::StringCollection const & params);
        bool ReportFault(Common::StringCollection const & params);
        bool ReportLoad(Common::StringCollection const & params);
        bool ReportCurrentPartitionHealth(StringCollection const & params);
        bool VerifyLoadReport(Common::StringCollection const & params);
        bool VerifyLoadValue(Common::StringCollection const & params);
        bool VerifyNodeLoad(Common::StringCollection const & params);
        bool VerifyPartitionLoad(Common::StringCollection const & params);
        bool VerifyClusterLoad(Common::StringCollection const & params);
        bool VerifyReadWriteStatus(Common::StringCollection const & params);
        bool CheckIfLfupmIsEmpty(Common::StringCollection const & params);
        bool ReportMoveCost(Common::StringCollection const & params);
        bool VerifyMoveCostValue(Common::StringCollection const & params);
        bool VerifyUnplacedReason(Common::StringCollection const & params);
        bool VerifyApplicationLoad(Common::StringCollection const & params);
        bool VerifyResourceOnNode(Common::StringCollection const & params);
        bool VerifyLimitsEnforced(Common::StringCollection const & params);
        bool VerifyPLBAndLRMSync(Common::StringCollection const& params);
        bool VerifyContainerPods(Common::StringCollection const& params);

        bool SetSecondaryPumpEnabled(Common::StringCollection const & params);
        TestStoreClientSPtr GetTestClient(std::wstring const& serviceName);

        bool ListNodes();
        bool ShowFM();
        bool ShowNodes();
        bool ShowGFUM(Common::StringCollection const & params);
        bool ShowLFUM(Common::StringCollection const & params);
        bool UpdateServicePackageVersionInstance(Common::StringCollection const & params);
        bool ShowService();
        bool ShowServiceType();
        bool ShowApplication();
        bool ShowActiveServices(Common::StringCollection const & params);
        bool ShowLoadMetricInfo(Common::StringCollection const & params);
        void ShowLFUMOnNode(ReliabilityTestApi::ReconfigurationAgentComponentTestApi::ReconfigurationAgentTestHelper & ra, std::wstring const & fuid);
        bool ShowHM(Common::StringCollection const & params);

        bool ProcessPendingPlbUpdates(Common::StringCollection const& params);
        bool SwapPrimary(Common::StringCollection const & params);
        bool PromoteToPrimary(Common::StringCollection const & params);
        bool MoveSecondary(Common::StringCollection const & params);
        bool DropReplica(Common::StringCollection const & params);
        bool MovePrimaryReplicaFromClient(Common::StringCollection const & params);
        bool MoveSecondaryReplicaFromClient(Common::StringCollection const & params);
        bool PLBUpdateService(Common::StringCollection const & params);
        bool UpdateNodeImages(Common::StringCollection const & params);

        bool CallService(Common::StringCollection const & params);

        bool UseAdminClientCredential();
        bool UseReadonlyClientCredential();

        ReliabilityTestApi::FailoverManagerComponentTestApi::FailoverManagerTestHelperUPtr VerifyFM();
        bool VerifyNodes(std::vector<ReliabilityTestApi::FailoverManagerComponentTestApi::NodeSnapshot> const & nodes, size_t upCount, std::wstring const& fmType);
        bool VerifyAll(bool verifyPLB = false);
        bool VerifyAll(Common::StringCollection const & params, bool verifyPLB = false);
        bool VerifyFMM();
        bool VerifyFMM(int expectedReplicaCount);
        bool VerifyGFUM(
            std::map<Reliability::FailoverUnitId, ReliabilityTestApi::FailoverManagerComponentTestApi::FailoverUnitSnapshot> const & failoverUnits,
            std::vector<ReliabilityTestApi::FailoverManagerComponentTestApi::ServiceSnapshot> const & services);
        bool VerifyLFUM();
        bool VerifyLFUMOnNode(
            ReliabilityTestApi::ReconfigurationAgentComponentTestApi::ReconfigurationAgentTestHelper & ra,
            Federation::NodeId nodeId,
            ReliabilityTestApi::FailoverManagerComponentTestApi::FailoverManagerTestHelperUPtr & fm,
            ReliabilityTestApi::FailoverManagerComponentTestApi::FailoverManagerTestHelperUPtr & fmm,
            Common::StopwatchTime nodeOpenTimestamp);
        bool VerifyNodeActivationStatus(
            ReliabilityTestApi::ReconfigurationAgentComponentTestApi::ReconfigurationAgentTestHelper & ra,
            bool isFmm,
            ReliabilityTestApi::FailoverManagerComponentTestApi::NodeSnapshot const & fmNodeInfo);

        bool VerifyFMReplicaActivationStatus(ReliabilityTestApi::FailoverManagerComponentTestApi::ReplicaSnapshot & replica, Reliability::FailoverUnitId const & ftId);

        bool VerifyServiceLocation(std::vector<std::wstring> const & serviceLocations, std::wstring const & toCheck);
        bool VerifyServicePlacement();
        void ClearNodeServicePlacementList();

        bool KillCodePackage(Common::StringCollection const & params);
        bool SetCodePackageKillPending(Common::StringCollection const & params);

        bool KillService(
            Common::StringCollection const & params,
            ReliabilityTestApi::FailoverManagerComponentTestApi::FailoverManagerTestHelper* = nullptr);
        bool KillFailoverManagerService(Common::StringCollection const & params);
        bool KillNamingService(Common::StringCollection const & params);
        bool InjectError(Common::StringCollection const & params);
        bool RemoveError(Common::StringCollection const & params);

        bool OpenHttpGateway(Common::StringCollection const & params);
        void CloseHttpGateway();

        bool EseDump(Common::StringCollection const & params);

        bool RequestCheckpoint(Common::StringCollection const & params);

        Common::ErrorCode OpenFabricActivationManager();
        Common::ErrorCode CloseFabricActivationManager();

        void FindAndInvoke(
            Federation::NodeId const & nodeId,
            std::wstring const & serviceName,
            std::function<void (ITestStoreServiceSPtr const &)> storeServiceFunc,
            std::function<void (CalculatorServiceSPtr const &)> calculatorServiceFunc);

        void AddUnreliableTransportBehavior(
            std::wstring const & behaviorName,
            std::wstring const & nodeIdString,
            std::wstring const & destinationFilter,
            std::wstring const & actionFilter,
            float probabilityToApply,
            Common::TimeSpan delay,
            Common::TimeSpan delaySpan = Common::TimeSpan::Zero,
            int priority = 0);
        void RemoveUnreliableTransportBehavior(std::wstring const & behaviorName);

        bool DeactivateNodes(Common::StringCollection const & params);
        bool RemoveNodeDeactivation(Common::StringCollection const& params);
        bool VerifyNodeDeactivationStatus(Common::StringCollection const & params);

        Reliability::NodeDeactivationIntent::Enum ParseNodeDeactivationIntent(std::wstring const& intentString);
        Reliability::NodeDeactivationStatus::Enum ParseNodeDeactivationStatus(std::wstring const& statusString);

        std::wstring GetReplicaRoleString(Reliability::ReplicaRole::Enum replicaRole);
        std::wstring GetReplicaStateString(Reliability::ReplicaStates::Enum replicaState);
        std::wstring GetReplicaStatusString(Reliability::ReplicaStatus::Enum replicaStatus);

        std::wstring GetServiceReplicaStatusString(FABRIC_QUERY_SERVICE_REPLICA_STATUS replicaStatus);

        ReliabilityTestApi::FailoverManagerComponentTestApi::FailoverUnitSnapshotUPtr GetFMFailoverUnitFromParam(std::wstring const& param);
        ReliabilityTestApi::FailoverManagerComponentTestApi::ReplicaSnapshotUPtr GetReplicaFromParam(std::wstring const& param, ReliabilityTestApi::FailoverManagerComponentTestApi::FailoverUnitSnapshotUPtr &);
        ReliabilityTestApi::ReconfigurationAgentComponentTestApi::FailoverUnitSnapshotUPtr GetRAFailoverUnitFromParam(std::wstring const& param, Federation::NodeId nodeId);
        std::wstring GetFailoverUnitLocationState(Common::StringCollection const & params);

        std::wstring GetFMState(Common::StringCollection const & params);
        std::wstring GetFMMState(Common::StringCollection const & params);
        std::wstring GetRAState(Common::StringCollection const & params);
        std::wstring GetRAUpgradeState(Common::StringCollection const & params);

        std::wstring GetFMNodeState(Common::StringCollection const & params);
        std::wstring GetFMReplicaState(Common::StringCollection const & params);
        std::wstring GetFMUpReplicaCount(Common::StringCollection const & params);
        std::wstring GetFMOfflineReplicaCount(Common::StringCollection const & params);
        std::wstring GetFMApplicationState(Common::StringCollection const & params);
        std::wstring GetFMServiceState(Common::StringCollection const & params);
        std::wstring GetFMMServiceState(Common::StringCollection const & params);
        std::wstring GetFMFailoverUnitState(Common::StringCollection const & params);

        std::wstring GetDataLossVersion(StringCollection const & params);
        std::wstring GetReconfig(Common::StringCollection const & params);
        std::wstring GetQuorumLost(Common::StringCollection const & params);
        std::wstring GetIsInRebuild(StringCollection const & params);
        std::wstring GetPartitionId(Common::StringCollection const & params);
        std::wstring GetPartitionKind(Common::StringCollection const & params);
        std::wstring GetPartitionLowKey(Common::StringCollection const & params);
        std::wstring GetPartitionHighKey(Common::StringCollection const & params);
        std::wstring GetPartitionName(Common::StringCollection const & params);
        std::wstring GetTargetReplicaSetSize(Common::StringCollection const & params);
        std::wstring GetMinReplicaSetSize(Common::StringCollection const & params);
        std::wstring GetPartitionStatus(Common::StringCollection const & params);
        std::wstring GetIsOrphaned(Common::StringCollection const & params);
        std::wstring GetDroppedReplicaCount(Common::StringCollection const & params);
        std::wstring GetReplicaCount(Common::StringCollection const & params);
        std::wstring GetEpoch(Common::StringCollection const& params);

        std::wstring GetRAFUState(Common::StringCollection const & params);
        std::wstring GetRAReplicaState(Common::StringCollection const & params);
        std::wstring GetCodePackageId(Common::StringCollection const & params);

        void RemoveTicketFile(std::wstring const & id);
        void VerifyClientIsActive();
        void DoWork(ULONG threadId);
        bool VerifyResolveResult(Naming::ResolvedServicePartition const& result, std::wstring const& serviceName);
        bool ParseServiceGroupAddress(std::wstring const & serviceGroupAddress, std::wstring const & pattern, std::wstring & memberServiceAddress);

        void PrintAllQuorumAndRebuildLostFailoverUnits();
        bool IsQuorumLostForAnyFailoverUnit();
        void UpdateQuorumOrRebuildLostFailoverUnits();
        bool IsQuorumLostForReconfigurationFU(ReliabilityTestApi::FailoverManagerComponentTestApi::FailoverUnitSnapshot const& failoverUnitUPtr);
        bool FabricTestDispatcher::IsQuorumLostForReconfigurationFU(ReliabilityTestApi::ReconfigurationAgentComponentTestApi::FailoverUnitSnapshotUPtr & primaryRafu);
        bool WaitForAllToApplyLsn(Common::StringCollection const & params);
        bool WaitForAllToApplyLsnExtension(std::wstring & serviceName, NodeId & nodeId, TimeSpan timeout);
        FABRIC_SEQUENCE_NUMBER CalculateTargetLsn(std::wstring & serviceName, NodeId & nodeId);
        bool WaitForTargetLsn(std::wstring & serviceName, NodeId & nodeId, TimeSpan timeout, FABRIC_SEQUENCE_NUMBER targetLsn);
        bool EnableLogTruncationTimestampValidation(Common::StringCollection const & params);

        bool CheckContainers(Common::StringCollection const & params);

        bool FabricTestDispatcher::TryGetPrimaryRAFU(
            ReliabilityTestApi::FailoverManagerComponentTestApi::FailoverUnitSnapshot const& failoverUnitUPtr,
            ReliabilityTestApi::ReconfigurationAgentComponentTestApi::FailoverUnitSnapshotUPtr & primaryRafu);

        void GetDownNodesForFU(Common::Guid const& fuId, vector<Federation::NodeId> & nodeIds);

        bool CreateServiceNotificationClient(Common::StringCollection const & params);
        bool SetServiceNotificationWait(Common::StringCollection const & params);
        bool ServiceNotificationWait(Common::StringCollection const & params);

        void CreateServiceNotificationClientOnNodes(
            std::vector<std::wstring> const & nodeIdStrings,
            std::wstring const & clientName,
            Api::IServiceNotificationEventHandlerPtr const & handler);

        void CreateFabricClientOnNode(
            std::wstring const & nodeIdString,
            __out ComPointer<IFabricServiceManagementClient> &);
        void CreateFabricClientOnNode(
            std::wstring const & nodeIdString,
            __out ComPointer<IFabricServiceManagementClient5> &);
        void CreateFabricClientAtAddresses(
            Common::StringCollection const & addresses,
            __out ComPointer<IFabricServiceManagementClient> &);
        void CreateNativeImageStoreClient(
            __out ComPointer<IFabricNativeImageStoreClient> &);

        Api::IClientFactoryPtr CreateClientFactory();

        void UploadToNativeImageStore(std::wstring const & localLocation, std::wstring const & storeRelativePath, ComPointer<IFabricNativeImageStoreClient> const & imageStoreClientCPtr);

        void SetDefaultCertIssuers();
        void ClearDefaultCertIssuers();

        bool SetEseOnly();
        bool ClearEseOnly();

        bool Xcopy(Common::StringCollection const & params);

        static int const RetryWaitMilliSeconds;
        static int const ClientActivityCheckIntervalInMilliseconds;

        Reliability::ServiceDescription namingServiceDescription_;
        Reliability::ServiceDescription clusterManagerServiceDescription_;
        Reliability::ServiceDescription fileStoreServiceDescription_;
        Reliability::ServiceDescription repairManagerServiceDescription_;
        std::vector<Reliability::ServiceDescription> infrastructureServiceDescriptions_;

        Common::RwLock testClientsMapLock_;
        std::map<std::wstring, TestStoreClientSPtr> testClients_;

        Common::RwLock quorumAndRebuildLostFailoverUnitsLock_;
        std::vector<Common::Guid> quorumLostFailoverUnits_;
        std::vector<Common::Guid> rebuildLostFailoverUnits_;
        std::vector<Common::Guid> toBeDeletedFailoverUnits_;

        ULONG clientThreads_;
        double putRatio_;
        int64 maxClientOperationInterval_;
        bool clientsStopped_;
        Common::ExclusiveLock lastClientOperationMapLock_;
        std::map<__int64, Common::DateTime> lastClientOperationMap_;

        std::wstring defaultNodeCredentials_;
        Common::StringCollection clusterWideCredentials_;
        std::wstring clientCredentialsType_;
        Transport::SecuritySettings clientCredentials_;

        std::unique_ptr<NativeImageStoreExecutor> nativeImageStoreExecutor_;
        std::shared_ptr<TestFabricClient> fabricClient_;
        std::unique_ptr<TestFabricClientHealth> fabricClientHealth_;
        std::unique_ptr<CheckpointTruncationTimestampValidator> checkpointTruncationTimestampValidator_;
        std::unique_ptr<TestFabricClientQuery> fabricClientQuery_;
        std::unique_ptr<FabricTestQueryExecutor> queryExecutor_;
        std::unique_ptr<TestFabricClientUpgrade> fabricClientUpgrade_;
        std::unique_ptr<TestFabricClientScenarios> fabricClientScenarios_;
        std::unique_ptr<TestFabricClientSettings> fabricClientSettings_;
        std::shared_ptr<Hosting2::FabricActivationManager> fabricActivationManager_;
        ServiceLocationChangeClientManager clientManager_;
        std::map<std::wstring, Common::ComPointer<IFabricServiceNotificationEventHandler>> serviceNotificationEventHandlers_;
        FabricTestClientsTracker clientsTracker_;
        FabricTestCommandsTracker commandsTracker_;

        FabricTestFederation testFederation_;
        FabricTestServiceMap serviceMap_;
        FabricTestApplicationMap applicationMap_;
        TestUpgradeFabricData upgradeFabricData_;
        FabricTestNamingState namingState_;
        std::shared_ptr<HttpGateway::HttpGatewayImpl> httpGateway_;

        bool isNativeImageStoreEnabled_;
        bool isTStoreSystemServicesEnabled_;
        bool isSkipForTStoreEnabled_;

        std::map<std::wstring, CommandHandler> commandHandlers_;

        ConfigurationSetter configurationSetter_;
    };
}
