// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class IConstraintDiagnosticsData;

        class ISystemState
        {
        public:
            virtual bool HasNewReplica() const = 0;
            virtual bool HasMovableReplica() const = 0;
            virtual bool HasPartitionWithReplicaInUpgrade() const = 0;
            virtual bool HasExtraReplicas() const = 0;
            virtual bool HasPartitionInAppUpgrade() const = 0;
            virtual bool IsConstraintSatisfied(__out std::vector<std::shared_ptr<IConstraintDiagnosticsData>> & constraintsDiagnosticsData) const = 0;
            virtual bool IsBalanced() const = 0;
            virtual double GetAvgStdDev() const = 0;
            virtual std::set<Common::Guid> GetImbalancedFailoverUnits() const = 0;
            virtual size_t BalancingMovementCount() const = 0;
            virtual size_t PlacementMovementCount() const = 0;
            virtual size_t ExistingReplicaCount() const = 0;
            virtual ~ISystemState() = 0 { }
        };
    }
}
