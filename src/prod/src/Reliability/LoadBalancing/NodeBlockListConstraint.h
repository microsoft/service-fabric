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
        // classes for node blocklist
        // replicas of a service should not be placed on certain nodes
        /////////////////////////////////
        class NodeBlockListConstraint : public IStaticConstraint
        {
            DENY_COPY(NodeBlockListConstraint);
        public:
            NodeBlockListConstraint(int priority)
                : IStaticConstraint(priority)
            {
            }

            static void ExcludeServiceBlockList(NodeSet & candidateNodes, PlacementReplica const * replica);

            virtual Enum get_Type() const { return Enum::PlacementConstraint; }

            virtual void GetTargetNodes(
                TempSolution const& tempSolution,
                PlacementReplica const* replica,
                NodeSet & candidateNodes,
                NodeToConstraintDiagnosticsDataMapSPtr const nodeToConstraintDiagnosticsDataMapSPtr = nullptr) const;

            virtual bool CheckReplica(TempSolution const& tempSolution, PlacementReplica const* replica, NodeEntry const* target) const;

            virtual ~NodeBlockListConstraint() {}
        };
    }
}
