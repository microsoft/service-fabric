// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "FauxCache.h"

using namespace ModelChecker;

FauxCache::FauxCache(std::vector<NodeRecord> const& nodeList)
    : nodeList_( nodeList )
{
}

Reliability::FailoverManagerComponent::NodeInfoSPtr FauxCache::GetNode(Federation::NodeId id) const 
{
    auto it = find_if(nodeList_.begin(), nodeList_.end(), 
        [=] (NodeRecord const& nodeRecord) 
        {
            return (nodeRecord.NodeInformation.NodeInstance.Id == id);
        });
    
    return std::make_shared<Reliability::FailoverManagerComponent::NodeInfo>(it->NodeInformation);
}
