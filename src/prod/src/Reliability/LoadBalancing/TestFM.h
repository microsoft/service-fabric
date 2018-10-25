// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "PlacementAndLoadBalancingPublic.h"
#include "PlacementAndLoadBalancing.h"
#include "PlacementAndLoadBalancingTestHelper.h"
#include "TestUtility.h"
#include "DiagnosticsDataStructures.h"
#include "client/ClientPointers.h"
#include "client/client.h"

namespace PlacementAndLoadBalancingUnitTest
{

    class TestFM : public Common::ComponentRoot, public Reliability::LoadBalancingComponent::IFailoverManager
    {
    public:
        TestFM();

        class DummyHealthReportingTransport : public Client::HealthReportingTransport
        {
        public:
            DummyHealthReportingTransport(Common::ComponentRoot const& root);

            virtual Common::AsyncOperationSPtr BeginReport(
                Transport::MessageUPtr && message,
                Common::TimeSpan timeout,
                Common::AsyncCallback const& callback,
                Common::AsyncOperationSPtr const& parent) override;

            virtual Common::ErrorCode EndReport(
                Common::AsyncOperationSPtr const& operation,
                __out Transport::MessageUPtr & reply) override;
        };

        __declspec (property(get = get_MoveActions)) std::map<Common::Guid, Reliability::LoadBalancingComponent::FailoverUnitMovement> const& MoveActions;
        std::map<Common::Guid, Reliability::LoadBalancingComponent::FailoverUnitMovement> const& get_MoveActions() const { return moveActions_; }

        __declspec (property(get = get_FuMap)) std::map<Common::Guid, Reliability::LoadBalancingComponent::FailoverUnitDescription> & FuMap;
        std::map<Common::Guid, Reliability::LoadBalancingComponent::FailoverUnitDescription> & get_FuMap() { return fuMap_; }

        __declspec (property(get = get_PLB)) Reliability::LoadBalancingComponent::PlacementAndLoadBalancing & PLB;
        Reliability::LoadBalancingComponent::PlacementAndLoadBalancing & get_PLB() const { return *plb_; }

        __declspec(property(get = get_PLBTestHelper)) Reliability::LoadBalancingComponent::PlacementAndLoadBalancingTestHelper & PLBTestHelper;
        Reliability::LoadBalancingComponent::PlacementAndLoadBalancingTestHelper & get_PLBTestHelper() { return *plbTestHelper_; }

        __declspec(property(get = get_NumberOfUpdatesFromPLB)) uint64 const & numberOfUpdatesFromPLB;
        uint64 const & get_NumberOfUpdatesFromPLB() const { return numberOfUpdatesFromPLB_; }

        virtual void ProcessFailoverUnitMovements(std::map<Common::Guid, Reliability::LoadBalancingComponent::FailoverUnitMovement> && moveActions, Reliability::LoadBalancingComponent::DecisionToken && dToken);

        virtual void UpdateAppUpgradePLBSafetyCheckStatus(ServiceModel::ApplicationIdentifier const&);

        virtual void UpdateFailoverUnitTargetReplicaCount(Common::Guid const &, int targetCount);

        bool IsSafeToUpgradeApp(ServiceModel::ApplicationIdentifier const&);

        Client::HealthReportingComponentSPtr CreateDummyHealthReportingComponentSPtr();

        void Load();

        void Load(int initialPLBSeed);

        void Clear();

        void ClearMoveActions();

        void ApplyActions();

        void UpdatePlb();

        void AddReplica(Common::Guid fuId, int count);

        void RefreshPLB(Common::StopwatchTime now);

        int GetTargetReplicaCountForPartition(Common::Guid const& partitionId);

        int GetPartitionCountChangeForService(std::wstring const& serviceName);

    private:

        // Number of times plb calls fm->ProcessFailoverUnitMovements()
        // If batch placement is not used, then this number is equal to the number of service domains with at least one move
        // otherwise, it's equal to the total number of placement batches with at least one move
        uint64 numberOfUpdatesFromPLB_;

        std::vector<Reliability::LoadBalancingComponent::ReplicaDescription>::iterator GetReplicaIterator(
            std::vector<Reliability::LoadBalancingComponent::ReplicaDescription> & replicas,
            Federation::NodeId const& node,
            Reliability::LoadBalancingComponent::ReplicaRole::Enum role);

        void ApplySwapAction(Common::Guid fuId, Federation::NodeId const& sourceNode, Federation::NodeId const& targetNode);

        void ApplyMoveAction(
            Common::Guid fuId,
            Federation::NodeId const& sourceNode,
            Federation::NodeId const& targetNode,
            Reliability::LoadBalancingComponent::ReplicaRole::Enum role);

        void ApplyAddAction(
            Common::Guid fuId,
            Federation::NodeId const& targetNode,
            Reliability::LoadBalancingComponent::ReplicaRole::Enum role);

        void ApplyDropAction(Common::Guid fuId, Federation::NodeId const& targetNode);

        std::set<ServiceModel::ApplicationIdentifier> appUpgradeChecks_;
        std::map<Common::Guid, Reliability::LoadBalancingComponent::FailoverUnitMovement> moveActions_;
        std::map<Common::Guid, Reliability::LoadBalancingComponent::FailoverUnitDescription> fuMap_;
        std::unique_ptr<Reliability::LoadBalancingComponent::PlacementAndLoadBalancing> plb_;
        std::unique_ptr<Reliability::LoadBalancingComponent::PlacementAndLoadBalancingTestHelper> plbTestHelper_;
        std::map<Common::Guid, int> autoscalingTargetCount_;
        std::map<std::wstring, int> autoscalingPartitionCountChange_;
    };
}
