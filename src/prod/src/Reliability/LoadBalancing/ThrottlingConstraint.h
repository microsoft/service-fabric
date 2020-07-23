// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "IConstraint.h"
#include "PLBSchedulerActionType.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class ThrottlingSubspace : public ISubspace
        {
            DENY_COPY(ThrottlingSubspace);
        public:
            ThrottlingSubspace(bool relaxed);

            virtual void GetTargetNodes(
                TempSolution const& tempSolution,
                PlacementReplica const* replica,
                NodeSet & candidateNodes,
                bool useNodeBufferCapacity,
                NodeToConstraintDiagnosticsDataMapSPtr const nodeToConstraintDiagnosticsDataMapSPtr = nullptr) const;

            virtual IConstraint::Enum get_Type() const { return IConstraint::Throttling; }

            virtual ~ThrottlingSubspace() {}

        private:
            bool relaxed_;
        };

        class ThrottlingConstraint : public IDynamicConstraint
        {
            DENY_COPY(ThrottlingConstraint);
        public:
            ThrottlingConstraint(int priority);

            virtual Enum get_Type() const { return Enum::Throttling; }

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

            // Determines priority of this constraint based on action and on PLB config
            static int GetPriority(PLBSchedulerActionType::Enum action);

            virtual ~ThrottlingConstraint() {}
        private:
            PlacementReplicaSet GetInvalidReplicas(
                TempSolution const& solution,
                bool changedOnly,
                bool relaxed,
                Common::Random& random) const;
        };
    }
}
