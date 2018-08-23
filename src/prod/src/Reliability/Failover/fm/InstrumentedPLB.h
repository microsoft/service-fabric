// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class InstrumentedPLB : public Reliability::LoadBalancingComponent::IPlacementAndLoadBalancing
        {
            DENY_COPY(InstrumentedPLB);

        public:

            static InstrumentedPLBUPtr CreateInstrumentedPLB(
                FailoverManager const& fm,
                Reliability::LoadBalancingComponent::IPlacementAndLoadBalancingUPtr && plb);

            InstrumentedPLB(
                FailoverManager const& fm,
                Reliability::LoadBalancingComponent::IPlacementAndLoadBalancingUPtr && plb);

            virtual ~InstrumentedPLB();

            __declspec (property(get = get_IPlacementAndLoadBalancing)) LoadBalancingComponent::IPlacementAndLoadBalancing & PLB;
            LoadBalancingComponent::IPlacementAndLoadBalancing & get_IPlacementAndLoadBalancing() const { return *plb_; }

            virtual void SetMovementEnabled(bool constraintCheckEnabled, bool balancingEnabled);                        
            virtual void UpdateNode(LoadBalancingComponent::NodeDescription && nodeDescription, __out int64 & plbElapsedMilliseconds);
            virtual void UpdateServiceType(LoadBalancingComponent::ServiceTypeDescription && serviceTypeDescription, __out int64 & plbElapsedMilliseconds);
            virtual void DeleteServiceType(std::wstring const& serviceTypeName, __out int64 & plbElapsedMilliseconds);                        
            virtual Common::ErrorCode UpdateApplication(LoadBalancingComponent::ApplicationDescription && applicationDescription, __out int64 & plbElapsedMilliseconds, bool forceUpdate = false);
            virtual void DeleteApplication(std::wstring const& applicationName, __out int64 & plbElapsedMilliseconds, bool forceUpdate = false);
            virtual void UpdateClusterUpgrade(bool isUpgradeInProgress, std::set<std::wstring> && completedUDs);
            virtual Common::ErrorCode UpdateService(LoadBalancingComponent::ServiceDescription && serviceDescription, __out int64 & plbElapsedMilliseconds, bool forceUpdate = false);
            virtual void DeleteService(std::wstring const& serviceName, __out int64 & plbElapsedMilliseconds);
            virtual void UpdateFailoverUnit(LoadBalancingComponent::FailoverUnitDescription && failoverUnitDescription, __out int64 & plbElapsedMilliseconds);
            virtual void DeleteFailoverUnit(std::wstring && serviceName, Common::Guid failoverUnitId, __out int64 & plbElapsedMilliseconds);
            virtual void UpdateLoadOrMoveCost(LoadBalancingComponent::LoadOrMoveCostDescription && loadOrMoveCost);
            virtual Common::ErrorCode ResetPartitionLoad(FailoverUnitId const & failoverUnitId, std::wstring const & serviceName, bool isStateful);

            //Query APIs
            virtual Common::ErrorCode GetClusterLoadInformationQueryResult(ServiceModel::ClusterLoadInformationQueryResult & queryResult);
            virtual Common::ErrorCode GetNodeLoadInformationQueryResult(Federation::NodeId nodeId, ServiceModel::NodeLoadInformationQueryResult & queryResult, bool onlyViolations = false) const;

            virtual ServiceModel::UnplacedReplicaInformationQueryResult GetUnplacedReplicaInformationQueryResult(std::wstring const& serviceName, Common::Guid const& partitionId, bool onlyQueryPrimaries);

            virtual Common::ErrorCode GetApplicationLoadInformationResult(std::wstring const & applicationName, ServiceModel::ApplicationLoadInformationQueryResult & result);

            // given a FailoverUnit and two candidate secondary, return the comparision result for promoting to primary
            // ret<0 means node1 is prefered, ret > 0 means node2 is prefered, ret == 0 means they are equal
            virtual int CompareNodeForPromotion(std::wstring const& serviceName, Common::Guid failoverUnitId, Federation::NodeId node1, Federation::NodeId node2);

            // given a FailoverUnit and two candiate secondary, return the comparison result for dropping
            // ret<0 means node1 is prefered for drop, ret > 0 means node2 is prefered for drop, ret == 0 means they are equal
            //virtual int CompareNodeForDrop(std::wstring const& serviceName, Common::Guid failoverUnitId, Federation::NodeId node1, Federation::NodeId node2) = 0;

            virtual void Dispose();

            virtual Common::ErrorCode TriggerPromoteToPrimary(std::wstring const& serviceName, Common::Guid const& failoverUnitId, Federation::NodeId newPrimary);
            virtual Common::ErrorCode TriggerSwapPrimary(std::wstring const& serviceName, Common::Guid const& failoverUnitId, Federation::NodeId currentPrimary, Federation::NodeId & newPrimary, bool force = false, bool chooseRandom = false);
            virtual Common::ErrorCode TriggerMoveSecondary(std::wstring const& serviceName, Common::Guid const& failoverUnitId, Federation::NodeId currentSecondary, Federation::NodeId & newSecondary, bool force = false, bool chooseRandom = false);

            virtual void OnDroppedPLBMovement(Common::Guid const& failoverUnitId, PlbMovementIgnoredReasons::Enum reason, Common::Guid const& decisionId);
            virtual void OnDroppedPLBMovements(std::map<Common::Guid, LoadBalancingComponent::FailoverUnitMovement> && droppedMovements, PlbMovementIgnoredReasons::Enum reason, Common::Guid const& decisionId);
            virtual void OnExecutePLBMovement(Common::Guid const & partitionId);
            virtual void OnFMBusy();
            virtual void OnSafetyCheckAcknowledged(ServiceModel::ApplicationIdentifier const & appId);

            virtual Common::ErrorCode ToggleVerboseServicePlacementHealthReporting(bool enabled);

            virtual void UpdateAvailableImagesPerNode(std::wstring const& nodeId, std::vector<std::wstring> const& images);

        private:
            virtual void UpdateNode(LoadBalancingComponent::NodeDescription && nodeDescription);
            virtual void UpdateServiceType(LoadBalancingComponent::ServiceTypeDescription && serviceTypeDescription);
            virtual void DeleteServiceType(std::wstring const& serviceTypeName);
            virtual Common::ErrorCode UpdateApplication(LoadBalancingComponent::ApplicationDescription && applicationDescription, bool forceUpdate = false);
            virtual void DeleteApplication(std::wstring const& applicationName, bool forceUpdate = false);
            virtual Common::ErrorCode UpdateService(LoadBalancingComponent::ServiceDescription && serviceDescription, bool forceUpdate = false);
            virtual void DeleteService(std::wstring const& serviceName);
            virtual void UpdateFailoverUnit(LoadBalancingComponent::FailoverUnitDescription && failoverUnitDescription);
            virtual void DeleteFailoverUnit(std::wstring && serviceName, Common::Guid failoverUnitId);

            FailoverManager const& fm_;
            LoadBalancingComponent::IPlacementAndLoadBalancingUPtr plb_;
        };
    }
}
