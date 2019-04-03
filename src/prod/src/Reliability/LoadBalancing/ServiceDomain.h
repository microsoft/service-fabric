// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Service.h"
#include "FailoverUnit.h"
#include "ServiceDomainMetric.h"
#include "TraceHelpers.h"
#include "PartitionClosure.h"
#include "PLBScheduler.h"
#include "MovePlan.h"
#include "SystemState.h"
#include "client/ClientPointers.h"
#include "ApplicationLoad.h"
#include "Hasher.h"
#include "DynamicBitSet.h"
#include "ReservationLoad.h"
#include "ServicePackageNode.h"
#include "AutoScaler.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class LoadOrMoveCostDescription;
        class PLBEventSource;
        class ServicePackageNode;
        class Placement;
        typedef std::unique_ptr<Placement> PlacementUPtr;
        class Node;
        class Checker;
        typedef std::unique_ptr<Checker> CheckerUPtr;
        class BalanceChecker;
        class PLBDiagnostics;
        typedef std::unique_ptr<BalanceChecker> BalanceCheckerUPtr;
        typedef std::shared_ptr<PLBDiagnostics> PLBDiagnosticsSPtr;
        class ServicePackageDescription;
        class FailoverUnitMovement;
        typedef std::map<Common::Guid, FailoverUnitMovement> FailoverUnitMovementTable;

        class PlacementAndLoadBalancing;
        class DecisionToken;

        class ServiceDomain
        {
            friend class SystemState;
            friend class PlacementCreator;
            friend class AutoScaler;

            DENY_COPY(ServiceDomain);

        public:
            typedef std::wstring DomainId;

            //data structured used for searcher
            class DomainData
            {
                DENY_COPY(DomainData);
            public:
                DomainData(ServiceDomain::DomainId && id,
                    PLBSchedulerAction action,
                    SystemState && state,
                    FailoverUnitMovementTable && movementTable)
                    : domainId_(std::move(id)),
                    action_(action),
                    state_(std::move(state)),
                    isInterrupted_(false),
                    newAvgStdDev_(0),
                    movementTable_(std::move(movementTable)),
                    interruptTime_(Common::Stopwatch::Now())
                {
                }

                DomainData(DomainData && other)
                    : domainId_(std::move(other.domainId_)),
                    action_(other.action_),
                    state_(std::move(other.state_)),
                    isInterrupted_(other.isInterrupted_),
                    newAvgStdDev_(other.newAvgStdDev_),
                    movementTable_(std::move(other.movementTable_)),
                    dToken_(std::move(other.dToken_)),
                    interruptTime_(std::move(other.interruptTime_))
                {
                }

                __declspec(property(get = get_State)) SystemState const& State;
                SystemState const& get_State() const { return state_; }

                DomainId domainId_;
                PLBSchedulerAction action_;
                SystemState state_;
                bool isInterrupted_;
                double newAvgStdDev_;
                std::vector<Common::Guid> partitionsWithCreations_;
                FailoverUnitMovementTable movementTable_;
                DecisionToken dToken_;
                Common::StopwatchTime interruptTime_;
            };


            ServiceDomain(DomainId && id, PlacementAndLoadBalancing & plb);

            ServiceDomain(ServiceDomain && other);

            __declspec (property(get=get_DomainId)) DomainId const& Id;
            DomainId const& get_DomainId() const { return domainId_; }

            __declspec (property(get=get_Scheduler)) PLBScheduler const& Scheduler;
            PLBScheduler const& get_Scheduler() const { return scheduler_; }

            __declspec (property(get = get_Services)) Uint64UnorderedMap<Service> & Services;
            Uint64UnorderedMap<Service> & get_Services() { return serviceTable_; }

            //Map from parent Service Name to set of all child Service Names
            __declspec (property(get = get_ChildServices)) std::map<std::wstring, std::set<std::wstring>> const& ChildServices;
            std::map<std::wstring, std::set<std::wstring>> const& get_ChildServices() const { return childServices_; }

            __declspec (property(get = get_FailoverUnits)) GuidUnorderedMap<FailoverUnit> const& FailoverUnits;
            GuidUnorderedMap<FailoverUnit> const& get_FailoverUnits() const { return failoverUnitTable_; }

            __declspec (property(get=get_Metrics)) std::map<std::wstring, ServiceDomainMetric> const& Metrics;
            std::map<std::wstring, ServiceDomainMetric> const& get_Metrics() const { return metricTable_; }

            __declspec (property(get=get_PartitionsInUpgrade)) std::set<Common::Guid> const& PartitionsInUpgrade;
            std::set<Common::Guid> const& get_PartitionsInUpgrade() const { return partitionsWithInUpgradeReplicas_; }

            __declspec (property(get = get_LastApplicationTraced, put = put_LastApplicationTraced)) uint64 LastApplicationTraced;
            uint64 get_LastApplicationTraced() const { return lastTracedApplication_; }
            void put_LastApplicationTraced(uint64 lastTracedApplication) { lastTracedApplication_ = lastTracedApplication; }

            __declspec (property(get = get_PartitionsInAppUpgradeCount)) uint64 PartitionsInAppUpgradeCount;
            uint64 get_PartitionsInAppUpgradeCount() const { return partitionsInAppUpgrade_; }

            __declspec (property(get = get_AutoScalerComponent)) AutoScaler & AutoScalerComponent;
            AutoScaler & get_AutoScalerComponent() { return autoScaler_; }

            __declspec (property(get = get_InBuildCountPerNode)) Uint64UnorderedMap<uint64_t> const& InBuildCountPerNode;
            Uint64UnorderedMap<uint64_t> const& get_InBuildCountPerNode() const { return inBuildCountPerNode_; }

            size_t BalancingMovementCount() const;
            size_t PlacementMovementCount() const;

            static std::wstring GetDomainIdPrefix(DomainId const domainId);

            static std::wstring GetDomainIdSuffix(DomainId const domainId);

            // update methods
            void AddMetric(std::wstring,bool incrementAppCount = false);
            bool RemoveMetric(std::wstring, bool decrementAppCount = false);
            bool CheckMetrics();
            void AddService(ServiceDescription && serviceDescription);
            void DeleteService(uint64 serviceId,
                std::wstring const& serviceName,
                uint64 & applicationId,
                std::vector<std::wstring> & deletedMetrics,
                bool & depended,
                std::wstring & affinitizedService,
                bool assertFailoverUnitEmpty = true);
            bool HasDependentService(std::wstring const& dependedService) const;
            bool HasDependedService(std::wstring const& dependentService) const;
            void MergeDomain(ServiceDomain && other);
            void AddFailoverUnit(FailoverUnit && failoverUnitToAdd, Common::StopwatchTime timeStamp);
            bool UpdateFailoverUnit(FailoverUnitDescription && failoverUnitDescription, Common::StopwatchTime timeStamp, bool traceDetail = true);
            void UpdateLoadOrMoveCost(LoadOrMoveCostDescription && loadOrMoveCost, Common::StopwatchTime timeStamp);
            void RemoveFailoverUnit(Common::Guid fuId) { failoverUnitTable_.erase(fuId); }

            void UpdateFailoverUnitWithMoves(FailoverUnitMovement const& movement);

            void SetMovementEnabled(bool constraintCheckEnabled, bool balancingEnabled, bool isDummyPLB, Common::StopwatchTime timeStamp);

            // given a FailoverUnit and two candidate secondary, return the comparison result for promoting to primary
            // ret<0 means node1 is prefered, ret > 0 means node2 is prefered, ret == 0 means they are equal
            int CompareNodeForPromotion(Common::Guid fuId, Node const& node1, Node const& node2) const;

            // given a FailoverUnit and two candidate secondary, return the comparison result for dropping
            // ret<0 means node1 is prefered for drop, ret > 0 means node2 is prefered for drop, ret == 0 means they are equal
            //int CompareNodeForDrop(Common::Guid fuId, Federation::NodeId node1, Federation::NodeId node2) const;

            // event methods
            void OnNodeUp(uint64 node, Common::StopwatchTime timeStamp);
            void OnNodeDown(uint64 node, Common::StopwatchTime timeStamp);
            void OnNodeChanged(uint64 node, Common::StopwatchTime timeStamp);
            void OnDomainInterrupted(Common::StopwatchTime timeStamp);

            void OnServiceTypeChanged(std::wstring const& serviceTypeName);

            void UpdateFailoverUnitWithCreationMoves(std::vector<Common::Guid> & partitionsWithCreations);
            void OnMovementGenerated(
                Common::StopwatchTime timeStamp,
                Common::Guid decisionGuid,
                double newAvgStdDev = -1.0,
                FailoverUnitMovementTable && movementList = FailoverUnitMovementTable());

            FailoverUnitMovementTable GetMovements(Common::StopwatchTime now);

            DomainData TakeSnapshot(PLBDiagnosticsSPtr const& plbDiagnosticsSPtr);

            DomainData RefreshStates(Common::StopwatchTime now, PLBDiagnosticsSPtr const& plbDiagnosticsSPtr);

            void TraceNodeLoads(Common::StopwatchTime const& now);

            PartitionClosureUPtr GetPartitionClosure(PartitionClosureType::Enum closureType) const;
            BalanceCheckerUPtr GetBalanceChecker(PartitionClosureUPtr const& partitionClosure) const;
            PlacementUPtr GetPlacement(PartitionClosureUPtr const& partitionClosure, BalanceCheckerUPtr && balanceChecker, std::set<Common::Guid> && throttledPartitions) const;

            void UpdateContraintCheckClosure(std::set<Common::Guid> && partitions);
            int64 GetTotalRemainingResource(std::wstring const& metricName, double nodeBufferPercentage) const;

            // Total Reserved Load Used when load used by applications is L, and reservation is R:
            // If L < R  ==> TotalReservedLoadUsed == L
            // If L >= R ==> TotalReservedLoadUsed == R
            // UpdateTotalReservedLoadUsed guarantees that these two rules will always be enforced
            int64 GetTotalReservedLoadUsed(std::wstring const& metricName) const;
            void UpdateTotalReservedLoadUsed(std::wstring const& metricName, int64 loadChange);

            // The amount of time until next action is required for this service domain
            Common::TimeSpan GetNextActionInterval(Common::StopwatchTime now) { return scheduler_.GetNextRefreshInterval(now); }

            void UpdateDomainId(DomainId const& Id) { domainId_ = Id; }
            void CheckAndUpdateAppPartitions(uint64 applicationId);
            void AddAppPartitionsToPartialClosure(uint64 applicationId);

            bool ApplicationHasLoad(uint64 applicationId) const { return applicationLoadTable_.find(applicationId) != applicationLoadTable_.end(); }
            ApplicationLoad const& GetApplicationLoad(uint64 applicationId) const;
            // Adds or removes reserved load used to Service Domain tables. Please look at comments in the implementation before using.
            void UpdateApplicationReservedLoad(ApplicationDescription const& applicationDescription, bool isAdd);            

            // when application reservation is updated - for consistency of reservationLoadTable_
            // add and remove all failoverUnits
            void UpdateApplicationFailoverUnitsReservation(ApplicationDescription const& applicationDescription, bool isAdd);

            Service const& GetService(uint64 serviceId) const;
            Service & GetService(uint64 serviceId);

            // Add loads to nodes for this replica set
            void AddNodeLoad(Service const& service, FailoverUnit const& failoverUnit, std::vector<ReplicaDescription> const& replica, bool isLoadOrMoveCostChange = false);
            // Remove load from nodes for this replica set
            void DeleteNodeLoad(Service const& service, FailoverUnit const& failoverUnit, std::vector<ReplicaDescription> const& replica);

            // get all applications reservations on this node
            int64 GetReservationNodeLoad(std::wstring const& metricName, Federation::NodeId nodeId);

            size_t GetApplicationLoadTableSize() const { return applicationLoadTable_.size(); }

            // Remove application entry from the application partitions table
            void RemoveApplicationFromApplicationPartitions(uint64 applicationId);

            // Used to update reserved load when new application is added to this domain.
            void UpdateReservationForNewApplication(ApplicationDescription const& appDescription);

            bool IsSafeToUpgradeSP(uint64 servicePackageId, ServicePackageDescription const & servicePackage);

            // This will update the load only for services that have SHARED service package activation mode
            void UpdateServicePackageLoad(uint64 serviceId, bool isAdd);

            uint GetServicePackageRgReservation(uint64 servicePackageId, wstring const & metricName) const;

            void UpdateApplicationNodeCounts(uint64 applicationId, std::vector<ReplicaDescription> const& replicas, bool isAdd);
            void UpdateServicePackagePlacement(Service const & service, std::vector<ReplicaDescription> const& replicas, bool isAdd);

            void RemoveServicePackageInfo(uint64 servicePackageId);

            void Test_GetServicePackageReplicaCountForGivenNode(
                uint64 servicePackageId,
                int& exclusiveRegularReplicaCount,
                int& sharedRegularReplicaCount,
                int& sharedDisappearReplicaCount,
                Federation::NodeId nodeId);

        private:
            void UpdateNodeToFailoverUnitMapping(
                FailoverUnit const& failoverUnit,
                std::vector<ReplicaDescription> const& oldReplicas,
                std::vector<ReplicaDescription> const& newReplicas,
                bool isStateful);

            void UpdatePendingLoadsOrMoveCosts();

            void ComputeServiceBlockList(Service & service) const;

            bool IsServiceBlockNode(NodeDescription const& nodeDescription, Common::Expression & constraintExpression, std::wstring const& placementConstraints, bool forPrimary = false, bool traceDetail = true) const;
            bool IsPrimaryReplicaBlockNode(NodeDescription const& nodeDescription, Common::Expression & constraintExpression, std::wstring const& placementConstraints) const;

            bool IsServiceAffectingBalancing(Service const& service) const { return !service.ServiceDesc.OnEveryNode; }

            void UpdateServiceDomainMetricBlockList(Service const& service, uint64 nodeIndex, int delta);

            void UpdateConstraintCheckClosure() const;
            void ComputeTransitiveClosure(
                std::set<Common::Guid> const& initialPartitions,
                bool walkParentToChild,
                std::set<uint64> & relatedServices,
                std::set<Common::Guid> & relatedPartitions,
                std::set<uint64> & relatedApplications) const;
            void ComputeApplicationClosure(
                std::set<Common::Guid> & relatedPartitions,
                std::set<uint64> & relatedServices,
                std::set<uint64> & relatedApplications) const;
            std::set<uint64> GetApplicationsInSingletonReplicaUpgrade(std::set<uint64> & relatedApplications) const;

            void UpdateServiceBlockList(Service & service);
            void AddToPartitionStatistics(FailoverUnit const& failoverUnit);
            void RemoveFromPartitionStatistics(FailoverUnit const& failoverUnit);
            void AddToReplicaDifferenceStatistics(Common::Guid fuId, int replicaDiff);
            void RemoveFromReplicaDifferenceStatistics(Common::Guid fuId, int replicaDiff);
            void AddToUpgradeStatistics(FailoverUnitDescription const & newDescription);
            void RemoveFromUpgradeStatistics(FailoverUnit const& failoverUnit);

            bool ApplicationHasScaleoutOrCapacity(uint64 applicationId) const;
            void UpdateApplicationPartitions(uint64 applicationId, Common::Guid partitionId);
            void RemoveApplicationPartitions(uint64 applicationId, Common::Guid partitionId);

            ApplicationLoad & GetApplicationLoad(uint64 applicationId);
            void ApplicationUpdateLoad(uint64 applicationId, std::wstring const& metricName, int64 load, bool isAdd);

            Service const& GetService(std::wstring const& name) const;
            Service & GetService(std::wstring const& name);

            FailoverUnit const& GetFailoverUnit(Common::Guid const& id) const;
            FailoverUnit & GetFailoverUnit(Common::Guid const& id);

            void VerifyNodeLoads(bool includeDisappearingLoad) const;

            //first element is regular replicas, second is should disappear - only for shared services
            pair<int, int> GetSharedServicePackageNodeCount(uint64 servicePackageId, Federation::NodeId nodeId) const;

            // Application reservation check
            bool CheckReservationViolation(Node const& node, FailoverUnit const& currentFUnit, wstring const& metricName, 
                int64 loadChange, int64 currentNodeLoad, uint currentNodeCapacity) const;

            ReservationLoad & GetReservationLoad(std::wstring const& metricName);

            void UpdateReservationsServicesNoMetric(uint64 appId,
                std::set<std::wstring>& checkedMetrics,
                vector<ReplicaDescription> const& replicas,
                bool isAdd,
                bool decrementNodeCounts);

            DomainId domainId_;

            PlacementAndLoadBalancing & plb_;

            Uint64UnorderedMap<Service> serviceTable_;

            std::map<std::wstring, ServiceDomainMetric> metricTable_;

            // ApplicationName -> MetricName -> Loads!
            std::map<std::wstring, std::map<std::wstring, ServiceDomainMetric>> applicationMetricTable_;

            GuidUnorderedMap<FailoverUnit> failoverUnitTable_;

            MovePlan movePlan_;

            PLBScheduler scheduler_;

            // changed nodes since last refresh
            std::set<uint64> changedNodes_;

            // changed service types since last refresh
            std::set<std::wstring> changedServiceTypes_;

            // changed services since last refresh
            std::set<uint64> changedServices_;

            Common::StopwatchTime lastNodeLoadsTrace_;
            Common::StopwatchTime lastApplicationLoadsTrace_;
            uint64 lastTracedApplication_;
            uint applicationTraceIterationNumber_;


            int64 inTransitionPartitionCount_;
            int64 movableReplicaCount_;
            int64 existingReplicaCount_;
            int64 newReplicaCount_;

            // all failover units with to be created replicas
            std::set<Common::Guid> partitionsWithNewReplicas_;

            // all failover units with extra replicas
            std::set<Common::Guid> partitionsWithExtraReplicas_;

            // all failover units with in upgrade replicas
            std::set<Common::Guid> partitionsWithInUpgradeReplicas_;

            std::set<Common::Guid> partitionsWithInstOnEveryNode_;

            // all changed failover units since the last constraint check
            mutable std::set<Common::Guid> constraintCheckClosure_;

            // when node or service type changed, this is set to true so that all failover units need to be scanned
            mutable bool fullConstraintCheck_;

            // The set of replicas on each node (counts primary, secondary and standBy replicas).
            // Boolean value indicates whether primary (true) or secondary replica load (false) should be used.
            std::map<Federation::NodeId, std::map<Common::Guid, bool>> partitionsPerNode_;

            // Number of InBuild replicas per node
            Uint64UnorderedMap<uint64_t> inBuildCountPerNode_;

            std::map<Federation::NodeId, std::map<uint64, ServicePackageNode>> servicePackageReplicaCountPerNode_;

            std::map<std::wstring, std::set<std::wstring>> childServices_;

            // Partitions for each application with scaleout count; Used when calculate partition closure
            Uint64UnorderedMap<std::set<Common::Guid>> applicationPartitions_;

            // application to load map, for load reservation calculation
            Uint64UnorderedMap<ApplicationLoad> applicationLoadTable_;

            // reservedLoad for certain metrics
            std::map<std::wstring, ReservationLoad> reservationLoadTable_;

            //this represent the count of partitions whose apps are in upgrade
            uint64 partitionsInAppUpgrade_;

			//Does autoscaling
			AutoScaler autoScaler_;

            // Caching for block lists
            std::wstring lastEvaluatedPlacementConstraint_;
            std::wstring lastEvaluatedServiceType_;
            DynamicBitSet lastEvaluatedServiceBlockList_;
            DynamicBitSet lastEvaluatedPrimaryReplicaBlockList_;
            DynamicBitSet lastEvaluatedOverallBlocklist_;
            bool lastEvaluatedPartialPlacement_;
            Service::Type::Enum lastEvaluatedFDDistributionPolicy_;
        };
    }
}
