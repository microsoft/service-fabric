// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "client/ClientPointers.h"
#include "FailoverUnitMovement.h"
#include "IPlacementAndLoadBalancing.h"
#include "ReplicaRole.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class FailoverUnitDescription;
        class NodeDescription;
        class ApplicationDescription;
        class ServiceTypeDescription;
        class ServiceDescription;
        class LoadOrMoveCostDescription;
        class IFailoverManager;
        class PlacementAndLoadBalancing;
        class ServiceDomain;

        class PlacementAndLoadBalancingTestHelper;
        typedef std::unique_ptr<PlacementAndLoadBalancingTestHelper> PlacementAndLoadBalancingTestHelperUPtr;

        class PlacementAndLoadBalancingTestHelper : public IPlacementAndLoadBalancing
        {

            DENY_COPY(PlacementAndLoadBalancingTestHelper);

        public:

            PlacementAndLoadBalancingTestHelper(IPlacementAndLoadBalancing & plb);

            virtual ~PlacementAndLoadBalancingTestHelper() {}

            /******************************************************************
             *             IPlacementAndLoadBalancing methods                 *
             ******************************************************************/

            virtual void SetMovementEnabled(bool constraintCheckEnabled, bool balancingEnabled);

            virtual void UpdateNode(NodeDescription && nodeDescription);

            virtual void UpdateServiceType(ServiceTypeDescription && serviceTypeDescription);
            virtual void DeleteServiceType(std::wstring const& serviceTypeName);

            virtual void UpdateClusterUpgrade(bool isUpgradeInProgress, std::set<std::wstring> && completedUDs);

            virtual Common::ErrorCode UpdateService(ServiceDescription && serviceDescription, bool forceUpdate = false);
            virtual void DeleteService(std::wstring const& serviceName);

            virtual void UpdateFailoverUnit(FailoverUnitDescription && failoverUnitDescription);
            virtual void DeleteFailoverUnit(std::wstring && serviceName, Common::Guid fuId);

            virtual void UpdateLoadOrMoveCost(LoadOrMoveCostDescription && loadOrMoveCost);
            virtual Common::ErrorCode ResetPartitionLoad(FailoverUnitId const & failoverUnitId, std::wstring const & serviceName, bool isStateful);

            virtual Common::ErrorCode UpdateApplication(ApplicationDescription && applicationDescription, bool forceUpdate = false);
            virtual void DeleteApplication(std::wstring const& applicationName, bool forceUpdate = false);

            virtual Common::ErrorCode ToggleVerboseServicePlacementHealthReporting(bool enabled);

            virtual Common::ErrorCode GetClusterLoadInformationQueryResult(ServiceModel::ClusterLoadInformationQueryResult & queryResult);

            virtual Common::ErrorCode GetNodeLoadInformationQueryResult(Federation::NodeId nodeId, ServiceModel::NodeLoadInformationQueryResult & queryResult, bool onlyViolations = false) const;

            virtual ServiceModel::UnplacedReplicaInformationQueryResult GetUnplacedReplicaInformationQueryResult(std::wstring const& serviceName, Common::Guid const& partitionId, bool onlyQueryPrimaries);

            virtual Common::ErrorCode GetApplicationLoadInformationResult(std::wstring const & applicationName, ServiceModel::ApplicationLoadInformationQueryResult & result);

            virtual int CompareNodeForPromotion(std::wstring const& serviceName, Common::Guid fuId, Federation::NodeId node1, Federation::NodeId node2);

            virtual void Dispose();

            virtual Common::ErrorCode TriggerPromoteToPrimary(std::wstring const& serviceName, Common::Guid const& fuId, Federation::NodeId newPrimary);
            virtual Common::ErrorCode TriggerSwapPrimary(std::wstring const& serviceName, Common::Guid const& fuId, Federation::NodeId currentPrimary, Federation::NodeId & newPrimary, bool force = false, bool chooseRandom = false);
            virtual Common::ErrorCode TriggerMoveSecondary(std::wstring const& serviceName, Common::Guid const& fuId, Federation::NodeId currentSecondary, Federation::NodeId & newSecondary, bool force = false, bool chooseRandom = false);

            virtual void OnDroppedPLBMovement(Common::Guid const& failoverUnitId, PlbMovementIgnoredReasons::Enum reason, Common::Guid const& decisionId);
            virtual void OnDroppedPLBMovements(std::map<Common::Guid, FailoverUnitMovement> && droppedMovements, Reliability::PlbMovementIgnoredReasons::Enum reason, Common::Guid const& decisionId);
            virtual void OnExecutePLBMovement(Common::Guid const & partitionId);
            virtual void OnFMBusy();
            virtual void OnSafetyCheckAcknowledged(ServiceModel::ApplicationIdentifier const & appId);

            void ProcessPendingUpdatesPeriodicTask();

            void RefreshServicePackageForRG(bool & hasServicePackage, bool & hasApp);
            void Refresh(Common::StopwatchTime now);

            virtual void UpdateAvailableImagesPerNode(std::wstring const& nodeId, std::vector<std::wstring> const& images);

            /******************************************************************
             *                             Test Helper Methods                 *
             ******************************************************************/

            __declspec(property(get = get_PLB)) PlacementAndLoadBalancing & PLB;
            PlacementAndLoadBalancing & get_PLB() { return plb_; }

            bool CheckLoadReport(std::wstring const& serviceName, Common::Guid fuId, int numberOfReports);

            // Check secondary load map if exists, otherwise, return the secondary entry (default load)
            // If callGetSecondaryLoad is true, return by call GetSecondaryLoad
            bool CheckLoadValue(
                std::wstring const& serviceName,
                Common::Guid fuId,
                std::wstring const& metricName,
                ReplicaRole::Enum role,
                uint loadValue,
                Federation::NodeId const& nodeId,
                bool callGetSecondaryLoad = false);

            bool CheckMoveCostValue(std::wstring const& serviceName, Common::Guid fuId, ReplicaRole::Enum role, uint moveCostValue);

            void GetReservedLoad(std::wstring const& metricName, int64 & reservedCapacity, int64 & reservedLoadUsed) const;

            void Test_GetServicePackageReplicaCountForGivenNode(
                ServiceModel::ServicePackageIdentifier const& servicePackageIdentifier,
                int& exclusiveRegularReplicaCount,
                int& sharedRegularReplicaCount,
                int& sharedDisappearReplicaCount,
                Federation::NodeId nodeId) const;

            void GetReservedLoadNode(wstring const& metricName, int64 & reservedLoadUsed, Federation::NodeId) const;

            // helper function for getting load - used to merge the logic for nonAppGroup and appGroup applications
            void GetApplicationSumLoadAndCapacityHelper(
                ServiceDomain const& serviceDomain,
                uint64 appId,
                wstring const& metricName,
                int64 & appSumLoad,
                int64& size) const;

            void GetApplicationSumLoadAndCapacity(
                wstring const& appName, 
                wstring const& metricName, 
                int64 & reservedCapacity, 
                int64& size) const;

            bool CheckAutoScalingStatistics(std::wstring const & metricName, int partitionCount) const;
            // Verifies that numbers related to Service Packages are correct in RG statistics.
            bool CheckRGSPStatistics(uint64_t numSPs, uint64_t numGovernedSPs) const;

            // Verifies that numbers related to service package activation are correct in RG statistics.
            bool CheckRGServiceStatistics(uint64_t numShared, uint64_t numExclusive) const;

            // Verifies that loads are correct in RG statistics
            bool CheckRGMetricStatistics(std::wstring const& name,
                double usage,
                double minUsage,
                double maxUsage,
                int64 capacity) const;

            bool CheckDefragStatistics(
                uint64 numberOfBalancingMetrics,
                uint64 numberOfBalancingReservationMetrics,
                uint64 numberOfReservationMetrics,
                uint64 numberOfPackReservationMetrics,
                uint64 numberOfDefragMetrics) const;

            // Retreives partition count change for auto scaling
            int GetPartitionCountChangeForService(std::wstring const & serviceName) const;

            // Clear pending repartitions
            void ClearPendingRepartitions();

            void InduceRepartitioningFailure(std::wstring const& serviceName, Common::ErrorCode error);

            int64 GetInBuildCountPerNode(int nodeId, std::wstring metricName = L"");

            uint64 RefreshTime;

        private:
            // The actual PLB object
            PlacementAndLoadBalancing & plb_;

            void ResetTiming();
        };
    }
}
