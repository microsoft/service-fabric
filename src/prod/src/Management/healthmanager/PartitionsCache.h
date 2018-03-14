// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class PartitionsCache 
            : public HealthEntityCache<PartitionHealthId>
        {
            DENY_COPY(PartitionsCache);
            
        public:
            PartitionsCache(
                Common::ComponentRoot const & root,
                __in HealthManagerReplica & healthManagerReplica);

            virtual ~PartitionsCache();
            
            HEALTH_ENTITY_CACHE_TEMPLATED_METHODS_DECLARATIONS(PartitionHealthId, PartitionEntity)
        };
    }
}
