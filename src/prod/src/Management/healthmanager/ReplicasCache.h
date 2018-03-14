// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class ReplicasCache 
            : public HealthEntityCache<ReplicaHealthId>
        {
            DENY_COPY(ReplicasCache);
            
        public:
            ReplicasCache(
                Common::ComponentRoot const & root,
                __in HealthManagerReplica & healthManagerReplica);

            ~ReplicasCache();

            HEALTH_ENTITY_CACHE_TEMPLATED_METHODS_DECLARATIONS(ReplicaHealthId, ReplicaEntity)
        };
    }
}
