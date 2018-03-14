// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class MovementTask : public DynamicStateMachineTask
        {
            DENY_COPY(MovementTask);

        public:

			MovementTask(
                FailoverManager const& fm,
                INodeCache const& nodeCache,
                Reliability::LoadBalancingComponent::FailoverUnitMovement && movement,
                LoadBalancingComponent::DecisionToken && decisionToken);

            void CheckFailoverUnit(LockedFailoverUnitPtr & failoverUnit, std::vector<StateMachineActionUPtr> & actions);

        private:

            struct ActionType
            {
                enum Enum
                {
                    DropReplica = 0,
                    AddInstance = 1,
                    AddPrimaryReplica = 2,
                    AddReplica = 3,
                    AddReplicaAndPromote = 4,
                    PromoteReplica = 5,
                    ClearFlags = 6,
                    MoveReplica = 7
                };
            };

            typedef std::pair<Federation::NodeId, ActionType::Enum> Action;

            std::vector<Action> GetActions() const;
            std::vector<Action> GetActionsForEmptyFT() const;

            bool AddPrimary(LockedFailoverUnitPtr & failoverUnit, Federation::NodeId nodeId);
            bool AddReplica(LockedFailoverUnitPtr & failoverUnit, Federation::NodeId nodeId, bool isToBePromoted, __out bool & reconfigurationNeeded);
            bool AddInstance(LockedFailoverUnitPtr & failoverUnit, Federation::NodeId nodeId);
            bool DropReplica(LockedFailoverUnitPtr & failoverUnit, Federation::NodeId nodeId);
            bool PromoteReplica(LockedFailoverUnitPtr & failoverUnit, Federation::NodeId nodeId);
            bool MoveReplica(LockedFailoverUnitPtr & failoverUnit, Federation::NodeId nodeId);
            bool ClearFlags(LockedFailoverUnitPtr & failoverUnit, Federation::NodeId nodeId);

            FailoverManager const& fm_;
			INodeCache const& nodeCache_;
            Reliability::LoadBalancingComponent::FailoverUnitMovement movement_;
            LoadBalancingComponent::DecisionToken decisionToken_;
        };
    }
}
