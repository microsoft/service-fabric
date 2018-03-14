// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel;
using namespace Federation;
using namespace std;
using namespace Store;
using namespace Management::HealthManager;

NodesCache::NodesCache(
    Common::ComponentRoot const & root,
    __in HealthManagerReplica & healthManagerReplica)
    : HealthEntityCache(root, healthManagerReplica)
{
}

NodesCache::~NodesCache()
{
}

HEALTH_ENTITY_CACHE_ENTITY_TEMPLATED_METHODS_DEFINITIONS(NodesCache, NodeHealthId, NodeEntity, NodeAttributesStoreData)

