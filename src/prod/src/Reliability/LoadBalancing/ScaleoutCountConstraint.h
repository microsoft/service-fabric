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
        class ApplicationEntry;

        class ScaleoutCountSubspace : public ISubspace
        {
            DENY_COPY(ScaleoutCountSubspace);
        public:
            ScaleoutCountSubspace(std::map<ApplicationEntry const*, std::set<NodeEntry const*>> && applicationNodes, bool relaxed);

            virtual void GetTargetNodes(
                TempSolution const& tempSolution,
                PlacementReplica const* replica,
                NodeSet & candidateNodes,
                bool useNodeBufferCapacity,
				NodeToConstraintDiagnosticsDataMapSPtr const nodeToConstraintDiagnosticsDataMapSPtr = nullptr) const;

            virtual IConstraint::Enum get_Type() const { return IConstraint::ScaleoutCount; }

            virtual ~ScaleoutCountSubspace() {}

        private:
            std::map<ApplicationEntry const*, std::set<NodeEntry const *>> applicationNodes_;
            bool relaxed_;
        };

        class ScaleoutCountConstraint : public IDynamicConstraint
        {
            DENY_COPY(ScaleoutCountConstraint);
        public:
            ScaleoutCountConstraint(int priority);

            virtual Enum get_Type() const { return Enum::ScaleoutCount; }

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

            virtual ~ScaleoutCountConstraint(){}

        private:
            PlacementReplicaSet GetInvalidReplicas(
                TempSolution const& tempSolution,
                bool changedOnly,
                bool relaxed,
                Common::Random & random,
                std::map<ApplicationEntry const*, std::set<NodeEntry const *>> & appNodes,
                std::shared_ptr<IConstraintDiagnosticsData> const diagnosticsDataPtr = nullptr) const;
        };
    }
}
