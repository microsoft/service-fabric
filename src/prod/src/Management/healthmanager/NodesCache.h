// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class NodesCache 
            : public HealthEntityCache<NodeHealthId>
        {
            DENY_COPY(NodesCache);
            
        public:
            NodesCache(
                Common::ComponentRoot const & root,
                __in HealthManagerReplica & healthManagerReplica);

            ~NodesCache();

            HEALTH_ENTITY_CACHE_TEMPLATED_METHODS_DECLARATIONS(NodeHealthId, NodeEntity)
        };
    }
}
