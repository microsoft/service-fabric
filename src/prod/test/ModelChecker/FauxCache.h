// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Reliability/Failover/Failover.h"
#include "Reliability/Failover/FailoverPointers.h"

#include "FMNodeRecord.h"

namespace ModelChecker {
    // A fake cache for the State which, keeps track of all nodes
    // and let search for a node given its NodeId.
    struct FauxCache : Reliability::FailoverManagerComponent::INodeCache
    {
        explicit FauxCache(std::vector<NodeRecord> const&);

        Reliability::FailoverManagerComponent::NodeInfoSPtr GetNode(Federation::NodeId id) const;

        std::vector<NodeRecord> nodeList_;
    };
}
