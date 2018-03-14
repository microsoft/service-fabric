// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "IConstraint.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        /////////////////////////////////
        // classes for replica exclusion
        // (two replicas of the same partition cannot be placed on the same node)
        /////////////////////////////////

        class ReplicaExclusionStaticConstraint : public IStaticConstraint
        {
            DENY_COPY(ReplicaExclusionStaticConstraint);
        public:
            ReplicaExclusionStaticConstraint(int priority)
                : IStaticConstraint(priority)
            {
            }

            virtual Enum get_Type() const { return Enum::ReplicaExclusionStatic; }

            virtual void GetTargetNodes(
                TempSolution const& tempSolution,
                PlacementReplica const* replica,
                NodeSet & candidateNodes,
                NodeToConstraintDiagnosticsDataMapSPtr const nodeToConstraintDiagnosticsDataMapSPtr = nullptr) const;

            virtual bool CheckReplica(TempSolution const& tempSolution, PlacementReplica const* replica, NodeEntry const* target) const;

            virtual ~ReplicaExclusionStaticConstraint() {}
        };

        class ReplicaExclusionDynamicSubspace : public ISubspace
        {
            DENY_COPY(ReplicaExclusionDynamicSubspace);
        public:
            ReplicaExclusionDynamicSubspace();

            virtual void GetTargetNodes(
                TempSolution const& tempSolution,
                PlacementReplica const* replica,
                NodeSet & candidateNodes,
                bool useNodeBufferCapacity,
                NodeToConstraintDiagnosticsDataMapSPtr const nodeToConstraintDiagnosticsDataMapSPtr = nullptr) const;

            virtual IConstraint::Enum get_Type() const { return IConstraint::ReplicaExclusionDynamic; }

            virtual ~ReplicaExclusionDynamicSubspace() {}
        };

        class ReplicaExclusionDynamicConstraint : public IDynamicConstraint
        {
            DENY_COPY(ReplicaExclusionDynamicConstraint);
        public:
            ReplicaExclusionDynamicConstraint(int priority)
                : IDynamicConstraint(priority)
            {
            }

            virtual Enum get_Type() const { return Enum::ReplicaExclusionDynamic; }

            virtual IViolationUPtr GetViolations(
                TempSolution const& solution,
                bool changedOnly,
                bool relaxed,
                bool useNodeBufferCapacity,
                Common::Random& random) const;

            virtual ConstraintCheckResult CheckSolutionAndGenerateSubspace(
                TempSolution const& tempSolution,
                bool changedOnly,
                bool relaxed,
                bool useNodeBufferCapacity,
                Common::Random & random,
                std::shared_ptr<IConstraintDiagnosticsData> const diagnosticsDataPtr = nullptr) const;

            virtual ~ReplicaExclusionDynamicConstraint() {}
        };

    }
}
