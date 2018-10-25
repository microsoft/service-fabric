// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

#include "Constants.h"
#include "IPlacementAndLoadBalancing.h"
#include "PLBSchedulerAction.h"
#include "ServiceType.h"
#include "Node.h"
#include "ServiceDomain.h"
#include "PLBEventSource.h"
#include "PLBConfig.h"
#include "LoadBalancing.PerformanceCounters.h"
#include "Snapshot.h"
#include "Application.h"
#include "BooleanLock.h"
#include "IntervalCounter.h"
#include "MetricGraph.h"
#include "ServicePackage.h"
#include "PLBStatistics.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        #ifdef DBG
            #define DBGASSERT_IF(condition, ...)  { if (condition) { Common::Assert::CodingError(__VA_ARGS__); } }
        #else
            #define DBGASSERT_IF(condition, ...)
        #endif

        class IFailoverManager;
        class Movement;

        class Searcher;
        class CandidateSolution;
        class FailoverUnitMovement;
        class PLBDiagnostics;
        typedef std::unique_ptr<Searcher> SearcherUPtr;
        typedef std::shared_ptr<PLBDiagnostics> PLBDiagnosticsSPtr;

        class PlacementAndLoadBalancing : public IPlacementAndLoadBalancing, public Common::RootedObject
        {
            friend class PlacementCreator;
            friend class ServiceDomain;
            friend class PlacementAndLoadBalancingTestHelper;
            friend class MetricGraph;

            DENY_COPY(PlacementAndLoadBalancing);

        public:
            static Common::Global<PLBEventSource> const PLBTrace;
            static Common::Global<PLBEventSource> const PLBMTrace;
            static Common::Global<PLBEventSource> const CRMTrace;
            static Common::Global<PLBEventSource> const MCRMTrace;

            PlacementAndLoadBalancing(
                IFailoverManager & failoverManager,
                Common::ComponentRoot const& root,
                bool isMaster,
                bool movementEnabled,
                std::vector<NodeDescription> && nodes,
                std::vector<ApplicationDescription> && applications,
                std::vector<ServiceTypeDescription> && serviceTypes,
                std::vector<ServiceDescription> && services,
                std::vector<FailoverUnitDescription> && failoverUnits,
                std::vector<LoadOrMoveCostDescription> && loadAndMoveCosts,
                Client::HealthReportingComponentSPtr const & healthClient,
                Api::IServiceManagementClientPtr const& serviceManagementClient,
                Common::ConfigEntry<bool> const& isSingletonReplicaMoveAllowedDuringUpgradeEntry);

            virtual ~PlacementAndLoadBalancing();

            virtual void SetMovementEnabled(bool constraintCheckEnabled, bool balancingEnabled);

            virtual void UpdateNode(NodeDescription && nodeDescription);

            virtual void UpdateServiceType(ServiceTypeDescription && serviceTypeDescription);
            virtual void DeleteServiceType(std::wstring const& serviceTypeName);

            virtual Common::ErrorCode UpdateApplication(ApplicationDescription && applicationDescription, bool forceUpdate = false);
            virtual void DeleteApplication(std::wstring const& applicationName, bool forceUpdate = false);

            virtual void UpdateClusterUpgrade(bool isUpgradeInProgress, std::set<std::wstring> && completedUDs);

            virtual Common::ErrorCode UpdateService(ServiceDescription && serviceDescription, bool forceUpdate = false);
            virtual void DeleteService(std::wstring const& serviceName);

            virtual void UpdateFailoverUnit(FailoverUnitDescription && failoverUnitDescription);
            virtual void DeleteFailoverUnit(std::wstring && serviceName, Common::Guid fuId);

            virtual void UpdateLoadOrMoveCost(LoadOrMoveCostDescription && loadOrMoveCost);
            virtual Common::ErrorCode ResetPartitionLoad(FailoverUnitId const & failoverUnitId, std::wstring const & serviceName, bool isStateful);

            //Query APIs
            virtual Common::ErrorCode GetClusterLoadInformationQueryResult(ServiceModel::ClusterLoadInformationQueryResult & queryResult);
            virtual Common::ErrorCode GetNodeLoadInformationQueryResult(Federation::NodeId nodeId, ServiceModel::NodeLoadInformationQueryResult & queryResult, bool onlyViolations = false) const;

            virtual ServiceModel::UnplacedReplicaInformationQueryResult GetUnplacedReplicaInformationQueryResult(std::wstring const& serviceName, Common::Guid const& partitionId, bool onlyQueryPrimaries = false);

            virtual Common::ErrorCode GetApplicationLoadInformationResult(std::wstring const & applicationName, ServiceModel::ApplicationLoadInformationQueryResult & result);

            // given a FailoverUnit and two candidate secondary, return the comparision result for promoting to primary
            // ret<0 means node1 is prefered, ret > 0 means node2 is prefered, ret == 0 means they are equal
            virtual int CompareNodeForPromotion(std::wstring const& serviceName, Common::Guid fuId, Federation::NodeId node1, Federation::NodeId node2);

            // given a FailoverUnit and two candiate secondary, return the comparison result for dropping
            // ret<0 means node1 is prefered for drop, ret > 0 means node2 is prefered for drop, ret == 0 means they are equal
            //virtual int CompareNodeForDrop(std::wstring const& serviceName, Common::Guid fuId, Federation::NodeId node1, Federation::NodeId node2);

            virtual void Dispose();

            void Test_WaitForTracingThreadToFinish();
            bool Test_IsPLBStable();
            bool IsPLBStable(Common::Guid const& fuId);

            virtual void OnSafetyCheckAcknowledged(ServiceModel::ApplicationIdentifier const & appId);

            // Triggers the change of target replica count for auto scaling.
            void TriggerUpdateTargetReplicaCount(Common::Guid const& failoverUnitId, int targetReplicaSetSize) const;

            __declspec(property(get = get_PendingAutoScalingRepartitions)) std::vector<AutoScalingRepartitionInfo> & PendingAutoScalingRepartitions;
            std::vector<AutoScalingRepartitionInfo> & get_PendingAutoScalingRepartitions() { return pendingAutoScalingRepartitions_; }

            Common::ErrorCode TriggerSwapPrimary(std::wstring const& serviceName, Common::Guid const& fuId, Federation::NodeId currentPrimary, Federation::NodeId & newPrimary, bool force = false, bool chooseRandom = false );
            Common::ErrorCode TriggerPromoteToPrimary(std::wstring const& serviceName, Common::Guid const& fuId, Federation::NodeId newPrimary);
            Common::ErrorCode TriggerMoveSecondary(std::wstring const& serviceName, Common::Guid const& fuId, Federation::NodeId currentSecondary, Federation::NodeId & newSecondary, bool force = false, bool chooseRandom = false );

            // Get total capacity of the cluster for a given metric (load is not considered)
            int64 GetTotalClusterCapacity(std::wstring const& metricName) const;

            // load is not considered here, SD do the load calculation
            int64 GetTotalApplicationReservedCapacity(std::wstring const& metricName, uint64 appId) const;
            void UpdateTotalApplicationReservedCapacity(std::wstring const& metricName, int capacity);

            virtual Common::ErrorCode ToggleVerboseServicePlacementHealthReporting(bool enabled);

            void ProcessPendingUpdatesPeriodicTask();

            virtual void UpdateAvailableImagesPerNode(std::wstring const& nodeId, std::vector<std::wstring> const& images);

            __declspec (property(get=get_LoadBalancingCounters)) LoadBalancingCountersSPtr const& LoadBalancingCounters;
            LoadBalancingCountersSPtr const& get_LoadBalancingCounters() const { return plbCounterSPtr_; }

            __declspec (property(get=get_IsMaster)) bool IsMaster;
            bool get_IsMaster() const { return isMaster_; }

            __declspec(property(get = get_PLBDiagnosticsSptr)) PLBDiagnosticsSPtr PlbDiagnosticsSPtr;
            PLBDiagnosticsSPtr get_PLBDiagnosticsSptr() const { return plbDiagnosticsSPtr_; }

            __declspec (property(get=get_Trace)) PLBEventSource const& Trace;
            PLBEventSource const& get_Trace() const
            {
                return isMaster_ ? *PLBMTrace : *PLBTrace;
            }

            __declspec (property(get=get_PublicTrace)) PLBEventSource const& PublicTrace;
            PLBEventSource const& get_PublicTrace() const
            {
                return isMaster_ ? *MCRMTrace : *CRMTrace;
            }

            __declspec (property(get = get_GlobalBalancingMovementCounter)) IntervalCounter const& GlobalBalancingMovementCounter;
            IntervalCounter const& get_GlobalBalancingMovementCounter() const { return globalBalancingMovementCounter_; }

            __declspec (property(get = get_GlobalPlacementMovementCounter)) IntervalCounter const& GlobalPlacementMovementCounter;
            IntervalCounter const& get_GlobalPlacementMovementCounter() const { return globalPlacementMovementCounter_; }

            __declspec (property(get = getServiceIdMap)) std::map<std::wstring, uint64> const& ServiceIdMap;
            std::map<std::wstring, uint64> const& getServiceIdMap() const { return serviceToIdMap_; }

            __declspec(property(get = getApplicationMap)) std::map<std::wstring, uint64> const& ApplicationIdMap;
            std::map<std::wstring, uint64> const& getApplicationMap() const { return applicationToIdMap_; }

            //test plb interrupt search by thread
            __declspec(property(get = get_InterruptSearcherRunThread, put = set_InterruptSearcherRunThread)) Common::TimerSPtr & InterruptSearcherRunThread;
            Common::TimerSPtr & get_InterruptSearcherRunThread() { return interruptSearcherRunThread_; }
            void set_InterruptSearcherRunThread(Common::TimerSPtr value) { interruptSearcherRunThread_ = value; }

            //test synchronize plb interrupt search by thread
            __declspec(property(get = get_InterruptSearcherRunFinished, put = set_InterruptSearcherRunFinished)) bool InterruptSearcherRunFinished;
            bool get_InterruptSearcherRunFinished() { return interruptSearcherRunFinished_.load(); }
            void set_InterruptSearcherRunFinished(bool value) { interruptSearcherRunFinished_.store(value); }

            __declspec (property(get = getSearcherSettings)) SearcherSettings const& Settings;
            SearcherSettings const& getSearcherSettings() const { return settings_; }

            _declspec (property(get = getQuorumBasedServicesCache)) std::set<uint64> const& QuorumBasedServicesCache;
            std::set<uint64> const& getQuorumBasedServicesCache() const { return quorumBasedServicesCache_; }

            void Refresh(Common::StopwatchTime now);

            void FMProcessFailoverUnitMovements(std::map<Common::Guid, FailoverUnitMovement> && fuMovements);

            // test only data structure and methods
            class ServiceDomainData
            {
            public:
                ServiceDomainData(std::wstring && domainId, PLBSchedulerAction schedulerAction, std::set<std::wstring> && services)
                    : domainId_(std::move(domainId)), schedulerAction_(schedulerAction), services_(move(services))
                {
                }

                std::wstring domainId_;
                PLBSchedulerAction schedulerAction_;
                std::set<std::wstring> services_;
            };

            void PassMovementsToFM(ServiceDomain::DomainData * searcherDomainData);
            void UpdatePartitionsWithCreation(ServiceDomain::DomainData * searcherDomainData);
            
            // Timers that gives insight into PLB refresh duration
            struct PLBRefreshTimers
            {
                uint64 msPendingUpdatesTime = 0; // Time for processing pending updates
                uint64 msBeginRefreshTime = 0;   // Time for refreshing states
                uint64 msEndRefreshTime = 0;     // Time spent in EndRefresh()
                uint64 msSnapshotTime = 0;       // Time needed to take snapshots
                uint64 msTimeCountForEngine = 0; // Time needed for engine run
                uint64 msAutoScalingTime = 0;    // Time needed for auto scaling operations
                uint64 msRemainderTime = 0;      // Everything else
                uint64 msRefreshTime = 0;        // Total time spent in Refresh() 

                void Reset();
                void CalculateRemainderTime();
            };

            __declspec (property(get = getRefreshTimers)) PLBRefreshTimers const& RefreshTimers;
            PLBRefreshTimers const& getRefreshTimers() const { return plbRefreshTimers_; }

            void PassMovementsToFM(vector<ServiceDomain::DomainData> & searcherDataList);
            void UpdatePartitionsWithCreation(vector<ServiceDomain::DomainData> & searcherDataList);

            std::vector<ServiceDomainData> GetServiceDomains();

            Snapshot TakeSnapShot();

            // Called from service domain to determine the time for next action.
            void UpdateNextActionPeriod(Common::TimeSpan nextActionPeriod);

            void OnDroppedPLBMovement(Common::Guid const& failoverUnitId, PlbMovementIgnoredReasons::Enum reason, Common::Guid const& decisionId);
            void OnDroppedPLBMovements(std::map<Common::Guid, FailoverUnitMovement> && droppedMovements, PlbMovementIgnoredReasons::Enum reason, Common::Guid const& decisionId);
            void OnExecutePLBMovement(Common::Guid const & partitionId);
            void OnFMBusy();

            inline uint64 GetNodeIndex(Federation::NodeId const& nodeId) const
            {
                auto itNodeIndex = nodeToIndexMap_.find(nodeId);
                return itNodeIndex == nodeToIndexMap_.end() ? UINT64_MAX : itNodeIndex->second;
            }
            uint64 GetServiceId(std::wstring const& serviceName) const;

            static std::wstring UDsToString(std::set<std::wstring> UDs);

            bool IsClusterUpgradeInProgress();

            //returns nullptr in case the application is not present!
            Application const * GetApplicationPtrCallerHoldsLock(uint64 applicationId) const;

            Common::TextTraceWriter const WriteWarning;

        private:

            class TracingMovementsJob
            {
                DENY_COPY(TracingMovementsJob);

            public:
                TracingMovementsJob();
                TracingMovementsJob(ServiceDomain::DomainData const* searcherDomainData);

                TracingMovementsJob(TracingMovementsJob && other);
                TracingMovementsJob & operator=(TracingMovementsJob && other);

                bool ProcessJob(PlacementAndLoadBalancing & plb);

            private:
                Common::Stopwatch tracingMovementsStopwatch_;
                std::vector<pair<Common::Guid, FailoverUnitMovement>> domainFTMovements_;
            };

            class TracingMovementsJobQueue : public Common::JobQueue<TracingMovementsJob, PlacementAndLoadBalancing>
            {
                DENY_COPY(TracingMovementsJobQueue);

            public:

                TracingMovementsJobQueue(
                    std::wstring const & name,
                    PlacementAndLoadBalancing & root,
                    bool forceEnqueue,
                    int maxThreads,
                    bool& tracingJobQueueFinished,
#if !defined(PLATFORM_UNIX)
                    Common::ExclusiveLock& testTracingLock,
#else
                    Common::Mutex& testTracingLock,
#endif
                    Common::ConditionVariable& testTracingBarrier);

            protected:

                void OnFinishItems();

            private:

                bool& tracingJobQueueFinished_;
#if !defined(PLATFORM_UNIX)
                Common::ExclusiveLock& testTracingLock_;
#else
                Common::Mutex& testTracingLock_;
#endif
                Common::ConditionVariable& testTracingBarrier_;
            };

            typedef std::unique_ptr<TracingMovementsJobQueue> TracingMovementsJobQueueUPtr;
            typedef std::map<ServiceDomain::DomainId, ServiceDomain> ServiceDomainTable;
            typedef std::map<std::wstring, std::vector<ServiceDomainTable::iterator>> DependedServiceToDomainType;

            class ServiceDomainStats;
            void AddMetricToDomain(std::wstring const& metricName, ServiceDomainTable::iterator const& itDomain, bool incrementAppCount = false);
            void QueueDomainChange(ServiceDomain::DomainId const& domains, bool merge = true);
            void ExecuteDomainChange(bool mergeOnly = false);
            void ChangeServiceMetricsForApplication(uint64 ApplicationId, std::wstring ServiceName, std::vector<ServiceMetric> const& metrics, std::vector<ServiceMetric> const& originalMetrics = std::vector<ServiceMetric>());
            std::pair<Common::ErrorCode, bool> InternalUpdateService(ServiceDescription && serviceDescription, bool forceUpdate, bool traceDetail = true, bool updateMetricConnections=true, bool skipServicePackageUpdate = false);
            ServiceDomain::DomainId GenerateUniqueDomainId(ServiceDescription const& serviceName);
            ServiceDomain::DomainId GenerateDefaultDomainId(std::wstring const & metricName);
            ServiceDomain::DomainId GenerateUniqueDomainId(std::wstring const& metricName);
            void UpdateServiceDomainId(ServiceDomainTable::iterator itDomain);
            void AddServiceToDomain(ServiceDescription && serviceDescription, ServiceDomainTable::iterator itDomain);

            bool AddApplicationToDomain(
                ApplicationDescription const& applicationDescription,
                ServiceDomainTable::iterator itDomain,
                vector<wstring> const& metrics,
                bool changedAppMetrics = false);

            ServiceDomainTable::iterator MergeDomains(std::set<ServiceDomain::DomainId> const& domains);
            void SplitDomain(ServiceDomainTable::iterator itCurrentDomain);
            // Permanent delete is used to indicate that service is deleted permanently (not for service update).
            bool InternalDeleteService(std::wstring const& serviceName, bool permanentDelete, bool assertFailoverUnitEmpty = true);


            virtual Common::ErrorCode InternalUpdateServicePackageCallerHoldsLock(ServicePackageDescription && description, bool & changed);
            virtual void InternalDeleteServicePackageCallerHoldsLock(ServiceModel::ServicePackageIdentifier const & servicePackageIdentifier);

            void InternalUpdateFailoverUnit(FailoverUnitDescription && failverUnitDescription);
            void InternalDeleteFailoverUnit(std::wstring && serviceName, Common::Guid fuId);
            bool ProcessUpdateFailoverUnit(FailoverUnitDescription && failverUnitDescription, Common::StopwatchTime timeStamp);

            void InternalUpdateLoadOrMoveCost(LoadOrMoveCostDescription && loadOrMoveCostDescription);
            void ProcessUpdateLoadOrMoveCost(LoadOrMoveCostDescription && loadOrMoveCostDescription, Common::StopwatchTime timeStamp);

            void ProcessUpdateNode(NodeDescription && nodeDescription, Common::StopwatchTime timeStamp);
            void ProcessUpdateNodeImages(Federation::NodeId const& nodeId, vector<wstring>&& nodeImages);

            void BeginRefresh(std::vector<ServiceDomain::DomainData> & dataList, ServiceDomainStats & stats, Common::StopwatchTime refreshTime);
            void RunSearcher(ServiceDomain::DomainData * searcherDomainData,
                ServiceDomainStats const& stats,
                size_t& totalOperationCount,
                size_t& currentPlacementMovementCount,
                size_t& currentBalancingMovementCount);
            void EndRefresh(ServiceDomain::DomainData * searcherDomainData, Common::StopwatchTime refreshTime);

            void TracePeriodical(ServiceDomainStats& stats, Common::StopwatchTime refreshTime);

            // Processes pending updates. Caller needs to acquire write lock on global lock (lock_).
            // This function will acquire write lock on bufferUpdateLock_ variable
            // This function does not stop the searcher!
            bool ProcessPendingUpdatesCallerHoldsLock(Common::StopwatchTime now);

            // Checks if there are any updates to process, and calls function to process them.
            // This function will acquire exclusive lock on global lock (lock_) if needed.
            // This function will stop the searcher if needed!
            void CheckAndProcessPendingUpdates();

            // Calculate how many replicas can be moved based on absolute and percentage thresholds.
            size_t GetMovementThreshold(size_t existingReplicaCount, size_t absoluteThreshold, double percentageThreshold);

            ServiceType const& GetServiceType(ServiceDescription const& desc) const;

            Application const& GetApplication(uint64 applicationId) const;

            void ForEachDomain(std::function<void(ServiceDomain &)> processor);
            void ForEachDomain(std::function<void(ServiceDomain const&)> processor) const;

            bool IsDisposed() const;
            void StopSearcher();

            bool DoAllReplicasMatch(std::vector<ReplicaDescription> const& replicas) const;

            void UpdateNodePropertyConstraintKeyParams();

            void AddFailoverUnitMovement(
                Movement const& movement,
                PLBSchedulerActionType::Enum schedulerAction,
                FailoverUnitMovementTable & movementList) const;

            void AddDependedServiceToDomain(std::wstring const& service, ServiceDomainTable::iterator itDomain);
            void DeleteDependedServiceToDomain(DependedServiceToDomainType::iterator itDependedService, ServiceDomainTable::iterator itDomain);

            Common::ErrorCode TriggerSwapPrimaryHelper(
                std::wstring const& serviceName,
                FailoverUnit const & failoverUnit,
                Federation::NodeId currentPrimary,
                Federation::NodeId newPrimary,
                std::vector<FailoverUnitMovement::PLBAction>& actions,
                bool force,
                PLBSchedulerActionType::Enum schedulerAction);

            Common::ErrorCode TriggerPromoteToPrimaryHelper(
                std::wstring const& serviceName,
                FailoverUnit const & failoverUnit,
                Federation::NodeId newPrimary,
                std::vector<FailoverUnitMovement::PLBAction>& actions,
                bool force,
                PLBSchedulerActionType::Enum schedulerAction);

            Common::ErrorCode TriggerMoveSecondaryHelper(
                std::wstring const& serviceName,
                FailoverUnit const & failoverUnit,
                Federation::NodeId currentSecondary,
                Federation::NodeId newSecondary,
                std::vector<FailoverUnitMovement::PLBAction>& actions,
                bool force,
                PLBSchedulerActionType::Enum schedulerAction,
                bool isStatefull);

            Common::ErrorCode FindRandomReplicaHelper(
                std::wstring const& serviceName,
                FailoverUnit const & failoverUnit,
                Federation::NodeId currentNode,
                Federation::NodeId & newNode,
                std::vector<FailoverUnitMovement::PLBAction>& actions,
                ReplicaRole::Enum role,
                bool force,
                PLBSchedulerActionType::Enum schedulerAction,
                bool isStatefull);

            Common::ErrorCode SnapshotConstraintChecker(std::wstring const& serviceName,
                                                        Common::Guid const& fuId,
                                                        Federation::NodeId currentNode,
                                                        Federation::NodeId newNode,
                                                        ReplicaRole::Enum role);

            std::pair<Common::ErrorCode, std::vector<Federation::NodeId>> FindRandomNodes(std::wstring const& serviceName,
                                                        FailoverUnit const & failoverUnit,
                                                        Federation::NodeId currentNodeId,
                                                        ReplicaRole::Enum replicaRole,
                                                        Common::Random randomEng,
                                                        bool force);

            // detect if affinity chain exists
            bool IsAffinityChain(ServiceDescription & serviceDescription) const;
            // Used when creating new service
            bool CheckClusterResource(ServiceDescription & serviceDescription) const;
            // Used when creating or updating an application
            bool CheckClusterResourceForReservation(ApplicationDescription const& appDescription) const;
            bool CheckConstraintKeyDefined(ServiceDescription & serviceDescription);


            // Recompute total capacity of the cluster for a given metric based on information from nodes
            int64 RecomputeTotalClusterCapacity(std::wstring metricName, Federation::NodeId excludedNode) const;

            // Update cluster capacity when node is updated
            void UpdateTotalClusterCapacity(const NodeDescription &oldDescription, const NodeDescription &newDescription, bool isAdded);

            void UpdateAppReservedCapacity(ApplicationDescription const& desc, bool isAdd);
            void AddAppReservedMetricCapacity(std::wstring const& metricName, uint capacity);
            void DeleteAppReservedMetricCapacity(std::wstring const& metricName, uint capacity);

            void TrackDroppedPLBMovements();

            void UpdateQuorumBasedServicesCache(ServiceDomain::DomainData* searcherDomainData);

            //Logic for calculating number of allowed movements...
            size_t GetAllowedMovements(PLBSchedulerAction action,
                size_t existingReplicaCount,
                size_t currentPlacementMovementCount,
                size_t currentBalancingMovementCount);

            void UpdateUseSeparateSecondaryLoadConfig(bool newUseSeparateSecondaryLoad);

            void EmitCRMOperationTrace(
                Common::Guid const & failoverUnitId,
                Common::Guid const & decisionId,
                PLBSchedulerActionType::Enum schedulerActionType,
                FailoverUnitMovementType::Enum failoverUnitMovementType,
                std::wstring const & serviceName,
                Federation::NodeId const & sourceNode,
                Federation::NodeId const & destinationNode) const;

            void EmitCRMOperationIgnoredTrace(
                Common::Guid const & failoverUnitId,
                PlbMovementIgnoredReasons::Enum movementIgnoredReason,
                Common::Guid const & decisionId) const;

            bool const isMaster_;

            // TODO: add lock to ServiceDomain class, and this lock only controls global information update
            Common::RwLock lock_;
            mutable Common::RwLock snapshotLock_;
            Common::RwLock droppedMovementLock_;
            BooleanLock bufferUpdateLock_;
            Common::TimerSPtr timer_;
            Common::TimerSPtr processPendingUpdatesTimer_;

            Common::atomic_bool disposed_;

            SearcherUPtr searcher_;
            Common::atomic_bool stopSearching_;

            Common::atomic_bool constraintCheckEnabled_;
            Common::atomic_bool balancingEnabled_;

            Common::ConfigEntry<bool> const& isSingletonReplicaMoveAllowedDuringUpgradeEntry_;

            IFailoverManager & failoverManager_;

            int randomSeed_;

            std::map<Federation::NodeId, uint64> nodeToIndexMap_;
            std::vector<Node> nodes_;
            size_t upNodeCount_;

            std::map<uint64, ServicePackage> servicePackageTable_;
            std::map<ServiceModel::ServicePackageIdentifier, uint64> servicePackageToIdMap_;

            uint64 nextServicePackageIndex_;

            ServiceDomain::DomainId rgDomainId_;

            std::map<std::wstring, ServiceType> serviceTypeTable_;

            Uint64UnorderedMap<Application> applicationTable_;

            uint64 nextServiceId_;
            std::map<std::wstring, uint64> serviceToIdMap_;

            uint64 nextApplicationId_;
            std::map<std::wstring, uint64> applicationToIdMap_;

            // mapping from service name to service domain, it contains all services in the system
            Uint64UnorderedMap<ServiceDomainTable::iterator> serviceToDomainTable_;
            //The class that stores connections between metrics, and is responsible for deciding to split/merge servicedomains...
            MetricGraph metricConnections_;
            //Contains all the domains that need to be merged.
            std::set<ServiceDomain::DomainId> domainsToMerge_;
            //Contains all the domains that need to be split.
            ServiceDomain::DomainId domainToSplit_;

            // mapping from application name to service domain, it contains all services in the system
            Uint64UnorderedMap<ServiceDomainTable::iterator> applicationToDomainTable_;

            // cache for cluster capacity per metric
            std::map<std::wstring, uint64> clusterMetricCapacity_;

            // mapping from service name to service domains, which contain services that don't exist but are depended by existing services
            DependedServiceToDomainType dependedServiceToDomainTable_;

            // mapping from metric name to service domain, it contains all metrics in the system
            std::map<std::wstring, ServiceDomainTable::iterator> metricToDomainTable_;

            // mapping from service domain id (name of the first service in the domain) to service domain
            ServiceDomainTable serviceDomainTable_;

            std::vector<std::pair<FailoverUnitDescription, Common::StopwatchTime>> pendingFailoverUnitUpdates_;

            std::vector<std::pair<LoadOrMoveCostDescription, Common::StopwatchTime>> pendingLoadsOrMoveCosts_;

            std::vector<std::pair<NodeDescription, Common::StopwatchTime>> pendingNodeUpdates_;

            //Dropped and Executed movement queues operate under the same lock -- droppedMovementLock_
            std::queue<tuple<Common::Guid, PlbMovementIgnoredReasons::Enum, Common::Guid>> droppedMovementQueue_;
            std::queue<Common::Guid> executedMovementQueue_;

            std::map<std::wstring, std::wstring> nodePropertyConstraintKeyParams_;

            std::map<std::wstring, Common::ExpressionSPtr> serviceNameToPlacementConstraintTable_;
            Common::ExpressionSPtr expressionCache_;

            bool verbosePlacementHealthReportingEnabled_;

            Common::StopwatchTime lastPeriodicTrace_;
            uint64 nextApplicationToBeTraced_;
            uint64 nextNodeToBeTraced_;
            int nextServiceToBeTraced_;

            LoadBalancingCountersSPtr plbCounterSPtr_;

            Client::HealthReportingComponentSPtr healthClient_;

            PLBDiagnosticsSPtr plbDiagnosticsSPtr_;

            std::shared_ptr<std::pair<Snapshot,Snapshot>> snapshotsOnEachRefresh_;

            Common::StopwatchTime lastStatisticsTrace_;
            PLBStatistics plbStatistics_;

            Common::TimeSpan nextActionPeriod_;
            std::wstring deletedDomainId_;

            // DomainId for metrics without domain
            ServiceDomain::DomainId defaultDomainId_;
            
            // metric to total reserved load map for all applications
            // This is used before application added to service domain
            std::map<std::wstring, int64> totalReservedCapacity_;

            // Dummy service type used when actual service type cannot be found so that search can proceed
            ServiceType dummyServiceType_;

            // Counters for throttling....
            IntervalCounter globalBalancingMovementCounter_;
            IntervalCounter globalPlacementMovementCounter_;

            //test plb interrupt search
            Common::TimerSPtr interruptSearcherRunThread_;
            Common::atomic_bool interruptSearcherRunFinished_;

            std::set<std::wstring> upgradeCompletedUDs_;
            Common::atomic_bool clusterUpgradeInProgress_;

            //cached configs
            SearcherSettings settings_;

            //for these apps we need to do safety checks for RG
            std::set<uint64> applicationsInUpgradeCheckRg_;

            //for these apps the safety checks for RG are fine
            std::set<ServiceModel::ApplicationIdentifier> applicationsSafeToUpgrade_;

            TracingMovementsJobQueueUPtr tracingMovementsJobQueueUPtr_;

            // Timers that gives insight into PLB refresh duration
            PLBRefreshTimers plbRefreshTimers_;

            // synchronization for the test as TestFM can create multiple PLB objects
            bool tracingJobQueueFinished_;

            // Caches ids of services that use quorum based logic. 
            // Caching is done only if all three conditions are satisfied:
            //      + PLB config QuorumBasedReplicaDistributionPerUpgradeDomains is false
            //      + PLB config QuorumBasedReplicaDistributionPerFaultDomains is false
            //      + PLB config QuorumBasedLogicAutoSwitch is true
            // In other scenarios, there is no need to cache services.
            std::set<uint64> quorumBasedServicesCache_;

#if !defined(PLATFORM_UNIX)
            Common::ExclusiveLock testTracingLock_;
#else
            Common::Mutex testTracingLock_;
#endif
            Common::ConditionVariable testTracingBarrier_;

            //
            // Private methods and members for auto scaling
            //

            // Creates calls to perform repartition of services.
            // this method is executed outside of the lock.
            void ProcessAutoScalingRepartitioning();

            // Called when client call is done.
            void OnAutoScalingRepartitioningComplete(
                Common::AsyncOperationSPtr const& operation,
                uint64_t serviceId);

            void ProcessFailedAutoScalingRepartitionsCallerHoldsLock();

            // Client that is used for auto scaling repartitioning.
            Api::IServiceManagementClientPtr serviceManagementClient_;

            // Pending auto scale operations
            std::vector<AutoScalingRepartitionInfo> pendingAutoScalingRepartitions_;

            // Pending auto scale errors (repartitioning failed)
            std::vector<std::pair<uint64_t, Common::ErrorCode>> pendingAutoScalingFailedRepartitions_;

            // Protecting access to pendingAutoScalingFailedRepartitions_
            Common::RwLock autoScalingFailedOperationsVectorsLock_;

            // Failed operations for processing during current referesh.
            // We don't want to take autoScalingFailedOperationsVectorsLock_ during BeginRefresh.
            std::vector<std::pair<uint64_t, Common::ErrorCode>> pendingAutoScalingFailedRepartitionsForRefresh_;

            // Available images on nodes
            std::map<Federation::NodeId, std::vector<std::wstring>> pendingNodeImagesUpdate_;
        };
    }
}
