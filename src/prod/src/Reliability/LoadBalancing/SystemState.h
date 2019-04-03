// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "ISystemState.h"
#include "Score.h"
#include "PartitionClosure.h"
#include "PLBEventSource.h"
#include "client/ClientPointers.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class ServiceDomain;
        class PartitionClosure;
        typedef std::unique_ptr<PartitionClosure> PartitionClosureUPtr;
        class BalanceChecker;
        typedef std::unique_ptr<BalanceChecker> BalanceCheckerUPtr;
        class Placement;
        typedef std::unique_ptr<Placement> PlacementUPtr;
        class Checker;
        typedef std::unique_ptr<Checker> CheckerUPtr;

        class PLBDiagnostics;
        typedef std::shared_ptr<PLBDiagnostics> PLBDiagnosticsSPtr;

        class SystemState : public ISystemState
        {
            DENY_COPY(SystemState);

        public:
            SystemState(ServiceDomain & serviceDomain, PLBEventSource const& trace, PLBDiagnosticsSPtr const& plbDiagnosticsSPtr);

            SystemState(SystemState && other);

            __declspec (property(get=get_ServiceCount)) size_t ServiceCount;
            size_t get_ServiceCount() const { return serviceCount_; }

            __declspec (property(get=get_FailoverUnitCount)) size_t FailoverUnitCount;
            size_t get_FailoverUnitCount() const { return failoverUnitCount_; }

            __declspec (property(get=get_InTransitionPartitionCount)) size_t InTransitionPartitionCount;
            size_t get_InTransitionPartitionCount() const { return inTransitionPartitionCount_; }

            __declspec (property(get=get_NewReplicaCount)) size_t NewReplicaCount;
            size_t get_NewReplicaCount() const { return newReplicaCount_; }

            __declspec (property(get=get_MovableReplicaCount)) size_t MovableReplicaCount;
            size_t get_MovableReplicaCount() const { return movableReplicaCount_; }

            __declspec (property(get=get_PartitionsWithNewReplicaCount)) size_t PartitionsWithNewReplicaCount;
            size_t get_PartitionsWithNewReplicaCount() const { return partitionsWithNewReplicaCount_; }

            __declspec (property(get=get_PartitionsWithInstOnEveryNodeCount)) size_t PartitionsWithInstOnEveryNodeCount;
            size_t get_PartitionsWithInstOnEveryNodeCount() const { return partitionsWithInstOnEveryNodeCount_; }

            __declspec (property(get=get_Placement)) PlacementUPtr const& PlacementObj;
            PlacementUPtr const& get_Placement() const { return placement_; }

            __declspec (property(get=get_Checker)) CheckerUPtr const& CheckerObj;
            CheckerUPtr const& get_Checker() const { return checker_; }

            virtual bool HasNewReplica() const;
            //this returns true if the partition is in upgrade and has a replica in upgrade
            virtual bool HasPartitionWithReplicaInUpgrade() const;
            virtual bool HasMovableReplica() const;
            virtual bool HasExtraReplicas() const;
            //this returns true if the partition is in upgrade, nevermind if there is a replica in upgrade
            virtual bool HasPartitionInAppUpgrade() const;

            virtual bool IsConstraintSatisfied(__out std::vector<std::shared_ptr<IConstraintDiagnosticsData>> & constraintsDiagnosticsData) const;
            virtual bool IsBalanced() const;
            virtual std::set<Common::Guid> GetImbalancedFailoverUnits() const;
            virtual double GetAvgStdDev() const;
            virtual size_t ExistingReplicaCount() const { return existingReplicaCount_; }
            virtual size_t BalancingMovementCount() const;
            virtual size_t PlacementMovementCount() const;

            // Creates placement and checker based on closure type.
            void CreatePlacementAndChecker(PartitionClosureType::Enum closureType) const;
            // Creates placement and checker based on scheduler action (checker may be different for different actions)
            void CreatePlacementAndChecker(PLBSchedulerActionType::Enum action) const;

            ServiceDomain & serviceDomain_;

            virtual ~SystemState();

        private:
            BalanceCheckerUPtr const& GetBalanceChecker(PartitionClosureType::Enum closureType) const;
            PlacementUPtr const& GetPlacement(PartitionClosureType::Enum closureType) const;

            void ClearAll() const;

            size_t serviceCount_;
            size_t failoverUnitCount_;
            size_t inTransitionPartitionCount_;
            size_t newReplicaCount_;
            size_t existingReplicaCount_;
            size_t movableReplicaCount_;
            size_t partitionsWithExtraReplicaCount_;
            size_t upgradePartitionCount_;
            size_t partitionsWithNewReplicaCount_;
            size_t partitionsWithInstOnEveryNodeCount_;

            PLBEventSource const& trace_;
            PLBDiagnosticsSPtr plbDiagnosticsSPtr_;

            mutable PartitionClosureUPtr partitionClosure_;
            mutable BalanceCheckerUPtr balanceChecker_;
            mutable PlacementUPtr placement_;
            mutable CheckerUPtr checker_;

            mutable double avgStdDev_;
        };
    }
}
