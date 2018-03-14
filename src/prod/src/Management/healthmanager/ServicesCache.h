// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class ServicesCache 
            : public HealthEntityCache<ServiceHealthId>
        {
            DENY_COPY(ServicesCache);
            
        public:
            ServicesCache(
                Common::ComponentRoot const & root,
                __in HealthManagerReplica & healthManagerReplica);

            virtual ~ServicesCache();

            HEALTH_ENTITY_CACHE_TEMPLATED_METHODS_DECLARATIONS(ServiceHealthId, ServiceEntity)
        };
    }
}
