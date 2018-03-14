// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class FabricTestSession;
    class FabricTestDispatcher;

    class TestFabricClientHealth 
    {
        DENY_COPY(TestFabricClientHealth);

    public:
        TestFabricClientHealth(FabricTestDispatcher & dispatcher);
        ~TestFabricClientHealth() {}

        __declspec (property(get=getHealthTable)) TestHealthTableSPtr const & HealthTable;
        TestHealthTableSPtr const & getHealthTable() { return healthTable_; }

        bool ReportHealth(Common::StringCollection const & params);
        bool ReportHealthInternal(Common::StringCollection const & params);
        bool DeleteHealth(Common::StringCollection const & params);
        bool QueryHealth(Common::StringCollection const & params);
        bool QueryHealthStateChunk(Common::StringCollection const & params);
        bool QueryHealthList(Common::StringCollection const & params);
        bool CheckHMEntity(Common::StringCollection const & params);
        bool HealthPreInitialize(Common::StringCollection const & params);
        bool HealthPostInitialize(Common::StringCollection const & params);
        bool HealthGetProgress(Common::StringCollection const & params);
        bool HealthSkipSequence(Common::StringCollection const & params);
        bool ResetHealthClient(Common::StringCollection const & params);
        bool RunWatchDog(Common::StringCollection const & params);
        bool CheckHealthClient(Common::StringCollection const & params);
        bool CheckHM(Common::StringCollection const & params);
        bool CorruptHMEntity(Common::StringCollection const & params);
        bool SetHMThrottle(Common::StringCollection const & params);
        bool ShowHM(Management::HealthManager::HealthManagerReplicaSPtr const & hm, Common::StringCollection const & params);
        bool TakeClusterHealthSnapshot(Common::StringCollection const & params);
        bool CheckIsClusterHealthy(Common::StringCollection const & params);

        bool VerifyNodeHealthReports(
            Management::HealthManager::HealthManagerReplicaSPtr const & hm, 
            std::vector<ReliabilityTestApi::FailoverManagerComponentTestApi::NodeSnapshot> const & nodes, 
            size_t expectedUpCount);

        bool VerifyParititionHealthReports(
            Management::HealthManager::HealthManagerReplicaSPtr const & hm, 
            std::map<Reliability::FailoverUnitId, ReliabilityTestApi::FailoverManagerComponentTestApi::FailoverUnitSnapshot> const & failoverUnits);
        
        bool VerifyReplicaHealthReports(
            Management::HealthManager::HealthManagerReplicaSPtr const & hm, 
            std::map<Reliability::FailoverUnitId, ReliabilityTestApi::FailoverManagerComponentTestApi::FailoverUnitSnapshot> const & failoverUnits);

        bool VerifyServiceHealthReports(
            Management::HealthManager::HealthManagerReplicaSPtr const & hm, 
            std::vector<ReliabilityTestApi::FailoverManagerComponentTestApi::ServiceSnapshot> const & services);

        bool VerifyApplicationHealthReports(
            Management::HealthManager::HealthManagerReplicaSPtr const & hm, 
            std::vector<ReliabilityTestApi::FailoverManagerComponentTestApi::ServiceSnapshot> const & services);

        bool VerifyDeployedServicePackageAndApplicationHealthReports(
            Management::HealthManager::HealthManagerReplicaSPtr const & hm, 
            std::map<Management::HealthManager::DeployedServicePackageHealthId, Reliability::FailoverUnitId> && packages);

        bool HMLoadTest(Common::StringCollection const & params);

        static bool VerifyAggregatedHealthStates(CommandLineParser & parser, Management::HealthManager::HealthStateCountMap const & results);
        static bool VerifyHealthQueryListResult(CommandLineParser & parser, Query::QueryNames::Enum queryName, ServiceModel::QueryResult & queryResult);
        
        static bool VerifyHealthStateCount(std::wstring const & expectedHealthStatesString, Management::HealthManager::HealthStateCount const & counts);
        static bool VerifyHealthStateCountMap(std::wstring const & expectedHealthStatesString, Management::HealthManager::HealthStateCountMap const & counts);
        
        static bool VerifyExpectedHealthEvaluations(
            FABRIC_HEALTH_EVALUATION_LIST const * publicHealthEvaluations,
            std::wstring const & expectedHealthEvaluation);

        static void PrintHealthEvaluations(
            FABRIC_HEALTH_EVALUATION_LIST const * publicHealthEvaluations, 
            std::vector<FABRIC_HEALTH_EVALUATION_KIND> const & possibleReasons);
        
        static void PrintHealthEvaluations(
            std::vector<ServiceModel::HealthEvaluation> const & unhealthyEvaluations,
            std::vector<FABRIC_HEALTH_EVALUATION_KIND> const & possibleReasons);

        static std::wstring GetHealthStateString(FABRIC_HEALTH_STATE healthState);

        static bool GetApplicationHealthPolicies(__in CommandLineParser & parser, __inout ServiceModel::ApplicationHealthPolicyMapSPtr & applicationHealthPolicies);

        //Report Health Ipc

        bool ReportHealthIpc(StringCollection const & params);
        
    private:           
        static std::wstring ConvertHealthOutput(std::wstring const & input);

        FabricTestDispatcher & dispatcher_;
        ComPointer<IFabricHealthClient4> healthClient_;
        Api::IHealthClientPtr internalHealthClient_;
        TestHealthTableSPtr healthTable_;
        std::map<std::wstring, TestWatchDogSPtr> watchdogs_;

        void ParallelReportHealthBatch(std::vector<ServiceModel::HealthReport> && reports);

        void CreateFabricHealthClient(__in FabricTestFederation & testFederation);
        void CreateInternalFabricHealthClient(__in FabricTestFederation & testFederation);
        
        static bool CheckHResult(HRESULT hr, std::vector<HRESULT> const & expectedErrors);
        static FABRIC_HEALTH_STATE GetHealthState(std::wstring const & healthStateString);
        std::wstring GetEntityHealthBaseDetails(ServiceModel::EntityHealthBase const & entityBase);
        static bool GetHealthStateFilter(std::wstring const & value, __out DWORD & filter);

        Common::ErrorCode ReportHealthThroughInternalClientOrHmPrimary(CommandLineParser & parser, std::vector<ServiceModel::HealthReport> && reports);

        using ValidateHealthCallback = std::function<bool(bool expectEmpty, bool requiresValidation)>;
        bool PerformHealthQueryClientOperation(
            __in CommandLineParser & parser,
            std::wstring const & operationName,
            TestFabricClient::BeginFabricClientOperationCallback const & beginCallback,
            TestFabricClient::EndFabricClientOperationCallback const & endCallback,
            ValidateHealthCallback const & validateCallback);

        bool GetClusterHealth(Common::StringCollection const & params);
        bool GetNodeHealth(Common::StringCollection const & params);
        bool GetReplicaHealth(Common::StringCollection const & params);
        bool GetPartitionHealth(Common::StringCollection const & params);
        bool GetServiceHealth(Common::StringCollection const & params);
        bool GetApplicationHealth(Common::StringCollection const & params);
        bool GetDeployedApplicationHealth(Common::StringCollection const & params);
        bool GetDeployedServicePackageHealth(Common::StringCollection const & params);
        
        bool GetClusterHealthChunk(Common::StringCollection const & params, bool requiresValidation, std::vector<HRESULT> const & expectedErrors, std::vector<HRESULT> const & allowedErrorsOnRetry, std::vector<HRESULT> const & retryableErrors);

        bool ParseCommonHealthInformation(CommandLineParser & parser, FABRIC_SEQUENCE_NUMBER & sequenceNumber, std::wstring & sourceId, FABRIC_HEALTH_STATE & healthState, std::wstring & property, int64 & timeToLiveSeconds, std::wstring & description, bool & removeWhenExpired);
        bool ParsePartitionId(CommandLineParser & parser, __inout Common::Guid & partitionGuid);
        bool ParseReplicaId(CommandLineParser & parser, __inout Common::Guid & partitionGuid, __inout FABRIC_REPLICA_ID & replicaId, __inout FABRIC_INSTANCE_ID & replicaInstanceId);
        bool ParseNodeHealthInformation(CommandLineParser & parser, NodeId & nodeId, FABRIC_NODE_INSTANCE_ID & nodeInstanceId, std::wstring & nodeName);
        static bool ParseUnhealthyState(std::wstring const & input, __inout ULONG & error, __inout ULONG & total);
        static bool ParseSnapshot(std::wstring const & input, Management::HealthManager::ClusterUpgradeStateSnapshot & snapshot);

        bool CreateCommonHealthInformation(CommandLineParser & parser, Common::ScopedHeap & heap, FABRIC_HEALTH_INFORMATION & healthInformation);
        bool CreateClusterHealthReport(CommandLineParser & parser, Common::ScopedHeap & heap, FABRIC_HEALTH_REPORT & healthReport);
        bool CreateNodeHealthReport(CommandLineParser & parser, Common::ScopedHeap & heap, FABRIC_HEALTH_REPORT & healthReport);
        bool CreatePartitionHealthReport(CommandLineParser & parser, Common::ScopedHeap & heap, FABRIC_HEALTH_REPORT & healthReport);
        bool CreateReplicaHealthReport(CommandLineParser & parser, Common::ScopedHeap & heap, FABRIC_HEALTH_REPORT & healthReport);
        bool CreateInstanceHealthReport(CommandLineParser & parser, Common::ScopedHeap & heap, FABRIC_HEALTH_REPORT & healthReport);
        bool CreateDeployedApplicationHealthReport(CommandLineParser & parser, Common::ScopedHeap & heap, FABRIC_HEALTH_REPORT & healthReport);
        bool CreateDeployedServicePackageHealthReport(CommandLineParser & parser, Common::ScopedHeap & heap, FABRIC_HEALTH_REPORT & healthReport);
        bool CreateApplicationHealthReport(CommandLineParser & parser, Common::ScopedHeap & heap, FABRIC_HEALTH_REPORT & healthReport);
        bool CreateServiceHealthReport(CommandLineParser & parser, Common::ScopedHeap & heap, FABRIC_HEALTH_REPORT & healthReport);

        bool CreateInternalClusterHealthReport(CommandLineParser & parser, __inout ServiceModel::HealthReport & healthReport);
        bool CreateInternalNodeHealthReport(CommandLineParser & parser, __inout ServiceModel::HealthReport & healthReport);
        bool CreateInternalPartitionHealthReport(CommandLineParser & parser, __inout ServiceModel::HealthReport & healthReport);
        bool CreateInternalReplicaHealthReport(CommandLineParser & parser, __inout ServiceModel::HealthReport & healthReport);
        bool CreateInternalInstanceHealthReport(CommandLineParser & parser, __inout ServiceModel::HealthReport & healthReport);
        bool CreateInternalDeployedApplicationHealthReport(CommandLineParser & parser, __inout ServiceModel::HealthReport & healthReport);
        bool CreateInternalDeployedServicePackageHealthReport(CommandLineParser & parser, __inout ServiceModel::HealthReport & healthReport);
        bool CreateInternalApplicationHealthReport(CommandLineParser & parser, __inout ServiceModel::HealthReport & healthReport);
        bool CreateInternalServiceHealthReport(CommandLineParser & parser, __inout ServiceModel::HealthReport & healthReport);

        bool CreateNodeHealthDeleteReport(CommandLineParser & parser, __inout ServiceModel::HealthReport & healthReport);
        bool CreatePartitionHealthDeleteReport(CommandLineParser & parser, __inout ServiceModel::HealthReport & healthReport);
        bool CreateReplicaHealthDeleteReport(CommandLineParser & parser, __inout ServiceModel::HealthReport & healthReport);
        bool CreateInstanceHealthDeleteReport(CommandLineParser & parser, __inout ServiceModel::HealthReport & healthReport);
        bool CreateDeployedApplicationHealthDeleteReport(CommandLineParser & parser, __inout ServiceModel::HealthReport & healthReport);
        bool CreateDeployedServicePackageHealthDeleteReport(CommandLineParser & parser, __inout ServiceModel::HealthReport & healthReport);
        bool CreateServiceHealthDeleteReport(CommandLineParser & parser, __inout ServiceModel::HealthReport & healthReport);
        bool CreateApplicationHealthDeleteReport(CommandLineParser & parser, __inout ServiceModel::HealthReport & healthReport);
        
        bool CreateNodeHealthQueryList(CommandLineParser & parser, ServiceModel::QueryArgumentMap & queryArgs);
        bool CreatePartitionHealthQueryList(CommandLineParser & parser, ServiceModel::QueryArgumentMap & queryArgs);
        bool CreateReplicaHealthQueryList(CommandLineParser & parser, ServiceModel::QueryArgumentMap & queryArgs);
        bool CreateServiceHealthQueryList(CommandLineParser & parser, ServiceModel::QueryArgumentMap & queryArgs);
        bool CreateApplicationHealthQueryList(CommandLineParser & parser, ServiceModel::QueryArgumentMap & queryArgs);
        bool CreateDeployedApplicationHealthQueryList(CommandLineParser & parser, ServiceModel::QueryArgumentMap & queryArgs);
        bool CreateDeployedServicePackageHealthQueryList(CommandLineParser & parser, ServiceModel::QueryArgumentMap & queryArgs);

        bool ParseClusterHealthPolicy(CommandLineParser & parser, __inout std::unique_ptr<ServiceModel::ClusterHealthPolicy> & nodeHealthPolicy);
        static bool ParseApplicationHealthPolicy(CommandLineParser & parser, __inout std::unique_ptr<ServiceModel::ApplicationHealthPolicy> & applicationHealthPolicy);
        static bool ParseApplicationHealthPolicy(std::wstring const & appPolicyString, __inout std::unique_ptr<ServiceModel::ApplicationHealthPolicy> & applicationHealthPolicy);

        static bool ParseEvents(std::wstring const & eventsString, __out std::map<std::wstring, Common::StringCollection> & events);

        static bool ParseServicePackageActivationId(CommandLineParser & parser, __inout std::wstring & servicePackageActivationId);

        bool ValidateClusterHealthChunk(CommandLineParser & parser, FABRIC_CLUSTER_HEALTH_CHUNK const & publicClusterStateChunk, bool requiresValidation);

        bool ValidateClusterHealth(CommandLineParser & parser, FABRIC_CLUSTER_HEALTH const & publicClusterHealth, bool expectEmpty, bool requiresValidation);
        bool ValidateNodeHealth(CommandLineParser & parser, FABRIC_NODE_HEALTH const & publicNodeHealth, std::wstring const & nodeName, bool expectEmpty, bool requiresValidation);
        bool ValidateReplicaHealth(CommandLineParser & parser, FABRIC_REPLICA_HEALTH const & publicReplicaHealth, Common::Guid const & partitionGuid, FABRIC_REPLICA_ID replicaId, bool expectEmpty, bool requiresValidation);
        bool ValidatePartitionHealth(CommandLineParser & parser, FABRIC_PARTITION_HEALTH const & publicPartitionHealth, Common::Guid const & partitionGuid, bool expectEmpty, bool requiresValidation);
        bool ValidateServiceHealth(CommandLineParser & parser, FABRIC_SERVICE_HEALTH const & publicServiceHealth, std::wstring const & serviceName, bool expectEmpty, bool requiresValidation);
        bool ValidateApplicationHealth(CommandLineParser & parser, FABRIC_APPLICATION_HEALTH const & publicApplicationHealth, std::wstring const & applicationName, bool expectEmpty, bool requiresValidation);
        bool ValidateDeployedApplicationHealth(CommandLineParser & parser, FABRIC_DEPLOYED_APPLICATION_HEALTH const & publicDeployedApplicationHealth, std::wstring const & applicationName, std::wstring const & nodeName, bool expectEmpty, bool requiresValidation);
        
        bool ValidateDeployedServicePackageHealth(
            CommandLineParser & parser, 
            FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH const & publicDeployedServicePackageHealth, 
            std::wstring const & applicationName, 
            std::wstring const & serviceManifestName, 
            std::wstring const & servicePackageActivationId,
            std::wstring const & nodeName, 
            bool expectEmpty, 
            bool requiresValidation);

        bool VerifyEntityEventsAndHealthState(CommandLineParser & parser, bool isEntityPersisted, std::vector<ServiceModel::HealthEvent> const & events, FABRIC_HEALTH_STATE resultAggregatedHealthState, std::vector<ServiceModel::HealthEvaluation> const & unhealthyEvaluations, bool expectEmpty);

        static void PrintUnhealthyEvaluations(std::vector<ServiceModel::HealthEvaluation> const & unhealthyEvaluations);
        static bool VerifyExpectedHealthEvaluations(std::wstring const & expectedHealthEvaluation, std::vector<ServiceModel::HealthEvaluation> const & unhealthyEvaluations);
        static bool VerifyEventEvaluation(std::vector<std::wstring> const & expectedEvaluationTokens, ServiceModel::HealthEvaluationBaseSPtr const & reason);
        static bool VerifyChildrenEvaluation(std::vector<std::wstring> const & expectedEvaluationTokens, std::vector<ServiceModel::HealthEvaluation> const & unhealthyEvaluations, FABRIC_HEALTH_EVALUATION_KIND expectedChildrenKind, ULONG totalCount, BYTE maxUnhealthy, std::wstring const & description);
        static bool VerifyReplicasEvaluation(std::vector<std::wstring> const & expectedEvaluationTokens, ServiceModel::HealthEvaluationBaseSPtr const & reason);
        static bool VerifyNodesEvaluation(std::vector<std::wstring> const & expectedEvaluationTokens, ServiceModel::HealthEvaluationBaseSPtr const & reason);
        static bool VerifyPartitionsEvaluation(std::vector<std::wstring> const & expectedEvaluationTokens, ServiceModel::HealthEvaluationBaseSPtr const & reason);
        static bool VerifyDeployedApplicationsEvaluation(std::vector<std::wstring> const & expectedEvaluationTokens, ServiceModel::HealthEvaluationBaseSPtr const & reason);
        static bool VerifyDeployedServicePackagesEvaluation(std::vector<std::wstring> const & expectedEvaluationTokens, ServiceModel::HealthEvaluationBaseSPtr const & reason);
        static bool VerifyApplicationsEvaluation(std::vector<std::wstring> const & expectedEvaluationTokens, ServiceModel::HealthEvaluationBaseSPtr const & reason);
        static bool VerifyApplicationTypeApplicationsEvaluation(std::vector<std::wstring> const & expectedEvaluationTokens, ServiceModel::HealthEvaluationBaseSPtr const & reason);
        static bool VerifyServicesEvaluation(std::vector<std::wstring> const & expectedEvaluationTokens, ServiceModel::HealthEvaluationBaseSPtr const & reason);
        static bool VerifySystemAppEvaluation(std::vector<std::wstring> const & expectedEvaluationTokens, ServiceModel::HealthEvaluationBaseSPtr const & reason);
        static bool VerifyNodesPerUdEvaluation(std::vector<std::wstring> const & expectedEvaluationTokens, ServiceModel::HealthEvaluationBaseSPtr const & reason);
        static bool VerifyDeployedApplicationsPerUdEvaluation(std::vector<std::wstring> const & expectedEvaluationTokens, ServiceModel::HealthEvaluationBaseSPtr const & reason);
        static bool VerifyDeltaEvaluation(std::vector<std::wstring> const & expectedEvaluationTokens, ServiceModel::HealthEvaluationBaseSPtr const & reason);
        static bool VerifyDeltaPerUdEvaluation(std::vector<std::wstring> const & expectedEvaluationTokens, ServiceModel::HealthEvaluationBaseSPtr const & reason);

        static bool VerifyEventEvaluation(ServiceModel::HealthEvaluationBaseSPtr const & reason);
        static bool VerifyNodeEvaluation(ServiceModel::HealthEvaluationBaseSPtr const & reason);
        static bool VerifyReplicaEvaluation(ServiceModel::HealthEvaluationBaseSPtr const & reason);
        static bool VerifyPartitionEvaluation(ServiceModel::HealthEvaluationBaseSPtr const & reason);
        static bool VerifyServiceEvaluation(ServiceModel::HealthEvaluationBaseSPtr const & reason);
        static bool VerifyDeployedServicePackageEvaluation(ServiceModel::HealthEvaluationBaseSPtr const & reason);
        static bool VerifyDeployedApplicationEvaluation(ServiceModel::HealthEvaluationBaseSPtr const & reason);
        static bool VerifyApplicationEvaluation(ServiceModel::HealthEvaluationBaseSPtr const & reason);
        static bool VerifyNodesEvaluation(ServiceModel::HealthEvaluationBaseSPtr const & reason);
        static bool VerifyReplicasEvaluation(ServiceModel::HealthEvaluationBaseSPtr const & reason);
        static bool VerifyPartitionsEvaluation(ServiceModel::HealthEvaluationBaseSPtr const & reason);
        static bool VerifyServicesEvaluation(ServiceModel::HealthEvaluationBaseSPtr const & reason);
        static bool VerifyDeployedServicePackagesEvaluation(ServiceModel::HealthEvaluationBaseSPtr const & reason);
        static bool VerifyDeployedApplicationsEvaluation(ServiceModel::HealthEvaluationBaseSPtr const & reason);
        static bool VerifyApplicationsEvaluation(ServiceModel::HealthEvaluationBaseSPtr const & reason);
        static bool VerifyApplicationTypeApplicationsEvaluation(ServiceModel::HealthEvaluationBaseSPtr const & reason);
        static bool VerifySystemApplicationEvaluation(ServiceModel::HealthEvaluationBaseSPtr const & reason);
        static bool VerifyUDDeployedApplicationsEvaluation(ServiceModel::HealthEvaluationBaseSPtr const & reason);
        static bool VerifyUDNodesEvaluation(ServiceModel::HealthEvaluationBaseSPtr const & reason);
        static bool VerifyDeltaNodesEvaluation(ServiceModel::HealthEvaluationBaseSPtr const & reason);
        static bool VerifyUDDeltaNodesEvaluation(ServiceModel::HealthEvaluationBaseSPtr const & reason);
        
        bool CheckClusterEntityState(CommandLineParser & parser, Management::HealthManager::HealthManagerReplicaSPtr const & hmPrimary, bool & success);
        bool CheckNodeEntityState(CommandLineParser & parser, Management::HealthManager::HealthManagerReplicaSPtr const & hmPrimary, bool & success);
        bool CheckReplicaEntityState(CommandLineParser & parser, Management::HealthManager::HealthManagerReplicaSPtr const & hmPrimary, bool & success);
        bool CheckPartitionEntityState(CommandLineParser & parser, Management::HealthManager::HealthManagerReplicaSPtr const & hmPrimary, bool & success);
        bool CheckServiceEntityState(CommandLineParser & parser, Management::HealthManager::HealthManagerReplicaSPtr const & hmPrimary, bool & success);
        bool CheckApplicationEntityState(CommandLineParser & parser, Management::HealthManager::HealthManagerReplicaSPtr const & hmPrimary, bool & success);
        bool CheckDeployedApplicationEntityState(CommandLineParser & parser, Management::HealthManager::HealthManagerReplicaSPtr const & hmPrimary, bool & success);
        bool CheckDeployedServicePackageEntityState(CommandLineParser & parser, Management::HealthManager::HealthManagerReplicaSPtr const & hmPrimary, bool & success);
        
        bool CheckHealthStats(__in CommandLineParser & parser, Management::HealthManager::HealthStatisticsUPtr const & healthStats);

        static bool CheckHealthEntityCommonParameters(__in CommandLineParser & parser, Management::HealthManager::HealthEntitySPtr const & entity, __inout Management::HealthManager::AttributesStoreDataSPtr & attributes);
        static bool CheckEntityState(std::wstring const & expectedState, Management::HealthManager::AttributesStoreData const & attributes);
        static bool CheckCleanedUpEntityState(std::wstring const & expectedState);

        static bool GetHMPrimaryWithRetries(__inout Management::HealthManager::HealthManagerReplicaSPtr & hm);

        bool ReportHealthStress(Common::StringCollection const & params);

        FABRIC_HEALTH_REPORT_KIND GetHealthKind(wstring & healthKindString);
    };
}
