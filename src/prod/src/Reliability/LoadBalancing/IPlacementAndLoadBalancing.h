// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "client/ClientPointers.h"
#include "FailoverUnitMovement.h"
#include "Reliability/Failover/common/PlbMovementIgnoredReasons.h"


namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class FailoverUnitDescription;
        class NodeDescription;
        class ApplicationDescription;
        class ServicePackageDescription;
        class ServiceTypeDescription;
        class ServiceDescription;
        class LoadOrMoveCostDescription;
        class IFailoverManager;

        class IPlacementAndLoadBalancing;
        typedef std::unique_ptr<IPlacementAndLoadBalancing> IPlacementAndLoadBalancingUPtr;

        class IPlacementAndLoadBalancing
        {
        public:
            static IPlacementAndLoadBalancingUPtr Create(
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

            virtual ~IPlacementAndLoadBalancing() = 0 {}

            virtual void SetMovementEnabled(bool constraintCheckEnabled, bool balancingEnabled) = 0;

            virtual void UpdateNode(NodeDescription && nodeDescription) = 0;

            virtual void UpdateServiceType(ServiceTypeDescription && serviceTypeDescription) = 0;
            virtual void DeleteServiceType(std::wstring const& serviceTypeName) = 0;

            virtual Common::ErrorCode UpdateService(ServiceDescription && serviceDescription, bool forceUpdate = false) = 0;
            virtual void DeleteService(std::wstring const& serviceName) = 0;

            virtual void UpdateFailoverUnit(FailoverUnitDescription && failoverUnitDescription) = 0;
            virtual void DeleteFailoverUnit(std::wstring && serviceName, Common::Guid fuId) = 0;

            virtual void UpdateLoadOrMoveCost(LoadOrMoveCostDescription && loadOrMoveCost) = 0;
            virtual Common::ErrorCode ResetPartitionLoad(FailoverUnitId const & failoverUnitId, std::wstring const & serviceName, bool isStateful) = 0;

            virtual Common::ErrorCode UpdateApplication(ApplicationDescription && applicationDescription, bool forceUpdate = false) = 0;
            virtual void DeleteApplication(std::wstring const& applicationName, bool forceUpdate = false) = 0;

            virtual void UpdateClusterUpgrade(bool isUpgradeInProgress, std::set<std::wstring> && completedUDs) = 0;

            virtual Common::ErrorCode ToggleVerboseServicePlacementHealthReporting(bool enabled) = 0;

            virtual Common::ErrorCode GetClusterLoadInformationQueryResult(ServiceModel::ClusterLoadInformationQueryResult & queryResult) = 0;

            virtual Common::ErrorCode GetNodeLoadInformationQueryResult(Federation::NodeId nodeId, ServiceModel::NodeLoadInformationQueryResult & queryResult, bool onlyViolations = false) const = 0;

            virtual ServiceModel::UnplacedReplicaInformationQueryResult GetUnplacedReplicaInformationQueryResult(std::wstring const& serviceName, Common::Guid const& partitionId, bool onlyQueryPrimaries) = 0;

            virtual Common::ErrorCode GetApplicationLoadInformationResult(std::wstring const & applicationName, ServiceModel::ApplicationLoadInformationQueryResult & result) = 0;

            // given a FailoverUnit and two candidate secondary, return the comparision result for promoting to primary
            // ret<0 means node1 is prefered, ret > 0 means node2 is prefered, ret == 0 means they are equal
            virtual int CompareNodeForPromotion(std::wstring const& serviceName, Common::Guid fuId, Federation::NodeId node1, Federation::NodeId node2) = 0;

            // given a FailoverUnit and two candiate secondary, return the comparison result for dropping
            // ret<0 means node1 is prefered for drop, ret > 0 means node2 is prefered for drop, ret == 0 means they are equal
            //virtual int CompareNodeForDrop(std::wstring const& serviceName, Common::Guid fuId, Federation::NodeId node1, Federation::NodeId node2) = 0;

            virtual void Dispose() = 0;
            virtual Common::ErrorCode TriggerPromoteToPrimary(std::wstring const& serviceName, Common::Guid const& fuId, Federation::NodeId newPrimary) = 0;
            virtual Common::ErrorCode TriggerSwapPrimary(std::wstring const& serviceName, Common::Guid const& fuId, Federation::NodeId currentPrimary, Federation::NodeId & newPrimary, bool force = false, bool chooseRandom = false) = 0;
            virtual Common::ErrorCode TriggerMoveSecondary(std::wstring const& serviceName, Common::Guid const& fuId, Federation::NodeId currentSecondary, Federation::NodeId & newSecondary, bool force = false, bool chooseRandom = false) = 0;

            virtual void OnDroppedPLBMovement(Common::Guid const& failoverUnitId, PlbMovementIgnoredReasons::Enum reason, Common::Guid const& decisionId) = 0;
            virtual void OnDroppedPLBMovements(std::map<Common::Guid, FailoverUnitMovement> && droppedMovements, PlbMovementIgnoredReasons::Enum reason, Common::Guid const& decisionId) = 0;
            virtual void OnFMBusy() = 0;
            virtual void OnExecutePLBMovement(Common::Guid const & partitionId) = 0;
            virtual void OnSafetyCheckAcknowledged(ServiceModel::ApplicationIdentifier const & appId) = 0;

            // Function for updating PLB with available images on specified node
            virtual void UpdateAvailableImagesPerNode(std::wstring const& nodeId, std::vector<std::wstring> const& images) = 0;
        };
    }
}
